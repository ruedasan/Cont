// Grupo 4

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <TinyGPS++.h>
#include <QMC5883LCompass.h>
#include <SD.h>

Adafruit_BMP280 bmp;  // BMP280 sensor (I2C)
TinyGPSPlus gps;       // Object to handle GPS
#define RXD2 16
#define TXD2 17
HardwareSerial neogps(1);  // Communication with NEO-6M GPS module
QMC5883LCompass compass;

int calibrationData[3][2];
bool changed = false;
bool done = false;
int t = 0;
int c = 0;

bool calibrated = false;
int hourBogota;

long accelX, accelY, accelZ;
float gForceX, gForceY, gForceZ;

long gyroX, gyroY, gyroZ;
float rotX, rotY, rotZ;

void setupMPU();
void processAccelData();
void recordAccelRegisters();
void processGyroData();
void recordGyroRegisters();

void printData();

File dataFile;
unsigned long previousMillis = 0;
const long interval = 30000;  // Intervalo de 30 segundos
void appendToFile();
int fileCounter = 0;

void setup() {
  Serial.begin(115200);  // Serial communication with the Arduino IDE monitor
  neogps.begin(9600, SERIAL_8N1, RXD2, TXD2);  // GPS module configuration
  unsigned status = bmp.begin(0x76);  // BMP280 initialization
  compass.init();  // Compass initialization
    Wire.begin();
  setupMPU();

    if (!SD.begin(5)) {  // Pin 10 para el módulo CS de la tarjeta SD
    Serial.println("Error al iniciar la tarjeta SD");
    while (1);
    String fileName = "/data" +String(fileCounter) + ".txt";
    dataFile = SD.open(fileName, FILE_APPEND);
  }

  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or try a different address!"));
    while (1) delay(10);
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X2, Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::FILTER_X16, Adafruit_BMP280::STANDBY_MS_500);
}

void loop() {
  unsigned long currentMillis = millis();
  boolean newData = false;

  for (unsigned long start = millis(); millis() - start < 1000;) {
    while (neogps.available()) {
      if (gps.encode(neogps.read())) {
        newData = true;
      }
    }
  }

  if (!calibrated){
        //If not calibrated
      int x, y, z;
 
      //Read compass values
      compass.read();
 
     // Return XYZ readings
      x = compass.getX();
      y = compass.getY();
      z = compass.getZ();
 
      changed = false;
 
      if(x < calibrationData[0][0]) {
        calibrationData[0][0] = x;
        changed = true;
      }
      if(x > calibrationData[0][1]) {
        calibrationData[0][1] = x;
        changed = true;
      }
 
      if(y < calibrationData[1][0]) {
        calibrationData[1][0] = y;
        changed = true;
      }
      if(y > calibrationData[1][1]) {
        calibrationData[1][1] = y;
        changed = true;
      }
 
      if(z < calibrationData[2][0]) {
        calibrationData[2][0] = z;
        changed = true;
      }
      if(z > calibrationData[2][1]) {
        calibrationData[2][1] = z;
        changed = true;
      }
 
      if (changed && !done) {
        Serial.println("CALIBRATING... Keep moving your sensor around.");
        c = millis();
      }
        t = millis();
 
 
      if ( (t - c > 5000) && !done) {
        done = true;
        Serial.println("DONE.");
        Serial.println();
 
        Serial.print("compass.setCalibration(");
        Serial.print(calibrationData[0][0]);
        Serial.print(", ");
        Serial.print(calibrationData[0][1]);
        Serial.print(", ");
        Serial.print(calibrationData[1][0]);
        Serial.print(", ");
        Serial.print(calibrationData[1][1]);
        Serial.print(", ");
        Serial.print(calibrationData[2][0]);
        Serial.print(", ");
        Serial.print(calibrationData[2][1]);
        Serial.println(");");
 
        compass.setCalibration( calibrationData[0][0], calibrationData[0][1], calibrationData[1][0],
                                calibrationData[1][1], calibrationData[2][0], calibrationData[2][1]);
        calibrated = true;
        }
    }

  if (currentMillis - previousMillis >= interval) {
    // Agregar datos al archivo cada 30 segundos
    appendToFile();
    previousMillis = currentMillis;
  }else{
    dataFile.close();

  }

    delay(250);
    if (newData) {
    newData = false;
    printData(); // Imprimir datos del GPS
  } else {
    Serial.println("No GPS Data");
  }
  hourBogota = gps.time.hour() - 5;
    if (hourBogota < 0) {
      hourBogota += 24;  // Manejar casos donde la resta resulta en un valor negativo
    }

  recordAccelRegisters();
  recordGyroRegisters();
  }


  void setupMPU(){
  Wire.beginTransmission(0b1101000); //This is the I2C address of the MPU (b1101000/b1101001 for AC0 low/high datasheet sec. 9.2)
  Wire.write(0x6B); //Accessing the register 6B - Power Management (Sec. 4.28)
  Wire.write(0b00000000); //Setting SLEEP register to 0. (Required; see Note on p. 9)
  Wire.endTransmission();  
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1B); //Accessing the register 1B - Gyroscope Configuration (Sec. 4.4) 
  Wire.write(0x00000000); //Setting the gyro to full scale +/- 250deg./s 
  Wire.endTransmission(); 
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1C); //Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5) 
  Wire.write(0b00000000); //Setting the accel to +/- 2g
  Wire.endTransmission(); 
}

void processAccelData(){
  gForceX = accelX / 16384.0;
  gForceY = accelY / 16384.0; 
  gForceZ = accelZ / 16384.0;
}

void recordAccelRegisters() {
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x3B); //Starting register for Accel Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000,6); //Request Accel Registers (3B - 40)
  while(Wire.available() < 6);
  accelX = Wire.read()<<8|Wire.read(); //Store first two bytes into accelX
  accelY = Wire.read()<<8|Wire.read(); //Store middle two bytes into accelY
  accelZ = Wire.read()<<8|Wire.read(); //Store last two bytes into accelZ
  processAccelData();
}

void processGyroData() {
  rotX = gyroX / 131.0;
  rotY = gyroY / 131.0; 
  rotZ = gyroZ / 131.0;
}

void recordGyroRegisters() {
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x43); //Starting register for Gyro Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000,6); //Request Gyro Registers (43 - 48)
  while(Wire.available() < 6);
  gyroX = Wire.read()<<8|Wire.read(); //Store first two bytes into accelX
  gyroY = Wire.read()<<8|Wire.read(); //Store middle two bytes into accelY
  gyroZ = Wire.read()<<8|Wire.read(); //Store last two bytes into accelZ
  processGyroData();
}

void appendToFile() {

  //String fileName = "/data" +String(fileCounter) + ".txt";
  //dataFile = SD.open(fileName, FILE_APPEND);

  if (dataFile) {
    dataFile.println("Tiempo: " + String(hourBogota) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second()));
    dataFile.println("Datos de GPS:");
    dataFile.println("Latitud: " + String(gps.location.lat(), 6));
    dataFile.println("Longitud: " + String(gps.location.lng(), 6));
    dataFile.println("Datos BMP280:");
    dataFile.println("Temperatura = " + String(bmp.readTemperature()) + " *C");
    dataFile.println("Presión = " + String(bmp.readPressure()) + " Pa");
    dataFile.println("Altitud aproximada = " + String(bmp.readAltitude(1013.25)) + " m");
    dataFile.println("Datos MPU6050:");
    dataFile.println("Gyro (deg) X=" + String(rotX) + " Y=" + String(rotY) + " Z=" + String(rotZ));
    dataFile.println("Aceleración (g) X=" + String(gForceX) + " Y=" + String(gForceY) + " Z=" + String(gForceZ));

    int x, y, z;

    compass.read();

    x = compass.getX();
    y = compass.getY();
    z = compass.getZ();

    int azimut = compass.getAzimuth();

    azimut = (azimut + 360) % 360;

    dataFile.println();
    dataFile.print("Dirección: ");

    if ((azimut >= 337.5) || (azimut < 22.5))
      dataFile.print("Norte");
    if ((azimut >= 22.5) && (azimut < 67.5))
      dataFile.print("Noreste");
    if ((azimut >= 67.5) && (azimut < 112.5))
      dataFile.print("Este");
    if ((azimut >= 112.5) && (azimut < 157.5))
      dataFile.print("Sureste");
    if ((azimut >= 157.5) && (azimut < 202.5))
      dataFile.print("Sur");
    if ((azimut >= 202.5) && (azimut < 247.5))
      dataFile.print("Suroeste");
    if ((azimut >= 247.5) && (azimut < 292.5))
      dataFile.print("Oeste");
    if ((azimut >= 292.5) && (azimut < 337.5))
      dataFile.print("Noroeste");

    dataFile.print(" Azimut: ");
    dataFile.println(azimut);

    dataFile.println("--------------------");  // Separador entre conjuntos de datos

    
    //Serial.println("Datos agregados al archivo: " + fileName);
    fileCounter++;

    //dataFile.close();
  } else {
    Serial.println("Error al abrir el archivo");
  }
}



void printData() {
  if (gps.location.isValid()) {
    Serial.print("Time: ");
    Serial.print(hourBogota);
    Serial.print(":");
    Serial.print(gps.time.minute());
    Serial.print(":");
    Serial.println(gps.time.second());
    Serial.println("GPS Data:");
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);

    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
  } else {
    Serial.println("Invalid GPS Data");
  }

  Serial.println("BMP280 Data:");
  Serial.print(F("Temperature = "));
  Serial.print(bmp.readTemperature());
  Serial.println(" *C");

  Serial.print(F("Pressure = "));
  Serial.print(bmp.readPressure());
  Serial.println(" Pa");

  Serial.print(F("Approx altitude = "));
  Serial.print(bmp.readAltitude(1013.25)); /* Adjusted to local prediction */
  Serial.println(" m");

  Serial.println("MPU6050 Data:");
  Serial.print("Gyro (deg)");
  Serial.print(" X=");
  Serial.print(rotX);
  Serial.print(" Y=");
  Serial.print(rotY);
  Serial.print(" Z=");
  Serial.print(rotZ);
  Serial.print(" Accel (g)");
  Serial.print(" X=");
  Serial.print(gForceX);
  Serial.print(" Y=");
  Serial.print(gForceY);
  Serial.print(" Z=");
  Serial.println(gForceZ);

    int x, y, z;

  //Read compass values
  compass.read();

  //Return XYZ readings
  x = compass.getX();
  y = compass.getY();
  z = compass.getZ();

  int azimut = compass.getAzimuth();


  //Write direction
  //Reemplaza las condiciones existentes para todas las direcciones con las siguientes
  azimut = (azimut + 360) % 360; // Asegura que azimut esté en el rango de 0 a 360 grados
  Serial.println("GY-273 Data:");

  if ((azimut >= 337.5) || (azimut < 22.5))
    Serial.print("North     ");
  if ((azimut >= 22.5) && (azimut < 67.5))
    Serial.print("North-East");
  if ((azimut >= 67.5) && (azimut < 112.5))
    Serial.print("East      ");
  if ((azimut >= 112.5) && (azimut < 157.5))
    Serial.print("South-East");
  if ((azimut >= 157.5) && (azimut < 202.5))
    Serial.print("South     ");
  if ((azimut >= 202.5) && (azimut < 247.5))
    Serial.print("South-West");
  if ((azimut >= 247.5) && (azimut < 292.5))
    Serial.print("West      ");
  if ((azimut >= 292.5) && (azimut < 337.5))
    Serial.print("North-West");

  Serial.print(" Azimuth: ");
  Serial.print(azimut);


  Serial.println();
}
