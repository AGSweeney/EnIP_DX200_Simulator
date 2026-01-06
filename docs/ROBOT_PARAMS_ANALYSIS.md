# Robot Parameter Files Analysis

## Overview

The `RobotParams` folder contains parameter files extracted from a real Motoman DX200 robot controller. These files contain configuration data that can be used to populate the simulator with realistic values.

## File Structure

### ALL.PRM
- **Size**: 45,538 lines
- **Format**: Comma-separated values (CSV-like), 10 values per line typically
- **Sections**: Organized with section markers like `///RC1G`, `///RC2G`, etc.
- **Header**: Starts with `/ALL DN` and `//RC DN`
- **CRC**: Contains CRC value `1604859036`

### CIO.PRM
- **Size**: ~303 lines
- **Format**: Similar CSV format
- **Purpose**: Concurrent I/O configuration
- **Header**: `//CIO DN`

### FD.PRM
- **Size**: ~104 lines
- **Format**: Similar CSV format
- **Purpose**: Field Device configuration
- **Header**: `//FD DN`
- **CRC**: Contains CRC value `264048599`

### FMS.PRM
- **Size**: ~91 lines
- **Format**: Similar CSV format with section markers like `///FMS1B`, `///FMS2B`, etc.
- **Purpose**: FMS (Factory Management System) configuration
- **Header**: `//FMS DN`
- **CRC**: Contains CRC value `3412407861`

## Data Format

### Structure
- Lines starting with `/` or `//` are headers/comments
- Lines starting with `///` are section markers (e.g., `///RC1G`)
- Data lines contain comma-separated integer values
- Typically 10 values per line
- Values can be positive or negative integers

### Example Data from ALL.PRM

**Line 16 (Section RC1G):**
```
86,325000,0,1150000,0,300000,0,0,1225000,0
```
Possible interpretation: Axis limits or pulse ranges

**Line 22:**
```
992400,862200,20315200,720000,90000,268000,74000,0,0,0
```
Possible interpretation: Pulse encoder values or position data

**Line 26:**
```
38992,52169,-40329,0,0,0,0,0,0,0
```
Possible interpretation: Position offsets or calibration values

**Lines 46-50:**
```
27,0,0,180000,0,0,102000,104100,104100,92600
308400,50071,18972,-40526,15652,32558,26762,170700,93106,6
9468,2682,76216,76090,242560,41426,-59037,-4436,40087,14923
51632,25577,-1,1328,-1134,389,324,5000,24138,0
-1903,29,566,289,5000,2400,0,0,-24,11
```
Possible interpretation: Position data, calibration offsets, or configuration parameters

## Potential Uses for Simulator

### 1. Axis Configuration (Class 0x74)
- Extract axis limits, types, and configurations from RC sections
- Use pulse ranges and limits for realistic axis data

### 2. Position Data (Class 0x75)
- Extract position values that could represent home positions or calibration points
- Use pulse values to populate position instances

### 3. I/O Configuration
- CIO.PRM may contain I/O signal mappings
- Could help populate I/O class (0x78) with realistic signal configurations

### 4. System Parameters
- Extract system configuration values
- Could include RS022 parameter (though not found in search)
- Network settings, communication parameters

## Next Steps

1. **Parse the PRM files**: Create a parser to extract structured data
2. **Map to CIP classes**: Identify which sections map to which CIP classes
3. **Populate simulator**: Use extracted data to initialize simulator arrays
4. **Document mapping**: Create a reference document mapping PRM sections to CIP attributes

## Notes

- The format appears to be proprietary Motoman format
- Values are likely in robot-specific units (pulses, encoder counts, etc.)
- Some sections may be robot-model specific
- CRC values suggest file integrity checking
- Large file size (45K+ lines) suggests comprehensive robot configuration

## Questions to Resolve

1. What do the section markers mean? (RC1G, RC2G, FMS1B, etc.)
2. What units are the numeric values? (pulses, degrees, millimeters, etc.)
3. Which sections correspond to which CIP classes?
4. Are there parameter names/descriptions available in Motoman documentation?
5. How do we map array indices to specific parameters?

