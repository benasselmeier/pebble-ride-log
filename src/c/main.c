#include <pebble.h>

// Simple menu window and layer
static Window *main_window;
static MenuLayer *main_menu_layer;

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
  return NUM_MENU_ITEMS;
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  menu_cell_basic_header_draw(ctx, cell_layer, "Forces Recorder");
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  menu_cell_basic_draw(ctx, cell_layer, menu_items[cell_index->row], NULL, NULL);
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  // Simple vibration feedback for now
  vibes_short_pulse();
}

// Main window functions
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create the menu layer
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
  
  // Let the menu layer handle navigation
  menu_layer_set_click_config_onto_window(main_menu_layer, window);
  
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
  window_destroy(main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}
