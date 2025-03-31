#include "globals.h"

// Single definition here
IPAddress meshBrokerIP(0, 0, 0, 0);
int meshMode = 0; // 0 = LORA_ONLY, 1 = LORA + MQTT, 2 = LORA + BLE, 3 = LORA + BLE + MQTT, 4 = LORA + BLE + PainlessMesh