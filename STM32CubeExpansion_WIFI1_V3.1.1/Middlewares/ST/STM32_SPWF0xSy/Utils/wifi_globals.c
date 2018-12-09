/**
 ******************************************************************************
 * @file    wifi_globals.c
 * @author  Central LAB
 * @version V2.1.0
 * @date    06-April-2016
 * @brief   Store all global variables
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
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
#include "wifi_globals.h"

/** @addtogroup MIDDLEWARES
* @{
*/ 

/** @addtogroup NUCLEO_WIFI_MODULE_Private_Variables
  * @{
  */

/***********All Buffers**************/
#if defined (USE_STM32L0XX_NUCLEO) && defined (WIFI_USE_VCOM) 
char UserDataBuff[1];   /* Not used in case of VCOM */
uint8_t pop_buffer[1];
#else
char UserDataBuff[MAX_BUFFER_GLOBAL];   /* Used to store data that is to be send in callback to user */
uint8_t pop_buffer[MAX_BUFFER_GLOBAL];
#endif

wifi_event_TypeDef element;
/* Middleware version, Compatible Firmware version */
SW_FW_COMPATIBILITY_t SW_FW_COMPATIBILITY[1] = {
  {"3.1.0", "1.1.0"}
};

/* TIM handle declaration */
TIM_HandleTypeDef    TimHandle, PushTimHandle;

/* UART handle declaration */
UART_HandleTypeDef UartWiFiHandle,*UartMsgHandle;

WiFi_Config_HandleTypeDef WiFi_Config_Variables;
WiFi_Counter_Variables_t WiFi_Counter_Variables;
WiFi_Control_Variables_t WiFi_Control_Variables;
__IO IO_status_flag_typedef IO_status_flag;

volatile uint8_t wifi_connected = WIFI_FALSE;   //Set once if wifi is connected for first time
volatile uint8_t wifi_client_connected = 0;     //Set once if client is connected
volatile uint8_t wifi_client_disconnected = 0;  //Set once if client is dis-connected

wifi_bool open_sockets[CLIENT_SOCKET_COUNT];  // Each array element depicts one socket (true=open, false=closed)
wifi_bool open_server_sockets[SERVER_SOCKET_COUNT];
wifi_bool open_web_socket[WEB_CLIENT_SOCKET_COUNT]; 
wifi_bool Client_Socket_Close_Callback[CLIENT_SOCKET_COUNT];
wifi_bool client_connected_on_server_socket[SERVER_SOCKET_COUNT][CLIENT_SOCKET_COUNT];
wifi_scan *wifi_scanned_list;            // [MAX_WIFI_SCAN_NETWORK]

char *wifi_scan_string = "%d:\t BSS %[^ ] CHAN: %d RSSI: -%d SSID: '%[^'] %[^\n]";

#if defined (USE_STM32F4XX_NUCLEO) || defined(USE_STM32L4XX_NUCLEO)
#ifdef SPI_VCOM
uint8_t SPI_VCOM_Buff[3072];//3K size!
#endif

uint8_t WiFi_AT_Cmd_Buff[2048];
uint8_t WiFi_SPI_Packet[128];

/* SPI handle declaration */
SPI_HandleTypeDef SpiHandle;
#else

#if defined (USE_STM32L0XX_NUCLEO) && defined (WIFI_USE_VCOM)
uint8_t WiFi_AT_Cmd_Buff[1];
#else
uint8_t WiFi_AT_Cmd_Buff[1024];
#endif
#endif

uint8_t dma_buffer[DMA_BUFFER_SIZE];
uint8_t * dma_buffer_ptr = dma_buffer;

wifi_bool packet_payload_data_available;
WiFi_Mqtt_t mqtt_type = NO_TYPE;
struct tftp_recv *ROOT = NULL;
struct tftp_recv *curr_node = NULL;

/* Private Function ------------------------------------------------------------------*/
void Set_UartMsgHandle(UART_HandleTypeDef *UART_MsgHandle)
{
  UartMsgHandle = UART_MsgHandle;
}

/**
  * @}
  */

/**
  * @}
  */
	
	/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
	
