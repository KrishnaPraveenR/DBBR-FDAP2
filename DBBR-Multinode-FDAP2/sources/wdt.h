/***************************************************************************************************
* Name:         wdt.h
* Author:       Nivis LLC, Ion Ticus
* Date:         Aug 2008
* Description:  This header file is provided ...
*               This file holds definitions of the ...
* Changes:
* Revisions:
****************************************************************************************************/
#ifndef _NIVIS_WDT_H_
#define _NIVIS_WDT_H_

#define WDT_2_SEC_TIMEOUT       0x1FF  // 0xFFF means 16 sec

#define FEED_WDT()              *AT91C_WDTC_WDCR = 0xA5000001
#define SET_WDT_TIMEOUT()       *AT91C_WDTC_WDMR = WDT_2_SEC_TIMEOUT \
                                                    | AT91C_WDTC_WDDBGHLT \
                                                    | AT91C_WDTC_WDRSTEN \
                                                    | AT91C_WDTC_WDRPROC \
                                                    | ((unsigned long)WDT_2_SEC_TIMEOUT << 16) // WDD

#define WDT_INIT() { SET_WDT_TIMEOUT(); FEED_WDT(); }


enum
{
  CK_LOAD_MODEM_IRQ = 0,
  CK_LOAD_TIMER_IRQ,
  CK_LOAD_TASKS,
  
  CK_LOAD_STRUCT_NO
};

// -----------------------------  CHECK LOAD SUPPORT ---------------------------
#ifdef CK_LOAD_SUPPORT
  #include "timers.h"

  typedef struct 
  {
      uint16 m_unCrtCounter;
      uint16 m_unLastLoad;
      uint16 m_unMaxLoad;
      uint16 m_unAvgCounter;
      uint32 m_ulAvgAcc;      
  } CK_LOAD_STRUCT;
  
  extern CK_LOAD_STRUCT g_aCkLoadTable[CK_LOAD_STRUCT_NO];
  
  #define CK_LOAD_Start(idx) g_aCkLoadTable[idx].m_unCrtCounter = CK_LOAD_TMR_COUNTER
  void CK_LOAD_Stop(uint8 p_unIdx);   
  
#else
  
  #define CK_LOAD_Start(idx) 
  #define CK_LOAD_Stop(idx)   
  
#endif // CK_LOAD_SUPPORT


#define  _SLEEP_TEST_

#define BEGIN_MAIN (void *)0x00400080         /* appl´s start-up code address */

#define JSR(x) (*((void(*)(void))x))()
#define JSR_MAIN() JSR(BEGIN_MAIN)

extern uint8  gWakedUp;   /* this is a flag for wake-up from DOZE */
extern uint32 gRegister;  /* this variable is used to update the system clock */

void do_sleep (void);
void do_wakeup (void);





#endif /* _NIVIS_CRM_H_ */

