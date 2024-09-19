// MQTT Test Running currently
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

// Funcion para publicar
void mqttPublish(int temp, int behiveNum){
    String topic2use = "temperatura_"+ String(behiveNum)+ " : "; 
    String payload = "{";
    payload += topic2use;
    payload += temp;
    payload += "}";
    
    mqtt.publish("corzoBee/data/temp", payload.c_str());
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