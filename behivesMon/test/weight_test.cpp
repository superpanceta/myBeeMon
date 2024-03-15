/* 
Test de la bascula
*/

#include <Arduino.h>

#include <HX711.h>

// Variables
const int LOADCELL_DOUT_PIN = 12; // D6 // E- | DT
const int LOADCELL_SCK_PIN = 13;  // D7 // A- | SCK

HX711 scale; // Creamos un objeto para medir el peso


void setup() {
  Serial.begin(115200); // Inicializamos el serial monitor
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN); // Inicializamos el scale
}


void loop(){
    if (scale.is_ready()) {
    scale.set_scale();    
    Serial.println("Tare... remove any weights from the scale.");
    delay(5000);
    scale.tare();
    Serial.println("Tare done...");
    Serial.print("Place a known weight on the scale...");
    delay(5000);
    long reading = scale.get_units(10);
    Serial.print("Result: ");
    Serial.println(reading);
  } 
  else {
    Serial.println("HX711 not found.");
  }
  delay(1000);
}

//calibration factor will be the (reading)/(known weight)