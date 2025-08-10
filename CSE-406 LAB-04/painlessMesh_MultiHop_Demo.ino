/*
  Multi-hop direct messaging demo for ESP8266/ESP32 using painlessMesh
  - Choose a target nodeId from Serial (or hardcode).
  - Sends a direct message every 4s.
  - Enables CONNECTION and COMMUNICATION debug to observe routing/topology.
  - Any intermediate hops are selected automatically by painlessMesh.
*/

#include "painlessMesh.h"

#define   MESH_PREFIX     "RNTMESH"
#define   MESH_PASSWORD   "MESHpassword"
#define   MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

uint32_t targetNodeId = 0;
String serialBuffer;

void sendDirect();                                // forward-declare
Task taskSendDirect(TASK_SECOND * 4, TASK_FOREVER, &sendDirect);

void sendDirect() {
  if (targetNodeId == 0) return;
  String msg = "Hi Node " + String(targetNodeId) +
               "! From " + String(mesh.getNodeId());
  bool ok = mesh.sendSingle(targetNodeId, msg);
  Serial.printf("sendSingle(%u): %s\n", targetNodeId, ok ? "OK" : "FAILED");
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  // Reply so sender sees end-to-end success
  mesh.sendSingle(from, "Reply: Got your message: " + msg);
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("--> New Connection, nodeId=%u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.println("Changed connections (topology updated)");
  // Optional: print current connections table
  auto nodes = mesh.getNodeList(true);
  Serial.print("My neighbors: ");
  for (auto &&id : nodes) { Serial.printf("%u ", id); }
  Serial.println();
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset=%d\n", mesh.getNodeTime(), offset);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Multi-hop demo. Enter target nodeId and press Enter.");
  // Turn on extra debug so you can see link changes & communication flow
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | COMMUNICATION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);

  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskSendDirect);
  taskSendDirect.enable();
}

void readSerialForTarget() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      if (serialBuffer.length() > 0) {
        targetNodeId = (uint32_t) strtoul(serialBuffer.c_str(), nullptr, 10);
        Serial.printf("Target set to: %u\n", targetNodeId);
        serialBuffer = "";
      }
    } else if (isDigit(c)) {
      serialBuffer += c;
    } else {
      serialBuffer = "";
    }
  }
}

void loop() {
  mesh.update();
  readSerialForTarget();
}
