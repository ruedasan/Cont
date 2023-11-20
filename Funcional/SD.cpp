#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

const int chipSelect = 5;

void setup() {
  Serial.begin(115200);
  
  if (!SD.begin(chipSelect)) {
    Serial.println("Error al inicializar la tarjeta SD.");
    return;
  }
  Serial.println("Tarjeta SD inicializada correctamente.");

  // Crea un nuevo archivo llamado "datos.txt" en la raíz para escribir en él
  File archivo = SD.open("/datos.txt", FILE_WRITE);
  
  // Verifica si se pudo crear el archivo
  if (archivo) {
    Serial.println("Archivo 'datos.txt' creado correctamente.");
    
    // Escribe algunos datos en el archivo
    archivo.println("Hola, este es un ejemplo de datos en el archivo.");
    archivo.println("Puedes agregar más líneas según sea necesario.");

    // Cierra el archivo
    archivo.close();
  } else {
    Serial.println("Error al crear el archivo.");
  }
}

void loop() {
  // El loop no realiza ninguna acción en este ejemplo
}
