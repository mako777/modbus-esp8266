/*
  ModbusRTU ESP8266/ESP32
  True RTU-TCP bridge example

  (c)2021 Alexander Emelianov (a.m.emelianov@gmail.com)
  https://github.com/emelianov/modbus-esp8266

  This code is licensed under the BSD New License. See LICENSE.txt for more info.
*/

#include <WiFi.h>
#include <ModbusTCP.h>
#include <ModbusRTUtoTCP.h>

#define SLAVE_ID 1
#define ADDR IPAddress(192, 168, 30, 18)

ModbusRTUtoTCP rtu;
ModbusTCP tcp;

void setup() {
  Serial.begin(115000);
  WiFi.begin("SSID", "PASSWORD");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
    
  tcp.client();
  
  Serial1.begin(9600, SERIAL_8N1, 18, 19);
  rtu.begin(&Serial1);
  rtu.bridge(&tcp);
  rtu.addMap(SLAVE_ID, ADDR, 255);
}

void loop() {
  rtu.task();
  tcp.task();
  yield();
}