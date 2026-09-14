#ifndef __SAFETY_H__
#define __SAFETY_H__

#include <stdint.h>

/* 错误代码定义 */
typedef enum {
    ERR_NONE = 0x00,
    ERR_DOOR_OPEN = 0x01,
    ERR_WATER_LEAK = 0x02,
    ERR_WATER_TEMP = 0x03,
    ERR_MOTOR_OVERLOAD = 0x04,
    ERR_SENSOR_FAIL = 0x05,
    ERR_VALVE_FAIL = 0x06,
    ERR_EEPROM_FAIL = 0x07,
    ERR_COMM_FAIL = 0x08
} ErrorCode_t;

/* 安全检查函数 */
void Safety_Check_Door(uint8_t door_pin);
void Safety_Check_Water_Level(uint8_t water_level);
void Safety_Check_Temperature(float temp);
void Safety_Check_Motor_Current(uint16_t current);
void Safety_Check_Leak(uint8_t leak_pin);
void Safety_Trigger_Alarm(uint8_t error_code);
void Safety_Recovery(void);

#endif /* __SAFETY_H__ */
