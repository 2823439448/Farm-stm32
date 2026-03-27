#include "control.h"
#include "tim.h"
#include <stdint.h>

/* ---- 引脚定义 ---- */
#define PUMP_PORT  GPIOB
#define PUMP_PIN   GPIO_Pin_0   /* 水泵继电器，高电平触发 */

#define FAN_PORT   GPIOB
#define FAN_PIN    GPIO_Pin_2   /* 风扇继电器，高电平触发 */

#define BUZZ_PORT  GPIOB
#define BUZZ_PIN   GPIO_Pin_1   /* 蜂鸣器，高电平触发 */

/* ---- 报警阈值 ---- */
#define ALARM_TEMP_MAX   35.0f   /* 温度超过35°C报警 */
#define ALARM_HUMID_MAX  90.0f   /* 湿度超过90%RH报警 */

/* ---- 水泵软PWM参数 ---- */
#define PUMP_PERIOD_MS   10000U  /* 软PWM周期：10秒 */
#define PUMP_HYST        2       /* 湿度回差：目标±2%RH内不动作 */

/* ---- 风扇PID输出阈值 ---- */
#define FAN_ON_THRESHOLD  10.0f  /* PID输出超过10%即开风扇 */

/* ====================================================
   PID结构体（位置式，带积分限幅）
   ==================================================== */
typedef struct {
    float kp, ki, kd;
    float integral;
    float prev_err;
    float last_out;
    float out_min, out_max;
} PID_t;

static PID_t s_heat_pid = {
    .kp       = 0.5f,   /* 与论文整定参数一致 */
    .ki       = 0.03f,
    .kd       = 0.27f,
    .integral  = 0,
    .prev_err  = 0,
    .last_out  = 0,
    .out_min   = 0.0f,
    .out_max   = 100.0f
};

static float PID_Calc(PID_t *pid, float setpoint, float measured) {
    float err = setpoint - measured;

    /* 条件积分抗饱和：输出未饱和时才累积积分 */
    if (pid->last_out > pid->out_min &&
        pid->last_out < pid->out_max) {
        pid->integral += err;
    }

    float deriv = err - pid->prev_err;
    pid->prev_err = err;

    float out = pid->kp * err
              + pid->ki * pid->integral
              + pid->kd * deriv;

    if (out < pid->out_min) out = pid->out_min;
    if (out > pid->out_max) out = pid->out_max;
    pid->last_out = out;
    return out;
}

/* ====================================================
   初始化：水泵、风扇、蜂鸣器 GPIO
   ==================================================== */
void Control_Init(void) {
    GPIO_InitTypeDef g;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* PB0水泵 / PB1蜂鸣器 / PB2风扇，统一推挽输出 */
    g.GPIO_Pin   = PUMP_PIN | BUZZ_PIN | FAN_PIN;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &g);

    /* 上电全部关闭 */
    GPIO_ResetBits(GPIOB, PUMP_PIN | BUZZ_PIN | FAN_PIN);
}

/* ====================================================
   热床PID + 风扇联动（每分钟采样后调用一次）
   PID输出 → PA8 PWM控制热床功率
   PID输出超过阈值 → PB2开风扇辅助散热
   ==================================================== */
void Control_HeaterPID(void) {
    float out = PID_Calc(&s_heat_pid, g_target_temp, g_temp);

    /* 热床PWM输出（PA8，TIM1_CH1） */
    Heater_SetDuty((uint8_t)out);

    /* 风扇联动：PID输出超过阈值则开风扇 */
    if (out > FAN_ON_THRESHOLD) {
        GPIO_SetBits(FAN_PORT, FAN_PIN);
    } else {
        GPIO_ResetBits(FAN_PORT, FAN_PIN);
    }
}

/* ====================================================
   水泵软PWM控制（主循环高频轮询）
   周期10秒，占空比由湿度偏差线性决定：
     偏差1%RH → 占空比3%，上限90%
     偏差在回差范围内 → 停泵
   ==================================================== */
void Control_PumpUpdate(void) {
    static uint32_t s_cycle_start = 0;
    static uint8_t  s_pump_on     = 0;

    float   target  = g_target_humid;
    uint8_t cur     = g_humi;
    float   deficit = target - (float)cur;

    /* 在回差范围内或湿度已超目标，停泵并重置周期 */
    if (deficit <= (float)PUMP_HYST) {
        if (s_pump_on) {
            GPIO_ResetBits(PUMP_PORT, PUMP_PIN);
            s_pump_on = 0;
        }
        s_cycle_start = g_tick_ms;
        return;
    }

    /* 偏差映射占空比，上限90% */
    float duty = deficit * 3.0f;
    if (duty > 90.0f) duty = 90.0f;

    uint32_t on_ms   = (uint32_t)(PUMP_PERIOD_MS * duty / 100.0f);
    uint32_t elapsed = g_tick_ms - s_cycle_start;

    /* 新周期重置 */
    if (elapsed >= PUMP_PERIOD_MS) {
        s_cycle_start = g_tick_ms;
        elapsed = 0;
    }

    /* 开启阶段 */
    if (elapsed < on_ms) {
        if (!s_pump_on) {
            GPIO_SetBits(PUMP_PORT, PUMP_PIN);
            s_pump_on = 1;
        }
    /* 停止阶段（等待土壤渗水，传感器数值稳定） */
    } else {
        if (s_pump_on) {
            GPIO_ResetBits(PUMP_PORT, PUMP_PIN);
            s_pump_on = 0;
        }
    }
}

/* ====================================================
   蜂鸣器超限报警（主循环持续轮询）
   温度超过35°C 或 湿度超过90%RH 即鸣响
   ==================================================== */
void Control_AlarmUpdate(void) {
    if (g_temp > ALARM_TEMP_MAX || (float)g_humi > ALARM_HUMID_MAX) {
        GPIO_SetBits(BUZZ_PORT, BUZZ_PIN);
    } else {
        GPIO_ResetBits(BUZZ_PORT, BUZZ_PIN);
    }
}
