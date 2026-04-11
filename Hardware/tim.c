#include "tim.h"

volatile uint8_t g_sample_flag = 0;
static   uint8_t s_sec_cnt     = 0;

/* ---- TIM2:1??? ---- */
void TIM2_Init(void) {
    TIM_TimeBaseInitTypeDef ti;
    NVIC_InitTypeDef        ni;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    ti.TIM_Prescaler     = 7200 - 1;
    ti.TIM_Period        = 10000 - 1;
    ti.TIM_ClockDivision = TIM_CKD_DIV1;
    ti.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &ti);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);

    ni.NVIC_IRQChannel                   = TIM2_IRQn;
    ni.NVIC_IRQChannelPreemptionPriority = 0;
    ni.NVIC_IRQChannelSubPriority        = 0;
    ni.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&ni);
}

/* ????,60??????? */
void TIM2_IRQHandler(void) {
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        if (++s_sec_cnt >= 60) {
            s_sec_cnt     = 0;
            g_sample_flag = 1;
        }
    }
}

/* ---- TIM1 CH1 PWM:??,PA8,1kHz ---- */
void TIM1_PWM_Init(void) {
    GPIO_InitTypeDef        gi;
    TIM_TimeBaseInitTypeDef ti;
    TIM_OCInitTypeDef       oi;

    /* ? ?AFIO??,PA8??????? */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 | RCC_APB2Periph_GPIOA
                           | RCC_APB2Periph_AFIO, ENABLE);

    gi.GPIO_Pin   = GPIO_Pin_8;
    gi.GPIO_Mode  = GPIO_Mode_AF_PP;
    gi.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gi);

    /* 72MHz / 72 / 1000 = 1kHz */
    ti.TIM_Prescaler        = 72 - 1;
    ti.TIM_Period           = 1000 - 1;
    ti.TIM_ClockDivision    = TIM_CKD_DIV1;
    ti.TIM_CounterMode      = TIM_CounterMode_Up;
    ti.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &ti);

    oi.TIM_OCMode      = TIM_OCMode_PWM1;
    oi.TIM_OutputState = TIM_OutputState_Enable;
    oi.TIM_Pulse       = 0;
    oi.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC1Init(TIM1, &oi);

    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}

/* ??????? 0~100% */
/* ---- ????PWM??? 0~100% ---- */
void Heater_SetDuty(uint8_t pct)
{
    if (pct > 100) pct = 100;

    /* ??:??PWM(????????) */
    pct = 100 - pct;

    TIM_SetCompare1(TIM1, (uint32_t)pct * 10);
}
