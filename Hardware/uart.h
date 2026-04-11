#ifndef __UART_H
#define __UART_H

#include "stm32f10x.h"

#define UART_RXBUF_SIZE  64

extern volatile float   g_target_temp;
extern volatile float   g_target_humid;
extern volatile uint8_t g_rx_ready;

void UART1_Init(void);
void UART1_Send(const char *str);
void UART1_ParseCMD(void);

#endif
