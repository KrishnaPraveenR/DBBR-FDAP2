////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Mihaela Goloman
/// Date:         November 2008
/// Description:  This file implements the alert report management object in DMAP
/// Changes:      Created
/// Revisions:    1.0
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include "dmap_armo.h"
#include "dlmo_utils.h"
#include "dmap.h"
#include "aslsrvc.h"
#include "mlsm.h"
#include "tlde.h"

#define ARMO_DEFAULT_CONF_TIMEOUT  10
#define ARMO_MIN_CONF_TIMEOUT       5

extern uint8 * ASLSRVC_insertExtensibleValue(  uint8 * p_pData,
                                        uint16  p_unValue);
extern const uint8 * ASLSRVC_extractExtensibleValue( const uint8 * p_pData,
                                              uint16 * p_pValue);

ARMO_STRUCT g_aARMO[ALERT_CAT_NO];

uint16 g_unContractReqTimestamp;



#pragma data_alignment=4 
uint8  g_aucARMOQueue[ARMO_QUEUE_SIZE];  

uint8* g_pARMOQueueEnd = g_aucARMOQueue;

static void ARMO_readAlertCommEndpoint(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize);
static void ARMO_writeAlertCommEndpoint(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);

static uint8 ARMO_configureEndpoint(ALERT_COMM_ENDPOINT * p_pEndpoint, uint8 p_ucCategory );

static uint8 ARMO_recoverNextAlarm(uint8 p_ucCategory, uint8 p_ucLastID);
static uint8 ARMO_generateRecoveryEvent(uint8 p_ucCategory, uint8 p_ucAlertType);

const DMAP_FCT_STRUCT c_aARMOFct[ARMO_ATTR_NO] = {
   0,   0,                                                      DMAP_EmptyReadFunc,         NULL,            
  
   ATTR_CONST(g_aARMO[ALERT_CAT_DEV_DIAG].m_stAlertMaster),     ARMO_readAlertCommEndpoint, ARMO_writeAlertCommEndpoint, 
   ATTR_CONST(g_aARMO[ALERT_CAT_DEV_DIAG].m_ucConfTimeout),     DMAP_ReadUint8,             DMAP_WriteUint8, 
   ATTR_CONST(g_aARMO[ALERT_CAT_DEV_DIAG].m_ucAlertsDisable),   DMAP_ReadUint8,             DMAP_WriteUint8, 
      
   ATTR_CONST(g_aARMO[ALERT_CAT_COMM_DIAG].m_stAlertMaster),    ARMO_readAlertCommEndpoint, ARMO_writeAlertCommEndpoint, 
   ATTR_CONST(g_aARMO[ALERT_CAT_COMM_DIAG].m_ucConfTimeout),    DMAP_ReadUint8,             DMAP_WriteUint8, 
   ATTR_CONST(g_aARMO[ALERT_CAT_COMM_DIAG].m_ucAlertsDisable),  DMAP_ReadUint8,             DMAP_WriteUint8, 
   
   ATTR_CONST(g_aARMO[ALERT_CAT_SECURITY].m_stAlertMaster),     ARMO_readAlertCommEndpoint, ARMO_writeAlertCommEndpoint, 
   ATTR_CONST(g_aARMO[ALERT_CAT_SECURITY].m_ucConfTimeout),     DMAP_ReadUint8,             DMAP_WriteUint8, 
   ATTR_CONST(g_aARMO[ALERT_CAT_SECURITY].m_ucAlertsDisable),   DMAP_ReadUint8,             DMAP_WriteUint8, 
   
   ATTR_CONST(g_aARMO[ALERT_CAT_PROCESS].m_stAlertMaster),      ARMO_readAlertCommEndpoint, ARMO_writeAlertCommEndpoint, 
   ATTR_CONST(g_aARMO[ALERT_CAT_PROCESS].m_ucConfTimeout),      DMAP_ReadUint8,             DMAP_WriteUint8, 
   ATTR_CONST(g_aARMO[ALERT_CAT_PROCESS].m_ucAlertsDisable),    DMAP_ReadUint8,             DMAP_WriteUint8, 
   
   
   ATTR_CONST(g_aARMO[ALERT_CAT_COMM_DIAG].m_stRecoveryDescr),  DMAP_ReadAlertDescriptor,  DMAP_WriteAlertDescriptor, 
   ATTR_CONST(g_aARMO[ALERT_CAT_SECURITY].m_stRecoveryDescr),   DMAP_ReadAlertDescriptor,  DMAP_WriteAlertDescriptor, 
   ATTR_CONST(g_aARMO[ALERT_CAT_DEV_DIAG].m_stRecoveryDescr),   DMAP_ReadAlertDescriptor,  DMAP_WriteAlertDescriptor,    
   ATTR_CONST(g_aARMO[ALERT_CAT_PROCESS].m_stRecoveryDescr),    DMAP_ReadAlertDescriptor,  DMAP_WriteAlertDescriptor, 
};

void ARMO_readAlertCommEndpoint(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize)
{
  ALERT_COMM_ENDPOINT* pstAlertCommEndpoint = (ALERT_COMM_ENDPOINT*)p_pValue;
  
  memcpy(p_pBuf, pstAlertCommEndpoint->m_aNetworkAddr, 16);
  p_pBuf += 16;
  
  *(p_pBuf++) = pstAlertCommEndpoint->m_unTLPort >> 8;
  *(p_pBuf++) = pstAlertCommEndpoint->m_unTLPort;
    
  *(p_pBuf++) = pstAlertCommEndpoint->m_unObjID >> 8;
  *(p_pBuf++) = pstAlertCommEndpoint->m_unObjID;  

  *p_ucSize = 20; 
}

void ARMO_writeAlertCommEndpoint(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize)
{
  ALERT_COMM_ENDPOINT* pstAlertCommEndpoint = (ALERT_COMM_ENDPOINT*)p_pValue;
  
  memcpy(pstAlertCommEndpoint->m_aNetworkAddr, p_pBuf, 16);
  
  pstAlertCommEndpoint->m_unTLPort = ((uint16)p_pBuf[16] << 8) | p_pBuf[17];
  pstAlertCommEndpoint->m_unObjID = ((uint16)p_pBuf[18] << 8) | p_pBuf[19];
  
  pstAlertCommEndpoint->m_ucContractStatus = ARMO_NO_CONTR;  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman
/// @brief  Initializes ARMO's attributes
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ARMO_Init()
{  
  //defaults
  for(uint8 ucCnt=0; ucCnt < ALERT_CAT_NO; ucCnt++)
  {
      if( ARMO_MIN_CONF_TIMEOUT > g_aARMO[ucCnt].m_ucConfTimeout )
      {
          //the "m_ucConfTimeout" is persistent data
          g_aARMO[ucCnt].m_ucConfTimeout = ARMO_DEFAULT_CONF_TIMEOUT;
      }
      
      g_aARMO[ucCnt].m_stAlertMaster.m_ucContractStatus = ARMO_NO_CONTR;
      g_aARMO[ucCnt].m_ucAlertReportTimer = 0;      
  }
}

#pragma inline 
uint8 ARMO_getNextAlertId( uint8 p_ucCategory)
{
    // alert id's generated separately for each category
    // ack comes only with id, so the following line 
    // is a trick to avoid duplicate id's from different categories
    return (p_ucCategory << 6) | ((g_aARMO[p_ucCategory].m_ucAlertsNo++) & 0x3F);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Adds an alert to the ARMO alert queue
/// @param  p_pAlert - pointer to an alert type structure
/// @param  p_pBuf   - pointer to a buffer that contains alert data (Alert data len is inside p_pAlert->m_unSize)
/// @return service feddback code
/// @remarks
///      Access level: user level
uint8 ARMO_AddAlertToQueue(const ALERT* p_pAlert, const uint8* p_pBuf)
{
  uint16 unLen = DMAP_GetAlignedLength( sizeof(ALERT) + p_pAlert->m_unSize );
  
  if (unLen > sizeof(g_aucARMOQueue)) // alert too big; will not fit even if queue is empty 
      return SFC_INVALID_SIZE;                       
  
  if( !g_aARMO[p_pAlert->m_ucCategory].m_ucAlertsDisable && (unLen < sizeof(g_aucARMOQueue)) ) // valid alert
  {  
      while( (g_pARMOQueueEnd + unLen) > (g_aucARMOQueue + sizeof(g_aucARMOQueue) ) ) // not enough space
      {
          // remove first alarm (oldest one); do not check that g_pARMOQueueEnd > g_aucARMOQueue
          ALERT* pAlert = (ALERT*)g_aucARMOQueue;
          uint16 unAlertLen = DMAP_GetAlignedLength( sizeof(ALERT) + pAlert->m_unSize );
          
          g_pARMOQueueEnd -= unAlertLen;
          memmove(g_aucARMOQueue, g_aucARMOQueue+unAlertLen, g_pARMOQueueEnd-g_aucARMOQueue); 
      }
    
      ALERT* pNext = (ALERT*)g_pARMOQueueEnd;
      
      // copy alert header
      memcpy(pNext, p_pAlert, sizeof(ALERT));      
      
      pNext->m_ucID = ARMO_getNextAlertId( p_pAlert->m_ucCategory );
      pNext->m_ulNextSendTAI = 0;  
      
      // fill the detection time 
      MLSM_GetCrtTaiTime( &pNext->m_stDetectionTime.m_ulSeconds, &pNext->m_stDetectionTime.m_unFract );
      
      // copy alert data  
      memcpy(g_pARMOQueueEnd + sizeof(ALERT), p_pBuf, p_pAlert->m_unSize);
      
      g_pARMOQueueEnd += unLen;
             
      return SFC_SUCCESS;
  }
  
  return SFC_OBJECT_STATE_CONFLICT; // alerts are disabled for this category
}


void ARMO_ProcessAlertAck(uint8 p_ucID)
{
  uint8* pBuf = g_aucARMOQueue;
  
  while (  pBuf < g_pARMOQueueEnd )
  {
    ALERT* pAlert = (ALERT*)pBuf;
    uint16 unLen = DMAP_GetAlignedLength( sizeof(ALERT) + pAlert->m_unSize );
    if(pAlert->m_ucID == p_ucID) // alert found
    {
      g_pARMOQueueEnd -= unLen;
      memmove(pBuf, pBuf+unLen, g_pARMOQueueEnd-pBuf);
      break;
    }
    
    pBuf += unLen;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Handles the establishment of the four alarm contracts for each alarm category
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ARMO_checkAlarmContracts(void)
{  
  for( uint8 ucIdx = 0; ucIdx < ALERT_CAT_NO; ucIdx++ )
  {
      switch (g_aARMO[ucIdx].m_stAlertMaster.m_ucContractStatus)
      {
          case ARMO_CONTRACT_ACTIVE: break;
          
          case ARMO_AWAIT_CONTRACT_ESTABLISHMENT: 
          case ARMO_WAITING_CONTRACT_TERMINATION:   
              // check if the time for contract establishment or contract termination expired;
              if ( ((uint16)MLSM_GetCrtTaiSec() -  g_unContractReqTimestamp) > CONTRACT_WAIT_TIMEOUT )
              {
                  g_aARMO[ucIdx].m_stAlertMaster.m_ucContractStatus = ARMO_NO_CONTR;
              }                
              break;
          
          case ARMO_NO_CONTR:  
//              if (g_aARMO[ucIdx].m_stAlertMaster.m_unContractID)
//              {
//                  // there was an active contract running for this endpoint; endpoint has been reconfigured,
//                  // so termination of the previous contract is needed.    
//                  if ( SFC_SUCCESS == DMO_RequestContractTermination( g_aARMO[ucIdx].m_stAlertMaster.m_unContractID,
//                                                                      CONTRACT_TERMINATION) )
//                  {                  
//                      g_aARMO[ucIdx].m_stAlertMaster.m_ucContractStatus = ARMO_WAITING_CONTRACT_TERMINATION;
//                      g_unContractTimeout = MLSM_GetCrtTaiSec() + (uint16)CONTRACT_WAIT_TIMEOUT;  
//                  }
//              }
//              else 
//              {
                   ARMO_configureEndpoint(&g_aARMO[ucIdx].m_stAlertMaster, ucIdx);               
//              }
              
              break;
      }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Issues a new contract request to DMO object of DMAP
/// @param  p_pEndpoint - pointer to an endpoint descriptor
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 ARMO_configureEndpoint(ALERT_COMM_ENDPOINT * p_pEndpoint, uint8 p_ucCategory )
{
  uint8 ucStatus;
  
  if (p_ucCategory > ALERT_CAT_PROCESS)
      return SFC_INVALID_PARAMETER;
  
  // first check if there is already a useful contract in DMAP
  DMO_CONTRACT_ATTRIBUTE * pContract = DMO_GetContract( p_pEndpoint->m_aNetworkAddr, 
                                                        p_pEndpoint->m_unTLPort, 
                                                        ISA100_DMAP_PORT, 
                                                        SRVC_APERIODIC_COMM );  
  if (pContract)
  {
      g_aARMO[p_ucCategory].m_stAlertMaster.m_unContractID     = pContract->m_unContractID;
      g_aARMO[p_ucCategory].m_stAlertMaster.m_ucContractStatus = ARMO_CONTRACT_ACTIVE;  
      return SFC_SUCCESS; 
  }
  
  // contract not available; request a new contract. 
  // todo: check aperiodic properties with SM
  DMO_CONTRACT_BANDWIDTH stBandwidth;  
  stBandwidth.m_stAperiodic.m_nComittedBurst  = 1;
  stBandwidth.m_stAperiodic.m_nExcessBurst    = 1;
  stBandwidth.m_stAperiodic.m_ucMaxSendWindow = 1;

  ucStatus = DMO_RequestNewContract(  p_pEndpoint->m_aNetworkAddr,
                                      p_pEndpoint->m_unTLPort,
                                      ISA100_DMAP_PORT,           // p_unSrcSAP,
                                      0xFFFFFFFF,                 // contract life
                                      SRVC_APERIODIC_COMM,        // p_ucSrcvType,
                                      DMO_PRIORITY_BEST_EFFORT,   // p_ucPriority,
                                      MAX_APDU_SIZE,              // p_unMaxAPDUSize,
                                      1,                          //  p_ucReliability,
                                      0,                          // UAP request contract ID (doesn't matter)
                                      &stBandwidth );  
  if (SFC_SUCCESS == ucStatus)
  {
      g_unContractReqTimestamp = (uint16)MLSM_GetCrtTaiSec();  
      g_aARMO[p_ucCategory].m_stAlertMaster.m_ucContractStatus = ARMO_AWAIT_CONTRACT_ESTABLISHMENT;  
  }
  
  return ucStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Called by DMAP each time a new contract entry is written into DMO contract table.
/// @param  p_pContract - pointer to the new contract
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ARMO_NotifyAddContract(DMO_CONTRACT_ATTRIBUTE * p_pContract)
{
  ARMO_STRUCT * pArmo = g_aARMO;
  for( ; pArmo < g_aARMO+ALERT_CAT_NO; pArmo++ )
  {
      // check if this alarm category is waiting for a contract
      if( (pArmo->m_stAlertMaster.m_ucContractStatus == ARMO_AWAIT_CONTRACT_ESTABLISHMENT)
          && (pArmo->m_stAlertMaster.m_unTLPort  == p_pContract->m_unDstTLPort)
          && (!memcmp(pArmo->m_stAlertMaster.m_aNetworkAddr,
                      p_pContract->m_aDstAddr128,
                      sizeof(p_pContract->m_aDstAddr128)))
            )
      {
          pArmo->m_stAlertMaster.m_unContractID = p_pContract->m_unContractID;
          pArmo->m_stAlertMaster.m_ucContractStatus = ARMO_CONTRACT_ACTIVE;
      }                      
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Checks if a deleted contract in DMAP is in the scope of the ARMO object
/// @param  p_unContractID  - contract ID
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ARMO_NotifyContractDeletion(uint16 p_unContractID)
{
  // tbd
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Implements the alarm recovery state machine: generates the recovery start and end events
///         and recovers all required alarms.
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 ARMO_checkRecoveryState(void)
{
#warning "to decide if the state machine should wait for RECOVERY_START event acknowledge before reporting the alerts from queue "
  
  uint8 ucRecoveryStatus = FALSE;
  
  for (uint8 ucIdx = 0; ucIdx < ALERT_CAT_NO; ucIdx++)
  {
      if (g_aARMO[ucIdx].m_stRecovery.m_ucState != RECOVERY_DISABLED)
      {
          ucRecoveryStatus = TRUE;
          
          switch (g_aARMO[ucIdx].m_stRecovery.m_ucState)
          {
            case RECOVERY_ENABLED:  
                if (SFC_SUCCESS == ARMO_generateRecoveryEvent(ucIdx, ALARM_RECOVERY_START))
                {
                    g_aARMO[ucIdx].m_stRecovery.m_ucState = RECOVERY_START_SENT;                    
                } 
                break;            
            
            case RECOVERY_START_SENT:
                if (SFC_SUCCESS != ARMO_recoverNextAlarm(ucIdx, g_aARMO[ucIdx].m_stRecovery.m_ucLastID))
                {
                    g_aARMO[ucIdx].m_stRecovery.m_ucState = RECOVERY_DONE;        
                }
                break;
                
            case RECOVERY_DONE:
                if (SFC_SUCCESS == ARMO_generateRecoveryEvent(ucIdx, ALARM_RECOVERY_END))
                {
                    g_aARMO[ucIdx].m_stRecovery.m_ucState = RECOVERY_DISABLED; // recovery finished; disble recovery mode                    
                } 
                break;  
          }
          
          return ucRecoveryStatus; // do not check following categories untill current category is finished;
      }
  }
  
  return ucRecoveryStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 ARMO_recoverNextAlarm(uint8 p_ucCategory, uint8 p_ucLastID)
{ 
  uint8 * pBuf = g_aucARMOQueue;
  
  ALERT_REP_SRVC stAlertRep;  
  stAlertRep.m_unSrcOID = DMAP_ARMO_OBJ_ID;
  
  while(pBuf < g_pARMOQueueEnd)
  {
      ALERT * pAlert = (ALERT*)pBuf;
      uint16 unLen = DMAP_GetAlignedLength( sizeof(ALERT) + pAlert->m_unSize );
      
      if( (pAlert->m_ucCategory == p_ucCategory) 
          && ( (pAlert->m_ucID > p_ucLastID) || (((p_ucLastID & 0x3F) > 61) && ((pAlert->m_ucID & 0x3F) < 3)) )  // try to catch alertId overflows also  
          && (g_aARMO[p_ucCategory].m_stAlertMaster.m_ucContractStatus == ARMO_CONTRACT_ACTIVE) )
      {
          stAlertRep.m_unDstOID = g_aARMO[p_ucCategory].m_stAlertMaster.m_unObjID;          
                    
          memcpy(&stAlertRep.m_stAlertInfo, pAlert, sizeof(ALERT));
          stAlertRep.m_pAlertValue = pBuf + sizeof(ALERT);
          
          if (SFC_SUCCESS == ASLSRVC_AddGenericObject( &stAlertRep,
                                                       SRVC_ALERT_REP,
                                                       0,                     // priority
                                                       UAP_DMAP_ID,       // SrcTSAPID !?
                                                       g_aARMO[p_ucCategory].m_stAlertMaster.m_unTLPort & 0x0F,       // DstTSAPID !?
                                                       0,
                                                       NULL,
                                                       g_aARMO[p_ucCategory].m_stAlertMaster.m_unContractID,
                                                       0 )) //p_unBinSize)
          {
              g_aARMO[p_ucCategory].m_stRecovery.m_ucLastID = pAlert->m_ucID;
          }   
          return SFC_SUCCESS;
      }
      pBuf += unLen;
  }
  
  return SFC_FAILURE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Generates an ARMO type event. This event is not added to the queue because it has to be 
///         sent first. All other alarms are reported after this event is sent!
/// @return SFC_SUCCESS if succesflyy sent to ASL queu
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 ARMO_generateRecoveryEvent(uint8 p_ucCategory, uint8 p_ucAlertType)
{  
  if (p_ucCategory >= ALERT_CAT_NO)
      return SFC_INVALID_ARGUMENT;   // invalid alert category
  
  if (g_aARMO[p_ucCategory].m_stAlertMaster.m_ucContractStatus != ARMO_CONTRACT_ACTIVE)
  {
      return SFC_NO_CONTRACT;  
  }
  
  ALERT_REP_SRVC stAlertRep;  
  stAlertRep.m_unSrcOID = DMAP_ARMO_OBJ_ID;
  stAlertRep.m_unDstOID = g_aARMO[p_ucCategory].m_stAlertMaster.m_unObjID;
  
  MLSM_GetCrtTaiTime( &stAlertRep.m_stAlertInfo.m_stDetectionTime.m_ulSeconds, 
                      &stAlertRep.m_stAlertInfo.m_stDetectionTime.m_unFract );
  
  stAlertRep.m_stAlertInfo.m_unDetObjTLPort = 0xF0B0;  // standard ISA100 DMAP port 
  stAlertRep.m_stAlertInfo.m_unDetObjID     = DMAP_ARMO_OBJ_ID;
  stAlertRep.m_stAlertInfo.m_ucCategory     = p_ucCategory;
  stAlertRep.m_stAlertInfo.m_ucType         = p_ucAlertType;
  stAlertRep.m_stAlertInfo.m_ucClass        = ALERT_CLASS_EVENT; 
  stAlertRep.m_stAlertInfo.m_ucPriority     = ALERT_PR_LOW_L;
  stAlertRep.m_stAlertInfo.m_ucDirection    = ALARM_DIR_RET_OR_NO_ALARM;  
  
  stAlertRep.m_stAlertInfo.m_ucID = ARMO_getNextAlertId(p_ucCategory);
  
  stAlertRep.m_stAlertInfo.m_unSize = 0;  
  
  return ASLSRVC_AddGenericObject( &stAlertRep,
                                   SRVC_ALERT_REP,
                                   0,                     // priority
                                   UAP_DMAP_ID,       // SrcTSAPID !?
                                   g_aARMO[p_ucCategory].m_stAlertMaster.m_unTLPort & 0x0F,       // DstTSAPID !?
                                   0,
                                   NULL,
                                   g_aARMO[p_ucCategory].m_stAlertMaster.m_unContractID,
                                   0 ); //p_unBinSize
}
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Implements alert reporting management object's state machine
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ARMO_Task()
{    
  ARMO_checkAlarmContracts();
  
  // if recovery mode is active, alarm reporting will be performed inside ARMO_checkRecoveryState()
  if (TRUE == ARMO_checkRecoveryState())
      return; 
  

  ALERT_REP_SRVC stAlertRep;    
  stAlertRep.m_unSrcOID = DMAP_ARMO_OBJ_ID;
    
  uint8* pBuf = g_aucARMOQueue;
  uint32 ulCrtSec = MLSM_GetCrtTaiSec();
  while( pBuf < g_pARMOQueueEnd )
  {

    ALERT* pAlert = (ALERT*)pBuf;
    uint16 unLen = DMAP_GetAlignedLength( sizeof(ALERT) + pAlert->m_unSize );
    
    stAlertRep.m_unDstOID = g_aARMO[pAlert->m_ucCategory].m_stAlertMaster.m_unObjID;
    
    memcpy(&stAlertRep.m_stAlertInfo, pAlert, sizeof(ALERT)); 
    stAlertRep.m_pAlertValue = pBuf + sizeof(ALERT);
    
    pBuf += unLen;  // prepare for the next while loop
    
    // check if the associated contract is active
    if (g_aARMO[pAlert->m_ucCategory].m_stAlertMaster.m_ucContractStatus != ARMO_CONTRACT_ACTIVE)
        continue; // check next alarm, maybe has another contract that is active    
    
    
    if(  pAlert->m_ulNextSendTAI < ulCrtSec ) 
    {       
        if(SFC_SUCCESS == ASLSRVC_AddGenericObject( &stAlertRep,
                                                    SRVC_ALERT_REP,
                                                    pAlert->m_ucPriority, // pAlert->m_ucPriority,  // priority
                                                    UAP_DMAP_ID,          // SrcTSAPID !?
                                                    g_aARMO[pAlert->m_ucCategory].m_stAlertMaster.m_unTLPort & 0x0F,       // DstTSAPID !?
                                                    0,
                                                    NULL,
                                                    g_aARMO[pAlert->m_ucCategory].m_stAlertMaster.m_unContractID,
                                                   0 )) //p_unBinSize
          
        {
            pAlert->m_ulNextSendTAI = ulCrtSec + g_aARMO[pAlert->m_ucCategory].m_ucConfTimeout;
        }
    }    
  }

}

uint8 ARMO_Execute(uint8   p_ucMethID, 
                   uint16  p_unReqSize, 
                   uint8*  p_pReqBuf,
                   uint16* p_pRspSize,
                   uint8*  p_pRspBuf)
{
  // ARMO has only one method: alarm recovery method  
  *p_pRspSize = 0; 
  
  if(ARMO_ALARM_RECOVERY != p_ucMethID)
      return SFC_INVALID_METHOD;
  
  if(1 != p_unReqSize)
      return SFC_INVALID_SIZE;

  uint8  ucAlertCategory = *p_pReqBuf;       
  if (ucAlertCategory >= ALERT_CAT_NO)
      return SFC_INCONSISTENT_CONTENT;
  
  // check if recovery is already active for this category
  if (g_aARMO[ucAlertCategory].m_stRecovery.m_ucState != RECOVERY_DISABLED)
      return SFC_OBJECT_STATE_CONFLICT;
  
  // enable the recovery state for thi category (generation of the alarm_recovery_start 
  // will be handled by th ARMO_Task;
  g_aARMO[ucAlertCategory].m_stRecovery.m_ucState = RECOVERY_ENABLED;
  
  return SFC_SUCCESS;
}

