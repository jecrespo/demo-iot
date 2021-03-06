/*
   Ejemplo básico de nodo remoto para interacción con MQTT y Node-RED para Wemos D1 Mini + DHT11 Shield: https://wiki.wemos.cc/products:d1_mini_shields:dht_shield
   Publica en un topic cada 10 segundos con la temperatura
   En cada reset publica un mensaje indicando que se ha reiniciado
   Instalar librería https://github.com/wemos/WEMOS_DHT12_Arduino_Library

   El fichero secrets.h debe estar en el miso directorio que este fichero .ino y debe tener el formato
   #define SECRET_SSID "nombre_ssid_wifi"
   #define SECRET_PASS "password_wifi"
   #define SECRET_MQTT_USER "user_mqtt_broker"
   #define SECRET_MQTT_PASS "password_mqtt_broker"
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WEMOS_DHT12.h>
#include "secrets.h"

char ssid[] = SECRET_SSID;   // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password

const char* mqtt_server = "broker.mqtt-dashboard.com";

DHT12 dht12;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
long lastMsgM = 0;
char msg[50];
int value = 0;
int valueM = 0;

const char* publish_temp = "nodo1/temp";
const char* publish_reset = "nodo1/reset";
const char* subs_led = "nodo1/led";

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 10000) {
    if (dht12.get() == 0) {
      Serial.print("Temperature in Celsius : ");
      Serial.println(dht12.cTemp);
      Serial.print("Temperature in Fahrenheit : ");
      Serial.println(dht12.fTemp);
      Serial.print("Relative Humidity : ");
      Serial.println(dht12.humidity);
      Serial.println();
      Serial.print("Publish message: ");
      Serial.println(msg);
      float value = dht12.cTemp;
      snprintf (msg, 50, "#%f", value);
      client.publish(publish_temp, msg);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (String(topic) == String(subs_led)) {
    // Switch on the LED if an 1 was received as first character
    if ((char)payload[0] == '1') {
      digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
      // but actually the LED is on; this is because
      // it is active low on the ESP-01)
    } else {
      digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
    }
  }
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str()),SECRET_MQTT_USER,SECRET_MQTT_PASS) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(publish_reset, "reset");
      // ... and resubscribe
      client.subscribe(subs_led);
      client.subscribe(subs_text);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
