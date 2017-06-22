#include "pebble.h"

static Window *s_window;
static TextLayer *s_time_layer;
static GDrawCommandImage *s_symbol_image;
static Layer *s_canvas_layer;
//static GFont s_font;

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  static char s_time_text[] = "00:00";
  //static char s_date_text[] = "Xxxxxxxxx 00";

  char *time_format = clock_is_24h_style() ? "%R" : "%I:%M";
  strftime(s_time_text, sizeof(s_time_text), time_format, tick_time);

  // Handle lack of non-padded hour format string for twelve hour clock.
  if (!clock_is_24h_style() && (s_time_text[0] == '0')) {
    memmove(s_time_text, &s_time_text[1], sizeof(s_time_text) - 1);
  }
  text_layer_set_text(s_time_layer, s_time_text);

  // Update day, only if it's changed
  /*
  if (units_changed & DAY_UNIT) {
    strftime(s_date_text, sizeof(s_date_text), "%B %e", tick_time);
    text_layer_set_text(s_date_layer, s_date_text);
  }
  */
}

static void update_proc(Layer *layer, GContext *ctx) {
  // Set the origin offset from the context for drawing the image
  GPoint origin = GPoint(0, 0);

  // Draw the GDrawCommandImage to the GContext
  gdraw_command_image_draw(ctx, s_symbol_image, origin);
}

static void window_load(Window *window) {
  // Get info about the main window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_unobstructed_bounds(window_layer);

  // Create & position text layer
  s_time_layer = text_layer_create(GRect(0, bounds.size.h-60, bounds.size.w, 50));

  // Load the custom font
  //s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SYMBOL_FONT_42));

  // Style text layer
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  //text_layer_set_font(s_time_layer, s_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add text layer to window
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Create the canvas layer
  const int symbol_w = 100;
  const int symbol_h = 100;
  const int symbol_x = (bounds.size.w / 2) - (symbol_w / 2);
  s_canvas_layer = layer_create(GRect(symbol_x, 10, bounds.size.w, bounds.size.h));

  // Set the LayerUpdateProc
  layer_set_update_proc(s_canvas_layer, update_proc);

  // Add to parent Window
  layer_add_child(window_layer, s_canvas_layer);
}

static void window_unload(Window *window) {
  // Destroy text layer
  text_layer_destroy(s_time_layer);

  // Destroy canvas layer
  layer_destroy(s_canvas_layer);

  // Destroy symbol image
  gdraw_command_image_destroy(s_symbol_image);
}

static void init(void) {
  // Create main window
  s_window = window_create();

  // Set background color
  window_set_background_color(s_window, COLOR_FALLBACK(GColorFromHEX(0x3c998b), GColorWhite));

  // Set handlers to manage elements inside main window
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  // Create the symbol image from resource file
  s_symbol_image = gdraw_command_image_create_with_resource(RESOURCE_ID_SYMBOL_IMAGE);

  // Show window on the watch, animated
  const bool animated = true;
  window_stack_push(s_window, animated);

  setlocale(LC_ALL, "");

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

  // Prevent starting blank
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  handle_minute_tick(t, DAY_UNIT);
}

static void deinit(void) {
  // Destroy main window
  window_destroy(s_window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  deinit();
}
