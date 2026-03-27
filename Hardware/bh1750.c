#include "bh1750.h"

#define BH1750_ADDR   0x23        /* ADDR??GND??7??? */
#define BH1750_CMD    0x20        /* ???????? */
#define I2C_TIMEOUT   10000

/* ????I2C???? */
static uint8_t I2C_WaitEvent(uint32_t event) {
    uint32_t t = I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C1, event))
        if (--t == 0) return 0;
    return 1;
}

void BH1750_Init(void) {
    GPIO_InitTypeDef gi;
    I2C_InitTypeDef  ii;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,  ENABLE);

    /* PB6=SCL, PB7=SDA,????,?????????? */
    gi.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
    gi.GPIO_Mode  = GPIO_Mode_AF_OD;
    gi.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gi);

    ii.I2C_Mode                = I2C_Mode_I2C;
    ii.I2C_DutyCycle           = I2C_DutyCycle_2;
    ii.I2C_OwnAddress1         = 0x00;
    ii.I2C_Ack                 = I2C_Ack_Enable;
    ii.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    ii.I2C_ClockSpeed          = 100000;
    I2C_Init(I2C1, &ii);
    I2C_Cmd(I2C1, ENABLE);
}

/**
 * ????BH1750????????
 * ???????????180ms
 */
float BH1750_Read(void) {
    uint8_t b[2];
    uint16_t raw;

    /* --- ?????? --- */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (!I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT)) return 0;

    I2C_Send7bitAddress(I2C1, BH1750_ADDR << 1, I2C_Direction_Transmitter);
    if (!I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) return 0;

    I2C_SendData(I2C1, BH1750_CMD);
    if (!I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) return 0;

    I2C_GenerateSTOP(I2C1, ENABLE);

    /* ??BH1750????(?????180ms) */
    for (volatile uint32_t d = 0; d < 180 * 72000 / 5; d++) __NOP();

    /* --- ??2???? --- */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (!I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT)) return 0;

    I2C_Send7bitAddress(I2C1, BH1750_ADDR << 1, I2C_Direction_Receiver);
    if (!I2C_WaitEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) return 0;

    /* ??1??,???NACK+STOP */
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    if (!I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_RECEIVED)) return 0;
    b[0] = I2C_ReceiveData(I2C1);

    I2C_AcknowledgeConfig(I2C1, DISABLE);
    I2C_GenerateSTOP(I2C1, ENABLE);
    if (!I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_RECEIVED)) return 0;
    b[1] = I2C_ReceiveData(I2C1);

    raw = ((uint16_t)b[0] << 8) | b[1];
    return raw / 1.2f;   /* ????:lux = raw / 1.2 */
}
