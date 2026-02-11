#include <pebble.h>


// Forward declarations
static void update_colors();

static Window *s_main_window;
static Layer *s_canvas_layer;

static TextLayer *s_hour_layer;
static TextLayer *s_month_layer;
static TextLayer *s_day_layer;
static TextLayer *s_minute_layer;
static TextLayer *s_second_layer;
static TextLayer *s_step_name_layer;
static TextLayer *s_battery_name_layer;
static TextLayer *s_step_value_layer;
static TextLayer *s_battery_value_layer;

static GColor s_background_color;
static GColor s_accent_color;

static char s_hour_buffer[4];
static char s_month_buffer[8];
static char s_day_buffer[4];
static char s_minute_buffer[4];
static char s_second_buffer[4];
static char s_step_buffer[8];
static char s_battery_buffer[8];


// Load settings
static void load_settings() {
  s_background_color = persist_exists(MESSAGE_KEY_PRIMARY_COLOR) ? (GColor){ .argb = (uint8_t)persist_read_int(MESSAGE_KEY_PRIMARY_COLOR) } : GColorWhite;
  s_accent_color = persist_exists(MESSAGE_KEY_SECONDARY_COLOR) ? (GColor){ .argb = (uint8_t)persist_read_int(MESSAGE_KEY_SECONDARY_COLOR) } : GColorBlueMoon;
}

// Save settings
static void save_settings() {
  persist_write_int(MESSAGE_KEY_PRIMARY_COLOR, s_background_color.argb);
  persist_write_int(MESSAGE_KEY_SECONDARY_COLOR, s_accent_color.argb);
}

// Inbox received callback
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
 
  Tuple *bgcolor_tuple = dict_find(iterator, MESSAGE_KEY_PRIMARY_COLOR);
  if (bgcolor_tuple) {
    s_background_color = GColorFromHEX(bgcolor_tuple->value->int32);
  }

  Tuple *accentcolor_tuple = dict_find(iterator, MESSAGE_KEY_SECONDARY_COLOR);
  if (accentcolor_tuple) {
    s_accent_color = GColorFromHEX(accentcolor_tuple->value->int32);
  }

  save_settings();
  update_colors();
  layer_mark_dirty(s_canvas_layer);
  
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int half_height = bounds.size.h / 2;
  int center_x = bounds.size.w / 2;
  int circle_radius = 15;
  
  // Top half - Red background
  graphics_context_set_fill_color(ctx, s_accent_color);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, half_height), 0, GCornerNone);
  
  // Bottom half - Black background
  graphics_context_set_fill_color(ctx, s_background_color);
  graphics_fill_rect(ctx, GRect(0, half_height, bounds.size.w, half_height), 0, GCornerNone);
  
  // Black circle behind month (on red background)
  graphics_context_set_fill_color(ctx, s_background_color);
  graphics_fill_circle(ctx, GPoint(center_x - 15, half_height), circle_radius);
  
  // Red circle behind day (on black background)
  graphics_context_set_fill_color(ctx, s_accent_color);
  graphics_fill_circle(ctx, GPoint(center_x + 16, half_height), circle_radius);
}

static void update_colors() {
  text_layer_set_text_color(s_hour_layer, s_background_color);
  text_layer_set_text_color(s_month_layer, s_accent_color);
  text_layer_set_text_color(s_day_layer, s_background_color);
  text_layer_set_text_color(s_minute_layer, s_accent_color);
  text_layer_set_text_color(s_second_layer, s_accent_color);
  text_layer_set_text_color(s_step_name_layer, s_background_color);
  text_layer_set_text_color(s_battery_name_layer, s_background_color);
  text_layer_set_text_color(s_step_value_layer, s_accent_color);
  text_layer_set_text_color(s_battery_value_layer, s_accent_color);
}

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // Hour (12-hour format)
  strftime(s_hour_buffer, sizeof(s_hour_buffer), clock_is_24h_style() ? "%H" : "%I", tick_time);
  // Remove leading zero from hour if present
  if (s_hour_buffer[0] == '0') {
    memmove(s_hour_buffer, s_hour_buffer + 1, sizeof(s_hour_buffer) - 1);
  }
  text_layer_set_text(s_hour_layer, s_hour_buffer);
  
  // Month (abbreviated)
  strftime(s_month_buffer, sizeof(s_month_buffer), "%b", tick_time);
  // Convert to uppercase
  for (int i = 0; s_month_buffer[i]; i++) {
    if (s_month_buffer[i] >= 'a' && s_month_buffer[i] <= 'z') {
      s_month_buffer[i] -= 32;
    }
  }
  text_layer_set_text(s_month_layer, s_month_buffer);
  
  // Day of month
  strftime(s_day_buffer, sizeof(s_day_buffer), "%d", tick_time);
  text_layer_set_text(s_day_layer, s_day_buffer);
  
  // Minutes
  strftime(s_minute_buffer, sizeof(s_minute_buffer), "%M", tick_time);
  text_layer_set_text(s_minute_layer, s_minute_buffer);
  
  // Seconds
  strftime(s_second_buffer, sizeof(s_second_buffer), "%S", tick_time);
  text_layer_set_text(s_second_layer, s_second_buffer);

  text_layer_set_text(s_step_name_layer, "Steps");
  text_layer_set_text(s_battery_name_layer, "Batt");
}

static void update_battery(BatteryChargeState charge_state) {
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", charge_state.charge_percent);
  text_layer_set_text(s_battery_value_layer, s_battery_buffer);
}

static void update_steps() {
  HealthMetric metric = HealthMetricStepCount;
  time_t start = time_start_of_today();
  time_t end = time(NULL);
  
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);
  if (mask & HealthServiceAccessibilityMaskAvailable) {
    int steps = (int)health_service_sum_today(metric);
    snprintf(s_step_buffer, sizeof(s_step_buffer), "%d", steps);
  } else {
    snprintf(s_step_buffer, sizeof(s_step_buffer), "--");
  }
  text_layer_set_text(s_step_value_layer, s_step_buffer);
}

static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventSignificantUpdate || event == HealthEventMovementUpdate) {
    update_steps();
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  int half_height = bounds.size.h / 2;
  int center_x = bounds.size.w / 2;
  
  // Create canvas layer for background
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);
  
  // Hour layer - large text on red background (top half)
  s_hour_layer = text_layer_create(GRect(0, (bounds.size.h / 2) - 70, bounds.size.w, 70));
  text_layer_set_background_color(s_hour_layer, GColorClear);
  text_layer_set_text_color(s_hour_layer, s_background_color);
  text_layer_set_font(s_hour_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(s_hour_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_hour_layer));
  
  // Minute layer - large text on black background (bottom half)
  s_minute_layer = text_layer_create(GRect(0, (bounds.size.h / 2) + 12, bounds.size.w, 70));
  text_layer_set_background_color(s_minute_layer, GColorClear);
  text_layer_set_text_color(s_minute_layer, s_accent_color);
  text_layer_set_font(s_minute_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(s_minute_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_minute_layer));
  
  // Second layer - smaller text at bottom on black background
  s_second_layer = text_layer_create(GRect(0, bounds.size.h - 25, bounds.size.w, 30));
  text_layer_set_background_color(s_second_layer, GColorClear);
  text_layer_set_text_color(s_second_layer, s_accent_color);
  text_layer_set_font(s_second_layer, fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_second_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_second_layer));
  
  // Month layer - on black circle, left side of center
  s_month_layer = text_layer_create(GRect(center_x - 29, half_height - 10, 30, 26));
  text_layer_set_background_color(s_month_layer, GColorClear);
  text_layer_set_text_color(s_month_layer, s_accent_color);
  text_layer_set_font(s_month_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_month_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_month_layer));
  
  // Day layer - on red circle, right side of center
  s_day_layer = text_layer_create(GRect(center_x + 1, half_height - 10, 30, 26));
  text_layer_set_background_color(s_day_layer, GColorClear);
  text_layer_set_text_color(s_day_layer, s_background_color);
  text_layer_set_font(s_day_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_day_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_day_layer));

  int set_w_position = PBL_IF_ROUND_ELSE(10, 0);
  if (bounds.size.w > 144) {
    set_w_position = 10;
  }

  // Step name layer - smaller text above seconds on black background
  s_step_name_layer = text_layer_create(GRect(set_w_position, bounds.size.h / 2 - 18, 40, 30));
  text_layer_set_background_color(s_step_name_layer, GColorClear);
  text_layer_set_text_color(s_step_name_layer, s_background_color);
  text_layer_set_font(s_step_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_step_name_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_step_name_layer));

  // Step value layer - below step name on black background
  s_step_value_layer = text_layer_create(GRect(set_w_position, bounds.size.h / 2, 40, 20));
  text_layer_set_background_color(s_step_value_layer, GColorClear);
  text_layer_set_text_color(s_step_value_layer, s_accent_color);
  text_layer_set_font(s_step_value_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_step_value_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_step_value_layer));

  int set_b_position = PBL_IF_ROUND_ELSE(bounds.size.w - 50, bounds.size.w - 40);
  if (bounds.size.w > 144) {
    set_b_position = bounds.size.w - 50;
  }

  // Battery name layer - smaller text above seconds on black background
  s_battery_name_layer = text_layer_create(GRect(set_b_position, bounds.size.h / 2 - 18, 40, 30));
  text_layer_set_background_color(s_battery_name_layer, GColorClear);
  text_layer_set_text_color(s_battery_name_layer, s_background_color);
  text_layer_set_font(s_battery_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_battery_name_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_name_layer));

  // Battery value layer - below battery name on black background
  s_battery_value_layer = text_layer_create(GRect(set_b_position, bounds.size.h / 2, 40, 20));
  text_layer_set_background_color(s_battery_value_layer, GColorClear);
  text_layer_set_text_color(s_battery_value_layer, s_accent_color);
  text_layer_set_font(s_battery_value_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_battery_value_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_value_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_hour_layer);
  text_layer_destroy(s_month_layer);
  text_layer_destroy(s_day_layer);
  text_layer_destroy(s_minute_layer);
  text_layer_destroy(s_second_layer);
  text_layer_destroy(s_step_name_layer);
  text_layer_destroy(s_battery_name_layer);
  text_layer_destroy(s_step_value_layer);
  text_layer_destroy(s_battery_value_layer);
  layer_destroy(s_canvas_layer);
}

static void init() {
  load_settings();
  s_main_window = window_create();
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  // Register with BatteryStateService
  battery_state_service_subscribe(update_battery);
  
  // Register with HealthService for steps
  health_service_events_subscribe(health_handler, NULL);
  
  // Initial time display
  update_time();
  
  // Initial battery and steps display
  update_battery(battery_state_service_peek());
  update_steps();

  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(128, 64);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
