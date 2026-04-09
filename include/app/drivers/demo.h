/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_DRIVERS_DEMO_H_
#define APP_DRIVERS_DEMO_H_

#include <zephyr/device.h>
#include <zephyr/toolchain.h>

/**
 * @defgroup drivers_demo Demo driver class
 * @ingroup drivers
 * @{
 *
 * @brief Custom demo driver class
 *
 * A minimal custom driver class that demonstrates:
 *   - Defining a driver class API with function pointers
 *   - Implementing enable / disable / get_status operations
 *   - Wiring a DT node to a C driver via the compatible string
 *   - Using the Zephyr logger inside a driver
 *
 * This driver has no real hardware — every API call is answered with a
 * log message so course participants can observe the call chain.
 */

/**
 * @brief Status of a demo device instance.
 */
enum demo_status {
	/** Device is disabled (default after init). */
	DEMO_STATUS_DISABLED = 0,
	/** Device is enabled. */
	DEMO_STATUS_ENABLED  = 1,
};

/**
 * @brief Operations table for the demo driver class.
 *
 * Each concrete driver (e.g. demo_device.c) fills in these pointers.
 * Applications never use this struct directly — they call the inline
 * helpers below, which dispatch through the ops table.
 */
__subsystem struct demo_driver_api {
	/**
	 * @brief Enable the device.
	 *
	 * @param dev  Demo device instance.
	 * @retval 0        on success.
	 * @retval -EALREADY if already enabled.
	 * @retval -errno   on failure.
	 */
	int (*enable)(const struct device *dev);

	/**
	 * @brief Disable the device.
	 *
	 * @param dev  Demo device instance.
	 * @retval 0        on success.
	 * @retval -EALREADY if already disabled.
	 * @retval -errno   on failure.
	 */
	int (*disable)(const struct device *dev);

	/**
	 * @brief Read the current status of the device.
	 *
	 * @param dev     Demo device instance.
	 * @param status  Output: current @ref demo_status value.
	 * @retval 0      on success.
	 * @retval -errno on failure.
	 */
	int (*get_status)(const struct device *dev, enum demo_status *status);
};

/**
 * @brief Enable the demo device.
 *
 * @param dev Demo device instance obtained via DEVICE_DT_GET().
 * @retval 0        Success.
 * @retval -EALREADY Device was already enabled.
 * @retval -errno   Driver-specific error.
 */
static inline int demo_enable(const struct device *dev)
{
	const struct demo_driver_api *api =
		(const struct demo_driver_api *)dev->api;

	return api->enable(dev);
}

/**
 * @brief Disable the demo device.
 *
 * @param dev Demo device instance obtained via DEVICE_DT_GET().
 * @retval 0        Success.
 * @retval -EALREADY Device was already disabled.
 * @retval -errno   Driver-specific error.
 */
static inline int demo_disable(const struct device *dev)
{
	const struct demo_driver_api *api =
		(const struct demo_driver_api *)dev->api;

	return api->disable(dev);
}

/**
 * @brief Get the current status of the demo device.
 *
 * @param dev    Demo device instance obtained via DEVICE_DT_GET().
 * @param status Pointer to store the returned @ref demo_status value.
 * @retval 0     Success.
 * @retval -errno Driver-specific error.
 */
static inline int demo_get_status(const struct device *dev,
				  enum demo_status *status)
{
	const struct demo_driver_api *api =
		(const struct demo_driver_api *)dev->api;

	return api->get_status(dev, status);
}

/** @} */

#endif /* APP_DRIVERS_DEMO_H_ */
