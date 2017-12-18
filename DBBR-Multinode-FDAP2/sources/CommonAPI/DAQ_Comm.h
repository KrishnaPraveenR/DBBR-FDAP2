/*************************************************************************
 * File: DAQ_Comm.h
 * Author: John Chapman, Nivis LLC
 * Control functions for handling comms with the DAQ card.
 *************************************************************************/

#ifndef DAQ_COMM_H_
#define DAQ_COMM_H_

#include "RadioApi.h"
#include "Common_API.h"

extern uint16 g_unHeartBeatTick;
extern uint8 g_ucUapId;

#ifndef _UAP_TYPE
  #define _UAP_TYPE 0
#endif

#if ( _UAP_TYPE != 0 )

    extern const DMAP_FCT_STRUCT c_aIsa100ReadWriteTable[ISA100_CONTRACT_REQUEST - ISA100_ITEMS];

    void DAQ_Init(void);
    void DAQ_Handler(void);
    uint8 DAQ_UpdateHeartBeatFreq(enum HEARTBEAT_FREQ);

    #if ( _UAP_TYPE == UAP_TYPE_ISA100_API )    
        #define DAQ_IsReady() (g_ucUapId)
    #else
        #define DAQ_IsReady() 1
    #endif        
    
    #if ( SPI1_MODE != NONE )
        #define DAQ_NeedPowerOn() (SPI_IsBusy() || EXT_WAKEUP_IS_ON())
    #elif ( UART2_MODE != NONE )
        #define DAQ_NeedPowerOn() (UART2_IsBusy() || EXT_WAKEUP_IS_ON())    
    #endif    
#else
    
    #define DAQ_Init()
    #define DAQ_Handler(...)
    
    #define DAQ_NeedPowerOn()  0    
    #define DAQ_IsReady()      1    
#endif //


#endif //DAQ_COMM_H_
