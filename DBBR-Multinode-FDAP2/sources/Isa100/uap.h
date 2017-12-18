////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Eduard Erdei
/// Date:         May 2008
/// Description:  This file holds common definitions of different demo application processes
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NIVIS_UAP_H_
#define _NIVIS_UAP_H_

#include "dmap_dmo.h"

enum{
    UAP_MO_OBJ_TYPE         = 1,
    UAP_UDO_OBY_TYPE        = 3,
    UAP_CO_OBJ_TYPE         = 4,
    UAP_DISP_OBJ_TYPE       = 5,
    ANALOG_INPUT_OBJ_TYPE   = 99,
    ANALOG_OUTPUT_OBJ_TYPE  = 98,
    BINARY_INPUT_OBJ_TYPE   = 97,
    BINARY_OUTPUT_OBJ_TYPE  = 96,
    UAP_DATA_OBJ_TYPE       = 129
};

enum
{
  UAP_MO_OBJ_ID = 1,    //Application Process Management Object - specified by standard with object identifier = 1 for all UAPs 
  UAP_UDO_OBJ_ID = 2,   //specified by the standard(Table 515 - Standard object instances)
  UAP_CO_OBJ_ID = 4,    //not specified by standard - Concentrator Object - needed for publishing data 
  UAP_DISP_OBJ_ID = 5,  //not specified by standard - Dispersion Object - needed for local loop
  UAP_DATA_OBJ_ID = 129,//not specified by standard
  UAP_OBJ_NO = 3                
}; // UAP_OBJECT_IDS

#if ( _UAP_TYPE == 0 )

      #define UAP_NotifyAddContract(...)
      #define UAP_NotifyContractDeletion(...)
      #define UAP_DataConfirm(...)
      #define UAP_Init()
      #define UAP_OnJoin()
      #define UAP_OnJoinReset()
      #define UAP_MainTask()
      #define UAP_NeedPowerOn() 0

#else

#if ( _UAP_TYPE == UAP_TYPE_SIMPLE_API )
      extern uint16 g_unAttributesMap;
      extern uint32 g_ulDAQReadInterval;
      extern uint8  g_ucShtReadRequest;
      extern uint8  g_ucRequestForLedsON;
      
      #define UAP_NeedPowerOn() (DAQ_NeedPowerOn() || WAKEUP_STATUS_IS_PRESSED() || g_ucRequestForLedsON)
      #define UAP_SHT_CRM_SETTINGS() (g_ucShtReadRequest ? 0x08 : 0x00) // KYB3_LINE
#else      
      #define UAP_NeedPowerOn() DAQ_NeedPowerOn()
      #define UAP_SHT_CRM_SETTINGS() 0x00
#endif
    
      void UAP_NotifyContractResponse( const uint8 * p_pContractRsp, uint8 p_ucContractRspLen );
      void UAP_NotifyContractDeletion(uint16 p_unContractID);
      void UAP_DataConfirm( uint16 p_unAppHandle, uint8 p_ucStatus );
      void UAP_Init(void);
      void UAP_OnJoin(void);
      void UAP_OnJoinReset(void);
      void UAP_MainTask(void);

#endif // _UAP_TYPE == 0

#endif // _NIVIS_UAP_H_
