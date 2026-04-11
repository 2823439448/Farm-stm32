#include "stm32f10x.h"
#include "dht11.h"
#include "bh1750.h"
#include "uart.h"
#include "tim.h"
#include "control.h"
#include <stdio.h>

volatile float   g_temp    = 0;
volatile uint8_t g_humi    = 0;
volatile float   g_lux     = 0;
volatile uint32_t g_tick_ms = 0;

void SysTick_Handler(void) { g_tick_ms++; }

int main(void) {
    char    txbuf[64];
    uint8_t raw_temp = 0, raw_humi = 0;

    SysTick_Config(SystemCoreClock / 1000);

    UART1_Init();
    BH1750_Init();
    DHT11_Init();
    TIM2_Init();
    TIM1_PWM_Init();
    Control_Init();          /* 内部调用Heater_SetDuty(0)，上电热床关闭 */

    /* 等待传感器上电稳定2秒 */
    for (volatile uint32_t d = 0; d < 2000; d++)
        for (volatile uint32_t e = 0; e < 72000; e++) __NOP();

    /* 立刻采样一次，获取真实传感器值 */
    g_sample_flag = 1;

    while (1) {
        /* ================================================
           1. 采样（每60秒 + 上电立刻一次）
           读完数据就发送，不做任何控制判断
           ================================================ */
        if (g_sample_flag) {
            g_sample_flag = 0;

            if (DHT11_Read(&raw_temp, &raw_humi)) {
                g_temp = (float)raw_temp;
                g_humi = raw_humi;
            }
            g_lux = BH1750_Read();

            snprintf(txbuf, sizeof(txbuf), "SENSOR:%d,%d,%.1f\n",
                     (int)g_temp, (int)g_humi, g_lux);
            UART1_Send(txbuf);
        }

        /* ================================================
           2. 收到CMD立刻解析并执行控制
           温度CMD → 立刻调整热床/风扇
           湿度CMD → 更新目标，Control_PumpUpdate下次轮询立刻响应
           两种CMD都调Control_HeaterPID和Control_PumpUpdate无妨，
           内部有g_temp==0/g_target==0保护
           ================================================ */
        if (g_rx_ready) {
            g_rx_ready = 0;
            UART1_ParseCMD();
            Control_HeaterPID();  /* 温度目标更新立刻响应 */
            /* 水泵由下面高频轮询响应，不需要单独调用 */
        }

        /* ================================================
           3. 水泵高频轮询（目标湿度或当前湿度变化立刻响应）
           ================================================ */
        Control_PumpUpdate();

        /* ================================================
           4. 蜂鸣器报警
           ================================================ */
        Control_AlarmUpdate();
    }
}
 /*
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                        引脚分配汇总                                       │
 * │                                                                          │
 * │  PA1   DHT11 DATA          单总线双向GPIO，外接4.7kΩ上拉至3.3V         │
 * │  PA8   TIM1_CH1 PWM        热床MOS管栅极，1kHz PWM，AF_PP              │
 * │  PA9   USART1_TX           → ESP8266 RX，AF_PP                        │
 * │  PA10  USART1_RX           ← ESP8266 TX，IN_FLOATING                  │
 * │  PB0   水泵继电器 IN2      OUT_PP，高电平触发继电器吸合                 │
 * │  PB1   蜂鸣器三极管基极    OUT_PP，高电平触发S8050导通                  │
 * │  PB10   风扇继电器 IN1      OUT_PP，高电平触发继电器吸合                 │
 * │  PB6   I2C1_SCL            → BH1750 SCL，AF_OD                        │
 * │  PB7   I2C1_SDA            → BH1750 SDA，AF_OD                        │
 * └─────────────────────────────────────────────────────────────────────────┘
 */

