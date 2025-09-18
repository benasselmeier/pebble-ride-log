#include <pebble.h>
#include <math.h>

#define MAX_HISTORY_ENTRIES 8
#define PERSIST_KEY_HISTORY 1
#define ACCEL_SAMPLES_PER_BATCH 10
#define SYNC_INDEX_NONE 0xFF

typedef struct {
  uint32_t start_time;
  uint32_t end_time;
  uint16_t sample_count;
  int16_t max_magnitude;
  int16_t min_magnitude;
  int16_t avg_magnitude;
  int16_t peak_x;
  int16_t peak_y;
  int16_t peak_z;
  uint8_t synced;
  uint8_t reserved;
} RideSummary;

typedef struct {
  RideSummary entries[MAX_HISTORY_ENTRIES];
  uint8_t count;
  uint8_t reserved;
} RideHistory;

typedef struct {
  uint32_t start_time;
  uint16_t sample_count;
  int16_t last_magnitude;
  int16_t max_magnitude;
  int16_t min_magnitude;
  int16_t peak_x;
  int16_t peak_y;
  int16_t peak_z;
  int32_t magnitude_sum;
} CurrentRide;

static Window *s_main_window;
static TextLayer *s_title_layer;
static TextLayer *s_body_layer;

static RideHistory s_history;
static CurrentRide s_current_ride;

static bool s_logging_active;
static bool s_connection_available;
static bool s_app_message_ready;
static uint8_t s_pending_sync_index = SYNC_INDEX_NONE;
static uint8_t s_selected_history_index;

static char s_title_buffer[64];
static char s_body_buffer[256];

static void save_history(void);
static void update_idle_display(void);
static void update_logging_display(void);
static void try_sync(void);

static void reset_current_ride(void) {
  memset(&s_current_ride, 0, sizeof(s_current_ride));
  s_current_ride.max_magnitude = INT16_MIN;
  s_current_ride.min_magnitude = INT16_MAX;
}

static void load_history(void) {
  memset(&s_history, 0, sizeof(s_history));
  if (persist_exists(PERSIST_KEY_HISTORY)) {
    int read = persist_read_data(PERSIST_KEY_HISTORY, &s_history, sizeof(s_history));
    if (read < (int)sizeof(s_history)) {
      // Data from an older version; just reset
      memset(&s_history, 0, sizeof(s_history));
    }
  }
  if (s_history.count > MAX_HISTORY_ENTRIES) {
    s_history.count = MAX_HISTORY_ENTRIES;
  }
  if (s_history.count == 0) {
    s_selected_history_index = 0;
  } else if (s_selected_history_index >= s_history.count) {
    s_selected_history_index = s_history.count - 1;
  }
}

static void save_history(void) {
  persist_write_data(PERSIST_KEY_HISTORY, &s_history, sizeof(s_history));
}

static void format_g_string(char *buffer, size_t size, int16_t mg_value) {
  int16_t whole = mg_value / 1000;
  int16_t fraction = mg_value % 1000;
  if (fraction < 0) {
    fraction = -fraction;
  }
  snprintf(buffer, size, "%d.%03dg", whole, fraction);
}

static void display_summary(uint8_t index) {
  if (index >= s_history.count) {
    return;
  }

  RideSummary *summary = &s_history.entries[index];
  time_t start_time = summary->start_time;
  time_t end_time = summary->end_time;
  struct tm *start_tm = localtime(&start_time);
  struct tm *end_tm = localtime(&end_time);

  char start_buffer[32];
  char end_buffer[32];
  if (start_tm) {
    strftime(start_buffer, sizeof(start_buffer), "%b %d %H:%M", start_tm);
  } else {
    snprintf(start_buffer, sizeof(start_buffer), "%lu", (unsigned long)start_time);
  }
  if (end_tm) {
    strftime(end_buffer, sizeof(end_buffer), "%H:%M:%S", end_tm);
  } else {
    snprintf(end_buffer, sizeof(end_buffer), "%lu", (unsigned long)end_time);
  }

  uint32_t duration_seconds = (end_time > start_time) ? (end_time - start_time) : 0;
  uint16_t duration_minutes = duration_seconds / 60;
  uint16_t duration_remain = duration_seconds % 60;

  char max_buffer[16];
  char min_buffer[16];
  char avg_buffer[16];
  format_g_string(max_buffer, sizeof(max_buffer), summary->max_magnitude);
  format_g_string(min_buffer, sizeof(min_buffer), summary->min_magnitude);
  format_g_string(avg_buffer, sizeof(avg_buffer), summary->avg_magnitude);

  snprintf(s_title_buffer, sizeof(s_title_buffer), "Ride %u%s",
           (unsigned int)(index + 1), summary->synced ? "" : " *");

  snprintf(s_body_buffer, sizeof(s_body_buffer),
           "Start: %s\nEnd: %s\nDuration: %u:%02u\nSamples: %u\nG max: %s\nG min: %s\nG avg: %s\nPeak axis (mg)\nX:%d Y:%d Z:%d\nSync: %s\nLink: %s",
           start_buffer,
           end_buffer,
           (unsigned int)duration_minutes,
           (unsigned int)duration_remain,
           (unsigned int)summary->sample_count,
           max_buffer,
           min_buffer,
           avg_buffer,
           summary->peak_x,
           summary->peak_y,
           summary->peak_z,
           summary->synced ? "Complete" : "Pending",
           s_connection_available ? "Connected" : "Waiting");

  text_layer_set_text(s_title_layer, s_title_buffer);
  text_layer_set_text(s_body_layer, s_body_buffer);
}

static void update_idle_display(void) {
  if (s_history.count == 0) {
    snprintf(s_title_buffer, sizeof(s_title_buffer), "Ride Logger");
    snprintf(s_body_buffer, sizeof(s_body_buffer),
             "Press Select to start logging.\nHold Select to sync.\nUp/Down to browse rides.\nLink: %s",
             s_connection_available ? "Connected" : "Waiting for phone");
    text_layer_set_text(s_title_layer, s_title_buffer);
    text_layer_set_text(s_body_layer, s_body_buffer);
  } else {
    if (s_selected_history_index >= s_history.count) {
      s_selected_history_index = s_history.count - 1;
    }
    display_summary(s_selected_history_index);
  }
}

static void update_logging_display(void) {
  if (!s_logging_active) {
    return;
  }

  snprintf(s_title_buffer, sizeof(s_title_buffer), "Logging ride...");

  char current_buffer[16];
  char max_buffer[16];
  char min_buffer[16];

  format_g_string(current_buffer, sizeof(current_buffer), s_current_ride.last_magnitude);
  int16_t min_value = (s_current_ride.min_magnitude == INT16_MAX) ? 0 : s_current_ride.min_magnitude;
  int16_t max_value = (s_current_ride.max_magnitude == INT16_MIN) ? 0 : s_current_ride.max_magnitude;
  format_g_string(max_buffer, sizeof(max_buffer), max_value);
  format_g_string(min_buffer, sizeof(min_buffer), min_value);

  time_t now = time(NULL);
  uint32_t elapsed = (now > s_current_ride.start_time) ? (now - s_current_ride.start_time) : 0;

  snprintf(s_body_buffer, sizeof(s_body_buffer),
           "Elapsed: %lus\nSamples: %u\nCurrent: %s\nMax: %s\nMin: %s\nPeak axis (mg)\nX:%d Y:%d Z:%d\nLink: %s",
           (unsigned long)elapsed,
           (unsigned int)s_current_ride.sample_count,
           current_buffer,
           max_buffer,
           min_buffer,
           s_current_ride.peak_x,
           s_current_ride.peak_y,
           s_current_ride.peak_z,
           s_connection_available ? "OK" : "Lost");

  text_layer_set_text(s_title_layer, s_title_buffer);
  text_layer_set_text(s_body_layer, s_body_buffer);
}

static void accel_data_handler(AccelData *data, uint32_t num_samples) {
  for (uint32_t i = 0; i < num_samples; ++i) {
    if (data[i].did_vibrate) {
      // Skip samples captured during vibrations to avoid noise
      continue;
    }

    int16_t x = data[i].x;
    int16_t y = data[i].y;
    int16_t z = data[i].z;

    int32_t magnitude_sq = (int32_t)x * x + (int32_t)y * y + (int32_t)z * z;
    int16_t magnitude = (int16_t)sqrtf((float)magnitude_sq);

    s_current_ride.last_magnitude = magnitude;
    s_current_ride.sample_count++;
    s_current_ride.magnitude_sum += magnitude;

    if (magnitude > s_current_ride.max_magnitude) {
      s_current_ride.max_magnitude = magnitude;
    }
    if (magnitude < s_current_ride.min_magnitude) {
      s_current_ride.min_magnitude = magnitude;
    }

    int16_t abs_x = x >= 0 ? x : -x;
    int16_t abs_y = y >= 0 ? y : -y;
    int16_t abs_z = z >= 0 ? z : -z;

    if (abs_x > s_current_ride.peak_x) {
      s_current_ride.peak_x = abs_x;
    }
    if (abs_y > s_current_ride.peak_y) {
      s_current_ride.peak_y = abs_y;
    }
    if (abs_z > s_current_ride.peak_z) {
      s_current_ride.peak_z = abs_z;
    }
  }

  update_logging_display();
}

static void send_history_entry(uint8_t index) {
  if (!s_app_message_ready || !s_connection_available) {
    return;
  }
  if (index >= s_history.count) {
    return;
  }
  if (s_pending_sync_index != SYNC_INDEX_NONE) {
    return;
  }

  DictionaryIterator *iter;
  AppMessageResult begin_result = app_message_outbox_begin(&iter);
  if (begin_result != APP_MSG_OK) {
    return;
  }

  RideSummary *summary = &s_history.entries[index];
  uint32_t duration_seconds = (summary->end_time > summary->start_time) ? (summary->end_time - summary->start_time) : 0;

  dict_write_uint8(iter, MESSAGE_KEY_RideId, index);
  dict_write_uint32(iter, MESSAGE_KEY_StartTime, summary->start_time);
  dict_write_uint32(iter, MESSAGE_KEY_EndTime, summary->end_time);
  dict_write_uint32(iter, MESSAGE_KEY_Duration, duration_seconds);
  dict_write_uint16(iter, MESSAGE_KEY_SampleCount, summary->sample_count);
  dict_write_int16(iter, MESSAGE_KEY_MaxG, summary->max_magnitude);
  dict_write_int16(iter, MESSAGE_KEY_MinG, summary->min_magnitude);
  dict_write_int16(iter, MESSAGE_KEY_AvgG, summary->avg_magnitude);
  dict_write_int16(iter, MESSAGE_KEY_PeakX, summary->peak_x);
  dict_write_int16(iter, MESSAGE_KEY_PeakY, summary->peak_y);
  dict_write_int16(iter, MESSAGE_KEY_PeakZ, summary->peak_z);
  dict_write_end(iter);

  AppMessageResult send_result = app_message_outbox_send();
  if (send_result == APP_MSG_OK) {
    s_pending_sync_index = index;
  }
}

static void try_sync(void) {
  if (!s_app_message_ready || !s_connection_available) {
    return;
  }
  if (s_pending_sync_index != SYNC_INDEX_NONE) {
    return;
  }
  for (uint8_t i = 0; i < s_history.count; ++i) {
    if (!s_history.entries[i].synced) {
      send_history_entry(i);
      return;
    }
  }
}

static void finish_logging(void) {
  accel_data_service_unsubscribe();
  s_logging_active = false;
  vibes_double_pulse();

  RideSummary summary;
  memset(&summary, 0, sizeof(summary));
  summary.start_time = s_current_ride.start_time;
  summary.end_time = (uint32_t)time(NULL);
  summary.sample_count = s_current_ride.sample_count;
  summary.max_magnitude = (s_current_ride.max_magnitude == INT16_MIN) ? 0 : s_current_ride.max_magnitude;
  summary.min_magnitude = (s_current_ride.min_magnitude == INT16_MAX) ? summary.max_magnitude : s_current_ride.min_magnitude;
  if (s_current_ride.sample_count > 0) {
    summary.avg_magnitude = (int16_t)(s_current_ride.magnitude_sum / (int32_t)s_current_ride.sample_count);
  } else {
    summary.avg_magnitude = 0;
  }
  summary.peak_x = s_current_ride.peak_x;
  summary.peak_y = s_current_ride.peak_y;
  summary.peak_z = s_current_ride.peak_z;
  summary.synced = 0;
  summary.reserved = 0;

  if (s_history.count < MAX_HISTORY_ENTRIES) {
    s_history.entries[s_history.count] = summary;
    s_selected_history_index = s_history.count;
    s_history.count++;
  } else {
    for (uint8_t i = 1; i < MAX_HISTORY_ENTRIES; ++i) {
      s_history.entries[i - 1] = s_history.entries[i];
    }
    s_history.entries[MAX_HISTORY_ENTRIES - 1] = summary;
    s_selected_history_index = MAX_HISTORY_ENTRIES - 1;
  }

  save_history();
  update_idle_display();
  try_sync();
}

static void start_logging(void) {
  if (s_logging_active) {
    return;
  }

  reset_current_ride();
  s_current_ride.start_time = (uint32_t)time(NULL);
  s_logging_active = true;
  vibes_short_pulse();

  accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
  accel_data_service_subscribe(ACCEL_SAMPLES_PER_BATCH, accel_data_handler);

  update_logging_display();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_logging_active) {
    finish_logging();
  } else {
    start_logging();
  }
}

static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_logging_active) {
    try_sync();
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_logging_active || s_history.count == 0) {
    return;
  }
  if (s_selected_history_index == 0) {
    s_selected_history_index = s_history.count - 1;
  } else {
    s_selected_history_index--;
  }
  display_summary(s_selected_history_index);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_logging_active || s_history.count == 0) {
    return;
  }
  s_selected_history_index++;
  if (s_selected_history_index >= s_history.count) {
    s_selected_history_index = 0;
  }
  display_summary(s_selected_history_index);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 700, select_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void outbox_sent_handler(DictionaryIterator *iterator, void *context) {
  if (s_pending_sync_index != SYNC_INDEX_NONE && s_pending_sync_index < s_history.count) {
    s_history.entries[s_pending_sync_index].synced = 1;
    save_history();
    if (!s_logging_active && s_selected_history_index == s_pending_sync_index) {
      display_summary(s_selected_history_index);
    }
  }
  s_pending_sync_index = SYNC_INDEX_NONE;
  try_sync();
}

static void outbox_failed_handler(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  s_pending_sync_index = SYNC_INDEX_NONE;
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  // Placeholder for future configuration messages or acknowledgements
}

static void connection_handler(bool connected) {
  s_connection_available = connected;
  if (connected) {
    try_sync();
  }
  if (!s_logging_active) {
    update_idle_display();
  } else {
    update_logging_display();
  }
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_title_layer = text_layer_create(GRect(4, 0, bounds.size.w - 8, 60));
  text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_background_color(s_title_layer, GColorClear);
  text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_title_layer));

  s_body_layer = text_layer_create(GRect(4, 60, bounds.size.w - 8, bounds.size.h - 60));
  text_layer_set_font(s_body_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(s_body_layer, GColorClear);
  text_layer_set_text_alignment(s_body_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_body_layer));

  window_set_click_config_provider(window, click_config_provider);

  update_idle_display();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_title_layer);
  text_layer_destroy(s_body_layer);
}

static void init(void) {
  load_history();

  s_connection_available = connection_service_peek_pebble_app_connection();

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers){
      .load = main_window_load,
      .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

  connection_service_subscribe((ConnectionHandlers){
      .pebble_app_connection_handler = connection_handler,
  });

  app_message_register_outbox_sent(outbox_sent_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_register_inbox_received(inbox_received_handler);

  AppMessageResult open_result = app_message_open(512, 512);
  s_app_message_ready = (open_result == APP_MSG_OK);
}

static void deinit(void) {
  if (s_logging_active) {
    accel_data_service_unsubscribe();
  }
  connection_service_unsubscribe();
  app_message_deregister_callbacks();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
