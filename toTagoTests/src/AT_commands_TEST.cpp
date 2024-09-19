/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/lilygo-t-sim7000g-esp32-lte-gprs-gps/
  
  Adaptando para envio de mensajes MQTT
*/

// Original code: https://github.com/Xinyuan-LilyGO/LilyGO-T-SIM7000G/blob/master/examples/Arduino_NetworkTest/Arduino_NetworkTest.ino

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// set GSM PIN, if any
#define GSM_PIN "6871"

// Your GPRS credentials, if any
const char apn[]  = "internet";     //SET TO YOUR APN
const char gprsUser[] = "";
const char gprsPass[] = "";

// MQTT details
const char* tagoDeviceName = "";
const char* tagoBroker = "mqtt.tago.io";
unsigned int tagoPort = 8883;
const char* tagoUser = "beeMon";
//const char* tagoToken = "5d44ef90-8e03-4c32-8012-b770c72b0787";
const char* tagoToken = "c972f429-2737-4ed1-841b-4a04881a09ad";

const char* topicWheight = "GsmClientTest/led";
const char* topicInTemp = "GsmClientTest/init";
const char* topicOuTemp = "GsmClientTest/ledStatus";
const char* topicInHum = "GsmClientTest/ledStatus";

#include <TinyGsmClient.h>
#include <PubsubClient.h> 
#include <ArduinoJson.h>

// #include <SPI.h> // No usamos la SD por el momento
// #include <SD.h>  // No usamos la SD por el momento
#include <Ticker.h> // No parece que la usemos, es para temporizar 

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

TinyGsmClient client(modem);
PubSubClient  mqtt(client);

// LilyGO T-SIM7000G Pinout
#define UART_BAUD           115200
#define PIN_DTR             25
#define PIN_TX              27
#define PIN_RX              26
#define PWR_PIN             4

#define SD_MISO             2
#define SD_MOSI             15
#define SD_SCLK             14
#define SD_CS               13
#define LED_PIN             12

// Encender modem
void modemPowerOn(){
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(1000);
  digitalWrite(PWR_PIN, HIGH);
}

// Apagar modem
void modemPowerOff(){
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(1500);
  digitalWrite(PWR_PIN, HIGH);
}

// Reiniciar modem
void modemRestart(){
  modemPowerOff();
  delay(1000);
  modemPowerOn();
}

// Conecta cliente MQTT
boolean mqttConnect() {
  if(!mqtt.connected()){
    Serial.println();
    Serial.println("Mqtt Client : [Not Connected]");
    Serial.println("Mqtt Client : [Connecting]");
    mqtt.setServer(tagoBroker, tagoPort);
    mqtt.setBufferSize(1024);

    if (mqtt.connect(tagoDeviceName, tagoUser, tagoToken)){
      Serial.println("Mqtt Client : [Broker Connected]");
      // Seguir aqui
      // GEstionar tema topics
    }
  }
}

void setup(){
  // Set console baud rate
  SerialMon.begin(115200);

  delay(10);

  // Set LED OFF
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  modemPowerOn();  // Encendemos el modem

  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);


  Serial.println("/**********************************************************/");
  Serial.println("To initialize the network test, please make sure your LTE ");
  Serial.println("antenna has been connected to the SIM interface on the board.");
  Serial.println("/**********************************************************/\n\n");

  // MQTT 
  mqtt.setServer(tagoBroker, 1883); // Need to modify for including token for TaGo

  delay(10000);
}

void loop(){
  String res;

  Serial.println("========INIT========");

  if (!modem.init()) {
    modemRestart();
    delay(2000);
    Serial.println("Failed to restart modem, attempting to continue without restarting");
  }

  Serial.println("========SIMCOMATI======");
  modem.sendAT("+SIMCOMATI");
  modem.waitResponse(1000L, res);
  res.replace(GSM_NL "OK" GSM_NL, "");
  Serial.println(res);
  res = "";
  Serial.println("=======================");

  Serial.println("=====Preferred mode selection=====");
  modem.sendAT("+CNMP?");
  if (modem.waitResponse(1000L, res) == 1) {
    res.replace(GSM_NL "OK" GSM_NL, "");
    Serial.println(res);
  }
  res = "";
  Serial.println("=======================");


  Serial.println("=====Preferred selection between CAT-M and NB-IoT=====");
  modem.sendAT("+CMNB?");
  if (modem.waitResponse(1000L, res) == 1) {
    res.replace(GSM_NL "OK" GSM_NL, "");
    Serial.println(res);
  }
  res = "";
  Serial.println("=======================");


  String name = modem.getModemName();
  Serial.println("Modem Name: " + name);

  String modemInfo = modem.getModemInfo();
  Serial.println("Modem Info: " + modemInfo);

  // Unlock your SIM card with a PIN if needed
  if ( GSM_PIN && modem.getSimStatus() != 3 ) {
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
  digitalWrite(LED_PIN, HIGH);

  Serial.println();
  Serial.println("Device is connected .");
  Serial.println();

  Serial.println("=====Inquiring UE system information=====");
  modem.sendAT("+CPSI?");
  if (modem.waitResponse(1000L, res) == 1) {
    res.replace(GSM_NL "OK" GSM_NL, "");
    Serial.println(res);
  }

  Serial.println("/**********************************************************/");
  Serial.println("After the network test is complete, please enter the  ");
  Serial.println("AT command in the serial terminal.");
  Serial.println("/**********************************************************/\n\n");

  while (1) {
    while (SerialAT.available()) {
      SerialMon.write(SerialAT.read());
    }
    while (SerialMon.available()) {
      SerialAT.write(SerialMon.read());
    }
  }
}