// Adaptado de: https://github.com/ahmadlogs/arduino-ide-examples/blob/main/esp32-gps-tracker/esp32-gps-tracker.ino
#include <Arduino.h>
#include <TinyGPS++.h>

#define RXD2 16
#define TXD2 17
HardwareSerial neogps(1);

void print_speed();

TinyGPSPlus gps;

void setup() {
  Serial.begin(115200);
  neogps.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

void loop() {
  boolean newData = false;
  for (unsigned long start = millis(); millis() - start < 1000;) {
    while (neogps.available()) {
      if (gps.encode(neogps.read())) {
        newData = true;
      }
    }
  }

  if (newData) {
    newData = false;
    Serial.println(gps.satellites.value());
    print_speed();
  } else {
    Serial.println("No Data");
  }
}

void print_speed() {
  if (gps.location.isValid()) {
    Serial.print("Lat: ");
    Serial.println(gps.location.lat(), 6);
    
    Serial.print("Lng: ");
    Serial.println(gps.location.lng(), 6);

    Serial.print("Speed: ");
    Serial.println(gps.speed.kmph());

    Serial.print("SAT: ");
    Serial.println(gps.satellites.value());

    Serial.print("ALT: ");
    Serial.println(gps.altitude.meters(), 0);
  } else {
    Serial.println("No Data");
  }
}
