# Rocket Altimeter — BMP280 Flight Data Logger

A model rocket avionics payload built around an Arduino Nano and a BMP280 barometric pressure sensor. Logs pressure and derived altitude to an SD card during flight and reports apogee (max altitude reached).

## Hardware

- Arduino Nano
- BMP280 barometric pressure/temperature sensor (I2C)
- microSD card module (SPI)
- [Your battery/power setup here]
- [Your enclosure/mounting details here]

**Wiring**

| BMP280 | Nano |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | A4 |
| SCL | A5 |

| SD module | Nano |
|---|---|
| CS | D10 |
| MOSI | D11 |
| MISO | D12 |
| SCK | D13 |

## How It Works

The BMP280 measures air pressure, which decreases predictably with altitude. Right before launch, the code takes an averaged pressure reading on the pad as a reference point. Every subsequent reading gets converted to a height above the pad using the standard barometric altitude formula (handled internally by the Adafruit_BMP280 library).

Apogee is tracked by continuously comparing each new altitude reading against the highest value seen so far during the flight. A simple threshold-based check on altitude looks for two events:
- **Launch**: a fast enough increase in altitude
- **Landing**: several consecutive readings that stay within a small band of each other

Once landing is detected, logging stops and apogee is written to the SD card alongside the full time-series data.

## Repository Contents

- `rocket_altimeter.ino` — main flight computer sketch
- `plot_comparison.py` — plots the OpenRocket simulation against a logged `flight.csv` and reports the apogee difference
- `openrocket_sim_digitized.csv` — theoretical altitude curve read off the OpenRocket simulation plot (export directly from OpenRocket for exact values — File > Export flight data)
- `flight.csv` — real flight log, written by the sketch to the SD card (populate this after an actual launch)

### A note on the demo plot

Before flying, `plot_comparison.py` was tested against a synthetic dataset (`pipeline_test_synthetic_data.csv`) generated to look like plausible sensor output — this is **not** flight data and the file, its header, and the plot title all say so explicitly. It exists only to confirm the parsing/plotting code works correctly before it matters, the same way you'd unit-test any data pipeline before pointing it at real hardware. Once an actual flight log exists, that synthetic file gets deleted and `flight.csv` (real data) is what the plot and this README should reference.

## Dependencies

- [Adafruit_BMP280 library](https://github.com/adafruit/Adafruit_BMP280_Library)
- [Adafruit_Sensor library](https://github.com/adafruit/Adafruit_Sensor) (required by Adafruit_BMP280)
- Arduino `SD` and `SPI` libraries (bundled with the Arduino IDE)

## OpenRocket Simulation

The flight computer's launch/landing detection logic and altitude range were designed against an OpenRocket simulation of the airframe and motor combination, which predicts:

- Apogee of roughly 190–195 m at around T+6s
- Motor burnout around T+1s
- Recovery deployment at apogee, followed by a steady descent under parachute
- Touchdown around T+60s

This simulation was used to sanity-check the code's launch-detection threshold and expected sample counts, and to size the landing-detection window against the expected descent rate.

## Status / Testing

<!-- TODO: replace this section once you've run real hardware -->
[Describe how you actually validated this: e.g. bench test results, ground test conditions, or actual flight data. Include your real plot here once you have one. Don't fill this in with anything you didn't actually measure.]

## Known Limitations / Future Work

- No temperature compensation or per-flight sensor calibration beyond the pad-pressure reference
- Apogee is only written to SD, not to non-volatile memory (EEPROM) — if the board loses power right after landing before the final write completes, apogee could be lost
- SD file is opened/closed every loop for write safety, which caps the max practical sample rate — buffering in RAM and flushing periodically would allow faster sampling
- Launch/landing thresholds are tuned for this specific motor and descent rate and would need retuning for a different rocket

## License

MIT
