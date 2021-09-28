/*
  ModbusRTU ESP8266/ESP32
  True RTU-TCP bridge example

  (c)2021 Alexander Emelianov (a.m.emelianov@gmail.com)
  https://github.com/emelianov/modbus-esp8266

  This code is licensed under the BSD New License. See LICENSE.txt for more info.
*/

#include <vector>
#include <WiFi.h>
#include <ModbusTCP.h>
#include <ModbusRTU.h>

ModbusRTU rtu;
ModbusTCP tcp;

// ModbusRTU(SlaveID) => ModbusTCP(IP) mapping table
struct slave_map_t {
  uint8_t slaveId;  // Slave id in incoming request
  IPAddress ip;     // IP address of MosbusTCP Server map request to
  uint8_t unitId = MODBUSIP_UNIT; // UnitId on target server
  slave_map_t(uint8_t s, IPAddress i, uint8_t u  = MODBUSIP_UNIT) {
    slaveId = s;
    ip = i;
    unitId = u; 
  };
};
std::vector<slave_map_t> mapping; // Slave => IP mappings
uint16_t transRunning = 0;  // Currently executed ModbusTCP transaction
uint8_t slaveRunning = 0;   // Current request slave
 
bool cbTcpTrans(Modbus::ResultCode event, uint16_t transactionId, void* data) { // Modbus Transaction callback
  if (event != Modbus::EX_SUCCESS)                  // If transaction got an error
    Serial.printf("Modbus result: %02X\n", event);  // Display Modbus error code
  if (event == Modbus::EX_TIMEOUT) {    // If Transaction timeout took place
    //mb.disconnect(remote);              // Close connection to slave and
    //mb.dropTransactions();              // Cancel all waiting transactions
  }
  return true;
}

// Callback receives raw data from ModbusTCP and sends it on behalf of slave (slaveRunning) to master
Modbus::ResultCode cbTcpRaw(uint8_t* data, uint8_t len, void* custom) {
  Serial.printf("TCP: %d bytes\n", len);
  auto src = (Modbus::frame_arg_t*) custom;
  if (transRunning == src->transactionId) { // Check if transaction id is match
    rtu.rawResponce(slaveRunning, data, len);
  } else
    return Modbus::EX_PASSTHROUG; // Allow frame to be processed by generic ModbusTCP routines
  transRunning = 0;
  slaveRunning = 0;
  return Modbus::EX_SUCCESS; // Stop other processing
}


// Callback receives raw data 
Modbus::ResultCode cbRtuRaw(uint8_t* data, uint8_t len, void* custom) {
  Serial.printf("RTU: %d bytes\n", len);
  auto src = (Modbus::frame_arg_t*) custom;
  auto it = std::find_if(mapping.begin(), mapping.end(), [src](slave_map_t& item){return (item.slaveId == src->slaveId);}); // Find mapping
  if (it != mapping.end()) {
    if (!tcp.isConnected(it->ip)) { // Check if connection established
      if (!tcp.connect(it->ip)) { // Try to connect if not
        rtu.errorResponce(it->slaveId, (Modbus::FunctionCode)data[0], Modbus::EX_DEVICE_FAILED_TO_RESPOND); // Send exceprional responce to master if no connection established
        return Modbus::EX_DEVICE_FAILED_TO_RESPOND; // Stop processing the frame
      }
    }
    // Save transaction ans slave it for responce processing
    transRunning = tcp.rawRequest(it->ip, data, len, cbTcpTrans, it->unitId);
    slaveRunning = it->slaveId;
    return Modbus::EX_SUCCESS; // Stop procesing the frame
  }
  return Modbus::EX_PASSTHROUGH; // Process by generic ModbusRTU routines if no mapping found
}


void setup() {
  Serial.begin(115000);
  WiFi.begin("E2", "fOlissio92");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
    
  tcp.client(); // Initialize ModbusTCP to pracess as client
  tcp.onRaw(cbTcpRaw); // Assign raw data processing callback
  
  Serial1.begin(9600, SERIAL_8N1, 18, 19);
  rtu.begin(&Serial1);
  rtu.slave(3); // Initialize ModbusRTU as slave
  rtu.onRaw(cbRtuRaw); // Assign raw data processing callback

// Assign mappings
  mapping.push_back({1, IPAddress(192,168,30,18)});
  mapping.push_back({2, IPAddress(192,168,30,19)});
}

void loop() {
  rtu.task();
  tcp.task();
  yield();
}