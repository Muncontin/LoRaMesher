#include <Arduino.h>
#include "driver/gpio.h"  // Include the GPIO driver header
#include "mqtt.h"
#include "ble.h"
#include "pmesh.h"

//Using LILYGO TTGO T-BEAM v1.1 
#define BOARD_LED   4
#define LED_ON      LOW
#define LED_OFF     HIGH

LoraMesher& radio = LoraMesher::getInstance();
RoutingTableService routingtableService;
IPAddress brokerIP;

uint32_t dataCounter = 0;
struct dataPacket {
    uint32_t counter = 0;
};

dataPacket* helloPacket = new dataPacket;

//Led flash
void led_Flash(uint16_t flashes, uint16_t delaymS) {
    uint16_t index;
    for (index = 1; index <= flashes; index++) {
        digitalWrite(BOARD_LED, LED_ON);
        delay(delaymS);
        digitalWrite(BOARD_LED, LED_OFF);
        delay(delaymS);
    }
}

// Gets nodes from routing table that are known as ROLE 1 NODES.
void printGatewayNodes() {
    NetworkNode* nodes = routingtableService.getAllNetworkNodes();

    if (!nodes) {
        Serial.println("Routing table is empty.");
        return;
    }

    int routingSize = routingtableService.routingTableSize();
    Serial.println("Nodes with Role ID 1:");
    Serial.println("-----------------------------------------");

    bool found = false;

    for (int i = 0; i < routingSize; i++) {
        if (nodes[i].role == 1) {
            IPAddress ip = nodes[i].mqttBrokerIP;

            Serial.printf(
                "%d | Address: %04X | Hops: %d | Role: %d | Broker IP: %d.%d.%d.%d\n",
                i,
                nodes[i].address,
                nodes[i].metric,
                nodes[i].role,
                ip[0], ip[1], ip[2], ip[3]
            );
            found = true;
        }
    }

    if (!found) {
        Serial.println("No nodes with Role ID 1 found.");
    }

    delete[] nodes;
}

void sendMessageToRole1Nodes(const String& message) {
    NetworkNode* nodes = routingtableService.getAllNetworkNodes();

    if (!nodes) {
        Serial.println("Routing table is empty.");
        return;
    }

    int routingSize = routingtableService.routingTableSize();
    bool found = false;

    for (int i = 0; i < routingSize; i++) {
        if (nodes[i].role == 1) {
            found = true;
            uint16_t destAddress = nodes[i].address;

            const char* msg = message.c_str();
            size_t msgLen = strlen(msg) + 1;  // Include null terminator

            Serial.printf("Sending to Role 1 node %04X: %s\n", destAddress, msg);

            radio.createPacketAndSend(destAddress, (void*)msg, msgLen);
        }
    }

    if (!found) {
        Serial.println("No Role 1 nodes found to send message.");
    }

    delete[] nodes;
}



void printRoutingTable() {
    routingtableService.printRoutingTable();
}

/**
 * @brief Print the counter of the packet
 *
 * @param data
 */
void printPacket(dataPacket data) {
    Serial.printf("Hello Counter received nº %d\n", data.counter);
}

/**
 * @brief Iterate through the payload of the packet and print the counter of the packet
 *
 * @param packet
 */
void printDataPacket(AppPacket<dataPacket>* packet) {
    Serial.printf("Packet arrived from %X with size %d\n", packet->src, packet->payloadSize);

    //Get the payload to iterate through it
    dataPacket* dPacket = packet->payload;
    size_t payloadLength = packet->getPayloadLength();

    for (size_t i = 0; i < payloadLength; i++) {
        //Print the packet
        printPacket(dPacket[i]);
    }
}

/**
 * @brief Function that process the received packets
 *
 */
void processReceivedPackets(void*) {
    for (;;) {
        /* Wait for the notification of processReceivedPackets and enter blocking */
        ulTaskNotifyTake(pdPASS, portMAX_DELAY);
        led_Flash(1, 100); //one quick LED flashes to indicate a packet has arrived

        //Iterate through all the packets inside the Received User Packets Queue
        while (radio.getReceivedQueueSize() > 0) {
            Serial.println("ReceivedUserData_TaskHandle notify received");
            Serial.printf("Queue receiveUserData size: %d\n", radio.getReceivedQueueSize());

            //Get the first element inside the Received User Packets Queue
            AppPacket<dataPacket>* packet = radio.getNextAppPacket<dataPacket>();

            Serial.println("Data received:");
            //Print the data packet
            printDataPacket(packet);

            printRoutingTable();

            //Delete the packet when used. It is very important to call this function to release the memory of the packet.
            radio.deletePacket(packet);
        }
    }
}

TaskHandle_t receiveLoRaMessage_Handle = NULL;

/**
 * @brief Create a Receive Messages Task and add it to the LoRaMesher
 *
 */
void createReceiveMessages() {
    int res = xTaskCreate(
        processReceivedPackets,
        "Receive App Task",
        4096,
        (void*) 1,
        2,
        &receiveLoRaMessage_Handle);
    if (res != pdPASS) {
        Serial.printf("Error: Receive App Task creation gave error: %d\n", res);
    }

    radio.setReceiveAppDataTaskHandle(receiveLoRaMessage_Handle);
}


/**
 * @brief Initialize LoRaMesher
 *
 */
void setupLoraMesher() {
    LoraMesher::LoraMesherConfig config;
    config.module = LoraMesher::LoraModules::SX1262_MOD;

    // Ensure SPI bus is initialized before LoRaMesher begins
    SPI.begin(5, 19, 27, 18); // SCK, MISO, MOSI, NSS

    // Manually specify SPI bus
    config.spi = &SPI;

    // Initialize LoRaMesher
    radio.begin(config);

    RoleService::setRole(0);
    createReceiveMessages();

    // Start LoRaMesher
    radio.start();
    Serial.println("LoRa initialized");
}

void setup() {
    Serial.begin(115200);

    // Install the GPIO ISR service
    if (gpio_install_isr_service(0) != ESP_OK) {
        Serial.println("Failed to install GPIO ISR service");
    }

    Serial.println("initBoard");
    pinMode(BOARD_LED, OUTPUT); //setup pin as output for indicator LED
    led_Flash(2, 125);          //two quick LED flashes to indicate program start
    setupLoraMesher();
    mqttSetup();
    setupBLEServer();
}


void loop() {
    for (;;) {

        Serial.printf("Send packet %d\n", dataCounter);
        
        radio.createPacketAndSend(BROADCAST_ADDR, helloPacket, 1);
        //sendMessageToRole1Nodes("This is a directed message to all role 1 nodes");

        printGatewayNodes();

        //Verifies if MQTT broker is connected.
        if (!client.connected()) {
            reconnect();
        } 
        client.loop();


        //Wait 20 seconds to send the next packet
        vTaskDelay(20000 / portTICK_PERIOD_MS);
    }
}