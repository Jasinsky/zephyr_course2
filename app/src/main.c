#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

static uint32_t click_count;

static void button_event_cb(lv_event_t *e)
{
	lv_obj_t *label = lv_event_get_user_data(e);

	click_count++;
	lv_label_set_text_fmt(label, "Clicked: %u", click_count);
	LOG_INF("Button clicked %u times", click_count);
}

int main(void)
{
	LOG_INF("Greet: %s", CONFIG_MY_CONFIGURATON_GREETING);
	const struct device *display_dev;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device not ready, aborting");
		return 0;
	}

	/* Create a label */
	lv_obj_t *label = lv_label_create(lv_screen_active());
	lv_label_set_text(label, "Hello LVGL!");
	lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);

	/* Create a button with a label inside */
	lv_obj_t *btn = lv_button_create(lv_screen_active());
	lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_size(btn, 150, 50);

	lv_obj_t *btn_label = lv_label_create(btn);
	lv_label_set_text(btn_label, "Click me!");
	lv_obj_center(btn_label);

	/* Create a counter label updated on button press */
	lv_obj_t *count_label = lv_label_create(lv_screen_active());
	lv_label_set_text(count_label, "Clicked: 0");
	lv_obj_align(count_label, LV_ALIGN_BOTTOM_MID, 0, -20);

	lv_obj_add_event_cb(btn, button_event_cb, LV_EVENT_CLICKED, count_label);

	lv_timer_handler();
	display_blanking_off(display_dev);

	while (1) {
		uint32_t sleep_ms = lv_timer_handler();

		k_msleep(MIN(sleep_ms, INT32_MAX));
	}

	return 0;
}

