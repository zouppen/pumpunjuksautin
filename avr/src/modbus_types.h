#pragma once

// Modbus types. Can be used without full Modbus support in return
// values etc.

typedef enum {
	COIL = 'c',
	DISCRETE_INPUT = 'd',
	INPUT_REGISTER = 'i',
	HOLDING_REGISTER = 'h',
} modbus_object_t;

// The codes match Modbus exception responses.
// https://libmodbus.org/docs/v3.1.6/modbus_reply_exception.html
// https://en.wikipedia.org/wiki/Modbus#Exception_responses
typedef enum {
	MODBUS_OK,
	MODBUS_EXCEPTION_ILLEGAL_FUNCTION = 1,
	MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS = 2,
	MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE = 3,
	MODBUS_EXCEPTION_SLAVE_OR_SERVER_FAILURE = 4,
	MODBUS_EXCEPTION_ACKNOWLEDGE = 5,
	MODBUS_EXCEPTION_SLAVE_OR_SERVER_BUSY = 6,
	MODBUS_EXCEPTION_NEGATIVE_ACKNOWLEDGE = 7,
	MODBUS_EXCEPTION_MEMORY_PARITY = 8,
	MODBUS_EXCEPTION_NOT_DEFINED = 9,
	MODBUS_EXCEPTION_GATEWAY_PATH = 10,
	MODBUS_EXCEPTION_GATEWAY_TARGET = 11,
} modbus_status_t;
