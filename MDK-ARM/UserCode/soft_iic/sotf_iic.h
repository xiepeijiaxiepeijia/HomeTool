#ifndef __SOTF_IIC_H__
#define __SOTF_IIC_H__
#include "main.h"

#define TRUE  0
#define FLASE 1
//IO方向设置
//#define SDA_IN()  {GPIOA->CRL&=0X0FFFFFFF;GPIOA->CRL|=(uint32_t)8<<28;}
//#define SDA_OUT() {GPIOA->CRL&=0X0FFFFFFF;GPIOA->CRL|=(uint32_t)3<<28;}
void delay_us(uint16_t time);
void IIC_START(void);
void IIC_STOP(void);
uint8_t IIC_Wait_ACK(void);
void IIC_ACK(void);
void NOACK(void);
void IIC_SEND_BYTE(uint8_t Txdata);
uint8_t IIC_RECEIVE_BYTE(void);

#define SET_IIC_SCL_HIGH                     	HAL_GPIO_WritePin(GPIOA,LCD1602_SCL_Pin,GPIO_PIN_SET)    // 输出高电平
#define SET_IIC_SCL_LOW                      	HAL_GPIO_WritePin(GPIOA,LCD1602_SCL_Pin,GPIO_PIN_RESET)  // 输出低电平
#define SET_IIC_SDA_HIGH                      HAL_GPIO_WritePin(GPIOA,LCD1602_SDA_Pin,GPIO_PIN_SET)    // 输出高电平
#define SET_IIC_SDA_LOW                      HAL_GPIO_WritePin(GPIOA,LCD1602_SDA_Pin,GPIO_PIN_RESET)  // 输出低电平
#define SDA_READ                      			HAL_GPIO_ReadPin(GPIOA,LCD1602_SDA_Pin)


#endif

