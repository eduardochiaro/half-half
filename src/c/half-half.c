#include <pebble.h>

// Forward declarations
static void update_colors();
static void update_time();
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void disable_seconds_callback(void *data);

static Window *s_main_window;
static Layer *s_canvas_layer;

static TextLayer *s_hour_layer;
static TextLayer *s_month_layer;
static TextLayer *s_day_layer;
static TextLayer *s_minute_layer;
static TextLayer *s_second_layer;
#if defined(PBL_HEALTH)
static TextLayer *s_step_name_layer;
#endif
static TextLayer *s_battery_name_layer;
#if defined(PBL_HEALTH)
static TextLayer *s_step_value_layer;
#endif
static TextLayer *s_battery_value_layer;

static GColor s_background_color;
static GColor s_accent_color;
static bool s_battery_save_enabled;
static bool s_show_seconds;

static char s_hour_buffer[4];
static char s_month_buffer[8];
static char s_day_buffer[4];
static char s_minute_buffer[4];
static char s_second_buffer[4];
#if defined(PBL_HEALTH)
static char s_step_buffer[8];
#endif
static char s_battery_buffer[8];

static bool s_is_focused = true;
static AppTimer *s_seconds_timer = NULL;

#if defined(PBL_PLATFORM_EMERY)
static bool is_large_screen = true;
#else
static bool is_large_screen = false;
#endif

#define SECONDS_DISPLAY_DURATION 10000  // Show seconds for 10 seconds after interaction


// Load settings
static void load_settings() {
  s_background_color = persist_exists(MESSAGE_KEY_PRIMARY_COLOR) ? (GColor){ .argb = (uint8_t)persist_read_int(MESSAGE_KEY_PRIMARY_COLOR) } : GColorWhite;
  s_accent_color = persist_exists(MESSAGE_KEY_SECONDARY_COLOR) ? (GColor){ .argb = (uint8_t)persist_read_int(MESSAGE_KEY_SECONDARY_COLOR) } : GColorBlueMoon;
  s_show_seconds = persist_exists(MESSAGE_KEY_SHOW_SECONDS) ? persist_read_bool(MESSAGE_KEY_SHOW_SECONDS) : true;
  s_battery_save_enabled = persist_exists(MESSAGE_KEY_BATTERY_SAVE_SECONDS) ? persist_read_bool(MESSAGE_KEY_BATTERY_SAVE_SECONDS) : false;
}

// Save settings
static void save_settings() {
  persist_write_int(MESSAGE_KEY_PRIMARY_COLOR, s_background_color.argb);
  persist_write_int(MESSAGE_KEY_SECONDARY_COLOR, s_accent_color.argb);
  persist_write_bool(MESSAGE_KEY_SHOW_SECONDS, s_show_seconds);
  persist_write_bool(MESSAGE_KEY_BATTERY_SAVE_SECONDS, s_battery_save_enabled);
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

  Tuple *show_seconds_tuple = dict_find(iterator, MESSAGE_KEY_SHOW_SECONDS);
  if (show_seconds_tuple) {
    bool new_show_seconds = show_seconds_tuple->value->int32 == 1;
    
    if (new_show_seconds != s_show_seconds) {
      s_show_seconds = new_show_seconds;
      
      if (s_show_seconds) {
        // Seconds were enabled - show the layer and subscribe to appropriate timer
        layer_set_hidden(text_layer_get_layer(s_second_layer), false);
        tick_timer_service_unsubscribe();
        tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
        s_is_focused = true;
        update_time();
        
        // Start battery save timer if enabled
        if (s_battery_save_enabled) {
          if (s_seconds_timer) {
            app_timer_cancel(s_seconds_timer);
          }
          s_seconds_timer = app_timer_register(SECONDS_DISPLAY_DURATION, 
            disable_seconds_callback, NULL);
        }
      } else {
        // Seconds were disabled - hide the layer and switch to minute updates
        layer_set_hidden(text_layer_get_layer(s_second_layer), true);
        if (s_seconds_timer) {
          app_timer_cancel(s_seconds_timer);
          s_seconds_timer = NULL;
        }
        tick_timer_service_unsubscribe();
        tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
        s_is_focused = false;
      }
    }
  }

  Tuple *battery_save_tuple = dict_find(iterator, MESSAGE_KEY_BATTERY_SAVE_SECONDS);
  if (battery_save_tuple) {
    s_battery_save_enabled = battery_save_tuple->value->int32 == 1;
    
    // Only apply battery saving logic if seconds are enabled
    if (s_show_seconds) {
      // If battery saving is disabled, make sure seconds are always on
      if (!s_battery_save_enabled && !s_is_focused) {
        s_is_focused = true;
        if (s_seconds_timer) {
          app_timer_cancel(s_seconds_timer);
          s_seconds_timer = NULL;
        }
        tick_timer_service_unsubscribe();
        tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
        update_time();
      } else if (s_battery_save_enabled && s_is_focused) {
        // Battery saving was just enabled - start the timer
        if (s_seconds_timer) {
          app_timer_cancel(s_seconds_timer);
        }
        s_seconds_timer = app_timer_register(SECONDS_DISPLAY_DURATION, 
          disable_seconds_callback, NULL);
      }
    }
  }

  save_settings();
  update_colors();
  layer_mark_dirty(s_canvas_layer);
  
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int half_height = bounds.size.h / 2;
  int center_x = bounds.size.w / 2;
  
  // Scale circle radius based on screen size
  int circle_radius = is_large_screen ? 22 : 15;
  int circle_spacing = is_large_screen ? 22 : 15;
  
  // Top half - Red background
  graphics_context_set_fill_color(ctx, s_accent_color);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, half_height), 0, GCornerNone);
  
  // Bottom half - Black background
  graphics_context_set_fill_color(ctx, s_background_color);
  graphics_fill_rect(ctx, GRect(0, half_height, bounds.size.w, half_height), 0, GCornerNone);
  
  // Black circle behind month (on red background)
  graphics_context_set_fill_color(ctx, s_background_color);
  graphics_fill_circle(ctx, GPoint(center_x - circle_spacing, half_height), circle_radius);
  
  // Red circle behind day (on black background)
  graphics_context_set_fill_color(ctx, s_accent_color);
  graphics_fill_circle(ctx, GPoint(center_x + circle_spacing + 1, half_height), circle_radius);
}

static void update_colors() {
  text_layer_set_text_color(s_hour_layer, s_background_color);
  text_layer_set_text_color(s_month_layer, s_accent_color);
  text_layer_set_text_color(s_day_layer, s_background_color);
  text_layer_set_text_color(s_minute_layer, s_accent_color);
  text_layer_set_text_color(s_second_layer, s_accent_color);
#if defined(PBL_HEALTH)
  text_layer_set_text_color(s_step_name_layer, s_background_color);
#endif
  text_layer_set_text_color(s_battery_name_layer, s_background_color);
#if defined(PBL_HEALTH)
  text_layer_set_text_color(s_step_value_layer, s_accent_color);
#endif
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
  
  // Seconds (only update when focused and show_seconds is enabled)
  if (s_show_seconds && s_is_focused) {
    strftime(s_second_buffer, sizeof(s_second_buffer), "%S", tick_time);
    text_layer_set_text(s_second_layer, s_second_buffer);
  }

#if defined(PBL_HEALTH)
  text_layer_set_text(s_step_name_layer, "Steps");
#endif
  text_layer_set_text(s_battery_name_layer, "Batt");
}

static void update_battery(BatteryChargeState charge_state) {
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", charge_state.charge_percent);
  text_layer_set_text(s_battery_value_layer, s_battery_buffer);
}

#if defined(PBL_HEALTH)
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
#endif

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void disable_seconds_callback(void *data) {
  // Only disable if battery saving is enabled and seconds are shown
  if (!s_battery_save_enabled || !s_show_seconds) {
    return;
  }
  
  // Timeout reached - disable seconds display
  s_is_focused = false;
  s_seconds_timer = NULL;
  tick_timer_service_unsubscribe();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  text_layer_set_text(s_second_layer, "");
}

static void enable_seconds_display(void) {
  // Only enable if seconds are shown
  if (!s_show_seconds) {
    return;
  }
  
  // Only enable battery saving timer if the feature is enabled
  if (!s_battery_save_enabled) {
    if (!s_is_focused) {
      s_is_focused = true;
      tick_timer_service_unsubscribe();
      tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
      update_time();
    }
    return;
  }
  
  if (!s_is_focused) {
    s_is_focused = true;
    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
    update_time();
  }
  
  // Cancel existing timer if any
  if (s_seconds_timer) {
    app_timer_cancel(s_seconds_timer);
  }
  
  // Set new timer to disable seconds after delay
  s_seconds_timer = app_timer_register(SECONDS_DISPLAY_DURATION, 
    disable_seconds_callback, NULL);
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  // Only enable seconds on tap if battery saving is enabled and seconds are shown
  if (s_battery_save_enabled && s_show_seconds) {
    enable_seconds_display();
  }
}

static void unobstructed_area_change_handler(AnimationProgress progress, void *context) {
  // This fires when the watchface becomes visible or covered
  // We'll check visibility in the did_change handler
}

static void unobstructed_area_did_change(void *context) {
  // Only use battery saving logic if enabled and seconds are shown
  if (!s_battery_save_enabled || !s_show_seconds) {
    return;
  }
  
  GRect unobstructed = layer_get_unobstructed_bounds(window_get_root_layer(s_main_window));
  GRect full_bounds = layer_get_bounds(window_get_root_layer(s_main_window));
  
  // Check if watchface is visible
  bool is_visible = grect_equal(&unobstructed, &full_bounds);
  
  if (is_visible && !s_is_focused) {
    // Watchface became visible - enable seconds temporarily
    enable_seconds_display();
  } else if (!is_visible && s_is_focused) {
    // Watchface is covered - disable seconds immediately
    s_is_focused = false;
    if (s_seconds_timer) {
      app_timer_cancel(s_seconds_timer);
      s_seconds_timer = NULL;
    }
    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    text_layer_set_text(s_second_layer, "");
  }
}

static void focus_handler(bool in_focus) {
  // Only use battery saving logic if enabled and seconds are shown
  if (!s_battery_save_enabled || !s_show_seconds) {
    return;
  }
  
  if (in_focus && !s_is_focused) {
    // App gained focus - enable seconds temporarily
    enable_seconds_display();
  } else if (!in_focus && s_is_focused) {
    // App lost focus - disable seconds immediately
    s_is_focused = false;
    if (s_seconds_timer) {
      app_timer_cancel(s_seconds_timer);
      s_seconds_timer = NULL;
    }
    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    text_layer_set_text(s_second_layer, "");
  }
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
  
  // Scale positioning and fonts based on screen size
  int hour_offset = is_large_screen ? 85 : 70;
  int minute_offset = is_large_screen ? 25: 12;
  
  // Hour layer - large text on red background (top half)
  s_hour_layer = text_layer_create(GRect(0, (bounds.size.h / 2) - hour_offset, bounds.size.w, hour_offset));
  text_layer_set_background_color(s_hour_layer, GColorClear);
  text_layer_set_text_color(s_hour_layer, s_background_color);
  text_layer_set_font(s_hour_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(s_hour_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_hour_layer));
  
  // Minute layer - large text on black background (bottom half)
  s_minute_layer = text_layer_create(GRect(0, (bounds.size.h / 2) + minute_offset, bounds.size.w, 70));
  text_layer_set_background_color(s_minute_layer, GColorClear);
  text_layer_set_text_color(s_minute_layer, s_accent_color);
  text_layer_set_font(s_minute_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(s_minute_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_minute_layer));
  
  // Second layer - smaller text at bottom on black background
  int seconds_height = is_large_screen ? 35 : 30;
  s_second_layer = text_layer_create(GRect(0, bounds.size.h - seconds_height, bounds.size.w, seconds_height));
  text_layer_set_background_color(s_second_layer, GColorClear);
  text_layer_set_text_color(s_second_layer, s_accent_color);
  text_layer_set_font(s_second_layer, fonts_get_system_font(is_large_screen ? FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM : FONT_KEY_LECO_20_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_second_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_second_layer));
  
  // Month and day positioning - adjust for larger circles
  int circle_spacing = is_large_screen ? 20 : 13;
  int date_width = is_large_screen ? 40 : 30;
  int date_height = is_large_screen ? 30 : 26;
  int date_offset = is_large_screen ? 13 : 10;
  
  // Month layer - on black circle, left side of center
  s_month_layer = text_layer_create(GRect(center_x - circle_spacing - (date_width/2) - 1, half_height - date_offset, date_width, date_height));
  text_layer_set_background_color(s_month_layer, GColorClear);
  text_layer_set_text_color(s_month_layer, s_accent_color);
  text_layer_set_font(s_month_layer, fonts_get_system_font(is_large_screen ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_month_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_month_layer));
  
  // Day layer - on red circle, right side of center
  s_day_layer = text_layer_create(GRect(center_x + circle_spacing - (date_width/2) + 1, half_height - date_offset, date_width, date_height));
  text_layer_set_background_color(s_day_layer, GColorClear);
  text_layer_set_text_color(s_day_layer, s_background_color);
  text_layer_set_font(s_day_layer, fonts_get_system_font(is_large_screen ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_day_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_day_layer));

#if defined(PBL_HEALTH)
  int set_w_position = PBL_IF_ROUND_ELSE(10, 0);
  int info_width = 40;
  
  if (is_large_screen) {
    set_w_position = 3;
    info_width = 50;
  }

  // Step name layer - smaller text above seconds on black background
  s_step_name_layer = text_layer_create(GRect(set_w_position, bounds.size.h / 2 - 22, info_width, 30));
  text_layer_set_background_color(s_step_name_layer, GColorClear);
  text_layer_set_text_color(s_step_name_layer, s_background_color);
  text_layer_set_font(s_step_name_layer, fonts_get_system_font(is_large_screen ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_step_name_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_step_name_layer));

  // Step value layer - below step name on black background
  s_step_value_layer = text_layer_create(GRect(set_w_position, bounds.size.h / 2 + 2, info_width, 24));
  text_layer_set_background_color(s_step_value_layer, GColorClear);
  text_layer_set_text_color(s_step_value_layer, s_accent_color);
  text_layer_set_font(s_step_value_layer, fonts_get_system_font(is_large_screen ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_step_value_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_step_value_layer));
#endif

  int set_b_position = PBL_IF_ROUND_ELSE(bounds.size.w - 50, bounds.size.w - 40);
  int batt_width = 40;
  
  if (is_large_screen) {
    set_b_position = bounds.size.w - 53;
    batt_width = 50;
  }

  // Battery name layer - smaller text above seconds on black background
  s_battery_name_layer = text_layer_create(GRect(set_b_position, bounds.size.h / 2 - 22, batt_width, 30));
  text_layer_set_background_color(s_battery_name_layer, GColorClear);
  text_layer_set_text_color(s_battery_name_layer, s_background_color);
  text_layer_set_font(s_battery_name_layer, fonts_get_system_font(is_large_screen ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_battery_name_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_name_layer));

  // Battery value layer - below battery name on black background
  s_battery_value_layer = text_layer_create(GRect(set_b_position, bounds.size.h / 2 + 2, batt_width, 24));
  text_layer_set_background_color(s_battery_value_layer, GColorClear);
  text_layer_set_text_color(s_battery_value_layer, s_accent_color);
  text_layer_set_font(s_battery_value_layer, fonts_get_system_font(is_large_screen ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_battery_value_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_value_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_hour_layer);
  text_layer_destroy(s_month_layer);
  text_layer_destroy(s_day_layer);
  text_layer_destroy(s_minute_layer);
  text_layer_destroy(s_second_layer);
#if defined(PBL_HEALTH)
  text_layer_destroy(s_step_name_layer);
#endif
  text_layer_destroy(s_battery_name_layer);
#if defined(PBL_HEALTH)
  text_layer_destroy(s_step_value_layer);
#endif
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
  
  // Register with TickTimerService - use appropriate unit based on settings
  if (s_show_seconds) {
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  } else {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    s_is_focused = false;
    layer_set_hidden(text_layer_get_layer(s_second_layer), true);
  }
  
  // Register with AppFocusService for battery saving
  app_focus_service_subscribe(focus_handler);
  
  // Register with AccelTapService to wake seconds on wrist movement
  accel_tap_service_subscribe(accel_tap_handler);
  
  // Register with UnobstructedAreaService to detect when watchface is visible
  UnobstructedAreaHandlers unobstructed_handlers = {
    .change = unobstructed_area_change_handler,
    .did_change = unobstructed_area_did_change
  };
  unobstructed_area_service_subscribe(unobstructed_handlers, NULL);
  
  // Register with BatteryStateService
  battery_state_service_subscribe(update_battery);
  
#if defined(PBL_HEALTH)
  // Register with HealthService for steps
  health_service_events_subscribe(health_handler, NULL);
#endif
  
  // Initial time display
  update_time();
  
  // Initial battery and steps display
  update_battery(battery_state_service_peek());
#if defined(PBL_HEALTH)
  update_steps();
#endif

  // Start timer to automatically disable seconds after initial display only if battery saving and seconds are enabled
  if (s_battery_save_enabled && s_show_seconds) {
    s_seconds_timer = app_timer_register(SECONDS_DISPLAY_DURATION, 
      disable_seconds_callback, NULL);
  }

  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(128, 64);
}

static void deinit() {
  // Cancel any active timer
  if (s_seconds_timer) {
    app_timer_cancel(s_seconds_timer);
  }
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
