#include "sotf_iic.h"

static void IIC_Mode_IPU(void);
static void IIC_Mode_Out_PP(void);



static void IIC_Mode_IPU(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  
  /* 串口外设功能GPIO配置 */
  GPIO_InitStruct.Pin   = LCD1602_SDA_Pin;
  GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
}

static void IIC_Mode_Out_PP(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  
  /* 串口外设功能GPIO配置 */
  GPIO_InitStruct.Pin = LCD1602_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct); 	 
}



void delay_us(uint16_t time)
{    
   uint16_t i = 0;  
   while(time--)
   {
      i = 10;   //随便定义的一个值, 无意义, 纯粹是消耗CPU时间而已, 可能会编译器被优化  
      while(i--);    
   }
}

void IIC_START(void)
{
	IIC_Mode_Out_PP(); //设置SDA线为输出模式
	SET_IIC_SDA_HIGH;
	SET_IIC_SCL_HIGH;
	delay_us(5);
	SET_IIC_SDA_LOW;
	delay_us(5);
	// SCL在低的时候, 是不传送任何数据, 也不作为开始和结束的条件, 
	//所以这样我们可以开始数据的发送而不会导致产生开始或者结束信号 
	//这个就是所谓的钳住I2C总线
	SET_IIC_SCL_LOW; 
}

void IIC_STOP(void)
{
	IIC_Mode_Out_PP();
	SET_IIC_SCL_LOW;  //先拉低SCL是为了防止重复产生开始信号
	SET_IIC_SDA_LOW;  //把SDA拉低为产生从低电平到高电平突变做好准备
	delay_us(5);
	SET_IIC_SCL_HIGH; //SCL拉高就是因为产生停止信号的首要条件就是SCL为高
	SET_IIC_SDA_HIGH;
	delay_us(5);	
}
//等待从机的ACK
uint8_t IIC_Wait_ACK(void)
{
	uint8_t time_out_cnt = 0;
	SET_IIC_SDA_HIGH;  //主机输出端的SDA至高
	delay_us(5);
	SET_IIC_SCL_HIGH;
	delay_us(5);
	IIC_Mode_IPU();//设置为输入模式等待从机发来应答信号
	while(SDA_READ)
	{
		time_out_cnt++;
		if(time_out_cnt>255)
		{
			IIC_STOP();
			return FLASE;//超时，退出程序
		}
	}
	SET_IIC_SCL_LOW;//接收到从机的应答信号结束操作
	return TRUE;
	
}
//主机发送到从机的ACK
void IIC_ACK(void)
{
	SET_IIC_SCL_LOW;//先拉低SCL是防止SDA的变化产生影响
	IIC_Mode_Out_PP();
	SET_IIC_SDA_LOW;
	delay_us(5);
	SET_IIC_SCL_HIGH;//将SCL拉高将ACK发送出去
	delay_us(5);
	SET_IIC_SCL_LOW; //结束ACK信号发送
	
}
//主机发送到从机的NACK，即没有应答信号
void NOACK(void)
{
	SET_IIC_SCL_LOW;
	IIC_Mode_Out_PP();
	SET_IIC_SDA_HIGH;
	delay_us(5);
	SET_IIC_SCL_HIGH;//将NOACK发送出去
	delay_us(5);
	SET_IIC_SCL_LOW; //结束发送
}

void IIC_SEND_BYTE(uint8_t Txdata)
{
	uint8_t i;
	IIC_Mode_Out_PP();
	SET_IIC_SCL_LOW;
	for(i=0;i<8;i++)
	{
		if((Txdata&0x80)) //按位与上0x80之后只剩下最高位
			SET_IIC_SDA_HIGH;
		else
			SET_IIC_SDA_LOW;
		Txdata <<=1; //左移一位
		delay_us(5);
		SET_IIC_SCL_HIGH; //将该位发送出去
		delay_us(5);
		SET_IIC_SCL_LOW;
		delay_us(5);
	}
}
uint8_t IIC_RECEIVE_BYTE(void)
{
	unsigned char t,receive_data = 0;
	IIC_Mode_IPU();
	for(t=0;t<8;t++)
	{
		SET_IIC_SCL_LOW;
		delay_us(5);
		SET_IIC_SCL_HIGH;//拉低后再拉高产生一个有效的时钟信号
		receive_data = (receive_data<<1)|SDA_READ;//一位一位的拼接上去
		delay_us(5);
	}
	return receive_data;
}
