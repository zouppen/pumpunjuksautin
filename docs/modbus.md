# Modbus register table

**THIS IS A DRAFT!**

## Coils

Coils are read-write.

|  Address | False   | True   | Description                               |
|---------:|---------|--------|-------------------------------------------|
| `0x0001` | LED off | LED on | Indicator LED control (for testing comms) |

## Input bits

Every input is read-only.

|  Address | False       | True      | Description                     |
|---------:|-------------|-----------|---------------------------------|
| `0x0001` | LED off     | LED on    | Error LED status                |
| `0x0002` | Clock unset | Clock set | Internal real-time clock status |

## Holding registers

A single register is 16-bit value. All values spanning multiple
registers have big-endian byte order.

| Address | Size | Read | Write | Data type | Unit   | Description                                   |
|---------|-----:|:----:|:-----:|-----------|--------|-----------------------------------------------|
| TBD     |   16 | X    |       | char[32]  |        | Firmware version string, NUL padded           |
| TBD     |    2 | X    | X     | uint32    | unix   | Current system time                           |
| TBD     |    2 | X    | X     | int32     | gmtoff | Current UTC offset                            |
| TBD     |    2 | X    | X     | uint32    | unix   | Time of next UTC offset change. 0 if not set. |
| TBD     |    2 | X    | X     | int32     | gmtoff | Next UTC offset                               |
| TBD     |    1 | X    |       | int16     | 0.01°C | Tank temperature                              |
| TBD     |    1 | X    |       | int16     | 0.01°C | Outside temperature                           |
| TBD     |    1 | X    | X     | int16     | mV     | Juksautus target voltage                      |

## Legend

* **size**: Data length in number of registers (`nb` in libmodbus)
* **unix**: Number of seconds since UNIX epoch
* **gmtoff**: UTC offset in seconds
