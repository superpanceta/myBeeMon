#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// set GSM PIN, if any
#define GSM_PIN "6871"


#include <TinyGsmClient.h>
#include <PubSubClient.h>

// Define the Serial connections
#define SerialMon Serial
#define MODEM_RST            5
#define MODEM_PWRKEY         4
#define MODEM_POWER_ON       23
#define MODEM_TX             27  // SIM7000 TX
#define MODEM_RX             26  // SIM7000 RX

// Your APN details
const char apn[] = "internet"; // Set your APN
const char gprsUser[] = "";    // GPRS User (empty if not needed)
const char gprsPass[] = "";    // GPRS Password (empty if not needed)
const char simPin[] = "6871";  // Your SIM card PIN

// TagoIO MQTT settings
const char* broker = "mqtt.tago.io";
const char* device_token = "c972f429-2737-4ed1-841b-4a04881a09ad";
const int   mqttPort = 1883;

// Initialize the GSM modem and client
TinyGsm modem(SerialAT);
TinyGsmClient gsmClient(modem);
PubSubClient mqttClient(gsmClient);

void setup() {
  // Set up the Serial monitors
  SerialMon.begin(115200);
  delay(10);
  
  // Start the modem Serial connection
  SerialAT.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);  

  // Initialize the modem
  SerialMon.println("Initializing modem...");
  modem.restart();
  
  // Unlock SIM with PIN
  if (simPin && modem.getSimStatus() != 3) {  // 3 indicates SIM is already unlocked
    SerialMon.print("Unlocking SIM...");
    if (modem.simUnlock(simPin)) {
      SerialMon.println(" success");
    } else {
      SerialMon.println(" fail");
      while (true);  // Stop if unable to unlock
    }
  }

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem: ");
  SerialMon.println(modemInfo);

  // Connect to network
  SerialMon.print("Connecting to network...");
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    while (true);
  }
  SerialMon.println(" success");

  // Configure the MQTT client
  mqttClient.setServer(broker, mqttPort);

  // Connect to TagoIO
  connectToMqtt();
}

void connectToMqtt() {
  SerialMon.print("Connecting to TagoIO...");
  while (!mqttClient.connected()) {
    if (mqttClient.connect("ESP32Client", device_token, "")) {
      SerialMon.println(" connected");
    } else {
      SerialMon.print(" failed, rc=");
      SerialMon.print(mqttClient.state());
      delay(5000);
    }
  }
}

void sendDataToTagoIO() {
  // Create a JSON payload
  String payload = "{\"variable\":\"temperature\",\"value\":23.5}";

  // Publish the payload
  if (mqttClient.publish("tago/data/post", payload.c_str())) {
    SerialMon.println("Data sent to TagoIO successfully");
  } else {
    SerialMon.println("Failed to send data");
  }
}

void loop() {
  if (!mqttClient.connected()) {
    connectToMqtt();
  }
  mqttClient.loop();

  // Send data to TagoIO every 10 seconds
  sendDataToTagoIO();
  delay(10000);
}
