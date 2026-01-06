# Pre-Initialized Data Reference

This document lists all pre-initialized variables and data values in the Motoman DX200 Simulator. All data is initialized in the `InitializeRobotData()` function in `motoman_dx200_simulator.c`.

## Status Data (Class 0x72)

### Status Data 1 (Attribute 1)
- **Value**: `0x00000044` (68 decimal)
- **Description**: Robot status bits

### Status Data 2 (Attribute 2)
- **Value**: `0x00000040` (64 decimal)
- **Description**: Additional status bits

## Job Info (Class 0x73)

### Job Name (Attribute 1)
- **Value**: `"WELD001.JBI"` (32-byte string, null-terminated)
- **Description**: Current job name

### Job Line (Attribute 2)
- **Value**: `15` (UDINT)
- **Description**: Current line number in job

### Step Number (Attribute 3)
- **Value**: `42` (UDINT)
- **Description**: Current step number (valid range: 1-9998)

### Speed Override (Attribute 4)
- **Value**: `8550` (UDINT)
- **Description**: Speed override in units of 0.01% (8550 = 85.5%)

## Axis Configuration (Class 0x74)

### Axis Count
- **Value**: `6`
- **Description**: Number of axes (6-axis robot)

### Axis Type Array (8 elements)
- **Index 0 (S-axis)**: `1` (Base rotation)
- **Index 1 (L-axis)**: `1` (Lower arm)
- **Index 2 (U-axis)**: `1` (Upper arm)
- **Index 3 (R-axis)**: `1` (Wrist roll)
- **Index 4 (B-axis)**: `1` (Wrist bend)
- **Index 5 (T-axis)**: `1` (Wrist twist)
- **Index 6**: `0` (Unused)
- **Index 7**: `0` (Unused)

## Position (Class 0x75)

### Instance 1 (Robot Pulse, Control Group 1)
- **Data Type (Attr 1)**: `0` (Pulse)
- **Axis 1 (S) (Attr 2)**: `1250` pulses (encoder counts)
- **Axis 2 (L) (Attr 3)**: `-15230` pulses (encoder counts)
- **Axis 3 (U) (Attr 4)**: `28340` pulses (encoder counts)
- **Axis 4 (R) (Attr 5)**: `-450` pulses (encoder counts)
- **Axis 5 (B) (Attr 6)**: `8920` pulses (encoder counts)
- **Axis 6 (T) (Attr 7)**: `-120` pulses (encoder counts)
- **Axis 7 (Attr 8)**: `0`
- **Axis 8 (Attr 9)**: `0`
- **Configuration (Attr 10)**: `0`
- **Tool Number (Attr 11)**: `0`
- **Reservation (Attr 12)**: `0`
- **Extended Config (Attr 13)**: `0`

### Instance 102 (Robot Base, Control Group 1)
- **Data Type (Attr 1)**: `16` (Base coordinates)
- **X Position (Attr 2)**: `1234567` (1234.567 mm in micrometers)
- **Y Position (Attr 3)**: `2345678` (2345.678 mm in micrometers)
- **Z Position (Attr 4)**: `3456789` (3456.789 mm in micrometers)
- **RX Rotation (Attr 5)**: `0` (angle in 0.0001° units)
- **RY Rotation (Attr 6)**: `1234567` (X Position Robot Base, in μm)
- **RZ Rotation (Attr 7)**: `0` (angle in 0.0001° units)
- **Axis 7 (Attr 8)**: `0`
- **Axis 8 (Attr 9)**: `0`
- **Configuration (Attr 10)**: `0`
- **Tool Number (Attr 11)**: `0`
- **Reservation (Attr 12)**: `0`
- **Extended Config (Attr 13)**: `0`

**Note**: All other instances (2-8, 11-18, 21-44, 103-108) are initialized to zero.

## Position Deviation (Class 0x76)

### Instance 1 (Single instance with 8 attributes)
- **Axis 1 (S) (Attr 1)**: `2` pulses deviation
- **Axis 2 (L) (Attr 2)**: `-3` pulses deviation
- **Axis 3 (U) (Attr 3)**: `1` pulse deviation
- **Axis 4 (R) (Attr 4)**: `0` (no deviation)
- **Axis 5 (B) (Attr 5)**: `-1` pulse deviation
- **Axis 6 (T) (Attr 6)**: `1` pulse deviation
- **Axis 7 (Attr 7)**: `0` (unused)
- **Axis 8 (Attr 8)**: `0` (unused)

## Torque (Class 0x77)

### Instance 1 (Single instance with 8 attributes)
- **Axis 1 (S) (Attr 1)**: `18500` (18.5% when divided by 1000)
- **Axis 2 (L) (Attr 2)**: `22300` (22.3% when divided by 1000)
- **Axis 3 (U) (Attr 3)**: `31200` (31.2% when divided by 1000)
- **Axis 4 (R) (Attr 4)**: `4500` (4.5% when divided by 1000)
- **Axis 5 (B) (Attr 5)**: `12800` (12.8% when divided by 1000)
- **Axis 6 (T) (Attr 6)**: `2100` (2.1% when divided by 1000)
- **Axis 7 (Attr 7)**: `0`
- **Axis 8 (Attr 8)**: `0`

**Note**: Values are stored as integers scaled by 1000 for 0.1% precision.

## I/O Data (Class 0x78)

### General Input Signals (Instances 1-512, Signals 00010-05127)
- **Instance 1 (Signal 00010)**: `0x01` (ON)
- **Instance 2 (Signal 00020)**: `0x00` (OFF)
- **Instance 3 (Signal 00030)**: `0x01` (ON)
- **Instance 4 (Signal 00040)**: `0x01` (ON)
- **Instance 11 (Signal 00110)**: `0x01` (ON)
- **Instance 51 (Signal 00510)**: `0x00` (OFF)

### General Output Signals (Instances 1001-1512, Signals 10010-15127)
- **Instance 1001 (Signal 10010)**: `0x01` (ON)
- **Instance 1002 (Signal 10020)**: `0x00` (OFF)
- **Instance 1003 (Signal 10030)**: `0x01` (ON)

### External Input Signals (Instances 2001-2512, Signals 20010-25127)
- **Instance 2001 (Signal 20010)**: `0x01` (ON)
- **Instance 2002 (Signal 20020)**: `0x00` (OFF)

**Note**: All other I/O signals are initialized to `0x00` (OFF). Maximum instances: 8220.

## Registers (Class 0x79)

### General Registers (M000-M559)
- **M000**: `100` (UINT16)
- **M001**: `250` (UINT16)
- **M002**: `500` (UINT16)
- **M010**: `1234` (UINT16)
- **M020**: `5678` (UINT16)
- **M100**: `9999` (UINT16)
- **M559**: `0` (UINT16)

### Analog Output Registers (M560-M599)
- **M560**: `32767` (UINT16, max value)
- **M599**: `0` (UINT16)

### Analog Input Registers (M600-M639, Read-Only)
- **M600**: `16384` (UINT16)
- **M639**: `0` (UINT16)

### System Registers (M640-M999, Read-Only)
- **M640**: `0` (UINT16)
- **M999**: `0` (UINT16)

**Note**: All other registers are initialized to `0`. Maximum registers: 1000 (M000-M999).

## Variable B (Byte Variables, Class 0x7A)

- **B[0]**: `10` (UINT8)
- **B[1]**: `20` (UINT8)
- **B[2]**: `30` (UINT8)
- **B[10]**: `100` (UINT8)

**Note**: All other Variable B values are initialized to `0`. Maximum variables: 1000.

## Variable I (Integer Variables, Class 0x7B)

- **I[0]**: `1234` (INT16)
- **I[1]**: `-567` (INT16)
- **I[2]**: `8901` (INT16)
- **I[10]**: `42` (INT16)

**Note**: All other Variable I values are initialized to `0`. Maximum variables: 1000.

## Variable D (Double Integer Variables, Class 0x7C)

- **D[0]**: `123456` (INT32)
- **D[1]**: `-789012` (INT32)
- **D[2]**: `345678` (INT32)
- **D[10]**: `999999` (INT32)

**Note**: All other Variable D values are initialized to `0`. Maximum variables: 1000.

## Variable R (Real/Float Variables, Class 0x7D)

- **R[0]**: `123.456f` (REAL)
- **R[1]**: `-45.678f` (REAL)
- **R[2]**: `789.012f` (REAL)
- **R[10]**: `3.14159f` (REAL)

**Note**: All other Variable R values are initialized to `0.0f`. Maximum variables: 1000.

## Variable S (String Variables, Class 0x8C)

- **S[0]**: `"HELLO"` (32-byte fixed array)
- **S[1]**: `"WORLD"` (32-byte fixed array)
- **S[2]**: `"ROBOT"` (32-byte fixed array)

**Note**: All other Variable S values are initialized to empty strings (null-terminated). Maximum variables: 1000.

## Variable P (Position Variables, Class 0x7F)

### P[0] - Home Position (Pulse Type)
- **Data Type (Attr 1)**: `0` (Pulse)
- **Axis 1 (S) (Attr 2)**: `1`
- **Axis 2 (L) (Attr 3)**: `0`
- **Axis 3 (U) (Attr 4)**: `0`
- **Axis 4 (R) (Attr 5)**: `0`
- **Axis 5 (B) (Attr 6)**: `0`
- **Axis 6 (T) (Attr 7)**: `5`
- **Axis 7 (Attr 8)**: `0`
- **Axis 8 (Attr 9)**: `0`
- **Configuration (Attr 10)**: `0`
- **Tool Number (Attr 11)**: `0`
- **User Coordinate (Attr 12)**: `0`
- **Extended Config (Attr 13)**: `0`

### P[1] - Example Position (Base Type)
- **Data Type (Attr 1)**: `16` (Base coordinates)
- **X Position (Attr 2)**: `500000` (500.000 mm in micrometers)
- **Y Position (Attr 3)**: `300000` (300.000 mm in micrometers)
- **Z Position (Attr 4)**: `1200000` (1200.000 mm in micrometers)
- **RX Rotation (Attr 5)**: `900000` (90.0000° in 0.0001° units)
- **RY Rotation (Attr 6)**: `180000` (18.0000° in 0.0001° units)
- **RZ Rotation (Attr 7)**: `450000` (45.0000° in 0.0001° units)
- **Axis 7 (Attr 8)**: `0`
- **Axis 8 (Attr 9)**: `0`
- **Configuration (Attr 10)**: `0x0000`
- **Tool Number (Attr 11)**: `1`
- **User Coordinate (Attr 12)**: `0`
- **Extended Config (Attr 13)**: `0`

**Note**: All other Variable P values are initialized to zero. Maximum variables: 128.

## Variable BP (Base Position Variables, Class 0x80)

### BP[0] - Home Position (Pulse Type)
- **Data Type (Attr 1)**: `0` (Pulse)
- **Axis 1 (Attr 2)**: `0`
- **Axis 2 (Attr 3)**: `0`
- **Axis 3 (Attr 4)**: `0`
- **Axis 4 (Attr 5)**: `0`
- **Axis 5 (Attr 6)**: `0`
- **Axis 6 (Attr 7)**: `0`
- **Axis 7 (Attr 8)**: `0`
- **Axis 8 (Attr 9)**: `0`

### BP[1] - Example Position (Base Type)
- **Data Type (Attr 1)**: `16` (Base coordinates)
- **X Position (Attr 2)**: `500000` (500.000 mm in micrometers)
- **Y Position (Attr 3)**: `600000` (600.000 mm in micrometers)
- **Z Position (Attr 4)**: `700000` (700.000 mm in micrometers)
- **RX Rotation (Attr 5)**: `0`
- **RY Rotation (Attr 6)**: `0`
- **RZ Rotation (Attr 7)**: `0`
- **Axis 7 (Attr 8)**: `0`
- **Axis 8 (Attr 9)**: `0`

**Note**: All other Variable BP values are initialized to zero. Maximum variables: 1000.

## Variable EX (External Axis Variables, Class 0x81)

### EX[0] - Home Position (Pulse Type)
- **Data Type (Attr 1)**: `0` (Pulse)
- **Axis 1 (Attr 2)**: `0`
- **Axis 2 (Attr 3)**: `0`
- **Axis 3 (Attr 4)**: `0`
- **Axis 4 (Attr 5)**: `0`
- **Axis 5 (Attr 6)**: `0`
- **Axis 6 (Attr 7)**: `0`
- **Axis 7 (Attr 8)**: `0`
- **Axis 8 (Attr 9)**: `0`

### EX[1] - Example Position (Pulse Type)
- **Data Type (Attr 1)**: `0` (Pulse)
- **Axis 1 (Attr 2)**: `10000` pulses
- **Axis 2 (Attr 3)**: `20000` pulses
- **Axis 3 (Attr 4)**: `0`
- **Axis 4 (Attr 5)**: `0`
- **Axis 5 (Attr 6)**: `0`
- **Axis 6 (Attr 7)**: `0`
- **Axis 7 (Attr 8)**: `0`
- **Axis 8 (Attr 9)**: `0`

**Note**: All other Variable EX values are initialized to zero. Maximum variables: 1000.

## Alarm History (Class 0x71)

### Instance 1
- **Alarm Code (Attr 1)**: `2100` (UDINT)
- **Alarm Data (Attr 2)**: `1` (UDINT)
- **Data Type (Attr 3)**: `0` (UDINT)
- **Date/Time (Attr 4)**: `"2024/01/15 10:30"` (16-byte string)
- **Alarm String (Attr 5)**: `"ALARM 2100"` (32-byte string)

### Instance 2
- **Alarm Code (Attr 1)**: `2101` (UDINT)
- **Alarm Data (Attr 2)**: `0` (UDINT)
- **Data Type (Attr 3)**: `0` (UDINT)
- **Date/Time (Attr 4)**: `"2024/01/15 11:00"` (16-byte string)
- **Alarm String (Attr 5)**: `"ALARM 2101"` (32-byte string)

### Instance 3
- **Alarm Code (Attr 1)**: `2200` (UDINT)
- **Alarm Data (Attr 2)**: `2` (UDINT)
- **Data Type (Attr 3)**: `0` (UDINT)
- **Date/Time (Attr 4)**: `"2024/01/15 12:00"` (16-byte string)
- **Alarm String (Attr 5)**: `"ALARM 2200"` (32-byte string)

**Note**: All other alarm history entries are initialized to zero. Maximum history entries: 100.

## Active Alarms (Class 0x70)

**Note**: All active alarms are initialized to zero (no active alarms). Maximum active alarms: 4.

## Data Units and Formats

### Position Data
- **Pulse Type**: Integer pulse values (encoder counts). The conversion from pulses to degrees depends on the robot model, axis, encoder resolution, and gear ratio. Each robot model has different pulse-to-degree conversion factors.
- **Base Type**: 
  - Length values in micrometers (μm), e.g., 500000 = 500.000 mm
  - Angle values in 0.0001° units, e.g., 900000 = 90.0000°

### Torque Data
- Stored as integer scaled by 1000 for 0.1% precision
- Example: 18500 = 18.5% (18500 / 1000)
- Typical range: 15-35% during normal operation

### Speed Override
- Stored as UDINT in units of 0.01%
- Example: 8550 = 85.5% (8550 * 0.01%)

### String Variables
- Fixed 32-byte arrays (null-terminated)
- Unused bytes are zero-filled

### Date/Time Format
- Fixed 16-byte string format: `"YYYY/MM/DD HH:MM"`
- Example: `"2024/01/15 10:30"`

## Memory Allocation

All large data arrays are allocated from PSRAM (32MB available on ESP32-P4 board):
- I/O Data: 8220 bytes
- Registers: 2000 bytes (1000 registers × 2 bytes)
- Variable B: 1000 bytes
- Variable I: 2000 bytes (1000 variables × 2 bytes)
- Variable D: 4000 bytes (1000 variables × 4 bytes)
- Variable R: 4000 bytes (1000 variables × 4 bytes)
- Variable S: 32000 bytes (1000 variables × 32 bytes)
- Variable P: 6656 bytes (128 variables × 13 attributes × 4 bytes)
- Variable BP: 36000 bytes (1000 variables × 9 attributes × 4 bytes)
- Variable EX: 36000 bytes (1000 variables × 9 attributes × 4 bytes)
- Position Data: 5616 bytes (108 instances × 13 attributes × 4 bytes)

If PSRAM allocation fails, arrays fall back to internal RAM allocation.

