---
marp: true
theme: default
paginate: true
style: |
  section {
    font-size: 1.4rem;
  }
  section.title {
    text-align: center;
    justify-content: center;
  }
  pre {
    font-size: 0.75rem;
  }
  code {
    font-size: 0.8rem;
  }
  table {
    font-size: 0.85rem;
  }
---

<!-- _class: title -->

# Building Zephyr Device Drivers

## The Blink GPIO LED Driver — A Complete Walkthrough

> For engineers new to Zephyr driver development

---

## Project Structure

```
zephyr_course2/
├── boards/vendor/custom_plank/
│   └── custom_plank.dts          ← Device Tree Source (hardware instance)
├── dts/bindings/blink/
│   └── blink-gpio-leds.yaml      ← Node schema / binding
├── drivers/blink/
│   ├── gpio_led.c                ← Driver implementation
│   ├── CMakeLists.txt            ← Build integration
│   ├── Kconfig                   ← Driver class config
│   └── Kconfig.gpio_led          ← Per-driver config
└── include/app/drivers/
    └── blink.h                   ← Public API header
```

---

## Where Is the `.dts` File?

### `boards/vendor/custom_plank/custom_plank.dts`

In Zephyr, `.dts` files live under `boards/<vendor>/<board_name>/`

- One `.dts` per board target
- Describes the **hardware topology** of a specific board
- Where you **instantiate** your driver as a devicetree node

> The YAML binding defines the **schema**, the `.dts` provides the **instance**

---

## The Glue: `compatible` String

The `compatible` property is the link between all layers:

```
custom_plank.dts
    compatible = "blink-gpio-led"
          │
          ▼
blink-gpio-leds.yaml   (schema validation)
          │
          ▼
gpio_led.c
    #define DT_DRV_COMPAT blink_gpio_led
```

**One string connects the hardware description → schema → C driver.**

---

## Five-File Architecture

| # | File | Role |
|---|------|------|
| 1 | `custom_plank.dts` | Hardware instance — which GPIO, what period |
| 2 | `blink-gpio-leds.yaml` | Schema / contract — valid properties & types |
| 3 | `blink.h` | Public API — what applications call |
| 4 | `gpio_led.c` | Implementation — the actual driver code |
| 5 | `Kconfig.gpio_led` | Feature selection — auto-enable from DT |

---

## File 1: The `.dts` — Hardware Instance

```dts
/ {
    blink_led: blink-led {
        compatible = "blink-gpio-led";           /* links to YAML binding */
        led-gpios = <&gpio0 13 GPIO_ACTIVE_LOW>; /* LED on GPIO0 pin 13 */
        blink-period-ms = <1000>;                /* blink every 1 second */
    };
};
```

- The **board bring-up engineer** writes this
- Add a second `blink-led` node → get a second driver instance automatically
- No C code changes required

---

## File 2: The Binding YAML — Schema

```yaml
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

- Zephyr's `dtc` validates every `.dts` node against this YAML at build time
- No vendor prefix → **virtual/software device**, no real silicon

---

## File 3: The Driver Header — Public API

```c
/* Operations table (vtable) for the blink driver class */
__subsystem struct blink_driver_api {
    int (*set_period_ms)(const struct device *dev, unsigned int period_ms);
};

/* Public function applications call */
__syscall int blink_set_period_ms(const struct device *dev,
                                  unsigned int period_ms);
```

- `__subsystem` enables Zephyr's syscall generator to create userspace wrappers
- `__syscall` allows crossing the kernel/userspace boundary
- Applications call `blink_set_period_ms()` — never `gpio_led.c` directly

> **Polymorphism via function pointers** — `blink_driver_api` is the interface

---

## File 4: Driver Implementation — Key Parts (1/3)

### DT compat and config struct

```c
#define DT_DRV_COMPAT blink_gpio_led  /* snake_case of "blink-gpio-led" */

struct blink_gpio_led_config {
    struct gpio_dt_spec led;    /* populated from led-gpios in DTS */
    unsigned int period_ms;     /* populated from blink-period-ms in DTS */
};
```

Values are read from the devicetree **at compile time** — zero runtime overhead.

---

## File 4: Driver Implementation — Key Parts (2/3)

### Timer callback and API table

```c
static void blink_gpio_led_on_timer_expire(struct k_timer *timer)
{
    const struct device *dev = k_timer_user_data_get(timer);
    const struct blink_gpio_led_config *config = dev->config;
    gpio_pin_toggle_dt(&config->led);  /* toggle the LED */
}

static DEVICE_API(blink, blink_gpio_led_api) = {
    .set_period_ms = &blink_gpio_led_set_period_ms,
};
```

`DEVICE_API` registers this struct as the implementation of the blink interface.

---

## File 4: Driver Implementation — Key Parts (3/3)

### Instance macro — the magic

```c
#define BLINK_GPIO_LED_DEFINE(inst)                              \
    static struct blink_gpio_led_data data##inst;                \
    static const struct blink_gpio_led_config config##inst = {   \
        .led = GPIO_DT_SPEC_INST_GET(inst, led_gpios),           \
        .period_ms = DT_INST_PROP_OR(inst, blink_period_ms, 0U), \
    };                                                           \
    DEVICE_DT_INST_DEFINE(inst, blink_gpio_led_init, NULL,       \
                          &data##inst, &config##inst,            \
                          POST_KERNEL, CONFIG_BLINK_INIT_PRIORITY,\
                          &blink_gpio_led_api);

DT_INST_FOREACH_STATUS_OKAY(BLINK_GPIO_LED_DEFINE)
```

`DT_INST_FOREACH_STATUS_OKAY` expands the macro **once per enabled DT node**.

---

## File 5: Kconfig — Auto-Enable from DT

```kconfig
config BLINK_GPIO_LED
    bool "GPIO-controlled LED blink driver"
    default y
    depends on DT_HAS_BLINK_GPIO_LED_ENABLED  /* set automatically by dtc */
    select GPIO
```

- `DT_HAS_BLINK_GPIO_LED_ENABLED` is auto-generated when a matching DT node exists
- Driver compiles **only when** the devicetree declares it
- `CMakeLists.txt`: `zephyr_library_sources_ifdef(CONFIG_BLINK_GPIO_LED gpio_led.c)`

---

## Data Flow: DTS → Running Driver

```
custom_plank.dts
    └── blink-led node  (compatible = "blink-gpio-led")
              │
              ▼  dtc validates against...
    blink-gpio-leds.yaml
              │
              ▼  generates devicetree.h macros used by...
    gpio_led.c
        GPIO_DT_SPEC_INST_GET()  → reads led-gpios
        DT_INST_PROP_OR()        → reads blink-period-ms
        DEVICE_DT_INST_DEFINE()  → creates struct device at boot
              │
              ▼  device registered, API wired to...
    blink_set_period_ms()  →  blink_gpio_led_set_period_ms()
```

---

## Key Macros Reference

| Macro | Purpose |
|-------|---------|
| `DT_DRV_COMPAT` | Compatible string for this driver (snake_case) |
| `GPIO_DT_SPEC_INST_GET(inst, prop)` | Read GPIO phandle-array from DTS |
| `DT_INST_PROP_OR(inst, prop, default)` | Read integer property from DTS |
| `DEVICE_DT_INST_DEFINE(inst, ...)` | Create `struct device` from DT node |
| `DT_INST_FOREACH_STATUS_OKAY(fn)` | Iterate all enabled matching nodes |
| `DEVICE_API(class, name)` | Declare driver class operations struct |

---

## Writing a New Driver — Checklist

1. **Binding YAML** — `dts/bindings/<class>/your-device.yaml`
   Define `compatible`, properties and their types

2. **DT node** — `boards/<vendor>/<board>/board.dts`
   Instantiate the device with real hardware parameters

3. **Driver C file** — `drivers/<class>/your_driver.c`
   `DT_DRV_COMPAT`, config struct, init, API ops, instance macro

4. **Kconfig** — depend on `DT_HAS_<COMPAT>_ENABLED`, select dependencies

5. **CMakeLists.txt** — `zephyr_library_sources_ifdef(CONFIG_... file.c)`

6. **Public header** — `include/app/drivers/your_class.h` (if reusable class)

---

<!-- _class: title -->

## Summary

- The `.dts` file is in `boards/<vendor>/<board>/`
- `compatible` string is the glue between DTS → YAML → C driver
- `DT_INST_FOREACH_STATUS_OKAY` auto-instantiates one device per DT node
- Kconfig auto-enables the driver when the DT node is present
- Applications use only the public API header — never driver internals
