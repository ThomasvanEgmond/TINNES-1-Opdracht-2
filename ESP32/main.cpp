// MQTT code from https://github.com/emqx/MQTT-Client-Examples/blob/master/mqtt-client-ESP32/esp32_connect_mqtt_via_tls.ino
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <secret.h>
#include <string.h>
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bmp; // I2C

float pressure;		//To store the barometric pressure (Pa)
float temperature;
float humidity;

WiFiClientSecure esp_client;
PubSubClient mqtt_client(esp_client);

const char *mqtt_topic = "chat/message";
bool connected = false;
bool ledState = false;

void connectToWiFi();

void connectToMQTT();

void mqttCallback(char *topic, byte *payload, unsigned int length);


void setup() {
    Serial.begin(115200);
    pinMode(2, OUTPUT);

  unsigned status;

    status = bmp.begin(0x76);
    while (!status) bmp.begin(0x76);

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */ 

    connectToWiFi();

    // Set Root CA certificate
    esp_client.setCACert(local_root_ca);

    mqtt_client.setServer(MQTT_HOST, MQTT_PORT);
    mqtt_client.setKeepAlive(60);
    mqtt_client.setCallback(mqttCallback);
    connectToMQTT();
}

//from https://www.martinbroadhurst.com/trim-a-string-in-c
char* trimString(char *str) { 

  char *token = strtok(str, " \t\n\r");
  char *secondToLast;

  while (token != NULL){
    secondToLast = token;
    token = strtok(NULL, " \t\n\r");
    Serial.println(token);
  }
  return secondToLast;
}

void connectToWiFi() {
    WiFi.begin(ssid, pass);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
}

void connectToMQTT() {
    while (!mqtt_client.connected()) {
        String client_id = MQTT_CLIENT_ID;
        Serial.printf("Connecting to MQTT Broker as %s...\n", client_id.c_str());
        if (mqtt_client.connect(client_id.c_str(), MQTT_USER, MQTT_PASS)) {
            Serial.println("Connected to MQTT broker");
            mqtt_client.subscribe(mqtt_topic);
            // mqtt_client.publish(mqtt_topic, "asd");  // Publish message upon connection
        } else {
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.print(mqtt_client.state());
            Serial.println(" Retrying in 5 seconds.");
            delay(5000);
        }
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  if (!connected){
    connected = true;
    return;
  }

  char data[500] = "";
  for (unsigned int i = 0; i < length; i++) {
    char tmp = (char) payload[i];
    strncat(data, &tmp, 1);
  }

  String subData = strtok(data, ">");
  subData.trim();

  if(!subData.equalsIgnoreCase(MQTT_CLIENT_ID)) return;

  Serial.println("\n-----------------------");

  while (subData != NULL){
    Serial.println(subData);
    subData.trim();
    subData.toLowerCase();
    if (subData.equals("led")){
      subData = strtok(NULL, ":");
      if (subData != NULL){
        String ledState = subData;
        ledState.trim();
        ledState.toLowerCase();
        if (ledState.equals("aan")){
          digitalWrite(2, HIGH);
          mqtt_client.publish(mqtt_topic, "BOT-1038854: LED is aan");
        }

        if (ledState.equals("uit")){
          digitalWrite(2, LOW);
          mqtt_client.publish(mqtt_topic, "BOT-1038854: LED is uit");
        }
      }

    }
    if (subData.equals("temperatuur")){
      char test[100];
      sprintf(test, "BOT-1038854: Temperatuur is %.2f °C", bmp.readTemperature());
      mqtt_client.publish(mqtt_topic, test);
    }
    if (subData.equals("druk")){
      char test[100];
      sprintf(test, "BOT-1038854: Druk is %.2f Pa", bmp.readPressure());
      mqtt_client.publish(mqtt_topic, test);
    }
    subData = strtok(NULL, ":");
  }
  Serial.println("\n-----------------------");
  
}


void loop() {
    if (!mqtt_client.connected()) {
        connectToMQTT();
    }
    mqtt_client.loop();
}