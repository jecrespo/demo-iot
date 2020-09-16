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
#include <SimpleDHT.h>
#include "secrets.h"

char ssid[] = SECRET_SSID;   // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password

const char* mqtt_server = "node02.myqtthub.com";

int pinDHT11 = D4;
SimpleDHT11 dht11(pinDHT11);

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
boolean estado_anterior;

const char* publish_temp = "demo/temperatura/2";
const char* publish_reset = "demo/reset/2";
const char* publish_alert = "demo/alerta/2";
const char* subs_led = "demo/led/2";

void setup() {
  pinMode(D1, OUTPUT);     // Initialize the RELAY pin as an output
  pinMode(D2, INPUT);     // Initialize the RELAY pin as an output
  estado_anterior = digitalRead(D2);
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

  boolean estado = digitalRead(D2);
  if (estado_anterior != estado) {
    estado_anterior = estado;
    if (estado == HIGH) {
      Serial.println("Pulsado");
      client.publish(publish_alert, "alerta conectado");
    }
    else {
      Serial.println("libre");
      client.publish(publish_alert, "alerta desconectado");
    }
  }

  long now = millis();
  if (now - lastMsg > 60000) {
    lastMsg = now;
    Serial.println("Sample DHT11...");
    // read without samples.
    byte temperature = 0;
    byte humidity = 0;
    int err = SimpleDHTErrSuccess;
    if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
      Serial.print("Read DHT11 failed, err="); Serial.println(err); delay(1000);
      return;
    }
    Serial.print("Sample OK: ");
    Serial.print((float)temperature); Serial.print(" *C, ");
    Serial.println();
    float value = (float)temperature;
    snprintf (msg, 50, "%2.1f", value);
    client.publish(publish_temp, msg);
    Serial.print("Publish message: ");
    Serial.println(msg);
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
      digitalWrite(D1, HIGH);   // Turn the LED on
    } else {
      digitalWrite(D1, LOW);  // Turn the LED off
    }
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

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

  Serial.print("Attempting MQTT connection...");
  // Create a random client ID
  String clientId = "esp8266-demo-01";
  //clientId += String(random(0xffff), HEX);
  // Attempt to connect
  if (client.connect(clientId.c_str(), SECRET_MQTT_USER, SECRET_MQTT_PASS)) {
    Serial.println("connected");
    // Once connected, publish an announcement...
    client.publish(publish_reset, "reset");
    Serial.println("publicado reset");
    // ... and resubscribe
    client.subscribe(subs_led);
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    // Wait 5 seconds before retrying
    delay(5000);
  }
}
