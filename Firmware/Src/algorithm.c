#include "algorithm.h"
#include <math.h>

/* 工作模式参数定义 */
static const WashModeParam_t g_wash_modes[] = {
    /* 标准模式 */
    {.base_wash_time = 1800, .rinse_count = 3, .spin_time = 180, 
     .target_temp = 45, .spin_speed = 80, .water_level = 3},
    
    /* 快速模式 */
    {.base_wash_time = 900, .rinse_count = 1, .spin_time = 120, 
     .target_temp = 25, .spin_speed = 60, .water_level = 2},
    
    /* 精细模式 */
    {.base_wash_time = 2400, .rinse_count = 5, .spin_time = 240, 
     .target_temp = 30, .spin_speed = 40, .water_level = 4},
    
    /* 强力模式 */
    {.base_wash_time = 2700, .rinse_count = 3, .spin_time = 180, 
     .target_temp = 60, .spin_speed = 100, .water_level = 5}
};

/* 自适应参数 */
static const AdaptiveParam_t g_adaptive = {
    .turbidity_low = 0.5f,
    .turbidity_mid = 1.5f,
    .turbidity_high = 2.5f,
    .weight_base = 2500.0f,  // 2.5kg
    .time_per_kg = 180.0f     // 每kg增加180秒
};

/* PID参数 */
static float kp = 2.0f, ki = 0.5f, kd = 1.0f;
static float integral = 0.0f, prev_error = 0.0f;

/**
 * @brief  获取洗涤模式参数
 */
WashModeParam_t* Get_WashMode_Param(uint8_t mode)
{
    if(mode < 4) {
        return (WashModeParam_t*)&g_wash_modes[mode];
    }
    return (WashModeParam_t*)&g_wash_modes[0];  // 默认返回标准模式
}

/**
 * @brief  计算自适应参数
 */
void Calculate_Adaptive_Params(uint16_t weight, float turbidity, 
                               uint16_t* wash_time, uint8_t* rinse_count)
{
    /* 根据衣物重量调整洗涤时间 */
    float weight_ratio = (float)weight / g_adaptive.weight_base;
    *wash_time = (uint16_t)(1800 + (weight - 2500) * 0.18f);
    
    if(*wash_time < 900) *wash_time = 900;      // 最少15分钟
    if(*wash_time > 3600) *wash_time = 3600;    // 最多60分钟
    
    /* 根据浊度调整漂洗次数 */
    if(turbidity > g_adaptive.turbidity_high) {
        *rinse_count = 5;  // 很脏
    } else if(turbidity > g_adaptive.turbidity_mid) {
        *rinse_count = 3;  // 中等
    } else if(turbidity > g_adaptive.turbidity_low) {
        *rinse_count = 2;  // 轻度
    } else {
        *rinse_count = 1;  // 很洁净
    }
}

/**
 * @brief  安全状态检查
 */
SafetyStatus_t Check_Safety_Status(uint8_t door_closed, uint8_t water_level, 
                                    float temp, uint16_t motor_current)
{
    /* 门状态检查 */
    if(!door_closed) {
        return SAFETY_DOOR_OPEN;
    }
    
    /* 水位检查 */
    if(water_level > 95) {
        return SAFETY_WATER_HIGH;
    }
    if(water_level < 10) {
        return SAFETY_WATER_LOW;
    }
    
    /* 温度检查 */
    if(temp > 75) {
        return SAFETY_TEMP_HIGH;
    }
    
    /* 电机电流检查(过载) */
    if(motor_current > 5000) {  // 5A
        return SAFETY_MOTOR_OVERLOAD;
    }
    
    return SAFETY_OK;
}

/**
 * @brief  PID温度控制
 */
void PID_Temperature_Control(float current_temp, float target_temp, 
                             uint8_t* heater_output)
{
    float error = target_temp - current_temp;
    float dt = 0.1f;  // 采样时间 100ms
    
    /* PID计算 */
    integral += error * dt;
    if(integral > 100) integral = 100;      // 积分限幅
    if(integral < -100) integral = -100;
    
    float derivative = (error - prev_error) / dt;
    float output = kp * error + ki * integral + kd * derivative;
    
    prev_error = error;
    
    /* 输出限制 */
    if(output > 100) output = 100;
    if(output < 0) output = 0;
    
    *heater_output = (uint8_t)output;
}
