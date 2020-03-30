#include "stm32f4x7_eth.h"
#include "netconf.h"
#include "main.h"
#include "httpd.h"
#include "serial_debug.h"

#define SYSTEMTICK_PERIOD_MS  10

#if defined (STM32F40XX)
#define MESSAGE1   "    STM32F40/41x     "
#elif defined (STM32F427X)
#define MESSAGE1   "     STM32F427x      "
#endif
#define MESSAGE2   "  STM32F-4 Series   "
#define MESSAGE3   "   Webserver Demo   "
#define MESSAGE4   "                    "

__IO uint32_t LocalTime = 0; 
uint32_t timingdelay;

void LCD_LED_Init(void);

int main(void)
{
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

#ifdef SERIAL_DEBUG
  DebugComPort_Init();
#endif

  LCD_LED_Init();

  ETH_BSP_Config();
 
  LwIP_Init();
  
  httpd_init();

  while (1)
  {  
    if (ETH_CheckFrameReceived())
    { 
      LwIP_Pkt_Handle();
    }
    LwIP_Periodic_Handle(LocalTime);
  } 
}

void Delay(uint32_t nCount)
{
  timingdelay = LocalTime + nCount;  

  while(timingdelay > LocalTime)
  {     
  }
}

void Time_Update(void)
{
  LocalTime += SYSTEMTICK_PERIOD_MS;
}

void LCD_LED_Init(void)
{
#ifdef USE_LCD

  STM324xG_LCD_Init();
#endif

  STM_EVAL_LEDInit(LED1);
  STM_EVAL_LEDInit(LED2);
  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);
  
#ifdef USE_LCD
  LCD_Clear(Black);

  LCD_SetBackColor(Black);

  LCD_SetTextColor(White);

  LCD_DisplayStringLine(Line0, (uint8_t*)MESSAGE1);
  LCD_DisplayStringLine(Line1, (uint8_t*)MESSAGE2);
  LCD_DisplayStringLine(Line2, (uint8_t*)MESSAGE3);
  LCD_DisplayStringLine(Line3, (uint8_t*)MESSAGE4);  
#endif
}

#ifdef  USE_FULL_ASSERT


#endif
