  /**
  ******************************************************************************
  * @file    wifi_globals.h
  * @author  Central LAB
  * @version V2.0.1
  * @date    06-April-2016
  * @brief   Header File for storing all the global variables of WiFi module
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
#include "wifi_const.h"
#include "event_buffer.h"
#include "ring_buffer.h"
#include "wifi_module.h"

#ifndef __WIFI_GLOBALS_H
#define __WIFI_GLOBALS_H

/** @addtogroup MIDDLEWARES
* @{
*/ 

/** @addtogroup NUCLEO_WIFI_MODULE_Private_Macros
  * @{
  */

/** @addtogroup NUCLEO_WIFI_MODULE_Private_Variables
  * @{
  */

/* Private variables ---------------------------------------------------------*/

#define CLIENT_SOCKET_COUNT 8      // Client sockets supported: 8
#define SERVER_SOCKET_COUNT 2      // Server sockets supported: 2
#define WEB_CLIENT_SOCKET_COUNT 1  // Web socket supported: 1
#define TIMEOUT                 30000  // 30s timeout
#define MAX_CLIENT_WRITE_SIZE   4096  //FOR SPWF01 only 4*1024 bytes can be written in one go

/***********All Buffers**************/
#if defined (USE_STM32L0XX_NUCLEO) && defined (WIFI_USE_VCOM) 
extern char UserDataBuff[1];   /* Not used in case of VCOM */
extern uint8_t pop_buffer[1];
#else
extern char UserDataBuff[MAX_BUFFER_GLOBAL];   /* Used to store data that is to be send in callback to user */
extern uint8_t pop_buffer[MAX_BUFFER_GLOBAL];
#endif
/* Exported Declaration ----------------------------------------------------- */
extern wifi_event_TypeDef element;
extern wifi_scan *wifi_scanned_list;          // [MAX_WIFI_SCAN_NETWORK]
extern TIM_HandleTypeDef TimHandle,PushTimHandle;
extern UART_HandleTypeDef UartWiFiHandle,*UartMsgHandle;
extern WiFi_Config_HandleTypeDef WiFi_Config_Variables;
extern WiFi_Counter_Variables_t WiFi_Counter_Variables;
extern WiFi_Control_Variables_t WiFi_Control_Variables;
extern __IO IO_status_flag_typedef IO_status_flag;
extern SW_FW_COMPATIBILITY_t SW_FW_COMPATIBILITY[1];
extern char *wifi_scan_string;

extern volatile uint8_t wifi_connected;            //Set once if wifi is connected for first time
extern volatile uint8_t wifi_client_connected;     //Set once if client is connected
extern volatile uint8_t wifi_client_disconnected;  //Set once if client is dis-connected

extern wifi_bool open_sockets[CLIENT_SOCKET_COUNT];
extern wifi_bool open_server_sockets[SERVER_SOCKET_COUNT];
extern wifi_bool open_web_socket[WEB_CLIENT_SOCKET_COUNT]; 
extern wifi_bool Client_Socket_Close_Callback[CLIENT_SOCKET_COUNT];
extern wifi_bool client_connected_on_server_socket[SERVER_SOCKET_COUNT][CLIENT_SOCKET_COUNT];

#if defined (USE_STM32F4XX_NUCLEO) || defined(USE_STM32L4XX_NUCLEO)
#ifdef SPI_VCOM
extern uint8_t SPI_VCOM_Buff[3072];//3K size!
#endif
extern uint8_t WiFi_AT_Cmd_Buff[2048];
extern uint8_t WiFi_SPI_Packet[128];
extern SPI_HandleTypeDef SpiHandle;
#else
#if defined (USE_STM32L0XX_NUCLEO) && defined (WIFI_USE_VCOM)
extern uint8_t WiFi_AT_Cmd_Buff[1];
#else
extern uint8_t WiFi_AT_Cmd_Buff[1024];
#endif
#endif

extern wifi_bool packet_payload_data_available;

extern uint8_t dma_buffer[DMA_BUFFER_SIZE];
extern uint8_t * dma_buffer_ptr;
extern WiFi_Mqtt_t mqtt_type;
extern struct tftp_recv *ROOT;
extern struct tftp_recv *curr_node;

/* Private Function ---------------------------------------------------------*/
void Set_UartMsgHandle(UART_HandleTypeDef *UART_MsgHandle);

/* Exported macro ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#endif /* __WIFI_GLOBALS_H */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

