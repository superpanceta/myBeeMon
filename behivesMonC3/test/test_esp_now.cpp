// Sender

#include <WiFi.h>  
#include <esp_now.h>     // Transmision entre ESP32s



uint8_t broadcastAddress[] = {0x30, 0x83, 0x98, 0x7b, 0xdb, 0x90}; // MAC receiver LillyGo SIM7000

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  int numtoSend;
} struct_message;

// Create a struct_message called myData
struct_message myData;

esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}



void setup() {
  // Iniciamos el Serial Monitor
  Serial.begin(115200);

  // Poner el ESP como Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Iniciamos esp-now
  if (esp_now_init() != ESP_OK){
    Serial.println("Error iniciando ESP-NOW");
    return;
  } 

  // Registrar para saber el estatus del envio 
  esp_now_register_send_cb(OnDataSent);

  // Resgistrarnos como sender donde enviaremos
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Añadimos el peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

}

void loop() {
  // Generamos el dato a enviar
  myData.numtoSend = random(0, 20);

  // Enviamos los datos
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  
  // Checkeamos si se ha enviado
  if (result = ESP_OK){
    Serial.println("Enviado OK");
  }
  else {
    Serial.println("Enviado NOK");
  }
  delay(5000);
}
