#include "BLEDevice.h"
#include <M5StickCPlus.h>

// Default Temperature in Celsius
#define temperatureCelsius

// BLE Server name to connect to
#define bleServerName "CSC2106-BLE#836834"

// UUIDs for the service and characteristics
static BLEUUID bleServiceUUID("01234567-0123-4567-89ab-0123456789ab");
static BLEUUID temperatureCharacteristicUUID("01234567-0123-4567-89ab-0123456789cd");
static BLEUUID voltageCharacteristicUUID("01234567-0123-4567-89ab-0123456789ef");
static BLEUUID ledCharacteristicUUID("01234567-0123-4567-89ab-0123456789ac"); // Add LED control characteristic UUID

// Connection flags
static boolean doConnect = false;
static boolean connected = false;
static BLEAddress *pServerAddress;

// Characteristics
static BLERemoteCharacteristic* temperatureCharacteristic;
static BLERemoteCharacteristic* voltageCharacteristic;
static BLERemoteCharacteristic* ledCharacteristic;

// Variables for button handling
bool buttonPressed = false;

// Callback for scanning
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("Found device: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.getName() == bleServerName) {
      advertisedDevice.getScan()->stop();
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
      Serial.println("Device found. Connecting!");
    }
  }
};

// Unified notification callback for Temperature and Voltage
static void TempVoltnotifyCallback(
    BLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* pData,
    size_t length,
    bool isNotify) {
  // Determine the source characteristic by UUID
  if (pBLERemoteCharacteristic->getUUID().equals(temperatureCharacteristicUUID)) {
    // Temperature update
    std::string temperature = std::string((char*)pData, length);
    Serial.print("Temperature Updated: ");
    Serial.println(temperature.c_str());

    // Update M5 LCD
    M5.Lcd.fillScreen(BLACK); // Clear screen
    M5.Lcd.setCursor(0, 0, 2);
    M5.Lcd.printf("Temp: %s C\n", temperature.c_str());
  } else if (pBLERemoteCharacteristic->getUUID().equals(voltageCharacteristicUUID)) {
    // Voltage update
    std::string voltage = std::string((char*)pData, length);
    Serial.print("Voltage Updated: ");
    Serial.println(voltage.c_str());

    // Update M5 LCD
    M5.Lcd.setCursor(0, 20, 2);
    M5.Lcd.printf("Battery: %s V\n", voltage.c_str());
  } else {
    Serial.println("Unknown notification received.");
  }
}


// Notification Callback for LED
// static void ledNotifyCallback(
//     BLERemoteCharacteristic* pBLERemoteCharacteristic,
//     uint8_t* pData,
//     size_t length,
//     bool isNotify) {
//   std::string ledData = std::string((char*)pData, length);
//   Serial.print("LED State Changed: ");
//   Serial.println(ledData.c_str());
  
//   M5.Lcd.setCursor(0, 20, 2);
//   if (ledData == "1") {
//     M5.Lcd.printf("LED: ON");
//   } else if (ledData == "0") {
//     M5.Lcd.printf("LED: OFF");
//   } else {
//     M5.Lcd.printf("LED: Unknown");
//   }
// }

// Function to connect to BLE server
bool connectToServer(BLEAddress pAddress) {
  BLEClient* pClient = BLEDevice::createClient();
  if (!pClient->connect(pAddress)) {
    Serial.println("Failed to connect to server.");
    return false;
  }
  Serial.println("Connected to server.");

  BLERemoteService* pRemoteService = pClient->getService(bleServiceUUID);
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find service UUID.");
    return false;
  }

  temperatureCharacteristic = pRemoteService->getCharacteristic(temperatureCharacteristicUUID);
  voltageCharacteristic = pRemoteService->getCharacteristic(voltageCharacteristicUUID);
  ledCharacteristic = pRemoteService->getCharacteristic(ledCharacteristicUUID);


  if (!temperatureCharacteristic) {
    Serial.println("Failed to find temp characteristics.");
    return false;
  } else if(!voltageCharacteristic) {
    Serial.println("Failed to find voltage characteristics.");
    return false;
  } else if(!ledCharacteristic) {
    Serial.println("Failed to find led characteristics.");
    return false;
  }

  Serial.println("Found characteristics.");


   // Register callbacks for notifications
  if (temperatureCharacteristic->canNotify()) {
    temperatureCharacteristic->registerForNotify(TempVoltnotifyCallback);
    Serial.println("Registered for temperature notifications.");
  }
  else {
    Serial.println("Fail to register temperature notifications.");
  }

  if (voltageCharacteristic->canNotify()) {
    voltageCharacteristic->registerForNotify(TempVoltnotifyCallback);
    Serial.println("Registered for voltage notifications.");
  }
  else {
    Serial.println("Fail to register voltage notifications.");
  }

  // if (ledCharacteristic->canNotify()) {
  //   ledCharacteristic->registerForNotify(ledNotifyCallback);
  //   Serial.println("Registered for LED notifications.");
  // }
  // else {
  //   Serial.println("Fail to register LED notifications.");
  // }

  return true;
}

// Function to write to the LED characteristic
void toggleLED() {
  if (connected && ledCharacteristic->canWrite()) {
    static bool ledState = false;
    ledState = !ledState;
    ledCharacteristic->writeValue(ledState ? "1" : "0");
    Serial.println(ledState ? "LED OFF" : "LED ON");
    M5.Lcd.fillScreen(BLACK); // Clear screen
    M5.Lcd.printf(ledState ? "LED OFF" : "LED ON");
  }
}

void writeCharacteristics() {
  Serial.println("Attempting to write characteristics...");
  if (connected && ledCharacteristic->canWrite()) {
    static bool ledState = false;
    ledState = !ledState;
    ledCharacteristic->writeValue(ledState ? "1" : "0");
    Serial.println(ledState ? "LED OFF" : "LED ON");
  }
}

// Function to read characteristics
void readCharacteristics() {
    Serial.println("Attempting to read characteristics...");

    // Read the temperature characteristic
    //if (temperatureCharacteristic != nullptr) {
    //    try {
    //        std::string temperature = temperatureCharacteristic->readValue();
    //        Serial.print("Temperature: ");
    //        Serial.println(temperature.c_str());
    //        M5.Lcd.fillScreen(BLACK); // Clear screen
    //        M5.Lcd.setCursor(0, 0, 2);
    //        M5.Lcd.printf("Temp: %s C\n", temperature.c_str());
    //    } catch (...) {
    //        Serial.println("Failed to read temperature characteristic.");
    //    }
    //} else {
    //    Serial.println("Temperature characteristic is null.");
    //}

    // Read the voltage characteristic
    //if (voltageCharacteristic != nullptr) {
    //    try {
    //        std::string voltage = voltageCharacteristic->readValue();
    //        Serial.print("Voltage: ");
    //        Serial.println(voltage.c_str());
    //        M5.Lcd.setCursor(0, 20, 2); // Move cursor to avoid overlapping
    //        M5.Lcd.printf("Battery: %s V\n", voltage.c_str());
    //    } catch (...) {
    //        Serial.println("Failed to read voltage characteristic.");
    //    }
    //} else {
    //    Serial.println("Voltage characteristic is null.");
    //}

    if (ledCharacteristic != nullptr) {
        try {
            std::string led = ledCharacteristic->readValue();
            Serial.print("Led: ");
            Serial.println(led.c_str());
            M5.Lcd.setCursor(0, 20, 2); // Move cursor to avoid overlapping
            M5.Lcd.fillScreen(BLACK); // Clear screen
            if(led == "0"){
              M5.Lcd.printf("Led On");
              Serial.println("Led Turned On");
            } else if(led == "1"){
              M5.Lcd.printf("Led Off");
              Serial.println("Led Turned Off ");
            }
            
        } catch (...) {
            Serial.println("Failed to read Led characteristic.");
        }
    } else {
        Serial.println("Led characteristic is null.");
    }
}

// BLE Characteristic Write Callback for LED Control
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.print("Received LED Command: ");
      Serial.println(value.c_str());

    }
  }
};


void setup() {
  Serial.begin(115200);
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("BLE Client", 0);

  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
}

void loop() {
  M5.update();

  // Connect to server
  if (doConnect) {
    connected = connectToServer(*pServerAddress);
    doConnect = false;
  }
  

  // Button handling for LED toggle
  if (M5.BtnA.wasPressed()) {
    readCharacteristics();
  }

  // Button handling for reading characteristics
  if (M5.BtnB.wasPressed()) {
    writeCharacteristics();
  }
}
