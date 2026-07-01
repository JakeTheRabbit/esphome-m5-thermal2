# M5Stack Unit Thermal2 component (`m5_thermal2`)

ESPHome external component for the **M5Stack Unit Thermal2** (MLX90640, 110° FoV,
SKU U149).

## Why this is a separate component from `mlx90640`

The sibling [`mlx90640`](../mlx90640) component talks to a **bare** Melexis
MLX90640 over I2C — it dumps the EEPROM, reads frame RAM, and runs the Melexis
math on the ESP32. That is the right driver for breakout boards and the *original*
M5Stack Thermal Unit.

The **Unit Thermal2 is different.** It puts an **ESP32‑PICO‑D4 co‑processor in
front of the sensor.** The MLX90640 is wired to that MCU, not to the Grove bus.
The host instead talks to the MCU at I2C address **`0x32`** and reads a small
*processed* register map (min / max / average / median, hotspot coordinates,
button, plus buzzer / RGB‑LED / alarm control). So the raw‑MLX90640 driver cannot
drive this unit at all — this component speaks the MCU's protocol instead.

Protocol source: [`m5stack/M5Unit-Thermal2`](https://github.com/m5stack/M5Unit-Thermal2)
(`src/M5_Thermal2.h` / `.cpp`).

## What it gives you

- `sensor`: `average_temperature`, `median_temperature`, `max_temperature`,
  `min_temperature`, `diff_temperature` (largest in‑frame difference),
  `hotspot_x`, `hotspot_y` (column/row of the hottest pixel).
- `binary_sensor`: `button` (the unit's function button).
- Runtime control methods (call from lambdas / automations):
  `set_led(r,g,b)`, `led_on()`, `led_off()`, `set_buzzer(freq,vol)`,
  `buzzer_on()`, `buzzer_off()`, `set_refresh_rate_live(code)`,
  `set_noise_filter_live(level)`, `set_alarm_high(...)`, `set_alarm_low(...)`,
  `alarm_on(mask=0xFF)`, `alarm_off(mask=0xFF)`.

> Not (yet) implemented: the full 32×24 per‑pixel thermal image / web JPEG viewer.
> The unit exposes the 768‑pixel block right after the overview registers (read
> 0x70 → 16 summary bytes, then 768 pixel bytes), so it can be added; the ATOM /
> ESP32‑PoE targets are headless so the processed values above are usually what
> you actually want in Home Assistant.

## Wiring & pins

The unit needs **5 V** (its MCU is 5 V powered). Grove wire colours:
`Red=5V  Black=GND  Yellow=SDA  White=SCL`.

| Board | SDA | SCL | `esp32: board:` |
|---|---|---|---|
| ATOM Lite / Matrix | GPIO26 | GPIO32 | `m5stack-atom` |
| ATOMS3 / ATOMS3 Lite | GPIO2 | GPIO1 | `m5stack-atoms3` |
| Olimex ESP32‑PoE (UEXT) | GPIO13 | GPIO16 | `esp32-poe` |

Set the I2C bus `frequency: 400000`. Only ~16 bytes are read per update, so
400 kHz is more than enough and reliable even over a Grove lead. (High clocks like
800 kHz only matter for streaming the raw pixel block, which this component
doesn't do.)

## Address note

The M5Stack store page lists `0x33`, but the official library hard‑codes **`0x32`**
(`i2c_default_addr`), and the unit's address is user‑changeable and stored in
flash. Leave `scan: true` on the `i2c:` bus — the boot log prints the detected
address. If it isn't `0x32`, set `address:` to whatever the scan reports. A wrong
address is also caught at setup: the component verifies the device‑ID bytes
(`0x90`, `0x64`) and logs a clear error.

## Quick start

See [`example.yaml`](example.yaml) for the full reference, or the ready‑to‑flash
configs in the repo root: `thermal2-atomlite.yaml`, `thermal2-esp32poe.yaml`.

```yaml
external_components:
  - source: { type: local, path: . }
    components: [m5_thermal2]

i2c:
  sda: GPIO26
  scl: GPIO32
  frequency: 400000
  scan: true

m5_thermal2:
  id: thermal
  address: 0x32
  refresh_rate: "16Hz"
  update_interval: 5s

sensor:
  - platform: m5_thermal2
    m5_thermal2_id: thermal
    max_temperature: { name: "Thermal Max" }
    min_temperature: { name: "Thermal Min" }
```
