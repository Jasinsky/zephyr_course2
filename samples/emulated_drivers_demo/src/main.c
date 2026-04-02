/*
 * Emulated Drivers Demo for native_sim
 *
 * Demonstrates using Zephyr's emulated ADC and GPIO drivers on native_sim.
 * This is course material showing how emulated peripherals work without
 * real hardware.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/adc_emul.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(emulated_demo, LOG_LEVEL_INF);

/* LED on gpio0 pin 0 (alias led0 in native_sim.dts) */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* An extra emulated GPIO input pin (gpio0, pin 1) configured via overlay */
#define INPUT_NODE DT_PATH(zephyr_user)
static const struct gpio_dt_spec button =
	GPIO_DT_SPEC_GET(INPUT_NODE, input_gpios);

/* ---------- ADC configuration ---------- */
#define ADC_NODE       DT_NODELABEL(adc0)
#define ADC_RESOLUTION 10
#define ADC_VREF_MV    3300
#define ADC_NUM_CHANNELS 2

static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);

static const struct adc_channel_cfg channel_cfgs[ADC_NUM_CHANNELS] = {
	{
		.gain = ADC_GAIN_1,
		.reference = ADC_REF_VDD_1,
		.acquisition_time = ADC_ACQ_TIME_DEFAULT,
		.channel_id = 0,
	},
	{
		.gain = ADC_GAIN_1,
		.reference = ADC_REF_VDD_1,
		.acquisition_time = ADC_ACQ_TIME_DEFAULT,
		.channel_id = 1,
	},
};

int main(void)
{
	int ret;
	uint16_t adc_buf;
	struct adc_sequence sequence = {
		.buffer = &adc_buf,
		.buffer_size = sizeof(adc_buf),
		.resolution = ADC_RESOLUTION,
	};

	printk("\n=== Emulated Drivers Demo (native_sim) ===\n\n");

	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("LED GPIO device not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure LED pin: %d", ret);
		return ret;
	}

	if (!gpio_is_ready_dt(&button)) {
		LOG_ERR("Button GPIO device not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure button pin: %d", ret);
		return ret;
	}

	LOG_INF("GPIO configured: LED on %s pin %d, Button on %s pin %d",
		led.port->name, led.pin,
		button.port->name, button.pin);

	if (!device_is_ready(adc_dev)) {
		LOG_ERR("ADC device not ready");
		return -ENODEV;
	}

	for (int i = 0; i < ADC_NUM_CHANNELS; i++) {
		ret = adc_channel_setup(adc_dev, &channel_cfgs[i]);
		if (ret < 0) {
			LOG_ERR("ADC channel %d setup failed: %d", i, ret);
			return ret;
		}
	}

	/*
	 * Pre-load the emulated ADC with known millivolt values so we
	 * get deterministic readings in the demo.
	 *   Channel 0 -> 1500 mV  (simulated temperature sensor)
	 *   Channel 1 -> 3100 mV  (simulated battery voltage)
	 */
	ret = adc_emul_const_value_set(adc_dev, 0, 1500);
	if (ret < 0) {
		LOG_WRN("Could not set emulated ADC ch0 value: %d", ret);
	}
	ret = adc_emul_const_value_set(adc_dev, 1, 3100);
	if (ret < 0) {
		LOG_WRN("Could not set emulated ADC ch1 value: %d", ret);
	}

	LOG_INF("ADC configured: %d channel(s) on %s (Vref=%d mV, %d-bit)",
		ADC_NUM_CHANNELS, adc_dev->name, ADC_VREF_MV, ADC_RESOLUTION);

	printk("\n--- Starting periodic readout ---\n\n");

	/* ---- Main loop ---- */
	for (int iter = 0; iter < 10; iter++) {
		/* Toggle the LED */
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			LOG_ERR("LED toggle failed: %d", ret);
		}
		int led_state = gpio_pin_get_dt(&led);

		/* Read the emulated button input */
		int btn_state = gpio_pin_get_dt(&button);

		printk("[%d] LED=%d  Button=%d", iter, led_state, btn_state);

		/* Read each ADC channel */
		for (int i = 0; i < ADC_NUM_CHANNELS; i++) {
			int32_t val_mv;

			sequence.channels = BIT(channel_cfgs[i].channel_id);
			ret = adc_read(adc_dev, &sequence);
			if (ret < 0) {
				printk("  ADC ch%d: read error %d", i, ret);
				continue;
			}

			val_mv = (int32_t)adc_buf;
			adc_raw_to_millivolts(ADC_VREF_MV, ADC_GAIN_1,
					      ADC_RESOLUTION, &val_mv);
			printk("  ADC ch%d: %d mV (raw=%d)", i, val_mv, adc_buf);
		}
		printk("\n");

		k_msleep(1000);
	}

	printk("\n=== Demo complete ===\n");
	return 0;
}
