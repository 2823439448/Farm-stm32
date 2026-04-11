#include "uart.h"
#include <string.h>
#include <stdlib.h>

volatile float   g_target_temp  = 0.0f;  /* 0?????,??ESP8266?? */
volatile float   g_target_humid = 0.0f;

static volatile char    s_rxbuf[UART_RXBUF_SIZE];
static volatile uint8_t s_rxidx   = 0;
volatile uint8_t        g_rx_ready = 0;

void UART1_Init(void) {
    GPIO_InitTypeDef  gi;
    USART_InitTypeDef ui;
    NVIC_InitTypeDef  ni;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    gi.GPIO_Pin   = GPIO_Pin_9;
    gi.GPIO_Mode  = GPIO_Mode_AF_PP;
    gi.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gi);

    gi.GPIO_Pin  = GPIO_Pin_10;
    gi.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gi);

    ui.USART_BaudRate            = 115200;
    ui.USART_WordLength          = USART_WordLength_8b;
    ui.USART_StopBits            = USART_StopBits_1;
    ui.USART_Parity              = USART_Parity_No;
    ui.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    ui.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &ui);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    ni.NVIC_IRQChannel                   = USART1_IRQn;
    ni.NVIC_IRQChannelPreemptionPriority = 1;
    ni.NVIC_IRQChannelSubPriority        = 0;
    ni.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&ni);

    USART_Cmd(USART1, ENABLE);
}

void USART1_IRQHandler(void) {
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        char ch = (char)USART_ReceiveData(USART1);
        if (ch == '\n') {
            s_rxbuf[s_rxidx] = '\0';
            s_rxidx    = 0;
            g_rx_ready = 1;
        } else if (ch != '\r' && s_rxidx < UART_RXBUF_SIZE - 1) {
            s_rxbuf[s_rxidx++] = ch;
        }
    }
}

void UART1_Send(const char *str) {
    while (*str) {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, (uint8_t)*str++);
    }
}

void UART1_ParseCMD(void) {
    const char *buf = (const char *)s_rxbuf;
    if (strncmp(buf, "CMD:heat,", 9) == 0) {
        float val = atof(buf + 9);
        if (val > 0 && val <= 80.0f)
            g_target_temp = val;
    } else if (strncmp(buf, "CMD:humid,", 10) == 0) {
        float val = atof(buf + 10);
        if (val >= 0 && val <= 100.0f)
            g_target_humid = val;
    }
}
