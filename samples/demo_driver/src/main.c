/**
 * @file main.c
 * @brief Demo Driver Sample
 *
 * Demonstrates the full lifecycle of the custom "demo" driver class.
 * Two device instances are declared in the devicetree overlay and
 * exercised here so participants can see log output for every API call.
 *
 * @par Build for native_sim:
 * @code
 *   west build -b native_sim samples/demo_driver
 *   west build -b native_sim samples/demo_driver -- -DBOARD_ROOT=<course_root>
 * @endcode
 *
 * @par Run:
 * @code
 *   ./build/zephyr/zephyr.exe
 * @endcode
 *
 * @copyright Copyright (c) 2024 Nordic Semiconductor ASA
 * @license   Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#include <app/drivers/demo.h>

LOG_MODULE_REGISTER(demo_sample, LOG_LEVEL_INF);

/** @brief First demo device instance, obtained from the devicetree at compile time. */
static const struct device *demo0 = DEVICE_DT_GET(DT_NODELABEL(demo_dev0));

/** @brief Second demo device instance, obtained from the devicetree at compile time. */
static const struct device *demo1 = DEVICE_DT_GET(DT_NODELABEL(demo_dev1));

/**
 * @brief Query and log the current status of a demo device.
 *
 * Calls @ref demo_get_status() and prints the result via the Zephyr logger.
 *
 * @param dev  Demo device instance to query.
 */
static void print_status(const struct device *dev)
{
	enum demo_status status;
	int ret;

	ret = demo_get_status(dev, &status);
	if (ret < 0) {
		LOG_ERR("get_status failed for %s: %d", dev->name, ret);
		return;
	}

	LOG_INF("  --> %s status = %s",
		dev->name,
		status == DEMO_STATUS_ENABLED ? "ENABLED" : "DISABLED");
}

/**
 * @brief Application entry point.
 *
 * Exercises the demo driver API in six steps:
 *  -# Read initial status (both devices should be DISABLED after init).
 *  -# Enable both devices.
 *  -# Attempt to enable again — expects @c -EALREADY.
 *  -# Disable only demo0.
 *  -# Attempt to disable demo0 again — expects @c -EALREADY.
 *  -# Re-enable demo0.
 *
 * @return 0 on success, negative errno on device-readiness failure.
 */
int main(void)
{
	int ret;

	printk("\n=== Demo Driver Sample ===\n\n");

	/** @note Verify both device instances are ready before use. */
	if (!device_is_ready(demo0)) {
		LOG_ERR("demo0 is not ready!");
		return -ENODEV;
	}
	if (!device_is_ready(demo1)) {
		LOG_ERR("demo1 is not ready!");
		return -ENODEV;
	}

	LOG_INF("Both demo devices are ready.\n");

	/** @par Step 1: Read initial status — both devices should be DISABLED after init. */
	LOG_INF("--- Step 1: initial status ---");
	print_status(demo0);
	print_status(demo1);

	/** @par Step 2: Enable both devices and confirm status flips to ENABLED. */
	LOG_INF("\n--- Step 2: enable both ---");
	ret = demo_enable(demo0);
	LOG_INF("demo_enable(demo0) returned %d", ret);

	ret = demo_enable(demo1);
	LOG_INF("demo_enable(demo1) returned %d", ret);

	print_status(demo0);
	print_status(demo1);

	/** @par Step 3: Enable an already-enabled device — driver must return @c -EALREADY. */
	LOG_INF("\n--- Step 3: enable again (expect -EALREADY) ---");
	ret = demo_enable(demo0);
	LOG_INF("demo_enable(demo0) returned %d (expected %d)",
		ret, -EALREADY);

	/** @par Step 4: Disable only demo0; demo1 must remain ENABLED. */
	LOG_INF("\n--- Step 4: disable demo0 ---");
	ret = demo_disable(demo0);
	LOG_INF("demo_disable(demo0) returned %d", ret);

	print_status(demo0);
	print_status(demo1);

	/** @par Step 5: Disable an already-disabled device — driver must return @c -EALREADY. */
	LOG_INF("\n--- Step 5: disable demo0 again (expect -EALREADY) ---");
	ret = demo_disable(demo0);
	LOG_INF("demo_disable(demo0) returned %d (expected %d)",
		ret, -EALREADY);

	/** @par Step 6: Re-enable demo0 — both devices should now be ENABLED. */
	LOG_INF("\n--- Step 6: re-enable demo0 ---");
	ret = demo_enable(demo0);
	LOG_INF("demo_enable(demo0) returned %d", ret);

	print_status(demo0);
	print_status(demo1);

	LOG_INF("\n=== Sample complete ===");
	return 0;
}
