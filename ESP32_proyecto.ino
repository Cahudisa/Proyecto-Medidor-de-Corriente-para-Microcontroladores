// ------------------------------------------------------------
// CONFIGURACIÓN PARA BLYNK CON ESP32 Y SENSOR INA228
// ------------------------------------------------------------

// Datos necesarios para conectar con Blynk
#define BLYNK_TEMPLATE_ID "TMPL2cA0Jl21f"
#define BLYNK_TEMPLATE_NAME "Proteus"
#define BLYNK_AUTH_TOKEN "cAp-n7WnmrCjnp0Ny1YMqxFUBJMtup5l"

#include <Wire.h>                   // Librería para I2C
#include <BlynkSimpleEsp32.h>      // Blynk para ESP32 con Wi-Fi

char auth[] = BLYNK_AUTH_TOKEN;     // Token de autenticación Blynk
char ssid[] = "TU_SSID";            // <-- Reemplaza con tu red Wi-Fi
char pass[] = "TU_PASSWORD";        // <-- Reemplaza con tu contraseña Wi-Fi

BlynkTimer timer;                   // Temporizador para funciones periódicas

// Direcciones de registros del INA228
#define INA228_ADDR 0x40            // Dirección I2C del INA228
#define REG_CONFIG 0x00             // Registro de configuración
#define REG_SHUNT_VOLTAGE 0x04      // Registro de voltaje en la resistencia shunt
#define ADCRANGE_1_CONFIG 0x8000    // Configuración para ADCRANGE = 1

// Pines actualizados para ESP32 (seguros para uso general)
const int modoPin = 4;             // GPIO4 para indicar modo manual/automático
const int rangoPin = 14;            // GPIO14 para controlar selección de shunt

int modo = 0;                       // 0 = automático, 1 = manual
int rango = 0;                      // 0 = microamperios, 1 = miliamperios
float rshunt = 36.0;                // Valor inicial de la resistencia shunt (36Ω)


// Lee el botón virtual V4 de la app Blynk para cambiar el modo
BLYNK_WRITE(V4) {
  modo = param.asInt();            // 0 = automático, 1 = manual
  digitalWrite(modoPin, modo);     // Enciende/apaga el pin según el modo
}

// Lee el botón virtual V5 para cambiar el rango (solo si modo = manual)
BLYNK_WRITE(V5) {
  rango = param.asInt();           // 0 = uA (36Ω), 1 = mA (36mΩ)
}


// Función que escribe un registro en el INA228
void writeINA228Register(uint8_t reg, uint16_t value) {
  Wire.beginTransmission(INA228_ADDR);
  Wire.write(reg);                  // Dirección del registro
  Wire.write(value >> 8);           // Byte alto
  Wire.write(value & 0xFF);         // Byte bajo
  Wire.endTransmission();
}


// Función que lee el voltaje en la shunt desde el INA228
float readShuntVoltage() {
  Wire.beginTransmission(INA228_ADDR);
  Wire.write(REG_SHUNT_VOLTAGE);
  Wire.endTransmission(false);     // Mantiene el bus activo

  Wire.requestFrom(INA228_ADDR, 3); // Solicita 3 bytes
  uint32_t raw = 0;
  raw |= Wire.read() << 16;
  raw |= Wire.read() << 8;
  raw |= Wire.read();

  // Cada bit equivale a 78.125 nV (según datasheet)
  float voltage = raw * 0.000000078125;
  return voltage;
}


// Función que lee los datos del INA228 y los envía a Blynk
void sendINA228DataToBlynk() {
  if (modo) {
    // MODO MANUAL: el usuario controla el rango
    if (rango == 0) {
      rshunt = 36.0;               // 36Ω → microamperios
      digitalWrite(rangoPin, LOW);
    } else {
      rshunt = 0.036;              // 36mΩ → miliamperios
      digitalWrite(rangoPin, HIGH);
    }
  } else {
    // MODO AUTOMÁTICO: el ESP32 decide el rango
    float temp_voltage = readShuntVoltage();
    float temp_current = temp_voltage / 36.0;  // Suponiendo 36Ω

    if (temp_current <= 0.001) {   // Corriente baja (≤1000 uA)
      rshunt = 36.0;
      digitalWrite(rangoPin, LOW);
    } else {
      rshunt = 0.036;
      digitalWrite(rangoPin, HIGH);
    }
  }

  // Lectura real
  float vshunt = readShuntVoltage();
  float current = vshunt / rshunt;       // Corriente en amperios
  float bus_voltage = 3.3;               // Valor fijo (se puede leer otro registro)
  float power = current * bus_voltage;   // Potencia estimada

  // Envío a Blynk
  Blynk.virtualWrite(V1, current * 1000.0); // mA
  Blynk.virtualWrite(V2, bus_voltage);     // V
  Blynk.virtualWrite(V3, power * 1000.0);   // mW
}


// Configuración inicial
void setup() {
  Serial.begin(115200);           // Monitor serial
  Wire.begin();                   // Inicia I2C (SDA, SCL por defecto)

  pinMode(rangoPin, OUTPUT);      // Pin para cambiar shunt
  pinMode(modoPin, OUTPUT);       // Pin para indicar modo

  digitalWrite(rangoPin, HIGH);   // Inicia con shunt de 36Ω

  // Conecta a Wi-Fi y Blynk
  Blynk.begin(auth, ssid, pass);

  // Configura el INA228
  writeINA228Register(REG_CONFIG, ADCRANGE_1_CONFIG);

  // Llama a la función de lectura cada segundo
  timer.setInterval(1000L, sendINA228DataToBlynk);
}


// Bucle principal
void loop() {
  Blynk.run();     // Mantiene la conexión con Blynk
  timer.run();     // Ejecuta las funciones periódicas
}
