# ESPHome component for the M5Stack Unit Thermal2

An [ESPHome](https://esphome.io) external component for the **M5Stack Unit
Thermal2** (MLX90640, 110° FoV thermal camera, SKU U149). Publishes the unit's
processed temperatures (average / median / min / max / hotspot) to Home
Assistant, plus button, RGB-LED, buzzer and on-device temperature alarms.

> **Why a custom component?** The Unit Thermal2 is **not** a bare MLX90640. It has
> an onboard ESP32-PICO-D4 that reads the sensor and exposes a processed register
> map over I2C at address **`0x32`** — so the generic MLX90640 drivers (which talk
> raw sensor protocol at `0x33`) can't drive it. This component speaks the unit's
> own protocol. Details in [`components/m5_thermal2/README.md`](components/m5_thermal2/README.md).

## Install

You don't copy anything — ESPHome pulls the component from this repo. Add to your
ESPHome device YAML:

```yaml
external_components:
  - source: github://JakeTheRabbit/esphome-m5-thermal2@main
    components: [m5_thermal2]

i2c:
  sda: GPIO26          # your board's SDA (see table below)
  scl: GPIO32          # your board's SCL
  frequency: 400000
  scan: true           # boot log confirms the unit at 0x32

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
    average_temperature: { name: "Thermal Average" }

binary_sensor:
  - platform: m5_thermal2
    m5_thermal2_id: thermal
    button: { name: "Thermal Button" }
```

Then flash from the ESPHome dashboard, or:

```bash
esphome run your-device.yaml
```

### Ready-to-flash configs

Copy one of these into your ESPHome dashboard and edit the Wi-Fi / device name:

- [`thermal2-atomlite.yaml`](thermal2-atomlite.yaml) — M5Stack ATOM Lite / Matrix / AtomS3 (Wi-Fi)
- [`thermal2-esp32poe.yaml`](thermal2-esp32poe.yaml) — Olimex ESP32-PoE (wired Ethernet)
- [`example.yaml`](example.yaml) — full reference showing every option

## Wiring

The unit needs **5 V** (its MCU is 5 V powered). Grove colours:
`Red=5V  Black=GND  Yellow=SDA  White=SCL`.

| Board | SDA | SCL | `esp32: board:` |
|---|---|---|---|
| ATOM Lite / Matrix (Grove) | GPIO26 | GPIO32 | `m5stack-atom` |
| AtomS3 / AtomS3 Lite (Grove) | GPIO2 | GPIO1 | `m5stack-atoms3` |
| Olimex ESP32-PoE (UEXT header) | GPIO13 | GPIO16 | `esp32-poe` |

The ESP32-PoE has no Grove socket — wire the Grove tail to the header pins above.
Ethernet (LAN8720) already uses GPIO 12/17/18/19/21/22/23/25/26/27, so don't reuse those.

## Entities & controls

- **Sensors:** `average_temperature`, `median_temperature`, `max_temperature`,
  `min_temperature`, `diff_temperature`, `hotspot_x`, `hotspot_y`
- **Binary sensor:** `button`
- **Lambda-callable methods:** `set_led(r,g,b)`, `led_on/off()`,
  `set_buzzer(freq,vol)`, `buzzer_on/off()`, `set_alarm_high(...)`,
  `set_alarm_low(...)`, `alarm_on()`, `alarm_off()`

## Address note

The M5Stack store page lists `0x33`, but the official library uses **`0x32`**
(and the address is user-changeable, stored in flash). Keep `scan: true` — the
boot log prints the detected address. Setup also verifies the device-ID bytes
(`0x90`/`0x64`) and logs a clear error if the address is wrong.

## Status

Both example configs are verified: `esphome config` valid and `esphome compile`
builds firmware for ATOM and ESP32-PoE. The I2C protocol is taken from M5Stack's
own [M5Unit-Thermal2](https://github.com/m5stack/M5Unit-Thermal2) library.

The live 32×24 thermal **image** / web-JPEG viewer is not implemented yet (ATOM /
ESP32-PoE are headless sensor nodes); the processed temperatures above are what
shows up in Home Assistant.

## Credits & licence

MIT. I2C protocol reverse-mapped from M5Stack's MIT-licensed
[M5Unit-Thermal2](https://github.com/m5stack/M5Unit-Thermal2). Not affiliated with
or endorsed by M5Stack.
