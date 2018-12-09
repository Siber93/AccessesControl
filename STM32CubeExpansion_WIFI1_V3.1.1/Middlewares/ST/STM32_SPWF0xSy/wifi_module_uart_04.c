/**
 ******************************************************************************
 * @file    wifi_module_uart_04.c
 * @author  Central LAB
 * @version V2.1.0
 * @date    25-Nov-2016
 * @brief   Enable Wi-Fi functionality using AT cmd set
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
#include "wifi_module.h"
#include "wifi_globals.h"


/** @addtogroup MIDDLEWARES
* @{
*/ 

/** @defgroup  NUCLEO_WIFI_MODULE
  * @brief Wi-Fi driver modules
  * @{
  */

/** @defgroup NUCLEO_WIFI_MODULE_Private_Defines
  * @{
  */

/**
  * @}
  */

/** @addtogroup NUCLEO_WIFI_MODULE_Private_Variables
  * @{
  */

/***** Variables Declaration *****/
#define BUFFER_SIZE 900
extern wifi_instances_t wifi_instances;

int parsing_networks;

char str[BUFFER_SIZE];
uint8_t topic[100];
uint8_t message_pending_buffer[BUFFER_SIZE];

int bytes_read = 0;
int8_t client_id = -1;
int last_message_size = 0;
int last_dma_buffer_count = 0;
uint8_t message_pending = WIFI_FALSE;
uint32_t message_pending_buffer_index;
uint32_t remaining_unused_datalength = 0;
int32_t message_size = -1,total_message_size = -1;

#if defined(SPWF04)

#undef memmem
void *memmem(const void *haystack, size_t haystack_len, const void *needle, size_t needle_len);
/*
 * Find the first occurrence of the byte string s in byte string l.
*/
/**
* @brief  memmem
*         Find the first occurrence of the byte string s in byte string l.
* @param  haystack:     string to be scanned
* @param  haystack_len: length of the string to be scanned
* @param  needle:       string containing the sequence of characters to match
* @param  needle_len:   length of needle.
* @retval None
*/
void *memmem(const void *haystack, size_t haystack_len, const void *needle, size_t needle_len)
{
  register char *cur, *last;
  const char *ch = (const char *)haystack;
  const char *cn = (const char *)needle;

  /* we need something to compare */
  if (haystack_len == 0 || needle_len == 0)
          return NULL;

  /* "needle" must be smaller or equal to "l" */
  if (haystack_len < needle_len)
          return NULL;

  /* special case where needle_len == 1 */
  if (needle_len == 1)
          return memchr(haystack, (int)*cn, haystack_len);

  /* the last position where its possible to find "s" in "haystack" */
  last = (char *)ch + haystack_len - needle_len;

  for (cur = (char *)ch; cur <= last; cur++)
          if (cur[0] == cn[0] && memcmp(cur, cn, needle_len) == 0)
                  return cur;

  return NULL;
}


WiFi_Status_t Soft_Reset(void)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

#if DEBUG_PRINT  
  printf("\r\n >>Soft Reset Wi-Fi module\r\n");
#endif

  Reset_AT_CMD_Buffer();

  /* AT : send AT command */
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_RESET);  
//  WiFi_WIND_State.WiFiReset = WIFI_FALSE;
  IO_status_flag.WiFi_WIND_State = Undefine_state;
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status != WiFi_MODULE_SUCCESS) 
    return status;
  /* AT+CFUN=1 //Soft reset */
  while(IO_status_flag.WiFi_WIND_State != WiFiHWStarted)
  {
    __NOP(); //nothing to do
  }
  return status;
}
#endif

#if defined(SPWF04) && defined(CONSOLE_UART_ENABLED)

/**
* @brief  unused_data_handling
*         ignore the next "length_of_data" bytes of data
* @param  length_of_data: amount of data to be ignored
* @retval None
*/
void unused_data_handling(uint32_t length_of_data)
{
  char *pStr;
  
  /* Check if there is some message in between the data */
  if(( pStr = strstr((const char *)dma_buffer_ptr+WiFi_Counter_Variables.dma_buffer_count,"+WIND:")) != NULL 
  || ( pStr = strstr((const char *)dma_buffer_ptr+WiFi_Counter_Variables.dma_buffer_count,"AT+S.")) != NULL
  || ( pStr = strstr((const char *)dma_buffer_ptr+WiFi_Counter_Variables.dma_buffer_count,"AT-S.")) != NULL)
  {
    int index_of_message = (uint8_t *)pStr - (uint8_t *)dma_buffer_ptr;
    if(index_of_message < WiFi_Counter_Variables.dma_buffer_count + length_of_data)
    {
      length_of_data = index_of_message - WiFi_Counter_Variables.dma_buffer_count;
      WiFi_Control_Variables.pending_unused_data = WIFI_TRUE;
    }
  }

  int length_of_data_in_dma_buffer = strlen((const char *)dma_buffer_ptr+WiFi_Counter_Variables.dma_buffer_count);
  /* If available data in buffer less than the data to be ignored */
  if(length_of_data_in_dma_buffer < length_of_data)
  {
    length_of_data = length_of_data_in_dma_buffer;
    WiFi_Control_Variables.pending_unused_data = WIFI_TRUE;
  }

  remaining_unused_datalength -= length_of_data;
  WiFi_Counter_Variables.dma_buffer_count += length_of_data;
  if(WiFi_Counter_Variables.dma_buffer_count >= DMA_BUFFER_SIZE)
  {
    memset(dma_buffer+WiFi_Counter_Variables.dma_buffer_previous_index,0x00, DMA_BUFFER_SIZE - WiFi_Counter_Variables.dma_buffer_previous_index);
    WiFi_Counter_Variables.dma_buffer_previous_index =0;
    WiFi_Counter_Variables.dma_buffer_count %= DMA_BUFFER_SIZE;
  }
  if(WiFi_Counter_Variables.dma_buffer_count > 0)
    memset(dma_buffer+ WiFi_Counter_Variables.dma_buffer_previous_index,0x00,WiFi_Counter_Variables.dma_buffer_count-WiFi_Counter_Variables.dma_buffer_previous_index);

  WiFi_Counter_Variables.dma_buffer_previous_index = WiFi_Counter_Variables.dma_buffer_count;
}


/**
* @brief  Read_DMA_Buffer
*         Processes the DMA buffer if dma_buffer is non empty 
* @param  None
* @retval None
*/
void Read_DMA_Buffer()
{
  /* is there data in the buffer? */
  if(*(dma_buffer_ptr + WiFi_Counter_Variables.dma_buffer_count) != 0x00)
  {
    Process_DMA_Buffer();
    if(WiFi_Counter_Variables.dma_buffer_count != 0 && (WiFi_Counter_Variables.dma_buffer_count !=WiFi_Counter_Variables.dma_buffer_previous_index))
    {
      /* TBD: resest the last part of dma_buffer */
      memset(dma_buffer_ptr+WiFi_Counter_Variables.dma_buffer_previous_index,0x00,WiFi_Counter_Variables.dma_buffer_count-WiFi_Counter_Variables.dma_buffer_previous_index);
      WiFi_Counter_Variables.dma_buffer_previous_index = WiFi_Counter_Variables.dma_buffer_count;
    }
  }
}


/**
* @brief  Adjust_DMA_Buffer_Index
*         Adjust the DMA Buffer Index i.e. dma_buffer_count variable
* @param  bytes_read: number of bytes read
* @retval None
*/
void Adjust_DMA_Buffer_Index(int bytes_read)
{
  WiFi_Counter_Variables.dma_buffer_count += bytes_read;
  /* Wrap the DMA buffer pointer (if required) */
  if(WiFi_Counter_Variables.dma_buffer_count > DMA_BUFFER_SIZE)
    WiFi_Counter_Variables.dma_buffer_count = DMA_BUFFER_SIZE;
}


/**
* @brief  are_extra_bytes_read
*         are bytes read from another buffer apart from the one which we were reading.
* @param  idn: identifier(WIND/CMD/INTERIM_DATA/UNDEFINE)
* @retval None
*/
void Check_for_Extra_Bytes(int idn)
{
  if(WiFi_Counter_Variables.dma_buffer_count + bytes_read > DMA_BUFFER_END_INDEX)
  {
    message_classification_TypeDef message_type;
    message_type = (message_classification_TypeDef)idn;
    int length = WiFi_Counter_Variables.dma_buffer_count + bytes_read - DMA_BUFFER_SIZE;
    bytes_read = DMA_BUFFER_SIZE - WiFi_Counter_Variables.dma_buffer_count;
    switch(message_type)
    {
        case WIND:
          memset(str+(strlen(str) - length),0x00,length);
          break;
        case CMD:
        case INTERIM_DATA:
          memset(str+(bytes_read-5),0x00,length);
          break;
        case UNDEFINE:
          memset(str+bytes_read,0x00,length);
          break;
        default:
          break;
    }
  }
}


/**
* @brief  end_of_buffer_reached
*         start reading data from next buffer once end of previous buffer is reached
* @param  None
* @retval wifi_bool: true if end of buffer reached, false otherwise
*/
wifi_bool is_End_of_Buffer_Reached()
{
  /* WiFi_Counter_Variables.dma_buffer_count + bytes_read == DMA_BUFFER_SIZE when partial message received 
     WiFi_Counter_Variables.dma_buffer_count == DMA_BUFFER_SIZE  when complete end of message received */

  if(WiFi_Counter_Variables.dma_buffer_count == DMA_BUFFER_SIZE || 
     WiFi_Counter_Variables.dma_buffer_count + bytes_read == DMA_BUFFER_SIZE)
  {
    if(WiFi_Counter_Variables.dma_buffer_count + bytes_read == DMA_BUFFER_SIZE)
    {
      /* the rest of message is stored in another buffer, copy the pending message to a temporary buffer */
      message_pending_buffer_index = (DMA_BUFFER_SIZE) - WiFi_Counter_Variables.dma_buffer_count;
      memcpy(message_pending_buffer, dma_buffer_ptr+WiFi_Counter_Variables.dma_buffer_count, message_pending_buffer_index);
      message_pending = WIFI_TRUE;
    }
    memset(dma_buffer_ptr+WiFi_Counter_Variables.dma_buffer_previous_index,0x00,DMA_BUFFER_SIZE-WiFi_Counter_Variables.dma_buffer_previous_index);
    WiFi_Counter_Variables.dma_buffer_count = 0;
    WiFi_Counter_Variables.dma_buffer_previous_index  = 0;
    return WIFI_TRUE;
  }
  return WIFI_FALSE;
}


/**
* @brief  Process_DMA_Buffer
*         Classify the DMA buffer messages as WIND/AT-S./AT+S./other
* @param  None
* @retval None
*/
void Process_DMA_Buffer(void)
{
  int wind_no;
  WiFi_Control_Variables.complete_message_recv = WIFI_TRUE;
  const char* pStr;

  while(WiFi_Control_Variables.complete_message_recv && (*(dma_buffer_ptr + WiFi_Counter_Variables.dma_buffer_count)) != 0x00)
  {
    memset(str,0x00,sizeof(str));
    bytes_read = 0;
    pStr = (const char*) dma_buffer_ptr + WiFi_Counter_Variables.dma_buffer_count;

    /* WIND Message */
    if(sscanf(pStr,"+WIND:%d:%[^\n]%n", &wind_no,str,&bytes_read))
    {
      /* if while reading bytes we have automatically switched to another buffer apart from the one we were filling */
      Check_for_Extra_Bytes(WIND);

      /* is complete message recv? */
      /* Check for complete message till 0x0A(\n) rather than 0x0D(\r).
         If we check till 0x0D(\r) and 0x0A(\n) is not recv then it(\n) might become a part of the next data */
      if(*(dma_buffer_ptr + WiFi_Counter_Variables.dma_buffer_count + bytes_read) == 0x0A)
      {
        #if DEBUG_PRINT
          printf("\r\n+WIND:%d:",wind_no);
          printf((const char*)str);
          printf("\r\n");
        #endif
        Adjust_DMA_Buffer_Index (bytes_read+1); 
        Process_DMA_Buffer_Messages(WIND,wind_no,(uint8_t *)str);
        
        if(WiFi_Counter_Variables.dma_buffer_count >= DMA_BUFFER_SIZE)
          is_End_of_Buffer_Reached();
      }
      else
      {
        if(is_End_of_Buffer_Reached()) 
        {
          #if DEBUG_PRINT
            printf("\r\n+WIND:%d:",wind_no);
            printf((const char*)str);
            printf("\r\n");
          #endif
        }
        else
        {
            /* corner case, if only "+WIND: or +WIND:55:" arrived */
            int bytes_in_dma_buffer = strlen((const char *)dma_buffer+WiFi_Counter_Variables.dma_buffer_count);
            if(WiFi_Counter_Variables.dma_buffer_count+bytes_in_dma_buffer == DMA_BUFFER_SIZE)
            {
                bytes_read = bytes_in_dma_buffer;
                is_End_of_Buffer_Reached();
            }
            else
              WiFi_Control_Variables.complete_message_recv = WIFI_FALSE;
        }
      }
    } // end of WIND message

    /* Command Received..AT+S.*/
    else if(sscanf(pStr,"AT+S.%[^\n]%n",str,&bytes_read))
    {
      /* if while reading bytes we have automatically switched to another buffer apart from the one we were filling */
      Check_for_Extra_Bytes(CMD);

      /* is complete message recv? */
      if(*(dma_buffer_ptr + (WiFi_Counter_Variables.dma_buffer_count + bytes_read)) == 0x0A)
      {
        #if DEBUG_PRINT
          printf("\r\n");
          printf("AT+S.");
          printf((const char*)str);
          printf("\r\n");
        #endif
        Adjust_DMA_Buffer_Index (bytes_read+1); 
        Process_DMA_Buffer_Messages(CMD,bytes_read,(uint8_t *)str);

       if(WiFi_Counter_Variables.dma_buffer_count == DMA_BUFFER_SIZE)
        is_End_of_Buffer_Reached();
      }
      else
      {
        /* complete message not received but end of buffer reached? */
        if(is_End_of_Buffer_Reached()) 
        {
          #if DEBUG_PRINT
            printf("AT+S.");
            printf((const char*)str);
            printf("\r\n");
          #endif  
        }
        else
        {
            /* corner case, if "AT+S." arrived */
            int bytes_in_dma_buffer = strlen((const char *)dma_buffer+WiFi_Counter_Variables.dma_buffer_count);
            if(WiFi_Counter_Variables.dma_buffer_count+bytes_in_dma_buffer == DMA_BUFFER_SIZE)
            {
                bytes_read = bytes_in_dma_buffer;
                is_End_of_Buffer_Reached();
            }
            else
              WiFi_Control_Variables.complete_message_recv = WIFI_FALSE;
        }
      }
    } // end of AT+S. message

    /* Response Received..AT-S.*/
    else if(sscanf(pStr,"AT-S.%[^\n]%n",str,&bytes_read))
    {
      /* if while reading bytes we have automatically switched to another buffer apart from the one we were filling */
      Check_for_Extra_Bytes(INTERIM_DATA);

      /* is complete message recv? */
      if(*(dma_buffer_ptr + (WiFi_Counter_Variables.dma_buffer_count + bytes_read)) == 0x0A)
      {
        /* If fw update or http request ongoing */
        if(WiFi_Control_Variables.enable_fw_update_read || WiFi_Control_Variables.enable_receive_http_response)
        {
          Process_DMA_Buffer_Messages(UNDEFINE,-1,(uint8_t *)pStr);
        }
        else
        {
          #if DEBUG_PRINT
            printf("\r\n");
            printf("AT-S.");
            printf((const char*)str);
            printf("\r\n");
          #endif
          Adjust_DMA_Buffer_Index (bytes_read+1); 
          if(strstr(str,"OK"))
            Process_DMA_Buffer_Messages(OK,-1,(uint8_t *)str);
          else if(strstr(str,"ERROR"))
            Process_DMA_Buffer_Messages(ERROR_MSG,-1,(uint8_t *)str);
          else
            Process_DMA_Buffer_Messages(INTERIM_DATA,strlen(str),(uint8_t *)str);
          
         if(WiFi_Counter_Variables.dma_buffer_count == DMA_BUFFER_SIZE)
            is_End_of_Buffer_Reached();
        }
      }
      else
      {
        if(is_End_of_Buffer_Reached()) 
        {
          #if DEBUG_PRINT
            printf("AT-S.");
            printf((const char*)str);
            printf("\r\n");
          #endif
        }
        else
        {
          /* corner case, if "AT-S." arrived */
          int bytes_in_dma_buffer = strlen((const char *)dma_buffer+WiFi_Counter_Variables.dma_buffer_count);
          if(WiFi_Counter_Variables.dma_buffer_count+bytes_in_dma_buffer == DMA_BUFFER_SIZE)
          {
              bytes_read = bytes_in_dma_buffer;
              is_End_of_Buffer_Reached();
          }
          else
            WiFi_Control_Variables.complete_message_recv = WIFI_FALSE;
        }
      }
    } // end of AT-S. message

    /* Some incomplete message/other messages...maybe result of scan*/
    else
    {
      /* a broken message in between the unused data */
      if((message_pending == WIFI_TRUE) && (WiFi_Control_Variables.pending_unused_data == WIFI_TRUE))
      {
        WiFi_Control_Variables.temporarily_modify_variable_value = WIFI_TRUE;
        WiFi_Control_Variables.pending_unused_data = WIFI_FALSE;
      }
      
      if(!WiFi_Control_Variables.enable_receive_http_response 
       && !WiFi_Control_Variables.enable_sock_read
       && !WiFi_Control_Variables.pending_unused_data)
      {
        sscanf(pStr,"%[^\n]%n",str,&bytes_read);

        /* if the value at WiFi_Counter_Variables.dma_buffer_count is \n, then bytes_read will have garbage value */
        if(*(dma_buffer_ptr+WiFi_Counter_Variables.dma_buffer_count) == 0x0A)
          bytes_read=0;

        /* if while reading bytes we have automatically switched to another buffer apart from the one we were filling */
        Check_for_Extra_Bytes(UNDEFINE);

        /* is complete message recv? */
        if(*(dma_buffer_ptr + (WiFi_Counter_Variables.dma_buffer_count + bytes_read)) == 0x0A)
        {
          /* The value at dma_buffer_count is 0x0A */
          #if DEBUG_PRINT
            printf("\r\n");
            printf((const char*)str);
            printf("\r\n");
          #endif
          Adjust_DMA_Buffer_Index (bytes_read+1);
          /* A broken message was encountered */
          if(message_pending)
          {
            /* strcat(destination,source) */
            strcat((char *)message_pending_buffer,str);
            pStr = (const char*)&message_pending_buffer[0];
            /* WIND */
            if(sscanf(pStr,"+WIND:%d:%[^\n]%n", &wind_no,str,&bytes_read))
              Process_DMA_Buffer_Messages(WIND,wind_no,(uint8_t *)str);
            else if(sscanf(pStr,"AT+S.%[^\n]%n",str,&bytes_read))
              Process_DMA_Buffer_Messages(CMD,bytes_read,(uint8_t *)str);
            else if(sscanf(pStr,"AT-S.%[^\n]%n",str,&bytes_read))
            {
              if(strstr(str,"OK"))
                Process_DMA_Buffer_Messages(OK,-1,(uint8_t *)str);
              else if(strstr(str,"ERROR"))
                Process_DMA_Buffer_Messages(ERROR_MSG,-1,(uint8_t *)str);
              else
                Process_DMA_Buffer_Messages(INTERIM_DATA,-1,(uint8_t *)str);
            } //end of else if
            else  /* To check if this condition will ever arise */
              Process_DMA_Buffer_Messages(UNDEFINE,bytes_read,message_pending_buffer);
            
            memset(message_pending_buffer,0x00,sizeof(message_pending_buffer));
            message_pending_buffer_index =0;
            message_pending = WIFI_FALSE;
            if(WiFi_Control_Variables.temporarily_modify_variable_value == WIFI_TRUE)
            {
              WiFi_Control_Variables.pending_unused_data = WIFI_TRUE;
              WiFi_Control_Variables.temporarily_modify_variable_value = WIFI_FALSE;
            }
          }
          else
            Process_DMA_Buffer_Messages(UNDEFINE,bytes_read,(uint8_t *)str);
          
         if(WiFi_Counter_Variables.dma_buffer_count == DMA_BUFFER_SIZE)
          is_End_of_Buffer_Reached();
        }
        else
        {
          if(is_End_of_Buffer_Reached()) 
          {
  //          printf("\r\n other\r\n");
  //          printf((const char*)str);
  //          printf("\r\n");
          }
          else
            WiFi_Control_Variables.complete_message_recv = WIFI_FALSE;
        }
      } //end of if
      else if(WiFi_Control_Variables.pending_unused_data)
      {
          /* change in the value of variable has to be done before calling the unused_data_handling(). */
          WiFi_Control_Variables.pending_unused_data = WIFI_FALSE;
          unused_data_handling(remaining_unused_datalength);
      }
      else
      {
        Process_DMA_Buffer_Messages(UNDEFINE,-1,(uint8_t *)pStr);
      }
    } // end of other message
 }  //end of while

  WiFi_Control_Variables.complete_message_recv = WiFi_Control_Variables.complete_message_recv == WIFI_FALSE ? WIFI_TRUE:WiFi_Control_Variables.complete_message_recv;
}  //end of process_dma_buffer_messages()



/**
* @brief  Process_DMA_Buffer_Messages
*         Process the classified DMA buffer messages
* @param  idn: identifier to the type of message(WIND/ Command(AT+S.)/ Response(AT-S.)/ other)
* @param  wind_no: wind_no in case of WIND message else -1
* @param  ptr: pointer to the message
* @retval None
*/

void Process_DMA_Buffer_Messages(int idn, int wind_no, uint8_t * ptr)
{
  static uint32_t Fillptr=0;
  char SocketId_No[2];
  char databytes_No[4];
  char * pStr;
  static uint8_t HTTP_Runway_Buff[10];  //Used to store the last 6 bytes in between User Callbacks during HTTP tx
  static uint8_t process_buffer[MAX_BUFFER_GLOBAL];

  message_classification_TypeDef message_type;
  reset_event(&wifi_instances.wifi_event);

  message_type = (message_classification_TypeDef)idn;

  if(!WiFi_Control_Variables.enable_receive_http_response
       && !WiFi_Control_Variables.enable_receive_wifi_scan_response 
       && !WiFi_Control_Variables.enable_sock_read
       && !WiFi_Control_Variables.enable_fw_update_read)
  {
    switch(message_type)
    {
      case WIND:
          //end of msg received. Will not receive any other msg till we process this.
          Stop_Timer();
          #if defined (USE_STM32L0XX_NUCLEO) || (USE_STM32F4XX_NUCLEO) || (USE_STM32L4XX_NUCLEO)
          __disable_irq();
          #endif
          Process_Wind_Indication(wind_no,ptr);
          #if defined (USE_STM32L0XX_NUCLEO) || (USE_STM32F4XX_NUCLEO) || (USE_STM32L4XX_NUCLEO)
          __enable_irq();
          #endif
          Start_Timer();

          if(!WiFi_Control_Variables.prevent_push_WIFI_event)
            {
                __disable_irq();
                push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                 __enable_irq();
                reset_event(&wifi_instances.wifi_event);
            }

          else if(!WiFi_Control_Variables.queue_wifi_wind_message)  //if we do not want to queue a WIND: message
            {
                reset_event(&wifi_instances.wifi_event);
                WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_FALSE;
                WiFi_Control_Variables.queue_wifi_wind_message = WIFI_TRUE;
            }
          if (!WiFi_Control_Variables.do_not_reset_push_WIFI_event) 
            WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_FALSE;

          Fillptr=0;
          if(WiFi_Control_Variables.enable_sock_read)
            WiFi_Control_Variables.Q_Contains_Message = WIFI_FALSE;
          else if(WiFi_Control_Variables.mqtt_data_available == WIFI_FALSE)
            return;
        break;

      case CMD:
          /* Two commands received simultaneously..ignore the previous command of which there is no response */
          if(Fillptr!=0)
          {
            memset(process_buffer,0x00,MAX_BUFFER_GLOBAL);
            Fillptr=0;
          }
          strcpy((char *)process_buffer,"AT+S.");
          strcpy((char *)process_buffer+5,(const char *)ptr);
          Fillptr += wind_no;
          if(WiFi_Control_Variables.Ok_terminated_data_request_pending)
          {
            WiFi_Control_Variables.enable_receive_http_response = WIFI_TRUE;
            WiFi_Control_Variables.Ok_terminated_data_request_pending = WIFI_FALSE;
            WiFi_Control_Variables.enable_timeout_timer = WIFI_FALSE; //stop the timeout timer as response from server received.
            WiFi_Counter_Variables.timeout_tick         = 0;
            Fillptr = 0;
            memset(process_buffer, 0x00, strlen((char const *)process_buffer));
          }
        break;

      case INTERIM_DATA:
        {
          /* AT+S.SCAN\r\n AT-S.Parsing Networks:%d */
          int network_count;
          int bytes1,bytes2;
          if(sscanf((const char*)ptr,"Parsing Networks:%d",&network_count))
          {
                  WiFi_Counter_Variables.user_scan_number = (WiFi_Counter_Variables.user_scan_number > network_count) ? network_count : WiFi_Counter_Variables.user_scan_number ;
                  WiFi_Control_Variables.enable_receive_wifi_scan_response = WIFI_TRUE;
                  return;
          }
          else if(sscanf((const char*)ptr,"Reading:%d:%d\r",&bytes1,&bytes2))
          {
                  WiFi_Counter_Variables.sock_total_count = 0;
                  WiFi_Control_Variables.enable_sock_read = WIFI_TRUE;

                  if(WiFi_Counter_Variables.dma_buffer_count)
                    memset(dma_buffer_ptr+WiFi_Counter_Variables.dma_buffer_previous_index,0x00,WiFi_Counter_Variables.dma_buffer_count - WiFi_Counter_Variables.dma_buffer_previous_index);
                  else
                    memset(dma_buffer_ptr+WiFi_Counter_Variables.dma_buffer_previous_index,0x00,1024 - WiFi_Counter_Variables.dma_buffer_previous_index);
                  WiFi_Counter_Variables.dma_buffer_previous_index = WiFi_Counter_Variables.dma_buffer_count;
          }

          else {
                  strcpy((char*)process_buffer+Fillptr,"AT-S.");
                  Fillptr+=5;
                  strcpy((char*)process_buffer+Fillptr,(const char*)ptr);
                  Fillptr += wind_no;
          }
        break;
        }

      case OK:
           /*Now Check to which AT Cmd response this OK belongs to so that correct parsing can be done*/

          // SOCKON ID (Open a client socket)
          //Reply to SOCKON for SPWF04 -> AT-S.On:<IP Address>:0
          if(((pStr=(strstr((const char *)process_buffer,"AT+S.SOCKON"))) != NULL))      
            {
                /* Stop the timer, response of client_socket_open received. */
                WiFi_Control_Variables.enable_timeout_timer = WIFI_FALSE;
                WiFi_Counter_Variables.timeout_tick = 0;

                char *token = strrchr((char *)process_buffer,':');
                SocketId_No[0] = *(token+1);

                wifi_instances.wifi_event.socket_id = (SocketId_No[0] - '0');
                wifi_instances.wifi_event.event     =  WIFI_SOCK_ID_EVENT;
                __disable_irq();
                push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                 __enable_irq();
                reset_event(&wifi_instances.wifi_event);
                memset(process_buffer,0x00,MAX_BUFFER_GLOBAL);
                Fillptr=0;
             }
          
          //Reply to WSOCKON (websocket) for SPWF04 -> +AT_S.WSOCKON=<IPAddress>,<port>,,0,,,,,\r\nAT-S.On:0\r\nAt-S.OK\r\n
           else if(((pStr=(strstr((const char *)process_buffer,"AT+S.WSOCKON"))) != NULL))
             {
                WiFi_Control_Variables.enable_timeout_timer = WIFI_FALSE;
                WiFi_Counter_Variables.timeout_tick = 0;

                char *token = strtok((char *)process_buffer, ":");
                //just use the 2nd token
                token = strtok(NULL, ":");
                //token = strtok(NULL, ":");
                
                SocketId_No[0]    = *token ;
                //SocketId_No[1]    = *(pStr + 5) ;
                
                wifi_instances.wifi_event.socket_id = (SocketId_No[0] - '0');
                wifi_instances.wifi_event.event     =  WIFI_WEBSOCK_ID_EVENT;
                __disable_irq();
                push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                 __enable_irq();
                reset_event(&wifi_instances.wifi_event);
             }
          
          // Datalen from websocket
            else if((pStr=(strstr((const char *)process_buffer,"AT+S.WSOCKQ"))) != NULL)
             {
                char *token = strtok((char *)process_buffer, ":");
                //just use the second token
                token = strtok(NULL, ":");
                
                //Find the DataLength and do a socket read
                databytes_No[0] = *(token);
                databytes_No[1] = *(token + 1);
                databytes_No[2] = *(token + 2);
                databytes_No[3] = *(token + 3);

                if( databytes_No[1] == '\r')
                  {
                    WiFi_Counter_Variables.Socket_Data_Length = databytes_No[0] - '0'; 
                  }
                else if( databytes_No[2] == '\r')
                  {
                    WiFi_Counter_Variables.Socket_Data_Length = (((databytes_No[0] - '0') * 10 ) + (databytes_No[1] - '0'));
                  }
                else if( databytes_No[3] == '\r')
                  {
                    WiFi_Counter_Variables.Socket_Data_Length = (((databytes_No[0] - '0') * 100 ) + ((databytes_No[1] - '0') * 10 ) + (databytes_No[2] - '0'));
                  }
                else //it's a 4-digit number
                  {
                    WiFi_Counter_Variables.Socket_Data_Length  = ((databytes_No[0] - '0') * 1000 ) + ((databytes_No[1] - '0') * 100 ) + ((databytes_No[2] - '0') * 10) + (databytes_No[3] - '0');
                  }
                if(WiFi_Counter_Variables.Socket_Data_Length != 0)
                  {
                    WiFi_Control_Variables.start_websock_read = WIFI_TRUE;
                    WiFi_Control_Variables.data_pending_websocket = WIFI_TRUE;
                  }
                else if(WiFi_Counter_Variables.Socket_Data_Length == 0)  //no data remaining to be read
                  {
                    if(WiFi_Counter_Variables.socket_close_pending[WiFi_Counter_Variables.sockon_query_id])
                      {
                        // Q socket_close event for that socket for which sock_close command could not be processed earlier due to ERROR: pending data.
                         if(open_sockets[WiFi_Counter_Variables.sockon_query_id])
                          {
                            //Queue_Client_Close_Event(WiFi_Counter_Variables.sockon_query_id, NET_SOCKET);
                          }
                        WiFi_Counter_Variables.socket_close_pending[WiFi_Counter_Variables.sockon_query_id] = WIFI_FALSE;
                      }
                    WiFi_Control_Variables.data_pending_websocket = WIFI_FALSE;
                    WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;  //continue popping events if nothing to read
                  }
             }

            // DATALEN from SOCKQ
           else if((pStr=(strstr((const char *)process_buffer,"KQ"))) != NULL)
             {
                char *token = strtok((char *)process_buffer, ":");
                //just use the second token
                token = strtok(NULL, ":");

                //Find the DataLength and do a socket read
                databytes_No[0] = *(token);
                databytes_No[1] = *(token + 1);
                databytes_No[2] = *(token + 2);
                databytes_No[3] = *(token + 3);

                if( databytes_No[1] == '\r')
                  {
                    WiFi_Counter_Variables.Socket_Data_Length = databytes_No[0] - '0'; 
                  }
                else if( databytes_No[2] == '\r')
                  {
                    WiFi_Counter_Variables.Socket_Data_Length = (((databytes_No[0] - '0') * 10 ) + (databytes_No[1] - '0'));
                  }
                else if( databytes_No[3] == '\r')
                  {
                    WiFi_Counter_Variables.Socket_Data_Length = (((databytes_No[0] - '0') * 100 ) + ((databytes_No[1] - '0') * 10 ) + (databytes_No[2] - '0'));
                  }
                else //it's a 4-digit number
                  {
                    WiFi_Counter_Variables.Socket_Data_Length  = ((databytes_No[0] - '0') * 1000 ) + ((databytes_No[1] - '0') * 100 ) + ((databytes_No[2] - '0') * 10) + (databytes_No[3] - '0');
                  }
                if(WiFi_Counter_Variables.Socket_Data_Length != 0)
                  {
                    WiFi_Control_Variables.start_sock_read = WIFI_TRUE;
                    WiFi_Control_Variables.data_pending_sockW = WIFI_TRUE;
                  }
                else if(WiFi_Counter_Variables.Socket_Data_Length == 0)  //no data remaining to be read
                  {
                    WiFi_Control_Variables.data_pending_sockW = WIFI_FALSE;
                    WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;  //continue popping events if nothing to read
                  }
                memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);
                return;
              }

          // DATALEN from SOCKDQ
           else if((pStr=(strstr((const char *)process_buffer,"DQ"))) != NULL)
             {
                char *token = strtok((char *)process_buffer, ":");
                //just use the second token
                token = strtok(NULL, ":");

                //Find the DataLength and do a socket read
                databytes_No[0] = *(token);
                databytes_No[1] = *(token + 1);
                databytes_No[2] = *(token + 2);
                databytes_No[3] = *(token + 3);

                if( databytes_No[1] == '\r')
                  {
                    WiFi_Counter_Variables.Socket_Data_Length = databytes_No[0] - '0'; 
                  }
                else if( databytes_No[2] == '\r')
                  {
                    WiFi_Counter_Variables.Socket_Data_Length = (((databytes_No[0] - '0') * 10 ) + (databytes_No[1] - '0'));
                  }
                else if( databytes_No[3] == '\r')
                  {
                    WiFi_Counter_Variables.Socket_Data_Length = (((databytes_No[0] - '0') * 100 ) + ((databytes_No[1] - '0') * 10 ) + (databytes_No[2] - '0'));
                  }
                else //it's a 4-digit number
                  {
                    WiFi_Counter_Variables.Socket_Data_Length  = ((databytes_No[0] - '0') * 1000 ) + ((databytes_No[1] - '0') * 100 ) + ((databytes_No[2] - '0') * 10) + (databytes_No[3] - '0');
                  }
                if(WiFi_Counter_Variables.Socket_Data_Length != 0)
                  {
                    WiFi_Control_Variables.start_sockd_read = WIFI_TRUE;
                    WiFi_Control_Variables.data_pending_sockD = WIFI_TRUE; //to make sure callback is called correctly.
                  }
                else if(WiFi_Counter_Variables.Socket_Data_Length == 0)  //no data remaining to be read
                  {
                    memset(process_buffer,0x00,Fillptr);
                    Fillptr=0;
                    if(WiFi_Control_Variables.close_specific_client)
                    {
                      if(WiFi_Counter_Variables.server_socket_close_pending[WiFi_Counter_Variables.sockdon_query_id][WiFi_Counter_Variables.sockon_query_id])
                      {
                        Queue_Server_Close_Event(WiFi_Counter_Variables.sockdon_query_id,WiFi_Counter_Variables.sockon_query_id);
                        WiFi_Counter_Variables.server_socket_close_pending[WiFi_Counter_Variables.sockdon_query_id][WiFi_Counter_Variables.sockon_query_id] = WIFI_FALSE;
                      }
                    }
                    WiFi_Control_Variables.data_pending_sockD = WIFI_FALSE;
                    WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;  //continue popping events if nothing to read
                  }
                memset(process_buffer,0x00,MAX_BUFFER_GLOBAL);
                Fillptr=0;
             }

           //Reply to SOCKDON (server socket) for SPWF04 -> AT-S.On:0
          else if((pStr=(strstr((const char *)process_buffer,"AT+S.SOCKDON"))) != NULL)
            {
                /* Stop the timer, response of client_socket_open received. */
                WiFi_Control_Variables.enable_timeout_timer = WIFI_FALSE;
                WiFi_Counter_Variables.timeout_tick = 0;

                char *token = strtok((char *)process_buffer, ":");
                //just use the 2nd token
                token = strtok(NULL, ":");
                //token = strtok(NULL, ":");
                
                SocketId_No[0]    = *token ;
                //SocketId_No[1]    = *(pStr + 5) ;
                
                wifi_instances.wifi_event.socket_id = (SocketId_No[0] - '0');
                wifi_instances.wifi_event.event     =  WIFI_SOCK_SERVER_ID_EVENT;
                __disable_irq();
                push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                 __enable_irq();
                reset_event(&wifi_instances.wifi_event);
                memset(process_buffer,0x00,MAX_BUFFER_GLOBAL);
                Fillptr=0;
             }
          
           else if((pStr = (char *)(strstr((const char *)process_buffer,"AT+S.STS"))) != NULL || 
                   (pStr = (char *)(strstr((const char *)process_buffer,"AT+S.GCFG"))) != NULL)
             {
                // AT command AT+S.GCFG or AT+S.STS
                char *token = strtok((char *)process_buffer, "=");
                //just use the 3rd token
                token = strtok(NULL, "=");
                token = strtok(NULL, "=");
                char* ptr = token;
                char* ptr_usr = (char *)WiFi_Counter_Variables.get_cfg_value;
                while(*ptr != '\r') *(ptr_usr++) = *(ptr++);//copy the var from token to get_cfg_value
                
                /* do not queue gcfg event when setting epoch time as we are waiting for the Ok in tim handler only */
                IO_status_flag.AT_Response_Received = WIFI_TRUE;
                WiFi_Counter_Variables.AT_RESPONSE = WiFi_MODULE_SUCCESS;

                memset(process_buffer,0x00,MAX_BUFFER_GLOBAL);
                Fillptr=0;
             }
          
           else if((strstr((const char *)process_buffer,"AT+S.GPIOR")) != NULL) 
              {
                  // AT+S.GPIOR
                char *token = strtok((char *)process_buffer, ":");
                //just use the 3rd and 4th token
                token = strtok(NULL, ":");
                token = strtok(NULL, ":");
                
                WiFi_Counter_Variables.gpio_value = *token - '0';
                token = strtok(NULL, ":");
                WiFi_Counter_Variables.gpio_dir= *token - '0';    //out
                    
                  //Push GPIO Read Event on Event_Queue
                  wifi_instances.wifi_event.event = WIFI_GPIO_EVENT;
                  __disable_irq();
                  push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                   __enable_irq();
                  reset_event(&wifi_instances.wifi_event);
                  Fillptr=0;
                  memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);

                  if(WiFi_Control_Variables.enable_sock_read)
                    WiFi_Control_Variables.Q_Contains_Message = WIFI_FALSE;
                  else
                    return;
              }
           else
             {
                //This is a standalone OK
                /*Cases possible
                - TLSCERT,TLSCERT2, TLSDOMAIN, SETTIME
                - S.SOCKW, SOCKR, S.SOCKC, S.SOCKD (open a server socket)
                - File Operations
                - S.GPIOC and S.GPIOW
                */
                //Push a simple OK Event, if this is an OK event required to be pushed to Q 
                if(IO_status_flag.prevent_push_OK_event)
                  {
                      //This OK is not to be handled, hence the pop action on OK completion to be done here                              
                      if(IO_status_flag.client_socket_close_ongoing) //OK received is of the sock close command
                        {
                            if(WiFi_Counter_Variables.no_of_open_client_sockets > 0)
                              WiFi_Counter_Variables.no_of_open_client_sockets--;
                            IO_status_flag.prevent_push_OK_event                         = WIFI_FALSE;
                            #if DEBUG_PRINT
                              printf("\r\n >> Socket Closed ID : %d\r\n",WiFi_Counter_Variables.remote_socket_closed_id);
                            #endif
                            open_sockets[WiFi_Counter_Variables.remote_socket_closed_id] = WIFI_FALSE;
                            //socket ID for which OK is received.
                            WiFi_Counter_Variables.closed_socket_id                      = WiFi_Counter_Variables.remote_socket_closed_id;
                            IO_status_flag.client_socket_close_ongoing                   = WIFI_FALSE;

                            //User Callback not required in case if sock_close is called by User (Only in WIND:58)
                            if(Client_Socket_Close_Callback[WiFi_Counter_Variables.closed_socket_id])
                              {
                                // User callback after successful socket Close
                                WiFi_Control_Variables.SockON_Server_Closed_Callback = WIFI_TRUE;
                                /*start de-Q only after the callback is successfully made*/
                              }
                            else
                              {
                                /* start de-Q only if we do not need a callback. */
                                Client_Socket_Close_Callback[WiFi_Counter_Variables.closed_socket_id] = WIFI_TRUE;
                                WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
                              }
                        }
                      else if(IO_status_flag.web_socket_close_ongoing)
                        {
                            IO_status_flag.prevent_push_OK_event    = WIFI_FALSE;
                            open_web_socket[WiFi_Counter_Variables.remote_socket_closed_id] = WIFI_FALSE;
                            //socket ID for which OK is received.
                            WiFi_Counter_Variables.closed_socket_id = WiFi_Counter_Variables.remote_socket_closed_id;
                            IO_status_flag.web_socket_close_ongoing = WIFI_FALSE;
                            IO_status_flag.client_socket_close_type = WEB_SOCKET;
                            WiFi_Control_Variables.SockON_Server_Closed_Callback = WIFI_TRUE;
                        }
                      else if(IO_status_flag.server_socket_close_ongoing)
                        {
                            if(WiFi_Control_Variables.close_complete_server_socket)
                            {
                              for(int i=0;i<8;i++)
                                client_connected_on_server_socket[WiFi_Counter_Variables.remote_server_closed_id][i] = WIFI_FALSE;
                              open_server_sockets[WiFi_Counter_Variables.remote_server_closed_id] = WIFI_FALSE;
                              WiFi_Control_Variables.close_complete_server_socket = WIFI_FALSE;
                            }
                            else
                            {
                              client_connected_on_server_socket[WiFi_Counter_Variables.remote_server_closed_id][WiFi_Counter_Variables.remote_socket_closed_id] = WIFI_FALSE;
                              WiFi_Control_Variables.close_specific_client = WIFI_FALSE;
                            }
                            WiFi_Control_Variables.stop_event_dequeue    = WIFI_FALSE;
                            IO_status_flag.server_socket_close_ongoing   = WIFI_FALSE;
                        }
                  }
                else
                  {
                    /* result of input ssi, do not queue Ok as Timx is disabled */
                    if(WiFi_Control_Variables.fill_buffer_command_ongoing)
                    {
                      IO_status_flag.AT_Response_Received = WIFI_TRUE;
                      WiFi_Counter_Variables.AT_RESPONSE = WiFi_MODULE_SUCCESS;
                      IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
                      WiFi_Control_Variables.fill_buffer_command_ongoing = WIFI_FALSE;
                    }
                    else
                    {
                      wifi_instances.wifi_event.event = WIFI_OK_EVENT;
                      __disable_irq();
                      push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                       __enable_irq();
                      reset_event(&wifi_instances.wifi_event);
                    }
                  }
                IO_status_flag.prevent_push_OK_event = WIFI_FALSE;
                
                if(WiFi_Control_Variables.Scan_Ongoing == WIFI_TRUE)//If Scan was ongoing
                {
                  WiFi_Control_Variables.Scan_Ongoing = WIFI_FALSE; //Enable next scan             
                  WiFi_Counter_Variables.scanned_ssids=0;                       
                  WiFi_Control_Variables.enable_receive_wifi_scan_response = WIFI_FALSE;
                }
             }
          Fillptr=0;
          memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);

          if(WiFi_Control_Variables.enable_sock_read)
            WiFi_Control_Variables.Q_Contains_Message = WIFI_FALSE;
          else
            return;
        
        break;
        
        
    case ERROR_MSG:
          //This is an ERROR
          //There can be only ONE outstanding AT command and hence this ERROR belongs to that
          //HTTP -> ERROR: host not found
          //@TBD: Check all Errors Possible here???
          if((strstr((const char *)ptr,"77:Pending data\r\n")) != NULL) //if Error after sock close command and not 'OK'
            {
              /* Error: Pending data received for server socket */
              if(( pStr = strstr((const char *)process_buffer,"AT+S.SOCKDC")) != NULL)
              {
                printf("\r\n Error: Server socket could not be closed \r\n");
                IO_status_flag.server_socket_close_ongoing = WIFI_FALSE;
                IO_status_flag.prevent_push_OK_event       = WIFI_FALSE;
                if(WiFi_Control_Variables.close_specific_client)
                    WiFi_Counter_Variables.server_socket_close_pending[WiFi_Counter_Variables.remote_server_closed_id][WiFi_Counter_Variables.remote_socket_closed_id] = WIFI_TRUE;
                WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
              }
              else
              {
                printf("\r\nERROR: Socket could not be closed..PENDING DATA\r\n");
                //prevent the OK received after socket close command to be Q'ed
                IO_status_flag.prevent_push_OK_event        = WIFI_FALSE;     
                IO_status_flag.client_socket_close_ongoing  = WIFI_FALSE;

                //when error while user trying to close a socket, so now callback required whenever the socket gets closed.
                if(!Client_Socket_Close_Callback[WiFi_Counter_Variables.remote_socket_closed_id])
                  {
                    //enable the SockON_Server_Closed_Callback
                    Client_Socket_Close_Callback[WiFi_Counter_Variables.remote_socket_closed_id] = WIFI_TRUE;
                  }
                //close the socket for which ERROR is received afterwards, after reading the data on that socket
                WiFi_Counter_Variables.socket_close_pending[WiFi_Counter_Variables.remote_socket_closed_id] = WIFI_TRUE;
                WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
              }
            }
          else
            {
                if(IO_status_flag.server_socket_close_ongoing && (strstr((const char *)ptr,"76:Illegal Socket ID")) != NULL)
                  {
                      memset(process_buffer,0x00,sizeof(process_buffer));
                      Fillptr = 0;
                      IO_status_flag.server_socket_close_ongoing = WIFI_FALSE;
                      IO_status_flag.prevent_push_OK_event       = WIFI_FALSE;
                      WiFi_Control_Variables.stop_event_dequeue  = WIFI_FALSE;
                      break;
                  }
                else if(strstr((const char *)ptr,"78:Socket not connected") != NULL)
                  {
                    remaining_unused_datalength = WiFi_Counter_Variables.curr_DataLength;
                    unused_data_handling(remaining_unused_datalength);
                  }
                else if(WiFi_Control_Variables.fill_buffer_command_ongoing)
                  {
                    WiFi_Control_Variables.fill_buffer_command_ongoing = WIFI_FALSE;
                    IO_status_flag.AT_Response_Received = WIFI_TRUE;
                    WiFi_Counter_Variables.AT_RESPONSE = WiFi_AT_CMD_RESP_ERROR;
                    IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
                    Fillptr=0;
                    memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);
                    return;
                  }

                wifi_instances.wifi_event.event = WIFI_ERROR_EVENT;
                __disable_irq();
                push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                 __enable_irq();
                reset_event(&wifi_instances.wifi_event);
                if(WiFi_Control_Variables.stop_event_dequeue)
                  /*ERROR:Illegal Socket ID*/
                  WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;//continue popping events if nothing to read
                if(WiFi_Control_Variables.enable_timeout_timer)
                  {
                    WiFi_Control_Variables.enable_timeout_timer = WIFI_FALSE;
                    WiFi_Counter_Variables.timeout_tick = 0;
                  }
            }
          Fillptr=0;
          memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);

          if(WiFi_Control_Variables.enable_sock_read)
            WiFi_Control_Variables.Q_Contains_Message = WIFI_FALSE;
          else
            return;
      break;
    default:
      break;
    } // end of switch
  } //end of if

  else if(WiFi_Control_Variables.in_standby_mode)
    {
      /* Eliminate the NULL values received after WIND:67:Going into Standby */
      if(process_buffer[Fillptr-1] == '\0')
        Fillptr = 0;
    }
  else if(Fillptr >= MAX_BUFFER_GLOBAL-1)
    {
      #if DEBUG_PRINT
      printf("\rJust Looping with unhandled data!\r\n");
      #endif          
      Fillptr=0;          
      memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL); 
    }

  /*********************************************************************************************
   *                                                                                           *
   *                             Socket Read Is Enabled.                                       *
   *                                                                                           *
   ********************************************************************************************/

 if(WiFi_Control_Variables.enable_sock_read)     /*read is enabled*/
  {
      uint32_t length_of_data_to_read = 0;
      uint32_t interim_length;

      while(WiFi_Counter_Variables.sock_total_count < WiFi_Counter_Variables.SockON_Data_Length)
      {
          pStr = (char*) (dma_buffer_ptr + WiFi_Counter_Variables.dma_buffer_count);
          interim_length = WiFi_Counter_Variables.SockON_Data_Length - WiFi_Counter_Variables.sock_total_count;
          length_of_data_to_read = (interim_length > MAX_BUFFER_GLOBAL) \
                                 ? MAX_BUFFER_GLOBAL \
                                 : interim_length;

          interim_length = DMA_BUFFER_SIZE - WiFi_Counter_Variables.dma_buffer_count;
          length_of_data_to_read= interim_length >= length_of_data_to_read ? length_of_data_to_read : interim_length;
          length_of_data_to_read = length_of_data_to_read+WiFi_Counter_Variables.UserDataBuff_index > MAX_BUFFER_GLOBAL         \
                       ? MAX_BUFFER_GLOBAL - WiFi_Counter_Variables.UserDataBuff_index                                          \
                       : length_of_data_to_read;

          /* Less data in dma buffer, than expected */
          #if defined (USE_STM32L4XX_NUCLEO) || defined (USE_STM32F1xx_NUCLEO) || defined (USE_STM32L0XX_NUCLEO)
          int ndtr_value = DMA_BUFFER_SIZE - (UartWiFiHandle.hdmarx->Instance->CNDTR);
          #elif defined (USE_STM32F4XX_NUCLEO)
          int ndtr_value = DMA_BUFFER_SIZE - (UartWiFiHandle.hdmarx->Instance->NDTR);
          #endif
      
          //int ndtr_value = DMA_BUFFER_SIZE - (UartWiFiHandle.hdmarx->Instance->CNDTR);
          if(ndtr_value >= WiFi_Counter_Variables.dma_buffer_count && ndtr_value <= WiFi_Counter_Variables.dma_buffer_count + length_of_data_to_read)
              length_of_data_to_read = length_of_data_to_read > (ndtr_value - WiFi_Counter_Variables.dma_buffer_count)     \
                                     ? (ndtr_value - WiFi_Counter_Variables.dma_buffer_count)                              \
                                     : length_of_data_to_read;

          memcpy(UserDataBuff + WiFi_Counter_Variables.UserDataBuff_index, pStr,length_of_data_to_read);

          WiFi_Counter_Variables.sock_total_count += length_of_data_to_read;
          WiFi_Counter_Variables.UserDataBuff_index += length_of_data_to_read;
          WiFi_Counter_Variables.dma_buffer_count+=length_of_data_to_read;
          memset(dma_buffer_ptr+WiFi_Counter_Variables.dma_buffer_previous_index,0x00,WiFi_Counter_Variables.dma_buffer_count - WiFi_Counter_Variables.dma_buffer_previous_index);
          WiFi_Counter_Variables.dma_buffer_previous_index = WiFi_Counter_Variables.dma_buffer_count;

          if(WiFi_Counter_Variables.dma_buffer_count == DMA_BUFFER_SIZE)
          {
            WiFi_Counter_Variables.dma_buffer_count = 0;
            WiFi_Counter_Variables.dma_buffer_previous_index  = 0;
          }

          /* Complete data REcv */
          if(WiFi_Counter_Variables.sock_total_count >= WiFi_Counter_Variables.SockON_Data_Length)
          {
            if(WiFi_Control_Variables.data_pending_sockD)
              {
                //do we have more data?
                WiFi_Control_Variables.enable_server_query = WIFI_TRUE;
                WiFi_Control_Variables.enable_query = WIFI_FALSE;
                WiFi_Control_Variables.enable_websocket_query = WIFI_FALSE;
                WiFi_Counter_Variables.sockon_id_user = WiFi_Counter_Variables.sockon_query_id;
                WiFi_Counter_Variables.sockdon_id_user = WiFi_Counter_Variables.sockdon_query_id;
              }
            else if(WiFi_Control_Variables.data_pending_sockW)
              {
                //do we have more data?
                WiFi_Control_Variables.enable_query = WIFI_TRUE;
                WiFi_Control_Variables.enable_server_query = WIFI_FALSE;
                WiFi_Control_Variables.enable_websocket_query = WIFI_FALSE;
                WiFi_Counter_Variables.sockdon_id_user = -1;
                WiFi_Counter_Variables.sockon_id_user = WiFi_Counter_Variables.sockon_query_id;
              }
            else if(WiFi_Control_Variables.data_pending_websocket)
              {
                WiFi_Control_Variables.enable_websocket_query = WIFI_TRUE;
                WiFi_Control_Variables.enable_server_query = WIFI_FALSE;
                WiFi_Control_Variables.enable_query = WIFI_FALSE;
                WiFi_Counter_Variables.sockon_id_user = 0;
                WiFi_Counter_Variables.sockdon_id_user = -1;
              }
            /*@TODO: Do not need to prevent OK push in case of server socket*/
            IO_status_flag.prevent_push_OK_event = WIFI_TRUE; //prevent the qeueuing of the OK after this read operation
            WiFi_Control_Variables.enable_sock_read = WIFI_FALSE;

            if(WiFi_Control_Variables.data_pending_sockD || WiFi_Control_Variables.data_pending_sockW)
              {
                /* Callback to uSer */
                ind_wifi_socket_data_received(WiFi_Counter_Variables.sockdon_id_user, WiFi_Counter_Variables.sockon_id_user, (uint8_t *)UserDataBuff, WiFi_Counter_Variables.SockON_Data_Length, WiFi_Counter_Variables.UserDataBuff_index, NET_SOCKET);
              }
            else if(WiFi_Control_Variables.data_pending_websocket)
              {
                //set this to callback to user with User Buffer pointer
                ind_wifi_socket_data_received(WiFi_Counter_Variables.sockdon_id_user, WiFi_Counter_Variables.sockon_id_user, (uint8_t *)UserDataBuff, WiFi_Counter_Variables.SockON_Data_Length, WiFi_Counter_Variables.UserDataBuff_index, WEB_SOCKET);
              }
            WiFi_Counter_Variables.UserDataBuff_index =0;
            memset(UserDataBuff ,0x00,MAX_BUFFER_GLOBAL);
            break;
          }

          /* UserDataBuff full */
          else if (WiFi_Counter_Variables.UserDataBuff_index == MAX_BUFFER_GLOBAL)
          {
              WiFi_Counter_Variables.sockon_id_user = WiFi_Counter_Variables.sockon_query_id;
              if(WiFi_Control_Variables.data_pending_sockD)
                {
                  WiFi_Counter_Variables.sockdon_id_user = WiFi_Counter_Variables.sockdon_query_id;
                  ind_wifi_socket_data_received(WiFi_Counter_Variables.sockdon_id_user, WiFi_Counter_Variables.sockon_id_user, (uint8_t *)UserDataBuff, WiFi_Counter_Variables.SockON_Data_Length,512,NET_SOCKET);
                }
              else if(WiFi_Control_Variables.data_pending_sockW)
                {
                  WiFi_Counter_Variables.sockdon_id_user = -1;
                  ind_wifi_socket_data_received(WiFi_Counter_Variables.sockdon_id_user, WiFi_Counter_Variables.sockon_id_user, (uint8_t *)UserDataBuff, WiFi_Counter_Variables.SockON_Data_Length,512,NET_SOCKET);
                }
              else if(WiFi_Control_Variables.data_pending_websocket)
                {
                  WiFi_Counter_Variables.sockdon_id_user = -1;
                  WiFi_Counter_Variables.sockon_id_user = 0;
                  ind_wifi_socket_data_received(WiFi_Counter_Variables.sockdon_id_user, WiFi_Counter_Variables.sockon_id_user, (uint8_t *)UserDataBuff, WiFi_Counter_Variables.SockON_Data_Length, 512,WEB_SOCKET);
                }
            WiFi_Counter_Variables.UserDataBuff_index = 0;
            memset(UserDataBuff,0x00,MAX_BUFFER_GLOBAL);
          }
      } //end of while
  }
  
  /********************************************************************************************
   *                                                                                           *
   *                             HTTP/ Response Is Enabled.                                    *
   *                                                                                           *
   ********************************************************************************************/

  else if (WiFi_Control_Variables.enable_receive_http_response) // http response enabled
  {
    int complete_message_length,length;
    while(!WiFi_Control_Variables.request_complete)
    {
      #if defined (USE_STM32L4XX_NUCLEO) || defined (USE_STM32F1xx_NUCLEO) || defined (USE_STM32L0XX_NUCLEO)
      int ndtr_value = DMA_BUFFER_SIZE - (UartWiFiHandle.hdmarx->Instance->CNDTR);
      #elif defined (USE_STM32F4XX_NUCLEO)
      int ndtr_value = DMA_BUFFER_SIZE - (UartWiFiHandle.hdmarx->Instance->NDTR);
      #endif
      
      length = strlen((char const *)dma_buffer + WiFi_Counter_Variables.dma_buffer_count); // Find the length of data available in dma_buffer
      if(length == (int)NULL && ndtr_value != WiFi_Counter_Variables.dma_buffer_count) // could be no more data or null is a part of data
        length = 1;  //memcpy one NULL char at a time.
      else
      {
        length = WiFi_Counter_Variables.dma_buffer_count + length > DMA_BUFFER_SIZE  \
              ? DMA_BUFFER_SIZE - WiFi_Counter_Variables.dma_buffer_count           \
              : length;

        /* If data available in dma_buffer > 512, then firstly use 512 bytes else use the length of bytes available */
        length = length >= MAX_BUFFER_GLOBAL ? MAX_BUFFER_GLOBAL : length;

        /* If we have data in UserDataBuff, then reduce the length of length of data  */
        length = (length + WiFi_Counter_Variables.UserDataBuff_index)>MAX_BUFFER_GLOBAL ? MAX_BUFFER_GLOBAL - WiFi_Counter_Variables.UserDataBuff_index : length;
      }
      memcpy(UserDataBuff+WiFi_Counter_Variables.UserDataBuff_index,dma_buffer_ptr + WiFi_Counter_Variables.dma_buffer_count,length);
      WiFi_Counter_Variables.UserDataBuff_index += length;

      /* OK received */
      if((pStr = ((char*)memmem(UserDataBuff,WiFi_Counter_Variables.UserDataBuff_index,"AT-S.OK\r\n",sizeof("AT-S.OK\r\n")-1))) != NULL)
      {
        printf("\r\n Complete Data received \r\n");
        complete_message_length = (uint8_t *)pStr - (uint8_t *)UserDataBuff;
        memset(UserDataBuff + complete_message_length,0x00, WiFi_Counter_Variables.UserDataBuff_index-complete_message_length);
        WiFi_Counter_Variables.UserDataBuff_index = complete_message_length;

        if(WiFi_Counter_Variables.UserDataBuff_index < WiFi_Counter_Variables.UserDataBuff_previous_index)
        {
          /* Case when AT-S.OK is broken, so Ok will not be Q'ed. 
             Make user wait variable TRUE from here only */
          Adjust_DMA_Buffer_Index((WiFi_Counter_Variables.UserDataBuff_index+9)- WiFi_Counter_Variables.UserDataBuff_previous_index);
          WiFi_Control_Variables.broken_message = WIFI_TRUE;
        }
        else
          Adjust_DMA_Buffer_Index(complete_message_length - WiFi_Counter_Variables.UserDataBuff_previous_index);
        memset(dma_buffer_ptr+WiFi_Counter_Variables.dma_buffer_previous_index,0x00,WiFi_Counter_Variables.dma_buffer_count - WiFi_Counter_Variables.dma_buffer_previous_index);
        WiFi_Counter_Variables.dma_buffer_previous_index = WiFi_Counter_Variables.dma_buffer_count;
        complete_message_length = 0;

        /* While reading data EOB reached */
        if(WiFi_Counter_Variables.dma_buffer_count == DMA_BUFFER_SIZE)
        {
          WiFi_Counter_Variables.dma_buffer_count = 0;
          WiFi_Counter_Variables.dma_buffer_previous_index  = 0;
        }

        if(WiFi_Control_Variables.enable_receive_socket_list_response)
        {
          ind_wifi_socket_list_data_available((uint8_t *)UserDataBuff);
          WiFi_Control_Variables.enable_receive_socket_list_response = WIFI_FALSE;
        }
        else if(WiFi_Control_Variables.enable_receive_file_response)
        {
          ind_wifi_file_data_available((uint8_t *)UserDataBuff);
          WiFi_Control_Variables.enable_receive_file_response = WIFI_FALSE;
        }
        else
          ind_wifi_http_data_available((uint8_t *)UserDataBuff,WiFi_Counter_Variables.UserDataBuff_index);
        WiFi_Counter_Variables.UserDataBuff_index = 0;
        WiFi_Counter_Variables.UserDataBuff_previous_index =0;
        WiFi_Control_Variables.request_complete = WIFI_TRUE;
        WiFi_Control_Variables.enable_receive_http_response = WIFI_FALSE;
        
        WiFi_Control_Variables.enable_fw_update_read = WIFI_FALSE;
        memset(UserDataBuff,0x00,MAX_BUFFER_GLOBAL);

        if(WiFi_Control_Variables.broken_message)
        {
          WiFi_Control_Variables.broken_message = WIFI_FALSE;
          IO_status_flag.AT_Response_Received = WIFI_TRUE;
          WiFi_Counter_Variables.AT_RESPONSE = WiFi_MODULE_SUCCESS;
        }
        WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
      }

      /* ERROR received */
      else if((pStr = ((char*)memmem(UserDataBuff,WiFi_Counter_Variables.UserDataBuff_index,"AT-S.ERROR:",sizeof("AT-S.ERROR:")-1))) != NULL)
      {
          int len = ((uint8_t *)pStr - (uint8_t *)UserDataBuff);
          /* \r\n after AT-S.ERROR: */
          if((pStr = ((char*)strstr(UserDataBuff + len,"\r\n"))) != NULL)
          {
            #ifdef DEBUG_PRINT
              printf((const char *)UserDataBuff);
              printf("\r\n\r\n");
            #endif
            len = ((uint8_t *)pStr - (uint8_t *)UserDataBuff)+2;
            Adjust_DMA_Buffer_Index(len - WiFi_Counter_Variables.UserDataBuff_previous_index);
            IO_status_flag.AT_Response_Received = WIFI_TRUE;
            WiFi_Counter_Variables.AT_RESPONSE = WiFi_AT_CMD_RESP_ERROR;
            WiFi_Control_Variables.enable_receive_http_response = WIFI_FALSE;
            WiFi_Control_Variables.request_complete = WIFI_TRUE;

            memset(dma_buffer_ptr+WiFi_Counter_Variables.dma_buffer_previous_index,0x00,WiFi_Counter_Variables.dma_buffer_count - WiFi_Counter_Variables.dma_buffer_previous_index);
            WiFi_Counter_Variables.dma_buffer_previous_index = WiFi_Counter_Variables.dma_buffer_count;

            /* While reading data EOB reached */
            if(WiFi_Counter_Variables.dma_buffer_count == DMA_BUFFER_SIZE)
            {
              WiFi_Counter_Variables.dma_buffer_count = 0;
              WiFi_Counter_Variables.dma_buffer_previous_index  = 0;
            }
            memset(UserDataBuff,0x00,MAX_BUFFER_GLOBAL);
            WiFi_Counter_Variables.UserDataBuff_index = 0;
            WiFi_Counter_Variables.UserDataBuff_previous_index=0;
            WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
          }
        else
        {
          Adjust_DMA_Buffer_Index(length);
          memset(dma_buffer_ptr+WiFi_Counter_Variables.dma_buffer_previous_index,0x00,WiFi_Counter_Variables.dma_buffer_count - WiFi_Counter_Variables.dma_buffer_previous_index);
          WiFi_Counter_Variables.dma_buffer_previous_index = WiFi_Counter_Variables.dma_buffer_count;
          WiFi_Counter_Variables.UserDataBuff_previous_index = WiFi_Counter_Variables.UserDataBuff_index;

          /* While reading data EOB reached */
          if(WiFi_Counter_Variables.dma_buffer_count == DMA_BUFFER_SIZE)
          {
            WiFi_Counter_Variables.dma_buffer_count = 0;
            WiFi_Counter_Variables.dma_buffer_previous_index  = 0;
          }
        }
      }

      /* 512 bytes */
      else if(WiFi_Counter_Variables.UserDataBuff_index == MAX_BUFFER_GLOBAL)
      {
        /* increase dma_buffer_count by 504 bytes only, if UserDataBuff_previous_index is 0 */
        if(MAX_BUFFER_GLOBAL-8 >= WiFi_Counter_Variables.UserDataBuff_previous_index)
        {
          //in case AT-S.OK\r\n is splitted, so leave last 8 bytes
          memset(UserDataBuff + MAX_BUFFER_GLOBAL-8, 0x00, 8);  //memset last 8 bytes
          Adjust_DMA_Buffer_Index((MAX_BUFFER_GLOBAL-8) - WiFi_Counter_Variables.UserDataBuff_previous_index);
        }
        else  /* More bytes read than required */
        {
           memcpy(HTTP_Runway_Buff,UserDataBuff+504,8);
           Adjust_DMA_Buffer_Index(length);
           memset(UserDataBuff + MAX_BUFFER_GLOBAL-8, 0x00, 8);  //memset last 8 bytes
           WiFi_Control_Variables.runway_buffer_contains_data = WIFI_TRUE;
        }

        memset(dma_buffer_ptr+WiFi_Counter_Variables.dma_buffer_previous_index,0x00,WiFi_Counter_Variables.dma_buffer_count - WiFi_Counter_Variables.dma_buffer_previous_index);
        WiFi_Counter_Variables.dma_buffer_previous_index = WiFi_Counter_Variables.dma_buffer_count;
        WiFi_Counter_Variables.UserDataBuff_index = MAX_BUFFER_GLOBAL-8;

        /* While reading data EOB reached */
        if(WiFi_Counter_Variables.dma_buffer_count == DMA_BUFFER_SIZE)
        {
          WiFi_Counter_Variables.dma_buffer_count = 0;
          WiFi_Counter_Variables.dma_buffer_previous_index  = 0;
        }

        /* Callback to user */
        if(WiFi_Control_Variables.enable_receive_socket_list_response)
          ind_wifi_socket_list_data_available((uint8_t *)UserDataBuff);
        else if(WiFi_Control_Variables.enable_receive_file_response)
          ind_wifi_file_data_available((uint8_t *)UserDataBuff);
        else
        ind_wifi_http_data_available((uint8_t *)UserDataBuff,WiFi_Counter_Variables.UserDataBuff_index);

        WiFi_Counter_Variables.UserDataBuff_index = 0;
        WiFi_Counter_Variables.UserDataBuff_previous_index=0;
        memset(UserDataBuff,0x00,MAX_BUFFER_GLOBAL);

        if(WiFi_Control_Variables.runway_buffer_contains_data)
        {
          memcpy(UserDataBuff,HTTP_Runway_Buff,8);
          WiFi_Counter_Variables.UserDataBuff_index = 8;
          WiFi_Counter_Variables.UserDataBuff_previous_index = 8;
          memset(HTTP_Runway_Buff,0x00, sizeof(HTTP_Runway_Buff));
          WiFi_Control_Variables.runway_buffer_contains_data = WIFI_FALSE;
        }
      }

      /* Intermediate data */
      else
      {
        Adjust_DMA_Buffer_Index(length);
        WiFi_Counter_Variables.UserDataBuff_previous_index = WiFi_Counter_Variables.UserDataBuff_index;
        memset(dma_buffer_ptr+WiFi_Counter_Variables.dma_buffer_previous_index,0x00,WiFi_Counter_Variables.dma_buffer_count - WiFi_Counter_Variables.dma_buffer_previous_index);
        WiFi_Counter_Variables.dma_buffer_previous_index = WiFi_Counter_Variables.dma_buffer_count;

        /* While reading data EOB reached */
        if(WiFi_Counter_Variables.dma_buffer_count == DMA_BUFFER_SIZE)
        {
          WiFi_Counter_Variables.dma_buffer_count = 0;
          WiFi_Counter_Variables.dma_buffer_previous_index  = 0;
        }
      }
    } //end of while
    WiFi_Control_Variables.request_complete = WIFI_FALSE;
  } //end of enable_receive_http_response

  /*********************************************************************************************
   *                                                                                           *
   *                             WiFi Scan Is Enabled.                                         *
   *                                                                                           *
   ********************************************************************************************/
    
   else if(WiFi_Control_Variables.enable_receive_wifi_scan_response)
    {
      char bss[100];

      if((char*)strstr((const char *)ptr,"OK\r") != NULL)
      {
          WiFi_Control_Variables.enable_receive_wifi_scan_response = WIFI_FALSE;
          WiFi_Control_Variables.Scan_Ongoing = WIFI_FALSE; //Enable next scan
          WiFi_Counter_Variables.scanned_ssids=0;
          memset(bss,0x00,sizeof(bss));

          /* Process and Queue the OK event. */
          wifi_instances.wifi_event.event = WIFI_OK_EVENT;
          __disable_irq();
          push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
           __enable_irq();
          reset_event(&wifi_instances.wifi_event);
      }
      else
      {
        if(WiFi_Counter_Variables.scanned_ssids < WiFi_Counter_Variables.user_scan_number)
        {
            uint8_t scanned_ssids = WiFi_Counter_Variables.scanned_ssids;
            sscanf((const char*)ptr,wifi_scan_string,&scanned_ssids,bss,&wifi_scanned_list[scanned_ssids].channel_num,  \
                         &wifi_scanned_list[scanned_ssids].rssi,wifi_scanned_list[scanned_ssids].ssid,process_buffer);

            pStr = (char *) strstr((const char *)&process_buffer,"WPA ");
            if(pStr != NULL)
                {
                    wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wpa = WIFI_TRUE;
                } else
                    wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wpa = WIFI_FALSE;

            pStr = (char *) strstr((const char *)&process_buffer,"WPA2 ");
            if(pStr != NULL)
                {
                    wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wpa2 = WIFI_TRUE;
                } else
                    wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wpa2 = WIFI_FALSE;

            pStr = (char *) strstr((const char *)&process_buffer,"WPS ");
            if(pStr != NULL)
                {
                    wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wps = WIFI_TRUE;
                } else
                    wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wps = WIFI_FALSE;
            WiFi_Counter_Variables.scanned_ssids = scanned_ssids;
        }
      }
      Fillptr = 0;
      memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);
    } //end of scan
  
  /*********************************************************************************************
  *                                                                                           *
  *                          MQTT Data Callback Is Enabled.                                   *
  *                                                                                           *
  ********************************************************************************************/
  else if(WiFi_Control_Variables.mqtt_data_available)
    {
      char *mqtt_string = "MQTT Published:%u:%[^:]:%*u:%*u:%*u:%u:%u:%n";
      int length_of_data_to_read =0;
      int data_read = 0;
      int length = 0;
      sscanf(str,mqtt_string,&client_id,topic,&message_size,&total_message_size,&length);

      while(data_read < message_size)
      {
        length_of_data_to_read = ((message_size - data_read) > MAX_BUFFER_GLOBAL) ? MAX_BUFFER_GLOBAL : message_size - data_read;
        pStr = (char *)(str + length);
        memcpy(UserDataBuff, pStr,length_of_data_to_read);
        WiFi_Counter_Variables.UserDataBuff_index = length_of_data_to_read;
        data_read+= length_of_data_to_read;

        if(data_read >= message_size)
        {
          ind_wifi_mqtt_data_received(client_id,topic,length_of_data_to_read,message_size,total_message_size,(uint8_t *)UserDataBuff);
          WiFi_Control_Variables.mqtt_data_available = WIFI_FALSE;
          WiFi_Counter_Variables.UserDataBuff_index = 0;
          memset(UserDataBuff,0x00,MAX_BUFFER_GLOBAL);
          memset(dma_buffer_ptr + WiFi_Counter_Variables.dma_buffer_previous_index,0x00, WiFi_Counter_Variables.dma_buffer_count - WiFi_Counter_Variables.dma_buffer_previous_index);
          WiFi_Counter_Variables.dma_buffer_previous_index = WiFi_Counter_Variables.dma_buffer_count;
          break;
        }
        else if(data_read == MAX_BUFFER_GLOBAL)
        {
          ind_wifi_mqtt_data_received(client_id,topic,MAX_BUFFER_GLOBAL,message_size,total_message_size,(uint8_t *)UserDataBuff);
          WiFi_Counter_Variables.UserDataBuff_index = 0;
          memset(UserDataBuff,0x00,MAX_BUFFER_GLOBAL);
        }
      }
      memset(str,0x00,sizeof(str));
  } //  else if(WiFi_Control_Variables.mqtt_data_available)
} 


/**
* @brief  Process_Wind_Indication_Cmd
*         Process Wind indication before queueing
* @param  process_buff_ptr: pointer of WiFi indication buffer
* @retval None
*/

void Process_Wind_Indication(int wind_no,uint8_t *ptr)
{
  char * ptr_offset;
  char databytes_No[4]; 
  int i;
  char *token;

  WiFi_Indication_t Wind_No = Undefine_Indication;

  Wind_No = (WiFi_Indication_t)wind_no;

  wifi_instances.wifi_event.wind = Wind_No;
  wifi_instances.wifi_event.event = WIFI_WIND_EVENT;
  switch (Wind_No)
  {
    case SockON_Data_Pending: /*WIND:55*/
      /*+WIND:55:Pending Data:<ServerID>:<ClientID>:<received_bytes>:<cumulated_bytes>*/   
      ptr_offset = (char *) strstr((const char *)ptr,"Data:");//ServerID is empty
      if(*(ptr_offset + 5) == ':')//means it is a client socket WIND:55
      {
        /*Need to find out which socket ID has data pending*/
        databytes_No[0] = *(ptr_offset + 6);
        wifi_instances.wifi_event.socket_id = (databytes_No[0] - '0'); //Max number of sockets is 8 (so single digit)
        wifi_instances.wifi_event.server_id = 9;
      }
      else //it is a server socket ID
      {
        /*Need to find out which server socket ID has data pending*/
        databytes_No[0] = *(ptr_offset + 5);
        databytes_No[1] = *(ptr_offset + 7);
        wifi_instances.wifi_event.server_id = (databytes_No[0] - '0'); //Max number of sockets is 8 (so single digit)
        wifi_instances.wifi_event.socket_id = (databytes_No[1] - '0');
      }
      break;

    case SockON_Server_Socket_Closed:
      /* +WIND:58:Socket Closed:Client_id:Closure Reason */
      ptr_offset = (char *) strstr((const char *)ptr,"Closed:");//ServerID is empty
      //Find the id of the socket closed
      databytes_No[0] = *(ptr_offset + 7) ;
      wifi_instances.wifi_event.socket_id = databytes_No[0] - '0'; //Max number of sockets is 8 (so single digit)
      break;

    case WiFi__MiniAP_Associated:
        //Find out which client joined by parsing the WIND //+WIND:28
        for(i=19;i<=35;i++)
          WiFi_Counter_Variables.client_MAC_address[i-19] = *(ptr + i) ;    
        IO_status_flag.WiFi_WIND_State = WiFiAPClientJoined;
        break;

    case WiFi_MiniAP_Disassociated:
        //Find out which client left by parsing the WIND //+WIND:72
        for(i=22;i<=39;i++)
          WiFi_Counter_Variables.client_MAC_address[i-17] = *(ptr + i) ;
        IO_status_flag.WiFi_WIND_State = WiFiAPClientLeft;
        break;

    case Incoming_socket_client:
        //Incoming Socket Client:<Client IP-Address>:<Client Port>:<Server ID>:<Client ID> 
        token = strrchr((char *)ptr, ':');
        wifi_instances.wifi_event.socket_id = *(token+1)-'0' ;
        *token='\0';
        token=strrchr((char *)ptr, ':');
        wifi_instances.wifi_event.server_id = *(token+1)-'0' ;                    
        //@TBD: Map IP Address to client number
        break;

    case Outgoing_socket_client:
        // Socket Client Gone:client IP:Client Port:server ID:client ID:closure reason
        token = strrchr((char *)ptr, ':');
        *token='\0';
        token=strrchr((char *)ptr, ':');
        wifi_instances.wifi_event.socket_id = *(token+1)-'0' ;
        *token='\0';
        token=strrchr((char *)ptr, ':');
        wifi_instances.wifi_event.server_id = *(token+1)-'0' ;                    
        break;
        
    case Websocket_Data:
        /*A websocket pending event has arrived*/
        /*+WIND:88:WebSocket Data:<ClientID>:<last_frame_flag?>:<last_frag_flag?>:<rcv_bytes>:<cumulated_bytes>*/
        //printf("^^\r\n");
        ptr_offset = (char *) strstr((const char *)ptr,"Data:");
        databytes_No[0] = *(ptr_offset + 5);
        wifi_instances.wifi_event.socket_id = (databytes_No[0] - '0');//This is the websocket client ID
        break;

    case Websocket_Closed:
        /*+WIND:89:WebSocket Closed:<ClientID>*/
        //Find the id of the socket closed
        ptr_offset = (char *) strstr((const char *)ptr,"Closed:");
        databytes_No[0] = *(ptr_offset + 7);
        wifi_instances.wifi_event.socket_id = databytes_No[0] - '0'; //Max number of web-sockets is 2 (so single digit)
        break;
        
    case MQTT_Closed:
        /*+WIND:87:MQTT Closed:<Client ID> */
        ptr_offset = (char *) strstr((const char *)ptr,"Closed:");
        databytes_No[0] = *(ptr_offset + 7);
        wifi_instances.wifi_event.socket_id = (databytes_No[0] - '0');//This is the MQTT client ID
        break;

    case Remote_Configuration:
        wifi_connected = 0;
        WiFi_Control_Variables.queue_wifi_wind_message = WIFI_FALSE;
        WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_TRUE;
        break;

    case Going_Into_Standby:
        wifi_instances.wifi_event.event = WIFI_STANDBY_CONFIG_EVENT;
        WiFi_Control_Variables.in_standby_mode = WIFI_TRUE;
        break;

    case Resuming_From_Standby:
        wifi_instances.wifi_event.event = WIFI_RESUME_CONFIG_EVENT;
        WiFi_Control_Variables.in_standby_mode = WIFI_FALSE;
        break;
        
    case Input_To_Remote:
        /* Disable TIMx   */
        HAL_NVIC_DisableIRQ(TIMx_IRQn);
        ind_wifi_inputssi_callback();
        WiFi_Control_Variables.queue_wifi_wind_message = WIFI_FALSE;
        WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_TRUE;
        break;
        
    case Output_From_Remote: ;
        /* length of text to be received is 40 or lesss*/
        int bytes_read;
        sscanf((const char*)ptr,"Output from remote:%*d:%n",&bytes_read);
        ind_wifi_output_from_remote_callback((uint8_t *)ptr+bytes_read);
        break;

    // Queueing of these events not required.
    case MQTT_Published:
       WiFi_Control_Variables.mqtt_data_available = WIFI_TRUE;
    case Console_Active:
    case WiFi_Reset:
    case Watchdog_Running:
    case Watchdog_Terminating:
    case Configuration_Failure:
    case CopyrightInfo:
    case WiFi_BSS_Regained:
    case WiFi_Signal_OK:
    case FW_update:
    case Encryption_key_Not_Recognized:
    case WiFi_Join:
    case WiFi_Scanning:
    case WiFi_Association_Successful:
    case WiFi_BSS_LOST:
    case WiFi_NETWORK_LOST:
    case WiFi_Unhandled_Event:
    case WiFi_UNHANDLED:
    case WiFi_MiniAP_Mode :
    case DOT11_AUTHILLEGAL:
    case Creating_PSK:
    case WPA_Terminated :
    case WPA_Supplicant_Failed:
    case WPA_Handshake_Complete:
    case GPIO_line:
    case Wakeup:
    case Factory_debug:
    case Low_Power_Mode_Enabled:
    case Rejected_Found_Network:
    case TFTP_File_Received:
        WiFi_Control_Variables.queue_wifi_wind_message = WIFI_FALSE;
        WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_TRUE;
        break;

      //Queue these Events.
    case Poweron:
    case MallocFailed:                                          
        if(WiFi_Control_Variables.stop_event_dequeue)
          WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
        break;
    case Heap_Too_Small:            
    case WiFi_Hardware_Dead:
    case Hard_Fault:
    case StackOverflow:                   
    case Error:
    case WiFi_PS_Mode_Failure:
    case WiFi_Signal_LOW:
    case JOINFAILED:
    case SCANBLEWUP:
    case SCANFAILED:
    case WiFi_Started_MiniAP_Mode:
    case Start_Failed :
    case WiFi_EXCEPTION :
    case WiFi_Hardware_Started :
    case Scan_Complete:
    case WiFi_UNHANDLED_IND:
    case WiFi_Powered_Down:
    case WiFi_Deauthentication:
    case WiFi_Disassociation:
    case RX_MGMT:
    case RX_DATA:
    case RX_UNK:
    case NTP_Server_Delivery:
         break;
    default:
         break;
  } //end of switch
} //end of Process_Wind_Indication
#endif

/**
* @brief  Queue_Server_Write_Event
*         Queues a Server Socket write event.
* @param  sock_id socket ID to write to
* @param  DataLength length of the data to be written
* @param  pData pointer to data
* @retval None
*/
void Queue_Server_Write_Event(uint8_t server_id, uint8_t client_id, uint32_t DataLength, char * pData)
{
    Wait_For_Sock_Read_To_Complete();
    WiFi_Counter_Variables.curr_DataLength = DataLength;
    WiFi_Counter_Variables.curr_data = pData;
    WiFi_Counter_Variables.curr_sockID = client_id;
    WiFi_Counter_Variables.curr_serverID = server_id;

    wifi_instances.wifi_event.event = WIFI_SERVER_SOCKET_WRITE_EVENT;
    __disable_irq();
    push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
    __enable_irq();

    reset_event(&wifi_instances.wifi_event);
}


/**
  * @}
  */ 

/**
  * @}
  */ 


/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2015 STMicroelectronics *****END OF FILE****/

