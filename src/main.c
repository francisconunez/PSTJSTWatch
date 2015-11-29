#include <pebble.h>
#define KEY_TIMEZONE_OFFSET 0
static Window *s_main_window;
static TextLayer *pst_time_layer, *pst_date_layer, 
                 *jst_time_layer, *jst_date_layer, 
                 *pst_layer, *jst_layer;
static Layer *s_battery_layer;
static InverterLayer *inverter_layer;
static GFont s_time_font, s_date_font, tz_font;


static int s_battery_level;
static int s_utcOffset;
static bool s_battery_plugged;
static struct tm zulu_tick_time;


static void inbox_received_callback(DictionaryIterator *received, void *context) {
  Tuple *timezone_offset_tuple = dict_find(received, 0);

  if (timezone_offset_tuple) {
    int32_t timezone_offset = timezone_offset_tuple->value->int32;

    // Calculate UTC time
    s_utcOffset = timezone_offset;
    
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  s_battery_plugged = state.is_plugged;
  // Update meter
  layer_mark_dirty(s_battery_layer);
}

static void update_time() {
  //time_t local, utc;
  //local = time(NULL);

		//utc = local + s_utcOffset;


		//zulu_tick_time = *(localtime(&utc));
  
  
  time_t temp, utc, jst, pst; 
  temp = time(NULL);
  utc = time(NULL);
  struct tm *tick_time = localtime(&temp);
  utc = utc + s_utcOffset; //utc time
  jst = utc + 32400; //utc to jst hours->min->sec
  pst = utc + (-28800);
  struct tm *tick_time2 = localtime(&jst);

  // Create a long-lived buffer, and show the time
  static char pstBuffer[] = "00:00AM";
  static char jstBuffer[] = "00:00AM";
  
  //Display JST time
  if(clock_is_24h_style()) {
    strftime(jstBuffer, sizeof("00:00"), "%H:%M", tick_time2);
    text_layer_set_text(jst_time_layer, jstBuffer);
  } else {
    strftime(jstBuffer, sizeof("00:00AM"), "%I:%M%p", tick_time2);
    text_layer_set_text(jst_time_layer, jstBuffer+(('0' == jstBuffer[0])?1:0));
  }
  //Display JST date
  static char jst_date_buffer[16];
  strftime(jst_date_buffer, sizeof(jst_date_buffer), "%a.%b.%d%n%Y", tick_time2);
  text_layer_set_text(jst_date_layer, jst_date_buffer);
  
  
  //Display PST time
  tick_time = localtime(&pst);
  if(clock_is_24h_style()) {
    strftime(pstBuffer, sizeof("00:00"), "%H:%M", tick_time);
    text_layer_set_text(pst_time_layer, pstBuffer);
  } else {
    strftime(pstBuffer, sizeof("00:00AM"), "%I:%M%p", tick_time);
    text_layer_set_text(pst_time_layer, pstBuffer+(('0' == pstBuffer[0])?1:0));
  }
    
  //Display JST date
  static char pst_date_buffer[16];
  strftime(pst_date_buffer, sizeof(pst_date_buffer), "%a.%b.%d%n%Y", tick_time);
  text_layer_set_text(pst_date_layer, pst_date_buffer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  // Find the height of the bar

  int height = (int)(float)(((float)s_battery_level / 100.0F) * 168.0F);
  int heightOffset = 168 - height;
  int topheight = heightOffset/2;
  int bottomheight = bounds.size.h - heightOffset;
  // Draw the background GColorClear
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, GCornerNone, 0);
  
  // Draw the bar
  if (!s_battery_plugged)
    {
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_fill_rect(ctx, GRect(0, topheight, bounds.size.w, bottomheight), GCornerNone, 0);
    }
  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  // Create PST & JST Label TextLayer
  tz_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SOLARIA_12));
  pst_layer = text_layer_create(GRect(4, 0, 50, 20));
  text_layer_set_background_color(pst_layer, GColorClear);
  text_layer_set_text_color(pst_layer, GColorWhite);
  text_layer_set_text(pst_layer, "PST");
  text_layer_set_font(pst_layer, tz_font);
  text_layer_set_text_alignment(pst_layer, GTextAlignmentLeft);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(pst_layer));
  
  jst_layer = text_layer_create(GRect(4, 84, 50, 20));
  text_layer_set_background_color(jst_layer, GColorClear);
  text_layer_set_text_color(jst_layer, GColorWhite);
  text_layer_set_text(jst_layer, "JST");
  text_layer_set_font(jst_layer, tz_font);
  text_layer_set_text_alignment(jst_layer, GTextAlignmentLeft);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(jst_layer));
  
  // Create GFonts
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BRACIOLA_34));
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SOLARIA_16));
  
  // Create PST time TextLayer
  pst_time_layer = text_layer_create(GRect(4, 6, 144, 50));
  text_layer_set_background_color(pst_time_layer, GColorClear);
  text_layer_set_text_color(pst_time_layer, GColorWhite);
  text_layer_set_text(pst_time_layer, "00:00");
  text_layer_set_font(pst_time_layer, s_time_font);
  text_layer_set_text_alignment(pst_time_layer, GTextAlignmentLeft);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(pst_time_layer));
  
  // Create PST date TextLayer
  pst_date_layer = text_layer_create(GRect(4, 44, 144, 60));
  text_layer_set_text_color(pst_date_layer, GColorWhite);
  text_layer_set_background_color(pst_date_layer, GColorClear);
  text_layer_set_text_alignment(pst_date_layer, GTextAlignmentLeft);
  text_layer_set_text(pst_date_layer, "Sept 23");
  text_layer_set_font(pst_date_layer, s_date_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(pst_date_layer));
  
  // Create JST time TextLayer
  jst_time_layer = text_layer_create(GRect(4, 90, 144, 50));
  text_layer_set_background_color(jst_time_layer, GColorClear);
  text_layer_set_text_color(jst_time_layer, GColorWhite);
  text_layer_set_text(jst_time_layer, "00:00");
  text_layer_set_font(jst_time_layer, s_time_font);
  text_layer_set_text_alignment(jst_time_layer, GTextAlignmentLeft);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(jst_time_layer));
  
  // Create JST date TextLayer
  jst_date_layer = text_layer_create(GRect(4, 128, 144, 60));
  text_layer_set_text_color(jst_date_layer, GColorWhite);
  text_layer_set_background_color(jst_date_layer, GColorClear);
  text_layer_set_text_alignment(jst_date_layer, GTextAlignmentLeft);
  text_layer_set_text(jst_date_layer, "Sept 23");
  text_layer_set_font(jst_date_layer, s_date_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(jst_date_layer));

  // Create battery meter Layer
  s_battery_layer = layer_create(GRect(138, 0, 6, 168));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(window_get_root_layer(window), s_battery_layer);
  
  // Create the inverter layer for bottom timezone
  inverter_layer = inverter_layer_create(GRect(0, 84, 144, 84));
  layer_add_child(window_get_root_layer(window), (Layer*) inverter_layer);
  
  // Initialize the display
  update_time();
  battery_callback(battery_state_service_peek());


}

static void main_window_unload(Window *window) {
  fonts_unload_custom_font(tz_font);
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_date_font);
  




  
  text_layer_destroy(pst_time_layer);
  text_layer_destroy(pst_date_layer);
  text_layer_destroy(jst_time_layer);
  text_layer_destroy(jst_date_layer);
  text_layer_destroy(pst_layer);
  text_layer_destroy(jst_layer);
  layer_destroy(s_battery_layer);
  inverter_layer_destroy(inverter_layer);
}
  
static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  
  // Register with Event Services
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);
 
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
