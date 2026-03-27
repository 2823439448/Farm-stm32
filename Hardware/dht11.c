#include "dht11.h"

/* ---- ????(72MHz??,?DHT11???)---- */
static void dht_delay_us(uint32_t us) {
    us *= 9;
    while (us--) __NOP();
}

/* ---- ?????? ---- */
static void DHT11_SetOutput(void) {
    GPIO_InitTypeDef g;
    g.GPIO_Pin   = DHT11_PIN;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &g);
}

static void DHT11_SetInput(void) {
    GPIO_InitTypeDef g;
    g.GPIO_Pin   = DHT11_PIN;
    g.GPIO_Mode  = GPIO_Mode_IPU;  /* ???? */
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &g);
}

void DHT11_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    DHT11_SetOutput();
    GPIO_SetBits(DHT11_PORT, DHT11_PIN);  /* ???? */
}

/**
 * ??DHT11??
 * @param temp  ??????(°C)
 * @param humi  ??????(%RH)
 * @return 1=??, 0=??(??????)
 */
uint8_t DHT11_Read(uint8_t *temp, uint8_t *humi) {
    uint8_t  data[5] = {0};
    uint32_t t;
    int      i, j;

    /* 1. ??????:??18ms,???30us */
    DHT11_SetOutput();
    GPIO_ResetBits(DHT11_PORT, DHT11_PIN);

    /* ???????18ms(??????delay_ms???,????) */
    for (volatile uint32_t d = 0; d < 18000 * 4; d++) __NOP();

    GPIO_SetBits(DHT11_PORT, DHT11_PIN);
    dht_delay_us(30);

    /* 2. ?????,??????? */
    DHT11_SetInput();

    /* ???????(????) */
    t = 50000;
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == SET)
        if (--t == 0) return 0;

    /* ??????? */
    t = 50000;
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == RESET)
        if (--t == 0) return 0;

    /* ?????????(?????) */
    t = 50000;
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == SET)
        if (--t == 0) return 0;

    /* 3. ??40???(5??) */
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 8; j++) {
            /* ?????????? */
            t = 50000;
            while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == RESET)
                if (--t == 0) return 0;

            /* ??40us???:?=bit1,?=bit0 */
            dht_delay_us(40);
            data[i] <<= 1;
            if (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == SET) {
                data[i] |= 0x01;
                /* ??????? */
                t = 50000;
                while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == SET)
                    if (--t == 0) return 0;
            }
        }
    }

    /* 4. ????? */
    if (data[4] != (uint8_t)(data[0] + data[1] + data[2] + data[3]))
        return 0;

    *humi = data[0];
    *temp = data[2];
    return 1;
}
