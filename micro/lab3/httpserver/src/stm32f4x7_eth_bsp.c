#include "lwip/opt.h"
#include "stm32f4x7_eth.h"
#include "stm32f4x7_eth_bsp.h"
#include "main.h"
#include "netif.h"
#include "netconf.h"
#include "lwip/dhcp.h"

ETH_InitTypeDef ETH_InitStructure;
__IO uint32_t  EthStatus = 0;
extern struct netif gnetif;
#ifdef USE_DHCP
extern __IO uint8_t DHCP_state;
#endif

static void ETH_GPIO_Config(void);
static void ETH_MACDMA_Config(void);

void ETH_BSP_Config(void)
{
  RCC_ClocksTypeDef RCC_Clocks;
  
  SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);

  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);

  NVIC_SetPriority (SysTick_IRQn, 0);

  ETH_GPIO_Config();

  ETH_MACDMA_Config();

  if(ETH_ReadPHYRegister(DP83848_PHY_ADDRESS, PHY_SR) & 1)
  {
    EthStatus |= ETH_LINK_FLAG;
  }

  Eth_Link_PHYITConfig(DP83848_PHY_ADDRESS);

  Eth_Link_EXTIConfig(); 
}

static void ETH_MACDMA_Config(void)
{
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_ETH_MAC | RCC_AHB1Periph_ETH_MAC_Tx |
                        RCC_AHB1Periph_ETH_MAC_Rx, ENABLE);

  ETH_DeInit();

  ETH_SoftwareReset();

  while (ETH_GetSoftwareResetStatus() == SET);

  ETH_StructInit(&ETH_InitStructure);

  ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;

  ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Disable;
  ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Disable;
  ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable;
  ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Disable;
  ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable;
  ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;
  ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;
  ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect;
#ifdef CHECKSUM_BY_HARDWARE
  ETH_InitStructure.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable;
#endif

  ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable; 
  ETH_InitStructure.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;
  ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;

  ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Disable;
  ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Disable;
  ETH_InitStructure.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Enable;
  ETH_InitStructure.ETH_AddressAlignedBeats = ETH_AddressAlignedBeats_Enable;
  ETH_InitStructure.ETH_FixedBurst = ETH_FixedBurst_Enable;
  ETH_InitStructure.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;
  ETH_InitStructure.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;
  ETH_InitStructure.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_2_1;

  EthStatus = ETH_Init(&ETH_InitStructure, DP83848_PHY_ADDRESS);
}

void ETH_GPIO_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB |
                         RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOI |
                         RCC_AHB1Periph_GPIOG | RCC_AHB1Periph_GPIOH |
                         RCC_AHB1Periph_GPIOF, ENABLE);


  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;  
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
#ifdef MII_MODE 
 #ifdef PHY_CLOCK_MCO

  RCC_MCO1Config(RCC_MCO1Source_HSE, RCC_MCO1Div_1);
 #endif 

  SYSCFG_ETH_MediaInterfaceConfig(SYSCFG_ETH_MediaInterface_MII);
#elif defined RMII_MODE  

  SYSCFG_ETH_MediaInterfaceConfig(SYSCFG_ETH_MediaInterface_RMII);
#endif

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_7;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_ETH);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_ETH);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_ETH);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_8;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_ETH);	
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_ETH);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource1, GPIO_AF_ETH);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource2, GPIO_AF_ETH);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_ETH);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource4, GPIO_AF_ETH);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource5, GPIO_AF_ETH);
                                
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_11 | GPIO_Pin_13 | GPIO_Pin_14;
  GPIO_Init(GPIOG, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource11, GPIO_AF_ETH);
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource13, GPIO_AF_ETH);
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource14, GPIO_AF_ETH);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_Init(GPIOH, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOH, GPIO_PinSource2, GPIO_AF_ETH);
  GPIO_PinAFConfig(GPIOH, GPIO_PinSource3, GPIO_AF_ETH);
  GPIO_PinAFConfig(GPIOH, GPIO_PinSource6, GPIO_AF_ETH);
  GPIO_PinAFConfig(GPIOH, GPIO_PinSource7, GPIO_AF_ETH);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_Init(GPIOI, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOI, GPIO_PinSource10, GPIO_AF_ETH);
}

uint32_t Eth_Link_PHYITConfig(uint16_t PHYAddress)
{
  uint16_t tmpreg = 0;

  tmpreg = ETH_ReadPHYRegister(PHYAddress, PHY_MICR);

  tmpreg |= (uint16_t)(PHY_MICR_INT_EN | PHY_MICR_INT_OE);
  if(!(ETH_WritePHYRegister(PHYAddress, PHY_MICR, tmpreg)))
  {
    return ETH_ERROR;
  }

  tmpreg = ETH_ReadPHYRegister(PHYAddress, PHY_MISR);

  tmpreg |= (uint16_t)PHY_MISR_LINK_INT_EN;
  if(!(ETH_WritePHYRegister(PHYAddress, PHY_MISR, tmpreg)))
  {
    return ETH_ERROR;
  }
  return ETH_SUCCESS;   
}

void Eth_Link_EXTIConfig(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  RCC_AHB1PeriphClockCmd(ETH_LINK_GPIO_CLK, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Pin = ETH_LINK_PIN;
  GPIO_Init(ETH_LINK_GPIO_PORT, &GPIO_InitStructure);

  SYSCFG_EXTILineConfig(ETH_LINK_EXTI_PORT_SOURCE, ETH_LINK_EXTI_PIN_SOURCE);

  EXTI_InitStructure.EXTI_Line = ETH_LINK_EXTI_LINE;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}


void Eth_Link_ITHandler(uint16_t PHYAddress)
{
  if(((ETH_ReadPHYRegister(PHYAddress, PHY_MISR)) & PHY_LINK_STATUS) != 0)
  {
    if((ETH_ReadPHYRegister(PHYAddress, PHY_SR) & 1))
    {
      netif_set_link_up(&gnetif);
    }
    else
    {
      netif_set_link_down(&gnetif);
    }
  }
}

void ETH_link_callback(struct netif *netif)
{
  __IO uint32_t timeout = 0;
 uint32_t tmpreg;
 uint16_t RegValue;
  struct ip_addr ipaddr;
  struct ip_addr netmask;
  struct ip_addr gw;
#ifndef USE_DHCP
  uint8_t iptab[4] = {0};
  uint8_t iptxt[20];
#endif 
  LCD_ClearLine(Line4);
  LCD_ClearLine(Line5);
  LCD_ClearLine(Line6);
  LCD_ClearLine(Line7);
  LCD_ClearLine(Line8);
  LCD_ClearLine(Line9);

  if(netif_is_link_up(netif))
  {
    if(ETH_InitStructure.ETH_AutoNegotiation != ETH_AutoNegotiation_Disable)
    {
      timeout = 0;

      ETH_WritePHYRegister(DP83848_PHY_ADDRESS, PHY_BCR, PHY_AutoNegotiation);

      do
      {
        timeout++;
      } while (!(ETH_ReadPHYRegister(DP83848_PHY_ADDRESS, PHY_BSR) & PHY_AutoNego_Complete) && (timeout < (uint32_t)PHY_READ_TO));

      timeout = 0;

      RegValue = ETH_ReadPHYRegister(DP83848_PHY_ADDRESS, PHY_SR);

      if((RegValue & PHY_DUPLEX_STATUS) != (uint16_t)RESET)
      {
        ETH_InitStructure.ETH_Mode = ETH_Mode_FullDuplex;  
      }
      else
      {
        ETH_InitStructure.ETH_Mode = ETH_Mode_HalfDuplex;
      }
      if(RegValue & PHY_SPEED_STATUS)
      {
        ETH_InitStructure.ETH_Speed = ETH_Speed_10M; 
      }
      else
      {
        ETH_InitStructure.ETH_Speed = ETH_Speed_100M;
      }

      tmpreg = ETH->MACCR;

      tmpreg |= (uint32_t)(ETH_InitStructure.ETH_Speed | ETH_InitStructure.ETH_Mode);

      ETH->MACCR = (uint32_t)tmpreg;

      _eth_delay_(ETH_REG_WRITE_DELAY);
      tmpreg = ETH->MACCR;
      ETH->MACCR = tmpreg;
    }

    ETH_Start();

#ifdef USE_DHCP
    ipaddr.addr = 0;
    netmask.addr = 0;
    gw.addr = 0;
    DHCP_state = DHCP_START;
#else
    IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
    IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
    IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
#endif 

    netif_set_addr(&gnetif, &ipaddr , &netmask, &gw);

    netif_set_up(&gnetif);    

#ifdef USE_LCD
    LCD_SetTextColor(Green);

    LCD_DisplayStringLine(Line5, (uint8_t*)"  Network Cable is  ");
    LCD_DisplayStringLine(Line6, (uint8_t*)"    now connected   ");

    LCD_SetTextColor(White);

  #ifndef USE_DHCP

    iptab[0] = IP_ADDR3;
    iptab[1] = IP_ADDR2;
    iptab[2] = IP_ADDR1;
    iptab[3] = IP_ADDR0;
    sprintf((char*)iptxt, "  %d.%d.%d.%d", iptab[3], iptab[2], iptab[1], iptab[0]); 
    LCD_DisplayStringLine(Line8, (uint8_t*)"  Static IP address   ");
    LCD_DisplayStringLine(Line9, iptxt);

    LCD_ClearLine(Line5);
    LCD_ClearLine(Line6);
  #endif 
#endif 
  }
  else
  {
    ETH_Stop();
#ifdef USE_DHCP
    DHCP_state = DHCP_LINK_DOWN;
    dhcp_stop(netif);
#endif 

    netif_set_down(&gnetif);
#ifdef USE_LCD

    LCD_SetTextColor(Red);

    LCD_DisplayStringLine(Line5, (uint8_t*)"  Network Cable is  ");
    LCD_DisplayStringLine(Line6, (uint8_t*)"     unplugged   ");

    LCD_SetTextColor(White);
#endif
  }
}