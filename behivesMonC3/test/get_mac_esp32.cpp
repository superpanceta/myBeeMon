#include <Arduino.h>

// In Arduino IDE set Tools -> USB CDC On Boot - Enabled to use Serial print
// Sketch for getting the esp32 MAC
// beeMonC3-1:  84:fc:e6:00:5e:ec
//

#include "WiFi.h"
 
void setup(){
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);
  Serial.println(WiFi.macAddress());
}
 
void loop(){

}