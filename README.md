# Motoman DX200 Simulator

An ESP32-P4 based EtherNet/IP simulator for the Motoman DX200 robot controller. This project provides a development tool for testing and developing EtherNet/IP scanner applications without requiring access to an actual robot controller.

## Overview

The Motoman DX200 Simulator implements the EtherNet/IP (CIP) protocol stack and emulates the vendor-specific CIP classes used by Yaskawa Motoman DX200 robot controllers. It runs on the ESP32-P4 microcontroller with Ethernet connectivity and provides a web-based configuration interface.

This simulator is designed to help in the development of EtherNet/IP scanner applications by providing a consistent, controllable test target that behaves like a real Motoman robot controller.

## Features

- **Full EtherNet/IP (CIP) Protocol Support**: Implements the OpENer EtherNet/IP stack
- **Motoman Vendor-Specific CIP Classes**: Emulates all Motoman-specific CIP classes (0x70-0x81, 0x8C)
- **Pre-Initialized Robot Data**: Includes realistic pre-configured robot data for immediate testing
- **Web-Based Configuration Interface**: HTTP web UI for network configuration and device management
- **PSRAM Support**: Utilizes 32MB PSRAM for large data arrays (I/O signals, registers, variables, position data)
- **Persistent Configuration**: NVS storage for network settings and device configuration

## Hardware Requirements

- **Board**: Waveshare ESP32-P4-WIFI6-POE-ETH (or compatible ESP32-P4 with Ethernet)
- **PSRAM**: 32MB PSRAM (required for large data arrays)
- **Ethernet**: 10/100 Mbps Ethernet connection

## Supported CIP Classes

The simulator implements the following Motoman vendor-specific CIP classes:

| Class | Hex | Decimal | Name | Description |
|-------|-----|---------|------|-------------|
| 0x70 | 112 | MotomanAlarm | Active Alarms |
| 0x71 | 113 | MotomanAlarmHistory | Alarm History |
| 0x72 | 114 | MotomanStatus | Robot Status |
| 0x73 | 115 | MotomanJobInfo | Job Information |
| 0x74 | 116 | MotomanAxisConfig | Axis Configuration |
| 0x75 | 117 | MotomanPosition | Robot Position |
| 0x76 | 118 | MotomanPositionDeviation | Position Deviation |
| 0x77 | 119 | MotomanTorque | Axis Torque |
| 0x78 | 120 | MotomanIO | I/O Signals |
| 0x79 | 121 | MotomanRegister | Registers |
| 0x7A | 122 | MotomanVariableB | Byte Variables |
| 0x7B | 123 | MotomanVariableI | Integer Variables |
| 0x7C | 124 | MotomanVariableD | Double Integer Variables |
| 0x7D | 125 | MotomanVariableR | Real Variables |
| 0x7F | 127 | MotomanVariableP | Position Variables |
| 0x80 | 128 | MotomanVariableBP | Base Position Variables |
| 0x81 | 129 | MotomanVariableEX | External Axis Variables |
| 0x8C | 140 | MotomanVariableS | String Variables |

For detailed information about each class, instances, and attributes, see:
- [CIP Classes Reference](docs/CIP_CLASSES_REFERENCE.md)
- [CIP Classes Decimal Reference](docs/CIP_CLASSES_DECIMAL_REFERENCE.md)

## Pre-Initialized Data

The simulator comes with pre-initialized robot data to facilitate immediate testing:

- **Status Data**: Robot status bits (Auto mode, Play mode, Servo On)
- **Job Info**: Current job name, line number, step number, speed override
- **Axis Configuration**: 6-axis robot configuration
- **Position Data**: Robot position (pulse values) for multiple instances
- **Position Deviation**: Axis position deviation values
- **Torque Data**: Axis torque values
- **I/O Signals**: Pre-configured I/O signal states
- **Registers**: Pre-initialized register values (M000-M999)
- **Variables**: Pre-initialized values for B, I, D, R, S, P, BP, EX variables

For a complete list of all pre-initialized values, see:
- [Pre-Initialized Data Reference](docs/PREINITIALIZED_DATA_REFERENCE.md)

## Building and Flashing

### Prerequisites

- ESP-IDF v5.1 or later
- Python 3.8 or later
- CMake 3.16 or later

### Build Steps

1. Clone the repository:
```bash
git clone <repository-url>
cd DX200Sim
```

2. Set up ESP-IDF environment:
```bash
. $HOME/esp/esp-idf/export.sh
```

3. Configure the project (optional - defaults are in `sdkconfig.defaults`):
```bash
idf.py menuconfig
```

4. Build the project:
```bash
idf.py build
```

5. Flash to device:
```bash
idf.py -p <PORT> flash
```

6. Monitor serial output:
```bash
idf.py -p <PORT> monitor
```

### Configuration

The project includes default configuration in `sdkconfig.defaults`:
- PSRAM enabled (32MB, HEX mode, 200MHz)
- Ethernet configuration for Waveshare ESP32-P4-WIFI6-POE-ETH
- OpENer EtherNet/IP stack configuration

For PSRAM configuration details, see:
- [PSRAM Enablement Guide](docs/PSRAM_ENABLEMENT.md)

## Usage

### Network Configuration

1. **Initial Setup**: The device starts with DHCP enabled by default. Check your DHCP server or serial monitor for the assigned IP address.

2. **Web Interface**: Open a web browser and navigate to `http://<device-ip>` to access the configuration interface.

3. **Static IP Configuration**: Use the web interface to configure static IP settings:
   - IP Address
   - Netmask
   - Gateway
   - DNS servers

4. **Reboot Required**: Network configuration changes require a device reboot to take effect.

### Testing with EtherNet/IP Scanner

1. **Connect Scanner**: Connect your EtherNet/IP scanner application to the simulator's IP address.

2. **Device Identity**: The simulator presents itself as:
   - **Vendor ID**: 44 (Yaskawa Electric America, Inc.)
   - **Device Type**: 12 (Communications Adapter)
   - **Product Code**: 1281
   - **Product Name**: "DX200 EtherNet/IP Module"
   - **Revision**: 1.1

3. **Read Robot Data**: Use your scanner to read from any of the supported CIP classes. All classes support `Get_Attribute_Single` and many support `Get_Attribute_All`.

4. **Verify Data**: Compare the data returned by the simulator with expected values from the [Pre-Initialized Data Reference](docs/PREINITIALIZED_DATA_REFERENCE.md).

### Using EtherNet/IP Explorer

The simulator is compatible with EtherNet/IP Explorer and other CIP diagnostic tools:

1. **Connect**: Enter the simulator's IP address in EtherNet/IP Explorer
2. **Browse Classes**: Navigate to the Motoman vendor-specific classes (112-140 decimal)
3. **Read Attributes**: Use `Get_Attribute_Single` or `Get_Attribute_All` to read data

For class number conversions, see:
- [CIP Classes Decimal Reference](docs/CIP_CLASSES_DECIMAL_REFERENCE.md)

## Project Structure

```
DX200Sim/
├── components/
│   ├── opener/              # OpENer EtherNet/IP stack
│   │   └── src/ports/ESP32/
│   │       └── motoman_dx200_simulator/  # Simulator implementation
│   ├── webui/               # Web-based configuration interface
│   └── system_config/       # System configuration management
├── docs/                    # Documentation
│   ├── CIP_CLASSES_REFERENCE.md
│   ├── PREINITIALIZED_DATA_REFERENCE.md
│   ├── PSRAM_ENABLEMENT.md
│   └── ...
├── main/                    # Main application code
├── RobotParams/             # Robot parameter files (for reference)
└── scripts/                 # Build and utility scripts
```

## Documentation

- [CIP Classes Reference](docs/CIP_CLASSES_REFERENCE.md) - Complete reference of all CIP classes, instances, and attributes
- [CIP Classes Decimal Reference](docs/CIP_CLASSES_DECIMAL_REFERENCE.md) - Hex to decimal conversion for CIP tools
- [Pre-Initialized Data Reference](docs/PREINITIALIZED_DATA_REFERENCE.md) - All pre-configured robot data values
- [PSRAM Enablement Guide](docs/PSRAM_ENABLEMENT.md) - PSRAM configuration and usage
- [Robot Parameters Analysis](docs/ROBOT_PARAMS_ANALYSIS.md) - Analysis of robot parameter files

## Development

This simulator is designed to help in the development of EtherNet/IP scanner applications. Key features for development:

- **Consistent Test Target**: Provides a stable, predictable test environment
- **Realistic Data**: Pre-initialized with realistic robot data
- **Full Protocol Support**: Implements all Motoman CIP classes used by real controllers
- **Easy Configuration**: Web interface for network and device settings
- **Debugging Support**: Serial logging for troubleshooting

### Modifying Robot Data

Robot data is initialized in `components/opener/src/ports/ESP32/motoman_dx200_simulator/motoman_dx200_simulator.c` in the `InitializeRobotData()` function. Modify the initialization values to test different scenarios.

### Adding New CIP Classes

To add support for additional CIP classes:

1. Define the class number in `motoman_dx200_simulator.c`
2. Create the class using OpENer's CIP class creation functions
3. Implement attribute handlers for `Get_Attribute_Single` and `Set_Attribute_Single` if needed
4. Update the documentation

## License

Copyright (c) 2026, Adam G. Sweeney <agsweeney@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

## Acknowledgments

- **OpENer**: EtherNet/IP stack implementation (https://github.com/EIPStackGroup/OpENer)
- **ESP-IDF**: Espressif IoT Development Framework
- **Motoman Manual 165838-1CD**: EtherNet/IP communication specifications

