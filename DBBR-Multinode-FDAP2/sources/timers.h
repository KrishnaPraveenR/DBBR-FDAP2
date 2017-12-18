/***************************************************************************************************
* Name:         timers.h
* Author:       Nivis LLC, Ion Ticus
* Date:         Aug, 2008
* Description:  This header file is provided ...
*               This file holds definitions of the ...
* Changes:
* Revisions:
****************************************************************************************************/
#ifndef _NIVIS_TIMERS_H_
#define _NIVIS_TIMERS_H_

#include "itc.h"
#include "typedef.h"

extern volatile unsigned char g_uc10msFlag;
extern volatile unsigned char g_uc250msFlag;


//#define MCK 18432000L
#define MCK (3*18432000L) // 55.296 MHz
#define MCK_DIV_FOR_10MHZ       (MCK/10000000L+1)

#define TIMER_Get250msOffset() AT91C_BASE_TC1->TC_CV 
#define TIMER_GetSlotOffset()  (AT91C_BASE_TC1->TC_CV - g_unTMRStartSlotOffset) 
#define TIMER_Set250msCorrection(nCorrection)  AT91C_BASE_TC1->TC_RC = (TMR_CLK_250MS - 1 - nCorrection)

#define TMR_SetEmacTimestamp() g_ulEmacTimestamp = TIMER_Get250msOffset()

void TMR_Init(void);
void TMR1_Interrupt(void);
void TMR2_Interrupt(void);
void PITC_Interrupt(void);
uint32 TMR_GetEmacElapsedTime(void);


void ExecAtXTics( uint8 p_ucRelativeFlag , uint16 p_nTics );
inline void TMR_DisablePendingActions( void ) 
{
    // disable any pending TMR2 action
    AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKDIS; // (TC) Counter Clock Disable Command    
    
    AT91C_BASE_TC2->TC_IDR = AT91C_TC_CPCS; 
  
    (void)AT91C_BASE_TC2->TC_SR; 
}

void TMR_SetCounter( uint16 p_unTmrCounter  );

extern uint16 g_unTMRStartSlotOffset;
extern uint32 g_ulEmacTimestamp;

#endif /* _NIVIS_TIMERS_H_ */

