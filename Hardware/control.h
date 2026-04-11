#ifndef __CONTROL_H
#define __CONTROL_H

#include "stm32f10x.h"

/* 从 main.c 读取当前传感器值 */
extern volatile float   g_temp;
extern volatile uint8_t g_humi;

/* 从 uart.c 读取目标设定值 */
extern volatile float g_target_temp;
extern volatile float g_target_humid;

/* SysTick毫秒计数（来自 main.c） */
extern volatile uint32_t g_tick_ms;

void Control_Init(void);        /* 初始化GPIO */
void Control_HeaterPID(void);   /* 每分钟调用，风扇控温 */
void Control_PumpUpdate(void);  /* 主循环调用，水泵软PWM控湿 */
void Control_AlarmUpdate(void); /* 主循环调用，超限蜂鸣器报警 */

#endif
