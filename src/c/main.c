#include <pebble.h>

// Window
static Window *s_main_window;
// Layers
static Layer     *s_canvas_layer;
static TextLayer *s_time_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_connection_layer;
// Static variables - battery
static uint8_t s_battery_level = 0;
static bool    s_charging = false;
static char    battery_text[] = "100% remaining";
// Static variables - bluetooth
static bool    s_bt_connected = false;
// Static variables - vibration
static bool    s_vibration = false;
// Static variables - text
static char    s_time_text[] = "00:00";

/********************************** Handlers *********************************/

// Time handler
static void handle_tick(struct tm* tick_time, TimeUnits units_changed) {
  strftime(s_time_text, sizeof(s_time_text), "%T", tick_time);
  text_layer_set_text(s_time_layer, s_time_text);
}

// Battery callback
static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  s_charging = state.is_charging;
  
  if(s_battery_level == 10 && !s_charging)
    vibes_double_pulse();

  // Update meter
  if (s_charging) {
    snprintf(battery_text, sizeof(battery_text), "charging");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%% remaining", s_battery_level);
  }
  text_layer_set_text(s_battery_layer, battery_text);
}

// Bluetooth callback
static void bluetooth_callback(bool connected) {
  s_bt_connected = connected;
  
  if(s_vibration)
    vibes_double_pulse();

  // Update bt
  text_layer_set_text(s_connection_layer, s_bt_connected ? "connected" : "disconnected");
}

/********************************* Draw Layers *******************************/

static void update_canvas(Layer *layer, GContext *ctx){
  GPoint center = (GPoint) {
    .x = 72,
    .y = 295,
  };
  graphics_context_set_fill_color(ctx, GColorBlack);
  // Create round rectangle on canvas layer
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 1);
  //graphics_draw_round_rect(ctx, GRect(0, 117, bounds.size.w, 100), 60);
  graphics_draw_circle(ctx, center, 180);
  
  center = (GPoint) {
    .x = 72,
    .y = 298,
  };
  
  graphics_draw_circle(ctx, center, 180);
  
  GRect battery_level = GRect(90, 130, 30, 30);
  GRect phone_level = GRect(95, 135, 20, 20);
  
  // External circle
  graphics_context_set_stroke_color(ctx, GColorDarkGreen);
  if(s_battery_level == 10){
    graphics_context_set_fill_color(ctx, GColorRed);
  }
  else {
    graphics_context_set_fill_color(ctx, GColorGreen);
  }
  graphics_draw_circle(ctx, grect_center_point(&battery_level), 16);
  graphics_fill_radial(ctx, battery_level, GOvalScaleModeFitCircle, 5, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE((s_battery_level*360)/100));
  // Inner circle
  graphics_context_set_stroke_color(ctx, GColorDarkGreen);
  graphics_context_set_fill_color(ctx, GColorYellow);
  graphics_draw_circle(ctx, grect_center_point(&battery_level), 10);
  graphics_fill_radial(ctx, phone_level, GOvalScaleModeFitCircle, 5, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(270));
  
  // Bluetooth status
  if(s_bt_connected){
     graphics_context_set_fill_color(ctx, GColorBlue);
  }
  else {
    graphics_context_set_fill_color(ctx, GColorRed);
  }
  
  center = (GPoint) {
    .x = 39,
    .y = 145,
  };
  graphics_fill_circle(ctx, center, 15);
}
/*********************************** Windows *********************************/
static void main_window_load(Window *window) {
  
  // Disable vibration at startup
  s_vibration = false;
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_time_layer = text_layer_create(GRect(0, 20, bounds.size.w, 54));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorRed);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  s_connection_layer = text_layer_create(GRect(0, 70, bounds.size.w, 34));
  text_layer_set_text_color(s_connection_layer, GColorWhite);
  text_layer_set_background_color(s_connection_layer, GColorClear);
  text_layer_set_font(s_connection_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_connection_layer, GTextAlignmentCenter);
  bluetooth_callback(connection_service_peek_pebble_app_connection());

  s_battery_layer = text_layer_create(GRect(0, 90, bounds.size.w, 34));
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  text_layer_set_text(s_battery_layer, "100% charged");

  // Retrieve time
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  handle_tick(current_time, MINUTE_UNIT);
  
  // Subscribe to tick handler
  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  
  // Subscribe to battery service
  battery_state_service_subscribe(battery_callback);

  // Subscribe to bluetooth service
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });
  
  // Create canvas
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, update_canvas);
  // Add layers
  layer_add_child(window_layer, s_canvas_layer);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_connection_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));

  // Update battery
  battery_callback(battery_state_service_peek());
  
  // Update connection
  bluetooth_callback(connection_service_peek_pebble_app_connection());
  
  // Enable vibration after startup
  s_vibration = true;
}

static void main_window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_connection_layer);
  text_layer_destroy(s_battery_layer);
  layer_destroy(s_canvas_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}