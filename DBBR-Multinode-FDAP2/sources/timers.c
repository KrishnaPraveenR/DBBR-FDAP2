/***************************************************************************************************
* Name:         timers.c
* Author:       Nivis LLC, Ion Ticus
* Date:         Aug, 2008
* Description:  This file is provided ...
*               This file holds definitions of the ...
* Changes:
* Revisions:
****************************************************************************************************/
#include "timers.h"
#include "typedef.h"
#include "ISA100\mlsm.h"
#include "ISA100\phy.h"
#include "ISA100\tmr_util.h"

volatile unsigned char g_uc10msFlag;
volatile unsigned char g_uc250msFlag;

uint16 g_unTMRStartSlotOffset;
uint32 g_ulEmacTimestamp;

// channel 0 -  not used
// channel 1 - RA compar for time slot, RC compare (and reset) for 250 ms
//    TA1 is output from timer but is not selected to be output from peripferials (anyway, it is not connected)
//    I chose that implementation in order to have 250ms and time slot on sync all time 
// channel 2 - RC start delayed modem action


void TMR_Init(void)
{
    AT91C_BASE_PMC->PMC_PCER = 0
                              | (1L << AT91C_ID_TC1)
                              | (1L << AT91C_ID_TC2)
                              ; // enable peripherial

    
// channel 1 -  MLSM_OnTimeSlotStart, MLSM_On250ms
    AT91C_BASE_TC1->TC_CCR = AT91C_TC_CLKDIS; // (TC) Counter Clock Disable Command    
    AT91C_BASE_TC1->TC_IDR = 0xFFFFFFFF;
    
    (void)AT91C_BASE_TC1->TC_SR;
    
    AT91C_BASE_TC1->TC_CMR = 0 
                    | AT91C_TC_CLKS_TIMER_DIV5_CLOCK // MCK / 1024
                    | AT91C_TC_CPCTRG               // RC Compare resets the counter and starts the counter clock.
                    | AT91C_TC_WAVE                 // PWM to allows RB interupts 
                      ;
    AT91C_BASE_TC1->TC_CCR = AT91C_TC_CLKEN; // (TC) Counter Clock Enable Command    
    
    
    AT91C_BASE_TC1->TC_IER = AT91C_TC_CPAS | AT91C_TC_CPCS; //Enables the RB and RC Compare Interrupt.

    g_unDllTimeslotLength = 10485;
    g_unDllTMR2SlotLength = TMR0_TO_TMR2(FRACTION2_TO_TMR0( 10485 >> 5 )); // aligned to 32kHz clock
    
    g_unTMRStartSlotOffset = 0;
    
    AT91C_BASE_TC1->TC_RA = 0xFFFF; 
    AT91C_BASE_TC1->TC_RB = 0xFFFF;
    AT91C_BASE_TC1->TC_RC = TMR_CLK_250MS - 1; // every 250 ms
    AT91C_BASE_TC1->TC_CCR = AT91C_TC_SWTRG; // A software trigger is performed: the counter is reset and the clock is started.   

// channel 2 - PHY_OnTimeTriggeredAction
    AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKDIS; // (TC) Counter Clock Disable Command    
    AT91C_BASE_TC2->TC_IDR = 0xFFFFFFFF;
    
    (void)AT91C_BASE_TC2->TC_SR;
    
    AT91C_BASE_TC2->TC_CMR = 0 
                    | AT91C_TC_CLKS_TIMER_DIV5_CLOCK // MCK / 1024
                    | AT91C_TC_CPCTRG;               // RC Compare resets the counter and starts the counter clock.
    AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKEN; // (TC) Counter Clock Enable Command    
    
    
//    AT91C_BASE_TC2->TC_IER = AT91C_TC_CPCS; //Enables the RC Compare Interrupt.
      
// periodic interrupt -> every 10ms
    AT91C_BASE_PITC->PITC_PIMR =  AT91C_PITC_PITEN 
                                | AT91C_PITC_PITIEN
                                | (MCK/16/100 - 1) ;
    
    (void)AT91C_BASE_PITC->PITC_PIVR;
}


void PITC_Interrupt(void) // periodic interrupt
{    
    (void)AT91C_BASE_PITC->PITC_PIVR;
      
    g_uc10msFlag = 1;    
}


void TMR1_Interrupt(void)
{
    unsigned char ucTmrStatus = (unsigned char)AT91C_BASE_TC1->TC_SR;
    if( ucTmrStatus & AT91C_TC_CPCS ) // RC compair (and reset of counter) -> 250 ms interrupt
    {       
        //LED1_ON();        
        g_unTMRStartSlotOffset = 0;
        AT91C_BASE_TC1->TC_RA = g_unDllTMR2SlotLength;
        
        MLSM_On250ms();
        g_uc250msFlag = 1;
        
        //LED1_OFF();
    }
    else if( ucTmrStatus & AT91C_TC_CPAS ) // RA compair -> time slot interrupt
    {        
        if( g_stSlot.m_uc250msSlotNo < g_stSlot.m_ucsMax250mSlotNo - 1 )
        {
            g_stSlot.m_uc250msSlotNo ++;
            
            g_unTMRStartSlotOffset = TMR0_TO_TMR2(FRACTION_TO_TMR0( g_stSlot.m_uc250msSlotNo*(uint32)g_unDllTimeslotLength ) );
            AT91C_BASE_TC1->TC_RA = TMR0_TO_TMR2(FRACTION_TO_TMR0( (g_stSlot.m_uc250msSlotNo + 1)*(uint32)g_unDllTimeslotLength ) );        
            
            MLSM_OnTimeSlotStart(0);
        }
    }   
}

void TMR2_Interrupt(void)
{
    (void)AT91C_BASE_TC2->TC_SR; 
    
    PHY_OnTimeTriggeredAction();
}

void ExecAtXTics( uint8 p_ucRelativeFlag , uint16 p_nTics )
{
    if( p_nTics ) // valid delay
    {
        if ( p_ucRelativeFlag == ABSOLUTE_DELAY ) // absolute delay
        {
            p_nTics -= TMR_GetSlotOffset();
            if( p_nTics & 0x8000  ) // too late, try now 
            {
                p_nTics = 0;
            }
        }
    }
    
    if( !p_nTics ) // try now 
    {
       PHY_OnTimeTriggeredAction();
    }
    else
    {
        AT91C_BASE_TC2->TC_RC = p_nTics; 
        AT91C_BASE_AIC->AIC_ICCR = (1L << AT91C_ID_TC2); 
        AT91C_BASE_TC2->TC_IER = AT91C_TC_CPCS; //Enables the RC Compare Interrupt.      
        (void)AT91C_BASE_TC2->TC_SR;
        
        AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG; // A software trigger is performed: the counter is reset and the clock is started. 
    }
}


void TMR_SetCounter( uint16 p_unTmrCounter )
{
    // since cannot set counter, adjust end limit    
    uint16 unTmrRCValue = (TMR_CLK_250MS - 1 - p_unTmrCounter + AT91C_BASE_TC1->TC_CV);
    AT91C_BASE_TC1->TC_RC = unTmrRCValue;
    
    if( AT91C_BASE_TC1->TC_CV >= unTmrRCValue ) // clock overflow 
    {
        AT91C_BASE_TC1->TC_RC = TMR_CLK_250MS - 1 - p_unTmrCounter;
        AT91C_BASE_TC1->TC_CCR = AT91C_TC_SWTRG; // A software trigger is performed: the counter is reset and the clock is started.                
        (void)AT91C_BASE_TC1->TC_SR; // clear status register
    }
  
}

uint32 TMR_GetEmacElapsedTime(void)
{
    uint32 ulCrtTimestamp = TIMER_Get250msOffset();
    if( ulCrtTimestamp < g_ulEmacTimestamp )
    {
        ulCrtTimestamp += TMR_CLK_250MS;
    }
    
    return (ulCrtTimestamp - g_ulEmacTimestamp);
}

