/*
  Direct messaging demo for ESP8266/ESP32 using painlessMesh
  - Type a numeric nodeId in the Serial Monitor and press Enter to set target.
  - Sends "Hi <target>, from <myId>" to ONE node every 3 seconds.
*/

#include "painlessMesh.h"

#define   MESH_PREFIX     "RNTMESH"
#define   MESH_PASSWORD   "MESHpassword"
#define   MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

uint32_t targetNodeId = 0;                        // set from Serial, or hardcode
String serialBuffer;

void sendDirect();                                // forward-declare
Task taskSendDirect(TASK_SECOND * 3, TASK_FOREVER, &sendDirect);

void sendDirect() {
  if (targetNodeId == 0) return;                  // no target chosen yet
  if (!mesh.isConnected(targetNodeId)) {
    Serial.printf("Target %u not connected.\n", targetNodeId);
    return;
  }
  String msg = "Direct hello to " + String(targetNodeId) +
               " from " + String(mesh.getNodeId());
  bool ok = mesh.sendSingle(targetNodeId, msg);
  Serial.printf("sendSingle(%u): %s\n", targetNodeId, ok ? "OK" : "FAILED");
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  // Auto-ack to sender so they can see it arrived
  if (from != mesh.getNodeId()) {
    mesh.sendSingle(from, "Ack: got -> " + msg);
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId=%u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.println("Changed connections");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset=%d\n", mesh.getNodeTime(), offset);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Enter target nodeId and press Enter (e.g., 1163279684)");
  mesh.setDebugMsgTypes(ERROR | STARTUP);
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
      serialBuffer = ""; // ignore junk
    }
  }
}

void loop() {
  mesh.update();
  readSerialForTarget();
}
