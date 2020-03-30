#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/tcp.h"
#include "lwip/tcp_impl.h"
#include "lwip/udp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "ethernetif.h"
#include "main.h"
#include "netconf.h"
#include <stdio.h>

#define MAX_DHCP_TRIES        4

struct netif gnetif;
uint32_t TCPTimer = 0;
uint32_t ARPTimer = 0;
uint32_t IPaddress = 0;

#ifdef USE_DHCP
uint32_t DHCPfineTimer = 0;
uint32_t DHCPcoarseTimer = 0;
__IO uint8_t DHCP_state;
#endif
extern __IO uint32_t  EthStatus;

void LwIP_DHCP_Process_Handle(void);

void LwIP_Init(void)
{
  struct ip_addr ipaddr;
  struct ip_addr netmask;
  struct ip_addr gw;
#ifndef USE_DHCP
  uint8_t iptab[4] = {0};
  uint8_t iptxt[20];
#endif

  mem_init();
  
  memp_init();
  
#ifdef USE_DHCP
  ipaddr.addr = 0;
  netmask.addr = 0;
  gw.addr = 0;
#else
  IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
  IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
  IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
#endif  
  

  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &ethernet_input);

  netif_set_default(&gnetif);

  if (EthStatus == (ETH_INIT_FLAG | ETH_LINK_FLAG))
  {
    gnetif.flags |= NETIF_FLAG_LINK_UP;

    netif_set_up(&gnetif);

#ifdef USE_DHCP
    DHCP_state = DHCP_START;
#else
#ifdef USE_LCD

    iptab[0] = IP_ADDR3;
    iptab[1] = IP_ADDR2;
    iptab[2] = IP_ADDR1;
    iptab[3] = IP_ADDR0;

    sprintf((char*)iptxt, "  %d.%d.%d.%d", iptab[3], iptab[2], iptab[1], iptab[0]); 

    LCD_DisplayStringLine(Line8, (uint8_t*)"  Static IP address   ");
    LCD_DisplayStringLine(Line9, iptxt);
#endif
#endif 
  }
  else
  {
    netif_set_down(&gnetif);

#ifdef USE_DHCP
    DHCP_state = DHCP_LINK_DOWN;
#endif 
    LCD_SetTextColor(Red);

    LCD_DisplayStringLine(Line5, (uint8_t*)"  Network Cable is  ");
    LCD_DisplayStringLine(Line6, (uint8_t*)"    not connected   ");

    LCD_SetTextColor(White);
  }

  netif_set_link_callback(&gnetif, ETH_link_callback);
}

void LwIP_Pkt_Handle(void)
{
  ethernetif_input(&gnetif);
}


void LwIP_Periodic_Handle(__IO uint32_t localtime)
{
#if LWIP_TCP
  if (localtime - TCPTimer >= TCP_TMR_INTERVAL)
  {
    TCPTimer =  localtime;
    tcp_tmr();
  }
#endif
  
  if ((localtime - ARPTimer) >= ARP_TMR_INTERVAL)
  {
    ARPTimer =  localtime;
    etharp_tmr();
  }
  
#ifdef USE_DHCP
  if (localtime - DHCPfineTimer >= DHCP_FINE_TIMER_MSECS)
  {
    DHCPfineTimer =  localtime;
    dhcp_fine_tmr();
    if ((DHCP_state != DHCP_ADDRESS_ASSIGNED) && 
        (DHCP_state != DHCP_TIMEOUT) &&
          (DHCP_state != DHCP_LINK_DOWN))
    {
      STM_EVAL_LEDToggle(LED1);

      LwIP_DHCP_Process_Handle();    
    }
  }

  if (localtime - DHCPcoarseTimer >= DHCP_COARSE_TIMER_MSECS)
  {
    DHCPcoarseTimer =  localtime;
    dhcp_coarse_tmr();
  }

#endif
}

#ifdef USE_DHCP

void LwIP_DHCP_Process_Handle()
{
  struct ip_addr ipaddr;
  struct ip_addr netmask;
  struct ip_addr gw;
  uint8_t iptab[4] = {0};
  uint8_t iptxt[20];
  
  switch (DHCP_state)
  {
  case DHCP_START:
    {
      DHCP_state = DHCP_WAIT_ADDRESS;
      dhcp_start(&gnetif);
      IPaddress = 0;
#ifdef USE_LCD
      LCD_DisplayStringLine(Line4, (uint8_t*)"     Looking for    ");
      LCD_DisplayStringLine(Line5, (uint8_t*)"     DHCP server    ");
      LCD_DisplayStringLine(Line6, (uint8_t*)"     please wait... ");
#endif
    }
    break;

  case DHCP_WAIT_ADDRESS:
    {
      IPaddress = gnetif.ip_addr.addr;
      
      if (IPaddress!=0) 
      {
        DHCP_state = DHCP_ADDRESS_ASSIGNED;	
        
        dhcp_stop(&gnetif);

#ifdef USE_LCD      
        iptab[0] = (uint8_t)(IPaddress >> 24);
        iptab[1] = (uint8_t)(IPaddress >> 16);
        iptab[2] = (uint8_t)(IPaddress >> 8);
        iptab[3] = (uint8_t)(IPaddress);

        sprintf((char*)iptxt, " %d.%d.%d.%d", iptab[3], iptab[2], iptab[1], iptab[0]);       

        LCD_ClearLine(Line4);
        LCD_ClearLine(Line5);
        LCD_ClearLine(Line6);

        LCD_DisplayStringLine(Line7, (uint8_t*)"IP address assigned ");
        LCD_DisplayStringLine(Line8, (uint8_t*)"  by a DHCP server  ");
        LCD_DisplayStringLine(Line9, iptxt);
#endif
        STM_EVAL_LEDOn(LED1);
      }
      else
      {
        if (gnetif.dhcp->tries > MAX_DHCP_TRIES)
        {
          DHCP_state = DHCP_TIMEOUT;

          dhcp_stop(&gnetif);

          IP4_ADDR(&ipaddr, IP_ADDR0 ,IP_ADDR1 , IP_ADDR2 , IP_ADDR3 );
          IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
          IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
          netif_set_addr(&gnetif, &ipaddr , &netmask, &gw);

#ifdef USE_LCD   
          LCD_DisplayStringLine(Line7, (uint8_t*)"    DHCP timeout    ");

          iptab[0] = IP_ADDR3;
          iptab[1] = IP_ADDR2;
          iptab[2] = IP_ADDR1;
          iptab[3] = IP_ADDR0;

          sprintf((char*)iptxt, "  %d.%d.%d.%d", iptab[3], iptab[2], iptab[1], iptab[0]);

          LCD_ClearLine(Line4);
          LCD_ClearLine(Line5);
          LCD_ClearLine(Line6);

          LCD_DisplayStringLine(Line8, (uint8_t*)"  Static IP address   ");
          LCD_DisplayStringLine(Line9, iptxt);         
#endif
          STM_EVAL_LEDOn(LED1);
        }
      }
    }
    break;
  default: break;
  }
}
#endif
