/*
Autor : Angel Manuel Garcia Martin
gitHub: superpanceta@gmail.com
Placa : LilyGo SIM700 E
Comentarios: En Arduino IDE para imprimir por el serial se ha de habilitar :  Tools > USB CDC On Boot > Enabled
2024-09-01: Version: V1   - Codigo para el concentrador - Modo test envio a CrystalMQ
2024-09-11: Version: V1.1 - Añadidas callback para ESP-NOW y struct a usar igual que la usada en el sender
2024-09-11: Version: V1.2 - Creada funcion para enviar datos a mosquito / funcion mqttPublish usada en esp-now callback
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
const char *broker = "freemqttbroker.sfodo.crystalmq.com";
const int   mqttPort = 1883;
const char *device_token = "HDdINAkvt3W22JIPZL";
const char *mqttUser = "lNdB4opOHuTXRKZtA2";
#define ID_MQTT "0005" // id mqtt (para identificar sesión)
const char *mqttDeviceNAme = "testSIM";



#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <Ticker.h>
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
  int s_colmena_id;
  double s_angulos[3];
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
    if (mqtt.connect(ID_MQTT, mqttUser, device_token)) {
      SerialMon.println(" connected");
    } else {
      SerialMon.print(" failed, rc=");
      SerialMon.println(mqtt.state());
      delay(5000);
    }
  }
}

/*
// Funcion para publicar
void mqttPublish(int temp, int behiveNum){
    String topic2use = "temperatura_"+ String(behiveNum)+ " : "; 
    String payload = "{";
    payload += topic2use;
    payload += temp;
    payload += "}";
    
    mqtt.publish("corzoBee/data/temp", payload.c_str());
}
*/


// NEW Funcion para publicar, usaremos la estructura que nos llega de ESP-NOW
void mqttPublish(const struct_message &data){
    const char* medidas[] = {"temperatura", "humedad", "peso"};
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
  Serial.println(myData.s_angulos);
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
    Wifi.mode(WIFI_STA);                          // Seteamos el Wi-Fi como Station
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
    int tempRand = random(0, 50);
    int hiveNum = random(1, 3);
    mqttPublish(tempRand, hiveNum);
    delay(10000); 

}