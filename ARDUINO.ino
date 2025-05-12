// Define el ID de la plantilla, nombre y token de autenticación de Blynk
#define BLYNK_TEMPLATE_ID "TMPL2cA0Jl21f"
#define BLYNK_TEMPLATE_NAME "Proteus"
#define BLYNK_AUTH_TOKEN "cAp-n7WnmrCjnp0Ny1YMqxFUBJMtup5l"

// Incluye las librerías necesarias
#include <Wire.h>                   // Librería para comunicación I2C
#include <Adafruit_INA219.h>       // Librería para el sensor INA219
#include <BlynkSimpleStream.h>     // Librería de Blynk para comunicación por Serial

char auth[] = BLYNK_AUTH_TOKEN;    // Guarda el token en una variable para usar con Blynk

Adafruit_INA219 ina219;            // Crea un objeto para manejar el INA219
BlynkTimer timer;                  // Objeto para temporizar tareas periódicas

const int modoPin = 13;            // Pin que representa el modo (manual o automático)
const int rangoPin = 12;           // Pin que se usará para controlar el rango

int modo = 0;                      // Variable que indica el modo de operación: 0 = automático, 1 = manual
int rango = 0;                     // Variable que indica el rango seleccionado: 0 o 1

// Esta función se ejecuta cuando el valor del botón V4 cambia en la app de Blynk
BLYNK_WRITE(V4) {
  modo = param.asInt();            // Lee el valor del botón (0 o 1)
  digitalWrite(modoPin, modo);    // Escribe ese valor en el pin de salida
}

// Esta función se ejecuta cuando el botón V5 cambia en la app de Blynk
BLYNK_WRITE(V5) {
  rango = param.asInt();          // Lee el valor del botón (0 o 1)
  //digitalWrite(rangoPin, rango); // Comentado: ya no se escribe directamente aquí
}

// Esta función se llama cada segundo para leer datos del INA219 y enviarlos a Blynk
void sendINA219DataToBlynk() {
  float busVoltage = ina219.getBusVoltage_V();      // Lee el voltaje del bus en voltios
  float rawCurrent = ina219.getCurrent_mA();        // Lee la corriente en mA
  float power_mW   = ina219.getPower_mW() / 100;    // Lee la potencia y la escala
  float currentSend = 0;                            // Variable para la corriente a enviar

  if(modo){ // MODO MANUAL
    digitalWrite(rangoPin, rango);  // Escribe el rango seleccionado por el usuario
    if (rango == HIGH) {            // Si el rango es "alto" (mA)
      currentSend = rawCurrent / 10; // Se escala la corriente
    } else {
      currentSend = rawCurrent;     // Si el rango es bajo (uA), se envía directo
    }
  } else { // MODO AUTOMÁTICO
    if (rawCurrent <= 100) {        // Si la corriente es baja (menor a 100 mA)
      digitalWrite(rangoPin, HIGH); // Usa rango bajo (mayor sensibilidad)
      currentSend = rawCurrent;
    } else {                        // Si la corriente es alta
      digitalWrite(rangoPin, LOW);  // Cambia a rango bajo (menos sensibilidad)
      currentSend = rawCurrent / 10; // Escala para no saturar
    }
  }

  // Envío de datos a Blynk
  Blynk.virtualWrite(V1, currentSend);  // Corriente
  Blynk.virtualWrite(V2, busVoltage);   // Voltaje
  Blynk.virtualWrite(V3, power_mW);     // Potencia
}

// Configuración inicial
void setup() {
  pinMode(rangoPin, INPUT);        // Configura rangoPin como entrada
  pinMode(modoPin, OUTPUT);        // Configura modoPin como salida

  Serial.begin(9600);              // Inicializa comunicación Serial (para Blynk)
  Blynk.begin(Serial, auth);       // Inicia Blynk con el token

  ina219.begin();                  // Inicia el sensor INA219
  timer.setInterval(1000L, sendINA219DataToBlynk); // Ejecuta función cada 1000 ms (1 segundo)
}

// Bucle principal
void loop() {
  Blynk.run();                     // Mantiene conexión con Blynk
  timer.run();                     // Ejecuta las tareas programadas por el temporizador
}
