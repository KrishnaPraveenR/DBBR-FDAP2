/*************************************************************************
* File: main.c
* Author: Nivis LLC, Ion Ticus 
* MAIN
*************************************************************************/

#ifndef UT_ACTIVED  // unit testing support must be off

#include <string.h>
#include <stdio.h>

#include "global.h"
#include "digitals.h"
#include "timers.h"
#include "itc.h"
#include "eth.h"
#include "spi0.h"
#include "spi1.h"
#include "i2c.h"
#include "spi_eeprom.h"
#include "spi_pa_dac.h"

#include "wdt.h"
#include "asm.h"

#ifdef BBR1_HW
#include "CC2420/CC2420.h"
#endif

#ifdef BBR2_HW
#include "CC2520/CC2520.h"
#endif
#include "ISA100/tmr_util.h"
#include "ISA100/phy.h"
#include "ISA100/NLDE.h"
#include "ISA100/DMAP.h"
#include "ISA100/DLDE.h"
#include "ISA100/aslsrvc.h"
#include "ISA100/slme.h"
#include "ISA100/mlsm.h"
#include "ISA100/uap.h"

#include "CommonAPI/DAQ_Comm.h"
#include "ipv4/udp.h"
#include "usart.h"
#include "logger/logger.h"
#include "logger/logger_ack.h"

///////////////////////////////////////////////////////////////////////////////////
// Name: MAIN Function
///////////////////////////////////////////////////////////////////////////////////
int main()
{
//  GlobalDisableIRQ();
  WDT_INIT();
    
  Digitals_Init();  
    
  IRQ_Init();
  TMR_Init();
  
  I2C_Init();
  PROVISION_Init();  

  ETH_Init();
  UDP_Init();
  
  SPI0_Init();  
// SPI1_Init(); // commented because is on   CC2420_Init()
  
  SPI_InitEEPROM();
  SPI_InitDAC();

#ifdef BBR1_HW
  CC2420_Init();    
#endif   
  
#ifdef BBR2_HW
  CC2520_Init(CC2520_RADIO_1, 1);
#endif
  
  LOGQ_Init(); LOGAckQ_Init();
  WCI_InitQueue();

  USART0_Provision(); // USART0_Init() and USART0_disable() are called inside  
  
  LogShort(LOG_WARNING,LOG_M_MAP,MAPLOG_Reset,1,0);
  DMAP_DLMO_ResetStack(0);
  GlobalEnableIRQ(); // performed at end of DMAP_DLMO_ResetStack()

  LogShort( LOG_DEBUG, LOG_M_SYS, SYSOP_GENERAL, 0, 0 ); // start

  for (;;)
  {
      FEED_WDT();
      
      ETH_Task();
      
      if( g_uc10msFlag ) // 10ms Tasks
      {          
          g_uc10msFlag = 0;  
          
          ETH_Inc10msCounter();          
          
          DMAP_Task();
          ASLDE_ASLTask();
          
          if( g_uc250msFlag ) // 250ms Tasks
          {
              g_uc250msFlag = 0;
                  
              if( !g_stTAI.m_uc250msStep ) // first 250ms slot from each second -> 1sec Tasks
              {
                   ASLDE_PerformOneSecondOperations();
                   DMAP_DMO_CheckNewDevInfoTTL();
        
                   //need to be called before "DMAP_DMO_CheckPingTimeout" routine!!!!!!
                   DMAP_CheckSecondCounter();
        
//                   SLME_KeyUpdateTask();
                    
                   DMO_PerformOneSecondTasks();  
                                
//                   DMAP_LogStatus();
               } 
          }
      }
      
//        The Processor Clock PCK is enabled after a reset and is automatically re-enabled by any
//    enabled interrupt. The Processor Idle Mode is achieved by disabling the Processor Clock, which
//    is automatically re-enabled by any enabled fast or normal interrupt, or by the reset of the product.
      AT91C_BASE_PMC->PMC_SCDR = AT91C_PMC_PCK; // Disables the Processor clock. This is used to enter the processor in Idle Mode.
  }
}


#endif // #ifndef UT_ACTIVED -> unit testing support must be off
