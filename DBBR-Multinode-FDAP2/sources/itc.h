/***************************************************************************************************
* Name:         itc.h
* Author:       Nivis LLC,Ion Ticus
* Date:         Aug, 2008
* Description:  This header file is provided ...
*               This file holds definitions of the ...
* Changes:
* Revisions:
****************************************************************************************************/
#ifndef _NIVIS_ITC_H_
#define _NIVIS_ITC_H_

#include "typedef.h"
#include "global.h"

__arm void GlobalEnableFIQ(void);
__arm void GlobalDisableFIQ(void);
__arm void GlobalEnableIRQ(void);
__arm void GlobalDisableIRQ(void);

__arm unsigned long GlobalDisablePushIRQ(void);
__arm void GlobalRestorePopIRQ(unsigned long);

//• GMSK: General Mask
//0 = The nIRQ and nFIQ lines are normally controlled by the AIC.
//1 = The nIRQ and nFIQ lines are tied to their inactive state.
//#define  EnableAllIRQ()  { *AT91C_AIC_DCR = 0; }
//#define  DisableAllIRQ() { *AT91C_AIC_DCR = AT91C_AIC_DCR_GMSK; }

__arm void IRQ_Init(void);

#define DISABLE_DLL_TMRIRQ() AT91C_BASE_AIC->AIC_IDCR = (1L << AT91C_ID_TC1) | (1L << AT91C_ID_TC1) | (1L << AT91C_ID_PIOB) 
#define ENABLE_DLL_TMRIRQ()  AT91C_BASE_AIC->AIC_IECR = (1L << AT91C_ID_TC1) | (1L << AT91C_ID_TC1) | (1L << AT91C_ID_PIOB)

extern const unsigned int c_aIRQVectors[32];

#endif /* _NIVIS_ITC_H_ */

