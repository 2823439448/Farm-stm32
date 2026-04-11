#include "bh1750.h"

/* ======================================================
   ??I2C,????PB6(SCL) PB7(SDA)
   ====================================================== */
#define SCL_PORT  GPIOB
#define SCL_PIN   GPIO_Pin_6
#define SDA_PORT  GPIOB
#define SDA_PIN   GPIO_Pin_7

#define BH1750_ADDR  0x23
#define BH1750_CMD   0x20

static void i2c_delay(void) {
    for (volatile uint32_t i = 0; i < 72; i++) __NOP(); /* ~1us @72MHz */
}

static void SCL_H(void) { GPIO_SetBits(SCL_PORT, SCL_PIN); i2c_delay(); }
static void SCL_L(void) { GPIO_ResetBits(SCL_PORT, SCL_PIN); i2c_delay(); }
static void SDA_H(void) { GPIO_SetBits(SDA_PORT, SDA_PIN); i2c_delay(); }
static void SDA_L(void) { GPIO_ResetBits(SDA_PORT, SDA_PIN); i2c_delay(); }
static uint8_t SDA_Read(void) { return GPIO_ReadInputDataBit(SDA_PORT, SDA_PIN); }

/* SDA????? */
static void SDA_Input(void) {
    GPIO_InitTypeDef g;
    g.GPIO_Pin   = SDA_PIN;
    g.GPIO_Mode  = GPIO_Mode_IPU;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SDA_PORT, &g);
}

/* SDA????? */
static void SDA_Output(void) {
    GPIO_InitTypeDef g;
    g.GPIO_Pin   = SDA_PIN;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SDA_PORT, &g);
}

static void i2c_start(void) {
    SDA_Output();
    SDA_H(); SCL_H();
    SDA_L(); /* SDA??? */
    SCL_L();
}

static void i2c_stop(void) {
    SDA_Output();
    SCL_L(); SDA_L();
    SCL_H(); SDA_H(); /* SDA??? */
}

/* ??1??,??ACK(0=ACK, 1=NACK) */
static uint8_t i2c_write(uint8_t byte) {
    uint8_t ack;
    SDA_Output();
    for (int i = 7; i >= 0; i--) {
        SCL_L();
        if (byte & (1 << i)) SDA_H(); else SDA_L();
        SCL_H();
    }
    /* ?ACK */
    SCL_L();
    SDA_Input();
    SCL_H();
    ack = SDA_Read();
    SCL_L();
    SDA_Output();
    return ack; /* 0=ACK */
}

/* ?1??,ack=1?ACK,ack=0?NACK */
static uint8_t i2c_read(uint8_t ack) {
    uint8_t byte = 0;
    SDA_Input();
    for (int i = 7; i >= 0; i--) {
        SCL_L(); SCL_H();
        if (SDA_Read()) byte |= (1 << i);
    }
    SCL_L();
    SDA_Output();
    if (ack) SDA_L(); else SDA_H();
    SCL_H(); SCL_L();
    return byte;
}

void BH1750_Init(void) {
    GPIO_InitTypeDef g;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* PB6 SCL, PB7 SDA,???? */
    g.GPIO_Pin   = SCL_PIN | SDA_PIN;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &g);

    SCL_H(); SDA_H();

    /* ??BH1750???? */
    for (volatile uint32_t d = 0; d < 50 * 8000; d++) __NOP();

    /* ?? Power On ?? */
    i2c_start();
    i2c_write((BH1750_ADDR << 1) | 0x00);
    i2c_write(0x01);  /* Power On */
    i2c_stop();

    for (volatile uint32_t d = 0; d < 10 * 8000; d++) __NOP();
}

float BH1750_Read(void) {
    uint8_t b[2];
    uint16_t raw;

    /* ?????? 0x20 = One Time H-Resolution Mode2 */
    i2c_start();
    if (i2c_write((BH1750_ADDR << 1) | 0x00) != 0) return -1; /* ???ACK */
    i2c_write(BH1750_CMD);
    i2c_stop();

    /* ??????,0x20???180ms */
    for (volatile uint32_t d = 0; d < 180 * 8000; d++) __NOP();

    /* ??2?? */
    i2c_start();
    if (i2c_write((BH1750_ADDR << 1) | 0x01) != 0) return -2; /* ????ACK */
    b[0] = i2c_read(1); /* ???,?ACK */
    b[1] = i2c_read(0); /* ???,?NACK */
    i2c_stop();

    raw = ((uint16_t)b[0] << 8) | b[1];
    return raw / 1.2f;
}
