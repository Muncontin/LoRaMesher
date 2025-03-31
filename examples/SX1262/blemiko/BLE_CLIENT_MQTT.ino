// BLE Client and MQTT Sub&Pub

#include "BLEDevice.h"
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

// change the BLE Server name to connect to
#define bleServerName "IOT-PROJ"

// BLE Service
static BLEUUID bleServiceUUID("01234567-0123-4567-89ab-0123456789ab");
static BLEUUID messageCharacteristicUUID("01234567-0123-4567-89ab-0123456789cd");

// Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;

// Address of the peripheral device. Address will be found during scanning...
static BLEAddress *pServerAddress;
 
// Characteristic to read
static BLERemoteCharacteristic* messageCharacteristic;

// Activate notify
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};

// Variables to store messages
char receivedMessage[100];

// Flags to check whether new message readings are available
boolean newMessage = false;

//Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress) {
   BLEClient* pClient = BLEDevice::createClient();
 
  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");
 
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(bleServiceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(bleServiceUUID.toString().c_str());
    return (false);
  }
 
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  messageCharacteristic = pRemoteService->getCharacteristic(messageCharacteristicUUID);

  if (messageCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID");
    return false;
  }
  Serial.println(" - Found our characteristics");
 
  //Assign callback functions for the characteristics
  messageCharacteristic->registerForNotify(messageNotifyCallback);

  return true;
}

// Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == bleServerName) { // Check if the name of the advertiser matches
      advertisedDevice.getScan()->stop(); // Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); // Address of advertiser is the one we need
      doConnect = true; // Set doConnect indicator, stating that we are ready to connect
      Serial.println("Device found. Connecting!");
    }
    else
      Serial.print(".");
  }
};
 
// Callback function to trigger when the BLE server sends a message update
static void messageNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  Serial.println("Received notification");
  strncpy(receivedMessage, (char*)pData, length);
  receivedMessage[length] = '\0';
  newMessage = true;
}

// Send message function
void sendMessageToServer(const String& message) {
  if (connected && messageCharacteristic) {
    // Validate message length
    if (message.length() > 0 && message.length() < 100) {
      const char* messageBuffer = message.c_str();
      
      // Write message to characteristic
      messageCharacteristic->writeValue(messageBuffer);
      
      // Display sent message
      Serial.print("[BLE-CLIENT] Sent message to BLE Server: ");
      Serial.println(messageBuffer);
      
      M5.Lcd.setCursor(0, 40, 2);
      M5.Lcd.fillRect(0, 40, M5.Lcd.width(), 20, BLACK);  // Clear previous message
      M5.Lcd.printf("[BLE-CLIENT] Sent message to BLE Server: %s", messageBuffer);
    } else {
      Serial.println("Message too long or empty");
    }
  } else {
    Serial.println("Cannot send - not connected");
  }
}

void setup() {
  //Start serial communication
  Serial.begin(115200);
  Serial.println("Starting BLE Client application...");

  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("BLE Client", 0);

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

  //Initialize BLE device
  BLEDevice::init("");
 
  // Retrieve a Scanner and set the callback we want to use to be informed when we have detected a new device.  Specify that we want active scanning and start the scan to run for 30 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
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
        
    String testMessage = "Hello from M5StickCPlus!";
    client.publish("m5stickc/ble_scan", testMessage.c_str());
        
    Serial.print("[MQTT-PUBLISH] Sent message: ");
    Serial.println(testMessage);
  }
    
  delay(10); 

  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("Connected to the BLE Server.");
      
      //Activate the Notify property of each Characteristic
      messageCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      connected = true;
    } else {
      Serial.println("Failed to connect to the server; Restart device to scan for nearby BLE server again.");
    }
    doConnect = false;
  }

  // Print new message received in the OLED if it is available.
  if (newMessage) {
    newMessage = false;
    Serial.print("[BLE-CLIENT] Received message from BLE Server: ");
    Serial.println(receivedMessage);
    updateBLELog(receivedMessage);

  }

  if (connected && Serial.available()) {
    String inputMessage = Serial.readStringUntil('\n');
    inputMessage.trim();

    if (inputMessage.length() > 0) {
      sendMessageToServer(inputMessage);
    }
  }

  delay(1000); // Delay one second between loops.
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

    // Display message on M5StickCPlus screen
    updateMQTTLog(message.c_str());  
}

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