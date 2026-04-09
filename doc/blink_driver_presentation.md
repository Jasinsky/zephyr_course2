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

## The Demo Device Driver — A Complete Walkthrough

> For engineers new to Zephyr driver development

---

## Project Structure

```
zephyr_course2/
├── samples/demo_driver/
│   ├── boards/native_sim.overlay ← DT overlay — instantiates the device
│   ├── src/main.c                ← Sample application
│   ├── prj.conf                  ← Kconfig fragment
│   └── CMakeLists.txt
├── dts/bindings/demo/
│   └── demo-device.yaml          ← Node schema / binding
├── drivers/demo/
│   ├── demo_device.c             ← Driver implementation
│   ├── CMakeLists.txt            ← Build integration
│   ├── Kconfig                   ← Driver class config
│   └── Kconfig.demo_device       ← Per-driver config
└── include/app/drivers/
    └── demo.h                    ← Public API header
```

---

## What Does This Driver Do?

The `demo-device` is a **pure software driver** — no real hardware needed.

Every API call is answered with a **Zephyr log message**, making the call chain fully visible at runtime.

**Three API functions:**

| Function | What it does |
|----------|-------------|
| `demo_enable(dev)` | Transitions device → ENABLED; logs the event |
| `demo_disable(dev)` | Transitions device → DISABLED; logs the event |
| `demo_get_status(dev, &status)` | Reads and logs the current state |

> Perfect for teaching how drivers are structured — without needing hardware.

---

## The Glue: `compatible` String

The `compatible` property connects all three layers:

```
native_sim.overlay
    compatible = "demo-device"
          │
          ▼
demo-device.yaml   (schema validation at build time)
          │
          ▼
demo_device.c
    #define DT_DRV_COMPAT demo_device
```

**One string links the DT instance → YAML schema → C driver.**

`demo_device` is the **snake_case** form of `"demo-device"`.

---

## Five-File Architecture

| # | File | Role |
|---|------|------|
| 1 | `native_sim.overlay` | DT instance — label, numeric_id, status |
| 2 | `demo-device.yaml` | Schema / contract — valid properties & types |
| 3 | `demo.h` | Public API — what applications call |
| 4 | `demo_device.c` | Implementation — logging + state machine |
| 5 | `Kconfig.demo_device` | Feature selection — auto-enable from DT |

---

## File 1: The DT Overlay — Device Instance

```dts
/ {
    demo_dev0: demo-device-0 {
        compatible = "demo-device"; /* links to YAML binding */
        label = "DEMO_0";           /* name used in log messages */
        numeric_id = <1>;           /* optional numeric identifier */
        status = "okay";
    };

    demo_dev1: demo-device-1 {
        compatible = "demo-device";
        label = "DEMO_1";
        numeric_id = <2>;
        status = "okay";
    };
};
```

- Two nodes → **two independent driver instances** — no C changes needed
- Overlay is board-specific: `boards/native_sim.overlay`

---

## File 2: The Binding YAML — Schema

```yaml
compatible: "demo-device"
include: base.yaml

properties:
  label:
    type: string
    description: |
      Human-readable name for this demo device instance.
      Used in log messages so you can distinguish multiple instances.

  numeric_id:
    type: int
    description: |
      An optional numeric ID for this demo device instance.
```

- Zephyr's `dtc` validates every DT node against this YAML **at build time**
- No vendor prefix → **virtual/software device**, no real silicon required

---

## File 3: The Driver Header — Public API (`demo.h`)

```c
/** Status of a demo device instance. */
enum demo_status {
    DEMO_STATUS_DISABLED = 0,
    DEMO_STATUS_ENABLED  = 1,
};

/** Operations table (vtable) — filled in by demo_device.c */
__subsystem struct demo_driver_api {
    int (*enable)    (const struct device *dev);
    int (*disable)   (const struct device *dev);
    int (*get_status)(const struct device *dev, enum demo_status *status);
};

/** Public inline wrappers — the only functions applications call */
static inline int demo_enable    (const struct device *dev);
static inline int demo_disable   (const struct device *dev);
static inline int demo_get_status(const struct device *dev,
                                   enum demo_status *status);
```

> **Polymorphism via function pointers** — `demo_driver_api` is the interface; `demo_device.c` is one concrete implementation.

---

## File 4: Driver Implementation — Structs (1/3)

```c
/** @cond */
#define DT_DRV_COMPAT demo_device  /* snake_case of "demo-device" */
/** @endcond */

/** Per-instance runtime state — mutated by enable/disable. */
struct demo_device_data {
    bool enabled; /**< Current enable/disable state. */
};

/** Per-instance compile-time config — read from DTS, never changes. */
struct demo_device_config {
    const char *label;  /**< From DTS label property; may be NULL. */
    int numeric_id;     /**< From DTS numeric_id property; -1 if absent. */
};
```

Config values are baked in **at compile time** via DT macros — zero runtime overhead.

---

## File 4: Driver Implementation — API Functions (2/3)

```c
static int demo_device_enable(const struct device *dev)
{
    struct demo_device_data *data = dev->data;

    LOG_INF("[%s] demo_enable() called", dev_label(dev));

    if (data->enabled) {
        LOG_WRN("[%s] already enabled — returning -EALREADY",
                dev_label(dev));
        return -EALREADY;
    }
    data->enabled = true;
    LOG_INF("[%s] device is now ENABLED", dev_label(dev));
    return 0;
}
```

- Same pattern for `demo_device_disable()` and `demo_device_get_status()`
- Every path through the code **emits a log line**

---

## File 4: Driver Implementation — Instance Macro (3/3)

```c
#define DEMO_DEVICE_DEFINE(inst)                                  \
    static struct demo_device_data demo_data_##inst;              \
    static const struct demo_device_config demo_config_##inst = { \
        .label      = DT_INST_PROP_OR(inst, label, NULL),         \
        .numeric_id = DT_INST_PROP_OR(inst, numeric_id, -1),      \
    };                                                            \
    DEVICE_DT_INST_DEFINE(inst, demo_device_init, NULL,           \
                          &demo_data_##inst, &demo_config_##inst, \
                          POST_KERNEL, CONFIG_DEMO_INIT_PRIORITY, \
                          &demo_device_api);

DT_INST_FOREACH_STATUS_OKAY(DEMO_DEVICE_DEFINE)
```

`DT_INST_FOREACH_STATUS_OKAY` expands the macro **once per enabled DT node** — both `DEMO_0` and `DEMO_1` are instantiated from this single definition.

---

## File 5: Kconfig — Auto-Enable from DT

```kconfig
menuconfig DEMO
    bool "Demo device drivers"

config DEMO_INIT_PRIORITY
    int "Demo device drivers init priority"
    default KERNEL_INIT_PRIORITY_DEVICE

config DEMO_DEVICE
    bool "Virtual demo device driver"
    default y
    depends on DT_HAS_DEMO_DEVICE_ENABLED
```

- `DT_HAS_DEMO_DEVICE_ENABLED` is **auto-generated** when a `demo-device` DT node with `status = "okay"` exists
- Driver compiles only when the devicetree declares it
- `prj.conf` sets `CONFIG_DEMO=y` and `CONFIG_DEMO_LOG_LEVEL_DBG=y`

---

## Data Flow: DT Overlay → Running Driver

```
native_sim.overlay
    └── demo-device-0  (compatible = "demo-device", label = "DEMO_0")
    └── demo-device-1  (compatible = "demo-device", label = "DEMO_1")
              │
              ▼  dtc validates against...
    demo-device.yaml
              │
              ▼  generates devicetree.h macros used by...
    demo_device.c
        DT_INST_PROP_OR(inst, label, NULL)      → "DEMO_0" / "DEMO_1"
        DT_INST_PROP_OR(inst, numeric_id, -1)   → 1 / 2
        DEVICE_DT_INST_DEFINE()  → two struct device objects at boot
              │
              ▼  dispatched through demo_driver_api vtable...
    demo_enable() / demo_disable() / demo_get_status()
```

---

## Sample Application — What You See

```
[00:00:00.000,000] <inf> demo_device: [DEMO_0] demo_device initialised
[00:00:00.000,000] <inf> demo_device: [DEMO_1] demo_device initialised

--- Step 2: enable both ---
[00:00:00.000,000] <inf> demo_device: [DEMO_0] demo_enable() called
[00:00:00.000,000] <inf> demo_device: [DEMO_0] device is now ENABLED

--- Step 3: enable again (expect -EALREADY) ---
[00:00:00.000,000] <inf> demo_device: [DEMO_0] demo_enable() called
[00:00:00.000,000] <wrn> demo_device: [DEMO_0] already enabled — returning -EALREADY

--- Step 4: disable demo0 ---
[00:00:00.000,000] <inf> demo_device: [DEMO_0] demo_disable() called
[00:00:00.000,000] <inf> demo_device: [DEMO_0] device is now DISABLED
```

Every call goes: **app → `demo.h` inline → vtable → `demo_device.c` → log**

---

## Key Macros Reference

| Macro | Purpose |
|-------|---------|
| `DT_DRV_COMPAT` | Binds this driver to a compatible string (snake_case) |
| `DT_INST_PROP_OR(inst, prop, default)` | Read a DT property; fall back to default |
| `DEVICE_DT_INST_DEFINE(inst, ...)` | Create a `struct device` from a DT node |
| `DT_INST_FOREACH_STATUS_OKAY(fn)` | Expand `fn(inst)` for each enabled node |
| `DEVICE_API(class, name)` | Declare a driver class operations struct |
| `DEVICE_DT_GET(node)` | Get a device pointer from a DT node at compile time |

---

## Writing a New Driver — Checklist

1. **Binding YAML** — `dts/bindings/<class>/your-device.yaml`
   Define `compatible`, properties and their types

2. **DT overlay / `.dts`** — add a node with your `compatible` and `status = "okay"`

3. **Driver C file** — `drivers/<class>/your_driver.c`
   `DT_DRV_COMPAT`, data/config structs, init, API ops table, instance macro

4. **Kconfig** — `depends on DT_HAS_<COMPAT>_ENABLED`, set `default y`

5. **CMakeLists.txt** — `zephyr_library_sources_ifdef(CONFIG_... file.c)`

6. **Public header** — `include/app/drivers/your_class.h`
   `__subsystem` struct + `static inline` API wrappers

---

<!-- _class: title -->

## Summary

- `compatible` string is the glue: DT node → YAML schema → C driver
- No hardware needed — a **software-only driver** is a valid Zephyr driver
- `DT_INST_FOREACH_STATUS_OKAY` instantiates one device per DT node automatically
- Kconfig is auto-enabled via `DT_HAS_DEMO_DEVICE_ENABLED`
- Applications call only the **public API** (`demo.h`) — never driver internals
- Zephyr **logger** (`LOG_INF`, `LOG_WRN`) makes the call chain fully observable
