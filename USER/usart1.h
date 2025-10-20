#ifndef __USART1_H
#define	__USART1_H

#include "stm32f4xx.h"                  // Device header for F4
#include <stdio.h>
#include <stdarg.h>

extern char Serial_RxPacket[100];              // 接收数据包数组，格式: @MSG\r\n
extern uint8_t Serial_RxFlag;              // 接收数据包标志位

/**
  * 函数名：串口初始化
  * 参数：无
  * 返回值：无
  */
void Serial_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(char *String);
uint32_t Serial_Pow(uint32_t X, uint32_t Y);
void Serial_SendNumber(uint32_t Number, uint8_t Length);
int fputc(int ch, FILE *f);
void Serial_Printf(char *format, ...);


#endif /* __USART1_H */