# Bridge functions

## Basic

Basic 'Bridge'. Indeed this sample pulling data from Modbus Server and stores it to local registers. Local registers can be accessed via Modbus Client instance that running aside.

## True

Fullfunctional ModbusRTU to ModbusTCP bridge.

```c
uint16_t rawRequest(id_ip, uint8_t* data, uint16_t len, cbTransaction cb = nullptr, uint8_t unit = MODBUSIP_UNIT);
uint16_t rawResponce(id_ip, uint8_t* data, uint16_t len, uint8_t unit = MODBUSIP_UNIT);
uint16_t errorResponce(id_ip, Modbus::FunctionCode fn, Modbus::ResultCode excode, uint8_t unit = MODBUSIP_UNIT);
```
- `id_ip` SlaveId (`uint8_t`) or server IP address (`IPAddress`)
- `data` Pointer to data buffer to send
- `len` Byte count to send
- `unit` UnitId (ModbusTCP/TLS only)
- `fn` function code in responce
- `excode` Exceprion code in responce

```c
union frame_arg_t {
    uint8_t slaveId;
    struct {
	    uint8_t unitId;
	    uint32_t ipaddr;
	    uint16_t transactionId = 0;
    };
};
typedef std::function<ResultCode(uint8_t*, uint8_t, void*)> cbRaw; // Callback function Type for STL
typedef ResultCode (*cbRaw)(uint8_t* frame, uint8 len, void* data); // Callback function Type
bool onRaw(cbRaw cb = nullptr);
```
- `frame` Modbus payload frame with stripped MBAP, slaveid and crc
- `len' frame size in bytes
- `data` Pointer to frame_arg_t filled with frame header information