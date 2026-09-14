#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>

/* 系统状态定义 */
typedef enum {
    SYS_IDLE = 0,
    SYS_RUNNING = 1,
    SYS_PAUSED = 2,
    SYS_ERROR = 3,
    SYS_MAINTENANCE = 4
} SystemState_t;

/* 工作模式定义 */
typedef enum {
    MODE_STANDARD = 0,      // 标准模式
    MODE_QUICK = 1,         // 快速模式
    MODE_GENTLE = 2,        // 精细模式
    MODE_STRONG = 3,        // 强力模式
    MODE_CUSTOM = 4         // 自定义模式
} WashMode_t;

/* 系统状态结构体 */
typedef struct {
    SystemState_t state;
    WashMode_t mode;
    uint16_t remaining_time;   // 秒
    float current_temp;        // 摄氏度
    uint8_t water_level;       // 百分比 0-100
    float turbidity;           // 浊度值
    uint16_t garment_weight;   // 克
    uint8_t error_code;        // 错误代码
    uint8_t spin_speed;        // 脱水转速 0-100
    uint32_t work_count;       // 工作次数
} SystemStatus_t;

/* 全局变量 */
SystemStatus_t g_system;
volatile uint32_t g_sys_tick = 0;
volatile uint8_t g_key_flag = 0;

/* 函数声明 */
void System_Init(void);
void GPIO_Init(void);
void UART_Init(void);
void Timer_Init(void);
void EXTI_Init(void);
void ADC_Init(void);
void SysTick_Init(void);

void Main_Loop(void);
void Process_Sensor_Data(void);
void Update_Display(void);
void Send_Status_To_PC(void);

/* LED控制宏定义 */
#define LED_RED_ON()    GPIO_SetBits(GPIOB, GPIO_Pin_6)
#define LED_RED_OFF()   GPIO_ResetBits(GPIOB, GPIO_Pin_6)
#define LED_GREEN_ON()  GPIO_SetBits(GPIOB, GPIO_Pin_7)
#define LED_GREEN_OFF() GPIO_ResetBits(GPIOB, GPIO_Pin_7)

/* 主程序入口 */
int main(void)
{
    System_Init();
    SysTick_Init();
    
    /* 显示欢迎信息 */
    USART_SendString(USART1, "\r\n==================================\r\n");
    USART_SendString(USART1, "Smart Washing Machine System v1.0\r\n");
    USART_SendString(USART1, "==================================\r\n");
    USART_SendString(USART1, "System initialized successfully!\r\n");
    
    /* 初始化系统状态 */
    g_system.state = SYS_IDLE;
    g_system.mode = MODE_STANDARD;
    g_system.remaining_time = 0;
    g_system.current_temp = 25.0;
    g_system.water_level = 0;
    g_system.turbidity = 0.0;
    g_system.garment_weight = 0;
    g_system.error_code = 0;
    g_system.spin_speed = 0;
    g_system.work_count = 0;
    
    LED_GREEN_ON();
    
    /* 主循环 */
    while(1)
    {
        Main_Loop();
    }
    
    return 0;
}

/**
 * @brief  系统初始化
 */
void System_Init(void)
{
    /* 使能GPIOA、GPIOB、GPIOC时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | 
                           RCC_APB2Periph_GPIOB | 
                           RCC_APB2Periph_GPIOC, ENABLE);
    
    /* 使能UART、Timer、ADC时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | 
                           RCC_APB1Periph_TIM3 | 
                           RCC_APB1Periph_TIM4, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    
    GPIO_Init();
    UART_Init();
    Timer_Init();
    EXTI_Init();
    ADC_Init();
}

/**
 * @brief  GPIO初始化
 */
void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* 配置输出引脚 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    /* PB6-PB8: LED指示灯 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    /* PB0-PB5: 控制输出(电机、阀门等) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | 
                                  GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    /* 配置输入引脚 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 内部上拉
    
    /* PA1: 门开关中断 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* 配置ADC输入引脚 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;  // 模拟输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | 
                                  GPIO_Pin_7;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/**
 * @brief  UART初始化
 */
void UART_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    
    /* 配置UART1的GPIO */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;     // TX
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;    // RX
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* USART1配置 */
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);
    
    USART_Cmd(USART1, ENABLE);
    
    /* 启用UART中断 */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    NVIC_EnableIRQ(USART1_IRQn);
}

/**
 * @brief  定时器初始化
 */
void Timer_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    
    /* Timer2: 传感器采样(1ms周期) */
    TIM_TimeBaseStructure.TIM_Period = 72 - 1;        // 1ms
    TIM_TimeBaseStructure.TIM_Prescaler = 1000 - 1;   // 72MHz / 1000 = 72kHz
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
    
    /* Timer3: 按键消抖(20ms周期) */
    TIM_TimeBaseStructure.TIM_Period = 1440 - 1;      // 20ms
    TIM_TimeBaseStructure.TIM_Prescaler = 1000 - 1;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
    
    /* Timer4: 系统计时(100Hz) */
    TIM_TimeBaseStructure.TIM_Period = 7200 - 1;      // 100ms
    TIM_TimeBaseStructure.TIM_Prescaler = 100 - 1;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
    
    /* 配置中断优先级 */
    NVIC_EnableIRQ(TIM2_IRQn);
    NVIC_EnableIRQ(TIM3_IRQn);
    NVIC_EnableIRQ(TIM4_IRQn);
}

/**
 * @brief  外部中断初始化
 */
void EXTI_Init(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    /* 使能AFIO时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    
    /* PA1: 门开关 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);
    EXTI_InitStructure.EXTI_Line = EXTI_Line1;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // 下降沿触发
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    
    /* 配置中断优先级 */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/**
 * @brief  ADC初始化
 */
void ADC_Init(void)
{
    ADC_InitTypeDef ADC_InitStructure;
    
    /* ADC1配置 */
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 4;
    ADC_Init(ADC1, &ADC_InitStructure);
    
    /* 配置ADC通道 */
    ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 1, ADC_SampleTime_55Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 2, ADC_SampleTime_55Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 3, ADC_SampleTime_55Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_7, 4, ADC_SampleTime_55Cycles5);
    
    /* 启用ADC */
    ADC_Cmd(ADC1, ENABLE);
    
    /* ADC校准 */
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}

/**
 * @brief  SysTick初始化
 */
void SysTick_Init(void)
{
    if(SysTick_Config(SystemCoreClock / 1000)) // 1ms中断
    {
        while(1);
    }
}

/**
 * @brief  主循环
 */
void Main_Loop(void)
{
    static uint32_t last_update = 0;
    
    /* 每100ms更新一次显示 */
    if((g_sys_tick - last_update) >= 100)
    {
        last_update = g_sys_tick;
        
        Process_Sensor_Data();
        Update_Display();
        Send_Status_To_PC();
    }
}

/**
 * @brief  处理传感器数据
 */
void Process_Sensor_Data(void)
{
    /* 启动ADC转换 */
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    
    uint16_t adc_value1 = ADC_GetConversionValue(ADC1);
    uint16_t adc_value2 = ADC_GetConversionValue(ADC1);
    uint16_t adc_value3 = ADC_GetConversionValue(ADC1);
    uint16_t adc_value4 = ADC_GetConversionValue(ADC1);
    
    /* 水位计算 (0-100%) */
    g_system.water_level = (adc_value1 * 100) / 4095;
    
    /* 浊度计算 */
    g_system.turbidity = (adc_value2 * 3.3f) / 4095;
    
    /* 重量计算 */
    g_system.garment_weight = (adc_value3 * 5000) / 4095;  // 0-5kg
    
    /* 温度传感器读取 (模拟) */
    g_system.current_temp = 20 + (adc_value4 * 80) / 4095;  // 20-100°C
}

/**
 * @brief  更新显示
 */
void Update_Display(void)
{
    /* 状态指示LED */
    switch(g_system.state)
    {
        case SYS_IDLE:
            LED_GREEN_ON();
            LED_RED_OFF();
            break;
        case SYS_RUNNING:
            LED_GREEN_OFF();
            LED_RED_OFF();
            break;
        case SYS_ERROR:
            LED_RED_ON();
            LED_GREEN_OFF();
            break;
        default:
            break;
    }
}

/**
 * @brief  发送状态到PC
 */
void Send_Status_To_PC(void)
{
    static char buffer[256];
    
    sprintf(buffer, 
            "{\"state\":%d,\"temp\":%.1f,\"water\":%d,\"weight\":%d,\"time\":%d}\r\n",
            g_system.state, g_system.current_temp, g_system.water_level,
            g_system.garment_weight, g_system.remaining_time);
    
    USART_SendString(USART1, buffer);
}

/**
 * @brief  USART1发送字符串
 */
void USART_SendString(USART_TypeDef* USARTx, char* str)
{
    while(*str)
    {
        while(!USART_GetFlagStatus(USARTx, USART_FLAG_TXE));
        USART_SendData(USARTx, (uint8_t)*str++);
    }
}

/**
 * @brief  SysTick中断处理
 */
void SysTick_Handler(void)
{
    g_sys_tick++;
}

/**
 * @brief  Timer4中断处理(系统计时)
 */
void TIM4_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
        
        if(g_system.state == SYS_RUNNING && g_system.remaining_time > 0)
        {
            g_system.remaining_time--;
        }
    }
}

/**
 * @brief  外部中断1(门开关)
 */
void EXTI1_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line1) != RESET)
    {
        /* 门打开，紧急停止 */
        if(g_system.state == SYS_RUNNING)
        {
            g_system.state = SYS_PAUSED;
            LED_RED_ON();
        }
        
        EXTI_ClearITPendingBit(EXTI_Line1);
    }
}
