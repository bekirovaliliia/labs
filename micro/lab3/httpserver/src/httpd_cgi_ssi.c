#include "lwip/debug.h"
#include "httpd.h"
#include "lwip/tcp.h"
#include "fs.h"
#include "main.h"

#include <string.h>
#include <stdlib.h>

tSSIHandler ADC_Page_SSI_Handler;
uint32_t ADC_not_configured=1;

char const* TAGCHAR="t";
char const** TAGS=&TAGCHAR;

const char * LEDS_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

const tCGI LEDS_CGI={"/leds.cgi", LEDS_CGI_Handler};

tCGI CGI_TAB[1];

static void ADC_Configuration(void)
{
  ADC_InitTypeDef ADC_InitStructure;
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOF, &GPIO_InitStructure);

  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div6;
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles; 
  ADC_CommonInit(&ADC_CommonInitStructure); 

  ADC_StructInit(&ADC_InitStructure);
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None; 
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion = 1;
  ADC_Init(ADC3, &ADC_InitStructure);

  ADC_RegularChannelConfig(ADC3, ADC_Channel_7, 1, ADC_SampleTime_56Cycles);

  ADC_Cmd(ADC3, ENABLE);

  ADC_SoftwareStartConv(ADC3);
}


u16_t ADC_Handler(int iIndex, char *pcInsert, int iInsertLen)
{
  if (iIndex ==0)
  {  
    char Digit1=0, Digit2=0, Digit3=0, Digit4=0; 
    uint32_t ADCVal = 0;        

     if (ADC_not_configured ==1)       
     {
        ADC_Configuration();
        ADC_not_configured=0;
     }
     
     ADCVal = ADC_GetConversionValue(ADC3);
     
     ADCVal = (uint32_t)(ADCVal * 0.8);  
          
     Digit1= ADCVal/1000;
     Digit2= (ADCVal-(Digit1*1000))/100 ;
     Digit3= (ADCVal-((Digit1*1000)+(Digit2*100)))/10;
     Digit4= ADCVal -((Digit1*1000)+(Digit2*100)+ (Digit3*10));
        
     *pcInsert       = (char)(Digit1+0x30);
     *(pcInsert + 1) = (char)(Digit2+0x30);
     *(pcInsert + 2) = (char)(Digit3+0x30);
     *(pcInsert + 3) = (char)(Digit4+0x30);

    return 4;
  }
  return 0;
}

const char * LEDS_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  uint32_t i=0;

  if (iIndex==0)
  {
    STM_EVAL_LEDOff(LED1);
    STM_EVAL_LEDOff(LED2);
    STM_EVAL_LEDOff(LED3);
    STM_EVAL_LEDOff(LED4);

    for (i=0; i<iNumParams; i++)
    {

      if (strcmp(pcParam[i] , "led")==0)   
      {

        if(strcmp(pcValue[i], "1") ==0) 
          STM_EVAL_LEDOn(LED1);

        else if(strcmp(pcValue[i], "2") ==0) 
          STM_EVAL_LEDOn(LED2);

        else if(strcmp(pcValue[i], "3") ==0) 
          STM_EVAL_LEDOn(LED3);

        else if(strcmp(pcValue[i], "4") ==0) 
          STM_EVAL_LEDOn(LED4);
      }
    }
  }

  return "/STM32F4x7LED.html";  
}

void httpd_ssi_init(void)
{  
  http_set_ssi_handler(ADC_Handler, (char const **)TAGS, 1);
}

void httpd_cgi_init(void)
{ 
  CGI_TAB[0] = LEDS_CGI;
  http_set_cgi_handlers(CGI_TAB, 1);
}