/*
  Configurar el cliente del navegador HiveMQ para visualizar los
  mensajes MQTT publicados por el cliente MQTT.

  1) Abrir la URL descripta y cliquear el boton conectar
     http://www.hivemq.com/demos/websocket-client/

  2) Adicione los tópicos que el ESP32 usa:
     topic_on_off_led/#
     topic_sensor_temperature/#
     topic_sensor_humidity/#

  3) Publicar en el topico topic_on_off_led con un mensaje 1 ó 0
     para prender o apagar el LED"

  IMPORTANTE: es necesario ejecutar Wokwi Gateway y habilitar la opción
  "Enable Private Wokwi IoT Gateway" a través de la tecla de atajo F1 en el editor de código.

  Para descargar Wokwi IoT Network Gateway aceda al siguiente link:
    https://github.com/wokwi/wokwigw/releases/
*/

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// set GSM PIN, if any
#define GSM_PIN "6871"

// Librerias
#include <PubSubClient.h>
#include <ArduinoJson.h> 



#define PUBLISH_DELAY 2000   // Atraso de publicación (2 segundos)

#define ID_MQTT "650f53cc4d9a6300095323d4" // id mqtt (para identificar sesión) TO BE DEFINED by SENDER

// Para testear usamos Wokwi 
const char *SSID = "Wokwi-GUEST"; // SSID / nombre de red WI-FI que desea conectar
const char *PASSWORD = "";        // Pass de red WI-FI que desea conectar


// URL del broker MQTT que se desea utilizar
const char* tagoBroker = "mqtt.tago.io";
const char* tagoUser = "beeMon";
const char* tagoToken = "fcd6bd40-bf3e-4e00-be86-f0189891fb20";
const char* tagoDeviceNAme = "CorzoBee";
int tagoPort = 1883; // Puerto del Broker MQTT

unsigned long publishUpdate;

// Variables y objetos globales
WiFiClient espClient;         // Crea el objeto espClient
PubSubClient mqtt(espClient); // Instancia el Cliente MQTT pasando el objeto espClient

/* Prototipos */


void initWiFi(void);
void initMQTT(void);
void callbackMQTT(char *topic, byte *payload, unsigned int length);
void reconnectMQTT(void);
void reconnectWiFi(void);
void checkWiFIAndMQTT(void);

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message
{
    char* variable[32];
    float value;
    char* topico;
} struct_message;
// Creamos las estructura
struct_message dataIn;

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&dataIn, incomingData, sizeof(dataIn));
  SendDataToTago(dataIn.variable, dataIn.value, dataIn.topico);
}

/* Inicializa y conecta la red WI-FI deseada */
void initWiFi(void)
{
  delay(10);
  Serial.println("------Conexión WI-FI------");
  Serial.print("Conectandose a la red: ");
  Serial.println(SSID);
  Serial.println("Aguarde");

  reconnectWiFi();
}

/* Inicializa los parámetros de conexión MQTT */
void initMQTT(void)
{
  mqtt.setServer(tagoBroker, tagoPort); // Informa cual broker y puerto debe ser conectado
  mqtt.setBufferSize(1024);
  mqtt.setCallback(callbackMQTT);           
}

/* Función de callback,  es llamada cada vez que una información
   de los tópicos subscriptos llega */
void callbackMQTT(char *topic, byte *payload, unsigned int length)
{
  String msg;

  // Obtenemos string del payload recibido
  for (int i = 0; i < length; i++) {
    char c = (char)payload[i];
    msg += c;
  }

  Serial.printf("Ingresa el string via MQTT: %s del topico: %s\n", msg, topic);

  /* Toma accion dependiendo de string recibida */
  if (msg.equals("1")) {
    digitalWrite(PIN_LED, HIGH);
    Serial.print("LED encendido por comando MQTT");
  }

  if (msg.equals("0")) {
    digitalWrite(PIN_LED, LOW);
    Serial.print("LED apagado por comando MQTT");
  }
}

/* Reconectarse al broker MQTT */
void reconnectMQTT(void)
{
  while (!mqtt.connected()) {
    Serial.print("* Intentando conectar al Broker MQTT: ");
    Serial.println(tagoBroker);
    if (mqtt.connect(ID_MQTT, tagoUser, tagoToken)) {
      Serial.println("Conectado exitosamente al broker MQTT!");
      //mqtt.subscribe(TOPIC_SUBSCRIBE_LED);
    } else {
      Serial.println("Falla al reconectar al broker.");
      Serial.println("Nuevo intento de conexión en 2 segundos.");
      delay(2000);
    }
  }
}

/* Verifica el estado de conexión WiFI y al broker MQTT.
   */
void checkWiFIAndMQTT(void)
{
  if (!mqtt.connected())
    reconnectMQTT(); 

  reconnectWiFi(); 
}

void reconnectWiFi(void)
{
  // Evalua conexión a red WI-FI.
  // Caso contrário efectua tentativas de conexión
  if (WiFi.status() == WL_CONNECTED)
    return;

  WiFi.begin(SSID, PASSWORD); // Conecta a red WI-FI

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectado exitosamente a la red ");
  Serial.print(SSID);
  Serial.println("IP obtenido: ");
  Serial.println(WiFi.localIP());
}

/* Test Json variable by variable */
void SendDataToTago(char* variable, float value, char* topico){
  JsonDocument doc;
  char buffer[1024];
  JsonObject doc_0 = doc.createNestedObject();
    doc_0["variable"] = variable;
    doc_0["value"] = value;
    char* topico = topico;
    serializeJson(doc, buffer);
    mqtt.publish(topico, buffer);
}

void setup()
{
  Serial.begin(115200);

  // Inicializa la conexión Wi-Fi
  initWiFi();

  // Inicializa la conexiòn al broker MQTT
  initMQTT();
}

void loop()
{
  /* Repite los ciclos cada 2 segundos */
  if ((millis() - publishUpdate) >= PUBLISH_DELAY) {
    publishUpdate = millis();
    // Verifica el funcionamento de conexiones WiFi y al broker MQTT
    checkWiFIAndMQTT();

    // Da formato a strings enviadas para el dashboard (campos texto)
    sprintf(strTemperature, "%.2fC", getTemperature());
    sprintf(strHumidity, "%.2f", getHumidity());

    // Envia las strings al dashboard MQTT
    SendDataToTago("temp");
    SendDataToTago("hum");
    
    //SerializeAndPublish();
    //mqtt.publish(TOPIC_PUBLISH_TEMPERATURE, strTemperature);
    //mqtt.publish(TOPIC_PUBLISH_HUMIDITY, strHumidity);

    // Mantiene la comunicación con broker MQTT
    mqtt.loop();
  }
}
