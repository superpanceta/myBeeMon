/*
Autor : Angel Manuel Garcia Martin
gitHub: superpanceta@gmail.com
Placa : ESP32-C3 SuperMini
Comentarios: 
2024-08-08: En Arduino IDE para imprimir por el serial se ha de habilitar :  Tools > /*
Autor : Angel Manuel Garcia Martin
gitHub: superpanceta@gmail.com
Placa : ESP32-C3 SuperMini
Comentarios: 
2024-08-08: En Arduino IDE para imprimir por el serial se ha de habilitar :  Tools > USB CDC On Boot > Enabled -- Solo aparece con la placa C3
2024-09-26: Testing behavior once reconnect from sleeping mode

Versiones:
2024-09-01: Version: V1   - Codigo para el sender - Sera el mismo para todas las colmenas , cambiando solamente el ID  y la calibracion de las celdas de carga
2024-09-10: Version: V1.1 - Deep Sleep añadido 
2024-09-25: Version: V1.2 - Test new Library for MPU -- https://github.com/tockn/MPU6050_tockn
*/ 

#include <Arduino.h>     // Needed for Visual Studio Code
#include <esp_sleep.h>   // Libreria para dormir el ESP32
#include <WiFi.h>        // Wifi para ESP32
#include <DHTesp.h>      // Sensor temp y humedad
//#include <DHT_U.h>
#include <esp_now.h>     // Transmision entre ESP32s
#include <HX711.h>       // Celdas de carga
#include <Wire.h>        // Giroscopio

// Global Variables
// D4 - Usado para el SCL del giroscopio        -- Giroscopio
// D5 - Usado para el SDA del giroscopio        -- Giroscopio
/* Esto era en el ESP8266
#define DHTpin 14  //D5 of NodeMCUis GPIO14     -- Sensor temperatura/humedad
#define LOADCELL_DOUT_PIN 12 // D6 // E- | DT   -- Celda de Carga
#define LOADCELL_SCK_PIN 13  // D7 // A- | SCK  -- Celda de Carga
*/
// Para el ESP32 C3 Mini
#define DHTpin 2  //GPIO2     -- Sensor temperatura/humedad
#define LOADCELL_DOUT_PIN 3 // GPIO3 // E- | DT   -- Celda de Carga
#define LOADCELL_SCK_PIN 4  // GPIO4 // A- | SCK  -- Celda de Carga


// Variables Giroscopio
const int MPU_addr=0x68;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
int minVal=265;
int maxVal=402;
double angulos[3];


// Crear instancia del DHT
DHTesp dht;

// Creamos un objeto para medir el peso
HX711 scale; 
float weightCalibration[] = {-24.03, 0, 0};  // Calibracion para las diferentes celdas de carga

// Constantes
const uint8_t broadcastAddress[] = {0x30, 0x83, 0x98, 0x7b, 0xdb, 0x90}; // MAC receiver LillyGo SIM7000 -- De momento solo tenemos un concentrador

// Variables

float humedad;
float temperatura;
float peso;
int colmena_id = 1;   // ID Colmena

// Estructura de los datos que se enviaran
typedef struct struct_message 
{
  float s_temperatura;
  float s_humedad;
  float s_peso;
  float s_tempExt;
  int s_colmena_id;
  //double s_angulos[3];
} struct_message;

// Creamos variable para los datos a enviar
struct_message myData;

esp_now_peer_info_t peerInfo;


// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail"); // Otra opcion de checkear
  if (status == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
};

// Function to messure the Temp and humidity
void getTmp_hum(){
  delay(dht.getMinimumSamplingPeriod());
  humedad = dht.getHumidity();
  temperatura = dht.getTemperature();
}

// Funcion para pesar la colmena
void getWeight(){
  scale.power_up(); // Despertamos la bascula
  peso = scale.read_average(20);
  scale.power_down();
}

// Funcion para obtener la inclinacion
void getInclination(){
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true);
  AcX=Wire.read()<<8|Wire.read();
  AcY=Wire.read()<<8|Wire.read();
  AcZ=Wire.read()<<8|Wire.read();
  int xAng = map(AcX,minVal,maxVal,-90,90);
  int yAng = map(AcY,minVal,maxVal,-90,90);
  int zAng = map(AcZ,minVal,maxVal,-90,90);
  //Obtenemos los angulos 
  angulos[0]= RAD_TO_DEG * (atan2(-yAng, -zAng)+PI);
  angulos[1]= RAD_TO_DEG * (atan2(-xAng, -zAng)+PI);
  angulos[2]= RAD_TO_DEG * (atan2(-yAng, -xAng)+PI);
}

// Funcion para enviar los datos
void sendData(){
  myData.s_colmena_id = colmena_id;
  myData.s_humedad = humedad;
  myData.s_temperatura = temperatura;
  myData.s_peso = peso;
  myData.s_angulos[0] = angulos[0];
  myData.s_angulos[1] = angulos[1];
  myData.s_angulos[2] = angulos[2];
  // Enviar datos
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
}

// Imprimir datos por el Serial Monitor a efectos de debug
void printData(){
  Serial.print("Colmena :");
  Serial.println(myData.s_colmena_id);
  Serial.print("Temperatura: ");
  Serial.println(myData.s_temperatura);
  Serial.print("Humedad");
  Serial.println(myData.s_humedad);
  Serial.print("Peso: ");
  Serial.println(myData.s_peso);
  //Serial.print("AnguloX, AnguloY, AnguloZ");
  //Serial.println(angulos);
}



void setup(){
  // Inicializamos el serial monitor
  Serial.begin(115200);
  
  // Configuramos el dispositivo como Wi-Fi station
  WiFi.mode(WIFI_STA);

  // Iniciamos ESP-NOW
  if(esp_now_init() != 0){
    Serial.println("Error iniciando ESP-NOW");
    return;
  }
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  // esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);  // Only needed in esp8266
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");
  // Initialize dht
  dht.setup(DHTpin, DHTesp::DHT11); 
  
  // Inicializamos el scale
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN); 
  Serial.println(scale.read());      // Imprimimios una medida de prueba
  scale.set_scale(weightCalibration[0]); // Añadimos el factor de calibracion 0 - Colmena1, 1 - Colmena2, 2 - Colmena3

  // Giroscopio -- Comentado para hacer los test sin el giroscopio
  //Wire.begin();
  //Wire.beginTransmission(MPU_addr);
  //Wire.write(0x6B);
  //Wire.write(0);
  //Wire.endTransmission(true);
  
  // Medir datos en Setup ya que depues dormimos el ESP
  getTmp_hum(); // Temperatura y Humedad
  getWeight();  // Peso
  //getInclination(); // Inclinacion
  printData();  // Imprimir datos
  //sendData();   // Enviar a LilyGo
  
  // Dormimos el ESP -- Modo test 5 min -- 5 secs --1000000
  //esp_deep_sleep(2000000); // 10 Seconds 2000000 -- 5 min = 2000000 * 30 
  esp_deep_sleep(12000000);  // 1 min -- 1 Second 200000  = 200000 X 60 = 

  // A partir de aqui no se ejecuta nada ya que cuando despierta comienza como arrancando de Zero

}


void loop() {
  
  
} > Enabled
2024-09-26: Testing behavior once reconnect from sleeping mode

Versiones:
2024-09-01: Version: V1   - Codigo para el sender - Sera el mismo para todas las colmenas , cambiando solamente el ID  y la calibracion de las celdas de carga
2024-09-10: Version: V1.1 - Deep Sleep añadido 
2024-09-25: Version: V1.2 - Test new Library for MPU -- https://github.com/tockn/MPU6050_tockn
*/ 

#include <Arduino.h>     // Needed for Visual Studio Code
#include <esp_sleep.h>   // Libreria para dormir el ESP32
#include <WiFi.h>        // Wifi para ESP32
#include <DHTesp.h>      // Sensor temp y humedad
//#include <DHT_U.h>
#include <esp_now.h>     // Transmision entre ESP32s
#include <HX711.h>       // Celdas de carga
#include <Wire.h>        // Giroscopio

// Global Variables
// D4 - Usado para el SCL del giroscopio        -- Giroscopio
// D5 - Usado para el SDA del giroscopio        -- Giroscopio
/* Esto era en el ESP8266
#define DHTpin 14  //D5 of NodeMCUis GPIO14     -- Sensor temperatura/humedad
#define LOADCELL_DOUT_PIN 12 // D6 // E- | DT   -- Celda de Carga
#define LOADCELL_SCK_PIN 13  // D7 // A- | SCK  -- Celda de Carga
*/
// Para el ESP32 C3 Mini
#define DHTpin 2  //GPIO2     -- Sensor temperatura/humedad
#define LOADCELL_DOUT_PIN 3 // GPIO3 // E- | DT   -- Celda de Carga
#define LOADCELL_SCK_PIN 4  // GPIO4 // A- | SCK  -- Celda de Carga


// Variables Giroscopio
const int MPU_addr=0x68;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
int minVal=265;
int maxVal=402;
double angulos[3];


// Crear instancia del DHT
DHTesp dht;

// Creamos un objeto para medir el peso
HX711 scale; 
float weightCalibration[] = {-24.03, 0, 0};  // Calibracion para las diferentes celdas de carga

// Constantes
const uint8_t broadcastAddress[] = {0x30, 0x83, 0x98, 0x7b, 0xdb, 0x90}; // MAC receiver LillyGo SIM7000 -- De momento solo tenemos un concentrador

// Variables

float humedad;
float temperatura;
float peso;
int colmena_id = 1;   // ID Colmena

// Estructura de los datos que se enviaran
typedef struct struct_message 
{
  float s_temperatura;
  float s_humedad;
  float s_peso;
  float s_tempExt;
  int s_colmena_id;
  double s_angulos[3];
} struct_message;

// Creamos variable para los datos a enviar
struct_message myData;

esp_now_peer_info_t peerInfo;


// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail"); // Otra opcion de checkear
  if (status == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
};

// Function to messure the Temp and humidity
void getTmp_hum(){
  delay(dht.getMinimumSamplingPeriod());
  humedad = dht.getHumidity();
  temperatura = dht.getTemperature();
}

// Funcion para pesar la colmena
void getWeight(){
  scale.power_up(); // Despertamos la bascula
  peso = scale.read_average(20);
  scale.power_down();
}

// Funcion para obtener la inclinacion
void getInclination(){
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true);
  AcX=Wire.read()<<8|Wire.read();
  AcY=Wire.read()<<8|Wire.read();
  AcZ=Wire.read()<<8|Wire.read();
  int xAng = map(AcX,minVal,maxVal,-90,90);
  int yAng = map(AcY,minVal,maxVal,-90,90);
  int zAng = map(AcZ,minVal,maxVal,-90,90);
  //Obtenemos los angulos 
  angulos[0]= RAD_TO_DEG * (atan2(-yAng, -zAng)+PI);
  angulos[1]= RAD_TO_DEG * (atan2(-xAng, -zAng)+PI);
  angulos[2]= RAD_TO_DEG * (atan2(-yAng, -xAng)+PI);
}

// Funcion para enviar los datos
void sendData(){
  myData.s_colmena_id = colmena_id;
  myData.s_humedad = humedad;
  myData.s_temperatura = temperatura;
  myData.s_peso = peso;
  myData.s_angulos[0] = angulos[0];
  myData.s_angulos[1] = angulos[1];
  myData.s_angulos[2] = angulos[2];
  // Enviar datos
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
}

// Imprimir datos por el Serial Monitor a efectos de debug
void printData(){
  Serial.print("Colmena :");
  Serial.println(myData.s_colmena_id);
  Serial.print("Temperatura: ");
  Serial.println(myData.s_temperatura);
  Serial.print("Humedad");
  Serial.println(myData.s_humedad);
  Serial.print("Peso: ");
  Serial.println(myData.s_peso);
  //Serial.print("AnguloX, AnguloY, AnguloZ");
  //Serial.println(angulos);
}



void setup(){
  // Inicializamos el serial monitor
  Serial.begin(115200);
  
  // Configuramos el dispositivo como Wi-Fi station
  WiFi.mode(WIFI_STA);

  // Iniciamos ESP-NOW
  if(esp_now_init() != 0){
    Serial.println("Error iniciando ESP-NOW");
    return;
  }
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  // esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);  // Only needed in esp8266
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");
  // Initialize dht
  dht.setup(DHTpin, DHTesp::DHT11); 
  
  // Inicializamos el scale
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN); 
  Serial.println(scale.read());      // Imprimimios una medida de prueba
  scale.set_scale(weightCalibration[0]); // Añadimos el factor de calibracion 0 - Colmena1, 1 - Colmena2, 2 - Colmena3

  // Giroscopio -- Comentado para hacer los test sin el giroscopio
  //Wire.begin();
  //Wire.beginTransmission(MPU_addr);
  //Wire.write(0x6B);
  //Wire.write(0);
  //Wire.endTransmission(true);
  
  // Medir datos en Setup ya que depues dormimos el ESP
  getTmp_hum(); // Temperatura y Humedad
  getWeight();  // Peso
  //getInclination(); // Inclinacion
  printData();  // Imprimir datos
  //sendData();   // Enviar a LilyGo
  
  // Dormimos el ESP -- Modo test 5 min -- 5 secs --1000000
  //esp_deep_sleep(2000000); // 10 Seconds 2000000 -- 5 min = 2000000 * 30 
  esp_deep_sleep(12000000);  // 1 min -- 1 Second 200000  = 200000 X 60 = 

  // A partir de aqui no se ejecuta nada ya que cuando despierta comienza como arrancando de Zero

}


void loop() {
  
  
}