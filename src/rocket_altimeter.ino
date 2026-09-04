```cpp
/*
  Rocket Altimeter
  Arduino Nano + BMP280 + SD Card

  The BMP280 measures pressure so the Arduino can estimate altitude.
  The altitude data is saved to the SD card so it can be compared
  with the OpenRocket simulation after the flight.

  Wiring:
  BMP280:
    SDA -> A4
    SCL -> A5
    VCC -> 3.3V
    GND -> GND

  SD Card:
    CS   -> D10
    MOSI -> D11
    MISO -> D12
    SCK  -> D13
*/

#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <SPI.h>
#include <SD.h>

Adafruit_BMP280 bmp;

const int SD_CS_PIN = 10;

// These keep track of the pressure and altitude during the flight
float groundPressure = 0;
float maxAltitude = 0;
float lastAltitude = 0;

int stableCount = 0;

bool launched = false;
bool landed = false;

// The rocket has to go above this altitude before launch is detected
const float LAUNCH_THRESHOLD = 5.0;

// Used to determine if the rocket has stopped moving vertically
const float LANDING_BAND = 0.5;

// Number of stable readings needed before calling it a landing
const int LANDING_SAMPLES = 50;

File logFile;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Start the BMP280
  // Most of the sensors I used had the address 0x76
  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 not found. Check wiring.");
    while (1) {
      delay(10);
    }
  }

  // Higher pressure oversampling gives more stable pressure readings
  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    Adafruit_BMP280::SAMPLING_X2,
    Adafruit_BMP280::SAMPLING_X16,
    Adafruit_BMP280::FILTER_X4,
    Adafruit_BMP280::STANDBY_MS_1
  );

  // Start the SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card failed to start.");
    while (1) {
      delay(10);
    }
  }

  /*
    Before launch, the rocket is sitting on the ground.
    I take several pressure readings and average them so that
    the starting pressure is not based on just one reading.
  */
  float pressureSum = 0;
  const int groundSamples = 50;

  for (int i = 0; i < groundSamples; i++) {
    pressureSum += bmp.readPressure() / 100.0;
    delay(20);
  }

  groundPressure = pressureSum / groundSamples;

  // Create the flight data file and add the column names
  logFile = SD.open("flight.csv", FILE_WRITE);

  if (logFile) {
    logFile.println("time_ms,pressure_hPa,altitude_m");
    logFile.close();
  }

  Serial.print("Ground pressure: ");
  Serial.print(groundPressure);
  Serial.println(" hPa");

  Serial.println("Altimeter ready. Waiting for launch...");
}

void loop() {

  // Once the rocket has landed, there is no reason to keep logging
  if (landed) {
    return;
  }

  // Read the current pressure and convert Pa to hPa
  float pressure = bmp.readPressure() / 100.0;

  // Calculate altitude using the pressure measured before launch
  float altitude = bmp.readAltitude(groundPressure);

  // Keep track of the highest altitude reached
  if (altitude > maxAltitude) {
    maxAltitude = altitude;
  }

  // Check if the rocket has left the launch pad
  if (!launched && altitude > LAUNCH_THRESHOLD) {
    launched = true;

    Serial.println("Launch detected.");
  }

  /*
    After launch, check if the altitude is staying mostly the same.
    If enough readings are close together, the rocket is probably
    back on the ground.
  */
  if (launched) {

    if (abs(altitude - lastAltitude) < LANDING_BAND) {
      stableCount++;
    }
    else {
      stableCount = 0;
    }

    if (stableCount > LANDING_SAMPLES) {

      landed = true;

      Serial.println("Landing detected.");
      Serial.print("Apogee: ");
      Serial.print(maxAltitude);
      Serial.println(" m");

      // Save the final apogee to the end of the file
      logFile = SD.open("flight.csv", FILE_WRITE);

      if (logFile) {
        logFile.print("APOGEE_M,");
        logFile.println(maxAltitude);
        logFile.close();
      }
    }
  }

  // Save this reading so it can be compared to the next one
  lastAltitude = altitude;

  /*
    Write the current reading to the SD card.
    millis() gives the time since the Arduino started.
  */
  logFile = SD.open("flight.csv", FILE_WRITE);

  if (logFile) {

    logFile.print(millis());
    logFile.print(",");

    logFile.print(pressure, 2);
    logFile.print(",");

    logFile.println(altitude, 2);

    logFile.close();
  }
}
```
