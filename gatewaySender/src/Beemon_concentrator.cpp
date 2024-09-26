/*
Autor : Angel Manuel Garcia Martin
gitHub: superpanceta@gmail.com
Placa : LilyGo SIM700 E
Comentarios: En Arduino IDE para imprimir por el serial se ha de habilitar :  Tools > USB CDC On Boot > Enabled
2024-09-01: Version: V1   - Codigo para el concentrador - Modo test envio a CrystalMQ
2024-09-11: Version: V1.1 - Añadidas callback para ESP-NOW y struct a usar igual que la usada en el sender
2024-09-11: Version: V1.2 - Creada funcion para enviar datos a mosquito / funcion mqttPublish usada en esp-now callback
2024-09-19: Version: V2.0 - Migrar el envio del mqtt de cristalMq a la Raspberry
                          - Cambio topics a solamente /temperatura
                                                      /humedad
                                                      /peso
                                                      /tempExt
2024-09-23: Version: 2.1  - Funcion envio a rasp funcionando en modo tets, llegan todos los datos probados, temperatura, humedad, peso
2024-09-23: Version: 2.2  - Probar integracion con monitores
*/ 

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial


// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]      = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";

// MQTT details
const char *broker = "mynoipmq.ddns.net";
const int   mqttPort = 1883;
const char *mqttPass = "aporellos";
const char *mqttUser = "pance";
#define ID_MQTT "0005" // id mqtt (para identificar sesión)
const char *mqttDeviceNAme = "testSIM";



#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <WiFi.h>
#include <esp_now.h>

// Just in case someone defined the wrong thing..
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false


#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger);
#else
TinyGsm        modem(SerialAT);
#endif

// Creamos los objetos para conectarnos via GPRS
TinyGsmClient client(modem);
PubSubClient  mqtt(client);

Ticker tick;

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  60          // Time ESP32 will go to sleep (in seconds)

#define UART_BAUD   115200
#define PIN_DTR     25
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4

#define SD_MISO     2  // To be deleted
#define SD_MOSI     15 // To be deleted
#define SD_SCLK     14 // To be deleted
#define SD_CS       13 // To be deleted
#define LED_PIN     12 // To be deleted

uint32_t lastReconnectAttempt = 0;

// Estructura para recobir los datos de la colmena, debe coincidir con la de los sender 
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

void modemPowerOn(){
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(1000);
  digitalWrite(PWR_PIN, HIGH);
}

void modemPowerOff(){
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(1500);
  digitalWrite(PWR_PIN, HIGH);
}


void modemRestart(){
  modemPowerOff();
  delay(1000);
  modemPowerOn();
}

//boolean mqttConnect()
void mqttConnect() {
  SerialMon.print("Connecting to TagoIO...");
  while (!mqtt.connected()) {
    if (mqtt.connect(ID_MQTT, mqttUser, mqttPass)) {
      SerialMon.println(" connected");
    } else {
      SerialMon.print(" failed, rc=");
      SerialMon.println(mqtt.state());
      delay(5000);
    }
  }
}


// Last Funcion para publicar
// Estructura a mandar que es con la que esta funcionando actualmente
// A probar enviando via wokWiki
// "{ \"beehive\": \"beehive1\", \"valor\": \"35%\" }"
void mqttPublish(const struct_message &data){
    const char* medidas[] = {"temperatura", "humedad", "peso", "tempExt"};
    size_t length = sizeof(medidas) / sizeof(medidas[0]); 
    for (size_t i = 0; i < length; i++) {
      char topic[100]="";
      char beehive[100]="";
      char message[100]="";
      char buffer[50];
      char valor[50];
      // Creamos el topic que sera el timpo de medida
      strcat(topic, medidas[i]); 
      // Convertimos el id de la colmena a string
      sprintf(buffer, "%d", data.s_colmena_id);
      // Creamos el beehive number con beehive + s_colmena_id
      strcat(beehive, "beehive");
      strcat(beehive, buffer);
      switch (i)
      {
        case 0:
          Serial.println(topic);
          /*
          Serial.println(beehive);
          Serial.println(s_temperatura);*/
          // Pasamos temp de float a str
          sprintf(valor, "%.2f", data.s_temperatura);
          //Creamos el message a enviar
          strcat(message, "{ \"beehive\": \"");
          strcat(message, beehive);
          strcat(message, "\", \"valor\": \"");
          strcat(message, valor);
          strcat(message, "\" }");
          Serial.println(message);
          //mqtt.publish(topic, message);
          break;
        case 1:
          Serial.println(topic);
          /*
          Serial.println(beehive);
          Serial.println(s_humedad);*/
          // Pasamos temp de float a str
          sprintf(valor, "%.2f", data.s_humedad);
          //Creamos el message a enviar
          strcat(message, "{ \"beehive\": \"");
          strcat(message, beehive);
          strcat(message, "\", \"valor\": \"");
          strcat(message, valor);
          strcat(message, "\" }");
          Serial.println(message);
          //mqtt.publish(topic, message);
          break;
        case 2:
          Serial.println(topic);
          /*
          Serial.println(beehive);
          Serial.println(s_peso);*/
          // Pasamos temp de float a str
          sprintf(valor, "%.2f", data.s_peso);
          //Creamos el message a enviar
          strcat(message, "{ \"beehive\": \"");
          strcat(message, beehive);
          strcat(message, "\", \"valor\": \"");
          strcat(message, valor);
          strcat(message, "\" }");
          Serial.println(message);
          //mqtt.publish(topic, message);
          break;
        case 3:
          Serial.println(topic);
          /*
          Serial.println(beehive);
          Serial.println(s_tempExt); */
          // Pasamos temp de float a str
          sprintf(valor, "%.2f", data.s_tempExt);
          //Creamos el message a enviar
          strcat(message, "{ \"beehive\": \"");
          strcat(message, beehive);
          strcat(message, "\", \"valor\": \"");
          strcat(message, valor);
          strcat(message, "\" }");
          Serial.println(message);
          //mqtt.publish(topic, message);
          break;
        default:
          break;  
      }
}
}


// FUNCION PARA TESTEAR GRAFANA
// Last Funcion para publicar
// Estructura a mandar que es con la que esta funcionando actualmente
// A probar enviando via wokWiki
// "{ \"beehive\": \"beehive1\", \"valor\": \"35%\" }"
void mqttPublish_test(float s_temperatura, float s_humedad, float s_peso, float s_tempExt, int s_colmena_id){
    const char* medidas[] = {"temperatura", "humedad", "peso", "tempExt"};
    size_t length = sizeof(medidas) / sizeof(medidas[0]); 
    for (size_t i = 0; i < length; i++) {
      char topic[100]="";
      char beehive[100]="";
      char message[100]="";
      char buffer[50];
      char valor[50];
      // Creamos el topic que sera el timpo de medida
      strcat(topic, medidas[i]); 
      // Convertimos el id de la colmena a string
      sprintf(buffer, "%d", s_colmena_id);
      // Creamos el beehive number con beehive + s_colmena_id
      strcat(beehive, "beehive");
      strcat(beehive, buffer);
      switch (i)
      {
        case 0:
          Serial.println(topic);
          /*
          Serial.println(beehive);
          Serial.println(s_temperatura);*/
          // Pasamos temp de float a str
          sprintf(valor, "%.2f", s_temperatura);
          //Creamos el message a enviar
          strcat(message, "{ \"beehive\": \"");
          strcat(message, beehive);
          strcat(message, "\", \"valor\": \"");
          strcat(message, valor);
          strcat(message, "\" }");
          Serial.println(message);
          mqtt.publish(topic, message);
          break;
        case 1:
          Serial.println(topic);
          /*
          Serial.println(beehive);
          Serial.println(s_humedad);*/
          // Pasamos temp de float a str
          sprintf(valor, "%.2f", s_humedad);
          //Creamos el message a enviar
          strcat(message, "{ \"beehive\": \"");
          strcat(message, beehive);
          strcat(message, "\", \"valor\": \"");
          strcat(message, valor);
          strcat(message, "\" }");
          Serial.println(message);
          mqtt.publish(topic, message);
          break;
        case 2:
          Serial.println(topic);
          // Pasamos temp de float a str
          sprintf(valor, "%.2f", s_peso);
          //Creamos el message a enviar
          strcat(message, "{ \"beehive\": \"");
          strcat(message, beehive);
          strcat(message, "\", \"valor\": \"");
          strcat(message, valor);
          strcat(message, "\" }");
          Serial.println(message);
          mqtt.publish(topic, message);
          break;
        case 3:
          Serial.println(topic);
          // Pasamos temp de float a str
          sprintf(valor, "%.2f", s_tempExt);
          //Creamos el message a enviar
          strcat(message, "{ \"beehive\": \"");
          strcat(message, beehive);
          strcat(message, "\", \"valor\": \"");
          strcat(message, valor);
          strcat(message, "\" }");
          Serial.println(message);
          mqtt.publish(topic, message);
          break;
        default:
          break;  
      }
}
}



/*
// Funcion antigua para el envio de los daros a mosquito
// Estructura a mandar
// "{ \"beehive\": \"beehive1\", \"valor\": \"35%\" }"
void mqttPublish(const struct_message &data){
    const char* medidas[] = {"temperatura", "humedad", "peso", "tempExt"};
    size_t length = sizeof(medidas) / sizeof(medidas[0]);      // Calculamos el tamaño del array
    // Recorremos el array para ir generando el topic y mandando los datos
    for (size_t i = 0; i < length; i++) {
      char topic[100]="";
      char buffer[50];
      // Convertimos el id de la colmena a string
      sprintf(buffer, "%d", data.s_colmena_id);
      // Creamos el topic que corresponda concatenando el id de la colmena 
      strcat(topic, medidas[i]);
      strcat(topic, "/");
      strcat(topic, buffer);
      // Enviamos los datos, dependiendo del dato a enviar      
      switch (i)
      {
      case 0:
        mqtt.publish(topic, data.s_temperatura);
        break;
      case 1:
        mqtt.publish(topic, data.s_humedad);
        break;
      case 2:
        mqtt.publish(topic, data.s_peso;
        break;      
      default:
        break;
      }
      topic[0] = '\0';
    }
}
*/



// Funcion para Checkear GPRS y reconectar
void checkNetConnecttion(){
    if (!modem.isNetworkConnected()) {
        SerialMon.println("Network connection failed. Reconnecting...");
        modem.gprsConnect(apn, gprsUser, gprsPass);
        }
    if (modem.isGprsConnected()) {
        SerialMon.println("GPRS connected successfully.");
        } 
    else {
        SerialMon.println("GPRS connection failed.");
        }
}

// Callback que se ejecuta cuando llega algun mensaje via esp-now procedente de los sender
// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Colmena Nº: ");
  Serial.println(myData.s_colmena_id);
  Serial.print("Temperatura: ");
  Serial.println(myData.s_temperatura);
  Serial.print("Humedad: ");
  Serial.println(myData.s_humedad);
  Serial.print("Peso: ");
  Serial.println(myData.s_peso);
  Serial.print("Angulos: ");
  //Serial.println(myData.s_angulos);
  Serial.println();
  // Envio a MQTT
  mqttPublish(myData);

}

void setup()
{
    // Set console baud rate
    Serial.begin(115200);
    delay(10);

    // Set LED OFF
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    // Start the modem
    SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
    delay(3000);

    SerialMon.println("Iniciando el modem...");
    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    // Estamos usando stop/start en su lugar
    modemRestart();

    // Pintamos los datos del modem 
    // Poca utilidad una vez que funcione
    // A borrar en un futuro
    String name = modem.getModemName();
    DBG("Modem Name:", name);

    String modemInfo = modem.getModemInfo();
    DBG("Modem Info:", modemInfo);

    // Unlock your SIM card with a PIN if needed
    if (GSM_PIN && modem.getSimStatus() != 3) {
        modem.simUnlock(GSM_PIN);
    }
   
    for (int i = 0; i <= 4; i++) {
    uint8_t network[] = {
        2,  /*Automatic*/
        13, /*GSM only*/
        38, /*LTE only*/
        51  /*GSM and LTE only*/
    };
    Serial.printf("Try %d method\n", network[i]);
    modem.setNetworkMode(network[i]);
    delay(3000);
    bool isConnected = false;
    int tryCount = 60;
    while (tryCount--) {
      int16_t signal =  modem.getSignalQuality();
      Serial.print("Signal: ");
      Serial.print(signal);
      Serial.print(" ");
      Serial.print("isNetworkConnected: ");
      isConnected = modem.isNetworkConnected();
      Serial.println( isConnected ? "CONNECT" : "NO CONNECT");
      if (isConnected) {
        break;
      }
      delay(1000);
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
    if (isConnected) {
        break;
    }
  }
    delay(5000);  // Wait for 5 seconds to ensure a stable GPRS connection

    // Connect the modem
    #if TINY_GSM_USE_GPRS
    // GPRS connection parameters are usually set after network registration
    SerialMon.print(F("Connecting to "));
    SerialMon.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        SerialMon.println("GPRS Connection failed");
        delay(10000);
        return;
    }
    SerialMon.println(" success");

    if (modem.isGprsConnected()) {
        SerialMon.println("GPRS connected");
    }
    #endif

    // MQTT Broker setup
    mqtt.setServer(broker, mqttPort);

    // Check connection
    checkNetConnecttion();

    // Conectar a TagoIO
    mqttConnect();

    // ESP-NOW
    WiFi.mode(WIFI_STA);                          // Seteamos el Wi-Fi como Station
    if (esp_now_init() != ESP_OK){                // Iniciamos ESP-NOW
      SerialMon.println("Error niciando ESP-NOW");
      return;
    }
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));  // Una vez conectado registramos el callback para ecobir los datos

}

void loop()
{    // Make sure we're still registered on the network
    if (!modem.isNetworkConnected()) {
        SerialMon.println("Network disconnected");
        int16_t signal =  modem.getSignalQuality();
        SerialMon.print("Señal : ");
        SerialMon.println(signal);
        if (!modem.waitForNetwork(180000L, true)) {
            SerialMon.println(" fail");
            delay(10000);
            return;
        }
        if (modem.isNetworkConnected()) {
            SerialMon.println("Network re-connected");
        }

#if TINY_GSM_USE_GPRS
        // and make sure GPRS/EPS is still connected
        if (!modem.isGprsConnected()) {
            SerialMon.println("GPRS disconnected!");
            SerialMon.print(F("Connecting to "));
            SerialMon.print(apn);
            if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
                SerialMon.println(" fail");
                delay(10000);
                return;
            }
            if (modem.isGprsConnected()) {
                SerialMon.println("GPRS reconnected");
            }
        }
#endif
    }

   if (!mqtt.connected()) {
    mqttConnect();
  }

    // mqtt.loop(); // No es necesario si solo queremos publicar
    float tempRand = random(31, 45);
    int hiveNum = random(1, 8);
    float pesoRand = random(10, 50);
    float humRand = random(40, 100);
    float tempExtRand = random(0,31);
    // mqttPublish(tempRand, hiveNum);
    // mqttPublish_test(float s_temperatura, float s_humedad, float s_peso, float s_tempExt, int s_colmena_id)
    mqttPublish_test(tempRand, humRand, pesoRand, tempExtRand, hiveNum);
    delay(10000); 

}