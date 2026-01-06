# PSRAM Enablement Guide

## Overview

This document outlines the process of enabling and utilizing the 32MB PSRAM (Pseudo-Static RAM) on the Waveshare ESP32-P4-WIFI6-POE-ETH board for the Motoman DX200 Simulator project. PSRAM is essential for storing large data arrays (I/O signals, registers, variables, position data) that exceed the internal RAM capacity.

## Hardware Specifications

- **Board**: Waveshare ESP32-P4-WIFI6-POE-ETH
- **PSRAM Size**: 32MB
- **PSRAM Type**: HEX mode
- **PSRAM Speed**: 200MHz
- **PSRAM Interface**: SPI (Serial Peripheral Interface)

## Problem Statement

The Motoman DX200 Simulator requires large data arrays for:
- 512 I/O signals
- 1000 registers
- 1000 variables of each type (B, I, D, R, S, P, BP, EX)
- 108 position instances with 13 attributes each

These arrays total approximately **~500KB** of memory, which exceeds the available internal RAM on the ESP32-P4. Without PSRAM, the application would crash due to memory exhaustion.

## Solution: PSRAM Configuration

### Step 1: SDK Configuration (`sdkconfig.defaults`)

The following configuration options were added to `sdkconfig.defaults`:

```ini
# PSRAM (board has 32MB PSRAM - HEX mode, 200MHz)
# Based on Waveshare ESP32-P4-WIFI6-POE-ETH board example configs
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_HEX=y
CONFIG_SPIRAM_SPEED_200M=y
CONFIG_SPIRAM_BOOT_INIT=y
CONFIG_SPIRAM_USE_MALLOC=y
CONFIG_SPIRAM_MEMTEST=y
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384
CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=32768
```

#### Configuration Options Explained

- **`CONFIG_SPIRAM=y`**: Enables PSRAM support
- **`CONFIG_SPIRAM_MODE_HEX=y`**: Sets PSRAM to HEX mode (required for this board)
- **`CONFIG_SPIRAM_SPEED_200M=y`**: Configures PSRAM to run at 200MHz (board specification)
- **`CONFIG_SPIRAM_BOOT_INIT=y`**: Initializes PSRAM during boot (critical for early access)
- **`CONFIG_SPIRAM_USE_MALLOC=y`**: Allows standard `malloc()` to use PSRAM (via heap allocator)
- **`CONFIG_SPIRAM_MEMTEST=y`**: Performs memory test on PSRAM during initialization (helps detect hardware issues)
- **`CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384`**: Forces allocations ≤16KB to use internal RAM (for performance)
- **`CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=32768`**: Reserves 32KB of internal RAM for critical allocations

### Step 2: Project Reconfiguration

After modifying `sdkconfig.defaults`, the project must be reconfigured:

```bash
idf.py reconfigure
```

This step is **critical** - without reconfiguration, the changes in `sdkconfig.defaults` will not take effect.

### Step 3: Code Implementation

#### 3.1 Include Headers

**Important**: Use `esp_psram.h` (not the deprecated `esp_spiram.h`)

```c
#include "esp_psram.h"
#include "esp_heap_caps.h"
```

#### 3.2 PSRAM Diagnostics (Early Verification)

Add diagnostic code at the start of `app_main()` in `main/main.c`:

```c
#include "esp_psram.h"

void app_main(void) {
    // PSRAM diagnostics
    if (esp_psram_is_initialized()) {
        size_t psram_size = esp_psram_get_size();
        ESP_LOGI(TAG, "PSRAM initialized: %zu bytes (%.2f MB)", 
                 psram_size, psram_size / (1024.0f * 1024.0f));
    } else {
        ESP_LOGW(TAG, "PSRAM NOT initialized!");
    }
    
    // ... rest of initialization
}
```

#### 3.3 Allocating from PSRAM

Use `heap_caps_malloc()` with `MALLOC_CAP_SPIRAM` flag to allocate from PSRAM:

```c
static bool AllocateRobotDataArrays(void) {
    // Check PSRAM status
    if (esp_psram_is_initialized()) {
        size_t psram_size = esp_psram_get_size();
        ESP_LOGI(TAG, "PSRAM initialized, total size: %zu bytes (%.2f MB)", 
                 psram_size, psram_size / (1024.0f * 1024.0f));
        heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
    } else {
        ESP_LOGW(TAG, "PSRAM not initialized, will fall back to internal RAM");
    }
    
    // Allocate from PSRAM with fallback to internal RAM
    size_t io_size = MOTOMAN_MAX_IO_SIGNALS * sizeof(EipUint8);
    s_io_data = (EipUint8*)heap_caps_malloc(io_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    
    if (!s_io_data) {
        ESP_LOGW(TAG, "PSRAM allocation failed, using internal RAM");
        s_io_data = (EipUint8*)calloc(MOTOMAN_MAX_IO_SIGNALS, sizeof(EipUint8));
    } else {
        ESP_LOGI(TAG, "I/O data allocated in PSRAM at %p", s_io_data);
    }
    
    if (!s_io_data) return false;
    
    // Repeat for other arrays...
}
```

#### 3.4 Allocation Pattern

The pattern used for all large arrays:

1. **Try PSRAM first**: Use `heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)`
2. **Fallback to internal RAM**: If PSRAM allocation fails, use `calloc()` (internal RAM)
3. **Error handling**: Return `false` if allocation fails completely
4. **Logging**: Log allocation location (PSRAM vs internal RAM) for debugging

### Step 4: Component Dependencies

Ensure the `opener` component can access PSRAM functions by adding to `components/opener/CMakeLists.txt`:

```cmake
idf_component_register(
    # ... other settings ...
    REQUIRES
        # ... other dependencies ...
        esp_psram  # Added for PSRAM support
)
```

**Note**: The `main` component does NOT need `esp_psram` in its `REQUIRES` list because PSRAM is automatically available via `CONFIG_SPIRAM`.

## Verification

### Expected Log Output

When PSRAM is properly initialized, you should see:

```
I (xxxx) MotomanSim: PSRAM initialized, total size: 33554432 bytes (32.00 MB)
I (xxxx) MotomanSim: I/O data allocated in PSRAM at 0x480xxxxx
I (xxxx) MotomanSim: Registers allocated in PSRAM at 0x480xxxxx
I (xxxx) MotomanSim: Variable B allocated in PSRAM at 0x480xxxxx
...
```

### Heap Information

The diagnostic code prints heap information showing PSRAM allocations:

```
Heap summary for capabilities 0x00000400:
  At 0x48000000 len 33554432 free 33403284 allocated 148512
    largest_free_block 33030144
```

- **Address `0x48000000`**: PSRAM memory region
- **Length `33554432`**: 32MB total
- **Free/Allocated**: Shows PSRAM usage

## Common Issues and Solutions

### Issue 1: PSRAM Not Initializing

**Symptoms**: Log shows "PSRAM NOT initialized!" or PSRAM size is 0

**Solutions**:
1. Verify `sdkconfig.defaults` has all required options
2. Run `idf.py reconfigure` (critical step)
3. Check board hardware - ensure PSRAM chip is properly connected
4. Verify `CONFIG_SPIRAM_BOOT_INIT=y` is set

### Issue 2: Using Deprecated Header

**Symptoms**: Compiler warning: `"esp_spiram.h is deprecated, please migrate to esp_psram.h"`

**Solution**: Replace `#include "esp_spiram.h"` with `#include "esp_psram.h"`

### Issue 3: Allocation Fails Even with PSRAM

**Symptoms**: Allocations fall back to internal RAM despite PSRAM being initialized

**Possible Causes**:
- PSRAM heap is exhausted (check heap info)
- Allocation size exceeds available PSRAM
- Memory fragmentation

**Solution**: Check heap info output and verify available PSRAM space

### Issue 4: Project Not Reconfigured

**Symptoms**: Configuration changes in `sdkconfig.defaults` don't take effect

**Solution**: Always run `idf.py reconfigure` after modifying `sdkconfig.defaults`

## Memory Allocation Summary

The following arrays are allocated from PSRAM:

| Array | Size | Location |
|-------|------|----------|
| I/O Data | 512 bytes | PSRAM |
| Registers | 2,000 bytes | PSRAM |
| Variable B | 1,000 bytes | PSRAM |
| Variable I | 2,000 bytes | PSRAM |
| Variable D | 4,000 bytes | PSRAM |
| Variable R | 4,000 bytes | PSRAM |
| Variable S | 32,000 bytes | PSRAM |
| Variable P | 32,000 bytes | PSRAM |
| Variable BP | 32,000 bytes | PSRAM |
| Variable EX | 32,000 bytes | PSRAM |
| Position Data | 5,616 bytes | PSRAM |

**Total**: ~115KB allocated from PSRAM (well within 32MB capacity)

## Best Practices

1. **Always check PSRAM initialization** before allocating
2. **Provide fallback** to internal RAM for robustness
3. **Log allocation locations** for debugging
4. **Use `esp_psram.h`** (not deprecated `esp_spiram.h`)
5. **Reconfigure project** after changing `sdkconfig.defaults`
6. **Monitor heap usage** to detect memory leaks
7. **Reserve internal RAM** for time-critical allocations

## References

- ESP-IDF PSRAM Documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32p4/api-guides/external-ram.html
- Waveshare ESP32-P4-WIFI6-POE-ETH Board Wiki
- ESP-IDF Heap Memory Allocation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32p4/api-reference/system/heap_debug.html

## Revision History

- **2025-01-XX**: Initial document creation
- Documented PSRAM enablement process for Waveshare ESP32-P4 board
- Included configuration, code examples, and troubleshooting guide

