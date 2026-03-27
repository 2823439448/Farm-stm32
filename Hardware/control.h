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

void Control_Init(void);       /* 初始化GPIO（水泵、风扇、蜂鸣器） */
void Control_HeaterPID(void);  /* 每分钟采样后调用，计算PID更新热床及风扇 */
void Control_PumpUpdate(void); /* 主循环高频调用，水泵软PWM状态机 */
void Control_AlarmUpdate(void);/* 主循环调用，超限驱动蜂鸣器 */

#endif
