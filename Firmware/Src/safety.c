#include "safety.h"
#include "stm32f10x.h"

static ErrorCode_t g_current_error = ERR_NONE;

/**
 * @brief  检查门状态
 */
void Safety_Check_Door(uint8_t door_pin)
{
    /* 如果门打开 */
    if(door_pin == 0) {
        Safety_Trigger_Alarm(ERR_DOOR_OPEN);
    }
}

/**
 * @brief  检查水位
 */
void Safety_Check_Water_Level(uint8_t water_level)
{
    if(water_level > 95) {
        /* 关闭进水阀 */
        GPIO_ResetBits(GPIOB, GPIO_Pin_2);
    }
    
    if(water_level < 5) {
        /* 可能漏水 */
        Safety_Trigger_Alarm(ERR_WATER_LEAK);
    }
}

/**
 * @brief  检查温度
 */
void Safety_Check_Temperature(float temp)
{
    if(temp > 75) {
        /* 关闭加热管 */
        GPIO_ResetBits(GPIOB, GPIO_Pin_4);
        Safety_Trigger_Alarm(ERR_WATER_TEMP);
    }
}

/**
 * @brief  检查电机电流
 */
void Safety_Check_Motor_Current(uint16_t current)
{
    if(current > 5000) {  // 5A
        /* 停止电机 */
        GPIO_ResetBits(GPIOB, GPIO_Pin_0);
        Safety_Trigger_Alarm(ERR_MOTOR_OVERLOAD);
    }
}

/**
 * @brief  检查漏水
 */
void Safety_Check_Leak(uint8_t leak_pin)
{
    if(leak_pin == 0) {
        Safety_Trigger_Alarm(ERR_WATER_LEAK);
    }
}

/**
 * @brief  触发报警
 */
void Safety_Trigger_Alarm(uint8_t error_code)
{
    g_current_error = (ErrorCode_t)error_code;
    
    /* 开启红色LED */
    GPIO_SetBits(GPIOB, GPIO_Pin_6);
    
    /* 启动蜂鸣器 */
    GPIO_SetBits(GPIOB, GPIO_Pin_5);
}

/**
 * @brief  安全恢复
 */
void Safety_Recovery(void)
{
    g_current_error = ERR_NONE;
    
    /* 关闭红色LED */
    GPIO_ResetBits(GPIOB, GPIO_Pin_6);
    
    /* 停止蜂鸣器 */
    GPIO_ResetBits(GPIOB, GPIO_Pin_5);
}
