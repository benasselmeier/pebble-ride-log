#include <pebble.h>
#include "config.h"

// Platform-specific optimizations for memory usage
#ifdef PBL_PLATFORM_APLITE
  // Aplite has limited RAM - reduce data structures
  #define MAX_STORED_RIDES 10
  #define MAX_CUSTOM_COASTERS_PER_PARK 8
  #define MAX_CUSTOM_PARKS 3
  #define PARK_NAME_SIZE 24
  #define COASTER_NAME_SIZE 40
  #define COASTER_TYPE_SIZE 20
#else
  // Other platforms have more RAM
  #define MAX_STORED_RIDES 20
  #define MAX_CUSTOM_COASTERS_PER_PARK 20
  #define MAX_CUSTOM_PARKS 10
  #define PARK_NAME_SIZE 32
  #define COASTER_NAME_SIZE 50
  #define COASTER_TYPE_SIZE 30
#endif

// Constants for accelerometer
#define ACCEL_SAMPLES_PER_UPDATE 25
#define ACCEL_SAMPLES_PEBBLE2 10  // Fewer samples for Pebble 2 to reduce sensor conflicts

// Menu states
typedef enum {
  MENU_STATE_MAIN,
  MENU_STATE_PARK_SELECTION,
  MENU_STATE_COASTER_SELECTION,
  MENU_STATE_RECORDING
} MenuState;

// Recording states
typedef enum {
  RECORDING_STOPPED,
  RECORDING_RUNNING,
  RECORDING_PAUSED
} RecordingState;

// Ride recording data structure
typedef struct {
  char park_name[PARK_NAME_SIZE];
  char coaster_name[PARK_NAME_SIZE];
  int duration_seconds;
  int max_g_force_hundredths; // G-force * 100 (e.g., 150 = 1.50g)
  int min_g_force_hundredths; // G-force * 100 (e.g., 80 = 0.80g)
  time_t timestamp;
} RideRecord;

// Configuration settings received from phone
typedef struct {
  int recording_mode;       // 0=manual, 1=auto, 2=quick
  int g_force_threshold;    // Alert threshold in hundredths of g
  bool vibrate_on_peaks;    // Vibrate on G-force peaks
  bool sound_alerts;        // Sound alerts (if supported)
  int time_format;          // 0=MM:SS, 1=MM:SS.CC, 2=SS.CC
  int g_force_units;        // 0=g, 1=m/s², 2=ft/s²
  bool show_current_g;      // Show real-time G-force
  int max_stored_rides;     // Maximum rides to store
  bool auto_save;           // Auto-save recordings
} AppSettings;

static AppSettings app_settings = {
  .recording_mode = 0,      // Manual by default
  .g_force_threshold = 150, // 1.50g
  .vibrate_on_peaks = true,
  .sound_alerts = false,
  .time_format = 1,         // MM:SS.CC
  .g_force_units = 0,       // G-forces
  .show_current_g = true,
  .max_stored_rides = 20,
  .auto_save = true
};

// Data storage keys
#define PERSIST_KEY_RIDE_COUNT 1
#define PERSIST_KEY_RIDE_BASE 10

// Coaster structure
typedef struct {
  char name[COASTER_NAME_SIZE];
  int id;
  char type[COASTER_TYPE_SIZE]; // e.g., "Steel", "Wood", "Hybrid"
} Coaster;

typedef struct {
  char park_id[PARK_NAME_SIZE];
  int coaster_count;
  Coaster coasters[MAX_CUSTOM_COASTERS_PER_PARK];
} CustomParkData;

static CustomParkData custom_parks[MAX_CUSTOM_PARKS];
static int custom_park_count = 0;

// Windows and layers
static Window *main_window;
static Window *park_window;
static Window *coaster_window;
static Window *recording_window;
static MenuLayer *main_menu_layer;
static MenuLayer *park_menu_layer;
static MenuLayer *coaster_menu_layer;

// Recording UI layers
static TextLayer *coaster_name_layer;
static TextLayer *elapsed_time_layer;
static TextLayer *max_g_layer;
static TextLayer *min_g_layer;
static TextLayer *current_g_layer;
static TextLayer *start_button_layer;
static TextLayer *up_button_layer;
static TextLayer *down_button_layer;

// Menu items
#define NUM_MENU_SECTIONS 1
#define NUM_MAIN_MENU_ITEMS 5

// Main menu item titles
static char* main_menu_items[] = {
  "New Ride",
  "Set Current Park",
  "Ride History", 
  "Options",
  "Quick Start"
};

// US Theme Parks organized by company
typedef struct {
  char name[50];
  int id;
} Park;

// Six Flags St. Louis Coasters (manually curated)
#ifdef PBL_PLATFORM_APLITE
// Reduced coaster set for Aplite to save memory
static Coaster sixflags_stlouis_coasters[] = {
  {"American Thunder", 0, "Wood"},
  {"Batman the Ride", 0, "Steel"},
  {"Boss", 0, "Wood"},
  {"Mr. Freeze Reverse Blast", 0, "Steel"},
  {"Ninja", 0, "Steel"},
  {"Screamin' Eagle", 0, "Wood"}
};
#else
// Full coaster set for other platforms
static Coaster sixflags_stlouis_coasters[] = {
  {"American Thunder", 0, "Wood"},
  {"Batman the Ride", 0, "Steel"},
  {"Boomerang", 0, "Steel"},
  {"Boss", 0, "Wood"},
  {"Mr. Freeze Reverse Blast", 0, "Steel"},
  {"Ninja", 0, "Steel"},
  {"Pandemonium", 0, "Steel"},
  {"River King Mine Train", 0, "Steel"},
  {"Rookie Racer", 0, "Steel"},
  {"Screamin' Eagle", 0, "Wood"},
};
#endif

// Six Flags Over Georgia Coasters (for Atlanta trip)
#ifdef PBL_PLATFORM_APLITE
static Coaster sixflags_georgia_coasters[] = {
  {"Batman the Ride", 0, "Steel"},
  {"Blue Hawk", 0, "Steel"},
  {"Dare Devil Dive", 0, "Steel"},
  {"Georgia Scorcher", 0, "Steel"},
  {"Goliath", 0, "Steel"},
  {"Great American Scream Machine", 0, "Wood"},
  {"Riddler Mindbender", 0, "Steel"}
};
#else
static Coaster sixflags_georgia_coasters[] = {
  {"Batman the Ride", 0, "Steel"},
  {"Blue Hawk", 0, "Steel"},
  {"Dahlonega Mine Train", 0, "Steel"},
  {"Dare Devil Dive", 0, "Steel"},
  {"Georgia Goldrusher", 0, "Steel"},
  {"Georgia Scorcher", 0, "Steel"},
  {"Goliath", 0, "Steel"},
  {"Great American Scream Machine", 0, "Wood"},
  {"Joker Funhouse Coaster", 0, "Steel"},
  {"Riddler Mindbender", 0, "Steel"},
  {"Superman - Ultimate Flight", 0, "Steel"},
  {"Twisted Cyclone", 0, "Steel"},
};
#endif

// St. Louis Union Station Coasters
static Coaster union_station_coasters[] = {
  {"Loco Motion", 0, "Steel"},
};

// Currently selected park and its coasters
static Park *current_park = NULL;
static Coaster *current_coasters = NULL;
static int current_coaster_count = 0;

// Six Flags Legacy Parks
static Park sixflags_parks[] = {
  {"Six Flags Magic Mountain", 0},
  {"Six Flags Great Adventure", 0},
  {"Six Flags Fiesta Texas", 5},
  {"Six Flags Over Texas", 0},
  {"Six Flags Great America", 0},
  {"Six Flags America", 102},
  {"Six Flags St. Louis", 107},
  {"Six Flags Discovery Kingdom", 118},
  {"Six Flags Great Escape", 110},
  {"Six Flags Over Georgia", 0},
  {"Six Flags New England", 0},
  {"Six Flags Great Escape", 0},
  {"Frontier City", 0},
  {"La Ronde", 0},
  {"Six Flags Mexico", 0},
  {"Six Flags Darien Lake", 0},

};

// Cedar Fair Legacy Parks  
static Park cedarfair_parks[] = {
  {"Cedar Point", 0},
  {"Kings Island", 0},
  {"Canada's Wonderland", 0},
  {"Carowinds", 0},
  {"Kings Dominion", 0},
  {"Knott's Berry Farm", 0},
  {"Michigan's Adventure", 109},
  {"Valleyfair", 0},
  {"Worlds of Fun", 0},
  {"Dorney Park", 0},
  {"California's Great America", 0}
};

// Herschend Family Entertainment
static Park herschend_parks[] = {
  {"Silver Dollar City", 0},
  {"Dollywood", 0},
  {"Wild Adventures", 0},
  {"Kennywood", 0},
  {"Lake Compounce", 0},
  {"Adventureland Iowa", 114}
};

// Disney Parks
static Park disney_parks[] = {
  {"Magic Kingdom", 0},
  {"EPCOT", 0},
  {"Disney's Hollywood Studios", 0},
  {"Disney's Animal Kingdom", 0},
  {"Disneyland", 0},
  {"Disney California Adventure", 0}
};

// United Parks & Resorts (SeaWorld & Busch Gardens)
static Park unitedparks_parks[] = {
  {"Busch Gardens Williamsburg", 108},
  {"Busch Gardens Tampa Bay", 0},
  {"SeaWorld Orlando", 0},
  {"SeaWorld San Diego", 0}
};

// Koch Family Parks
static Park koch_parks[] = {
  {"Holiday World", 0},
  {"Alabama Adventure", 0}
};

// Universal Parks (NBCUniversal)
static Park universal_parks[] = {
  {"Universal Studios Florida", 0},
  {"Islands of Adventure", 0},
  {"Epic Universe", 0},
  {"Universal Studios Hollywood", 0}
};

// Independent Parks
static Park independent_parks[] = {
  {"Kentucky Kingdom", 116},
  {"Indiana Beach", 103},
  {"St. Louis Union Station", 0}
};

static char selected_park[64] = "No park selected";
static char selected_coaster[64] = "No coaster selected";

// Recording state
static RecordingState recording_state = RECORDING_STOPPED;
static int recording_elapsed_seconds = 0;
static int recording_elapsed_centiseconds = 0; // For more precise timing display
static int max_g_force_hundredths = 100; // 100 = 1.00g
static int min_g_force_hundredths = 100; // 100 = 1.00g
static int current_g_force_hundredths = 100; // Start at 1g (gravity)

// Timer for updating the recording UI
static AppTimer *recording_timer = NULL;
static AppTimer *display_update_timer = NULL;

// Forward declarations
static void save_ride_record(void);

// Debug function to check platform compatibility
static void debug_platform_info(void) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Starting Forces Recorder");
  #ifdef PBL_PLATFORM_DIORITE
    APP_LOG(APP_LOG_LEVEL_INFO, "Running on Diorite platform");
  #endif
  #ifdef PBL_PLATFORM_BASALT
    APP_LOG(APP_LOG_LEVEL_INFO, "Running on Basalt platform");
  #endif
  #ifdef PBL_PLATFORM_CHALK
    APP_LOG(APP_LOG_LEVEL_INFO, "Running on Chalk platform");
  #endif
  #ifdef PBL_PLATFORM_APLITE
    APP_LOG(APP_LOG_LEVEL_INFO, "Running on Aplite platform");
  #endif
}

// Simple integer square root approximation
static uint32_t isqrt(uint32_t n) {
  if (n == 0) return 0;
  uint32_t x = n;
  uint32_t y = (x + 1) / 2;
  while (y < x) {
    x = y;
    y = (x + n / x) / 2;
  }
  return x;
}

// Process custom coasters from phone
static void processCustomCoasters(DictionaryIterator *iterator) {
  Tuple *park_id_tuple = dict_find(iterator, MESSAGE_KEY_SELECTED_PARK);
  if (!park_id_tuple) return;
  
  char *park_id = park_id_tuple->value->cstring;
  APP_LOG(APP_LOG_LEVEL_INFO, "Processing custom coasters for park: %s", park_id);
  
  // Find or create park entry
  CustomParkData *park_data = NULL;
  for (int i = 0; i < custom_park_count; i++) {
    if (strcmp(custom_parks[i].park_id, park_id) == 0) {
      park_data = &custom_parks[i];
      break;
    }
  }
  
  if (!park_data && custom_park_count < MAX_CUSTOM_PARKS) {
    park_data = &custom_parks[custom_park_count];
    strncpy(park_data->park_id, park_id, sizeof(park_data->park_id) - 1);
    park_data->park_id[sizeof(park_data->park_id) - 1] = '\0';
    park_data->coaster_count = 0;
    custom_park_count++;
  }
  
  if (!park_data) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Cannot add more custom parks");
    return;
  }
  
  // Add coaster from message
  Tuple *coaster_name = dict_find(iterator, MESSAGE_KEY_NEW_COASTER_NAME);
  Tuple *coaster_type = dict_find(iterator, MESSAGE_KEY_NEW_COASTER_TYPE);
  
  if (coaster_name && park_data->coaster_count < MAX_CUSTOM_COASTERS_PER_PARK) {
    Coaster *new_coaster = &park_data->coasters[park_data->coaster_count];
    strncpy(new_coaster->name, coaster_name->value->cstring, sizeof(new_coaster->name) - 1);
    new_coaster->name[sizeof(new_coaster->name) - 1] = '\0';
    
    if (coaster_type) {
      strncpy(new_coaster->type, coaster_type->value->cstring, sizeof(new_coaster->type) - 1);
      new_coaster->type[sizeof(new_coaster->type) - 1] = '\0';
    } else {
      strncpy(new_coaster->type, "Steel", sizeof(new_coaster->type) - 1);
    }
    
    new_coaster->id = park_data->coaster_count + 1000; // Offset to avoid conflicts
    park_data->coaster_count++;
    
    APP_LOG(APP_LOG_LEVEL_INFO, "Added coaster: %s (%s) to %s", 
            new_coaster->name, new_coaster->type, park_id);
    
    vibes_short_pulse(); // Confirm addition
  }
}

// Load coasters for park including custom ones
static void load_coasters_for_park_enhanced(Park *park) {
  // First check for built-in coaster data
  if (park && park->id == 107) { // Six Flags St. Louis
    current_coasters = sixflags_stlouis_coasters;
    current_coaster_count = sizeof(sixflags_stlouis_coasters) / sizeof(sixflags_stlouis_coasters[0]);
    return;
  }
  
  // Check for Six Flags Over Georgia
  if (park && strstr(park->name, "Six Flags Over Georgia")) {
    current_coasters = sixflags_georgia_coasters;
    current_coaster_count = sizeof(sixflags_georgia_coasters) / sizeof(sixflags_georgia_coasters[0]);
    return;
  }
  
  // Check for St. Louis Union Station
  if (park && strstr(park->name, "Union Station")) {
    current_coasters = union_station_coasters;
    current_coaster_count = sizeof(union_station_coasters) / sizeof(union_station_coasters[0]);
    return;
  }
  
  // Check for custom park data
  char park_search[64];
  snprintf(park_search, sizeof(park_search), "%s", park ? park->name : "");
  
  for (int i = 0; i < custom_park_count; i++) {
    // Simple name matching - could be enhanced
    if (strstr(park_search, "Six Flags St. Louis") && 
        strcmp(custom_parks[i].park_id, "sixflags_stlouis") == 0) {
      current_coasters = custom_parks[i].coasters;
      current_coaster_count = custom_parks[i].coaster_count;
      APP_LOG(APP_LOG_LEVEL_INFO, "Loaded %d custom coasters for %s", 
              current_coaster_count, custom_parks[i].park_id);
      return;
    }
  }
  
  // No coaster data available for this park
  current_coasters = NULL;
  current_coaster_count = 0;
}

// Accelerometer data handler
static void accel_data_handler(AccelData *data, uint32_t num_samples) {
  if (recording_state == RECORDING_RUNNING) {
    // Calculate magnitude of acceleration vector
    for (uint32_t i = 0; i < num_samples; i++) {
      int32_t x = data[i].x;
      int32_t y = data[i].y;
      int32_t z = data[i].z;
      
      // Calculate magnitude in g-forces (Pebble accel is in milli-g)
      // Use integer square root to avoid math library
      uint32_t magnitude_squared = x*x + y*y + z*z;
      uint32_t magnitude_mg = isqrt(magnitude_squared);
      
      // Convert to hundredths of g (e.g., 150 = 1.50g)
      int magnitude_hundredths = (int)(magnitude_mg / 10); // milli-g to hundredths of g
      
      // Update max/min values
      if (magnitude_hundredths > max_g_force_hundredths) {
        max_g_force_hundredths = magnitude_hundredths;
        // Vibrate on new peak if enabled and above threshold
        if (app_settings.vibrate_on_peaks && magnitude_hundredths > app_settings.g_force_threshold) {
          vibes_short_pulse();
        }
      }
      if (magnitude_hundredths < min_g_force_hundredths) {
        min_g_force_hundredths = magnitude_hundredths;
        // Vibrate on new low G-force (airtime!) if enabled
        if (app_settings.vibrate_on_peaks && magnitude_hundredths < 50) { // Less than 0.5g
          vibes_short_pulse();
        }
      }
      
      current_g_force_hundredths = magnitude_hundredths;
    }
  }
}

// Message handling for phone communication
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Message received from phone");
  
  // Read settings data from phone
  Tuple *settings_tuple = dict_find(iterator, MESSAGE_KEY_SETTINGS_DATA);
  if (settings_tuple) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Received settings update from phone");
    
    // Update individual settings
    Tuple *recording_mode = dict_find(iterator, MESSAGE_KEY_RECORDING_MODE);
    if (recording_mode) {
      app_settings.recording_mode = recording_mode->value->int32;
      APP_LOG(APP_LOG_LEVEL_INFO, "Recording mode: %d", app_settings.recording_mode);
    }
    
    Tuple *g_threshold = dict_find(iterator, MESSAGE_KEY_G_FORCE_THRESHOLD);
    if (g_threshold) {
      app_settings.g_force_threshold = g_threshold->value->int32;
      APP_LOG(APP_LOG_LEVEL_INFO, "G-force threshold: %d", app_settings.g_force_threshold);
    }
    
    Tuple *vibrate_peaks = dict_find(iterator, MESSAGE_KEY_VIBRATE_ON_PEAKS);
    if (vibrate_peaks) {
      app_settings.vibrate_on_peaks = vibrate_peaks->value->int32 == 1;
    }
    
    Tuple *time_format = dict_find(iterator, MESSAGE_KEY_TIME_FORMAT);
    if (time_format) {
      app_settings.time_format = time_format->value->int32;
    }
    
    Tuple *max_rides = dict_find(iterator, MESSAGE_KEY_MAX_STORED_RIDES);
    if (max_rides) {
      app_settings.max_stored_rides = max_rides->value->int32;
    }
    
    Tuple *auto_save = dict_find(iterator, MESSAGE_KEY_AUTO_SAVE);
    if (auto_save) {
      app_settings.auto_save = auto_save->value->int32 == 1;
    }
  }
  
  // Handle clear data command
  Tuple *clear_data = dict_find(iterator, MESSAGE_KEY_CLEAR_WATCH_DATA);
  if (clear_data) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Clearing all watch data");
    persist_delete(PERSIST_KEY_RIDE_COUNT);
    for (int i = 0; i < MAX_STORED_RIDES; i++) {
      persist_delete(PERSIST_KEY_RIDE_BASE + i);
    }
    vibes_short_pulse();
  }
  
  // Handle custom coasters data
  Tuple *custom_coasters = dict_find(iterator, MESSAGE_KEY_ADD_COASTER);
  if (custom_coasters) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Received custom coasters data from phone");
    
    Tuple *park_id_tuple = dict_find(iterator, MESSAGE_KEY_SELECTED_PARK);
    Tuple *coaster_count_tuple = dict_find(iterator, MESSAGE_KEY_NEW_COASTER_NAME);
    
    if (park_id_tuple && coaster_count_tuple) {
      // Process custom coaster data
      processCustomCoasters(iterator);
    }
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed: %d", reason);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success");
}

// Send ride data to phone for storage/export
static void send_ride_to_phone(RideRecord *record) {
  if (!app_settings.auto_save) return;
  
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  
  if (iter == NULL) return;
  
  dict_write_int(iter, MESSAGE_KEY_RIDE_DATA, &(int){1}, sizeof(int), true);
  dict_write_cstring(iter, MESSAGE_KEY_PARK_NAME, record->park_name);
  dict_write_cstring(iter, MESSAGE_KEY_COASTER_NAME, record->coaster_name);
  dict_write_int(iter, MESSAGE_KEY_DURATION, &record->duration_seconds, sizeof(int), true);
  dict_write_int(iter, MESSAGE_KEY_MAX_G_FORCE, &record->max_g_force_hundredths, sizeof(int), true);
  dict_write_int(iter, MESSAGE_KEY_MIN_G_FORCE, &record->min_g_force_hundredths, sizeof(int), true);
  
  // Add device type for analytics
  #ifdef PBL_PLATFORM_APLITE
    dict_write_cstring(iter, MESSAGE_KEY_DEVICE_TYPE, "Pebble Original");
  #elif defined(PBL_PLATFORM_BASALT)
    dict_write_cstring(iter, MESSAGE_KEY_DEVICE_TYPE, "Pebble Time/Steel");
  #elif defined(PBL_PLATFORM_CHALK)
    dict_write_cstring(iter, MESSAGE_KEY_DEVICE_TYPE, "Pebble Time Round");
  #elif defined(PBL_PLATFORM_DIORITE)
    dict_write_cstring(iter, MESSAGE_KEY_DEVICE_TYPE, "Pebble 2");
  #else
    dict_write_cstring(iter, MESSAGE_KEY_DEVICE_TYPE, "Unknown Pebble");
  #endif
  
  app_message_outbox_send();
  APP_LOG(APP_LOG_LEVEL_INFO, "Sent ride data to phone");
}

// Request settings from phone on startup
static void request_settings_from_phone(void) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  
  if (iter == NULL) return;
  
  dict_write_int(iter, MESSAGE_KEY_REQUEST_SETTINGS, &(int){1}, sizeof(int), true);
  app_message_outbox_send();
  APP_LOG(APP_LOG_LEVEL_INFO, "Requested settings from phone");
}

// Main menu callbacks
static uint16_t main_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

static uint16_t main_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return NUM_MAIN_MENU_ITEMS;
}

static int16_t main_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void main_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  menu_cell_basic_header_draw(ctx, cell_layer, "Forces Recorder");
}

static void main_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  if (cell_index->row == 1) { // "Set Current Park" item
    menu_cell_basic_draw(ctx, cell_layer, main_menu_items[cell_index->row], selected_park, NULL);
  } else if (cell_index->row == 4) { // "Quick Start" item
    if (strlen(selected_coaster) > 0 && strcmp(selected_coaster, "No coaster selected") != 0) {
      menu_cell_basic_draw(ctx, cell_layer, main_menu_items[cell_index->row], selected_coaster, NULL);
    } else {
      menu_cell_basic_draw(ctx, cell_layer, main_menu_items[cell_index->row], "Select coaster first", NULL);
    }
  } else {
    menu_cell_basic_draw(ctx, cell_layer, main_menu_items[cell_index->row], NULL, NULL);
  }
}

// Park menu callbacks
static uint16_t park_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return 7; // Six Flags, Cedar Fair, Herschend, Disney, United Parks, Koch, Universal, Independent
}

static uint16_t park_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch(section_index) {
    case 0: return sizeof(sixflags_parks) / sizeof(sixflags_parks[0]);
    case 1: return sizeof(cedarfair_parks) / sizeof(cedarfair_parks[0]);
    case 2: return sizeof(herschend_parks) / sizeof(herschend_parks[0]);
    case 3: return sizeof(disney_parks) / sizeof(disney_parks[0]);
    case 4: return sizeof(unitedparks_parks) / sizeof(unitedparks_parks[0]);
    case 5: return sizeof(koch_parks) / sizeof(koch_parks[0]);
    case 6: return sizeof(universal_parks) / sizeof(universal_parks[0]);
    default: return sizeof(independent_parks) / sizeof(independent_parks[0]);
  }
}

static int16_t park_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void park_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  switch(section_index) {
    case 0:
      menu_cell_basic_header_draw(ctx, cell_layer, "Six Flags (Legacy)");
      break;
    case 1:
      menu_cell_basic_header_draw(ctx, cell_layer, "Cedar Fair (Legacy)");
      break;
    case 2:
      menu_cell_basic_header_draw(ctx, cell_layer, "Herschend Family");
      break;
    case 3:
      menu_cell_basic_header_draw(ctx, cell_layer, "Disney Parks");
      break;
    case 4:
      menu_cell_basic_header_draw(ctx, cell_layer, "United Parks & Resorts");
      break;
    case 5:
      menu_cell_basic_header_draw(ctx, cell_layer, "Koch Family Parks");
      break;
    case 6:
      menu_cell_basic_header_draw(ctx, cell_layer, "Universal Parks");
      break;
    default:
      menu_cell_basic_header_draw(ctx, cell_layer, "Independent");
      break;
  }
}

static void park_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  Park *park = NULL;
  
  switch(cell_index->section) {
    case 0:
      park = &sixflags_parks[cell_index->row];
      break;
    case 1:
      park = &cedarfair_parks[cell_index->row];
      break;
    case 2:
      park = &herschend_parks[cell_index->row];
      break;
    case 3:
      park = &disney_parks[cell_index->row];
      break;
    case 4:
      park = &unitedparks_parks[cell_index->row];
      break;
    case 5:
      park = &koch_parks[cell_index->row];
      break;
    case 6:
      park = &universal_parks[cell_index->row];
      break;
    default:
      park = &independent_parks[cell_index->row];
      break;
  }
  
  if (park) {
    menu_cell_basic_draw(ctx, cell_layer, park->name, NULL, NULL);
  }
}

static void load_coasters_for_park(Park *park) {
  load_coasters_for_park_enhanced(park);
}

// Coaster menu callbacks
static uint16_t coaster_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return 1;
}

static uint16_t coaster_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return current_coaster_count;
}

static int16_t coaster_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void coaster_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  if (current_park) {
    menu_cell_basic_header_draw(ctx, cell_layer, current_park->name);
  } else {
    menu_cell_basic_header_draw(ctx, cell_layer, "Select Coaster");
  }
}

static void coaster_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  if (current_coasters && cell_index->row < current_coaster_count) {
    Coaster *coaster = &current_coasters[cell_index->row];
    menu_cell_basic_draw(ctx, cell_layer, coaster->name, coaster->type, NULL);
  }
}

static void coaster_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  if (current_coasters && cell_index->row < current_coaster_count) {
    Coaster *selected_coaster_ptr = &current_coasters[cell_index->row];
    
    // Set the selected coaster
    strncpy(selected_coaster, selected_coaster_ptr->name, sizeof(selected_coaster) - 1);
    selected_coaster[sizeof(selected_coaster) - 1] = '\0';
    
    // Reset recording state
    recording_state = RECORDING_STOPPED;
    max_g_force_hundredths = 100;
    min_g_force_hundredths = 100;
    current_g_force_hundredths = 100;
    recording_elapsed_seconds = 0;
    recording_elapsed_centiseconds = 0;
    
    vibes_short_pulse();
    
    // Open the recording window
    window_stack_push(recording_window, true);
  }
}

static void park_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  Park *selected = NULL;
  
  switch(cell_index->section) {
    case 0:
      selected = &sixflags_parks[cell_index->row];
      break;
    case 1:
      selected = &cedarfair_parks[cell_index->row];
      break;
    case 2:
      selected = &herschend_parks[cell_index->row];
      break;
    case 3:
      selected = &disney_parks[cell_index->row];
      break;
    case 4:
      selected = &unitedparks_parks[cell_index->row];
      break;
    case 5:
      selected = &koch_parks[cell_index->row];
      break;
    case 6:
      selected = &universal_parks[cell_index->row];
      break;
    default:
      selected = &independent_parks[cell_index->row];
      break;
  }
  
  if (selected) {
    // Set the selected park
    strncpy(selected_park, selected->name, sizeof(selected_park) - 1);
    selected_park[sizeof(selected_park) - 1] = '\0';
    
    // Store the selected park and load its coasters
    current_park = selected;
    load_coasters_for_park(selected);
    
    vibes_short_pulse();
    
    // Go back to main menu
    window_stack_pop(true);
    
    // Refresh main menu to show selected park
    menu_layer_reload_data(main_menu_layer);
  }
}

// Recording functions
static void update_recording_display(void) {
  static char elapsed_text[32];
  static char max_g_text[32];
  static char min_g_text[32];
  static char current_g_text[32];
  
  // Update elapsed time with configurable format
  int minutes = recording_elapsed_seconds / 60;
  int seconds = recording_elapsed_seconds % 60;
  int centiseconds = recording_elapsed_centiseconds;
  
  if (recording_state == RECORDING_RUNNING) {
    // Show centiseconds when recording and format allows
    if (app_settings.time_format == 1) { // MM:SS.CC
      snprintf(elapsed_text, sizeof(elapsed_text), "%02d:%02d.%02d", minutes, seconds, centiseconds);
    } else if (app_settings.time_format == 2) { // SS.CC
      snprintf(elapsed_text, sizeof(elapsed_text), "%02d.%02d", recording_elapsed_seconds, centiseconds);
    } else { // MM:SS
      snprintf(elapsed_text, sizeof(elapsed_text), "%02d:%02d", minutes, seconds);
    }
  } else {
    // When stopped/paused, use simpler format
    if (app_settings.time_format == 2) { // SS.CC
      snprintf(elapsed_text, sizeof(elapsed_text), "%02d.00", recording_elapsed_seconds);
    } else { // MM:SS
      snprintf(elapsed_text, sizeof(elapsed_text), "%02d:%02d", minutes, seconds);
    }
  }
  text_layer_set_text(elapsed_time_layer, elapsed_text);
  
  // Update current G force (convert hundredths to display format)
  int g_whole = current_g_force_hundredths / 100;
  int g_frac = current_g_force_hundredths % 100;
  snprintf(current_g_text, sizeof(current_g_text), "%d.%02dg", g_whole, g_frac);
  text_layer_set_text(current_g_layer, current_g_text);
  
  // Update max G force
  g_whole = max_g_force_hundredths / 100;
  g_frac = max_g_force_hundredths % 100;
  snprintf(max_g_text, sizeof(max_g_text), "Max: %d.%02dg", g_whole, g_frac);
  text_layer_set_text(max_g_layer, max_g_text);
  
  // Update min G force  
  g_whole = min_g_force_hundredths / 100;
  g_frac = min_g_force_hundredths % 100;
  snprintf(min_g_text, sizeof(min_g_text), "Min: %d.%02dg", g_whole, g_frac);
  text_layer_set_text(min_g_layer, min_g_text);
  
  // Update button labels and styling
  if (recording_state == RECORDING_STOPPED) {
    text_layer_set_text(start_button_layer, "START");
    #ifdef PBL_COLOR
      text_layer_set_text_color(start_button_layer, GColorWhite);
      text_layer_set_text_color(up_button_layer, GColorLightGray);
      text_layer_set_text_color(down_button_layer, GColorLightGray);
    #else
      text_layer_set_text_color(start_button_layer, GColorWhite);
      text_layer_set_text_color(up_button_layer, GColorLightGray);
      text_layer_set_text_color(down_button_layer, GColorLightGray);
    #endif
  } else if (recording_state == RECORDING_RUNNING) {
    text_layer_set_text(start_button_layer, "PAUSE");
    #ifdef PBL_COLOR
      text_layer_set_text_color(start_button_layer, GColorYellow);
      text_layer_set_text_color(up_button_layer, GColorWhite);
      text_layer_set_text_color(down_button_layer, GColorRed);
    #else
      text_layer_set_text_color(start_button_layer, GColorWhite);
      text_layer_set_text_color(up_button_layer, GColorWhite);
      text_layer_set_text_color(down_button_layer, GColorWhite);
    #endif
  } else {
    text_layer_set_text(start_button_layer, "RESUME");
    #ifdef PBL_COLOR
      text_layer_set_text_color(start_button_layer, GColorGreen);
      text_layer_set_text_color(up_button_layer, GColorWhite);
      text_layer_set_text_color(down_button_layer, GColorRed);
    #else
      text_layer_set_text_color(start_button_layer, GColorWhite);
      text_layer_set_text_color(up_button_layer, GColorWhite);
      text_layer_set_text_color(down_button_layer, GColorWhite);
    #endif
  }
}

static void recording_timer_callback(void *data) {
  if (recording_state == RECORDING_RUNNING) {
    recording_elapsed_seconds++;
    update_recording_display();
    recording_timer = app_timer_register(1000, recording_timer_callback, NULL);
  }
}

// Fast update timer for real-time G-force display and centiseconds
static void display_update_callback(void *data) {
  if (recording_state == RECORDING_RUNNING) {
    // Update centiseconds (0-99, resets every second)
    recording_elapsed_centiseconds += 10; // 100ms intervals = 10 centiseconds
    if (recording_elapsed_centiseconds >= 100) {
      recording_elapsed_centiseconds = 0;
    }
    
    update_recording_display();
    display_update_timer = app_timer_register(100, display_update_callback, NULL); // Update every 100ms
  }
}

static void start_pause_recording(void) {
  if (recording_state == RECORDING_STOPPED || recording_state == RECORDING_PAUSED) {
    recording_state = RECORDING_RUNNING;
    
    // Start accelerometer service - with platform-specific handling
    #ifdef PBL_PLATFORM_DIORITE
      // Pebble 2 - use fewer samples and lower sampling rate to avoid conflicts with heart rate sensor
      accel_data_service_subscribe(ACCEL_SAMPLES_PEBBLE2, accel_data_handler);
      accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
      APP_LOG(APP_LOG_LEVEL_INFO, "Pebble 2 detected - using 25Hz accelerometer sampling with reduced samples");
    #else
      // Other platforms - use higher sampling rate for better precision
      accel_data_service_subscribe(ACCEL_SAMPLES_PER_UPDATE, accel_data_handler);
      accel_service_set_sampling_rate(ACCEL_SAMPLING_50HZ);
    #endif
    
    // Start timers
    if (recording_timer) {
      app_timer_cancel(recording_timer);
    }
    if (display_update_timer) {
      app_timer_cancel(display_update_timer);
    }
    recording_timer = app_timer_register(1000, recording_timer_callback, NULL);
    display_update_timer = app_timer_register(100, display_update_callback, NULL);
    vibes_short_pulse();
  } else if (recording_state == RECORDING_RUNNING) {
    recording_state = RECORDING_PAUSED;
    
    // Pause accelerometer service on all platforms
    accel_data_service_unsubscribe();
    
    // Cancel timers
    if (recording_timer) {
      app_timer_cancel(recording_timer);
      recording_timer = NULL;
    }
    if (display_update_timer) {
      app_timer_cancel(display_update_timer);
      display_update_timer = NULL;
    }
    vibes_short_pulse();
  }
  update_recording_display();
}

static void stop_recording(void) {
  recording_state = RECORDING_STOPPED;
  
  // Stop accelerometer service on all platforms
  accel_data_service_unsubscribe();
  
  // Cancel timers
  if (recording_timer) {
    app_timer_cancel(recording_timer);
    recording_timer = NULL;
  }
  if (display_update_timer) {
    app_timer_cancel(display_update_timer);
    display_update_timer = NULL;
  }
  
  // Save recording data
  if (recording_elapsed_seconds > 0) {
    save_ride_record();
  }
  
  vibes_double_pulse();
  window_stack_pop(true); // Go back to coaster selection
}

// Save ride recording data to persistent storage
static void save_ride_record(void) {
  RideRecord record;
  
  // Fill in the record data
  strncpy(record.park_name, selected_park, sizeof(record.park_name) - 1);
  record.park_name[sizeof(record.park_name) - 1] = '\0';
  strncpy(record.coaster_name, selected_coaster, sizeof(record.coaster_name) - 1);
  record.coaster_name[sizeof(record.coaster_name) - 1] = '\0';
  record.duration_seconds = recording_elapsed_seconds;
  record.max_g_force_hundredths = max_g_force_hundredths;
  record.min_g_force_hundredths = min_g_force_hundredths;
  record.timestamp = time(NULL);
  
  // Get current ride count
  int ride_count = persist_exists(PERSIST_KEY_RIDE_COUNT) ? persist_read_int(PERSIST_KEY_RIDE_COUNT) : 0;
  
  // If we're at max capacity, shift older records
  if (ride_count >= MAX_STORED_RIDES) {
    for (int i = 1; i < MAX_STORED_RIDES; i++) {
      RideRecord temp;
      if (persist_read_data(PERSIST_KEY_RIDE_BASE + i, &temp, sizeof(RideRecord)) == sizeof(RideRecord)) {
        persist_write_data(PERSIST_KEY_RIDE_BASE + i - 1, &temp, sizeof(RideRecord));
      }
    }
    ride_count = MAX_STORED_RIDES - 1;
  }
  
  // Save the new record
  persist_write_data(PERSIST_KEY_RIDE_BASE + ride_count, &record, sizeof(RideRecord));
  persist_write_int(PERSIST_KEY_RIDE_COUNT, ride_count + 1);
  
  // Send ride data to phone for web interface
  send_ride_to_phone(&record);
}

static void recording_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  start_pause_recording();
}

static void recording_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Reset max/min G-force values
  max_g_force_hundredths = current_g_force_hundredths;
  min_g_force_hundredths = current_g_force_hundredths;
  update_recording_display();
  vibes_short_pulse();
}

static void recording_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  stop_recording();
}

static void recording_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, recording_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, recording_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, recording_down_click_handler);
}

static void main_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  switch(cell_index->row) {
    case 0: // New Ride
      if (current_park && current_coaster_count > 0) {
        window_stack_push(coaster_window, true);
      } else {
        // No park selected or no coasters available
        vibes_short_pulse();
      }
      break;
    case 1: // Set Current Park
      window_stack_push(park_window, true);
      break;
    case 2: // Ride History
      vibes_short_pulse();
      break;
    case 3: // Options
      vibes_short_pulse();
      break;
    case 4: // Quick Start
      if (strlen(selected_coaster) > 0 && strcmp(selected_coaster, "No coaster selected") != 0) {
        // Reset recording state for quick start
        recording_state = RECORDING_STOPPED;
        max_g_force_hundredths = 100;
        min_g_force_hundredths = 100;
        current_g_force_hundredths = 100;
        recording_elapsed_seconds = 0;
        recording_elapsed_centiseconds = 0;
        
        vibes_short_pulse();
        window_stack_push(recording_window, true);
      } else {
        // No coaster selected
        vibes_short_pulse();
      }
      break;
  }
}

// Main window functions
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create the menu layer
  main_menu_layer = menu_layer_create(bounds);
  
  // Set menu callbacks
  menu_layer_set_callbacks(main_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = main_menu_get_num_sections_callback,
    .get_num_rows = main_menu_get_num_rows_callback,
    .get_header_height = main_menu_get_header_height_callback,
    .draw_header = main_menu_draw_header_callback,
    .draw_row = main_menu_draw_row_callback,
    .select_click = main_menu_select_callback,
  });
  
  // Let the menu layer handle navigation
  menu_layer_set_click_config_onto_window(main_menu_layer, window);
  
  // Add the menu layer to the window
  layer_add_child(window_layer, menu_layer_get_layer(main_menu_layer));
}

static void main_window_unload(Window *window) {
  menu_layer_destroy(main_menu_layer);
}

static void park_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create the park menu layer
  park_menu_layer = menu_layer_create(bounds);
  
  // Set park menu callbacks
  menu_layer_set_callbacks(park_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = park_menu_get_num_sections_callback,
    .get_num_rows = park_menu_get_num_rows_callback,
    .get_header_height = park_menu_get_header_height_callback,
    .draw_header = park_menu_draw_header_callback,
    .draw_row = park_menu_draw_row_callback,
    .select_click = park_menu_select_callback,
  });
  
  // Let the park menu layer handle navigation
  menu_layer_set_click_config_onto_window(park_menu_layer, window);
  
  // Add the park menu layer to the window
  layer_add_child(window_layer, menu_layer_get_layer(park_menu_layer));
}

static void park_window_unload(Window *window) {
  menu_layer_destroy(park_menu_layer);
}

static void coaster_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create the coaster menu layer
  coaster_menu_layer = menu_layer_create(bounds);
  
  // Set coaster menu callbacks
  menu_layer_set_callbacks(coaster_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = coaster_menu_get_num_sections_callback,
    .get_num_rows = coaster_menu_get_num_rows_callback,
    .get_header_height = coaster_menu_get_header_height_callback,
    .draw_header = coaster_menu_draw_header_callback,
    .draw_row = coaster_menu_draw_row_callback,
    .select_click = coaster_menu_select_callback,
  });
  
  // Let the coaster menu layer handle navigation
  menu_layer_set_click_config_onto_window(coaster_menu_layer, window);
  
  // Add the coaster menu layer to the window
  layer_add_child(window_layer, menu_layer_get_layer(coaster_menu_layer));
}

static void coaster_window_unload(Window *window) {
  menu_layer_destroy(coaster_menu_layer);
}

static void recording_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Set window background color - blue for color models, black for B&W
  #ifdef PBL_COLOR
    window_set_background_color(window, GColorOxfordBlue);
  #else
    window_set_background_color(window, GColorBlack);
  #endif
  
  // Create coaster name layer (top, smaller and more compact)
  coaster_name_layer = text_layer_create(GRect(0, 0, bounds.size.w, 20));
  text_layer_set_text(coaster_name_layer, selected_coaster);
  text_layer_set_text_alignment(coaster_name_layer, GTextAlignmentCenter);
  text_layer_set_font(coaster_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  #ifdef PBL_COLOR
    text_layer_set_text_color(coaster_name_layer, GColorYellow);
  #else
    text_layer_set_text_color(coaster_name_layer, GColorWhite);
  #endif
  text_layer_set_background_color(coaster_name_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(coaster_name_layer));
  
  // Create elapsed time layer (center large) - main timer display like stopwatch
  elapsed_time_layer = text_layer_create(GRect(0, 25, bounds.size.w, 45));
  text_layer_set_text(elapsed_time_layer, "00:00");
  text_layer_set_text_alignment(elapsed_time_layer, GTextAlignmentCenter);
  text_layer_set_font(elapsed_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  #ifdef PBL_COLOR
    text_layer_set_text_color(elapsed_time_layer, GColorWhite);
  #else
    text_layer_set_text_color(elapsed_time_layer, GColorWhite);
  #endif
  text_layer_set_background_color(elapsed_time_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(elapsed_time_layer));
  
  // Create current G force layer (prominently displayed below timer)
  current_g_layer = text_layer_create(GRect(0, 70, bounds.size.w, 30));
  text_layer_set_text(current_g_layer, "1.00g");
  text_layer_set_text_alignment(current_g_layer, GTextAlignmentCenter);
  text_layer_set_font(current_g_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  #ifdef PBL_COLOR
    text_layer_set_text_color(current_g_layer, GColorGreen);
  #else
    text_layer_set_text_color(current_g_layer, GColorWhite);
  #endif
  text_layer_set_background_color(current_g_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(current_g_layer));
  
  // Create max and min G force layers (side by side at bottom)
  max_g_layer = text_layer_create(GRect(0, 105, bounds.size.w/2, 20));
  text_layer_set_text(max_g_layer, "Max: 1.00g");
  text_layer_set_text_alignment(max_g_layer, GTextAlignmentCenter);
  text_layer_set_font(max_g_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  #ifdef PBL_COLOR
    text_layer_set_text_color(max_g_layer, GColorRed);
  #else
    text_layer_set_text_color(max_g_layer, GColorWhite);
  #endif
  text_layer_set_background_color(max_g_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(max_g_layer));
  
  min_g_layer = text_layer_create(GRect(bounds.size.w/2, 105, bounds.size.w/2, 20));
  text_layer_set_text(min_g_layer, "Min: 1.00g");
  text_layer_set_text_alignment(min_g_layer, GTextAlignmentCenter);
  text_layer_set_font(min_g_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  #ifdef PBL_COLOR
    text_layer_set_text_color(min_g_layer, GColorCyan);
  #else
    text_layer_set_text_color(min_g_layer, GColorWhite);
  #endif
  text_layer_set_background_color(min_g_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(min_g_layer));
  
  // Button indicators positioned like Pebble stopwatch (more subtle)
  up_button_layer = text_layer_create(GRect(bounds.size.w - 45, 10, 45, 15));
  text_layer_set_text(up_button_layer, "RESET");
  text_layer_set_text_alignment(up_button_layer, GTextAlignmentCenter);
  text_layer_set_font(up_button_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  #ifdef PBL_COLOR
    text_layer_set_text_color(up_button_layer, GColorLightGray);
  #else
    text_layer_set_text_color(up_button_layer, GColorLightGray);
  #endif
  text_layer_set_background_color(up_button_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(up_button_layer));
  
  start_button_layer = text_layer_create(GRect(bounds.size.w - 45, 75, 45, 15));
  text_layer_set_text(start_button_layer, "START");
  text_layer_set_text_alignment(start_button_layer, GTextAlignmentCenter);
  text_layer_set_font(start_button_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  #ifdef PBL_COLOR
    text_layer_set_text_color(start_button_layer, GColorWhite);
  #else
    text_layer_set_text_color(start_button_layer, GColorWhite);
  #endif
  text_layer_set_background_color(start_button_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(start_button_layer));
  
  down_button_layer = text_layer_create(GRect(bounds.size.w - 45, 140, 45, 15));
  text_layer_set_text(down_button_layer, "STOP");
  text_layer_set_text_alignment(down_button_layer, GTextAlignmentCenter);
  text_layer_set_font(down_button_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  #ifdef PBL_COLOR
    text_layer_set_text_color(down_button_layer, GColorLightGray);
  #else
    text_layer_set_text_color(down_button_layer, GColorLightGray);
  #endif
  text_layer_set_background_color(down_button_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(down_button_layer));
  
  // Set up click handlers
  window_set_click_config_provider(window, recording_click_config_provider);
  
  // Initialize display
  update_recording_display();
}

static void recording_window_unload(Window *window) {
  // Clean up timers
  if (recording_timer) {
    app_timer_cancel(recording_timer);
    recording_timer = NULL;
  }
  if (display_update_timer) {
    app_timer_cancel(display_update_timer);
    display_update_timer = NULL;
  }
  
  // Stop accelerometer service if running
  if (recording_state == RECORDING_RUNNING) {
    accel_data_service_unsubscribe();
  }
  
  // Destroy text layers
  text_layer_destroy(coaster_name_layer);
  text_layer_destroy(elapsed_time_layer);
  text_layer_destroy(current_g_layer);
  text_layer_destroy(max_g_layer);
  text_layer_destroy(min_g_layer);
  text_layer_destroy(start_button_layer);
  text_layer_destroy(up_button_layer);
  text_layer_destroy(down_button_layer);
}

static void init(void) {
  // Debug platform information
  debug_platform_info();
  
  // Initialize app message communication
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open app message with reasonable buffer sizes
  app_message_open(512, 512);
  
  // Request settings from phone
  request_settings_from_phone();
  
  // Create main window
  main_window = window_create();
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  // Create park window
  park_window = window_create();
  window_set_window_handlers(park_window, (WindowHandlers) {
    .load = park_window_load,
    .unload = park_window_unload,
  });
  
  // Create coaster window
  coaster_window = window_create();
  window_set_window_handlers(coaster_window, (WindowHandlers) {
    .load = coaster_window_load,
    .unload = coaster_window_unload,
  });
  
  // Create recording window
  recording_window = window_create();
  window_set_window_handlers(recording_window, (WindowHandlers) {
    .load = recording_window_load,
    .unload = recording_window_unload,
  });
  
  // Push the main window onto the stack
  window_stack_push(main_window, true);
}

static void deinit(void) {
  window_destroy(main_window);
  window_destroy(park_window);
  window_destroy(coaster_window);
  window_destroy(recording_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}
