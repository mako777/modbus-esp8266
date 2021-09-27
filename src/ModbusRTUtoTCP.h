/*
    Modbus Library for Arduino
	ModbusRTU True Bridge
    Copyright (C) 2021 Alexander Emelianov (a.m.emelianov@gmail.com)
	https://github.com/emelianov/modbus-esp8266
	This code is licensed under the BSD New License. See LICENSE.txt for more info.
*/
#pragma once
#include "ModbusRTU.h"
#include "ModbusTCP.h"

Modbus::ResultCode cbRawHandler(uint8_t* data, uint8_t len, void* custom) {
	Serial.printf("Received %d bytes\n", len);
	return Modbus::EX_SUCCESS;
}

class ModbusRTUtoTCP : public ModbusRTU {
    protected:
		struct map_t {
			uint8_t slaveId;
			uint8_t unitId = MODBUSIP_UNIT;
			IPAddress ip;
		};
		map_t mapping;
		ModbusTCP* tcp;
		uint16_t transWaiting = 0;
		uint8_t idWaiting = 0;
    public:
        void task();
		void client() { };
		inline void master() {client();}
		void server(uint8_t serverId) {_slaveId = serverId;};
		inline void slave(uint8_t slaveId) {server(slaveId);}
		uint8_t server() { return _slaveId; }
		inline uint8_t slave() { return server(); }
		void bridge(ModbusTCP* t) {
			tcp = t;
			tcp->onRaw(cbRawHandler);
		}
		bool addMap(uint8_t id, IPAddress ip, uint8_t unitid) { mapping.slaveId = id; mapping.ip = ip; return true; }
};

bool cbBri(Modbus::ResultCode event, uint16_t transactionId, void* data) { // Modbus Transaction callback
  if (event != Modbus::EX_SUCCESS)                  // If transaction got an error
    Serial.printf("Modbus result: %02X\n", event);  // Display Modbus error code
  if (event == Modbus::EX_TIMEOUT) {    // If Transaction timeout took place
    //mb.disconnect(remote);              // Close connection to slave and
    //mb.dropTransactions();              // Cancel all waiting transactions
  }
  return true;
}

void ModbusRTUtoTCP::task() {
	#if defined(ESP32)
	//taskENTER_CRITICAL(&mux);
	//vTaskSuspendAll();
	#endif
    if (_port->available() > _len) {
        _len = _port->available();
        t = millis();
    }
	if (_len == 0) {
		#if defined(ESP32)
    	//taskEXIT_CRITICAL(&mux);
		//xTaskResumeAll();
 		#endif
		return;
	}
	// For slave wait for whole message to come (unless MODBUSRTU_MAX_READMS reached)
	uint32_t taskStart = millis();
    while (millis() - t < _t) { // Wait data whitespace
    	if (_port->available() > _len) {
        	_len = _port->available();
        	t = millis();
		}
		if (millis() - taskStart > MODBUSRTU_MAX_READMS) { // Prevent from task() executed too long
			#if defined(ESP32)
    		//taskEXIT_CRITICAL(&mux);
			//xTaskResumeAll();
 			#endif
			return;
		}
	}
	#if defined(ESP32)
    //taskEXIT_CRITICAL(&mux);
	//xTaskResumeAll();
 	#endif

    uint8_t address = _port->read(); //first byte of frame = address
    _len--; // Decrease by slaveId byte
//    if (address != MODBUSRTU_BROADCAST && address != _slaveId) {     // SlaveId Check
//        for (uint8_t i=0 ; i < _len ; i++) _port->read();   // Skip packet if SlaveId doesn't mach
//        _len = 0;
//		if (isMaster) cleanup();
//        return;
//    }
Serial.printf("got %d bytes\n", _len);
	free(_frame);	//Just in case
    _frame = (uint8_t*) malloc(_len);
    if (!_frame) {  // Fail to allocate buffer
      for (uint8_t i=0 ; i < _len ; i++) _port->read(); // Skip packet if can't allocate buffer
      _len = 0;
      return;
    }
    for (uint8_t i=0 ; i < _len ; i++) {
		_frame[i] = _port->read();   // read data + crc
		#if defined(MODBUSRTU_DEBUG)
		Serial.print(_frame[i], HEX);
		Serial.print(" ");
		#endif
	}
	#if defined(MODBUSRTU_DEBUG)
	Serial.println();
	#endif
	//_port->readBytes(_frame, _len);
    uint16_t frameCrc = ((_frame[_len - 2] << 8) | _frame[_len - 1]); // Last two byts = crc
    _len = _len - 2;    // Decrease by CRC 2 bytes
    if (frameCrc != crc16(address, _frame, _len)) {  // CRC Check
        free(_frame);
        _frame = nullptr;
		_len = 0;
        return;
    }
	
	if (address == mapping.slaveId) {
		if (!tcp->isConnected(mapping.ip)) {
			if (!tcp->connect(mapping.ip))
				return;
		}
		transWaiting = tcp->writeRaw(mapping.ip, _frame, _len, cbBri, mapping.unitId);
		idWaiting = address;
//		return;
	}

/*    slavePDU(_frame);
    if (address == MODBUSRTU_BROADCAST)
		_reply = Modbus::REPLY_OFF;    // No reply for Broadcasts
    if (_reply != Modbus::REPLY_OFF)
		rawSend(_slaveId, _frame, _len);
*/
    // Cleanup
    free(_frame);
    _frame = nullptr;
    _len = 0;
}