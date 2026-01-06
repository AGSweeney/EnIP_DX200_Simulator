# Motoman CIP Classes - Hex to Decimal Conversion

## Class Number Conversions

| Hex | Decimal | Class Name | Description |
|-----|---------|------------|-------------|
| 0x70 | **112** | MotomanAlarm | Active Alarms |
| 0x71 | **113** | MotomanAlarmHistory | Alarm History |
| 0x72 | **114** | MotomanStatus | Robot Status |
| 0x73 | **115** | MotomanJobInfo | Job Information |
| 0x74 | **116** | MotomanAxisConfig | Axis Configuration |
| 0x75 | **117** | MotomanPosition | Robot Position |
| 0x76 | **118** | MotomanPositionDeviation | Position Deviation |
| 0x77 | **119** | MotomanTorque | Axis Torque |
| 0x78 | **120** | MotomanIO | I/O Signals |
| 0x79 | **121** | MotomanRegister | Registers |
| 0x7A | **122** | MotomanVariableB | Byte Variables |
| 0x7B | **123** | MotomanVariableI | Integer Variables |
| 0x7C | **124** | MotomanVariableD | Double Integer Variables |
| 0x7D | **125** | MotomanVariableR | Real Variables |
| 0x7F | **127** | MotomanVariableP | Position Variables |
| 0x80 | **128** | MotomanVariableBP | Base Position Variables |
| 0x81 | **129** | MotomanVariableEX | External Axis Variables |
| 0x8C | **140** | MotomanVariableS | String Variables |

## Standard CIP Classes

| Hex | Decimal | Class Name | Description |
|-----|---------|------------|-------------|
| 0x01 | **1** | Identity Object | Device identification |
| 0x02 | **2** | Message Router | CIP message routing |
| 0x04 | **4** | Assembly Object | I/O assembly data |
| 0x05 | **5** | Connection Manager | Connection management |
| 0x06 | **6** | Connection Object | Connection configuration |
| 0xF5 | **245** | TCP/IP Interface | Network configuration |

## Quick Reference for EtherNet/IP Explorer

When using EtherNet/IP Explorer or other tools that require decimal class numbers:

**Motoman Classes (Decimal)**:
- Class **112** = Active Alarms
- Class **113** = Alarm History
- Class **114** = Robot Status
- Class **115** = Job Information
- Class **116** = Axis Configuration
- Class **117** = Robot Position
- Class **118** = Position Deviation
- Class **119** = Axis Torque
- Class **120** = I/O Signals
- Class **121** = Registers
- Class **122** = Byte Variables (B)
- Class **123** = Integer Variables (I)
- Class **124** = Double Integer Variables (D)
- Class **125** = Real Variables (R)
- Class **127** = Position Variables (P)
- Class **128** = Base Position Variables (BP)
- Class **129** = External Axis Variables (EX)
- Class **140** = String Variables (S)

## Example Usage

**In EtherNet/IP Explorer (if it uses decimal)**:
- Browse to Class **114** (Status), Instance **1**, Attribute **1**
- Browse to Class **120** (I/O), Instance **1**, Attribute **1**
- Browse to Class **121** (Register), Instance **1**, Attribute **1**

**In CIP Path Encoding (hex)**:
- Class 0x72 = Class **114** (decimal)
- Class 0x78 = Class **120** (decimal)
- Class 0x79 = Class **121** (decimal)

## Conversion Formula

To convert hex to decimal manually:
- 0x70 = 7×16 + 0 = 112
- 0x71 = 7×16 + 1 = 113
- 0x72 = 7×16 + 2 = 114
- ...and so on

Or use: `hex_value = decimal_value` (just remove the 0x prefix and convert)

