// https://docs.m5stack.com/en/quick_start/m5stickc_plus/arduino
// Tan Mun Tat 2300979 CSC
#include <WiFi.h>
#include <WebServer.h>
#include <M5StickCPlus.h>

/* Put your SSID & Password */
const char* ssid = "tootnetwork";
const char* password =  "password";

// read new set of sensors value on button push
float pitch, roll, yaw, temperature, accX, accY, accZ;

WebServer server(80);

void setup() {
  Serial.begin(115200);

  // put your setup code here, to run once:
  M5.begin();

  int x = M5.IMU.Init(); //return 0 is ok, return -1 is unknown
  if(x!=0)
    Serial.println("IMU initialisation fail!");  

  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("RESTful API", 0);

  M5.Imu.Init();

  WiFi.begin(ssid, password);

  // Setting the hostname
  WiFi.setHostname("group01-stick");

  Serial.print("Start WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("Connecting ..");
  }
  M5.Lcd.setCursor(0, 20, 2);
  M5.Lcd.print("IP: ");
  M5.Lcd.println(WiFi.localIP());

  pinMode(M5_BUTTON_HOME, INPUT);
  delay(100);

  server.on("/", handle_JsonResponse);
  server.on("/gyro/", handle_gyro);
  server.on("/accel/", handle_accel);
  server.on("/temp/", handle_temp);

  server.on("/led/1", handle_Led_On);
  server.on("/led/0", handle_Led_Off);
  server.on("/buzzer/1", handle_Buzzer_On);
  server.on("/buzzer/0", handle_Buzzer_Off);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
  Serial.print("Connected to the WiFi network. IP: ");
  Serial.println(WiFi.localIP());
}
 
void handle_JsonResponse(){
  String response;
  response += "{ \"imu\": { \"pitch\": ";
  response += String(pitch, 6);
  response += ", \"roll\": ";
  response += String(roll, 6);
  response += ", \"yaw\": ";
  response += String(yaw, 6);
  response += ", \"accX\": ";
  response += String(accX, 6);
  response += ", \"accY\": ";
  response += String(accY, 6);
  response += ", \"accZ\": ";
  response += String(accZ, 6);
  response += ", \"temperature\": ";
  response += String(temperature, 6);
  response += " } }";
  Serial.println(response);
  digitalWrite(M5_LED, 1);

  server.send(200, "application/json", response);
}

void handle_gyro(){
  String response;
  response += "{ \"imu\": { \"pitch\": ";
  response += String(pitch, 6);
  response += ", \"roll\": ";
  response += String(roll, 6);
  response += ", \"yaw\": ";
  response += String(yaw, 6);
  response += " } }";
  Serial.println(response);
  digitalWrite(M5_LED, 1);

  server.send(200, "application/json", response);
}

void handle_accel(){
  String response;
  response += "{ \"imu\": { \"accX\": ";
  response += String(accX, 6);
  response += ", \"accY\": ";
  response += String(accY, 6);
  response += ", \"accZ\": ";
  response += String(accZ, 6);
  response += " } }";
  Serial.println(response);
  digitalWrite(M5_LED, 1);

  server.send(200, "application/json", response);
}

void handle_temp(){
  String response;
  response += "{ \"temperature\": ";
  response += String(temperature, 6);
  response += "}";
  Serial.println(response);
  digitalWrite(M5_LED, 1);

  server.send(200, "application/json", response);
}

void handle_Led_On(){
  pinMode(10, OUTPUT);          // Set GPIO10 as an output
  digitalWrite(10, LOW);        // Turn ON the LED (active LOW)
  server.send(200, "text/plain", "LED turned on");
}

void handle_Led_Off(){
  pinMode(10, OUTPUT);          // Set GPIO10 as an output
  digitalWrite(10, HIGH);        // Turn OFF the LED (active HIGH)
  server.send(200, "text/plain", "LED turned on");
}

void handle_Buzzer_On(){
  m5.Beep.tone(4000);
  server.send(200, "text/plain", "Buzzer turned on");
}

void handle_Buzzer_Off(){
  m5.Beep.end();
  server.send(200, "text/plain", "Buzzer turned off");
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

//SENSORS
bool readGyro(){
  M5.IMU.getAhrsData(&pitch, &roll, &yaw);
  return true;
}

bool readAccel(){
  M5.Imu.getAccelData(&accX, &accY, &accZ);
  return true;
}

float readTemperature(){
  float t;
  M5.IMU.getTempData(&t);
  t = (t-32.0)*5.0/9.0;
  return t;
}

uint8_t setup_flag = 1;

bool readSensors() {
  bool status = readGyro();
  bool accelStatus = readAccel();
  if (status==true && accelStatus==true) {
    M5.Lcd.setCursor(0, 40, 2);
    M5.Lcd.printf("                                        ");
  }
  Serial.println("Gyro:");
  Serial.print("Pitch[X]: ");
  Serial.println(pitch);
  Serial.print("Roll[Y]: ");
  Serial.println(roll);
  Serial.print("Yaw[Z]: ");
  Serial.println(yaw);

  Serial.println("Accel:");
  Serial.print("Accel[X]: ");
  Serial.println(accX);
  Serial.print("Accel[Y]: ");
  Serial.println(accY);
  Serial.print("Accel[Z]: ");
  Serial.println(accZ);

  temperature = readTemperature();
  M5.Lcd.setCursor(0, 60, 2);
  M5.Lcd.printf("Temperature = %2.1f", temperature);
  Serial.print("Temperature: ");
  Serial.println(temperature);

  return status;
}
 
void loop() {
  M5.update();
  server.handleClient();

  if (setup_flag == 1) {
    M5.Lcd.setCursor(0, 40, 2);
    M5.Lcd.printf("Gyro: X = %3.2f, Y = %3.2f, Z = %3.2f", pitch, roll, yaw);
    M5.Lcd.setCursor(0, 60, 2);
    M5.Lcd.printf("Temperature = %2.1f", temperature);
    M5.Lcd.setCursor(0,80,2);
    M5.Lcd.printf("Accel: X = %3.2f, Y = %3.2f, Z = %3.2f", accX, accY, accZ);
  }

  if(!setup_flag){
    setup_flag = 1;
    bool status = readSensors();
    if (status)
      Serial.print("\n\rRead Sensors success...\n");
    else
      Serial.print("\n\rRead Sensors failed...\n");
  }

  if(digitalRead(M5_BUTTON_HOME) == LOW){
    setup_flag = 0;
    while(digitalRead(M5_BUTTON_HOME) == LOW);
  }
}