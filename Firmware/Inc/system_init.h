#ifndef __SYSTEM_INIT_H__
#define __SYSTEM_INIT_H__

#include "stm32f10x.h"

/* 系统初始化函数声明 */
void System_Init(void);
void GPIO_Init(void);
void UART_Init(void);
void Timer_Init(void);
void EXTI_Init(void);
void ADC_Init(void);
void SysTick_Init(void);

/* UART发送函数 */
void USART_SendString(USART_TypeDef* USARTx, char* str);

#endif /* __SYSTEM_INIT_H__ */
