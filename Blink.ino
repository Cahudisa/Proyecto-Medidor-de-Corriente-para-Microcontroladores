#define BLYNK_TEMPLATE_ID "TMPL2cA0Jl21f"
#define BLYNK_TEMPLATE_NAME "Proteus"
#define BLYNK_AUTH_TOKEN "cAp-n7WnmrCjnp0Ny1YMqxFUBJMtup5l"

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <BlynkSimpleStream.h>

char auth[] = BLYNK_AUTH_TOKEN;

Adafruit_INA219 ina219;
BlynkTimer timer;

const int modoPin = 13;    // Este pin será controlado por Blynk (V4)
const int rangoPin = 12;   // Entrada de lectura de modo

int modo = 0;   // Variable para almacenar el valor de V4
int rango = 0;
// Función que se llama cuando V4 cambia desde la app Blynk
BLYNK_WRITE(V4) {
  modo = param.asInt();   // Lee 0 o 1 desde Blynk
  digitalWrite(modoPin, modo); // Escribe ese valor en el pin 13
}

BLYNK_WRITE(V5) {
      rango = param.asInt();   // Lee 0 o 1 desde Blynk
      //digitalWrite(rangoPin, rango); // Escribe ese valor en el pin 12
}

void sendINA219DataToBlynk() {
  float busVoltage = ina219.getBusVoltage_V();        
  float rawCurrent = ina219.getCurrent_mA();
  float power_mW   = ina219.getPower_mW() / 100;
  float currentSend = 0;
  

  if(modo){
    digitalWrite(rangoPin, rango);
    if (rango == HIGH) {
      currentSend = rawCurrent/10;
    } else {
      currentSend = rawCurrent;
    }
  } else {
    if (rawCurrent <= 100) {
      digitalWrite(rangoPin, HIGH);
      currentSend = rawCurrent;
    } else {
      digitalWrite(rangoPin, LOW);
      currentSend = rawCurrent/10;
    }
  }


  Blynk.virtualWrite(V1, currentSend); 
  Blynk.virtualWrite(V2, busVoltage);    
  Blynk.virtualWrite(V3, power_mW);      
}

void setup() {
  pinMode(rangoPin, INPUT);     // Pin de entrada
  pinMode(modoPin, OUTPUT);     // Pin de salida controlado por Blynk

  Serial.begin(9600);
  Blynk.begin(Serial, auth);

  ina219.begin();
  timer.setInterval(1000L, sendINA219DataToBlynk);
}

void loop() {
  Blynk.run();
  timer.run();
}
