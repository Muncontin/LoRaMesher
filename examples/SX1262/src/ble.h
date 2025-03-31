#include <BLEDevice.h>
#include <BLEServer.h>
#include <Arduino.h>

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
        //updateBLELog(receivedMsgToShow.c_str());
      }
    }
  };

  void setupBLEServer() {
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