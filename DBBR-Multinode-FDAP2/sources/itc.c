/***************************************************************************************************
* Name:         itc.c
* Author:       Nivis LLC, Ion Ticus
* Date:         Aug, 2008
* Description:  This file is provided implmentation of Interrupt controller
* Changes:
* Revisions:
****************************************************************************************************/
#include "itc.h"
#include "timers.h"
#include "eth.h"
#include "isa100/phy.h"


void SYS_Interrupt( void );
/*
const unsigned int c_aIRQVectors[32] = 
{
  (unsigned int)idleIrq       , //  0         AIC Advanced Interrupt Controller     FIQ
  (unsigned int)SYS_Interrupt, //  1         SYSC(1) System Controller
  (unsigned int)idleIrq       , //  2         PIOA Parallel I/O Controller A
  (unsigned int)RF_Interrupt  , //  3         PIOB Parallel I/O Controller B
  (unsigned int)idleIrq       , //  4         SPI0 Serial Peripheral Interface 0
  (unsigned int)idleIrq       , //  5         SPI1 Serial Peripheral Interface 1
  (unsigned int)idleIrq       , //  6         US0 USART 0
  (unsigned int)idleIrq       , //  7         US1 USART 1
  (unsigned int)idleIrq       , //  8         SSC Synchronous Serial Controller
  (unsigned int)idleIrq       , //  9         TWI Two-wire Interface
  (unsigned int)idleIrq       , //  10        PWMC Pulse Width Modulation Controller
  (unsigned int)idleIrq       , //  11        UDP USB Device Port
  (unsigned int)idleIrq       , //  12        TC0 Timer/Counter 0  
  (unsigned int)TMR1_Interrupt, //  13        TC1 Timer/Counter 1
  (unsigned int)TMR2_Interrupt, //  14        TC2 Timer/Counter 2
  (unsigned int)idleIrq       , //  15        CAN CAN Controller
  (unsigned int)ETH_Interrupt,  //  16        EMAC Ethernet MAC
  (unsigned int)idleIrq       , //  17        ADC(1) Analog-to Digital Converter
  (unsigned int)idleIrq       , //  18        Reserved
  (unsigned int)idleIrq       , //  19        Reserved
  (unsigned int)idleIrq       , //  20        Reserved
  (unsigned int)idleIrq       , //  21        Reserved
  (unsigned int)idleIrq       , //  22        Reserved
  (unsigned int)idleIrq       , //  23        Reserved
  (unsigned int)idleIrq       , //  24        Reserved
  (unsigned int)idleIrq       , //  25        Reserved
  (unsigned int)idleIrq       , //  26        Reserved
  (unsigned int)idleIrq       , //  27        Reserved
  (unsigned int)idleIrq       , //  28        Reserved
  (unsigned int)idleIrq       , //  29        Reserved
  (unsigned int)idleIrq       , //  30        AIC Advanced Interrupt Controller       IRQ0
  (unsigned int)idleIrq         //  31        AIC Advanced Interrupt Controller       IRQ1
};
*/

// if decomment that function be sure you have space on FIQ interrupt stack
__arm void GlobalEnableFIQ(void)
{
  __asm("   mrs r0,CPSR \n "
        "   bic r0,r0,#0x40 \n "
        "   msr CPSR_c,r0 ");
}

__arm void GlobalDisableFIQ(void)
{
  __asm("   mrs r0,CPSR \n "
        "   orr r0,r0,#0x40 \n "
        "   msr CPSR_c,r0 ");
}

__arm void GlobalEnableIRQ(void)
{
   __asm("  mrs r0,CPSR \n "
         "  bic r0,r0,#0x80 \n "
         "  msr CPSR_c,r0 ");
}

__arm void GlobalDisableIRQ(void)
{
  __asm("   mrs r0,CPSR \n "
        "   orr r0,r0,#0x80 \n "
        "   msr CPSR_c,r0 ");
}


__arm void IRQ_Init(void)
{
#ifdef BBR1_HW
  GlobalDisableIRQ();
  GlobalDisableFIQ();
    
  AT91C_BASE_AIC->AIC_FFER = (1L << AT91C_ID_EMAC); // force EMAC as fast interrupt
  
  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_FIQ ] = (unsigned int)ETH_Interrupt; // force EMAC as fast interrupt
  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_SYS  ] = (unsigned int)SYS_Interrupt;
  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_PIOB ] = (unsigned int)RF_Interrupt;
  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_TC1 ]  = (unsigned int)TMR1_Interrupt;
  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_TC2 ]  = (unsigned int)TMR2_Interrupt;
//  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_EMAC ] = (unsigned int)ETH_Interrupt;
    
  // IRQ and FIQ exceptions = all interrupts Normal and Fast
  // for IRQ and FIQ exceptions reset and initialise the state of itc
  AT91C_BASE_AIC->AIC_IDCR = 0xFFFFFFFF; // disable all interrupts in interrupt controller
  AT91C_BASE_AIC->AIC_ICCR = 0xFFFFFFFF; // clear all interrupts in interrupt controller


  AT91C_BASE_AIC->AIC_IECR = 0
                      | (1L << AT91C_ID_SYS) 
                      | (1L << AT91C_ID_TC1) 
                      | (1L << AT91C_ID_TC2) 
                      | (1L << AT91C_ID_EMAC) 
                      | (1L << AT91C_ID_PIOB) 
                      ;
  
  GlobalEnableFIQ();
  
#endif
#ifdef BBR2_HW
    GlobalDisableIRQ();
  GlobalDisableFIQ();
  
//  AT91C_BASE_AIC->AIC_FFER = (1L << AT91C_ID_EMAC); // force EMAC as fast interrupt
  
//  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_FIQ ] = (unsigned int)ETH_Interrupt; // force EMAC as fast interrupt
  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_SYS  ] = (unsigned int)SYS_Interrupt;
  AT91C_BASE_AIC->AIC_SMR[ AT91C_ID_SYS ]  = (AT91C_AIC_PRIOR & 4);
  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_PIOB ] = (unsigned int)RF_Interrupt;
  AT91C_BASE_AIC->AIC_SMR[ AT91C_ID_PIOB ]  = (AT91C_AIC_PRIOR & 6);
  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_TC1 ]  = (unsigned int)TMR1_Interrupt;
  AT91C_BASE_AIC->AIC_SMR[ AT91C_ID_TC1 ]  = (AT91C_AIC_PRIOR & 7);
  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_TC2 ]  = (unsigned int)TMR2_Interrupt;
  AT91C_BASE_AIC->AIC_SMR[ AT91C_ID_TC2 ]  = (AT91C_AIC_PRIOR & 6);
  AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_EMAC ] = (unsigned int)ETH_Interrupt;
  AT91C_BASE_AIC->AIC_SMR[ AT91C_ID_EMAC ]  = (AT91C_AIC_PRIOR & 5);
    
  // IRQ and FIQ exceptions = all interrupts Normal and Fast
  // for IRQ and FIQ exceptions reset and initialise the state of itc
  AT91C_BASE_AIC->AIC_IDCR = 0xFFFFFFFF; // disable all interrupts in interrupt controller
  AT91C_BASE_AIC->AIC_ICCR = 0xFFFFFFFF; // clear all interrupts in interrupt controller


  AT91C_BASE_AIC->AIC_IECR = 0
                      | (1L << AT91C_ID_SYS) 
                      | (1L << AT91C_ID_TC1) 
                      | (1L << AT91C_ID_TC2) 
                      | (1L << AT91C_ID_EMAC) 
                      | (1L << AT91C_ID_PIOB) 
                      ;
  
//  GlobalEnableFIQ();
#endif   
}

void SYS_Interrupt( void )
{
    if( AT91C_BASE_PITC->PITC_PISR & AT91C_PITC_PITS )
    {
        PITC_Interrupt();
    }
}



#pragma diag_suppress=Pe940
#pragma optimize=no_inline
__arm unsigned long Get_CPSR( void )
{
    /* On function exit, function return value should be present in R0 */
    __asm("   mrs r0,CPSR \n ");
}
#pragma diag_default=Pe940

__arm unsigned long GlobalDisablePushIRQ(void)
{   
    unsigned long ulIntState = Get_CPSR();
    GlobalDisableIRQ();
    return ulIntState;
}

__arm void GlobalRestorePopIRQ(unsigned long ucPrevState)
{
    if(!(ucPrevState&0x80)) GlobalEnableIRQ();
}

