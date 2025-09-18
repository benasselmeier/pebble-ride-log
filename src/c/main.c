#include <pebble.h>
#include "config.h"

// Use the API key from config.h
const char* api_key = CAPTAIN_COASTERS_API_KEY;

// Menu state management
typedef enum {
  MENU_STATE_MAIN,
  MENU_STATE_NEW_RIDE,
  MENU_STATE_PARK_LIST,
  MENU_STATE_COASTER_LIST
} MenuState;

static MenuState current_menu_state = MENU_STATE_MAIN;
static char current_park_name[64] = "No Park Selected";

// New Ride submenu items
static char* new_ride_menu_items[] = {
  "Current Park",
  "Other Parks"
};
#define NUM_NEW_RIDE_ITEMS 2

// Main menu window and layer
static Window *main_window;
static MenuLayer *main_menu_layer;

// Placeholder window for menu items
static Window *placeholder_window;
static TextLayer *placeholder_text_layer;

// Menu items
#define NUM_MENU_SECTIONS 1
#define NUM_MENU_ITEMS 5

// Menu item titles
static char* menu_items[] = {
  "New Ride",
  "Set Current Park",
  "Ride History", 
  "Options",
  "Quick Start"
};

// Menu callbacks
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch(current_menu_state) {
    case MENU_STATE_NEW_RIDE:
      return NUM_NEW_RIDE_ITEMS;
    default:
      return NUM_MENU_ITEMS;
  }
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  // Draw different headers based on menu state
  switch(current_menu_state) {
    case MENU_STATE_NEW_RIDE:
      menu_cell_basic_header_draw(ctx, cell_layer, "New Ride");
      break;
    case MENU_STATE_PARK_LIST:
      menu_cell_basic_header_draw(ctx, cell_layer, "Select Park");
      break;
    case MENU_STATE_COASTER_LIST:
      menu_cell_basic_header_draw(ctx, cell_layer, current_park_name);
      break;
    default:
      menu_cell_basic_header_draw(ctx, cell_layer, "Forces Recorder");
      break;
  }
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  // Draw menu items based on current state
  switch(current_menu_state) {
    case MENU_STATE_NEW_RIDE:
      menu_cell_basic_draw(ctx, cell_layer, new_ride_menu_items[cell_index->row], NULL, NULL);
      break;
    case MENU_STATE_MAIN:
      // Show current park status for "Set Current Park" item
      if(cell_index->row == 1) { // "Set Current Park" is index 1
        menu_cell_basic_draw(ctx, cell_layer, menu_items[cell_index->row], current_park_name, NULL);
      } else {
        menu_cell_basic_draw(ctx, cell_layer, menu_items[cell_index->row], NULL, NULL);
      }
      break;
    default:
      menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
      break;
  }
}

// Placeholder window functions
static char placeholder_text[64];

static void placeholder_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  placeholder_text_layer = text_layer_create(GRect(5, 50, bounds.size.w - 10, 60));
  text_layer_set_background_color(placeholder_text_layer, GColorClear);
  text_layer_set_text_color(placeholder_text_layer, GColorBlack);
  text_layer_set_font(placeholder_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(placeholder_text_layer, GTextAlignmentCenter);
  text_layer_set_text(placeholder_text_layer, placeholder_text);
  layer_add_child(window_layer, text_layer_get_layer(placeholder_text_layer));
}

static void placeholder_window_unload(Window *window) {
  text_layer_destroy(placeholder_text_layer);
}

static void show_placeholder(const char* feature_name) {
  // Set the placeholder text before creating/showing window
  snprintf(placeholder_text, sizeof(placeholder_text), "%s\nis not yet\nimplemented", feature_name);
  
  if(!placeholder_window) {
    placeholder_window = window_create();
    window_set_window_handlers(placeholder_window, (WindowHandlers) {
      .load = placeholder_window_load,
      .unload = placeholder_window_unload,
    });
  }
  
  window_stack_push(placeholder_window, true);
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  switch(current_menu_state) {
    case MENU_STATE_MAIN:
      switch(cell_index->row) {
        case 0: // New Ride
          current_menu_state = MENU_STATE_NEW_RIDE;
          menu_layer_reload_data(main_menu_layer);
          break;
        case 1: // Set Current Park
          show_placeholder("Set Current Park");
          break;
        case 2: // Ride History
          show_placeholder("Ride History");
          break;
        case 3: // Options
          show_placeholder("Options");
          break;
        case 4: // Quick Start
          show_placeholder("Quick Start");
          break;
      }
      break;
      
    case MENU_STATE_NEW_RIDE:
      switch(cell_index->row) {
        case 0: // Current Park
          if(strcmp(current_park_name, "No Park Selected") == 0) {
            show_placeholder("No park selected.\nPlease set a current park first.");
          } else {
            show_placeholder("Current Park Coasters");
          }
          break;
        case 1: // Other Parks
          show_placeholder("Other Parks");
          break;
      }
      break;
      
    default:
      show_placeholder("Unknown menu state");
      break;
  }
}

// Back button handler for menu navigation
static void menu_back_callback(ClickRecognizerRef recognizer, void *context) {
  switch(current_menu_state) {
    case MENU_STATE_NEW_RIDE:
    case MENU_STATE_PARK_LIST:
    case MENU_STATE_COASTER_LIST:
      current_menu_state = MENU_STATE_MAIN;
      menu_layer_reload_data(main_menu_layer);
      break;
    case MENU_STATE_MAIN:
      // Exit app on back from main menu
      window_stack_pop(true);
      break;
  }
}

// Custom click config provider for menu navigation
static void menu_click_config_provider(void *context) {
  // Add our custom back button handling
  window_single_click_subscribe(BUTTON_ID_BACK, menu_back_callback);
}

// Main window functions
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create the menu layer using native PebbleOS styling
  main_menu_layer = menu_layer_create(bounds);
  
  // Set menu callbacks
  menu_layer_set_callbacks(main_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });
  
  // Let the menu layer handle its own navigation (up/down/select)
  menu_layer_set_click_config_onto_window(main_menu_layer, window);
  
  // Set our custom click config provider for the back button only
  window_set_click_config_provider(window, menu_click_config_provider);
  
  // Set menu colors
  menu_layer_set_normal_colors(main_menu_layer, GColorWhite, GColorBlack);
  menu_layer_set_highlight_colors(main_menu_layer, GColorBlack, GColorWhite);
  
  // Add the menu layer to the window
  layer_add_child(window_layer, menu_layer_get_layer(main_menu_layer));
}

static void main_window_unload(Window *window) {
  menu_layer_destroy(main_menu_layer);
}

static void init(void) {
  // Create main window
  main_window = window_create();
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  // Push the main window onto the stack
  window_stack_push(main_window, true);
}

static void deinit(void) {
  if(placeholder_window) {
    window_destroy(placeholder_window);
  }
  window_destroy(main_window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", main_window);

  // Use the API key like:
  // const char* api_key = CAPTAIN_COASTERS_API_KEY;

  app_event_loop();
  deinit();
}
