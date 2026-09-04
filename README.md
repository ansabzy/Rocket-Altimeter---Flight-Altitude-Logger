# Rocket Altimeter — BMP280 Flight Data Logger

A model rocket avionics payload built around an Arduino Nano and a BMP280 barometric pressure sensor. Logs pressure and derived altitude to an SD card during flight and reports apogee (max altitude reached).

## Hardware

- Arduino Nano
- BMP280 barometric pressure/temperature sensor (I2C)
- microSD card module (SPI)
- Utilized 1S battery (3.3V)
- Small model rocket airframe

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

The BMP280 measures air pressure, which decreases predictably with altitude. Right before launch, the code takes an averaged pressure reading on the pad as a reference point. Every following reading gets converted to a height above the pad using the barometric pressure altitude formula (coded internally by the Adafruit_BMP280 library).

Apogee is tracked by continuously comparing each new altitude reading against the highest value seen so far during the flight. A simple threshold-based check on altitude looks for two events:
- **Launch**: a fast enough increase in altitude
- **Landing**: several consecutive readings that stay within a small band of each other

Once landing is detected, logging stops and apogee is written to the SD card alongside the full time-series data.

## Repository Contents

- `rocket_altimeter.ino` — main flight computer sketch
- `plot_comparison.py` — plots the OpenRocket simulation against logged data and reports the apogee difference
- `OpenRocket_Sim_Data.csv` — theoretical altitude curve read off the OpenRocket simulation plot (export directly from OpenRocket for exact values — File > Export flight data)
- `DIYAltimeter_experimental_data.csv` — real flight log, written by the sketch to the SD card (populate this after an actual launch)

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

## Testing
The system was assembled and tested as a functional flight-data logging system before being flown on the model rocket.

Initial testing showed a problem with the pressure readings during the first launch. The resulting altitude data was skewed; this was likely because the air pressure readings by the BMP280 sensor were not accurate to the atmospheric pressure. The rocket airframe was modified by adding small vent holes to improve accuracy of BMP280 readings. This modification was intended to produce more representative pressure measurements during flight.

The physical electronics assembly also required several iterations of wiring and component placement. Because of the limited 2.5 cm airframe diameter, the final design used a vertically stacked arrangement of the electronics rather than placing all components side by side.

Flight data is stored on the microSD card as a CSV file and can be processed using plot_comparison.py to compare measured altitude against the OpenRocket simulation.

## Limitations

- No temperature compensation or flight sensor calibration beyond the pad-pressure reference at the start of every launch
- Since altitude and apogee are recorded on the SD card, the board losing power will erase data from SD card module. EEPROM (on board memory) can be used to fix this issue.
- SD file is opened/closed every loop for write safety, which caps the max practical sample rate — buffering in RAM and flushing periodically would allow faster sampling
- Launch and landing thresholds are tuned for this specific motor and rocket and would need retuning for a different rocket or motor.

### Future Improvements

- Using a smaller custom PCB or compact microcontroller to reduce the size of the electronics package
- Improving the pressure-vent design and sensor isolation
- Adding an accelerometer or IMU to provide additional flight-state data
- Increasing the sensor sampling rate
- Adding EEPROM or other non-volatile storage for critical flight information
- Testing multiple flights to determine the repeatability and accuracy of the altitude measurements

## License

MIT
