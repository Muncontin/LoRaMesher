#include <painlessMesh.h>

#define   MESH_SSID       "tootnetwork"
#define   MESH_PASSWORD   "meshpotatoes"
#define   MESH_PORT       5555

// Prototypes
void sendMessage(); 
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback(); 
void nodeTimeAdjustedCallback(int32_t offset); 
void delayReceivedCallback(uint32_t from, int32_t delay);

Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;

bool calc_delay = false;
SimpleList<uint32_t> nodes;

void sendMessage(); // Prototype
Task taskSendMessage( TASK_SECOND * 1, TASK_FOREVER, &sendMessage ); // start with a one-second interval

void pMeshSetup() {
    mesh.setDebugMsgTypes(ERROR | DEBUG);  // Enable debug messages

    mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
    mesh.onNodeDelayReceived(&delayReceivedCallback);

    userScheduler.addTask(taskSendMessage);
    taskSendMessage.enable();
}

void sendMessage() {
    String msg = "Hello from M5StickC " + String(mesh.getNodeId());
    mesh.sendBroadcast(msg);
  
    if (calc_delay) {
      for (auto node : nodes) {
        mesh.startDelayMeas(node);
      }
      calc_delay = false;
    }
  
    Serial.printf("Sending message: %s\n", msg.c_str());
    
    taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

void receivedCallback(uint32_t from, String & msg) {
    Serial.printf("Received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New Connection: nodeId = %u\n", nodeId);
  }
  
  void changedConnectionCallback() {
    Serial.println("Changed connections");
  
    nodes = mesh.getNodeList();
    Serial.printf("Num nodes: %d\n", nodes.size());
  
    Serial.print("Connection list: ");
    for (auto node : nodes) {
      Serial.printf(" %u", node);
    }
    Serial.println();
    calc_delay = true;
  }
  
  void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time: %u. Offset = %d\n", mesh.getNodeTime(), offset);
  }
  
  void delayReceivedCallback(uint32_t from, int32_t delay) {
    Serial.printf("Delay to node %u is %d us\n", from, delay);
  }