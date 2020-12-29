/**
  ******************************************************************************
  * @file           : app.c
  * @brief          : app program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2019 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "app.h"
#include "gizwits_protocol.h"
#include "gizwits_product.h"
#include "hal_key.h"

#define GPIO_KEY_NUM 2 ///< Defines the total number of key member
#include "bsp_DHT11.h"

///------------------- G0 TO F7 ------------------------
static short temperature = 0, humidity = 0;  
float gizwits_temperature = 0, gizwits_humidity = 0;
uint8_t tui_buf[10];
int temp_flag;
#define FRM_START                0x68
#define FRM_CMD_UPD_SENSOR       0x01//温湿度传感器
#define FRM_CMD_UPD_RESTWIFI     0x02//复位WIFI
#define FRM_CMD_UPD_AIRLINK      0x03//配置WIFI到路由器
#define FRM_CMD_UPD_CONFIGEND    0x04//链接到路由器
#define FRM_FIXLEN               14
#define FRM_END0                 0x5c
#define FRM_END1                 0x6e

#define FRM_POS_START            0
#define FRM_POS_CMD              1      
#define FRM_POS_LEN              2
#define FRM_POS_DATA             3
#define FRM_POS_CRC              11
#define FRM_POS_END0             12
#define FRM_POS_END1             13

keyTypedef_t singleKey[GPIO_KEY_NUM]; ///< Defines a single key member array pointer
static uint8_t s_wifi_router_status = 0; 
static void update_to_ui_wifi_status(uint8_t cmd)
{
    if(cmd==FRM_CMD_UPD_RESTWIFI||cmd==FRM_CMD_UPD_AIRLINK||cmd==FRM_CMD_UPD_CONFIGEND)
    {
      s_wifi_router_status = cmd;
    }
}

void ConfigWIFIfinish(void)
{
	update_to_ui_wifi_status(FRM_CMD_UPD_CONFIGEND);
}
void key1ShortPress(void)
{
    GIZWITS_LOG("KEY1 PRESS ,Production Mode\n");
    gizwitsSetMode(WIFI_PRODUCTION_TEST);
}
/**
* key1 long press handle
* @param none
* @return none
*/
void key1LongPress(void)
{
    GIZWITS_LOG("KEY1 PRESS LONG ,Soft AP\n");
    gizwitsSetMode(WIFI_SOFTAP_MODE);
}
/**
* key2 short press handle
* @param none
* @return none
*/
void key2ShortPress(void)
{
        GIZWITS_LOG("------>KEY2 PRESS,Air Link mode\n\r");
        #if !MODULE_TYPE
        gizwitsSetMode(WIFI_AIRLINK_MODE);
        update_to_ui_wifi_status(FRM_CMD_UPD_AIRLINK);
        #endif
}
/**
* key2 long press handle
* @param none
* @return none
*/
void key2LongPress(void)
{
        //AirLink mode
        GIZWITS_LOG("KEY2 PRESS LONG ,Wifi Reset mode\n");
        #if !MODULE_TYPE
        gizwitsSetMode(WIFI_RESET_MODE);
        update_to_ui_wifi_status(FRM_CMD_UPD_RESTWIFI);
        #endif
}
void keyInit(void)
{
    singleKey[0] = keyInitOne(NULL, KEY1_GPIO_Port, KEY1_Pin, key1ShortPress, key1LongPress);
    singleKey[1] = keyInitOne(NULL, KEY2_GPIO_Port, KEY2_Pin, key2ShortPress, key2LongPress);
    keys.singleKey = (keyTypedef_t *)&singleKey;
    keyParaInit(&keys); 
}


typedef struct __frame_sensor_to_ui{
        uint8_t start ;
        uint8_t cmd;
        uint8_t len;
        uint8_t data[8];
        uint8_t crc; 
        uint8_t end ;        
}_frame_sensor_to_ui;
_frame_sensor_to_ui s_frame_sensor_to_ui;

//eg:68 cmd len t t h h x x y y crc 16 / crc = 68 + cmd + len + t + t + h + h + x + x + y + y 16
extern UART_HandleTypeDef huart2;
uint8_t cal_crc(uint8_t *buf,uint8_t len)
{
       uint8_t t_crc = 0;
       uint8_t r_crc = 0;
        //check crc
        for(int i=0; i<len ;i++)
        {
                t_crc += buf[i];
        }
        r_crc = (uint8_t)t_crc;
        return r_crc;
}

#define __DEBUG_UPDATE_UI_ARRAY__
void update_to_ui(uint8_t cmd,uint8_t *data)
{
        uint8_t t_buf[20];
        int i = 0;
        memset(t_buf,0,sizeof(t_buf));
        t_buf[FRM_POS_START]=FRM_START;
        t_buf[FRM_POS_CMD]=cmd;
        t_buf[FRM_POS_LEN]=FRM_FIXLEN;
        t_buf[FRM_POS_DATA]= data[0];
        t_buf[FRM_POS_DATA+1]= data[1];
        t_buf[FRM_POS_DATA+2]= data[2];
        t_buf[FRM_POS_DATA+3]= data[3];
        t_buf[FRM_POS_DATA+4]= s_wifi_router_status;     //wifi status update
        t_buf[FRM_POS_DATA+5]= temp_flag;                
        t_buf[FRM_POS_DATA+6]= 0;
        t_buf[FRM_POS_DATA+7]= 0;
        t_buf[FRM_POS_CRC]=cal_crc(t_buf,FRM_FIXLEN-3);
        t_buf[FRM_POS_END0]=FRM_END0;
        t_buf[FRM_POS_END1]=FRM_END1;
#ifdef __DEBUG_UPDATE_UI_ARRAY__
        GIZWITS_LOG("update_to_ui:");
        for(i=0; i<FRM_FIXLEN; i++)
        {
            GIZWITS_LOG("%02x ", t_buf[i]);
        }
        GIZWITS_LOG("\n\r");
#endif
        HAL_UART_Transmit(&huart2, t_buf, FRM_FIXLEN, 0xFFFF);
}

///------------------- APP ------------------------
extern int SHT3X_GetTempAndHumi(short *temp, short *humi);
void APP_Init(void)
{
        //s_wifi_router_status = 0; 

        timerInit();
        uartInit();

        userInit();
        gizwitsInit();
        keyInit();
        GIZWITS_LOG("MCU Init Success \n");

        printf("MCU HAL Success , SoftVersion = %s\r\n",SOFTWARE_VERSION);
        //Pro_D2W_Ask_Module_Reboot();
}
void APP_Process(void)
{	
//        if(DHT11_Read_TempAndHumidity(&temperature, &humidity)==0)
//        {
//                gizwits_temperature = temperature/10.00;
//                gizwits_humidity = humidity / 10.00;
//                //tui_buf[0] = (uint16_t)temperature;
//                tui_buf[0] = ((temperature >> 8) & 0xff);               
//                tui_buf[1] = (temperature & 0xff);
//                tui_buf[2] = ((humidity >> 8) & 0xff);
//                tui_buf[3] = (humidity & 0xff);
//                update_to_ui(FRM_CMD_UPD_SENSOR,tui_buf);
//                //printf("temp=%0.1f,humi=%0.1f\r\n",gizwits_temperature,gizwits_humidity);
//        }
}


