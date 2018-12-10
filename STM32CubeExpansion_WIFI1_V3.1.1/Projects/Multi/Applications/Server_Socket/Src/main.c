  /**
  ******************************************************************************
  * @file    main.c
  * @author  Central LAB
  * @version V2.1.0
  * @date    17-May-2015
  * @brief   Main program body
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
#include "main.h"
#include "stdio.h"
#include "string.h"
#include "wifi_module.h"
#include "wifi_globals.h"
#include "wifi_interface.h"
#include "ntp.h"

/** @defgroup WIFI_Examples
  * @{
  */

/** @defgroup WIFI_Example_Server_Socket
  * @{
  */
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
  
/* Private function prototypes -----------------------------------------------*/
char print_msg_buff[512];

/* Private functions ---------------------------------------------------------*/                   
void 	SystemClock_Config(void);
void    UART_Msg_Gpio_Init(void);
void    USART_PRINT_MSG_Configuration(UART_HandleTypeDef *UART_MsgHandle, uint32_t baud_rate);
WiFi_Status_t 	wifi_get_AP_settings(void);

/* Private Declarartion ------------------------------------------------------*/
wifi_state_t wifi_state;
wifi_config config;
UART_HandleTypeDef UART_MsgHandle;

ntp_state_t ntp_state;

uint8_t console_input[1], console_count=0;
char console_ssid[40];
char console_psk[20];

char * ssid = "SiberNet";
char * seckey = "fragoleAndroid";
uint8_t channel_num = 6;
WiFi_Priv_Mode mode = WPA_Personal;     
char echo[64];
char ip_addr[16];
char mac_addr[17];
uint16_t len;
uint8_t sock_id, server_id;

char * gcfg_key1 = "ip_ipaddr";
char * gcfg_key2 = "nv_model";
uint8_t socket_id;
char wifi_ip_addr[20];
uint16_t len;
uint32_t baud_rate = 115200;

static TIM_HandleTypeDef s_TimerInstance = { 
    .Instance = TIM4
};







/*
		START TIMER SECTION
*/



void TIM4_IRQHandler()
{
    HAL_TIM_IRQHandler(&s_TimerInstance);
}


/*
*	Called to init timer preferences
*/
void InitializeTimer()
{
    __TIM4_CLK_ENABLE();
    s_TimerInstance.Init.Prescaler = SystemCoreClock/10000 - 1;
    s_TimerInstance.Init.CounterMode = TIM_COUNTERMODE_UP;
    s_TimerInstance.Init.Period = 10000;
    s_TimerInstance.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    s_TimerInstance.Init.RepetitionCounter = 0;
    HAL_TIM_Base_Init(&s_TimerInstance);
		HAL_TIM_Base_Start_IT(&s_TimerInstance);
		HAL_NVIC_SetPriority(TIM4_IRQn, 0, 0);
    
}

/*
 * Start the timer ticking
 */
void StartTimer()
{
	
  HAL_NVIC_EnableIRQ(TIM4_IRQn);
}


/*
 * Stop the timer ticking
 */
void StopTimer()
{
	HAL_NVIC_DisableIRQ(TIM4_IRQn);
}



/*
		END TIMER SECTION
*/




 /**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  char *protocol = "t";
  uint32_t portnumber = 32000;
  
	/*
			INIT PERIFERICHE
	*/
	
	
  __GPIOA_CLK_ENABLE();
  HAL_Init();

  /* Configure the system clock to 64 MHz */
  SystemClock_Config();

  /* configure the timers  */
  Timer_Config( );
	
	
  
#ifdef USART_PRINT_MSG
  UART_Msg_Gpio_Init();
  USART_PRINT_MSG_Configuration(&UART_MsgHandle,baud_rate);
  Set_UartMsgHandle(&UART_MsgHandle);
#endif  
  
	
	/* Configure NTP Timer */
	InitializeTimer();
	
  /*status = wifi_get_AP_settings();
  if(status!=WiFi_MODULE_SUCCESS)
  {
    printf("\r\nError in AP Settings");
    return 0;
  }*/
	
	/*
			INIT WIFI CONF
	*/
  
  config.power=wifi_active;
  config.power_level=high;
  config.dhcp=on;//use DHCP IP address  
  config.mcu_baud_rate = baud_rate;
  wifi_state = wifi_state_idle;

  UART_Configuration(baud_rate); 

  printf("\r\nInitializing the wifi module...\r\n");

  /* Init the wi-fi module */  
  status = wifi_init(&config);
  if(status!=WiFi_MODULE_SUCCESS)
  {
    printf("Error in Config");
    return 0;
  }
  
  printf("\r\nInitializing complete.\r\n");
	
	// Initializing ntp state machine to reset
	ntp_state = ntp_state_reset;
  
	
	int timerValue = 0;
	
  while (1)
  {
    switch (wifi_state) 
        {
      case wifi_state_reset:
      break;

      case wifi_state_ready:

        //printf("\r\n >>setting up miniAP mode...\r\n");
        
        //wifi_ap_start((uint8_t *)console_ssid, console_psk, channel_num, mode);  <--- Set Module as AP
			
			
			
				printf("\r\n >>Conntecting to ");
				printf(ssid);
				printf("...\r\n");
        wifi_connect(ssid,seckey, mode);  //  <--- Connecting to the network

        wifi_state = wifi_state_idle;
      break;

      case wifi_state_connected:
				/*
						WiFi CONNECTION ESTABLISHED
				*/
        printf("\r\n >>connected...\r\n");      
        
        WiFi_Status_t status;
        
        status = wifi_get_IP_address((uint8_t*)wifi_ip_addr);
        printf("\r\n>>IP address is %s\r\n", wifi_ip_addr);
        
        memset(wifi_ip_addr, 0x00, 20);
        
        status = wifi_get_MAC_address((uint8_t*)wifi_ip_addr);
        printf("\r\n>>mac addr is %s\r\n", wifi_ip_addr);
        
        wifi_state = wifi_state_connected_idle;
      break;

      case wifi_state_disconnected:
        printf("\r\n >>disconnected..\r\n");
        wifi_state = wifi_state_idle;
      break;

      case wifi_state_socket:
      printf("\r\n >>WiFi server socket opening..\r\n");

      /* Read Write Socket data */        
      #ifdef SPWF04
        status = wifi_socket_server_open(portnumber, (uint8_t *)protocol, &server_id);
      #else
        status = wifi_socket_server_open(portnumber, (uint8_t *)protocol);
      #endif
      if(status == WiFi_MODULE_SUCCESS)
      {
        printf("\r\n >>Server Socket Open OK \r\n");          
      }
        wifi_state = wifi_state_idle;

      break;

    case wifi_state_socket_write:
        printf("\r\n >>Writing data to client\r\n");

        len = strlen(echo);

        /* Read Write Socket data */        
        #ifdef SPWF04
          status = wifi_socket_server_write(server_id, sock_id, len, echo);
        #else
          status = wifi_socket_server_write(len, echo);
        #endif
        if(status == WiFi_MODULE_SUCCESS)
        {
          printf("\r\n >>Server Socket Write OK \r\n");  
        }
        wifi_state = wifi_state_idle;

        break;

    case wifi_state_idle:
			// Check for connection		
      printf(".");
      fflush(stdout);
      HAL_Delay(500);
      break;
		
		case wifi_state_connected_idle:
			// Check NTP state machine
      switch(ntp_state)
			{
				case ntp_state_reset:
					printf("\r\n >>Init NTP Request");
					//Reset Variable
					ntps.addr = NULL;
					ntps.server_list_index = 0;							
					ntps.secsSince1900 = 0;
					ntps.fase = 0;
					ntps.retries_counter=0;
					// Change state
					ntp_state = ntp_state_send;					
					break;				
				case ntp_state_send:
					printf("\r\n >>Sending NTP Request to server #%d\r\n",ntps.server_list_index);		
					// Send the NTP request
					ntpRequest();
						
					// Now wait the response
					ntp_state = ntp_state_waiting;
					break;
				case ntp_state_waiting:
					// Check if the answer has been arrived,
					if(ntps.secsSince1900 > 0)
					{
						// Answer ok
						ntp_state = ntp_state_idle;
					}
					else
					{
						// Check if the waiting time has been expired
						if(ntps.retries_counter == NTP_SERVER_MAX_RETRIES)
						{
							printf("\r\n >>No NTP Response, move forward..");
							if(ntps.server_list_index + 1 == NTP_SERVER_NUM)
							{
								printf("\r\n >>NTP Error, No other servers aviable");
								// Error, no one server is reachable
								ntp_state = ntp_state_error;
							}
							else
							{
								// send the request to the next server
								ntps.server_list_index++;
								// Reset the ntps struct
								ntps.secsSince1900 = 0;
								ntps.fase = 0;
								ntps.retries_counter=0;
								// Back to send state
								ntp_state = ntp_state_send;
							}
						}
						else
						{
							printf(".");
							// Keep waiting for the response (same server)
							ntps.retries_counter++;
							HAL_Delay(50);
						}
					}
					break;
				case ntp_state_error:
					StopTimer();
					printf("\r\n >>NTP Error, Reseting the procedure..");
					ntp_state = ntp_state_reset;
					break;
				case ntp_state_idle:					
					HAL_Delay(1);
					break;
				case ntp_undefine_state:
					break;
			}
      break;
			
			// Check aux devices Connection State Machine
			
    default:
      break;
    }    
  }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSI)
  *            SYSCLK(Hz)                     = 64000000
  *            HCLK(Hz)                       = 64000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 2
  *            APB2 Prescaler                 = 1
  *            PLLMUL                         = 16
  *            Flash Latency(WS)              = 2
  * @param  None
  * @retval None
  */

#ifdef USE_STM32F1xx_NUCLEO

void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef clkinitstruct = {0};
  RCC_OscInitTypeDef oscinitstruct = {0};
  
  /* Configure PLL ------------------------------------------------------*/
  /* PLL configuration: PLLCLK = (HSI / 2) * PLLMUL = (8 / 2) * 16 = 64 MHz */
  /* PREDIV1 configuration: PREDIV1CLK = PLLCLK / HSEPredivValue = 64 / 1 = 64 MHz */
  /* Enable HSI and activate PLL with HSi_DIV2 as source */
  oscinitstruct.OscillatorType  = RCC_OSCILLATORTYPE_HSE;
  oscinitstruct.HSEState        = RCC_HSE_ON;
  oscinitstruct.LSEState        = RCC_LSE_OFF;
  oscinitstruct.HSIState        = RCC_HSI_OFF;
  oscinitstruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  oscinitstruct.HSEPredivValue    = RCC_HSE_PREDIV_DIV1;
  oscinitstruct.PLL.PLLState    = RCC_PLL_ON;
  oscinitstruct.PLL.PLLSource   = RCC_PLLSOURCE_HSE;
  oscinitstruct.PLL.PLLMUL      = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&oscinitstruct)!= HAL_OK)
  {
    /* Initialization Error */
    while(1); 
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  clkinitstruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  clkinitstruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clkinitstruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clkinitstruct.APB2CLKDivider = RCC_HCLK_DIV1;
  clkinitstruct.APB1CLKDivider = RCC_HCLK_DIV2;  
  if (HAL_RCC_ClockConfig(&clkinitstruct, FLASH_LATENCY_2)!= HAL_OK)
  {
    /* Initialization Error */
    while(1); 
  }
}
#endif

#ifdef USE_STM32F4XX_NUCLEO

void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  
  /* Enable HSI Oscillator and activate PLL with HSI as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 0x10;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);
   
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;  
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}
#endif

#ifdef USE_STM32L0XX_NUCLEO

/**
 * @brief  System Clock Configuration
 * @param  None
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  __PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 0x10;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);//RCC_CLOCKTYPE_SYSCLK;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;//RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1);

  __SYSCFG_CLK_ENABLE(); 
}
#endif

#ifdef USE_STM32L4XX_NUCLEO
/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (MSI)
  *            SYSCLK(Hz)                     = 80000000
  *            HCLK(Hz)                       = 80000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1
  *            APB2 Prescaler                 = 1
  *            MSI Frequency(Hz)              = 4000000
  *            PLL_M                          = 1
  *            PLL_N                          = 40
  *            PLL_R                          = 2
  *            PLL_P                          = 7
  *            PLL_Q                          = 4
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  /* MSI is enabled after System reset, activate PLL with MSI as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLP = 7;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;  
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
}
#endif

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

#ifdef USART_PRINT_MSG
void USART_PRINT_MSG_Configuration(UART_HandleTypeDef *UART_MsgHandle, uint32_t baud_rate)
{
  UART_MsgHandle->Instance             = WIFI_UART_MSG;
  UART_MsgHandle->Init.BaudRate        = baud_rate;
  UART_MsgHandle->Init.WordLength      = UART_WORDLENGTH_8B;
  UART_MsgHandle->Init.StopBits        = UART_STOPBITS_1;
  UART_MsgHandle->Init.Parity          = UART_PARITY_NONE ;
  UART_MsgHandle->Init.HwFlowCtl       = UART_HWCONTROL_NONE;// USART_HardwareFlowControl_RTS_CTS;
  UART_MsgHandle->Init.Mode            = UART_MODE_TX_RX;

  if(HAL_UART_DeInit(UART_MsgHandle) != HAL_OK)
  {
    Error_Handler();
  }  
  if(HAL_UART_Init(UART_MsgHandle) != HAL_OK)
  {
    Error_Handler();
  }
      
}

void UART_Msg_Gpio_Init()
{ 
  GPIO_InitTypeDef  GPIO_InitStruct;

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO TX/RX clock */
  USARTx_PRINT_TX_GPIO_CLK_ENABLE();
  USARTx_PRINT_RX_GPIO_CLK_ENABLE();


  /* Enable USARTx clock */
  USARTx_PRINT_CLK_ENABLE(); 
    __SYSCFG_CLK_ENABLE();
  /*##-2- Configure peripheral GPIO ##########################################*/  
  /* UART TX GPIO pin configuration  */
  GPIO_InitStruct.Pin       = WiFi_USART_PRINT_TX_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
#if defined (USE_STM32L0XX_NUCLEO) || defined(USE_STM32F4XX_NUCLEO) || defined(USE_STM32L4XX_NUCLEO)
  GPIO_InitStruct.Alternate = PRINTMSG_USARTx_TX_AF;
#endif  
  HAL_GPIO_Init(WiFi_USART_PRINT_TX_GPIO_PORT, &GPIO_InitStruct);

  /* UART RX GPIO pin configuration  */
  GPIO_InitStruct.Pin = WiFi_USART_PRINT_RX_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_INPUT;
#if defined (USE_STM32L0XX_NUCLEO) || defined(USE_STM32F4XX_NUCLEO) || defined(USE_STM32L4XX_NUCLEO)
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Alternate = PRINTMSG_USARTx_RX_AF;
#endif 
  
  HAL_GPIO_Init(WiFi_USART_PRINT_RX_GPIO_PORT, &GPIO_InitStruct);
  
#ifdef WIFI_USE_VCOM
  /*##-3- Configure the NVIC for UART ########################################*/
  /* NVIC for USART */
  HAL_NVIC_SetPriority(USARTx_PRINT_IRQn, 0, 1);
  HAL_NVIC_EnableIRQ(USARTx_PRINT_IRQn);
#endif
}
#endif  // end of USART_PRINT_MSG


/**
  * @brief  Query the User for SSID, password and encryption mode
  * @param  None
  * @retval WiFi_Status_t
  */
WiFi_Status_t wifi_get_AP_settings(void)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  printf("\r\n\n/********************************************************\n");
  printf("\r *                                                      *\n");
  printf("\r * X-CUBE-WIFI1 Expansion Software V3.1.1               *\n");
  printf("\r * X-NUCLEO-IDW0xx1 Wi-Fi Mini-AP Configuration.        *\n");
  printf("\r * Server-Socket Example                                *\n");
  printf("\r *                                                      *\n");
  printf("\r *******************************************************/\n");
  printf("\r\nDo you want to setup SSID?(y/n):");
  fflush(stdout);
  scanf("%s",console_input);
  //console_input[0]='n';
  
  //HAL_UART_Receive(UartMsgHandle, (uint8_t *)console_input, 1, 100000);
  if(console_input[0]=='y') 
        {
              printf("\r\nEnter the SSID for mini-AP:");
              fflush(stdout);

              console_count=0;
              console_count=scanf("%s",console_ssid);
              printf("\r\n");

                if(console_count==39) 
                    {
                        printf("Exceeded number of ssid characters permitted");
                        return WiFi_NOT_SUPPORTED;
                    }
                
              printf("Enter the password:");
              fflush(stdout);
              console_count=0;
              
              console_count=scanf("%s",console_psk);
              printf("\r\n");

                if(console_count==19) 
                    {
                        printf("Exceeded number of psk characters permitted");
                        return WiFi_NOT_SUPPORTED;
                    }
        } else 
            {
                printf("\r\n\nModule will connect with default settings.");
                memcpy(console_ssid, (const char*)ssid, strlen((char*)ssid));
                memcpy(console_psk, (const char*)seckey, strlen((char*)seckey));
            }

  printf("\r\n/*************************************************************\r\n");
  printf("\r\n * Configuration Complete                                     \r\n");
  printf("\r\n * Please make sure a server is listening at given hostname   \r\n");
  printf("\r\n *************************************************************\r\n");

  return status;
}

/******** Wi-Fi Indication User Callback *********/

void ind_wifi_socket_data_received(int8_t callback_server_id, int8_t socket_id, uint8_t * data_ptr, uint32_t message_size, uint32_t chunk_size, WiFi_Socket_t socket_type)
{
	if(socket_id == ntps.server_id)
	{
		// NTP Response
		uint32_t time = parseAnswer(data_ptr, message_size);
		
		time = time -  NTP_SEVENTY_YEARS;
		printf("\r\nNTP Data Receive: %lu \r\n",(unsigned long)time);
		//ntp_state = ntp_state_idle;
		// Close now the socket, just 1 answer
		//wifi_socket_client_close(socket_id);
		// Start the incrementer timer
		StartTimer();
	}
	else
	{
		if(socket_id == server_id)
		{
			// Command response
			printf("\r\nData Receive Callback...\r\n");
			memcpy(echo, data_ptr, 50);
			printf((const char*)echo);
			printf("\r\nsocket ID: %d\r\n",socket_id);
			printf("msg size: %lu\r\n",(unsigned long)message_size);
			printf("chunk size: %lu\r\n",(unsigned long)chunk_size);
			fflush(stdout);
			sock_id = socket_id;//client_ID from where message has arrived
			server_id = callback_server_id;//server_ID from where message has arrived
			wifi_state = wifi_state_socket_write;
		}
	}
	
  
}

void ind_wifi_on()
{
    wifi_state = wifi_state_ready;
}

void ind_wifi_connected()
{
  wifi_state = wifi_state_connected;
}

void ind_socket_server_client_joined(void)
{
  printf("\r\nUser callback: Client joined...\r\n");
  fflush(stdout);
}

void ind_socket_server_client_left(void)
{
  printf("\r\nUser callback: Client left...\r\n");
  fflush(stdout);
}

void ind_wifi_ap_client_joined(uint8_t * client_mac_address)
{
  printf("\r\n>>client joined callback...\r\n");
  printf(">>client MAC address: ");
  printf((const char*)client_mac_address);
  fflush(stdout);  
}

void ind_wifi_ap_client_left(uint8_t * client_mac_address)
{
  printf("\r\n>>client left callback...\r\n");
  printf("\r\n >>client MAC address: ");
  printf((const char*)client_mac_address);
  fflush(stdout);  
}

void ind_wifi_error(WiFi_Status_t error_code)
{
  if(error_code == WiFi_AT_CMD_RESP_ERROR)
  {
    wifi_state = wifi_state_idle;
    printf("\r\n WiFi Command Failed. \r\n User should now press the RESET Button(B2). \r\n");
  }
}




/**
  * @}
  */
  
/**
* @}
*/
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
