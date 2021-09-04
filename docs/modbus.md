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

Registers are read-write.

|  Address | Registers | Read | Write | Data type | Unit   | Description                |
|---------:|----------:|:----:|:-----:|-----------|--------|----------------------------|
| `0x0001` |         1 | ✓    |       | int16     | 0.01°C | Tank temperature           |
| `0x0002` |         1 | ✓    |       | int16     | 0.01°C | Outside temperature        |
| `0x0001` |         1 | ✓    | ✓     | int16     | mV     | Juksautus target voltage   |
| `0x0010` |        16 | ✓    |       | char[]    |        | Version string, NUL padded |
