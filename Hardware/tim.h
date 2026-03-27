#ifndef __TIM_H
#define __TIM_H

#include "stm32f10x.h"

extern volatile uint8_t g_sample_flag;   /* 1=?????? */

void TIM2_Init(void);              /* 1???,60????? */
void TIM1_PWM_Init(void);          /* ??PWM,PA8,1kHz */
void Heater_SetDuty(uint8_t pct); /* ???????0~100 */

#endif
