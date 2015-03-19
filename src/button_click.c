#include <pebble.h>

static Window *window;
static TextLayer *text_layer;

static bool alert = false;
static char result[64];

enum {
  ALERT_KEY = 0x0
};

static void update_time() {
  if (!alert) {
    // Get a tm structure
    time_t temp = time(NULL); 
    struct tm *tick_time = localtime(&temp);
  
    // Create a long-lived buffer
    static char time[] = "00:00";
  
    // Write the current hours and minutes into the buffer
    if(clock_is_24h_style() == true) {
      //Use 2h hour format
      strftime(time, sizeof(time), "%H:%M", tick_time);
    } else {
      //Use 12 hour format
      strftime(time, sizeof(time), "%I:%M", tick_time);
    }
    
    // Display this hour on the TextLayer
    text_layer_set_text(text_layer, time);
  }
}

static void alert_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  alert = true;
  text_layer_set_text(text_layer, "ALERT!");
  
  Tuplet alert_tuple = TupletInteger(ALERT_KEY, alert);
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (iter == NULL) {
    return;
  }
  dict_write_tuplet(iter, &alert_tuple);
  dict_write_end(iter);
  app_message_outbox_send();
    
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, alert_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, alert_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, alert_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, alert_click_handler);
}

static void out_sent_handler(DictionaryIterator *iterator, void *context){
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Outbox Sent.");
  alert = false;
  update_time();
}

char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  snprintf(result, sizeof(result), "%s", translate_error(reason));
  APP_LOG(APP_LOG_LEVEL_DEBUG, result);
  text_layer_set_text(text_layer, "ERROR");
}

static void app_message_init(void) {
  // Register message handlers
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
  // Init buffers
  app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create((GRect) { .origin = { 0, 57 }, .size = { bounds.size.w, 50 } });
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  // Make sure the time is displayed from the start
  update_time();
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void init(void) {
  // Setup AppMessages
  app_message_init();
  // Setup main Window
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
	.load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  // Destroy main Window
  window_destroy(window);
  // Destroy AppMessages
  app_message_deregister_callbacks();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}