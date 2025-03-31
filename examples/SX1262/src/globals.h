#pragma once
#include <IPAddress.h>

#include "LoraMesher.h"

// Globally accessible broker IP (default to 0.0.0.0)
extern IPAddress meshBrokerIP;

// 0 = LORA_ONLY
// 1 = LORA + MQTT
// 2 = LORA + BLE
// 3 = LORA + BLE + MQTT
// 4 = LORA + BLE + PainlessMesh

extern int meshMode; // 0 = LORA_ONLY, 1 = LORA + MQTT, 2 = LORA + BLE, 3 = LORA + BLE + MQTT, 4 = LORA + BLE + PainlessMesh