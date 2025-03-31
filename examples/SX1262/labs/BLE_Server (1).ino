#include <BLEDevice.h>
#include <BLEServer.h>
#include <M5StickCPlus.h>

// Default Temperature in Celsius
#define temperatureCelsius

// BLE server name
#define bleServerName "CSC2106-BLE#836834"

// UUIDs for BLE services and characteristics
#define SERVICE_UUID "01234567-0123-4567-89ab-0123456789ab"
#define LED_CHAR_UUID "01234567-0123-4567-89ab-0123456789ac"

// BLE Characteristics
#ifdef temperatureCelsius
BLECharacteristic imuTemperatureCelsiusCharacteristic("01234567-0123-4567-89ab-0123456789cd", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor imuTemperatureCelsiusDescriptor(BLEUUID((uint16_t)0x2902));
#endif

BLECharacteristic axpVoltageCharacteristic("01234567-0123-4567-89ab-0123456789ef", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor axpVoltageDescriptor(BLEUUID((uint16_t)0x2903));

BLECharacteristic ledControlCharacteristic(LED_CHAR_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor ledControlDescriptor(BLEUUID((uint16_t)0x2901));

// Global variables
float tempC = 25.0;
float vBatt = 5.0;
bool ledStatus = false; // LED state
bool deviceConnected = false;
unsigned long lastTime = 0;
unsigned long ledToggleTime = 0;
const unsigned long debounceDelay = 200; // 200ms debounce time
unsigned long lastButtonPress = 0; // Track last button press time
const unsigned long timerDelay = 15000; // 15 seconds

// Update LED state
void updateLEDState() {
  digitalWrite(10, ledStatus ? HIGH : LOW);
  Serial.print("LED State Updated: ");
  Serial.println(ledStatus ? "OFF" : "ON");

  // Notify the BLE client about the LED state
  ledControlCharacteristic.setValue(ledStatus ? "1" : "0");
  ledControlCharacteristic.notify();
}

// BLE Server Callback
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Client Connected");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Client Disconnected");
    delay(2000);
    pServer->getAdvertising()->start();
  }
};

// BLE Characteristic Write Callback for LED Control
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.print("Received LED Command: ");
      Serial.println(value.c_str());

      // Update LED state based on the received value
      if (value == "1") {
        ledStatus = true;
      } else if (value == "0") {
        ledStatus = false;
      }
      updateLEDState();
    }
  }
};

void setup() {
  // Initialize serial communication and M5StickC Plus
  Serial.begin(115200);
  pinMode(10, OUTPUT);  // Assuming GPIO 10 for the LED
  digitalWrite(10, LOW);

  delay(1000);

  M5.begin();
  M5.Axp.begin();

  // Display startup message
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.println("BLE Server");

  // Initialize BLE
  BLEDevice::init(bleServerName);
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE service
  BLEService* bleService = pServer->createService(SERVICE_UUID);

  // Add temperature characteristic
  #ifdef temperatureCelsius
  bleService->addCharacteristic(&imuTemperatureCelsiusCharacteristic);
  imuTemperatureCelsiusDescriptor.setValue("IMU Temperature(C)");
  imuTemperatureCelsiusCharacteristic.addDescriptor(&imuTemperatureCelsiusDescriptor);
  #endif

  // Add battery voltage characteristic
  bleService->addCharacteristic(&axpVoltageCharacteristic);
  axpVoltageDescriptor.setValue("AXP Battery(V)");
  axpVoltageCharacteristic.addDescriptor(&axpVoltageDescriptor);

  // Add LED control characteristic
  ledControlCharacteristic.setValue("0");
  ledControlCharacteristic.addDescriptor(&ledControlDescriptor);
  ledControlCharacteristic.setCallbacks(new MyCallbacks());
  bleService->addCharacteristic(&ledControlCharacteristic);

  // Start BLE service and advertising
  bleService->start();
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();

  Serial.println("Waiting for a client to connect...");
}

void notifyTempVolt(){
  // Notify temperature reading from IMU
      #ifdef temperatureCelsius
        static char temperatureCTemp[6];
        dtostrf(tempC, 6, 2, temperatureCTemp); // Convert temperature to string
        imuTemperatureCelsiusCharacteristic.setValue(temperatureCTemp);
        imuTemperatureCelsiusCharacteristic.notify();  // Notify the client with the new temperature
        Serial.print("Temperature = ");
        Serial.print(tempC);
        Serial.print(" C");
      #else
        static char temperatureFTemp[6];
        dtostrf(tempC, 6, 2, temperatureFTemp); // Convert temperature in Celsius
        imuTemperatureCelsiusCharacteristic.setValue(temperatureFTemp);
        imuTemperatureCelsiusCharacteristic.notify();  // Notify the client with the new temperature
        Serial.print("Temperature = ");
        Serial.print(tempC);
        Serial.print(" C");
      #endif

      // Notify battery voltage status reading from AXP192
      static char voltageBatt[6];
      dtostrf(vBatt, 6, 2, voltageBatt); // Convert voltage to string
      axpVoltageCharacteristic.setValue(voltageBatt);
      axpVoltageCharacteristic.notify();  // Notify the client with the new battery voltage

      Serial.print("Battery Voltage = ");
      Serial.print(vBatt);
      Serial.println(" V");

      // Display the updated values on the LCD screen
      M5.Lcd.setCursor(0, 20, 2);
      M5.Lcd.print("Temperature = ");
      M5.Lcd.print(tempC);
      M5.Lcd.println(" C");

      M5.Lcd.setCursor(0, 40, 2);
      M5.Lcd.print("Battery Voltage = ");
      M5.Lcd.print(vBatt);
      M5.Lcd.println(" V");
}

void loop() {
  if (deviceConnected) {
    // Ensure BtnA is pressed successively and not held
    if (M5.BtnA.wasPressed()) {
      unsigned long currentMillis = millis();
      if (currentMillis - lastButtonPress > debounceDelay) {
        lastButtonPress = currentMillis;  // Update the last button press time
        ledStatus = !ledStatus;

        // Update the LED hardware output
        digitalWrite(10, ledStatus ? HIGH : LOW);

        // Update BLE characteristic for LED control and notify the client
        ledControlCharacteristic.setValue(ledStatus ? "1" : "0");
        ledControlCharacteristic.notify();

        Serial.print("BtnA Pressed - LED State: ");
        Serial.println(ledStatus ? "OFF" : "ON");

        // Send temperature and voltage data after the button is pressed
        float rawTemp = M5.Axp.GetTempData(); // Raw temperature reading from the AXP192 sensor
        tempC = rawTemp / 10.0;  // Correct the scaling of the temperature

        vBatt = M5.Axp.GetVbatData() / 1000.0; // Battery voltage reading from the AXP192

        notifyTempVolt();
      }
    }
  }
  M5.update();
}
