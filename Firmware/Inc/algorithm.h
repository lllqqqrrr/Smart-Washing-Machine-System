#ifndef __ALGORITHM_H__
#define __ALGORITHM_H__

#include <stdint.h>

/* 工作模式参数结构体 */
typedef struct {
    uint16_t base_wash_time;      // 基础洗涤时间(秒)
    uint8_t rinse_count;           // 漂洗次数
    uint8_t spin_time;             // 脱水时间(秒)
    uint8_t target_temp;           // 目标温度(°C)
    uint8_t spin_speed;            // 脱水转速(0-100)
    uint8_t water_level;           // 用水量级别
} WashModeParam_t;

/* 自适应算法 */
typedef struct {
    float turbidity_low;
    float turbidity_mid;
    float turbidity_high;
    float weight_base;
    float time_per_kg;
} AdaptiveParam_t;

/* 安全保护 */
typedef enum {
    SAFETY_OK = 0,
    SAFETY_DOOR_OPEN = 1,
    SAFETY_WATER_LOW = 2,
    SAFETY_WATER_HIGH = 3,
    SAFETY_TEMP_HIGH = 4,
    SAFETY_MOTOR_OVERLOAD = 5,
    SAFETY_LEAK = 6
} SafetyStatus_t;

/* 工作模式参数获取 */
WashModeParam_t* Get_WashMode_Param(uint8_t mode);

/* 自适应计算 */
void Calculate_Adaptive_Params(uint16_t weight, float turbidity, 
                               uint16_t* wash_time, uint8_t* rinse_count);

/* 安全检查 */
SafetyStatus_t Check_Safety_Status(uint8_t door_closed, uint8_t water_level, 
                                    float temp, uint16_t motor_current);

/* PID温度控制 */
void PID_Temperature_Control(float current_temp, float target_temp, 
                             uint8_t* heater_output);

#endif /* __ALGORITHM_H__ */
