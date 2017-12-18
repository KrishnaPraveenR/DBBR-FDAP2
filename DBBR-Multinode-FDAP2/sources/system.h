/////////////////////////////////////////////////////////////////////////
// File:         system.h
// Author:       NIVIS LLC, Ion Ticus
// Date:         March, 2008
// Description:  This file holds definitions of the general system settings (HW depended)
// Changes:
// Revisions:
/////////////////////////////////////////////////////////////////////////

#ifndef _NIVIS_SYSTEM_H_
#define _NIVIS_SYSTEM_H_


    #define DEV_TYPE_MC13225      1
    #define DEV_TYPE_MSP430F1611  2
    #define DEV_TYPE_MSP430F2618  3
    #define DEV_TYPE_AP91SAM7X512 4

#include "global.h"

#if( DEVICE_TYPE == DEV_TYPE_MC13225 )
     
    #include "itc.h"

    #define DISABLE_ALL_IRQ() IntDisableAll()
    #define ENABLE_ALL_IRQ()  IntEnableIRQ() // enable IRQ only, not also the FIQ because FIQ is not used


//    #define MONITOR_ENTER()  unsigned int ulIrqStatus = IntDisableAll()
//    #define MONITOR_EXIT()   IntRestoreAll(ulIrqStatus)

    #define MONITOR_ENTER()     unsigned int ulIrqStatus = IntDisableIRQ()
    #define MONITOR_RE_ENTER()  ulIrqStatus = IntDisableIRQ()
    #define MONITOR_EXIT()      IntRestoreIRQ(ulIrqStatus)

    #define  CPU_WORD_SIZE 4

    #define __swap_bytes(x) ((((uint16)(x)) >> 8) | (((uint16)(x)) << 8))

    #define LogShort(...) // no logging
    #define Log(...)      // no logging
    #define WCI_Log(...)      // no logging

#elif( DEVICE_TYPE == DEV_TYPE_AP91SAM7X512)
     
    #include "itc.h"

    #define DISABLE_ALL_IRQ() GlobalDisableIRQ()  // FIQ is not used 
    #define ENABLE_ALL_IRQ()  GlobalEnableIRQ()   // FIQ is not used 

    #define MONITOR_ENTER()  unsigned long ulIrqStatus = GlobalDisablePushIRQ()
    #define MONITOR_EXIT()   GlobalRestorePopIRQ(ulIrqStatus)


    #define  CPU_WORD_SIZE 4

    #define _NOP() __asm("   nop ")

#elif ( DEVICE_TYPE == DEV_TYPE_MSP430F1611 )

    #define DISABLE_ALL_IRQ()     __disable_interrupt()
    #define ENABLE_ALL_IRQ()      __enable_interrupt()
    #define DISABLE_DLL_TMRIRQ()  { TACCTL2 &= ~CCIE; TACCTL1 &= ~CCIE; TACCTL0 &= ~CCIE; }
    #define ENABLE_DLL_TMRIRQ()   { TACCTL0 |=  CCIE; TACCTL1 |=  CCIE; TACCTL2 |=  CCIE; }

    #define MONITOR_ENTER()  // use __monitor insteead 
    #define MONITOR_EXIT()   

    #define  CPU_WORD_SIZE 2

    #define LogShort(...) // no logging
    #define Log(...)      // no logging
    #define WCI_Log(...)      // no logging

#elif ( DEVICE_TYPE == DEV_TYPE_MSP430F2618 )

    #define DISABLE_ALL_IRQ()     __disable_interrupt()
    #define ENABLE_ALL_IRQ()      __enable_interrupt()
    #define DISABLE_DLL_TMRIRQ()  { TACCTL2 &= ~CCIE; TACCTL1 &= ~CCIE; TACCTL0 &= ~CCIE; }
    #define ENABLE_DLL_TMRIRQ()   { TACCTL0 |=  CCIE; TACCTL1 |=  CCIE; TACCTL2 |=  CCIE; }

    #define MONITOR_ENTER()  // use __monitor insteead 
    #define MONITOR_EXIT()  

    #define _NOP()  __no_operation()

    #define  CPU_WORD_SIZE 2

    #define LogShort(...) // no logging
    #define Log(...)      // no logging
    #define WCI_Log(...)      // no logging
#else 
    #error "Unsupported device type"

#endif // DEVICE_TYPE

#endif // _NIVIS_SYSTEM_H_ 


