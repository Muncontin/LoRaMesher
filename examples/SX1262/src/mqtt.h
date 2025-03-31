#include <WiFi.h>
#include <PubSubClient.h>
#include <globals.h>

// WiFi Credentials
const char* ssid = "Chocolate";
const char* password = "bunnymagpie0520";

// MQTT Broker Settings
const char* mqttServer = "192.168.86.35";  // Public Broker  
const int mqttPort = 1883;
const char* mqttUser = ""; // Optional
const char* mqttPassword = ""; // Optional

WiFiClient espClient;
PubSubClient client(espClient);

void connectMQTT() {
    if (!client.connected()) {
        Serial.print("Attempting MQTT connection... ");

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi not connected.");
            return;
        }

        if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
            Serial.println("connected!");
            client.subscribe("esp32/receive");
            meshBrokerIP.fromString(mqttServer);
        } else {
            Serial.print("failed, rc=");
            Serial.println(client.state());
            meshBrokerIP.fromString("0.0.0.0"); // If no broker detected/wifi fail, reset the broker IP address
        }
    }
}


// Callback function to handle received MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");

    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);
} 

void mqttSetup() {
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");

    // Setup MQTT
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);
    connectMQTT();
}

void reconnect() {
    while (!client.connected()) {
        if (client.connect("NodeA")) {
            client.subscribe("nodeA/led");
        } else {
            delay(5000);
        }
    }
}

