#include <Arduino.h>
#include <Wire.h>

#define BMP280_ADDRESS 0x76 // Dirección I2C del BMP280

struct BMP280_Calibration
{
  uint16_t dig_T1;
  int16_t dig_T2, dig_T3;
  uint16_t dig_P1;
  int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
};

BMP280_Calibration bmp280_calib;
int32_t t_fine;

void readBMP280Calibration()
{
  Wire.beginTransmission(BMP280_ADDRESS);
  Wire.write(0x88); // Registro de calibración
  Wire.endTransmission();

  Wire.requestFrom(BMP280_ADDRESS, 24);

  bmp280_calib.dig_T1 = Wire.read() | (Wire.read() << 8);
  bmp280_calib.dig_T2 = Wire.read() | (Wire.read() << 8);
  bmp280_calib.dig_T3 = Wire.read() | (Wire.read() << 8);
  bmp280_calib.dig_P1 = Wire.read() | (Wire.read() << 8);
  bmp280_calib.dig_P2 = Wire.read() | (Wire.read() << 8);
  bmp280_calib.dig_P3 = Wire.read() | (Wire.read() << 8);
  bmp280_calib.dig_P4 = Wire.read() | (Wire.read() << 8);
  bmp280_calib.dig_P5 = Wire.read() | (Wire.read() << 8);
  bmp280_calib.dig_P6 = Wire.read() | (Wire.read() << 8);
  bmp280_calib.dig_P7 = Wire.read() | (Wire.read() << 8);
  bmp280_calib.dig_P8 = Wire.read() | (Wire.read() << 8);
  bmp280_calib.dig_P9 = Wire.read() | (Wire.read() << 8);
}

void initBMP280()
{
  // Leer la calibración del BMP280
  readBMP280Calibration();

  // Configurar el BMP280
  Wire.beginTransmission(BMP280_ADDRESS);
  Wire.write(0xF4); // Registro de control de medición
  Wire.write(0x27); // Configuración: temperatura x1, presión x1, modo normal
  Wire.endTransmission();
}

float readBMP280Temperature()
{
  Wire.beginTransmission(BMP280_ADDRESS);
  Wire.write(0xFA); // Registro de temperatura MSB
  Wire.endTransmission();

  Wire.requestFrom(BMP280_ADDRESS, 3);
  if (Wire.available() >= 3)
  {
    uint16_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    uint8_t xlsb = Wire.read();

    int32_t adc_T = (msb << 12) | (lsb << 4) | (xlsb >> 4);

    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)bmp280_calib.dig_T1 << 1))) * ((int32_t)bmp280_calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)bmp280_calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)bmp280_calib.dig_T1))) >> 12) * ((int32_t)bmp280_calib.dig_T3)) >> 14;

    t_fine = var1 + var2;
    float temp = (t_fine * 5 + 128) >> 8;
    return temp / 100.0;
  }
  return 0.0;
}

float readBMP280Pressure()
{
  Wire.beginTransmission(BMP280_ADDRESS);
  Wire.write(0xF7); // Registro de presión MSB
  Wire.endTransmission();

  Wire.requestFrom(BMP280_ADDRESS, 3);
  if (Wire.available() >= 3)
  {
    uint16_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    uint8_t xlsb = Wire.read();

    int32_t adc_P = (msb << 12) | (lsb << 4) | (xlsb >> 4);

    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)bmp280_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)bmp280_calib.dig_P5) << 17);
    var2 = var2 + ((int64_t)bmp280_calib.dig_P4 << 35);
    var1 = ((var1 * var1 * (int64_t)bmp280_calib.dig_P3) >> 8) + ((var1 * (int64_t)bmp280_calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)bmp280_calib.dig_P1) >> 33;

    if (var1 == 0)
    {
      return 0.0; // Evitar división por cero
    }

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)bmp280_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)bmp280_calib.dig_P8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (((int64_t)bmp280_calib.dig_P7) << 4);

    return (float)p / 256.0;
  }
  return 0.0;

}

void setup()
{
  Serial.begin(115200);
  Wire.begin();

  // Inicializar el BMP280
  initBMP280();
}

void loop()
{
  float temperature = readBMP280Temperature();
  float pressure = readBMP280Pressure();

  Serial.print("Temperatura = ");
  Serial.print(temperature);
  Serial.println(" *C");

  Serial.print("Presion = ");
  Serial.print(pressure);
  Serial.println(" hPa");

  delay(1000);
}



