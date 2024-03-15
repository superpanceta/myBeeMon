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

#include <DHTesp.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h> 

#define PIN_LED 15 // GPIO que está conectado el LED
#define PIN_DHT 12 // GPIO que está conectado el sensor

/* Configura los tópicos de MQTT */
/* 
#define TOPIC_SUBSCRIBE_LED       "topic_on_off_led"
#define TOPIC_PUBLISH_TEMPERATURE "topic_sensor_temperature"
#define TOPIC_PUBLISH_HUMIDITY    "topic_sensor_humidity"
*/
const char *TopicsToPublish[] = {
  "data",
  "info"
};



#define PUBLISH_DELAY 2000   // Atraso de publicación (2 segundos)

#define ID_MQTT "esp32_beemon_sender" // id mqtt (para identificar sesión)

/* Variables, constantes y objetos globales */
DHTesp dht;

const char *SSID = "Wokwi-GUEST"; // SSID / nombre de red WI-FI que desea conectar
const char *PASSWORD = "";        // Pass de red WI-FI que desea conectar

// URL del broker MQTT que se desea utilizar
const char* tagoBroker = "mqtt.tago.io";
const char* tagoUser = "beeMon";
const char* tagoToken = "5d44ef90-8e03-4c32-8012-b770c72b0787";
const char* tagoDeviceNAme = "";
int tagoPort = 1883; // Puerto del Broker MQTT

unsigned long publishUpdate;

static char strTemperature[10] = {0};
static char strHumidity[10] = {0};

// Variables y objetos globales
WiFiClient espClient;         // Crea el objeto espClient
PubSubClient mqtt(espClient); // Instancia el Cliente MQTT pasando el objeto espClient

/* Prototipos */
float getTemperature(void);
float getHumidity(void);

void initWiFi(void);
void initMQTT(void);
void callbackMQTT(char *topic, byte *payload, unsigned int length);
void reconnectMQTT(void);
void reconnectWiFi(void);
void checkWiFIAndMQTT(void);


/* Fase de lectura de temperatura (sensor DHT22)
   Retorno: temperatura (grados Celsius) */
float getTemperature(void)
{
  TempAndHumidity data = dht.getTempAndHumidity();

  if (!(isnan(data.temperature))){
    Serial.println(data.temperature);
    return data.temperature;}
  else
    return -99.99;
}

/* Fase de lectura de humedad (sensor DHT22)
   Retorno: humedad (0 - 100%) */
float getHumidity(void)
{
  TempAndHumidity data = dht.getTempAndHumidity();

  if (!(isnan(data.humidity))){
    Serial.println(data.humidity);
    return data.humidity;}
  else
    return -99.99;
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
      mqtt.subscribe(TOPIC_SUBSCRIBE_LED);
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

/* Test Json data*/
void SerializeAndPublish(){
  char buffer[1024];                                      // JSON serialization 
  StaticJsonDocument<512> doc;

  JsonObject doc_0 = doc.createNestedObject();
  doc_0["variable"] = "BeeHive1";
  doc_0["value"] = getTemperature();
  doc_0["unit"] = "C"

  JsonObject doc_1 = doc.createNestedObject();
  doc_1
  

}

void setup()
{
  Serial.begin(115200);

  // Configura el pin del LED como output e inicializa en nível bajo
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // Inicializa el sensor de temperatura
  dht.setup(PIN_DHT, DHTesp::DHT22);

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
    mqtt.publish(TOPIC_PUBLISH_TEMPERATURE, strTemperature);
    mqtt.publish(TOPIC_PUBLISH_HUMIDITY, strHumidity);

    // Mantiene la comunicación con broker MQTT
    mqtt.loop();
  }
}
