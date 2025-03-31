// BLE Server and MQTT Sub&Pub

#include <BLEDevice.h>
#include <BLEServer.h>
#include <M5StickCPlus.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Miko's S24+";
const char* password = "xtkzqk6427ja5zf";
const char* mqtt_server = "192.168.9.89";  // Replace with your broker's IP

WiFiClient espClient;
PubSubClient client(espClient);

void setupWifi();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
bool mqttConnected = false;

// Change to unique BLE server name
#define bleServerName "IOT-PROJ"

// Timer variables
unsigned long lastTime = 0;

unsigned long timerDelay = 15000; // Update information every 15 seconds

bool isClientConnected = false;
BLEAdvertising *pAdvertising;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "01234567-0123-4567-89ab-0123456789ab"
#define MESSAGE_CHAR_UUID "01234567-0123-4567-89ab-0123456789cd"

BLECharacteristic messageCharacteristic(MESSAGE_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
BLEDescriptor messageDescriptor(BLEUUID((uint16_t)0x2902));

void updateBLELog(const char* message) {
  M5.Lcd.fillRect(0, 22, M5.Lcd.width(), 58, BLACK); // Clear BLE log section
  M5.Lcd.setCursor(0, 30, 2);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.println(message);
}

void updateMQTTLog(const char* message) {
  M5.Lcd.fillRect(0, 82, M5.Lcd.width(), 58, BLACK); // Clear MQTT log section
  M5.Lcd.setCursor(0, 90, 2);
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.println(message);
}

// Setup callbacks onConnect and onDisconnect
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    isClientConnected = true;
    Serial.println("[INFO] Client is connected...");
  }

  void onDisconnect(BLEServer *pServer) {
    isClientConnected = false;
    Serial.println("[INFO] Client is disconnected...");
    // Restart advertising so other clients can connect
    pAdvertising->start();
    Serial.println("[INFO] Restarted advertising...");
  }
};

// Callback to handle received messages
class MessageCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string receivedMsg = pCharacteristic->getValue();
    if (receivedMsg.length() > 0) {
      Serial.print("[RECEIVED] Client Message: ");
      Serial.println(receivedMsg.c_str());
      std::string receivedMsgToShow = "[BLE Server Received]: " + receivedMsg;
      updateBLELog(receivedMsgToShow.c_str());
    }
  }
};

void setup() {
  // Start serial communication
  Serial.begin(115200);

  // Initialize M5StickCPlus
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("BLE Server", 0);

  // Draw section dividers
  M5.Lcd.drawLine(0, 20, M5.Lcd.width(), 20, WHITE);  // Divider between header and BLE logs
  M5.Lcd.drawLine(0, 80, M5.Lcd.width(), 80, WHITE);  // Divider between BLE logs and MQTT logs

  Serial.println("Connecting to WiFi...");
    
  setupWifi();
  Serial.println("Connected to WiFi");
    
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.println("Connecting to MQTT...");
  reconnect();
  updateMQTTLog("Connected to MQTT");

  // Create the BLE Device
  BLEDevice::init(bleServerName);

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *bleService = pServer->createService(SERVICE_UUID);
  messageCharacteristic.setCallbacks(new MessageCallbacks());
  bleService->addCharacteristic(&messageCharacteristic);
  messageDescriptor.setValue("Message");
  messageCharacteristic.addDescriptor(&messageDescriptor);

  // Start the service
  bleService->start();

  // Start advertising
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
  Serial.println("[INFO] Currently waiting for a client connection to notify...");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Simulate publishing a test message every 5 seconds
  static unsigned long lastPublishTime = 0;
  if (millis() - lastPublishTime > 5000) {
    lastPublishTime = millis();
        
    String testMessage = "Hello from M5StickCPlus MQTT!";
    client.publish("m5stickc/ble_scan", testMessage.c_str());
        
    Serial.print("[MQTT-PUBLISH] Sent message: ");
    Serial.println(testMessage);
  }
    
  delay(1000);  

  if (isClientConnected) {
    if ((millis() - lastTime) > timerDelay) {
    std::string msgToSend = "Hello from BLE Server";
    messageCharacteristic.setValue(msgToSend);
    messageCharacteristic.notify();
    Serial.printf("[BLE-INFO] Sent: %s\n", msgToSend.c_str());
    std::string msgToShow = "Sent: " + msgToSend;
    updateBLELog(msgToShow.c_str());
    lastTime = millis();
    }
  }
}

void setupWifi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi Connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP().toString());
}

void reconnect() {
    while (!client.connected()) {
        if (client.connect("M5StickCPlusTest")) {
            Serial.println("MQTT Connected");         
            if (!mqttConnected) {  // Only print this message once
              updateMQTTLog("MQTT Connected");
              mqttConnected = true;  // Set the flag to true to prevent further prints
            }    
            client.subscribe("m5stickc/ble_scan"); // Subscribe to an MQTT topic
        } else {
            updateMQTTLog("MQTT Failed, retrying...");  
            delay(5000);
        }
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("[MQTT-SUBSCRIBE] Message received on topic [");
    Serial.print(topic);
    Serial.print("]: ");


    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.println(message);

    String msgToShow = "[MQTT-SUBSCRIBE]: " + message;
    updateMQTTLog(msgToShow.c_str());  

}