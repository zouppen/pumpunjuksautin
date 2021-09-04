# Modbus register table

**THIS IS A DRAFT**

## Coils

No coils.

## Discrete inputs



| Address | False       | True      | Description                     |
|---------|-------------|-----------|---------------------------------|
| 0x0001  | LOW         | HIGH      | Error LED status                |
| 0x0002  | Clock unset | Clock set | Internal real-time clock status |

## Input registers

Every register is read-only.

| Address | Registers | Data type | Unit   | Description                |
|--------:|----------:|-----------|--------|----------------------------|
|  0x0001 |         1 | int16     | 0.01°C | Tank temperature           |
|  0x0002 |         1 | int16     | 0.01°C | Outside temperature        |
|  0x0010 |        16 | char[]    | -      | Version string, NUL padded |

## Holding registers

Registers are read-write.

| Address | Registers | Data type | Unit | Description              |
|---------|----------:|-----------|------|--------------------------|
| 0x0001  |         1 | int16     | mV   | Juksautus target voltage |

