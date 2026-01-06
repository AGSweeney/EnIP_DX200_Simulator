# Motoman DX200 Simulator - CIP Classes Reference

This document provides a complete reference of all CIP classes, instances, and attributes available on the Motoman DX200 Simulator for use with EtherNet/IP Explorer or other CIP tools.

## Device Information

- **Vendor ID**: 44 (Yaskawa Electric America, Inc.)
- **Device Type**: 12 (Communications Adapter)
- **Product Code**: 1281
- **Product Name**: "DX200 EtherNet/IP Module"
- **Revision**: 1.1

## Standard CIP Classes

### Class 0x01 - Identity Object
- **Instance 1**:
  - Attribute 1: Vendor ID (UINT) = 44
  - Attribute 2: Device Type (UINT) = 12
  - Attribute 3: Product Code (UINT) = 1281
  - Attribute 4: Revision (STRUCT: Major.Minor) = 1.1
  - Attribute 5: Status (WORD) = 0x0000
  - Attribute 6: Serial Number (UDINT) = 123456789
  - Attribute 7: Product Name (STRING) = "DX200 EtherNet/IP Module"

## Motoman Vendor-Specific CIP Classes

### Class Number Quick Reference (Hex to Decimal)

| Hex | Decimal | Class Name |
|-----|---------|------------|
| 0x70 | **112** | MotomanAlarm |
| 0x71 | **113** | MotomanAlarmHistory |
| 0x72 | **114** | MotomanStatus |
| 0x73 | **115** | MotomanJobInfo |
| 0x74 | **116** | MotomanAxisConfig |
| 0x75 | **117** | MotomanPosition |
| 0x76 | **118** | MotomanPositionDeviation |
| 0x77 | **119** | MotomanTorque |
| 0x78 | **120** | MotomanIO |
| 0x79 | **121** | MotomanRegister |
| 0x7A | **122** | MotomanVariableB |
| 0x7B | **123** | MotomanVariableI |
| 0x7C | **124** | MotomanVariableD |
| 0x7D | **125** | MotomanVariableR |
| 0x7F | **127** | MotomanVariableP |
| 0x80 | **128** | MotomanVariableBP |
| 0x81 | **129** | MotomanVariableEX |
| 0x8C | **140** | MotomanVariableS |

---

### Class 0x70 (112 decimal) - MotomanAlarm (Active Alarms)
**Instances**: 1-4 (4 instances total)
**Services**: Get_Attribute_Single (0x0E)

Each instance represents one active alarm:
- **Instance 1-4**:
  - Attribute 1: Alarm Code (UDINT) - Read Only
  - Attribute 2: Alarm Subcode (UDINT) - Read Only

**Example Paths**:
- Class 0x70, Instance 1, Attribute 1: `[0x20 0x70] [0x24 0x01] [0x30 0x01]`
- Class 0x70, Instance 2, Attribute 2: `[0x20 0x70] [0x24 0x02] [0x30 0x02]`

---

### Class 0x71 (113 decimal) - MotomanAlarmHistory (Alarm History)
**Instances**: 1-100 (100 instances total)
**Services**: Get_Attribute_Single (0x0E)

Each instance represents one historical alarm entry:
- **Instance 1-100**:
  - Attribute 1: Alarm Code (UDINT) - Read Only
  - Attribute 2: Alarm Subcode (UDINT) - Read Only

**Example Paths**:
- Class 0x71, Instance 1, Attribute 1: `[0x20 0x71] [0x24 0x01] [0x30 0x01]`
- Class 0x71, Instance 50, Attribute 2: `[0x20 0x71] [0x24 0x32] [0x30 0x02]`

---

### Class 0x72 (114 decimal) - MotomanStatus (Robot Status)
**Instances**: 1 (single instance)
**Services**: Get_Attribute_Single (0x0E), Get_Attribute_All (0x01)

- **Instance 1**:
  - Attribute 1: Status Data 1 (UDINT) - Read Only
    - bit 0: Step
    - bit 1: 1 cycle
    - bit 2: Auto
    - bit 3: Running
    - bit 4: Safety speed operation
    - bit 5: Teach
    - bit 6: Play
    - bit 7: Command remote
  - Attribute 2: Status Data 2 (UDINT) - Read Only
    - bit 1: Hold (Programming pendant)
    - bit 2: Hold (external)
    - bit 3: Hold (Command)
    - bit 4: Alarm
    - bit 5: Error
    - bit 6: Servo on

**Example Paths**:
- Class 0x72, Instance 1, Attribute 1: `[0x20 0x72] [0x24 0x01] [0x30 0x01]`
- Class 0x72, Instance 1, Attribute 2: `[0x20 0x72] [0x24 0x01] [0x30 0x02]`
- Class 0x72, Instance 1, Get_Attribute_All: `[0x20 0x72] [0x24 0x01] [0x01]`

---

### Class 0x73 (115 decimal) - MotomanJobInfo (Job Information)
**Instances**: 1 (single instance)
**Services**: Get_Attribute_Single (0x0E)

- **Instance 1**:
  - Attribute 1: Job Line (UDINT) - Read Only
  - Attribute 2: Step Number (UDINT) - Read Only
  - Attribute 3: Job Name (BYTE array, 32 bytes) - Read Only
  - Attribute 4: Speed Override (REAL) - Read Only

**Example Paths**:
- Class 0x73, Instance 1, Attribute 1: `[0x20 0x73] [0x24 0x01] [0x30 0x01]`
- Class 0x73, Instance 1, Attribute 4: `[0x20 0x73] [0x24 0x01] [0x30 0x04]`

---

### Class 0x74 (116 decimal) - MotomanAxisConfig (Axis Configuration)
**Instances**: 1 (single instance)
**Services**: Get_Attribute_Single (0x0E)

- **Instance 1**:
  - Attribute 1: Axis Count (USINT) - Read Only
  - Attribute 2: Axis Type (BYTE array, 8 bytes) - Read Only

**Example Paths**:
- Class 0x74, Instance 1, Attribute 1: `[0x20 0x74] [0x24 0x01] [0x30 0x01]`
- Class 0x74, Instance 1, Attribute 2: `[0x20 0x74] [0x24 0x01] [0x30 0x02]`

---

### Class 0x75 (117 decimal) - MotomanPosition (Robot Position)
**Instances**: 1-8 (8 instances, one per axis)
**Services**: Get_Attribute_Single (0x0E)

Each instance represents one robot axis:
- **Instance 1-8**:
  - Attribute 1: Position (DINT) - Read Only

**Example Paths**:
- Class 0x75, Instance 1, Attribute 1: `[0x20 0x75] [0x24 0x01] [0x30 0x01]` (S-axis)
- Class 0x75, Instance 2, Attribute 1: `[0x20 0x75] [0x24 0x02] [0x30 0x01]` (L-axis)
- Class 0x75, Instance 6, Attribute 1: `[0x20 0x75] [0x24 0x06] [0x30 0x01]` (T-axis)

---

### Class 0x76 (118 decimal) - MotomanPositionDeviation (Position Deviation)
**Instances**: 1-8 (8 instances, one per axis)
**Services**: Get_Attribute_Single (0x0E)

Each instance represents position deviation for one axis:
- **Instance 1-8**:
  - Attribute 1: Position Deviation (DINT) - Read Only

**Example Paths**:
- Class 0x76, Instance 1, Attribute 1: `[0x20 0x76] [0x24 0x01] [0x30 0x01]`
- Class 0x76, Instance 3, Attribute 1: `[0x20 0x76] [0x24 0x03] [0x30 0x01]`

---

### Class 0x77 (119 decimal) - MotomanTorque (Axis Torque)
**Instances**: 1-8 (8 instances, one per axis)
**Services**: Get_Attribute_Single (0x0E)

Each instance represents torque for one axis:
- **Instance 1-8**:
  - Attribute 1: Torque (DINT, scaled by 1000) - Read Only
    - Example: 18500 = 18.5%

**Example Paths**:
- Class 0x77, Instance 1, Attribute 1: `[0x20 0x77] [0x24 0x01] [0x30 0x01]`
- Class 0x77, Instance 4, Attribute 1: `[0x20 0x77] [0x24 0x04] [0x30 0x01]`

---

### Class 0x78 (120 decimal) - MotomanIO (I/O Signals)
**Instances**: 1-512 (512 instances total)
**Services**: Get_Attribute_Single (0x0E), Set_Attribute_Single (0x10)

Each instance represents one I/O signal:
- **Instance 1-512**:
  - Attribute 1: I/O Value (USINT) - Read/Write

**Note**: Instance number corresponds to signal number divided by 10. For example:
- Instance 1 = Signal 00010
- Instance 2 = Signal 00020
- Instance 100 = Signal 10010 (General Output)
- Instance 200 = Signal 20010 (External Input)

**Example Paths**:
- Class 0x78, Instance 1, Attribute 1: `[0x20 0x78] [0x24 0x01] [0x30 0x01]` (Read)
- Class 0x78, Instance 1, Attribute 1: `[0x20 0x78] [0x24 0x01] [0x30 0x01]` (Write with Set_Attribute_Single)

---

### Class 0x79 (121 decimal) - MotomanRegister (Registers)
**Instances**: 1-1000 (1000 instances total, M000-M999)
**Services**: Get_Attribute_Single (0x0E), Set_Attribute_Single (0x10)

Each instance represents one register:
- **Instance 1-1000**:
  - Attribute 1: Register Value (UINT) - Read/Write

**Note**: Instance number corresponds to register number:
- Instance 1 = M000
- Instance 2 = M001
- Instance 100 = M099
- Instance 1000 = M999

**Example Paths**:
- Class 0x79, Instance 1, Attribute 1: `[0x20 0x79] [0x24 0x01] [0x30 0x01]` (M000)
- Class 0x79, Instance 250, Attribute 1: `[0x20 0x79] [0x24 0xFA] [0x30 0x01]` (M249)

---

### Class 0x7A (122 decimal) - MotomanVariableB (Byte Variables)
**Instances**: 1-1000 (1000 instances total)
**Services**: Get_Attribute_Single (0x0E), Set_Attribute_Single (0x10)

Each instance represents one byte variable:
- **Instance 1-1000**:
  - Attribute 1: Variable Value (USINT) - Read/Write

**Example Paths**:
- Class 0x7A, Instance 1, Attribute 1: `[0x20 0x7A] [0x24 0x01] [0x30 0x01]` (B000)
- Class 0x7A, Instance 100, Attribute 1: `[0x20 0x7A] [0x24 0x64] [0x30 0x01]` (B099)

---

### Class 0x7B (123 decimal) - MotomanVariableI (Integer Variables)
**Instances**: 1-1000 (1000 instances total)
**Services**: Get_Attribute_Single (0x0E), Set_Attribute_Single (0x10)

Each instance represents one integer variable:
- **Instance 1-1000**:
  - Attribute 1: Variable Value (INT) - Read/Write

**Example Paths**:
- Class 0x7B, Instance 1, Attribute 1: `[0x20 0x7B] [0x24 0x01] [0x30 0x01]` (I000)
- Class 0x7B, Instance 500, Attribute 1: `[0x20 0x7B] [0x24 0x01F4] [0x30 0x01]` (I499)

---

### Class 0x7C (124 decimal) - MotomanVariableD (Double Integer Variables)
**Instances**: 1-1000 (1000 instances total)
**Services**: Get_Attribute_Single (0x0E), Set_Attribute_Single (0x10)

Each instance represents one double integer variable:
- **Instance 1-1000**:
  - Attribute 1: Variable Value (DINT) - Read/Write

**Example Paths**:
- Class 0x7C, Instance 1, Attribute 1: `[0x20 0x7C] [0x24 0x01] [0x30 0x01]` (D000)
- Class 0x7C, Instance 999, Attribute 1: `[0x20 0x7C] [0x24 0x03E7] [0x30 0x01]` (D998)

---

### Class 0x7D (125 decimal) - MotomanVariableR (Real Variables)
**Instances**: 1-1000 (1000 instances total)
**Services**: Get_Attribute_Single (0x0E), Set_Attribute_Single (0x10)

Each instance represents one real/float variable:
- **Instance 1-1000**:
  - Attribute 1: Variable Value (REAL) - Read/Write

**Example Paths**:
- Class 0x7D, Instance 1, Attribute 1: `[0x20 0x7D] [0x24 0x01] [0x30 0x01]` (R000)
- Class 0x7D, Instance 50, Attribute 1: `[0x20 0x7D] [0x24 0x32] [0x30 0x01]` (R049)

---

### Class 0x8C (140 decimal) - MotomanVariableS (String Variables)
**Instances**: 1-1000 (1000 instances total)
**Services**: Get_Attribute_Single (0x0E), Set_Attribute_Single (0x10)

Each instance represents one string variable:
- **Instance 1-1000**:
  - Attribute 1: Variable Value (STRING, max 32 characters) - Read/Write

**Example Paths**:
- Class 0x8C, Instance 1, Attribute 1: `[0x20 0x8C] [0x24 0x01] [0x30 0x01]` (S000)
- Class 0x8C, Instance 10, Attribute 1: `[0x20 0x8C] [0x24 0x0A] [0x30 0x01]` (S009)

---

### Class 0x7F (127 decimal) - MotomanVariableP (Position Variables)
**Instances**: 1-1000 (1000 instances total)
**Services**: Get_Attribute_Single (0x0E), Set_Attribute_Single (0x10)

Each instance represents one position variable (8 axes):
- **Instance 1-1000**:
  - Attribute 1: Position Value (BYTE array, 8 DINT values = 32 bytes) - Read/Write

**Example Paths**:
- Class 0x7F, Instance 1, Attribute 1: `[0x20 0x7F] [0x24 0x01] [0x30 0x01]` (P000)
- Class 0x7F, Instance 5, Attribute 1: `[0x20 0x7F] [0x24 0x05] [0x30 0x01]` (P004)

---

### Class 0x80 (128 decimal) - MotomanVariableBP (Base Position Variables)
**Instances**: 1-1000 (1000 instances total)
**Services**: Get_Attribute_Single (0x0E), Set_Attribute_Single (0x10)

Each instance represents one base position variable (8 axes):
- **Instance 1-1000**:
  - Attribute 1: Base Position Value (BYTE array, 8 DINT values = 32 bytes) - Read/Write

**Example Paths**:
- Class 0x80, Instance 1, Attribute 1: `[0x20 0x80] [0x24 0x01] [0x30 0x01]` (BP000)
- Class 0x80, Instance 20, Attribute 1: `[0x20 0x80] [0x24 0x14] [0x30 0x01]` (BP019)

---

### Class 0x81 (129 decimal) - MotomanVariableEX (External Axis Variables)
**Instances**: 1-1000 (1000 instances total)
**Services**: Get_Attribute_Single (0x0E), Set_Attribute_Single (0x10)

Each instance represents one external axis variable (8 axes):
- **Instance 1-1000**:
  - Attribute 1: External Axis Value (BYTE array, 8 DINT values = 32 bytes) - Read/Write

**Example Paths**:
- Class 0x81, Instance 1, Attribute 1: `[0x20 0x81] [0x24 0x01] [0x30 0x01]` (EX000)
- Class 0x81, Instance 100, Attribute 1: `[0x20 0x81] [0x24 0x64] [0x30 0x01]` (EX099)

---

## CIP Path Encoding Reference

CIP paths use the following segment types:
- `0x20` = Class segment (followed by 2-byte class code)
- `0x24` = Instance segment (followed by 2-byte instance number)
- `0x30` = Attribute segment (followed by 1-byte attribute number)

**Example**: Class 0x72, Instance 1, Attribute 1
```
Path: [0x20 0x72 0x00] [0x24 0x01 0x00] [0x30 0x01]
```

## CIP Services

- **0x01** = Get_Attribute_All (read all attributes of an instance)
- **0x0E** = Get_Attribute_Single (read a single attribute)
- **0x10** = Set_Attribute_Single (write a single attribute)

## Testing with EtherNet/IP Explorer

1. **Connect to Device**: IP address from boot log (e.g., 172.16.82.110)

2. **Browse Classes**: You should see all 18 Motoman classes (0x70-0x81) plus standard classes

3. **Test Read Operations**:
   - Class 0x72, Instance 1, Get_Attribute_All → Should return Status Data 1 and 2
   - Class 0x75, Instance 1, Attribute 1 → Should return S-axis position
   - Class 0x78, Instance 1, Attribute 1 → Should return I/O signal value

4. **Test Write Operations**:
   - Class 0x78, Instance 1, Attribute 1, Set_Attribute_Single → Write I/O value
   - Class 0x79, Instance 1, Attribute 1, Set_Attribute_Single → Write register M000
   - Class 0x7A, Instance 1, Attribute 1, Set_Attribute_Single → Write variable B000

## Pre-populated Data

The simulator includes realistic pre-populated data:
- **Status**: Auto mode, Play mode, Servo On
- **Job Info**: Job name "MAIN", line 1, step 0, speed override 100%
- **Position**: All axes at 0 (home position)
- **Torque**: Realistic values (18.5%, 22.3%, 31.2%, etc.)
- **I/O**: Some signals set to ON (instances 1, 3, 4, 10, 100, 102, 200)
- **Registers**: M000=100, M001=250, M002=500, M010=1234, M020=5678
- **Variables**: Various pre-populated values

## Notes

- All instances are numbered starting from 1 (CIP standard)
- Instance numbers are assigned sequentially by OpENer
- Read-only classes: 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77
- Read/Write classes: 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x8C, 0x7F, 0x80, 0x81
- Default class attributes (1-7) are automatically added by OpENer for all classes

