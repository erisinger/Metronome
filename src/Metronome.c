#include <pebble.h>

/* functions */
//timer-related
static void timer_handler();
static void set_timer();
static void start_stop_metronome();
static void increase_bpm();
static void decrease_bpm();
static void tick_tock();
static void pendulum_tick();
static void vibrate();

static void set_frame_timer();
static void frame_timer_handler();

//display-related
static void display_bpm();

/* fields */
//timer-related
static AppTimer *timer;
static double bpm = 120;
static double duration_seconds = 0.5;
static bool running = false;
bool left_to_right = true;
 
//display-related
static Window *window;
static TextLayer *text_layer;
static TextLayer *bpm_layer;
static Layer *pendulum_layer;
int PENDULUM_H = 20;
int PENDULUM_W = 20;
char BPM[64];

static AppTimer *frame_timer;
int LEFT = 0;
int FRAMES = 12;

static void timer_handler(){
	if (!running) {
		return;
	}
	tick_tock();
	set_timer();
	display_bpm();
}

static void frame_timer_handler(){
//	int shift_amount = 2;
	int shift_amount = (144 - PENDULUM_W) / FRAMES;
	if (left_to_right) {
		LEFT += shift_amount;
	}
	else {
		LEFT -= shift_amount;
	}
	layer_mark_dirty(pendulum_layer);
//	display_bpm();
	set_frame_timer();
}

static void tick_tock(){
	pendulum_tick();
	vibrate();
}

static void pendulum_tick(){
	if (left_to_right) {
		LEFT = 144 - PENDULUM_W;
	}
	else {
		LEFT = 0;
	}
	left_to_right = !left_to_right;
}

static void vibrate(){
	static const uint32_t const segments[] = { 100 };
	VibePattern buzz = {
	  .durations = segments,
	  .num_segments = ARRAY_LENGTH(segments),
	};

	vibes_enqueue_custom_pattern(buzz);
}

static void set_timer(){
	duration_seconds = 60.0 / bpm;
	
	if(running){
		timer = app_timer_register(duration_seconds * 1000, timer_handler, NULL);
	}
}

static void set_frame_timer(){
	if (running){
		frame_timer = app_timer_register((duration_seconds / FRAMES) * 1000, frame_timer_handler, NULL);
	}
}

static void start_stop_metronome(){
	if (!running) {
		LEFT = 0;
		left_to_right = true;
	}
	
	running = !running;
	set_timer();
	set_frame_timer();
}

static void increase_bpm(){
	if (bpm < 240) {
		bpm++;
	}
}

static void decrease_bpm(){
	if (bpm > 20) {
		bpm--;
	}
}

static void display_bpm(){
//	char BPM[64];
	snprintf(BPM, 64, "%d", (int)bpm);
	text_layer_set_text(text_layer, BPM);
}

void text_layer_update_proc(Layer *my_layer, GContext* ctx){
	display_bpm();
}

void pendulum_layer_update_proc(Layer *my_layer, GContext* ctx){
	
//	if (running) {
		GRect pendulum;
		pendulum.origin = GPoint(LEFT, 0);
		pendulum.size = GSize(PENDULUM_W, PENDULUM_H);
		graphics_fill_rect(ctx, pendulum, 0, GCornerNone);
//	}
	
}


static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
	start_stop_metronome();
	display_bpm();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	increase_bpm();
	display_bpm();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
	decrease_bpm();
	display_bpm();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 50, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 50, down_click_handler);
}

static void window_load(Window *window) {
  	Layer *window_layer = window_get_root_layer(window);
  	GRect bounds = layer_get_bounds(window_layer);

  	text_layer = text_layer_create((GRect) { .origin = { 0, 32 }, .size = { bounds.size.w, 50 } });
  	text_layer_set_text(text_layer, "120");
  	text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
	text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
//	layer_set_update_proc(text_layer_get_layer(text_layer), text_layer_update_proc);
  	layer_add_child(window_layer, text_layer_get_layer(text_layer));

	bpm_layer = text_layer_create((GRect) { .origin = { 0, 82}, .size = { bounds.size.w, 40 } } );
	text_layer_set_text(bpm_layer, "bpm");
	text_layer_set_text_alignment(bpm_layer, GTextAlignmentCenter);
	text_layer_set_font(bpm_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
	layer_add_child(window_layer, text_layer_get_layer(bpm_layer));
	
	pendulum_layer = layer_create((GRect) { .origin = { 0, 122 }, .size = { bounds.size.w, 46 } } );
	layer_set_update_proc(pendulum_layer, pendulum_layer_update_proc);
	layer_add_child(window_layer, pendulum_layer);
}

static void window_unload(Window *window) {
  	text_layer_destroy(text_layer);
	layer_destroy(pendulum_layer);
}

static void init(void) {
  	window = window_create();
  	window_set_click_config_provider(window, click_config_provider);
  	window_set_window_handlers(window, (WindowHandlers) {
    	.load = window_load,
    	.unload = window_unload,
  	});
  	const bool animated = true;
  	window_stack_push(window, animated);
	
	set_timer();
	set_frame_timer();
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
