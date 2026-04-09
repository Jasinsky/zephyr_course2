# Building Zephyr Device Drivers — The Blink Driver Example

## Overview

This document walks through the structure and relationships of a complete custom Zephyr device driver, using the **blink GPIO LED** driver from this course as the reference example. It is intended for engineers new to Zephyr driver development.

---

## Where Does Each File Live?

```
zephyr_course2/
├── boards/vendor/custom_plank/
│   └── custom_plank.dts          ← The Device Tree Source file (THE ANSWER)
├── dts/bindings/blink/
│   └── blink-gpio-leds.yaml      ← Describes the DT node schema (binding)
├── drivers/blink/
│   ├── gpio_led.c                ← The actual driver implementation
│   ├── CMakeLists.txt            ← Tells CMake to compile the driver
│   ├── Kconfig                   ← Top-level driver class Kconfig
│   └── Kconfig.gpio_led          ← Kconfig for the specific driver
└── include/app/drivers/
    └── blink.h                   ← Public API header for the driver class
```

---

## Where is the `.dts` File?

> **`zephyr_course2/boards/vendor/custom_plank/custom_plank.dts`**

In Zephyr, the `.dts` (Device Tree Source) file lives under `boards/<vendor>/<board_name>/`. It describes the **hardware topology** of a specific board. There is typically **one `.dts` per board target**.

The `.dts` file is where you **instantiate** your driver as a devicetree node — the YAML binding defines the schema, and the `.dts` uses it.

---

## The Five-File Architecture of a Zephyr Driver

### 1. The `.dts` — Hardware Description (Board-Level)

**File:** `boards/vendor/custom_plank/custom_plank.dts`

```dts
/dts-v1/;
#include <nordic/nrf52840_qiaa.dtsi>
#include "custom_plank-pinctrl.dtsi"

/ {
    model = "Custom Plank Board";
    compatible = "vendor,custom-plank";

    blink_led: blink-led {
        compatible = "blink-gpio-led";          /* matches the binding */
        led-gpios = <&gpio0 13 GPIO_ACTIVE_LOW>; /* LED on GPIO0 pin 13 */
        blink-period-ms = <1000>;               /* blink every 1 second */
    };
};
```

**What it does:** Declares that this board has a `blink-gpio-led` device, specifying which GPIO pin the LED is on and the default blink period. This is the only file a **board bring-up engineer** typically modifies to add the blink LED to a new board.

**Key concept:** The `compatible = "blink-gpio-led"` string is the glue that links the DT node → binding YAML → driver C code.

---

### 2. The Binding YAML — Schema / Contract

**File:** `dts/bindings/blink/blink-gpio-leds.yaml`

```yaml
description: |
  A generic binding for a GPIO-controlled blinking LED.

compatible: "blink-gpio-led"

include: base.yaml

properties:
  led-gpios:
    type: phandle-array
    required: true
    description: GPIO-controlled LED.

  blink-period-ms:
    type: int
    description: Initial blinking period in milliseconds.
```

**What it does:**
- Defines the **schema** for any DT node with `compatible = "blink-gpio-led"`.
- Lists all valid properties (`led-gpios`, `blink-period-ms`), their types, and whether they are required.
- During the build, Zephyr's `dtc` (devicetree compiler) validates the `.dts` against this YAML.

**Key concept:** No vendor prefix in `compatible` means this is a **virtual/software device** with no real silicon — just a software abstraction over a GPIO.

---

### 3. The Driver Header — Public API

**File:** `include/app/drivers/blink.h`

```c
__subsystem struct blink_driver_api {
    int (*set_period_ms)(const struct device *dev, unsigned int period_ms);
};

__syscall int blink_set_period_ms(const struct device *dev,
                                  unsigned int period_ms);
```

**What it does:**
- Defines `struct blink_driver_api` — the **operations table** (vtable) for the blink driver class.
- `__subsystem` tags the struct so Zephyr's syscall generator can create userspace wrappers.
- `__syscall` marks `blink_set_period_ms()` so it can cross the kernel/userspace boundary if needed.
- Applications `#include <app/drivers/blink.h>` and call `blink_set_period_ms()` — they never call `gpio_led.c` functions directly.

**Key concept:** Zephyr driver classes use **polymorphism via function pointers**. The `blink_driver_api` struct is the interface; `gpio_led.c` is one concrete implementation of that interface.

---

### 4. The Driver Implementation — `gpio_led.c`

**File:** `drivers/blink/gpio_led.c`

```c
#define DT_DRV_COMPAT blink_gpio_led   /* snake_case version of "blink-gpio-led" */
```

The `DT_DRV_COMPAT` macro tells Zephyr's devicetree macros which compatible string this driver handles.

#### Configuration struct — reads DT properties at compile time

```c
struct blink_gpio_led_config {
    struct gpio_dt_spec led;       /* populated from led-gpios */
    unsigned int period_ms;        /* populated from blink-period-ms */
};
```

#### Timer callback — called when the blink timer fires

```c
static void blink_gpio_led_on_timer_expire(struct k_timer *timer)
{
    const struct device *dev = k_timer_user_data_get(timer);
    const struct blink_gpio_led_config *config = dev->config;
    gpio_pin_toggle_dt(&config->led);  /* toggle the LED */
}
```

#### API operations table — implements the blink interface

```c
static DEVICE_API(blink, blink_gpio_led_api) = {
    .set_period_ms = &blink_gpio_led_set_period_ms,
};
```

#### Instance macro — one device instance per matching DT node

```c
#define BLINK_GPIO_LED_DEFINE(inst)                                         \
    static struct blink_gpio_led_data data##inst;                           \
    static const struct blink_gpio_led_config config##inst = {              \
        .led = GPIO_DT_SPEC_INST_GET(inst, led_gpios),  /* from DTS */      \
        .period_ms = DT_INST_PROP_OR(inst, blink_period_ms, 0U), /* DTS */  \
    };                                                                      \
    DEVICE_DT_INST_DEFINE(inst, blink_gpio_led_init, NULL,                  \
                          &data##inst, &config##inst,                       \
                          POST_KERNEL, CONFIG_BLINK_INIT_PRIORITY,          \
                          &blink_gpio_led_api);

DT_INST_FOREACH_STATUS_OKAY(BLINK_GPIO_LED_DEFINE)
```

**Key concept:** `DT_INST_FOREACH_STATUS_OKAY` iterates over **every enabled** `blink-gpio-led` node in the devicetree and instantiates a separate `struct device` for each one. If you add a second LED to the `.dts`, you get a second driver instance automatically — no C code changes needed.

---

### 5. Kconfig — Feature Selection

**File:** `drivers/blink/Kconfig.gpio_led`

```kconfig
config BLINK_GPIO_LED
    bool "GPIO-controlled LED blink driver"
    default y
    depends on DT_HAS_BLINK_GPIO_LED_ENABLED   /* auto-set when DT node exists */
    select GPIO
```

**What it does:**
- Automatically enables the driver (`default y`) when a `blink-gpio-led` node is present and enabled in the devicetree (`DT_HAS_BLINK_GPIO_LED_ENABLED`).
- Selects the `GPIO` subsystem as a dependency.
- The `CMakeLists.txt` only compiles `gpio_led.c` when `CONFIG_BLINK_GPIO_LED=y`.

---

## Data Flow: DTS → Driver

```
custom_plank.dts
    └── blink-led node (compatible = "blink-gpio-led")
            │
            ▼ dtc compiler validates against...
    blink-gpio-leds.yaml
            │
            ▼ generates devicetree.h macros consumed by...
    gpio_led.c
        DT_DRV_COMPAT = blink_gpio_led
        GPIO_DT_SPEC_INST_GET() → reads led-gpios from DTS
        DT_INST_PROP_OR()       → reads blink-period-ms from DTS
        DEVICE_DT_INST_DEFINE() → creates struct device at boot
            │
            ▼ registers with...
    blink.h (driver API)
        blink_set_period_ms() → calls config→api→set_period_ms()
```

---

## Key Zephyr Driver Macros Explained

| Macro | Purpose |
|-------|---------|
| `DT_DRV_COMPAT` | Defines which DT `compatible` string this driver handles (use snake_case) |
| `GPIO_DT_SPEC_INST_GET(inst, prop)` | Reads a `phandle-array` GPIO property from DTS into a `gpio_dt_spec` |
| `DT_INST_PROP_OR(inst, prop, default)` | Reads an integer property, falling back to `default` if absent |
| `DEVICE_DT_INST_DEFINE(inst, ...)` | Creates a `struct device` linked to the DT node |
| `DT_INST_FOREACH_STATUS_OKAY(fn)` | Calls `fn(inst)` for each enabled matching DT node |
| `DEVICE_API(class, name)` | Declares an operations struct for a driver class |

---

## Summary: What to Do When Writing a New Driver

1. **Write the binding YAML** in `dts/bindings/<class>/` — define your compatible string and properties.
2. **Add the DT node** in the board's `.dts` file under `boards/<vendor>/<board>/`.
3. **Write the driver** in `drivers/<class>/` — implement `DT_DRV_COMPAT`, config struct, init function, and API ops table.
4. **Write the Kconfig** — depend on `DT_HAS_<COMPAT>_ENABLED` to auto-enable.
5. **Update CMakeLists.txt** — conditionally compile under `CONFIG_<DRIVER>`.
6. **Expose the public API** in `include/` if building a reusable driver class.
