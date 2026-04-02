# Emulated Drivers Demo

This sample demonstrates Zephyr's **emulated drivers** running on the
`native_sim` board — no real hardware required.

## What it shows

| Feature | Details |
|---------|---------|
| **GPIO output** | Toggles the emulated LED (`led0` alias, `gpio0` pin 0) |
| **GPIO input** | Reads an emulated button (`gpio0` pin 1) |
| **ADC** | Reads two emulated ADC channels with pre-loaded millivolt values |
| **Console output** | Prints periodic status to the terminal |

The emulated ADC driver (`zephyr,adc-emul`) lets you inject known values
via `adc_emul_const_value_set()` at runtime, which is useful for testing
application logic without hardware.

## Building and running

```bash
west build -b native_sim samples/emulated_drivers_demo -p
west build -t run
```

