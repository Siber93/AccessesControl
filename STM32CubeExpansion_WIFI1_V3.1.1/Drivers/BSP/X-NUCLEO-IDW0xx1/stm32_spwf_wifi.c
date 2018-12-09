/**
 ******************************************************************************
 * @file    stm32_spwf_wifi.c
 * @author  Central LAB
 * @version V2.1.0
 * @date    17-May-2016
 * @brief   HAL related functionality of X-CUBE-WIFI1
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
#include "stm32_spwf_wifi.h"
#include "wifi_globals.h"

/** @addtogroup BSP
* @{
*/ 


/** @defgroup  NUCLEO_WIFI_DRIVER
  * @brief Wi-Fi_driver modules
  * @{
  */


/** @defgroup NUCLEO_WIFI_DRIVER_Private_Defines
  * @{
  */
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Prescaler declaration */
uint32_t uwPrescalerValue = 0;  

/* TIM handle declaration */

/**
  * @}
  */

/** @addtogroup NUCLEO_WIFI_DRIVER_Private_Variables
  * @{
  */
/* Private variables ---------------------------------------------------------*/

/**
  * @}
  */

  
/** @defgroup NUCLEO_WIFI_DRIVER_Private_Functions
  * @{
  */

  /*##-1- Configure the TIM peripheral #######################################*/
  /* -----------------------------------------------------------------------
    In this example TIM3 input clock (TIM3CLK)  is set to APB1 clock (PCLK1) x2,
    since APB1 prescaler is set to 4 (0x100).
       TIM3CLK = PCLK1*2
       PCLK1   = HCLK/2
    => TIM3CLK = PCLK1*2 = (HCLK/2)*2 = HCLK = SystemCoreClock
    To get TIM3 counter clock at 10 KHz, the Prescaler is computed as following:
    Prescaler = (TIM3CLK / TIM3 counter clock) - 1
    Prescaler = (SystemCoreClock /10 KHz) - 1

    Note:
     SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f1xx.c file.
     Each time the core clock (HCLK) changes, user had to update SystemCoreClock
     variable value. Otherwise, any configuration based on this variable will be incorrect.
     This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetSysClockFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
  ----------------------------------------------------------------------- */
void Timer_Config()
{
  /* Compute the prescaler value to have TIMx counter clock equal to 10000 Hz */
  uwPrescalerValue = (uint32_t)(SystemCoreClock / 10000) - 1;
  
  /* Set TIMx instance */
  TimHandle.Instance = TIMx;

  /* Initialize TIMx peripheral as follows:
       + Period = 10000 - 1
       + Prescaler = (SystemCoreClock/10000) - 1
       + ClockDivision = 0
       + Counter direction = Up
  */
#if defined (USE_STM32L0XX_NUCLEO) || defined (USE_STM32F4XX_NUCLEO) || defined (USE_STM32L4XX_NUCLEO)
  TimHandle.Init.Period            = 100 - 1;
#endif
#if defined (USE_STM32F1xx_NUCLEO) 
  TimHandle.Init.Period            = 100 - 1;
#endif  
  TimHandle.Init.Prescaler         = uwPrescalerValue;
  TimHandle.Init.ClockDivision     = 0;
  TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
#ifdef USE_STM32F1xx_NUCLEO
  TimHandle.Init.RepetitionCounter = 0;
#endif 

  if (HAL_TIM_Base_Init(&TimHandle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler(); 
  }

}

/**
  * @brief Push_Timer_Config
  *        This function configures the Push Timer
  * @param None
  * @retval None
  */
void Push_Timer_Config()
{
  /* Compute the prescaler value to have TIMx counter clock equal to 10000 Hz */
  uwPrescalerValue = (uint32_t)(SystemCoreClock / 10000) - 1;
  
  /* Set TIMx instance */
  PushTimHandle.Instance = TIMp;

  /* Initialize TIMx peripheral as follows:
       + Period = 10000 - 1
       + Prescaler = (SystemCoreClock/10000) - 1
       + ClockDivision = 0
       + Counter direction = Up
  */
  PushTimHandle.Init.Period            = 100 - 1;//10000
  PushTimHandle.Init.Prescaler         = uwPrescalerValue;
  PushTimHandle.Init.ClockDivision     = 0;
  PushTimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
#ifdef USE_STM32F1xx_NUCLEO
  PushTimHandle.Init.RepetitionCounter = 0;
#endif 

  if (HAL_TIM_Base_Init(&PushTimHandle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler(); 
  }

}

/**
* @brief  USART_Configuration
* WB_WIFI_UART configured as follow:
*      - BaudRate = 115200 baud  
*      - Word Length = 8 Bits
*      - One Stop Bit
*      - No parity
*      - Hardware flow control enabled (RTS and CTS signals)
*      - Receive and transmit enabled
*
* @param  None
* @retval None
*/
void UART_Configuration(uint32_t baud_rate)
{
  UartWiFiHandle.Instance             = WB_WIFI_UART;
  UartWiFiHandle.Init.BaudRate        = baud_rate;
  UartWiFiHandle.Init.WordLength      = UART_WORDLENGTH_8B;
  UartWiFiHandle.Init.StopBits        = UART_STOPBITS_1;
  UartWiFiHandle.Init.Parity          = UART_PARITY_NONE ;

#ifdef SPWF04
  UartWiFiHandle.Init.HwFlowCtl       = UART_HWCONTROL_NONE;
#else
  UartWiFiHandle.Init.HwFlowCtl       = UART_HWCONTROL_RTS;
#endif
  UartWiFiHandle.Init.Mode            = UART_MODE_TX_RX;
  UartWiFiHandle.Init.OverSampling    = UART_OVERSAMPLING_16;
  //UartWiFiHandle.Init.OneBitSampling  = UART_ONEBIT_SAMPLING_ENABLED;
  
  if(HAL_UART_DeInit(&UartWiFiHandle) != HAL_OK)
  {
    Error_Handler();
  }  
  if(HAL_UART_Init(&UartWiFiHandle) != HAL_OK)
  {
    Error_Handler();
  }  
#ifdef WIFI_USE_VCOM  
  /*## -1- Enable USART1 DMAR #################################################*/
  UartWiFiHandle.Instance->CR3 |= 0x00000040;
#endif //WIFI_USE_VCOM   
 }

#if defined(SPWF01)
#define NUCLEO_GPIO_PIN                 GPIO_PIN_8
#define NUCLEO_GPIO                     GPIOC
#define __NUCLEO_GPIO_CLK_ENABLE()      __GPIOC_CLK_ENABLE()
#elif defined(SPWF04)
#define NUCLEO_GPIO_PIN                 GPIO_PIN_0
#define NUCLEO_GPIO                     GPIOB
#define __NUCLEO_GPIO_CLK_ENABLE()      __GPIOB_CLK_ENABLE()
#endif

uint32_t WiFi_CheckBaudrate_pin(void){
  GPIO_InitTypeDef GPIO_InitStruct;
  uint8_t i = 0;
  //uint32_t bauds[16] = {110, 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 0};  
  uint32_t bauds[6] = {4800, 9600, 115200, 460800, 921600, 0};
  GPIO_PinState Init_Pin_Value;
  GPIO_PinState End_Pin_Value;
  uint8_t gpio_value;
 
 // Configure InputGPIO
  __NUCLEO_GPIO_CLK_ENABLE();
 
  GPIO_InitStruct.Pin = NUCLEO_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
 
  HAL_GPIO_Init(NUCLEO_GPIO, &GPIO_InitStruct);
 
  Init_Pin_Value  = HAL_GPIO_ReadPin(NUCLEO_GPIO, NUCLEO_GPIO_PIN);
  End_Pin_Value = (Init_Pin_Value == GPIO_PIN_RESET) ? GPIO_PIN_SET : GPIO_PIN_RESET;
  gpio_value = (Init_Pin_Value == GPIO_PIN_RESET) ? 1 : 0;
 
  while(bauds[i] > 0){
    UART_Configuration(bauds[i]);
    HAL_Delay(10);
  
    Reset_AT_CMD_Buffer();
    sprintf((char*) WiFi_AT_Cmd_Buff, "AT+S.GPIOC=6,out\r");
    USART_Transmit_AT_Cmd(strlen((char*) WiFi_AT_Cmd_Buff));
    HAL_Delay(30); 
    Reset_AT_CMD_Buffer();
    sprintf((char*) WiFi_AT_Cmd_Buff, "AT+S.GPIOW=6,%d\r", gpio_value);
    USART_Transmit_AT_Cmd(strlen((char*) WiFi_AT_Cmd_Buff));
    HAL_Delay(30);
    
    if (HAL_GPIO_ReadPin(NUCLEO_GPIO, NUCLEO_GPIO_PIN) != End_Pin_Value){  
      i++;
      continue; // Failed to trigger  GPIO
    }
    break; 
  } 
  return bauds[i]; 
} 

 
uint32_t WiFi_CheckBaudrate(void){
 uint8_t i = 0;
 uint32_t bauds[6] = {4800, 9600, 115200, 460800, 921600, 0};
 
 while(bauds[i] > 0){
   UART_Configuration(bauds[i]);
   HAL_Delay(10);
   
   if (Attention_Cmd_Check() != WiFi_MODULE_SUCCESS)
   {
     i++;
     continue;
   }
   break;   
 }
 return bauds[i]; 
} 


HAL_StatusTypeDef WiFi_Module_UART_Configuration(uint32_t baud_rate)
{
  WiFi_Status_t status;
  uint32_t brcheck;
  
  IO_status_flag.WiFi_Enabled = WIFI_TRUE;  
  
  //brcheck = WiFi_CheckBaudrate_pin();
  brcheck = WiFi_CheckBaudrate();
  
  if (brcheck > 0) 
  {
    //if (brcheck != baud_rate)//must save and reset every-time and also re-configure UART
    {
      status = config_init_value(CONSOLE1_SPEED, baud_rate);
      
      Reset_AT_CMD_Buffer();  
      sprintf((char*)WiFi_AT_Cmd_Buff,AT_SAVE_CURRENT_SETTING);  
      status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));

      Reset_AT_CMD_Buffer();
      sprintf((char*)WiFi_AT_Cmd_Buff,AT_RESET);
      status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
      
      if (status == WiFi_MODULE_SUCCESS)
      {
        UART_Configuration(baud_rate); 
      }
	
      //Receive_Data(); 
    }
    return HAL_OK;
  }
  else  //Baudrate detection failed!
  {
    Error_Handler();
    return HAL_ERROR;
  }
 }

#if defined (USE_STM32F1xx_NUCLEO)
void DMA_ConfigAddress(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength)
{
  /* Configure DMA Channel data length */
  hdma->Instance->CNDTR = DataLength;
  
  /* Peripheral to Memory */
  if((hdma->Init.Direction) == DMA_MEMORY_TO_PERIPH)
  {
    /* Configure DMA Channel destination address */
    hdma->Instance->CPAR = DstAddress;
    
    /* Configure DMA Channel source address */
    hdma->Instance->CMAR = SrcAddress;
  }
  /* Memory to Peripheral */
  else
  {
    /* Configure DMA Channel source address */
    hdma->Instance->CPAR = SrcAddress;
    
    /* Configure DMA Channel destination address */
    hdma->Instance->CMAR = DstAddress;
  }
}
#endif

#if defined (USE_STM32F4XX_NUCLEO) || defined (USE_STM32L4XX_NUCLEO)
 
void WiFi_SPI_Init(SPI_HandleTypeDef * hspi)
{
  hspi->Instance = WIFI_SPI_INSTANCE;
  hspi->Init.Mode = WIFI_SPI_MODE;
  hspi->Init.Direction = WIFI_SPI_DIRECTION;
  hspi->Init.DataSize = WIFI_SPI_DATASIZE;
  hspi->Init.CLKPolarity = WIFI_SPI_CLKPOLARITY;
  hspi->Init.CLKPhase = WIFI_SPI_CLKPHASE;
  hspi->Init.NSS = WIFI_SPI_NSS;
  hspi->Init.FirstBit = WIFI_SPI_FIRSTBIT;
  hspi->Init.TIMode = WIFI_SPI_TIMODE;
  hspi->Init.CRCPolynomial = WIFI_SPI_CRCPOLYNOMIAL;
  hspi->Init.BaudRatePrescaler = WIFI_SPI_BAUDRATEPRESCALER;
  hspi->Init.CRCCalculation = WIFI_SPI_CRCCALCULATION;

  HAL_SPI_Init(hspi);

  __HAL_WIFI_SPI_ENABLE_DMAREQ(hspi, SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
  
  __HAL_SPI_ENABLE(hspi);
  
  return;
}

void SPI_Gpio_Init(SPI_HandleTypeDef* hspi)
{ 
  GPIO_InitTypeDef GPIO_InitStruct;
  if(hspi->Instance==WIFI_SPI_INSTANCE)
  {
    /* Enable peripherals clock */
    
    /* Enable GPIO Ports Clock */ 
    WIFI_SPI_SCLK_CLK_ENABLE();
    WIFI_SPI_MISO_CLK_ENABLE();
    WIFI_SPI_MOSI_CLK_ENABLE();
    WIFI_SPI_CS_CLK_ENABLE();
    WIFI_SPI_IRQ_CLK_ENABLE();
    
    /* Enable SPI clock */
    WIFI_SPI_CLK_ENABLE();
    
    /* SCLK */
    GPIO_InitStruct.Pin = WIFI_SPI_SCLK_PIN;
    GPIO_InitStruct.Mode = WIFI_SPI_SCLK_MODE;
    GPIO_InitStruct.Pull = WIFI_SPI_SCLK_PULL;
    GPIO_InitStruct.Speed = WIFI_SPI_SCLK_SPEED;
    GPIO_InitStruct.Alternate = WIFI_SPI_SCLK_ALTERNATE;
    HAL_GPIO_Init(WIFI_SPI_SCLK_PORT, &GPIO_InitStruct); 
    
    /* MISO */
    GPIO_InitStruct.Pin = WIFI_SPI_MISO_PIN;
    GPIO_InitStruct.Mode = WIFI_SPI_MISO_MODE;
    GPIO_InitStruct.Pull = WIFI_SPI_MISO_PULL;
    GPIO_InitStruct.Speed = WIFI_SPI_MISO_SPEED;
    GPIO_InitStruct.Alternate = WIFI_SPI_MISO_ALTERNATE;
    HAL_GPIO_Init(WIFI_SPI_MISO_PORT, &GPIO_InitStruct);
    
    /* MOSI */
    GPIO_InitStruct.Pin = WIFI_SPI_MOSI_PIN;
    GPIO_InitStruct.Mode = WIFI_SPI_MOSI_MODE;
    GPIO_InitStruct.Pull = WIFI_SPI_MOSI_PULL;
    GPIO_InitStruct.Speed = WIFI_SPI_MOSI_SPEED;
    GPIO_InitStruct.Alternate = WIFI_SPI_MOSI_ALTERNATE;
    HAL_GPIO_Init(WIFI_SPI_MOSI_PORT, &GPIO_InitStruct);
    
    /* NSS/CSN/CS */
    GPIO_InitStruct.Pin = WIFI_SPI_CS_PIN;
    GPIO_InitStruct.Mode = WIFI_SPI_CS_MODE;
    GPIO_InitStruct.Pull = WIFI_SPI_CS_PULL;
    GPIO_InitStruct.Speed = WIFI_SPI_CS_SPEED;
    GPIO_InitStruct.Alternate = WIFI_SPI_CS_ALTERNATE;
    HAL_GPIO_Init(WIFI_SPI_CS_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(WIFI_SPI_CS_PORT, WIFI_SPI_CS_PIN, GPIO_PIN_SET);
    
    /* IRQ -- INPUT */
    GPIO_InitStruct.Pin = WIFI_SPI_IRQ_PIN;
    GPIO_InitStruct.Mode = WIFI_SPI_IRQ_MODE;
    GPIO_InitStruct.Pull = WIFI_SPI_IRQ_PULL;
    GPIO_InitStruct.Speed = WIFI_SPI_IRQ_SPEED;
    GPIO_InitStruct.Alternate = WIFI_SPI_IRQ_ALTERNATE;
    HAL_GPIO_Init(WIFI_SPI_IRQ_PORT, &GPIO_InitStruct);
    
    /* Configure the NVIC for EXTI */  
    HAL_NVIC_SetPriority(WIFI_SPI_EXTI_IRQn, 2, 0);    
    //HAL_NVIC_EnableIRQ(WIFI_SPI_EXTI_IRQn);
  }
}

/**
 * @brief  Set in Output mode the IRQ.
 * @param  None
 * @retval None
 */
void set_irq_as_output(void)
{
  HAL_GPIO_WritePin(WIFI_SPI_IRQ_PORT, WIFI_SPI_IRQ_PIN, GPIO_PIN_SET);
  //HAL_LPPUART_GPIO_Set_Mode(WIFI_SPI_IRQ_PORT, WIFI_SPI_IRQ_PIN_POSITION, GPIO_MODE_OUTPUT_PP);
  __HAL_GPIO_EXTI_CLEAR_IT(WIFI_SPI_IRQ_PIN);
}

/**
 * @brief  Set the IRQ in input mode.
 * @param  None
 * @retval None
 */
void set_irq_as_input(void)
{
  HAL_GPIO_WritePin(WIFI_SPI_IRQ_PORT, WIFI_SPI_IRQ_PIN, GPIO_PIN_RESET); // WARNING: it may conflict with BlueNRG driving High
  //HAL_LPPUART_GPIO_Set_Mode(WIFI_SPI_IRQ_PORT, WIFI_SPI_IRQ_PIN_POSITION, GPIO_MODE_INPUT);
}

/**
 * @brief  Enable SPI IRQ.
 * @param  None
 * @retval None
 */
void Enable_SPI_Receiving_Path(void)
{  
  __HAL_GPIO_EXTI_CLEAR_IT(WIFI_SPI_EXTI_PIN);
  HAL_NVIC_ClearPendingIRQ(WIFI_SPI_EXTI_IRQn);
  HAL_NVIC_EnableIRQ(WIFI_SPI_EXTI_IRQn);
}

/**
 * @brief  Disable SPI IRQ.
 * @param  None
 * @retval None
 */
void Disable_SPI_Receiving_Path(void)
{  
  HAL_NVIC_DisableIRQ(WIFI_SPI_EXTI_IRQn);
}

/**
 * @brief  Enable SPI CS.
 * @param  None
 * @retval None
 */
void Enable_SPI_CS(void)
{
  /* CS reset */
  HAL_GPIO_WritePin(WIFI_SPI_CS_PORT, WIFI_SPI_CS_PIN, GPIO_PIN_RESET);
#if defined (USE_STM32F4XX_NUCLEO) || defined (USE_STM32L4XX_NUCLEO)
  while( HAL_GPIO_ReadPin(WIFI_SPI_CS_PORT, WIFI_SPI_CS_PIN) != GPIO_PIN_RESET );
#endif
}

/**
 * @brief  Disable SPI CS.
 * @param  None
 * @retval None
 */
void Disable_SPI_CS(void)
{
  while (__HAL_SPI_GET_FLAG(&SpiHandle,SPI_FLAG_BSY) == SET);
  
  /* CS set */
  HAL_GPIO_WritePin(WIFI_SPI_CS_PORT, WIFI_SPI_CS_PIN, GPIO_PIN_SET);
}

#define CS_PULSE_700NS_NBR_CYCLES_REQ  352
#define CS_PULSE_LENGTH (CS_PULSE_700NS_NBR_CYCLES_REQ/4)
#define DUMMY_RAM_ADDRESS_TO_READ (0x20000000)

/**
 * @brief  Disable and Enable SPI CS.
 * @param  None
 * @retval None
 */
void DisableEnable_SPI_CS(void)
{
  volatile uint8_t localloop;
  
  Disable_SPI_CS(); /**< CS Set */
  
  /**
   *  The CS shall be kept high for at least 625ns
   */
  for (localloop = 0 ; localloop < CS_PULSE_LENGTH ; localloop++)
  {
    *(volatile uint32_t*)DUMMY_RAM_ADDRESS_TO_READ;
  }
  
  Enable_SPI_CS(); /**< CS Reset */

  return;
}

#endif

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

