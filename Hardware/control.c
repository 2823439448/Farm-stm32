#include "control.h"
#include "tim.h"
#include <stdint.h>

#define PUMP_PORT  GPIOB
#define PUMP_PIN   GPIO_Pin_0
#define FAN_PORT   GPIOB
#define FAN_PIN    GPIO_Pin_10
#define BUZZ_PORT  GPIOB
#define BUZZ_PIN   GPIO_Pin_1

#define ALARM_TEMP_MAX   50.0f
#define ALARM_HUMID_MAX  90.0f
#define PUMP_PERIOD_MS   10000U
#define PUMP_HYST        2
#define FAN_HYST         1.0f
#define HEAT_DEADBAND    0.5f

typedef struct {
    float kp, kd;
    float prev_err;
    float last_out;
    float out_min, out_max;
} PD_t;

static PD_t s_heat_pd = {
    .kp=2.0f, .kd=0.5f,
    .prev_err=0, .last_out=0,
    .out_min=0.0f, .out_max=100.0f
};

static float s_last_target = -999.0f;

static float PD_Calc(PD_t *pd, float setpoint, float measured) {
    float err   = setpoint - measured;
    float deriv = err - pd->prev_err;
    pd->prev_err = err;
    float out = pd->kp * err + pd->kd * deriv;
    if (out < pd->out_min) out = pd->out_min;
    if (out > pd->out_max) out = pd->out_max;
    pd->last_out = out;
    return out;
}

void Control_Init(void) {
    GPIO_InitTypeDef g;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    g.GPIO_Pin   = PUMP_PIN | FAN_PIN | BUZZ_PIN;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &g);
    GPIO_ResetBits(GPIOB, PUMP_PIN | FAN_PIN | BUZZ_PIN);
    Heater_SetDuty(0);   /* 上电热床关闭，防止反转导致满功率 */
}

void Control_HeaterPID(void) {
    /* 没有真实传感器值或未收到目标温度，不控制 */
    if (g_temp == 0 || g_target_temp == 0) return;

    /* 目标变化重置PD状态 */
    if (g_target_temp != s_last_target) {
        s_heat_pd.prev_err = 0;
        s_heat_pd.last_out = 0;
        s_last_target = g_target_temp;
    }

    float err = g_target_temp - g_temp;

    if (err > HEAT_DEADBAND) {
        /* 温度低于目标：热床加热，关风扇 */
        float out = PD_Calc(&s_heat_pd, g_target_temp, g_temp);
        Heater_SetDuty((uint8_t)out);
        GPIO_ResetBits(FAN_PORT, FAN_PIN);
    } else if (err < -FAN_HYST) {
        /* 温度高于目标：关热床，开风扇 */
        Heater_SetDuty(0);
        s_heat_pd.prev_err = 0;
        GPIO_SetBits(FAN_PORT, FAN_PIN);
    } else {
        /* 稳定区：都关 */
        Heater_SetDuty(0);
        GPIO_ResetBits(FAN_PORT, FAN_PIN);
    }
}

void Control_PumpUpdate(void) {
    static uint32_t s_cycle_start = 0;
    static uint8_t  s_pump_on     = 0;

    /* 没有真实湿度值或未收到目标湿度，不控制 */
    if (g_humi == 0 || g_target_humid == 0) return;

    float deficit = g_target_humid - (float)g_humi;

    /* 湿度达标或目标低于当前：停泵 */
    if (deficit <= (float)PUMP_HYST) {
        if (s_pump_on) {
            GPIO_ResetBits(PUMP_PORT, PUMP_PIN);
            s_pump_on = 0;
        }
        s_cycle_start = g_tick_ms;
        return;
    }

    /* 偏差映射占空比：偏差1% → 3%，上限90% */
    float duty = deficit * 3.0f;
    if (duty > 90.0f) duty = 90.0f;

    uint32_t on_ms   = (uint32_t)(PUMP_PERIOD_MS * duty / 100.0f);
    uint32_t elapsed = g_tick_ms - s_cycle_start;

    if (elapsed >= PUMP_PERIOD_MS) {
        s_cycle_start = g_tick_ms;
        elapsed = 0;
    }

    if (elapsed < on_ms) {
        if (!s_pump_on) {
            GPIO_SetBits(PUMP_PORT, PUMP_PIN);
            s_pump_on = 1;
        }
    } else {
        if (s_pump_on) {
            GPIO_ResetBits(PUMP_PORT, PUMP_PIN);
            s_pump_on = 0;
        }
    }
}

void Control_AlarmUpdate(void) {
    if (g_temp > ALARM_TEMP_MAX || (float)g_humi > ALARM_HUMID_MAX)
        GPIO_SetBits(BUZZ_PORT, BUZZ_PIN);
    else
        GPIO_ResetBits(BUZZ_PORT, BUZZ_PIN);
}
