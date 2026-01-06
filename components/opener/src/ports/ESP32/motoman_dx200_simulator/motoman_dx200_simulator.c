#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "opener_api.h"
#include "appcontype.h"
#include "cipqos.h"
#include "ciptypes.h"
#include "typedefs.h"
#include "cipcommon.h"
#include "system_config.h"

static const char *TAG = "motoman_dx200_simulator";

struct netif;

#define MOTOMAN_CLASS_ALARM                   0x70
#define MOTOMAN_CLASS_ALARM_HISTORY           0x71
#define MOTOMAN_CLASS_STATUS                  0x72
#define MOTOMAN_CLASS_JOB_INFO                0x73
#define MOTOMAN_CLASS_AXIS_CONFIG             0x74
#define MOTOMAN_CLASS_POSITION                0x75
#define MOTOMAN_CLASS_POSITION_DEVIATION       0x76
#define MOTOMAN_CLASS_TORQUE                  0x77
#define MOTOMAN_CLASS_IO                      0x78
#define MOTOMAN_CLASS_REGISTER                0x79
#define MOTOMAN_CLASS_VARIABLE_B              0x7A
#define MOTOMAN_CLASS_VARIABLE_I              0x7B
#define MOTOMAN_CLASS_VARIABLE_D              0x7C
#define MOTOMAN_CLASS_VARIABLE_R              0x7D
#define MOTOMAN_CLASS_VARIABLE_S              0x8C
#define MOTOMAN_CLASS_VARIABLE_P              0x7F
#define MOTOMAN_CLASS_VARIABLE_BP             0x80
#define MOTOMAN_CLASS_VARIABLE_EX             0x81

#define MOTOMAN_MAX_IO_SIGNALS                8220
#define MOTOMAN_MAX_REGISTERS                 1000
#define MOTOMAN_MAX_VARIABLES                  1000
#define MOTOMAN_MAX_VARIABLE_P                 128
#define MOTOMAN_MAX_STRING_LENGTH              32
#define MOTOMAN_MAX_AXES                      8
#define MOTOMAN_MAX_POSITION_INSTANCES         108
#define MOTOMAN_POSITION_ATTRIBUTES            13
#define MOTOMAN_VARIABLE_P_ATTRIBUTES          13
#define MOTOMAN_VARIABLE_BP_ATTRIBUTES         9
#define MOTOMAN_VARIABLE_EX_ATTRIBUTES         9
#define MOTOMAN_MAX_ACTIVE_ALARMS             4
#define MOTOMAN_ALARM_HISTORY_SIZE            100

typedef struct {
    EipUint32 code;
    EipUint32 data;
    EipUint32 data_type;
    char date_time[16];
    char string[32];
} MotomanAlarm;

static MotomanAlarm s_active_alarms[MOTOMAN_MAX_ACTIVE_ALARMS] = {0};
static MotomanAlarm s_alarm_history[MOTOMAN_ALARM_HISTORY_SIZE] = {0};
// s_alarm_history_count reserved for future use to track number of alarms in history
static EipUint32 s_status_data1 = 0x00000044;
static EipUint32 s_status_data2 = 0x00000040;
static EipUint32 s_job_line = 1;
static EipUint32 s_step_number = 0;
static EipUint32 s_speed_override = 10000;  // Speed override in units of 0.01% (10000 = 100.0%)
static EipUint8 s_job_name[32] = {'M','A','I','N','.','J','B','I',0};
static EipUint8 s_axis_count = 6;
static EipUint8 s_axis_type[MOTOMAN_MAX_AXES] = {1,1,1,1,1,1,0,0};
static EipInt32 s_position[MOTOMAN_MAX_AXES] = {0};
static EipInt32 (*s_position_data)[MOTOMAN_POSITION_ATTRIBUTES] = NULL;
static EipInt32 s_position_deviation[MOTOMAN_MAX_AXES] = {0};
static EipInt32 s_torque[MOTOMAN_MAX_AXES] = {0};
static EipUint8 *s_io_data = NULL;
static EipUint16 *s_registers = NULL;
static EipUint8 *s_variable_b = NULL;
static EipInt16 *s_variable_i = NULL;
static EipInt32 *s_variable_d = NULL;
static float *s_variable_r = NULL;
static char (*s_variable_s)[MOTOMAN_MAX_STRING_LENGTH] = NULL;

static void EncodeMotomanString32(const void *const data, ENIPMessage *const outgoing_message) {
    const char (*string_array)[MOTOMAN_MAX_STRING_LENGTH] = (const char (*)[MOTOMAN_MAX_STRING_LENGTH])data;
    memcpy(outgoing_message->current_message_position, *string_array, MOTOMAN_MAX_STRING_LENGTH);
    outgoing_message->current_message_position += MOTOMAN_MAX_STRING_LENGTH;
    outgoing_message->used_message_length += MOTOMAN_MAX_STRING_LENGTH;
}

static int DecodeMotomanString32(void *const data, CipMessageRouterRequest *const message_router_request, CipMessageRouterResponse *const message_router_response) {
    char (*string_array)[MOTOMAN_MAX_STRING_LENGTH] = (char (*)[MOTOMAN_MAX_STRING_LENGTH])data;
    memcpy(*string_array, message_router_request->data, MOTOMAN_MAX_STRING_LENGTH);
    message_router_request->data += MOTOMAN_MAX_STRING_LENGTH;
    message_router_response->general_status = kCipErrorSuccess;
    return MOTOMAN_MAX_STRING_LENGTH;
}
static EipInt32 (*s_variable_p)[MOTOMAN_VARIABLE_P_ATTRIBUTES] = NULL;
static EipInt32 (*s_variable_bp)[MOTOMAN_VARIABLE_BP_ATTRIBUTES] = NULL;
static EipInt32 (*s_variable_ex)[MOTOMAN_VARIABLE_EX_ATTRIBUTES] = NULL;

static bool s_rs022_enabled = true;

static inline int GetArrayIndexFromInstance(int instance_number) {
    // Note: In CIP, instance 0 is reserved for the class object, so instance_number will always be >= 1.
    // RS022=1: "Specify the variable P number" - Variable 0 (P0) should be instance 0, but instance 0 is reserved.
    //          So: Instance 1 = P[0] (variable 0), Instance 2 = P[1] (variable 1), etc.
    // RS022=0: "Specify the variable P number +1" - Instance 1 = P[0] (variable 0), Instance 2 = P[1] (variable 1), etc.
    // Manual 165838-1CD: RS022=1 means direct mapping (but instance 0 is reserved), RS022=0 means offset by +1
    // Both modes end up mapping the same way due to CIP's instance 0 reservation: instance N = variable[N-1]
    int idx = instance_number - 1;
    if (idx < 0 || idx >= MOTOMAN_MAX_VARIABLES) {
        return -1;  // Out of bounds
    }
    return idx;
}

static bool AllocateRobotDataArrays(void) {
    static const char *TAG = "MotomanSim";
    
    ESP_LOGI(TAG, "--- Allocating Robot Data Arrays ---");
    
    if (esp_psram_is_initialized()) {
        size_t psram_size = esp_psram_get_size();
        ESP_LOGI(TAG, "PSRAM initialized, total size: %zu bytes (%.2f MB)", psram_size, psram_size / (1024.0f * 1024.0f));
    } else {
        ESP_LOGW(TAG, "PSRAM not initialized, will fall back to internal RAM");
    }
    
    size_t io_size = MOTOMAN_MAX_IO_SIGNALS * sizeof(EipUint8);
    ESP_LOGI(TAG, "Allocating I/O data: %zu bytes", io_size);
    s_io_data = (EipUint8*)heap_caps_malloc(io_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_io_data) {
        ESP_LOGW(TAG, "PSRAM allocation failed for I/O data, using internal RAM");
        s_io_data = (EipUint8*)calloc(MOTOMAN_MAX_IO_SIGNALS, sizeof(EipUint8));
    } else {
        ESP_LOGI(TAG, "I/O data allocated in PSRAM at %p", s_io_data);
    }
    if (!s_io_data) return false;
    
    size_t registers_size = MOTOMAN_MAX_REGISTERS * sizeof(EipUint16);
    ESP_LOGI(TAG, "Allocating registers: %zu bytes", registers_size);
    s_registers = (EipUint16*)heap_caps_malloc(registers_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_registers) {
        ESP_LOGW(TAG, "PSRAM allocation failed for registers, using internal RAM");
        s_registers = (EipUint16*)calloc(MOTOMAN_MAX_REGISTERS, sizeof(EipUint16));
    } else {
        ESP_LOGI(TAG, "Registers allocated in PSRAM at %p", s_registers);
    }
    if (!s_registers) return false;
    
    size_t var_b_size = MOTOMAN_MAX_VARIABLES * sizeof(EipUint8);
    ESP_LOGI(TAG, "Allocating variable B: %zu bytes", var_b_size);
    s_variable_b = (EipUint8*)heap_caps_malloc(var_b_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_variable_b) {
        ESP_LOGW(TAG, "PSRAM allocation failed for variable B, using internal RAM");
        s_variable_b = (EipUint8*)calloc(MOTOMAN_MAX_VARIABLES, sizeof(EipUint8));
    } else {
        ESP_LOGI(TAG, "Variable B allocated in PSRAM at %p", s_variable_b);
    }
    if (!s_variable_b) return false;
    
    size_t var_i_size = MOTOMAN_MAX_VARIABLES * sizeof(EipInt16);
    ESP_LOGI(TAG, "Allocating variable I: %zu bytes", var_i_size);
    s_variable_i = (EipInt16*)heap_caps_malloc(var_i_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_variable_i) {
        ESP_LOGW(TAG, "PSRAM allocation failed for variable I, using internal RAM");
        s_variable_i = (EipInt16*)calloc(MOTOMAN_MAX_VARIABLES, sizeof(EipInt16));
    } else {
        ESP_LOGI(TAG, "Variable I allocated in PSRAM at %p", s_variable_i);
    }
    if (!s_variable_i) return false;
    
    size_t var_d_size = MOTOMAN_MAX_VARIABLES * sizeof(EipInt32);
    ESP_LOGI(TAG, "Allocating variable D: %zu bytes", var_d_size);
    s_variable_d = (EipInt32*)heap_caps_malloc(var_d_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_variable_d) {
        ESP_LOGW(TAG, "PSRAM allocation failed for variable D, using internal RAM");
        s_variable_d = (EipInt32*)calloc(MOTOMAN_MAX_VARIABLES, sizeof(EipInt32));
    } else {
        ESP_LOGI(TAG, "Variable D allocated in PSRAM at %p", s_variable_d);
    }
    if (!s_variable_d) return false;
    
    size_t var_r_size = MOTOMAN_MAX_VARIABLES * sizeof(float);
    ESP_LOGI(TAG, "Allocating variable R: %zu bytes", var_r_size);
    s_variable_r = (float*)heap_caps_malloc(var_r_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_variable_r) {
        ESP_LOGW(TAG, "PSRAM allocation failed for variable R, using internal RAM");
        s_variable_r = (float*)calloc(MOTOMAN_MAX_VARIABLES, sizeof(float));
    } else {
        ESP_LOGI(TAG, "Variable R allocated in PSRAM at %p", s_variable_r);
    }
    if (!s_variable_r) return false;
    
    size_t var_s_size = MOTOMAN_MAX_VARIABLES * sizeof(char[MOTOMAN_MAX_STRING_LENGTH]);
    ESP_LOGI(TAG, "Allocating variable S: %zu bytes", var_s_size);
    s_variable_s = (char(*)[MOTOMAN_MAX_STRING_LENGTH])heap_caps_malloc(var_s_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_variable_s) {
        ESP_LOGW(TAG, "PSRAM allocation failed for variable S, using internal RAM");
        s_variable_s = (char(*)[MOTOMAN_MAX_STRING_LENGTH])calloc(MOTOMAN_MAX_VARIABLES, sizeof(char[MOTOMAN_MAX_STRING_LENGTH]));
    } else {
        ESP_LOGI(TAG, "Variable S allocated in PSRAM at %p", s_variable_s);
    }
    if (!s_variable_s) return false;
    
    size_t var_p_size = MOTOMAN_MAX_VARIABLE_P * sizeof(EipInt32[MOTOMAN_VARIABLE_P_ATTRIBUTES]);
    ESP_LOGI(TAG, "Allocating variable P: %zu bytes", var_p_size);
    s_variable_p = (EipInt32(*)[MOTOMAN_VARIABLE_P_ATTRIBUTES])heap_caps_malloc(var_p_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_variable_p) {
        ESP_LOGW(TAG, "PSRAM allocation failed for variable P, using internal RAM");
        s_variable_p = (EipInt32(*)[MOTOMAN_VARIABLE_P_ATTRIBUTES])calloc(MOTOMAN_MAX_VARIABLE_P, sizeof(EipInt32[MOTOMAN_VARIABLE_P_ATTRIBUTES]));
    } else {
        ESP_LOGI(TAG, "Variable P allocated in PSRAM at %p", s_variable_p);
        memset(s_variable_p, 0, var_p_size);
    }
    if (!s_variable_p) return false;
    
    size_t var_bp_size = MOTOMAN_MAX_VARIABLES * sizeof(EipInt32[MOTOMAN_VARIABLE_BP_ATTRIBUTES]);
    ESP_LOGI(TAG, "Allocating variable BP: %zu bytes", var_bp_size);
    s_variable_bp = (EipInt32(*)[MOTOMAN_VARIABLE_BP_ATTRIBUTES])heap_caps_malloc(var_bp_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_variable_bp) {
        ESP_LOGW(TAG, "PSRAM allocation failed for variable BP, using internal RAM");
        s_variable_bp = (EipInt32(*)[MOTOMAN_VARIABLE_BP_ATTRIBUTES])calloc(MOTOMAN_MAX_VARIABLES, sizeof(EipInt32[MOTOMAN_VARIABLE_BP_ATTRIBUTES]));
    } else {
        ESP_LOGI(TAG, "Variable BP allocated in PSRAM at %p", s_variable_bp);
    }
    if (!s_variable_bp) return false;
    
    size_t var_ex_size = MOTOMAN_MAX_VARIABLES * sizeof(EipInt32[MOTOMAN_VARIABLE_EX_ATTRIBUTES]);
    ESP_LOGI(TAG, "Allocating variable EX: %zu bytes", var_ex_size);
    s_variable_ex = (EipInt32(*)[MOTOMAN_VARIABLE_EX_ATTRIBUTES])heap_caps_malloc(var_ex_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_variable_ex) {
        ESP_LOGW(TAG, "PSRAM allocation failed for variable EX, using internal RAM");
        s_variable_ex = (EipInt32(*)[MOTOMAN_VARIABLE_EX_ATTRIBUTES])calloc(MOTOMAN_MAX_VARIABLES, sizeof(EipInt32[MOTOMAN_VARIABLE_EX_ATTRIBUTES]));
    } else {
        ESP_LOGI(TAG, "Variable EX allocated in PSRAM at %p", s_variable_ex);
    }
    if (!s_variable_ex) return false;
    
    size_t position_data_size = MOTOMAN_MAX_POSITION_INSTANCES * sizeof(EipInt32[MOTOMAN_POSITION_ATTRIBUTES]);
    ESP_LOGI(TAG, "Allocating position data: %zu bytes", position_data_size);
    s_position_data = (EipInt32(*)[MOTOMAN_POSITION_ATTRIBUTES])heap_caps_malloc(position_data_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_position_data) {
        ESP_LOGW(TAG, "PSRAM allocation failed for position data, using internal RAM");
        s_position_data = (EipInt32(*)[MOTOMAN_POSITION_ATTRIBUTES])calloc(MOTOMAN_MAX_POSITION_INSTANCES, sizeof(EipInt32[MOTOMAN_POSITION_ATTRIBUTES]));
    } else {
        ESP_LOGI(TAG, "Position data allocated in PSRAM at %p", s_position_data);
        memset(s_position_data, 0, position_data_size);
    }
    if (!s_position_data) return false;
    
    ESP_LOGI(TAG, "--- All allocations complete ---");
    
    return true;
}

static void InitializeRobotData(void) {
    // Arrays should already be allocated by ApplicationInitialization
    if (!s_io_data || !s_registers || !s_variable_b || !s_position_data) {
        return;
    }
    // Initialize Status Data (per Manual 165838-1CD, Table 5-3)
    // Status Data 1 bits:
    //   bit 0: Step, bit 1: 1 cycle, bit 2: Auto, bit 3: Running
    //   bit 4: Safety speed operation, bit 5: Teach, bit 6: Play, bit 7: Command remote
    // Status Data 2 bits:
    //   bit 1: Hold (Programming pendant), bit 2: Hold (external), bit 3: Hold (Command)
    //   bit 4: Alarm, bit 5: Error, bit 6: Servo on
    // Set: Auto (bit 2), Play (bit 6) = 0x44
    s_status_data1 = 0x00000044;
    // Set: Servo On (bit 6) = 0x40
    s_status_data2 = 0x00000040;
    
    // Initialize Job Info (per Manual 165838-1CD, Table 5-4)
    // Line number: 0 to 9999, Step number: 1 to 9998
    // Speed override: Unit 0.01% (10000 = 100.0%, 8550 = 85.5%)
    s_job_line = 15;
    s_step_number = 42;  // Valid range: 1 to 9998
    s_speed_override = 8550;  // 85.5% (in units of 0.01%)
    strncpy((char*)s_job_name, "WELD001.JBI", sizeof(s_job_name) - 1);
    s_job_name[sizeof(s_job_name) - 1] = '\0';
    
    // Initialize Axis Configuration (6-axis robot)
    s_axis_count = 6;
    s_axis_type[0] = 1; // S-axis (base rotation)
    s_axis_type[1] = 1; // L-axis (lower arm)
    s_axis_type[2] = 1; // U-axis (upper arm)
    s_axis_type[3] = 1; // R-axis (wrist roll)
    s_axis_type[4] = 1; // B-axis (wrist bend)
    s_axis_type[5] = 1; // T-axis (wrist twist)
    s_axis_type[6] = 0; // Unused
    s_axis_type[7] = 0; // Unused
    
    // Initialize Position (per Manual 165838-1CD, Table 5-6)
    // For pulse: Each axis' pulse value (4 bytes per axis)
    // Pulse values are encoder counts - conversion to degrees depends on robot model and axis
    // Home position: all axes near zero with slight offsets
    s_position[0] = 1250;      // S-axis: 1250 pulses
    s_position[1] = -15230;    // L-axis: -15230 pulses
    s_position[2] = 28340;     // U-axis: 28340 pulses
    s_position[3] = -450;      // R-axis: -450 pulses
    s_position[4] = 8920;      // B-axis: 8920 pulses
    s_position[5] = -120;       // T-axis: -120 pulses
    s_position[6] = 0;          // 7th axis: unused
    s_position[7] = 0;          // 8th axis: unused
    
    // Initialize Position Data array (per Manual 165838-1CD, Table 5-6)
    // Instances: 1-8 (Robot Pulse), 11-18 (Base Pulse), 21-44 (Station Pulse), 101-108 (Robot Base)
    // Attributes: 1=Data type, 2-9=Axis data, 10-13=Config/Tool/Reservation/ExtConfig
    if (s_position_data) {
        for (int inst = 0; inst < MOTOMAN_MAX_POSITION_INSTANCES; inst++) {
            for (int attr = 0; attr < MOTOMAN_POSITION_ATTRIBUTES; attr++) {
                s_position_data[inst][attr] = 0;
            }
        }
        
        // Initialize instance 1 (Robot Pulse, 1-8 range)
        // Attribute 1: Data type = 0 (Pulse)
        // Attributes 2-9: Axis pulse data (encoder counts)
        s_position_data[0][0] = 0;      // Attribute 1: Data type = 0 (Pulse)
        s_position_data[0][1] = 1250;   // Attribute 2: 1st axis (S) = 1250 pulses
        s_position_data[0][2] = -15230; // Attribute 3: 2nd axis (L) = -15230 pulses
        s_position_data[0][3] = 28340;  // Attribute 4: 3rd axis (U) = 28340 pulses
        s_position_data[0][4] = -450;   // Attribute 5: 4th axis (R) = -450 pulses
        s_position_data[0][5] = 8920;   // Attribute 6: 5th axis (B) = 8920 pulses
        s_position_data[0][6] = -120;  // Attribute 7: 6th axis (T) = -120 pulses
        s_position_data[0][7] = 0;      // Attribute 8: 7th axis = 0
        s_position_data[0][8] = 0;      // Attribute 9: 8th axis = 0
        s_position_data[0][9] = 0;      // Attribute 10: Configuration = 0
        s_position_data[0][10] = 0;     // Attribute 11: Tool number = 0
        s_position_data[0][11] = 0;     // Attribute 12: Reservation = 0
        s_position_data[0][12] = 0;     // Attribute 13: Extended configuration = 0
        
        memset(&s_position_data[0][7], 0, 2 * sizeof(EipInt32));
        
        // Initialize instance 102 (Robot Base, 101-108 range)
        // Attribute 1: Data type = 16 (Base)
        // Attributes 2-9: Axis data (X, Y, Z, RX, RY, RZ, 7th, 8th)
        // Base coordinates: Length in μm (micrometers), Angle in 0.0001° units
        // Sample values: X=1234.567mm, Y=2345.678mm, Z=3456.789mm
        if (MOTOMAN_MAX_POSITION_INSTANCES > 102) {
            s_position_data[101][0] = 16;      // Attribute 1: Data type = Base
            s_position_data[101][1] = 1234567; // Attribute 2: X position = 1234.567 mm (in μm)
            s_position_data[101][2] = 2345678; // Attribute 3: Y position = 2345.678 mm (in μm)
            s_position_data[101][3] = 3456789; // Attribute 4: Z position = 3456.789 mm (in μm)
            s_position_data[101][4] = 0;       // Attribute 5: RX rotation = 0 (angle in 0.0001°)
            s_position_data[101][5] = 1234567;  // Attribute 6: X Position Robot Base (in μm) - matches Attribute 2
            s_position_data[101][6] = 0;       // Attribute 7: RZ rotation = 0 (angle in 0.0001°)
            s_position_data[101][7] = 0;        // Attribute 8: 7th axis = 0
            s_position_data[101][8] = 0;        // Attribute 9: 8th axis = 0
        }
    }
    
    // Initialize Position Deviation (per Manual 165838-1CD, Table 5-7)
    // "Read the deviation of each axis position" - pulse value of each axis
    // Small deviations from commanded position, in pulse units
    s_position_deviation[0] = 2;    // S-axis: 2 pulses deviation
    s_position_deviation[1] = -3;   // L-axis: -3 pulses deviation
    s_position_deviation[2] = 1;    // U-axis: 1 pulse deviation
    s_position_deviation[3] = 0;    // R-axis: no deviation
    s_position_deviation[4] = -1;   // B-axis: -1 pulse deviation
    s_position_deviation[5] = 1;    // T-axis: 1 pulse deviation
    s_position_deviation[6] = 0;    // 7th axis: unused
    s_position_deviation[7] = 0;    // 8th axis: unused
    
    // Initialize Torque (per Manual 165838-1CD, Table 5-8)
    // "The value for the torque of each axis is output as a percentage when the nominal value is 100%"
    // Stored as integer scaled by 1000 for 0.1% precision (e.g., 18.5% = 18500)
    // Typical values: 15-35% during normal operation
    s_torque[0] = 18500;   // S-axis: 18.5% (18500 / 1000)
    s_torque[1] = 22300;   // L-axis: 22.3% (22300 / 1000)
    s_torque[2] = 31200;   // U-axis: 31.2% (31200 / 1000)
    s_torque[3] = 4500;    // R-axis: 4.5% (4500 / 1000)
    s_torque[4] = 12800;   // B-axis: 12.8% (12800 / 1000)
    s_torque[5] = 2100;    // T-axis: 2.1% (2100 / 1000)
    s_torque[6] = 0;
    s_torque[7] = 0;
    
    // Initialize I/O Data (per ConcurrentIO Manual RE-CKI-A465, Table 2-1)
    // Instance = signal number / 10
    // Signal ranges for General Purpose Robot:
    //   General Input: 00010-05127 (4096 signals) → instances 1-512
    //   General Output: 10010-15127 (4096 signals) → instances 1001-1512
    //   External Input: 20010-25127 (4096 signals) → instances 2001-2512
    //   External Output: 30010-35127 (4096 signals) → instances 3001-3512
    //   Specific Input: 40010-41607 (1280 signals) → instances 4001-4160
    //   Specific Output: 50010-53007 (2400 signals) → instances 5001-5300
    // General Input signals (instances 1-512, signals 00010-05127)
    s_io_data[0] = 0x01;   // Input 1 (signal 00010): ON
    s_io_data[1] = 0x00;   // Input 2 (signal 00020): OFF
    s_io_data[2] = 0x01;   // Input 3 (signal 00030): ON
    s_io_data[3] = 0x01;   // Input 4 (signal 00040): ON
    s_io_data[10] = 0x01;  // Input 11 (signal 00110): ON
    s_io_data[50] = 0x00;  // Input 51 (signal 00510): OFF
    // General Output signals (instances 1001-1512, signals 10010-15127)
    // Note: Array size limits us to instances 0-511, so we map instances 1001+ to array indices
    // For now, we'll use a subset. Full implementation would need instance-based mapping.
    s_io_data[100] = 0x01; // Output 1 (signal 10010): ON (mapped from instance 1001)
    s_io_data[101] = 0x00; // Output 2 (signal 10020): OFF
    s_io_data[102] = 0x01; // Output 3 (signal 10030): ON
    // External Input signals (instances 2001-2512, signals 20010-25127)
    s_io_data[200] = 0x01; // External Input 1 (signal 20010): ON (mapped from instance 2001)
    s_io_data[201] = 0x00; // External Input 2 (signal 20020): OFF
    
    // Initialize Registers (per ConcurrentIO Manual RE-CKI-A465, Section 2.2)
    // Register ranges:
    //   M000-M559: General Register (560 registers, readable/writable)
    //   M560-M599: Analog Output Register (40 registers, readable/writable)
    //   M600-M639: Analog Input Register (40 registers, read-only)
    //   M640-M999: System Register (360 registers, read-only)
    // Total: 1000 registers (M000-M999) - allocated from PSRAM
    // Data: 2 bytes (UINT16) per register
    s_registers[0] = 100;   // M000 = 100
    s_registers[1] = 250;   // M001 = 250
    s_registers[2] = 500;   // M002 = 500
    s_registers[10] = 1234; // M010 = 1234
    s_registers[20] = 5678; // M020 = 5678
    s_registers[100] = 9999; // M100 = 9999
    s_registers[559] = 0;    // M559 = 0 (last general register)
    s_registers[560] = 32767; // M560 = 32767 (Analog Output 1, max value)
    s_registers[599] = 0;    // M599 = 0 (last analog output register)
    s_registers[600] = 16384; // M600 = 16384 (Analog Input 1, read-only)
    s_registers[639] = 0;    // M639 = 0 (last analog input register)
    s_registers[640] = 0;    // M640 = 0 (System Register 1, read-only)
    s_registers[999] = 0;    // M999 = 0 (last system register)
    
    // Initialize Variable B (Byte variables)
    s_variable_b[0] = 10;
    s_variable_b[1] = 20;
    s_variable_b[2] = 30;
    s_variable_b[10] = 100;
    
    // Initialize Variable I (Integer variables)
    s_variable_i[0] = 1234;
    s_variable_i[1] = -567;
    s_variable_i[2] = 8901;
    s_variable_i[10] = 42;
    
    // Initialize Variable D (Double integer variables)
    s_variable_d[0] = 123456;
    s_variable_d[1] = -789012;
    s_variable_d[2] = 345678;
    s_variable_d[10] = 999999;
    
    // Initialize Variable R (Real/Float variables)
    s_variable_r[0] = 123.456f;
    s_variable_r[1] = -45.678f;
    s_variable_r[2] = 789.012f;
    s_variable_r[10] = 3.14159f;
    
    // Initialize Variable S (String variables) - fixed 32-byte arrays per Manual 165838-1CD
    memset(s_variable_s[0], 0, MOTOMAN_MAX_STRING_LENGTH);
    strncpy(s_variable_s[0], "HELLO", MOTOMAN_MAX_STRING_LENGTH - 1);
    memset(s_variable_s[1], 0, MOTOMAN_MAX_STRING_LENGTH);
    strncpy(s_variable_s[1], "WORLD", MOTOMAN_MAX_STRING_LENGTH - 1);
    memset(s_variable_s[2], 0, MOTOMAN_MAX_STRING_LENGTH);
    strncpy(s_variable_s[2], "ROBOT", MOTOMAN_MAX_STRING_LENGTH - 1);
    
    // Initialize Variable P (Position variables) - per Manual 165838-1CD, Table 5-16
    // Attributes 1-13: 1=Data type, 2-9=Axis data, 10-13=Config/Tool/UserCoord/ExtConfig
    if (s_variable_p) {
        // P[0] - Home position (Pulse type)
        s_variable_p[0][0] = 0;      // Attribute 1: Data type = 0 (Pulse)
        s_variable_p[0][1] = 1;      // Attribute 2: 1st axis (S) = 0
        s_variable_p[0][2] = 0;      // Attribute 3: 2nd axis (L) = 0
        s_variable_p[0][3] = 0;      // Attribute 4: 3rd axis (U) = 0
        s_variable_p[0][4] = 0;      // Attribute 5: 4th axis (R) = 0
        s_variable_p[0][5] = 0;      // Attribute 6: 5th axis (B) = 0
        s_variable_p[0][6] = 5;      // Attribute 7: 6th axis (T) = 0
        s_variable_p[0][7] = 0;      // Attribute 8: 7th axis = 0
        s_variable_p[0][8] = 0;      // Attribute 9: 8th axis = 0
        s_variable_p[0][9] = 0;      // Attribute 10: Configuration = 0
        s_variable_p[0][10] = 0;     // Attribute 11: Tool number = 0
        s_variable_p[0][11] = 0;     // Attribute 12: User coordinate number = 0
        s_variable_p[0][12] = 0;     // Attribute 13: Extended configuration = 0
        
        // P[1] - Example position (Base type) - realistic robot position
        s_variable_p[1][0] = 16;     // Attribute 1: Data type = 16 (Base)
        s_variable_p[1][1] = 500000; // Attribute 2: X = 500.000 mm (in μm)
        s_variable_p[1][2] = 300000; // Attribute 3: Y = 300.000 mm (in μm)
        s_variable_p[1][3] = 1200000; // Attribute 4: Z = 1200.000 mm (in μm)
        s_variable_p[1][4] = 900000; // Attribute 5: RX = 90.0000 deg (in 0.0001°)
        s_variable_p[1][5] = 180000; // Attribute 6: RY = 18.0000 deg (in 0.0001°)
        s_variable_p[1][6] = 450000; // Attribute 7: RZ = 45.0000 deg (in 0.0001°)
        s_variable_p[1][7] = 0;      // Attribute 8: 7th axis = 0
        s_variable_p[1][8] = 0;      // Attribute 9: 8th axis = 0
        s_variable_p[1][9] = 0x0000; // Attribute 10: Configuration = 0
        s_variable_p[1][10] = 1;     // Attribute 11: Tool number = 1
        s_variable_p[1][11] = 0;     // Attribute 12: User coordinate number = 0
        s_variable_p[1][12] = 0;     // Attribute 13: Extended configuration = 0
    }
    
    // Initialize Variable BP (Base Position variables) - per Manual 165838-1CD, Table 5-17
    // Attributes 1-9: 1=Data type, 2-9=1st-8th axis data
    if (s_variable_bp) {
        // BP[0] - Home position (Pulse type)
        s_variable_bp[0][0] = 0;      // Attribute 1: Data type = 0 (Pulse)
        s_variable_bp[0][1] = 0;      // Attribute 2: 1st axis = 0
        s_variable_bp[0][2] = 0;      // Attribute 3: 2nd axis = 0
        s_variable_bp[0][3] = 0;      // Attribute 4: 3rd axis = 0
        s_variable_bp[0][4] = 0;      // Attribute 5: 4th axis = 0
        s_variable_bp[0][5] = 0;      // Attribute 6: 5th axis = 0
        s_variable_bp[0][6] = 0;      // Attribute 7: 6th axis = 0
        s_variable_bp[0][7] = 0;      // Attribute 8: 7th axis = 0
        s_variable_bp[0][8] = 0;      // Attribute 9: 8th axis = 0
        
        // BP[1] - Example position (Base type)
        s_variable_bp[1][0] = 16;     // Attribute 1: Data type = 16 (Base)
        s_variable_bp[1][1] = 500000; // Attribute 2: X = 500.000 mm (in μm)
        s_variable_bp[1][2] = 600000; // Attribute 3: Y = 600.000 mm (in μm)
        s_variable_bp[1][3] = 700000; // Attribute 4: Z = 700.000 mm (in μm)
        s_variable_bp[1][4] = 0;      // Attribute 5: RX = 0
        s_variable_bp[1][5] = 0;      // Attribute 6: RY = 0
        s_variable_bp[1][6] = 0;      // Attribute 7: RZ = 0
        s_variable_bp[1][7] = 0;      // Attribute 8: 7th axis = 0
        s_variable_bp[1][8] = 0;      // Attribute 9: 8th axis = 0
    }
    
    // Initialize Variable EX (External Axis variables) - per Manual 165838-1CD, Table 5-18
    // Attributes 1-9: 1=Data type, 2-9=1st-8th axis data
    if (s_variable_ex) {
        // EX[0] - Home position (Pulse type)
        s_variable_ex[0][0] = 0;      // Attribute 1: Data type = 0 (Pulse)
        s_variable_ex[0][1] = 0;      // Attribute 2: 1st external axis = 0
        s_variable_ex[0][2] = 0;      // Attribute 3: 2nd external axis = 0
        s_variable_ex[0][3] = 0;      // Attribute 4: 3rd external axis = 0
        s_variable_ex[0][4] = 0;      // Attribute 5: 4th external axis = 0
        s_variable_ex[0][5] = 0;      // Attribute 6: 5th external axis = 0
        s_variable_ex[0][6] = 0;      // Attribute 7: 6th external axis = 0
        s_variable_ex[0][7] = 0;      // Attribute 8: 7th external axis = 0
        s_variable_ex[0][8] = 0;      // Attribute 9: 8th external axis = 0
        
        // EX[1] - Example position
        s_variable_ex[1][0] = 0;      // Attribute 1: Data type = 0 (Pulse)
        s_variable_ex[1][1] = 10000;  // Attribute 2: 1st external axis = 10000 pulses
        s_variable_ex[1][2] = 20000;  // Attribute 3: 2nd external axis = 20000 pulses
        s_variable_ex[1][3] = 0;      // Attribute 4: 3rd external axis = 0
        s_variable_ex[1][4] = 0;      // Attribute 5: 4th external axis = 0
        s_variable_ex[1][5] = 0;      // Attribute 6: 5th external axis = 0
        s_variable_ex[1][6] = 0;      // Attribute 7: 6th external axis = 0
        s_variable_ex[1][7] = 0;      // Attribute 8: 7th external axis = 0
        s_variable_ex[1][8] = 0;      // Attribute 9: 8th external axis = 0
    }
    
    // Initialize Alarm History (per Manual 165838-1CD, Table 5-2)
    // Instance ranges: 1-100 (major), 1001-1100 (minor), 2001-2100 (User System),
    //                  3001-3100 (User), 4001-4100 (Offline)
    // Alarm code: 0 to 9999, Alarm data: 4 bytes
    // Recent alarm history entries (major alarms)
    s_alarm_history[0].code = 2100;
    s_alarm_history[0].data = 1;
    s_alarm_history[0].data_type = 0;
    memset(s_alarm_history[0].date_time, 0, sizeof(s_alarm_history[0].date_time));
    memcpy(s_alarm_history[0].date_time, "2024/01/15 10:30", 16);
    memset(s_alarm_history[0].string, 0, sizeof(s_alarm_history[0].string));
    memcpy(s_alarm_history[0].string, "ALARM 2100", 10);
    s_alarm_history[1].code = 2101;
    s_alarm_history[1].data = 0;
    s_alarm_history[1].data_type = 0;
    memset(s_alarm_history[1].date_time, 0, sizeof(s_alarm_history[1].date_time));
    memcpy(s_alarm_history[1].date_time, "2024/01/15 11:00", 16);
    memset(s_alarm_history[1].string, 0, sizeof(s_alarm_history[1].string));
    memcpy(s_alarm_history[1].string, "ALARM 2101", 10);
    s_alarm_history[2].code = 2200;
    s_alarm_history[2].data = 2;
    s_alarm_history[2].data_type = 0;
    memset(s_alarm_history[2].date_time, 0, sizeof(s_alarm_history[2].date_time));
    memcpy(s_alarm_history[2].date_time, "2024/01/15 12:00", 16);
    memset(s_alarm_history[2].string, 0, sizeof(s_alarm_history[2].string));
    memcpy(s_alarm_history[2].string, "ALARM 2200", 10);
}

static void EncodeMotomanAlarmDateTime16(const void *const data, ENIPMessage *const outgoing_message) {
    const char *date_time = (const char *)data;
    memcpy(outgoing_message->current_message_position, date_time, 16);
    outgoing_message->current_message_position += 16;
    outgoing_message->used_message_length += 16;
}

static void EncodeMotomanAlarmString32(const void *const data, ENIPMessage *const outgoing_message) {
    const char *alarm_string = (const char *)data;
    memcpy(outgoing_message->current_message_position, alarm_string, 32);
    outgoing_message->current_message_position += 32;
    outgoing_message->used_message_length += 32;
}

static EipStatus GetAttributeAllPositionOrder(CipInstance *RESTRICT const instance,
                                              CipMessageRouterRequest *const message_router_request,
                                              CipMessageRouterResponse *const message_router_response,
                                              const struct sockaddr *originator_address,
                                              const CipSessionHandle encapsulation_session) {
    (void)originator_address;
    (void)encapsulation_session;

    InitializeENIPMessage(&message_router_response->message);
    GenerateGetAttributeSingleHeader(message_router_request, message_router_response);
    message_router_response->general_status = kCipErrorSuccess;

    if (0 == instance->cip_class->number_of_attributes) {
        message_router_response->general_status = kCipErrorServiceNotSupported;
        message_router_response->size_of_additional_status = 0;
        return kEipStatusOkSend;
    }

    EipUint16 attr_order[] = {1, 10, 11, 12, 13, 2, 3, 4, 5, 6, 7, 8, 9};
    for (size_t i = 0; i < sizeof(attr_order) / sizeof(attr_order[0]); i++) {
        EipUint16 attr_num = attr_order[i];
        size_t index = attr_num / 8;
        if ((instance->cip_class->get_all_bit_mask[index]) & (1 << (attr_num % 8))) {
            CipAttributeStruct *attribute = GetCipAttribute(instance, attr_num);
            if (attribute != NULL && attribute->data != NULL) {
                message_router_request->request_path.attribute_number = attr_num;
                attribute->encode(attribute->data, &message_router_response->message);
            }
        }
    }

    return kEipStatusOkSend;
}

static EipStatus SetAttributeAll(CipInstance *RESTRICT const instance,
                                 CipMessageRouterRequest *const message_router_request,
                                 CipMessageRouterResponse *const message_router_response,
                                 const struct sockaddr *originator_address,
                                 const CipSessionHandle encapsulation_session) {
    (void)originator_address;
    (void)encapsulation_session;

    GenerateSetAttributeSingleHeader(message_router_request, message_router_response);
    message_router_response->general_status = kCipErrorSuccess;

    if (0 == instance->cip_class->number_of_attributes) {
        message_router_response->general_status = kCipErrorServiceNotSupported;
        message_router_response->size_of_additional_status = 0;
        return kEipStatusOkSend;
    }

    for (EipUint16 attr_num = 1; attr_num <= instance->cip_class->highest_attribute_number; attr_num++) {
        size_t index = attr_num / 8;
        uint8_t set_bit_mask = instance->cip_class->set_bit_mask[index];
        if (0 != (set_bit_mask & (1 << (attr_num % 8)))) {
            CipAttributeStruct *attribute = GetCipAttribute(instance, attr_num);
            if (attribute != NULL && attribute->data != NULL && attribute->decode != NULL) {
                if ((attribute->attribute_flags == kGetableAllDummy) ||
                    (attribute->attribute_flags == kNotSetOrGetable) ||
                    (attribute->attribute_flags == kGetableAll)) {
                    continue;
                }

                if ((attribute->attribute_flags & kPreSetFunc) &&
                    NULL != instance->cip_class->PreSetCallback) {
                    instance->cip_class->PreSetCallback(instance, attribute, message_router_request->service);
                }

                message_router_request->request_path.attribute_number = attr_num;
                attribute->decode(attribute->data, message_router_request, message_router_response);

                if ((attribute->attribute_flags & (kPostSetFunc | kNvDataFunc)) &&
                    NULL != instance->cip_class->PostSetCallback) {
                    instance->cip_class->PostSetCallback(instance, attribute, message_router_request->service);
                }

                if (message_router_response->general_status != kCipErrorSuccess) {
                    break;
                }
            }
        }
    }

    return kEipStatusOkSend;
}

static void CreateMotomanAlarmClass(void) {
    CipClass *alarm_class = CreateCipClass(MOTOMAN_CLASS_ALARM, 0, 7, 2, 5, 5, 2, MOTOMAN_MAX_ACTIVE_ALARMS, "MotomanAlarm", 1, NULL);
    if (alarm_class != NULL && alarm_class->instances != NULL) {
        CipInstance *instance = alarm_class->instances;
        int i = 0;
        while (instance != NULL && i < MOTOMAN_MAX_ACTIVE_ALARMS) {
            InsertAttribute(instance, 1, kCipUdint, EncodeCipUdint, NULL, 
                           &s_active_alarms[i].code, kGetableSingleAndAll);
            InsertAttribute(instance, 2, kCipUdint, EncodeCipUdint, NULL, 
                           &s_active_alarms[i].data, kGetableSingleAndAll);
            InsertAttribute(instance, 3, kCipUdint, EncodeCipUdint, NULL, 
                           &s_active_alarms[i].data_type, kGetableSingleAndAll);
            InsertAttribute(instance, 4, 0xFF, EncodeMotomanAlarmDateTime16, NULL, 
                           s_active_alarms[i].date_time, kGetableSingleAndAll);
            InsertAttribute(instance, 5, 0xFF, EncodeMotomanAlarmString32, NULL, 
                           s_active_alarms[i].string, kGetableSingleAndAll);
            instance = instance->next;
            i++;
        }
        InsertService(alarm_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(alarm_class, kGetAttributeAll, &GetAttributeAll, "GetAttributeAll");
    }
}

static void CreateMotomanAlarmHistoryClass(void) {
    CipClass *alarm_history_class = CreateCipClass(MOTOMAN_CLASS_ALARM_HISTORY, 0, 7, 2, 5, 5, 2, MOTOMAN_ALARM_HISTORY_SIZE, "MotomanAlarmHistory", 1, NULL);
    if (alarm_history_class != NULL && alarm_history_class->instances != NULL) {
        CipInstance *instance = alarm_history_class->instances;
        int i = 0;
        while (instance != NULL && i < MOTOMAN_ALARM_HISTORY_SIZE) {
            InsertAttribute(instance, 1, kCipUdint, EncodeCipUdint, NULL, 
                           &s_alarm_history[i].code, kGetableSingleAndAll);
            InsertAttribute(instance, 2, kCipUdint, EncodeCipUdint, NULL, 
                           &s_alarm_history[i].data, kGetableSingleAndAll);
            InsertAttribute(instance, 3, kCipUdint, EncodeCipUdint, NULL, 
                           &s_alarm_history[i].data_type, kGetableSingleAndAll);
            InsertAttribute(instance, 4, 0xFF, EncodeMotomanAlarmDateTime16, NULL, 
                           s_alarm_history[i].date_time, kGetableSingleAndAll);
            InsertAttribute(instance, 5, 0xFF, EncodeMotomanAlarmString32, NULL, 
                           s_alarm_history[i].string, kGetableSingleAndAll);
            instance = instance->next;
            i++;
        }
        InsertService(alarm_history_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(alarm_history_class, kGetAttributeAll, &GetAttributeAll, "GetAttributeAll");
    }
}

static void CreateMotomanStatusClass(void) {
    CipClass *status_class = CreateCipClass(MOTOMAN_CLASS_STATUS, 0, 7, 2, 2, 2, 2, 1, "MotomanStatus", 1, NULL);
    if (status_class != NULL && status_class->instances != NULL) {
        CipInstance *instance = status_class->instances;
        InsertAttribute(instance, 1, kCipUdint, EncodeCipUdint, NULL, 
                       &s_status_data1, kGetableSingleAndAll);
        InsertAttribute(instance, 2, kCipUdint, EncodeCipUdint, NULL, 
                       &s_status_data2, kGetableSingleAndAll);
        InsertService(status_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(status_class, kGetAttributeAll, &GetAttributeAll, "GetAttributeAll");
    }
}

static void EncodeMotomanJobName32(const void *const data, ENIPMessage *const outgoing_message) {
    const EipUint8 *job_name = (const EipUint8 *)data;
    memcpy(outgoing_message->current_message_position, job_name, 32);
    outgoing_message->current_message_position += 32;
    outgoing_message->used_message_length += 32;
}

static void CreateMotomanJobInfoClass(void) {
    CipClass *job_info_class = CreateCipClass(MOTOMAN_CLASS_JOB_INFO, 0, 7, 2, 4, 4, 2, 1, "MotomanJobInfo", 1, NULL);
    if (job_info_class != NULL && job_info_class->instances != NULL) {
        CipInstance *instance = job_info_class->instances;
        InsertAttribute(instance, 1, 0xFF, EncodeMotomanJobName32, NULL, 
                       s_job_name, kGetableSingleAndAll);
        InsertAttribute(instance, 2, kCipUdint, EncodeCipUdint, NULL, 
                       &s_job_line, kGetableSingleAndAll);
        InsertAttribute(instance, 3, kCipUdint, EncodeCipUdint, NULL, 
                       &s_step_number, kGetableSingleAndAll);
        InsertAttribute(instance, 4, kCipUdint, EncodeCipUdint, NULL, 
                       &s_speed_override, kGetableSingleAndAll);
        InsertService(job_info_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(job_info_class, kGetAttributeAll, &GetAttributeAll, "GetAttributeAll");
    }
}

static void CreateMotomanAxisConfigClass(void) {
    CipClass *axis_config_class = CreateCipClass(MOTOMAN_CLASS_AXIS_CONFIG, 0, 7, 2, 2, 2, 1, 1, "MotomanAxisConfig", 1, NULL);
    if (axis_config_class != NULL && axis_config_class->instances != NULL) {
        CipInstance *instance = axis_config_class->instances;
        InsertAttribute(instance, 1, kCipUsint, EncodeCipUsint, NULL, 
                       &s_axis_count, kGetableSingle);
        InsertAttribute(instance, 2, kCipByte, EncodeCipByte, NULL, 
                       s_axis_type, kGetableSingle);
        InsertService(axis_config_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
    }
}

static void CreateMotomanPositionClass(void) {
    // Per Manual 165838-1CD, Table 5-6: Instances 1-8, 11-18, 21-44, 101-108
    // Attributes 1-13: 1=Data type, 2-9=Axis data, 10-13=Config/Tool/Reservation/ExtConfig
    CipClass *position_class = CreateCipClass(MOTOMAN_CLASS_POSITION, 0, 7, 2, MOTOMAN_POSITION_ATTRIBUTES, MOTOMAN_POSITION_ATTRIBUTES, 2, MOTOMAN_MAX_POSITION_INSTANCES, "MotomanPosition", 1, NULL);
    if (position_class != NULL && position_class->instances != NULL && s_position_data != NULL) {
        CipInstance *instance = position_class->instances;
        int inst_num = 1;
        while (instance != NULL && inst_num <= MOTOMAN_MAX_POSITION_INSTANCES) {
            for (EipUint16 attr = 1; attr <= MOTOMAN_POSITION_ATTRIBUTES; attr++) {
                EipInt32 *data_ptr = &s_position_data[inst_num - 1][attr - 1];
                InsertAttribute(instance, attr, kCipDint, EncodeCipDint, NULL, 
                               data_ptr, kGetableSingleAndAll);
            }
            instance = instance->next;
            inst_num++;
        }
        InsertService(position_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(position_class, kGetAttributeAll, &GetAttributeAllPositionOrder, "GetAttributeAll");
    }
}

static void CreateMotomanPositionDeviationClass(void) {
    CipClass *position_deviation_class = CreateCipClass(MOTOMAN_CLASS_POSITION_DEVIATION, 0, 7, 2, MOTOMAN_MAX_AXES, MOTOMAN_MAX_AXES, 2, MOTOMAN_MAX_AXES, "MotomanPositionDeviation", 1, NULL);
    if (position_deviation_class != NULL && position_deviation_class->instances != NULL) {
        CipInstance *instance = position_deviation_class->instances;
        int inst_num = 1;
        while (instance != NULL && inst_num <= MOTOMAN_MAX_AXES) {
            for (EipUint16 attr = 1; attr <= MOTOMAN_MAX_AXES; attr++) {
                InsertAttribute(instance, attr, kCipDint, EncodeCipDint, NULL, 
                               &s_position_deviation[attr - 1], kGetableSingleAndAll);
            }
            instance = instance->next;
            inst_num++;
        }
        InsertService(position_deviation_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(position_deviation_class, kGetAttributeAll, &GetAttributeAll, "GetAttributeAll");
    }
}

static void CreateMotomanTorqueClass(void) {
    CipClass *torque_class = CreateCipClass(MOTOMAN_CLASS_TORQUE, 0, 7, 2, MOTOMAN_MAX_AXES, MOTOMAN_MAX_AXES, 2, MOTOMAN_MAX_AXES, "MotomanTorque", 1, NULL);
    if (torque_class != NULL && torque_class->instances != NULL) {
        CipInstance *instance = torque_class->instances;
        int inst_num = 1;
        while (instance != NULL && inst_num <= MOTOMAN_MAX_AXES) {
            for (EipUint16 attr = 1; attr <= MOTOMAN_MAX_AXES; attr++) {
                InsertAttribute(instance, attr, kCipDint, EncodeCipDint, NULL, 
                               &s_torque[attr - 1], kGetableSingleAndAll);
            }
            instance = instance->next;
            inst_num++;
        }
        InsertService(torque_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(torque_class, kGetAttributeAll, &GetAttributeAll, "GetAttributeAll");
    }
}

static void CreateMotomanIOClass(void) {
    CipClass *io_class = CreateCipClass(MOTOMAN_CLASS_IO, 0, 7, 2, 1, 1, 2, MOTOMAN_MAX_IO_SIGNALS, "MotomanIO", 1, NULL);
    if (io_class != NULL && io_class->instances != NULL) {
        CipInstance *instance = io_class->instances;
        int i = 0;
        while (instance != NULL && i < MOTOMAN_MAX_IO_SIGNALS) {
            InsertAttribute(instance, 1, kCipUsint, EncodeCipUsint, (CipAttributeDecodeFromMessage)DecodeCipUsint, 
                           &s_io_data[i], kSetAndGetAble);
            instance = instance->next;
            i++;
        }
        InsertService(io_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(io_class, kSetAttributeSingle, &SetAttributeSingle, "SetAttributeSingle");
    }
}

static void CreateMotomanRegisterClass(void) {
    // Note: In CIP, instance 0 is reserved for the class object, so we cannot create a data instance 0.
    // RS022=1: Instance 1 maps to Register[0], Instance 2 maps to Register[1], etc. (instance N = Register[N-1])
    // RS022=0: Instance 1 maps to Register[0], Instance 2 maps to Register[1], etc. (instance N = Register[N-1])
    // Both modes map the same way due to CIP's instance 0 reservation.
    CipClass *register_class = CreateCipClass(MOTOMAN_CLASS_REGISTER, 0, 7, 2, 1, 1, 2, MOTOMAN_MAX_REGISTERS, "MotomanRegister", 1, NULL);
    if (register_class != NULL && s_registers != NULL) {
        CipInstance *instance = register_class->instances;
        while (instance != NULL) {
            int array_idx = GetArrayIndexFromInstance(instance->instance_number);
            if (array_idx >= 0 && array_idx < MOTOMAN_MAX_REGISTERS) {
                InsertAttribute(instance, 1, kCipUint, EncodeCipUint, (CipAttributeDecodeFromMessage)DecodeCipUint, 
                               &s_registers[array_idx], kSetAndGetAble);
            }
            instance = instance->next;
        }
        InsertService(register_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(register_class, kSetAttributeSingle, &SetAttributeSingle, "SetAttributeSingle");
    }
}

static void CreateMotomanVariableBClass(void) {
    // Note: In CIP, instance 0 is reserved for the class object, so instance N maps to variable[N-1]
    CipClass *var_b_class = CreateCipClass(MOTOMAN_CLASS_VARIABLE_B, 0, 7, 2, 1, 1, 2, MOTOMAN_MAX_VARIABLES, "MotomanVariableB", 1, NULL);
    if (var_b_class != NULL && s_variable_b != NULL) {
        CipInstance *instance = var_b_class->instances;
        while (instance != NULL) {
            int array_idx = GetArrayIndexFromInstance(instance->instance_number);
            if (array_idx >= 0 && array_idx < MOTOMAN_MAX_VARIABLES) {
                InsertAttribute(instance, 1, kCipUsint, EncodeCipUsint, (CipAttributeDecodeFromMessage)DecodeCipUsint, 
                               &s_variable_b[array_idx], kSetAndGetAble);
            }
            instance = instance->next;
        }
        InsertService(var_b_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(var_b_class, kSetAttributeSingle, &SetAttributeSingle, "SetAttributeSingle");
    }
}

static void CreateMotomanVariableIClass(void) {
    // Note: In CIP, instance 0 is reserved for the class object, so instance N maps to variable[N-1]
    CipClass *var_i_class = CreateCipClass(MOTOMAN_CLASS_VARIABLE_I, 0, 7, 2, 1, 1, 2, MOTOMAN_MAX_VARIABLES, "MotomanVariableI", 1, NULL);
    if (var_i_class != NULL && s_variable_i != NULL) {
        CipInstance *instance = var_i_class->instances;
        while (instance != NULL) {
            int array_idx = GetArrayIndexFromInstance(instance->instance_number);
            if (array_idx >= 0 && array_idx < MOTOMAN_MAX_VARIABLES) {
                InsertAttribute(instance, 1, kCipInt, EncodeCipInt, (CipAttributeDecodeFromMessage)DecodeCipInt, 
                               &s_variable_i[array_idx], kSetAndGetAble);
            }
            instance = instance->next;
        }
        InsertService(var_i_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(var_i_class, kSetAttributeSingle, &SetAttributeSingle, "SetAttributeSingle");
    }
}

static void CreateMotomanVariableDClass(void) {
    // Note: In CIP, instance 0 is reserved for the class object, so instance N maps to variable[N-1]
    CipClass *var_d_class = CreateCipClass(MOTOMAN_CLASS_VARIABLE_D, 0, 7, 2, 1, 1, 2, MOTOMAN_MAX_VARIABLES, "MotomanVariableD", 1, NULL);
    if (var_d_class != NULL && s_variable_d != NULL) {
        CipInstance *instance = var_d_class->instances;
        while (instance != NULL) {
            int array_idx = GetArrayIndexFromInstance(instance->instance_number);
            if (array_idx >= 0 && array_idx < MOTOMAN_MAX_VARIABLES) {
                InsertAttribute(instance, 1, kCipDint, EncodeCipDint, (CipAttributeDecodeFromMessage)DecodeCipDint, 
                               &s_variable_d[array_idx], kSetAndGetAble);
            }
            instance = instance->next;
        }
        InsertService(var_d_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(var_d_class, kSetAttributeSingle, &SetAttributeSingle, "SetAttributeSingle");
    }
}

static void CreateMotomanVariableRClass(void) {
    // Note: In CIP, instance 0 is reserved for the class object, so instance N maps to variable[N-1]
    CipClass *var_r_class = CreateCipClass(MOTOMAN_CLASS_VARIABLE_R, 0, 7, 2, 1, 1, 2, MOTOMAN_MAX_VARIABLES, "MotomanVariableR", 1, NULL);
    if (var_r_class != NULL && s_variable_r != NULL) {
        CipInstance *instance = var_r_class->instances;
        while (instance != NULL) {
            int array_idx = GetArrayIndexFromInstance(instance->instance_number);
            if (array_idx >= 0 && array_idx < MOTOMAN_MAX_VARIABLES) {
                InsertAttribute(instance, 1, kCipReal, EncodeCipReal, (CipAttributeDecodeFromMessage)DecodeCipReal, 
                               &s_variable_r[array_idx], kSetAndGetAble);
            }
            instance = instance->next;
        }
        InsertService(var_r_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(var_r_class, kSetAttributeSingle, &SetAttributeSingle, "SetAttributeSingle");
    }
}

static void CreateMotomanVariableSClass(void) {
    // Note: In CIP, instance 0 is reserved for the class object, so instance N maps to variable[N-1]
    CipClass *var_s_class = CreateCipClass(MOTOMAN_CLASS_VARIABLE_S, 0, 7, 2, 1, 1, 2, MOTOMAN_MAX_VARIABLES, "MotomanVariableS", 1, NULL);
    if (var_s_class != NULL && s_variable_s != NULL) {
        CipInstance *instance = var_s_class->instances;
        while (instance != NULL) {
            int array_idx = GetArrayIndexFromInstance(instance->instance_number);
            if (array_idx >= 0 && array_idx < MOTOMAN_MAX_VARIABLES) {
                InsertAttribute(instance, 1, 0xFF, EncodeMotomanString32, (CipAttributeDecodeFromMessage)DecodeMotomanString32, 
                               &s_variable_s[array_idx], kSetAndGetAble);
            }
            instance = instance->next;
        }
        InsertService(var_s_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(var_s_class, kSetAttributeSingle, &SetAttributeSingle, "SetAttributeSingle");
    }
}

static void CreateMotomanVariablePClass(void) {
    // Per Manual 165838-1CD, Table 5-16: Attributes 1-13 (same as Position class)
    // Attribute 1: Data type, Attributes 2-9: Axis data, Attributes 10-13: Config/Tool/UserCoord/ExtConfig
    // RS022=1: Instance 1 = P[1], Instance 2 = P[2], etc.
    // RS022=0: Instance 1 = P[0], Instance 2 = P[1], etc.
    CipClass *var_p_class = CreateCipClass(MOTOMAN_CLASS_VARIABLE_P, 0, 7, 2, MOTOMAN_VARIABLE_P_ATTRIBUTES, MOTOMAN_VARIABLE_P_ATTRIBUTES, 4, MOTOMAN_MAX_VARIABLE_P, "MotomanVariableP", 1, NULL);
    if (var_p_class != NULL && s_variable_p != NULL) {
        CipInstance *instance = var_p_class->instances;
        while (instance != NULL) {
            int array_idx = GetArrayIndexFromInstance(instance->instance_number);
            if (array_idx >= 0 && array_idx < MOTOMAN_MAX_VARIABLE_P) {
                for (EipUint16 attr = 1; attr <= MOTOMAN_VARIABLE_P_ATTRIBUTES; attr++) {
                    EipInt32 *data_ptr = &s_variable_p[array_idx][attr - 1];
                    InsertAttribute(instance, attr, kCipDint, EncodeCipDint, (CipAttributeDecodeFromMessage)DecodeCipDint, 
                                   data_ptr, kSetAndGetAble | kGetableAll);
                }
            }
            instance = instance->next;
        }
        InsertService(var_p_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(var_p_class, kGetAttributeAll, &GetAttributeAllPositionOrder, "GetAttributeAll");
        InsertService(var_p_class, kSetAttributeSingle, &SetAttributeSingle, "SetAttributeSingle");
        InsertService(var_p_class, kSetAttributeAll, &SetAttributeAll, "SetAttributeAll");
    }
}

static void CreateMotomanVariableBPClass(void) {
    // Per Manual 165838-1CD, Table 5-17: Attributes 1-9
    // Attribute 1: Data type, Attributes 2-9: 1st-8th axis data
    // Note: In CIP, instance 0 is reserved for the class object, so instance N maps to variable[N-1]
    CipClass *var_bp_class = CreateCipClass(MOTOMAN_CLASS_VARIABLE_BP, 0, 7, 2, MOTOMAN_VARIABLE_BP_ATTRIBUTES, MOTOMAN_VARIABLE_BP_ATTRIBUTES, 2, MOTOMAN_MAX_VARIABLES, "MotomanVariableBP", 1, NULL);
    if (var_bp_class != NULL && s_variable_bp != NULL) {
        CipInstance *instance = var_bp_class->instances;
        while (instance != NULL) {
            int array_idx = GetArrayIndexFromInstance(instance->instance_number);
            if (array_idx >= 0 && array_idx < MOTOMAN_MAX_VARIABLES) {
                for (EipUint16 attr = 1; attr <= MOTOMAN_VARIABLE_BP_ATTRIBUTES; attr++) {
                    EipInt32 *data_ptr = &s_variable_bp[array_idx][attr - 1];
                    InsertAttribute(instance, attr, kCipDint, EncodeCipDint, (CipAttributeDecodeFromMessage)DecodeCipDint, 
                                   data_ptr, kSetAndGetAble);
                }
            }
            instance = instance->next;
        }
        InsertService(var_bp_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(var_bp_class, kSetAttributeSingle, &SetAttributeSingle, "SetAttributeSingle");
    }
}

static void CreateMotomanVariableEXClass(void) {
    // Per Manual 165838-1CD, Table 5-18: Attributes 1-9
    // Attribute 1: Data type, Attributes 2-9: 1st-8th axis data
    // Note: In CIP, instance 0 is reserved for the class object, so instance N maps to variable[N-1]
    CipClass *var_ex_class = CreateCipClass(MOTOMAN_CLASS_VARIABLE_EX, 0, 7, 2, MOTOMAN_VARIABLE_EX_ATTRIBUTES, MOTOMAN_VARIABLE_EX_ATTRIBUTES, 2, MOTOMAN_MAX_VARIABLES, "MotomanVariableEX", 1, NULL);
    if (var_ex_class != NULL && s_variable_ex != NULL) {
        CipInstance *instance = var_ex_class->instances;
        while (instance != NULL) {
            int array_idx = GetArrayIndexFromInstance(instance->instance_number);
            if (array_idx >= 0 && array_idx < MOTOMAN_MAX_VARIABLES) {
                for (EipUint16 attr = 1; attr <= MOTOMAN_VARIABLE_EX_ATTRIBUTES; attr++) {
                    EipInt32 *data_ptr = &s_variable_ex[array_idx][attr - 1];
                    InsertAttribute(instance, attr, kCipDint, EncodeCipDint, (CipAttributeDecodeFromMessage)DecodeCipDint, 
                                   data_ptr, kSetAndGetAble);
                }
            }
            instance = instance->next;
        }
        InsertService(var_ex_class, kGetAttributeSingle, &GetAttributeSingle, "GetAttributeSingle");
        InsertService(var_ex_class, kSetAttributeSingle, &SetAttributeSingle, "SetAttributeSingle");
    }
}

EipStatus ApplicationInitialization(void) {
    // Load RS022 configuration (defaults to true/RS022=1)
    system_motoman_rs022_load(&s_rs022_enabled);
    ESP_LOGI(TAG, "RS022 instance mapping: %s (instance 0 %s)", 
             s_rs022_enabled ? "enabled" : "disabled",
             s_rs022_enabled ? "allowed" : "not allowed");
    
    // Allocate robot data arrays from PSRAM first
    if (!AllocateRobotDataArrays()) {
        return kEipStatusError;
    }
    
    // Initialize all robot data with realistic values
    InitializeRobotData();
    
    CreateMotomanAlarmClass();
    CreateMotomanAlarmHistoryClass();
    CreateMotomanStatusClass();
    CreateMotomanJobInfoClass();
    CreateMotomanAxisConfigClass();
    CreateMotomanPositionClass();
    CreateMotomanPositionDeviationClass();
    CreateMotomanTorqueClass();
    CreateMotomanIOClass();
    CreateMotomanRegisterClass();
    CreateMotomanVariableBClass();
    CreateMotomanVariableIClass();
    CreateMotomanVariableDClass();
    CreateMotomanVariableRClass();
    CreateMotomanVariableSClass();
    CreateMotomanVariablePClass();
    CreateMotomanVariableBPClass();
    CreateMotomanVariableEXClass();
    
    return kEipStatusOk;
}

void HandleApplication(void) {
}

void CheckIoConnectionEvent(unsigned int output_assembly_id,
                            unsigned int input_assembly_id,
                            IoConnectionEvent io_connection_event) {
    (void) output_assembly_id;
    (void) input_assembly_id;
    (void) io_connection_event;
}

EipStatus AfterAssemblyDataReceived(CipInstance *instance) {
    (void) instance;
    return kEipStatusOk;
}

EipBool8 BeforeAssemblyDataSend(CipInstance *instance) {
    (void) instance;
    return true;
}

EipStatus ResetDevice(void) {
    CloseAllConnections();
    CipQosUpdateUsedSetQosValues();
    return kEipStatusOk;
}

EipStatus ResetDeviceToInitialConfiguration(void) {
    CloseAllConnections();
    CipQosResetAttributesToDefaultValues();
    return kEipStatusOk;
}

void* CipCalloc(size_t number_of_elements, size_t size_of_element) {
    return calloc(number_of_elements, size_of_element);
}

void CipFree(void *data) {
    free(data);
}

void RunIdleChanged(EipUint32 run_idle_value) {
    (void) run_idle_value;
}

void Motoman_EnIP_ApplicationNotifyLinkUp(void) {
}

void Motoman_EnIP_ApplicationNotifyLinkDown(void) {
}

void Motoman_EnIP_ApplicationSetActiveNetif(struct netif *netif) {
    (void) netif;
}

EipStatus EthLnkPreGetCallback(CipInstance *instance,
                                CipAttributeStruct *attribute,
                                CipByte service) {
    (void) instance;
    (void) attribute;
    (void) service;
    return kEipStatusOk;
}

EipStatus EthLnkPostGetCallback(CipInstance *instance,
                                  CipAttributeStruct *attribute,
                                  CipByte service) {
    (void) instance;
    (void) attribute;
    (void) service;
    return kEipStatusOk;
}
