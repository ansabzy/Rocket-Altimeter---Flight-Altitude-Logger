/*
  Rocket Altimeter Logger
  Arduino Nano + BMP280 (I2C) + SD card module

  Wiring:
    BMP280:  SDA -> A4, SCL -> A5, VIN -> 3.3V or 5V (check breakout), GND -> GND
    SD card: CS -> D10, MOSI -> D11, MISO -> D12, SCK -> D13 (standard SPI)

  Logic:
    1. On startup, take an average ground-level pressure reading as the reference.
    2. Sample pressure/altitude as fast as the sensor allows, log to SD.
    3. Track the highest altitude seen (apogee) in real time.
    4. Detect landing (altitude stable/near-zero for N samples) and stop logging.
    5. After landing, altitude data + apogee are on the SD card for retrieval.
*/

#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <SPI.h>
#include <SD.h>

Adafruit_BMP280 bmp;              // I2C BMP280
const int SD_CS_PIN = 10;

float groundPressure_hPa = 0;
float maxAltitude_m = 0;
float lastAltitude_m = 0;
int stableCount = 0;
bool landed = false;
bool launched = false;

const float LAUNCH_THRESHOLD_M = 5.0;   // altitude jump that counts as liftoff
const float LAND_STABLE_BAND_M = 0.5;   // how tight altitude must hold to call it "landed"
const int LAND_STABLE_SAMPLES = 50;     // consecutive stable samples before declaring landed

File logFile;
unsigned long sampleCount = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!bmp.begin(0x76)) {   // some breakouts use 0x77 — check yours if this fails
    Serial.println(F("BMP280 not found. Check wiring."));
    while (1) delay(10);
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                   Adafruit_BMP280::SAMPLING_X2,   // temperature oversampling
                 Adafruit_BMP280::SAMPLING_X16,  // pressure oversampling (precision)
                   Adafruit_BMP280::FILTER_X4,
                   Adafruit_BMP280::STANDBY_MS_1);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("SD card init failed."));
    while (1) delay(10);
  }

  // Average several readings on the pad to get a stable ground reference
  float sum = 0;
  const int groundSamples = 50;
  for (int i = 0; i < groundSamples; i++) {
    sum += bmp.readPressure() / 100.0F;  // Pa -> hPa
    delay(20);
  }
  groundPressure_hPa = sum / groundSamples;

  logFile = SD.open("flight.csv", FILE_WRITE);
  if (logFile) {
    logFile.println(F("time_ms,pressure_hPa,altitude_m"));
    logFile.close();
  }

  Serial.print(F("Ground pressure (hPa): "));
  Serial.println(groundPressure_hPa);
  Serial.println(F("Ready. Waiting for launch..."));
}

void loop() {
  if (landed) return;  // stop sampling once landed

  float pressure_hPa = bmp.readPressure() / 100.0F;
  float altitude_m = bmp.readAltitude(groundPressure_hPa);  // relative to ground ref

  if (altitude_m > maxAltitude_m) {
    maxAltitude_m = altitude_m;
  }

  if (!launched && altitude_m > LAUNCH_THRESHOLD_M) {
    launched = true;
    Serial.println(F("Launch detected."));
  }

  // Landing detection: only check once we've actually launched
  if (launched) {
    if (abs(altitude_m - lastAltitude_m) < LAND_STABLE_BAND_M) {
      stableCount++;
    } else {
      stableCount = 0;
    }
    if (stableCount > LAND_STABLE_SAMPLES) {
      landed = true;
      Serial.println(F("Landing detected. Logging stopped."));
      Serial.print(F("Apogee (m): "));
      Serial.println(maxAltitude_m);

      logFile = SD.open("flight.csv", FILE_WRITE);
      if (logFile) {
        logFile.print(F("APOGEE_M,"));
        logFile.println(maxAltitude_m);
        logFile.close();
      }
    }
  }

  lastAltitude_m = altitude_m;

  logFile = SD.open("flight.csv", FILE_WRITE);
  if (logFile) {
    logFile.print(millis());
    logFile.print(",");
    logFile.print(pressure_hPa, 2);
    logFile.print(",");
    logFile.println(altitude_m, 2);
    logFile.close();
  }

  sampleCount++;
  // No delay() here on purpose — sample as fast as the sensor/SD allow during flight.
  // If you find SD writes are choking your sample rate, buffer readings in a small
  // array in RAM and flush to SD every N samples instead of every loop.
}
