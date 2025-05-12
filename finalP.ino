// Datos necesarios para conectar con Blynk
#define BLYNK_TEMPLATE_ID "TMPL2cA0Jl21f"
#define BLYNK_TEMPLATE_NAME "Proteus"
#define BLYNK_AUTH_TOKEN "cAp-n7WnmrCjnp0Ny1YMqxFUBJMtup5l"

#include <Wire.h>                   // Comunicación I2C
#include <BlynkSimpleStream.h>      // Blynk por puerto serial

char auth[] = BLYNK_AUTH_TOKEN;     // Token de autenticación Blynk
BlynkTimer timer;                   // Temporizador para enviar datos cada cierto tiempo

// Direcciones de registros del INA228
#define INA228_ADDR 0x40            // Dirección I2C por defecto del INA228
#define REG_CONFIG 0x00             // Registro de configuración
#define REG_SHUNT_VOLTAGE 0x04      // Registro de voltaje en la resistencia shunt
#define ADCRANGE_1_CONFIG 0x8000    // Valor con ADCRANGE = 1 (para shunts grandes)

const int modoPin = 13;             // Pin de salida para controlar modo (manual o automático)
const int rangoPin = 12;            // Pin de salida para cambiar de rango (shunt 36Ω o 36mΩ)

int modo = 0;                       // Variable que guarda el modo (0: automático, 1: manual)
int rango = 0;                      // Variable que guarda el rango (0: uA, 1: mA)
float rshunt = 36.0;                // Valor de la resistencia shunt (inicia con 36 ohm para microamperios)


// Lee el botón V4 de la app Blynk para cambiar el modo
BLYNK_WRITE(V4) {
  modo = param.asInt();            // 0 = automático, 1 = manual
  digitalWrite(modoPin, modo);     // Actualiza el pin 13 según el modo
}

// Lee el botón V5 de la app Blynk para cambiar el rango (solo en modo manual)
BLYNK_WRITE(V5) {
  rango = param.asInt();           // 0 = uA (36Ω), 1 = mA (36mΩ)
}


// Función que escribe un registro del INA228
void writeINA228Register(uint8_t reg, uint16_t value) {
  Wire.beginTransmission(INA228_ADDR);
  Wire.write(reg);                 // Registro al que se va a escribir
  Wire.write(value >> 8);          // Parte alta del valor (MSB)
  Wire.write(value & 0xFF);        // Parte baja del valor (LSB)
  Wire.endTransmission();
}


// Función que lee el voltaje en la shunt desde el INA228
float readShuntVoltage() {
  Wire.beginTransmission(INA228_ADDR);
  Wire.write(REG_SHUNT_VOLTAGE);
  Wire.endTransmission(false);     // Repetir sin liberar el bus

  Wire.requestFrom(INA228_ADDR, 3); // Solicita 3 bytes
  uint32_t raw = 0;
  raw |= Wire.read() << 16;         // Byte más significativo
  raw |= Wire.read() << 8;
  raw |= Wire.read();              // Byte menos significativo

  // Cada bit equivale a 78.125 nV (según datasheet)
  float voltage = raw * 0.000000078125;
  return voltage;
}


// Esta función se ejecuta cada segundo para leer el INA228 y enviar datos a Blynk
void sendINA228DataToBlynk() {
  if (modo) {
    // MODO MANUAL (controlado desde Blynk)
    if (rango == 0) {
      rshunt = 36.0;               // Rango para microamperios
      digitalWrite(rangoPin, LOW);
    } else {
      rshunt = 0.036;              // Rango para miliamperios
      digitalWrite(rangoPin, HIGH);
    }
  } else {
    // MODO AUTOMÁTICO (elige el rango según la corriente medida)
    float temp_voltage = readShuntVoltage();
    float temp_current = temp_voltage / 36;  // Supone el rango menor para decisión

    if (temp_current <= 1000) {   // Comparacion con unidades de uA
      rshunt = 36.0;               // Corriente baja, usar shunt de 36Ω
      digitalWrite(rangoPin, LOW);
    } else {
      rshunt = 0.036;              // Corriente alta, usar shunt de 36mΩ
      digitalWrite(rangoPin, HIGH);
    }
  }

  float vshunt = readShuntVoltage();     // Lee voltaje en la shunt
  float current = vshunt / rshunt;       // Calcula la corriente (Ohm's law)
  float bus_voltage = 3.3;               // Valor fijo (puede mejorarse leyendo otro registro)
  float power = current * bus_voltage;   // Calcula potencia

  // Envía los datos a Blynk
  Blynk.virtualWrite(V1, current * 1000.0); // Corriente en mA
  Blynk.virtualWrite(V2, bus_voltage);     // Voltaje del bus
  Blynk.virtualWrite(V3, power * 1000.0);   // Potencia en mW
}


// Configuración inicial
void setup() {
  pinMode(rangoPin, OUTPUT);     // Pin que controla el cambio de shunt
  pinMode(modoPin, OUTPUT);      // Pin que indica modo manual o automático
  digitalWrite(rangoPin, HIGH);  // Por defecto, iniciar en shunt de 36Ω

  Serial.begin(9600);            // Comunicación serial (para Blynk)
  Wire.begin();                  // Inicia comunicación I2C
  Blynk.begin(Serial, auth);     // Inicia Blynk por puerto serial

  // Configura el INA228: ADCRANGE = 1
  writeINA228Register(REG_CONFIG, ADCRANGE_1_CONFIG);

  // Ejecuta cada segundo la función que envía datos
  timer.setInterval(1000L, sendINA228DataToBlynk);
}


// Bucle principal
void loop() {
  Blynk.run();      // Mantiene la conexión con Blynk activa
  timer.run();      // Ejecuta tareas periódicas (cada 1 segundo)
}
