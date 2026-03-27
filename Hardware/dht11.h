#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

/* DHT11 DATA??,?????? */
#define DHT11_PORT  GPIOA
#define DHT11_PIN   GPIO_Pin_1

void    DHT11_Init(void);
uint8_t DHT11_Read(uint8_t *temp, uint8_t *humi);

#endif
