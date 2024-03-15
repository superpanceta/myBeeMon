// Receiver code to be installed in Lillygo GSM7000E
// Librerias
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>


// Estructura de los datos que se enviaran
typedef struct struct_message 
{
  float temperatura;
  float humedad;
  float peso;
  String colmena_id;
} struct_message;

// Creamos variable para los datos recibidos
struct_message myData;

// Funcion a ejecutar cuando se reciban los datos de la colmena
// Enviaremos a TAO en un principio 
// test purpose now
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len)
{
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes recibidos: ");
  Serial.println(len);
  Serial.print("Informacion Colmena Nº :");
  Serial.println(myData.colmena_id);
  Serial.print("Temperatura: ");
  Serial.println(myData.temperatura);
  Serial.print("Humedad: ");
  Serial.println(myData.humedad);
  Serial.print("Peso: ");
  Serial.println(myData.peso);
  Serial.println();
}

void setup(){
  // Iniciar Serial Monitor
  Serial.begin(115200);

  // Configurar device como Wifi Station
  WiFi.mode(WIFI_STA);

  // Inicializar ESP-NOW
  if(esp_now_init() != ESP_OK){
    Serial.println("Error iniciando ESP-NOW");
    return;
  }

  // Una vez inicicado obtener informacion enviada
  esp_now_register_recv_cb(OnDataRecv);
  
}
 
void loop(){

}
