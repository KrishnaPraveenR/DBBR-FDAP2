////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Mihaela Goloman
/// Date:         January 2008
/// Description:  This file implements the device manager application process
/// Changes:      Created
/// Revisions:    1.0
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "provision.h"
#include "tlme.h"
#include "slme.h"
#include "nlme.h"
#include "tlde.h"
#include "nlde.h"
#include "dlde.h"
#include "dmap.h"
#include "dmap_dmo.h"
#include "dmap_dlmo.h"
#include "dmap_co.h"
#include "dmap_dpo.h"
#include "dmap_armo.h"
#include "dmap_udo.h"
#include "uap_mo.h"
#include "aslsrvc.h" 
#include "mlsm.h"
#include "uap.h"
#include "uap_data.h"
#include "dlmo_utils.h"
#include "tmr_util.h"
#include "../asm.h"
#include "../system.h"
#include "../eth.h"

#ifndef BACKBONE_SUPPORT 
    #include "../CommonAPI/DAQ_Comm.h"
#endif

#if (DEVICE_TYPE == DEV_TYPE_MC13225)
    #include "uart_link.h"

#elif (DEVICE_TYPE == DEV_TYPE_MSP430F2618)
  #include "../wdt.h"
  #include "../spi1_eeprom.h"
  #include "../spi1.h"
#elif (DEVICE_TYPE == DEV_TYPE_AP91SAM7X512)
  #include "../spi1.h"
#endif
  #include "../bootl.h"


typedef void (*PF_SWAP)(void);

/*==============================[ DMAP ]=====================================*/
uint16 g_unAppHandle = 0;

#ifndef BACKBONE_SUPPORT

    typedef struct
    {
      uint16 m_unRouterAddr;
      uint16 m_unBannedPeriod;  //in seconds  
    } BANNED_ROUTER;
    
    uint8 g_ucBannedRoutNo;
    BANNED_ROUTER g_astBannedRouters[MAX_BANNED_ROUTER_NO];
    
    static SHORT_ADDR s_unDevShortAddr;
    uint16 g_unLastJoinCnt;

    static void AddBannedAdvRouter(SHORT_ADDR p_unDAUXRouterAddr, uint16 p_unBannedPeriod);
    static void UpdateBannedRouterStatus();
#else
    uint8 g_ucSMLinkTimeout = SM_LINK_TIMEOUT;    
#endif  //#ifndef BACKBONE_SUPPORT

#if (MAX_SIMULTANEOUS_JOIN_NO==1)    
  #warning  "for SM support, no more simultaneous joining sessions accepted" 
#endif  
    
const uint8 c_aTLMEMethParamLen[TLME_METHOD_NO-1] = {1, 18, 16, 18, 18}; // input arguments lenghts for tlmo methods  
    
uint16 g_unSysMngContractID; 

SHORT_ADDR  g_unDAUXRouterAddr;
uint16 g_unRadioSleepReqId;
uint16 g_unJoinCommandReqId;

uint32              g_ulDAUXTimeSync;
uint8               g_ucJoinStatus;
JOIN_RETRY_CONTROL  g_stJoinRetryCntrl;

static __no_init uint32 g_aulNewNodeChallenge[4] ;
static uint8 g_aucSecMngChallenge[16];

#if defined(ROUTING_SUPPORT) || defined(BACKBONE_SUPPORT)
  NEW_DEVICE_JOIN_INFO g_stDevInfo[MAX_SIMULTANEOUS_JOIN_NO];
  uint8 g_ucDevInfoEntryNo;
#endif // #if defined(ROUTING_SUPPORT || defined(BACKBONE_SUPPORT)
/*==============================[ DMO ]======================================*/
static void DMAP_processReadRequest( READ_REQ_SRVC * p_pReadReq,
                                     APDU_IDTF *     p_pIdtf);    

static void DMAP_processWriteRequest( WRITE_REQ_SRVC * p_pWriteReq,
                                      APDU_IDTF *      p_pIdtf);                                 

static void DMAP_processExecuteRequest( EXEC_REQ_SRVC * p_pExecReq,
                                        APDU_IDTF *     p_pIdtf);

static void DMAP_processReadResponse(READ_RSP_SRVC * p_pReadRsp, APDU_IDTF * p_pAPDUIdtf);

static void DMAP_processWriteResponse(WRITE_RSP_SRVC * p_pWriteRsp);

static void DMAP_processExecuteResponse( EXEC_RSP_SRVC * p_pExecRsp,
                                         APDU_IDTF *     p_pAPDUIdtf );
static void DMAP_joinStateMachine(void);
  
static uint16 DMO_generateSMJoinReqBuffer(uint8* p_pBuf);
static uint16 DMO_generateSecJoinReqBuffer(uint8* p_pucBuf);
static void DMO_checkSecJoinResponse(EXEC_RSP_SRVC* p_pExecRsp);
static void DMO_checkSMJoinResponse(EXEC_RSP_SRVC* p_pExecRsp);

static void DMAP_DMO_applyJoinStatus( uint8 p_ucSFC );

#if defined(ROUTING_SUPPORT) || defined(BACKBONE_SUPPORT)

  static uint8 DMAP_DMO_forwardJoinReq( EXEC_REQ_SRVC * p_pExecReq,
                                        APDU_IDTF *     p_pIdtf);
  
  static void DMAP_DMO_forwardJoinResp ( EXEC_RSP_SRVC * p_pExecRsp );
  
  static void DMAP_DMO_saveNewDevJoinInfo(  APDU_IDTF * p_pIdtf,
                                            uint8       p_ucReqID,
                                            uint8       p_ucFwReqID );
  
  static uint8 DMAP_DMO_getNewDevJoinInfo( uint8       p_ucFwReqID,
                                           uint8     * p_pEUI64,
                                           uint8     * p_pPriority,
                                           uint8 *     p_pReqID );
  
#else  
  #define DMAP_DMO_forwardJoinReq(...)  SFC_INVALID_SERVICE
  #define DMAP_DMO_forwardJoinResp(...)  
#endif //#if defined(ROUTING_SUPPORT || defined(BACKBONE_SUPPORT)


/*==============================[ DLMO ]=====================================*/
static void DMAP_startJoin(void);

#ifndef BACKBONE_SUPPORT 
  static __monitor void DMAP_DLMO_prepareSMIBS(void);
#endif // not BACKBONE_SUPPORT

static uint8 NLMO_prepareJoinSMIBS(void);
static uint8 NLMO_updateJoinSMIBS(void);
  
/*==============================[ TLMO ]=====================================*/
static uint8 TLMO_execute( uint8    p_ucMethID,
                           uint16   p_unReqSize,
                           uint8*   p_pReqBuf,
                           uint16 * p_pRspSize,
                           uint8*   p_pRspBuf);

/*==============================[ NLMO ]=====================================*/
static uint8 NLMO_execute( uint8    p_ucMethID,
                           uint16   p_unReqSize,
                           uint8 *  p_pReqBuf,
                           uint16 * p_pRspSize,
                           uint8 *  p_pRspBuf);

/*===========================[ DMAP implementation ]=========================*/

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman
/// @brief  Initializes the device manager application process
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_Init()
{    
  UDO_Init();
  
#ifndef BACKBONE_SUPPORT
  if( g_ucJoinStatus != DEVICE_DISCOVERY )
  {
      if( (DEVICE_JOINED != g_ucJoinStatus) || g_unLastJoinCnt ) // not success joined or joined for a short period of time
      {
          //the last joining Router will be banned
          AddBannedAdvRouter(g_unDAUXRouterAddr, REJOIN_ROUTER_BANNED_PERIOD);
      }
  } 

#endif
  
  g_unSysMngContractID = INVALID_CONTRACTID;

  DLMO_DOUBLE_IDX_SMIB_ENTRY stTwoIdxSMIB;
  
  for ( const DLL_SMIB_ENTRY_CHANNEL * pEntry = c_aDefChannels; pEntry < (c_aDefChannels + DEFAULT_CH_NO); pEntry++)
  {
      stTwoIdxSMIB.m_unUID = *(uint16*)pEntry;
#pragma diag_suppress=Pa039
      memcpy(&stTwoIdxSMIB.m_stEntry, pEntry, sizeof(DLL_SMIB_ENTRY_CHANNEL));  
#pragma diag_default=Pa039
      DLME_SetSMIBRequest( DL_CH, 0, &stTwoIdxSMIB );
      DLME_ParseMsgQueue(); // avoid getting full of DLME action queue 
  }
  
  for ( const DLL_SMIB_ENTRY_TIMESLOT * pEntry = c_aDefTemplates; pEntry < c_aDefTemplates + DEFAULT_TS_TEMPLATE_NO; pEntry++)
  {
      stTwoIdxSMIB.m_unUID = *(uint16*)pEntry;
#pragma diag_suppress=Pa039
      memcpy(&stTwoIdxSMIB.m_stEntry, pEntry, sizeof(DLL_SMIB_ENTRY_TIMESLOT)); 
#pragma diag_default=Pa039
      DLME_SetSMIBRequest( DL_TS_TEMPL, 0, &stTwoIdxSMIB );
      DLME_ParseMsgQueue(); // avoid getting full of DLME action queue 
  }
  
  g_ucJoinStatus = DEVICE_DISCOVERY;
  
  g_unRadioSleepReqId = 0xFFFF; //Radio Sleep is active
  g_unJoinCommandReqId = JOIN_CMD_WAIT_REQ_ID;
  
  DPO_Init();
    
#if defined(ROUTING_SUPPORT) || defined(BACKBONE_SUPPORT)  
  g_ucDevInfoEntryNo = 0;
#endif 
  
  DMO_Init();
  CO_Init();
  ARMO_Init();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Main DMAP task 
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_Task()
{  
  if ( g_ucJoinStatus != DEVICE_JOINED )
  {
      DMAP_joinStateMachine();      
  }
  
  // check if there is an incoming APDU that has to be processed by DMAP
  APDU_IDTF stAPDUIdtf;
  const uint8 * pAPDUStart = ASLDE_GetMyAPDU( UAP_DMAP_ID, &stAPDUIdtf );
  
  if ( pAPDUStart )
  {
      GENERIC_ASL_SRVC  stGenSrvc;
      const uint8 *     pNext = pAPDUStart;

      
#ifdef DLME_QUEUE_PROTECTION_ENABLED
      g_ucDLMEQueueCmd = 0;
#endif  
      while ( pNext = ASLSRVC_GetGenericObject( pNext,
                                                stAPDUIdtf.m_unDataLen - (pNext - pAPDUStart),
                                                &stGenSrvc,
                                                stAPDUIdtf.m_aucAddr)
              )
      {
          switch ( stGenSrvc.m_ucType )
          {
          case SRVC_READ_REQ  : DMAP_processReadRequest(  &stGenSrvc.m_stSRVC.m_stReadReq, &stAPDUIdtf );   break;
          case SRVC_WRITE_REQ : DMAP_processWriteRequest( &stGenSrvc.m_stSRVC.m_stWriteReq, &stAPDUIdtf );  break;
          case SRVC_EXEC_REQ  : DMAP_processExecuteRequest( &stGenSrvc.m_stSRVC.m_stExecReq, &stAPDUIdtf ); break;
          case SRVC_READ_RSP  : DMAP_processReadResponse( &stGenSrvc.m_stSRVC.m_stReadRsp, &stAPDUIdtf) ;   break;          
          case SRVC_WRITE_RSP : DMAP_processWriteResponse( &stGenSrvc.m_stSRVC.m_stWriteRsp );              break;          
          case SRVC_EXEC_RSP  : DMAP_processExecuteResponse( &stGenSrvc.m_stSRVC.m_stExecRsp, &stAPDUIdtf); break;     
          case SRVC_ALERT_ACK : ARMO_ProcessAlertAck(stGenSrvc.m_stSRVC.m_stAlertAck.m_ucAlertID);          break;
          }        
      }
#ifdef DLME_QUEUE_PROTECTION_ENABLED // protect the DLME internal queue 
      if( g_ucDLMEQueueCmd )
      {
          if( !g_ucDLMEQueueBusyFlag )
          {
              g_ucDLMEQueueBusyFlag = 1;
              ASLDE_DeleteAPDU( (uint8*)pAPDUStart ); // todo: check if always applicable       
          }
      }
      else
#endif  
      {
          ASLDE_DeleteAPDU( (uint8*)pAPDUStart ); // todo: check if always applicable       
      }
  }
  
  CO_ConcentratorTask();
  
  if(DEVICE_JOINED == g_ucJoinStatus)
  {
      ARMO_Task();  
  }
}
extern uint8 g_Enabled2009;
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Generic function for writing an attribute
/// @param  p_ucTSAPID - SAP ID of the process
/// @param  p_unObjID - object identifier
/// @param  p_pIdtf - attribute identifier structure
/// @param  p_pBuf - buffer containing attribute data
/// @param  p_unLen - buffer size
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 SetGenericAttribute( uint8       p_ucTSAPID,
                           uint16      p_unObjID,
                           ATTR_IDTF * p_pIdtf,
                           const uint8 *     p_pBuf,
                           uint16      p_unLen)
{    
    if( UAP_DMAP_ID == p_ucTSAPID )
    {    
        switch( p_unObjID )
        {
        case DMAP_UDO_OBJ_ID:   return UDO_Write( p_pIdtf->m_unAttrID & 0x003F, p_unLen, p_pBuf );
        case  DMAP_DLMO_OBJ_ID: return DLMO_Write( p_pIdtf, &p_unLen, p_pBuf );
        case DMAP_DMO_OBJ_ID:   return DMO_Write( p_pIdtf->m_unAttrID, p_unLen, p_pBuf );            
        case DMAP_ARMO_OBJ_ID:  return ARMO_Write( p_pIdtf->m_unAttrID, p_unLen, p_pBuf );            
        case DMAP_DSMO_OBJ_ID:  return DSMO_Write( p_pIdtf->m_unAttrID, p_unLen, p_pBuf );
        case DMAP_DPO_OBJ_ID:   return DPO_Write(p_pIdtf->m_unAttrID, p_unLen, p_pBuf ); 
        case DMAP_ASLMO_OBJ_ID: return ASLMO_Write(p_pIdtf->m_unAttrID, p_unLen, p_pBuf);               
        case DMAP_HRCO_OBJ_ID:
            g_pstCrtCO = AddConcentratorObject(p_unObjID, UAP_DMAP_ID);
            if( g_pstCrtCO )
            {
                //the appropriate CO element already exist or was just added - update/set the object attributes  
                //g_pstCrtCO will be used inside CO_Write!!!!!
                return CO_Write(p_pIdtf->m_unAttrID, p_unLen, p_pBuf);
            }
            break;
        
        
        case DMAP_TLMO_OBJ_ID:               
            if (p_unLen < 0xFF)
            {
                return TLME_SetRow( p_pIdtf->m_unAttrID,
                                     0,                   // no TAI cutover
                                     p_pBuf,
                                     (uint8)p_unLen); 
            }
            break;
    
    #if defined(at91sam7x512)
        case DMAP_CUST_PROV_OBJ_ID: 
        {

            uint8 ulInitialStartTAISec = g_ulStartTAISec;
        
            if(p_pIdtf->m_unAttrID == ENABLE_2009_ADVT)
            {
              if(*p_pBuf == 1)
                g_Enabled2009 = 1;
              else
                g_Enabled2009 = 0;
              
              
              return SFC_SUCCESS;
            }  
            uint8 ucStatus = PROV_Write( p_pIdtf->m_unAttrID, p_unLen, p_pBuf );
            if( SFC_SUCCESS == ucStatus )
            {
                if( PROV_CLOCK_DRIFT == p_pIdtf->m_unAttrID )
                {
                    int16 nClockDrift = g_nClockDriftTMR;

                    if( g_nClockDriftMicroSec < 0 )
                        g_nClockDriftTMR = -USEC_TO_TMR0(-g_nClockDriftMicroSec);
                    else
                        g_nClockDriftTMR = USEC_TO_TMR0(g_nClockDriftMicroSec);

                    //first time when the clock drift is set the TAI will be instantaneous applied
                    if( g_ucDiscoveryState == DEVICE_JOINED && g_nClockDriftTMR != nClockDrift )
                    {
                        //the clock adjust will be done at next 250ms alignment
                        g_ucClockDriftStatus = DRIFT_APPLY_ASAP;    
                    }
                }
                
                if( PROV_DLL_QUEUE_ORDER > p_pIdtf->m_unAttrID || PROV_START_TAI_SEC == p_pIdtf->m_unAttrID || PROV_PA_LEVEL == p_pIdtf->m_unAttrID )
                {
                    SaveProvisioningData();

                    if( PROV_PA_LEVEL == p_pIdtf->m_unAttrID )
                    {
#ifdef BBR1_HW  
                        CC2420_MACRO_SELECT();
                        CC2420_MACRO_SET_PA(g_ucPALevel);
                        CC2420_MACRO_RELEASE();
#endif
#ifdef BBR2_HW 
                        CC2520_1_MACRO_SELECT();
                        CC2520_1_MACRO_SET_PA(g_ucPALevel);
                        CC2520_1_MACRO_RELEASE();                 
#endif
                    }
                
                    
                    if( ulInitialStartTAISec != g_ulStartTAISec && !g_ulStartTAISec )
                    {    
                        //if StartTAI != 0x00000000 and its value is changed to 0x00000000 the DBBR will be automatically reset and then starting with NTP synchronization
                        DMAP_DLMO_ResetStack(7);
                    }
                }
                else
                {
                    if( MANUF_EUI64_ADDR == p_pIdtf->m_unAttrID )
                    {
                        //currently just the EUI64 address from the manufacturting is used by DBBR
                        SaveManufacturingData();
                    }
                }
                
            }
            
            return ucStatus; 
        }
    #endif //defined(at91sam7x512)
           
//            case DMAP_NLMO_OBJ_ID:     break;
        }
    }
#if ( _UAP_TYPE == UAP_TYPE_SIMPLE_API )
    else if ( p_ucTSAPID == UAP_APP1_ID )
    {
        //UAP_PROCESS_ID
        switch( p_unObjID )
        {
        case UAP_DATA_OBJ_ID: return  UAPDATA_Write(p_pIdtf->m_unAttrID, p_unLen, p_pBuf);
        case UAP_MO_OBJ_ID:   return UAPMO_Write(p_pIdtf->m_unAttrID, p_unLen, p_pBuf);  
        case UAP_CO_OBJ_ID:
            g_pstCrtCO = AddConcentratorObject(p_unObjID, UAP_APP1_ID);
            if( g_pstCrtCO )
            {
                //the appropriate CO element already exist or was just added - update/set the object attributes  
                //g_pstCrtCO will be used inside CO_Write!!!!!
                return CO_Write(p_pIdtf->m_unAttrID, p_unLen, p_pBuf);
            }
            break;
        
//            case UAP_DISP_OBJ_ID: break;
        }
    }
#endif
    
    return SFC_INVALID_OBJ_ID;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Generic function for reading an attribute
/// @param  p_ucTSAPID - SAP ID of the process
/// @param  p_unObjID - object identifier
/// @param  p_pIdtf - attribute identifier structure
/// @param  p_pBuf - output buffer containing attribute data
/// @param  p_punLen - buffer size
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 GetGenericAttribute( uint8       p_ucTSAPID,
                           uint16      p_unObjID,
                           ATTR_IDTF * p_pIdtf,
                           uint8 *     p_pBuf,
                           uint16 *    p_punLen)
{    
    if( UAP_DMAP_ID == p_ucTSAPID )
    {
        switch( p_unObjID )
        {
        case DMAP_UDO_OBJ_ID:   return UDO_Read( p_pIdtf->m_unAttrID, p_punLen, p_pBuf ); 
        case DMAP_DLMO_OBJ_ID:  return DLMO_Read( p_pIdtf, p_punLen, p_pBuf );                      
        case DMAP_DMO_OBJ_ID:   return DMO_Read( p_pIdtf, p_punLen, p_pBuf );
        case DMAP_DSMO_OBJ_ID:  return DSMO_Read( p_pIdtf->m_unAttrID, p_punLen, p_pBuf );
        case DMAP_ARMO_OBJ_ID:  return ARMO_Read( p_pIdtf->m_unAttrID, p_punLen, p_pBuf );
        case DMAP_DPO_OBJ_ID:   return DPO_Read( p_pIdtf->m_unAttrID, p_punLen, p_pBuf );                
        case DMAP_ASLMO_OBJ_ID: return ASLMO_Read( p_pIdtf->m_unAttrID, p_punLen, p_pBuf );  
        
        case DMAP_HRCO_OBJ_ID:
            g_pstCrtCO = FindConcentratorByObjId(p_unObjID, UAP_DMAP_ID);
            if( g_pstCrtCO )
            {
                return CO_Read(p_pIdtf->m_unAttrID, p_punLen, p_pBuf );
            }
            break;        
            
        
        case DMAP_TLMO_OBJ_ID  : 
              *p_punLen = 0;
              return  TLME_GetRow( p_pIdtf->m_unAttrID, 
                                   NULL,        // no indexed attributes in TLMO
                                   0,           // no indexed attributes in TLMO
                                   p_pBuf,
                                   (uint8*)p_punLen );
#if defined(at91sam7x512)
        case DMAP_CUST_PROV_OBJ_ID: 
            return PROV_Read( p_pIdtf->m_unAttrID, p_punLen, p_pBuf );
#endif //defined(at91sam7x512)
            
//            case DMAP_NLMO_OBJ_ID  : break;// TBD            
              
        }
    }  
#if ( _UAP_TYPE == UAP_TYPE_SIMPLE_API )
    else if ( p_ucTSAPID == UAP_APP1_ID )
    {
        //UAP_PROCESS_ID
        switch( p_unObjID )
        {
        case UAP_MO_OBJ_ID:    return UAPMO_Read(p_pIdtf->m_unAttrID, p_punLen, p_pBuf);  
        case UAP_DATA_OBJ_ID:  return UAPDATA_Read(p_pIdtf->m_unAttrID, p_punLen, p_pBuf);
        
        case UAP_CO_OBJ_ID:
            g_pstCrtCO = FindConcentratorByObjId(p_unObjID, UAP_APP1_ID);
            if( g_pstCrtCO )
            {
                return CO_Read(p_pIdtf->m_unAttrID, p_punLen, p_pBuf );
            }
            break;
        
//            case UAP_DISP_OBJ_ID: break;  
        }
    }
#endif
              
    *p_punLen = 0;
    return SFC_INVALID_OBJ_ID;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Implements the join state machine
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_joinStateMachine(void)
{
    switch ( g_ucJoinStatus )
    {      
    case DEVICE_DISCOVERY:    
        if( g_ucProvisioned ) 
        {  
            if( !g_ulStartTAISec && (MLSM_GetCrtTaiSec() < 1576800000) ) // make sure the time was synchronized (aprox. at least 1stJanuary2008)
            {
                return;
            }
            DMAP_DMO_applyJoinStatus( SFC_SUCCESS );
        }
        break;        
    } 
}
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Processes a read request in the application queue and passes it to the target object 
/// @param  p_pReadReq - read request structure
/// @param  p_pIdtf - apdu structure
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_processReadRequest( READ_REQ_SRVC * p_pReadReq,
                              APDU_IDTF *     p_pIdtf)

{
  READ_RSP_SRVC stReadRsp;
  uint8         aucRspBuff[MAX_GENERIC_VAL_SIZE]; // todo: check the size of the buffer

  stReadRsp.m_unDstOID = p_pReadReq->m_unSrcOID;
  stReadRsp.m_unSrcOID = p_pReadReq->m_unDstOID;
  stReadRsp.m_ucReqID  = p_pReadReq->m_ucReqID;
  
  stReadRsp.m_pRspData = aucRspBuff;
  stReadRsp.m_unLen = sizeof(aucRspBuff); // inform following functions about maximum available buffer size;
  
  stReadRsp.m_ucSFC = GetGenericAttribute( UAP_DMAP_ID,
                                           p_pReadReq->m_unDstOID,    
                                           &p_pReadReq->m_stAttrIdtf, 
                                           stReadRsp.m_pRspData, 
                                           &stReadRsp.m_unLen );
  
  // todo: check if Add succsesfull, otherwise mark the processed apdu as NOT_PROCESSED
  ASLSRVC_AddGenericObject(  &stReadRsp,
                             SRVC_READ_RSP,
                             0,                         // priority
                             p_pIdtf->m_ucDstTSAPID,    // SrcTSAPID
                             p_pIdtf->m_ucSrcTSAPID,    // DstTSAPID
                             0,
                             NULL,                      // EUI64 addr
                             g_unSysMngContractID,
                             0 ); //p_unBinSize
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Processes a write request in the application queue and passes it to the target object 
/// @param  p_pWriteReq - write request structure
/// @param  p_pIdtf - apdu structure
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_processWriteRequest( WRITE_REQ_SRVC * p_pWriteReq,
                               APDU_IDTF *      p_pIdtf)
{
  WRITE_RSP_SRVC stWriteRsp;  
  
  stWriteRsp.m_unDstOID = p_pWriteReq->m_unSrcOID;
  stWriteRsp.m_unSrcOID = p_pWriteReq->m_unDstOID;
  stWriteRsp.m_ucReqID  = p_pWriteReq->m_ucReqID;
  
  stWriteRsp.m_ucSFC = SetGenericAttribute( UAP_DMAP_ID,
                                            p_pWriteReq->m_unDstOID,
                                            &p_pWriteReq->m_stAttrIdtf,
                                            p_pWriteReq->p_pReqData,
                                            p_pWriteReq->m_unLen );                 

  //exception for RadioSleep - DL_RADIO_SLEEP = 23
  //RadioSleep will be activated later after transmission confirm
  if( DL_RADIO_SLEEP == p_pWriteReq->m_stAttrIdtf.m_unAttrID && 
      DMAP_DLMO_OBJ_ID == p_pWriteReq->m_unDstOID )
  {
      g_unRadioSleepReqId = p_pWriteReq->m_ucReqID;
      g_ulRadioSleepCounter = 0;
  }
  
  //exception for JoinCommand
  if( DMO_JOIN_COMMAND == p_pWriteReq->m_stAttrIdtf.m_unAttrID && 
      DMAP_DMO_OBJ_ID == p_pWriteReq->m_unDstOID &&
      JOIN_CMD_WAIT_REQ_ID == g_unJoinCommandReqId )
  {
      g_unJoinCommandReqId = p_pWriteReq->m_ucReqID;
  }

  // todo: check if Add succsesfull, otherwise mark the processed apdu as NOT_PROCESSED
  ASLSRVC_AddGenericObject(  &stWriteRsp,
                             SRVC_WRITE_RSP,
                             0,
                             p_pIdtf->m_ucDstTSAPID,    // SrcTSAPID; faster than extracting it from contract
                             p_pIdtf->m_ucSrcTSAPID,    // DstTSAPID; faster than extracting it from contract  
                             0,
                             NULL,                      // EUI64 addr
                             g_unSysMngContractID,
                             0 );                       // p_unBinSize
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Processes an execute request in the application queue and passes it to the target object 
/// @param  p_pExecReq - execute request structure
/// @param  p_pIdtf - apdu structure
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_processExecuteRequest( EXEC_REQ_SRVC * p_pExecReq,
                                 APDU_IDTF *     p_pIdtf )
{
    EXEC_RSP_SRVC stExecRsp;
    uint8         aucRsp[MAX_RSP_SIZE]; // check this size
    
    stExecRsp.p_pRspData = aucRsp;
    stExecRsp.m_unDstOID = p_pExecReq->m_unSrcOID;
    stExecRsp.m_unSrcOID = p_pExecReq->m_unDstOID;
    stExecRsp.m_ucReqID  = p_pExecReq->m_ucReqID;
    
    stExecRsp.m_unLen = 0;
 
    switch( p_pExecReq->m_unDstOID )
    {
        case DMAP_UDO_OBJ_ID:
            UDO_ProcessExecuteRequest( p_pExecReq, &stExecRsp );
            break;
        
        case DMAP_DLMO_OBJ_ID:
            stExecRsp.m_ucSFC = DLMO_Execute( p_pExecReq->m_ucMethID,
                                             p_pExecReq->m_unLen,
                                             p_pExecReq->p_pReqData,
                                             &stExecRsp.m_unLen,
                                             stExecRsp.p_pRspData);      
            break;
        
        case DMAP_TLMO_OBJ_ID:
            stExecRsp.m_ucSFC = TLMO_execute( p_pExecReq->m_ucMethID,
                                              p_pExecReq->m_unLen,
                                              p_pExecReq->p_pReqData,
                                              &stExecRsp.m_unLen,
                                              stExecRsp.p_pRspData);
            break; 
        
        case DMAP_NLMO_OBJ_ID:
            stExecRsp.m_ucSFC = NLMO_execute( p_pExecReq->m_ucMethID,
                                             p_pExecReq->m_unLen,
                                             p_pExecReq->p_pReqData,
                                             &stExecRsp.m_unLen,
                                             stExecRsp.p_pRspData);
            break; 
        
        case DMAP_DMO_OBJ_ID:
            switch( p_pExecReq->m_ucMethID )
            {
                case DMO_PROXY_SM_JOIN_REQ: 
                case DMO_PROXY_SM_CONTR_REQ:
                case DMO_PROXY_SEC_SYM_REQ:
                    stExecRsp.m_ucSFC = DMAP_DMO_forwardJoinReq( p_pExecReq, p_pIdtf );
                    if( SFC_SUCCESS != stExecRsp.m_ucSFC )
                    {
                        ASLSRVC_AddGenericObject( &stExecRsp,
                                                  SRVC_EXEC_RSP,
                                                  0,                 // priority 
                                                  UAP_DMAP_ID,       // SrcTSAPID
                                                  UAP_DMAP_ID,       // DstTSAPID
                                                  0,                 
                                                  p_pIdtf->m_aucAddr,  // dest EUI64 addr
                                                  0,                   // no contract
                                                  0  );
                    }
                    return; // don't send a response yet 
                    
                default:
                    stExecRsp.m_ucSFC = DMO_Execute( p_pExecReq->m_ucMethID,
                                                     p_pExecReq->m_unLen,
                                                     p_pExecReq->p_pReqData,
                                                     &stExecRsp.m_unLen,
                                                     stExecRsp.p_pRspData);
            }
            break;
            
        case DMAP_DSMO_OBJ_ID:
            DSMO_Execute( p_pExecReq, &stExecRsp );
            break;
            
        case DMAP_DPO_OBJ_ID:     
            stExecRsp.m_ucSFC = DPO_Execute(  p_pExecReq->m_ucMethID,
                                              p_pExecReq->m_unLen, 
                                              p_pExecReq->p_pReqData,
                                              &stExecRsp.m_unLen,
                                              stExecRsp.p_pRspData);
            break;            
            
        case DMAP_ARMO_OBJ_ID:    
            stExecRsp.m_ucSFC = ARMO_Execute(  p_pExecReq->m_ucMethID,
                                              p_pExecReq->m_unLen, 
                                              p_pExecReq->p_pReqData,
                                              &stExecRsp.m_unLen,
                                              stExecRsp.p_pRspData);
            break;
            
        case DMAP_ASLMO_OBJ_ID:
            stExecRsp.m_ucSFC = ASLMO_Execute( p_pExecReq->m_ucMethID,
                                               p_pExecReq->m_unLen, 
                                               p_pExecReq->p_pReqData,
                                               &stExecRsp.m_unLen,
                                               stExecRsp.p_pRspData);
            break;
            
        default: 
            stExecRsp.m_ucSFC = SFC_INVALID_OBJ_ID;
            break;  
    }
    
    // todo: check if Add succsesfull, otherwise mark the processed apdu as NOT_PROCESSED
    ASLSRVC_AddGenericObject(  &stExecRsp,
                             SRVC_EXEC_RSP,
                             0,                         // priority
                             p_pIdtf->m_ucDstTSAPID,    // SrcTSAPID
                             p_pIdtf->m_ucSrcTSAPID,    // DstTSAPID
                             0,                             
                             NULL,                      // EUI64 addr
                             g_unSysMngContractID,
                             0 );                       // p_unBinSize
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Processes a read response service
/// @param  p_pReadRsp - pointer to the read response structure
/// @param  p_pAPDUIdtf - pointer to the appropriate request APDU
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_processReadResponse(READ_RSP_SRVC * p_pReadRsp, APDU_IDTF * p_pAPDUIdtf)
{
    uint16            unTxAPDULen;
    GENERIC_ASL_SRVC  stGenSrvc;
    
    const uint8 * pTxAPDU = ASLDE_SearchOriginalRequest( p_pReadRsp->m_ucReqID,                                                         
                                                         p_pReadRsp->m_unSrcOID,
                                                         p_pReadRsp->m_unDstOID,
                                                         p_pAPDUIdtf,                                                         
                                                         &unTxAPDULen);      
    if( NULL == pTxAPDU )
        return;
    
    if( NULL == ASLSRVC_GetGenericObject( pTxAPDU, unTxAPDULen, &stGenSrvc , NULL) )
        return;
    
    if( SRVC_READ_REQ != stGenSrvc.m_ucType )
        return;
    
    if( stGenSrvc.m_stSRVC.m_stReadReq.m_unDstOID != p_pReadRsp->m_unSrcOID )
        return; 
    
    if( SFC_SUCCESS == p_pReadRsp->m_ucSFC && SM_STSO_OBJ_ID == p_pReadRsp->m_unSrcOID )
    {
    
        switch( stGenSrvc.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_unAttrID )
        {
#ifdef BACKBONE_SUPPORT
            case STSO_CTR_UTC_OFFSET:
                if( sizeof(g_stDPO.m_nCurrentUTCAdjustment) == p_pReadRsp->m_unLen )   
                {
                    g_stDPO.m_nCurrentUTCAdjustment = ((uint16)p_pReadRsp->m_pRspData[0] << 8) | p_pReadRsp->m_pRspData[1];
                }    
                break;
            case STSO_NEXT_UTC_TIME:
                if( sizeof(g_stDMO.m_ulNextDriftTAI) == p_pReadRsp->m_unLen )
                {
                    g_stDMO.m_ulNextDriftTAI = ((uint32)*p_pReadRsp->m_pRspData << 24) |
                                               ((uint32)*(p_pReadRsp->m_pRspData+1) << 16) | 
                                               ((uint32)*(p_pReadRsp->m_pRspData+2) << 8) | 
                                               *(p_pReadRsp->m_pRspData+3);
                }
                break;
            case STSO_NEXT_UTC_OFFSET:
                if( sizeof(g_stDMO.m_unNextUTCDrift) == p_pReadRsp->m_unLen )
                {
                    g_stDMO.m_unNextUTCDrift = ((uint16)p_pReadRsp->m_pRspData[0] << 8) | p_pReadRsp->m_pRspData[1];
                }
                break;
#endif  //#ifdef BACKBONE_SUPPORT
            default:;
        }
    }
    else
    {
        //update for other desired attributes
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Processes a write response service
/// @param  p_pWriteRsp - pointer to the write response structure
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_processWriteResponse(WRITE_RSP_SRVC * p_pWriteRsp)
{
  // TBD; no write request is issued yet by DMAP  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, 
/// @brief  Processes an device management object response
/// @param  p_unSrcOID - source object identifier
/// @param  p_ucMethID - method identifier
/// @param  p_pExecRsp - pointer to the execute response structure
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_processDMOResponse(uint16 p_unSrcOID, uint8 p_ucMethID, EXEC_RSP_SRVC* p_pExecRsp)
{
  switch(p_unSrcOID)
  {
    case DMAP_DMO_OBJ_ID:
      {
          if( DMO_PROXY_SEC_SYM_REQ == p_ucMethID && DEVICE_SECURITY_JOIN_REQ_SENT == g_ucJoinStatus )
          {
            //backbone or device Security Join Response
            DMO_checkSecJoinResponse(p_pExecRsp);
            break;
          }  
          if( DMO_PROXY_SM_JOIN_REQ == p_ucMethID && DEVICE_SM_JOIN_REQ_SENT == g_ucJoinStatus )
          {
            //backbone or device SM Join Response
            DMO_checkSMJoinResponse(p_pExecRsp);
            break;
          }
          if( DMO_PROXY_SM_CONTR_REQ == p_ucMethID && DEVICE_SM_CONTR_REQ_SENT == g_ucJoinStatus )
          {
            //backbone or device SM Contract Response
            DMO_ProcessFirstContract(p_pExecRsp);   
            if( DEVICE_SEND_SEC_CONFIRM_REQ == g_ucJoinStatus )
            {
                //the contract response was successfully processed
                if( SFC_SUCCESS != NLMO_updateJoinSMIBS() )
                {
                    DMAP_DLMO_ResetStack(3);
                }
                else
                {
#ifndef BACKBONE_SUPPORT  
                    //the next requests will be sent using short address
                    g_stDMO.m_unShortAddr = s_unDevShortAddr;
                    
#warning " in the future the SM will manage this kind of delays "
                    //wait for the upper levels configuration(added a Neighbor SMIB on the Advertisement Router)
                    g_ulDAUXTimeSync = MLSM_GetCrtTaiSec() + 2;  //2 second delay until the SecConfirmRequest will be sent  
#endif  
                    
                    //contract and session key available - update the TL security level
                    uint8 ucTmp;
                    if( SLME_FindTxKey(g_stDMO.m_aucSysMng128BitAddr, (uint8)UAP_SMAP_ID & 0x0F, &ucTmp ) )  // have key to SM's UAP
                    {
                        //// SetTLSecurityLevel((GetSecurityLevel() >> 2) & 0x07); 
                      //// g_stDSMO.m_ucTLSecurityLevel = SECURITY_ENC_MIC_32;

                    }
                }
            }
            break;
          }
      }
      break;
    
    case SM_PSMO_OBJ_ID:
      {
          if( PSMO_SECURITY_JOIN_REQ == p_ucMethID )
          {
            //proxy router Security Join Response
            DMAP_DMO_forwardJoinResp(p_pExecRsp);
            break;
          }
          if( PSMO_SECURITY_JOIN_CONF == p_ucMethID && DEVICE_SEC_CONFIRM_REQ_SENT == g_ucJoinStatus )
          {
            //backbone or device Security Join Confirm Response
            DMAP_DMO_applyJoinStatus(p_pExecRsp->m_ucSFC);
            break;
          }
      }
      break;
      
    case SM_DMSO_OBJ_ID:
      {
          if((DMSO_JOIN_REQUEST == p_ucMethID) ||
             (DMSO_CONTRACT_REQUEST == p_ucMethID))
          {
            //proxy router SM Join Response or SM Contract Request
            DMAP_DMO_forwardJoinResp(p_pExecRsp);            
            break;
          }
      }
      break;
      
    case SM_SCO_OBJ_ID:
      {
        if(SCO_REQ_CONTRACT == p_ucMethID)
        {
          //process contract response
          DMO_ProcessContractResponse(p_pExecRsp);
          break;
        }
        /*
        if(SCO_TERMINATE_CONTRACT == p_ucMethID)
        {
          //TODO
        }
        */
      }
      break;
      
    default:
      break;
      
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Processes an execute response in the application queue and passes it to the target object 
/// @param  p_pExecRsp - pointer to the execute response structure
/// @param  p_pAPDUIdtf - pointer to the appropriate request APDU
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_processExecuteResponse( EXEC_RSP_SRVC * p_pExecRsp,
                                  APDU_IDTF *     p_pAPDUIdtf )
{  
    uint16            unTxAPDULen;
    GENERIC_ASL_SRVC  stGenSrvc;
    
    const uint8 * pTxAPDU = ASLDE_SearchOriginalRequest( p_pExecRsp->m_ucReqID,                                                         
                                                         p_pExecRsp->m_unSrcOID,
                                                         p_pExecRsp->m_unDstOID,
                                                         p_pAPDUIdtf,                                                         
                                                         &unTxAPDULen);      
    if( NULL == pTxAPDU )
        return;
    
    if( NULL == ASLSRVC_GetGenericObject( pTxAPDU, unTxAPDULen, &stGenSrvc, NULL ) )
        return;
    
    if( SRVC_EXEC_REQ != stGenSrvc.m_ucType )
        return;
    
    if( stGenSrvc.m_stSRVC.m_stExecReq.m_unDstOID != p_pExecRsp->m_unSrcOID )
        return; 
    
    switch( p_pExecRsp->m_unDstOID )
    {
        case DMAP_DLMO_OBJ_ID  :
        case DMAP_TLMO_OBJ_ID  :
        case DMAP_NLMO_OBJ_ID  :
        case DMAP_ASLMO_OBJ_ID : break;
        
        case DMAP_DSMO_OBJ_ID  :
            if( PSMO_SEC_NEW_SESSION == stGenSrvc.m_stSRVC.m_stExecReq.m_ucMethID )
            {
                //response for the Update_Session_Key_Request not needed to be parsed 
            }
            break;
        
        case DMAP_DMO_OBJ_ID   : 
          {
            DMAP_processDMOResponse(p_pExecRsp->m_unSrcOID, 
                                    stGenSrvc.m_stSRVC.m_stExecReq.m_ucMethID,   
                                    p_pExecRsp);
          }
          break;        
        default:
          break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Updates join status on a succesfull join-confirm-response
/// @param  p_ucSFC - service feedback code
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_DMO_applyJoinStatus( uint8 p_ucSFC )
{
    if ( p_ucSFC != SFC_SUCCESS )
    {      
        DMAP_DLMO_ResetStack(4);   
        return;  
    }  
    
    g_ucJoinStatus = DEVICE_JOINED;
    MLDE_SetDiscoveryState(DEVICE_JOINED);
    UAP_OnJoin();
    
#ifndef BACKBONE_SUPPORT  
    g_unLastJoinCnt = 2 * REJOIN_ROUTER_BANNED_PERIOD;

    //clear the banned router table
    g_ucBannedRoutNo = 0;    

    //delete the unnecessary SMIBs
    DLME_DeleteSMIBRequest(DL_LINK, JOIN_REQ_TX_LINK_UID, 0);
    DLME_DeleteSMIBRequest(DL_LINK, JOIN_RESP_RX_LINK_UID, 0);
#warning  "remove the Join Graph and Route after the SM will allocate a DLL route for publishing contract"
    //DLME_DeleteSMIBRequest(DL_ROUTE, JOIN_ROUTE_UID, 0);
    //DLME_DeleteSMIBRequest(DL_GRAPH, JOIN_GRAPH_UID, 0);
#endif
}

#ifndef BACKBONE_SUPPORT 
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @author NIVIS LLC, Eduard Erdei
    /// @brief  Informs DMAP that dll has received advertise 
    /// @param  p_unRouterAddr - router address
    /// @param  p_pstTimeSyncInfo - pointer to time synchronization structure
    /// @return none
    /// @remarks
    ///      Access level: interrupt level
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void DMAP_DLMO_IndicateDAUX(  uint16 p_unRouterAddr  )
    {     
      if( !DAQ_IsReady() && (g_stFilterTargetID != 1) ) // cannot join until receive DAQ confirmation
          return;
      
      uint8 ucIdx;
      for( ucIdx = 0; ucIdx < g_ucBannedRoutNo; ucIdx++ )
      {              
          if( g_astBannedRouters[ucIdx].m_unRouterAddr == p_unRouterAddr )
              return; //wait another advertisement
      }
      
      // store the router address into the candidates list
      if (SFC_SUCCESS == MLSM_AddNewCandidate(p_unRouterAddr))  // may return also duplicate or list full
          return; // first advertise
         
      // second advertise or advertise list full
      g_unDAUXRouterAddr = p_unRouterAddr;   
      g_ulDAUXTimeSync = g_ulDllTaiSeconds;
      g_stCandidates.m_ucCandidateNo = 0;       
      
      // Stop the discovery state of the MAC sub-layer
      MLDE_SetDiscoveryState(DEVICE_RECEIVED_ADVERTISE);
        
      g_ucJoinStatus = DEVICE_RECEIVED_ADVERTISE;
    }
  
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @author NIVIS LLC, Eduard Erdei, Mircea Vlasin
    /// @brief  Adds the necessary SMIBS, based on the data received inside advertisement 
    /// @param  none
    /// @return none
    /// @remarks
    ///      Access level: user level\n
    ///      Context: used to configure the minimal communication support for the join process  
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    __monitor void DMAP_DLMO_prepareSMIBS(void)
    {
      DLMO_DOUBLE_IDX_SMIB_ENTRY stTwoIdxSMIB;
        
      MONITOR_ENTER();
                  
      // the Advertisement Superframe's UID not included inside Advertisement DAUX  
      g_stDAUXSuperframe.m_unUID = JOIN_SF;
      
      //set the highest priority for the advertisement superframe
      g_stDAUXSuperframe.m_ucInfo |= 0x3C;  //15 - maximum priority

#pragma diag_suppress=Pa039
      
      // Add the superframe to DLL SMIBs
      stTwoIdxSMIB.m_unUID = g_stDAUXSuperframe.m_unUID;
      memcpy(&stTwoIdxSMIB.m_stEntry,  &g_stDAUXSuperframe, sizeof(stTwoIdxSMIB.m_stEntry.m_stSmib.m_stSuperframe));
      DLME_SetSMIBRequest(DL_SUPERFRAME, g_ulDAUXTimeSync, &stTwoIdxSMIB);     
      DLME_ParseMsgQueue(); // avoid getting full of DLME action queue
      
      memset( &stTwoIdxSMIB.m_stEntry.m_stSmib.m_stGraph, 0, sizeof(stTwoIdxSMIB.m_stEntry.m_stSmib.m_stGraph) );
      // create an outbound graph for transmiting the Join Requests messages 
      stTwoIdxSMIB.m_unUID = JOIN_GRAPH_UID;
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stGraph.m_unUID = JOIN_GRAPH_UID;
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stGraph.m_ucNeighCount = 1;
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stGraph.m_aNeighbors[0] = g_unDAUXRouterAddr;
      DLME_SetSMIBRequest(DL_GRAPH, 0, &stTwoIdxSMIB );
      DLME_ParseMsgQueue(); // avoid getting full of DLME action queue    
  
      memset( &stTwoIdxSMIB.m_stEntry.m_stSmib.m_stRoute, 0, sizeof(stTwoIdxSMIB.m_stEntry.m_stSmib.m_stRoute) );
      // create a default route per graph, to ensure that communication is possible with the advertising router;
      stTwoIdxSMIB.m_unUID = JOIN_ROUTE_UID;
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stRoute.m_unUID      = JOIN_ROUTE_UID;
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stRoute.m_ucInfo     = DLL_RTSEL_DEFAULT << DLL_ROT_RTALT;        // use graph
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stRoute.m_ucInfo     |= 1 << DLL_ROT_RTNO;                        // source routing, one hop away
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stRoute.m_ucFwdLimit = 16;
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stRoute.m_aRoutes[0] = GRAPH_ID_PREFIX | JOIN_GRAPH_UID;      
      DLME_SetSMIBRequest(DL_ROUTE, 0, &stTwoIdxSMIB );
      DLME_ParseMsgQueue(); // avoid getting full of DLME action queue
      
      
      memset( &stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink, 0, sizeof(stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink) );
      // create a tx link towards the advertising router
      stTwoIdxSMIB.m_unUID                                     = JOIN_REQ_TX_LINK_UID;
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_unUID         = JOIN_REQ_TX_LINK_UID;
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_unSfIdx       = g_stDAUXSuperframe.m_unUID;
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_mfLinkType    = DLL_MASK_LNKTX | DLL_MASK_LNKEXP;      // tx link
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_mfTypeOptions = 0x01 << DLL_ROT_LNKNEI;                // designates an address
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_unNeighbor    = g_unDAUXRouterAddr;
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_unTemplate1   = DEFAULT_TX_TEMPL_UID;                           
      // add the join info
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_mfTypeOptions |= ((g_stDllAdvJoinInfo.m_mfDllJoinLinksType & DLL_MASK_JOIN_TX_SCHEDULE) >> DLL_ROT_JOIN_TX_SCHD) << DLL_ROT_LNKSC;
      memcpy(&stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_aSchedule, 
             &g_stDllAdvJoinInfo.m_stDllJoinTx, 
             sizeof(DLL_SCHEDULE));
      
      DLME_SetSMIBRequest(DL_LINK, 0, &stTwoIdxSMIB );
      DLME_ParseMsgQueue(); // avoid getting full of DLME action queue
      
      // create an rx link from the advertising router
      // TAI cutover remains the same
      stTwoIdxSMIB.m_unUID                                   = JOIN_RESP_RX_LINK_UID;
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_unUID       = JOIN_RESP_RX_LINK_UID;
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_mfLinkType  = DLL_MASK_LNKRX;      // rx link
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_unTemplate1 = DEFAULT_RX_TEMPL_UID;  
      // add the join info
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_mfTypeOptions = 
          ((g_stDllAdvJoinInfo.m_mfDllJoinLinksType & DLL_MASK_JOIN_RX_SCHEDULE) >> DLL_ROT_JOIN_RX_SCHD) << DLL_ROT_LNKSC;
      
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_mfTypeOptions |= DLL_MASK_LNKPR; // explicit link priority
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_ucPriority = 0x01;
      
      memcpy(&stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_aSchedule, 
             &g_stDllAdvJoinInfo.m_stDllJoinRx, 
             sizeof(DLL_SCHEDULE));
      
      DLME_SetSMIBRequest(DL_LINK, 0, &stTwoIdxSMIB );
      DLME_ParseMsgQueue(); // avoid getting full of DLME action queue
    
      if( g_stDllAdvJoinInfo.m_mfDllJoinLinksType & DLL_MASK_ADV_RX_FLAG ) 
      {
          // create an rx link from the advertising router, link for receiving advertise
          // TAI cutover remains the same
          stTwoIdxSMIB.m_unUID                                   = JOIN_ADV_RX_LINK_UID;
          stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_unUID       = JOIN_ADV_RX_LINK_UID;
          stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_mfLinkType  = DLL_MASK_LNKRX;     // rx link
          stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_unTemplate1 = DEFAULT_TX_TEMPL_UID + 1;//DEFAULT_RX_TEMPL_UID;
          // add the join info
          stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_mfTypeOptions = 
              ((g_stDllAdvJoinInfo.m_mfDllJoinLinksType & DLL_MASK_ADV_RX_SCHEDULE) >> DLL_ROT_ADV_RX_SCHD) << DLL_ROT_LNKSC;
          
          stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_mfTypeOptions |= DLL_MASK_LNKPR; // explicit link priority
          stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_ucPriority = 0x01;
          
          memcpy(&stTwoIdxSMIB.m_stEntry.m_stSmib.m_stLink.m_aSchedule, 
                 &g_stDllAdvJoinInfo.m_stDllJoinAdvRx, 
                 sizeof(DLL_SCHEDULE));
          
          DLME_SetSMIBRequest(DL_LINK, 0, &stTwoIdxSMIB);
          DLME_ParseMsgQueue(); // avoid getting full of DLME action queue
      }
    
      memset( &stTwoIdxSMIB.m_stEntry.m_stSmib.m_stNeighbor, 0, sizeof(stTwoIdxSMIB.m_stEntry.m_stSmib.m_stNeighbor) );
      // add the router in the neighbor table
      stTwoIdxSMIB.m_unUID                                  = g_unDAUXRouterAddr;
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stNeighbor.m_unUID  = g_unDAUXRouterAddr;;
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stNeighbor.m_ucInfo = 0x80 | DLL_MASK_NEIDIAG_LINK;   // set as preffered clock source
      stTwoIdxSMIB.m_stEntry.m_stSmib.m_stNeighbor.m_ucCommHistory = 0xFF;
      DLME_SetSMIBRequest(DL_NEIGH, 0, &stTwoIdxSMIB );
      DLME_ParseMsgQueue(); // avoid getting full of DLME action queue
      
#pragma diag_default=Pa039
      
      MONITOR_EXIT();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @author NIVIS LLC, Eduard Erdei
    /// @brief  Informs DMAP that dll has received advertise 
    /// @param  p_unRouterAddr - router address
    /// @param  p_pstTimeSyncInfo - pointer to time synchronization structure
    /// @return none
    /// @remarks
    ///      Access level: user level
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void DMAP_CheckNeighborLink(void)
    {
        if (g_stTAI.m_ucClkInterogationStatus == MLSM_CLK_INTEROGATION_TIMEOUT)
        {
            DMAP_DLMO_ResetStack(5);
        }            
    }

#else   //#ifndef BACKBONE_SUPPORT
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei, Mircea Vlasin
/// @brief  Checks the link with the System Manager, based on a SM's response
/// @param  none
/// @return none
/// @remarks
///      Access level: user level\n
///      Context: if no SM response during SM_LINK_RETRY_TIMEOUT seconds reset the stack
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_DMO_CheckSMLink(void)
{
}
#endif // #ifndef BACKBONE_SUPPORT

#define CONTRACT_BUF_LEN    53
#define ATT_BUF_LEN         34
#define ROUTE_BUF_LEN       50
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Prepares indexed attributes needed for the join process
/// @param  none
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 NLMO_prepareJoinSMIBS(void)
{

#ifndef BACKBONE_SUPPORT
    DLMO_SMIB_ENTRY stEntry;
    // search neighbor table EUI64 address of router
    if( SFC_SUCCESS == DLME_GetSMIBRequest( DL_NEIGH, g_unDAUXRouterAddr, &stEntry) )
    {
        if (memcmp(stEntry.m_stSmib.m_stNeighbor.m_aEUI64,
                   c_aucInvalidEUI64Addr, 
                   sizeof(c_aucInvalidEUI64Addr)))
        {
            
            // add a contract to the advertising router
            uint8 aucTmpBuf[CONTRACT_BUF_LEN];
            aucTmpBuf[0] = (uint8)(JOIN_CONTRACT_ID) >> 8;
            aucTmpBuf[1] = (uint8)(JOIN_CONTRACT_ID);      
            memcpy(aucTmpBuf + 2, c_aLocalLinkIpv6Prefix, 8);
            memcpy(aucTmpBuf + 10, c_oEUI64BE, 8);
            //double the index
            memcpy(aucTmpBuf + 18, aucTmpBuf, 18);
            
            memcpy(aucTmpBuf + 36, c_aLocalLinkIpv6Prefix, 8);  
            DLME_CopyReversedEUI64Addr(aucTmpBuf + 44, stEntry.m_stSmib.m_stNeighbor.m_aEUI64);
            
            aucTmpBuf[52] = 0x03; // b1b0 = priority; b3= include contract flag;
            
            NLME_SetRow( NLME_ATRBT_ID_CONTRACT_TABLE, 
                        0, 
                        aucTmpBuf,
                        CONTRACT_BUF_LEN );   
            
            
            // add the address translation for the advertising router
            memcpy(aucTmpBuf, c_aLocalLinkIpv6Prefix, 8);
            DLME_CopyReversedEUI64Addr(aucTmpBuf + 8, stEntry.m_stSmib.m_stNeighbor.m_aEUI64);
            memcpy(aucTmpBuf + 16, aucTmpBuf, 16);
            aucTmpBuf[32] = g_unDAUXRouterAddr >> 8;
            aucTmpBuf[33] = g_unDAUXRouterAddr;
            
            NLME_SetRow( NLME_ATRBT_ID_ATT_TABLE, 
                        0, 
                        aucTmpBuf,
                        ATT_BUF_LEN );
            
            return SFC_SUCCESS;
        }
    }
    return SFC_FAILURE;
 #else
    //add a contract to the SM
    uint8 aucTmpBuf[CONTRACT_BUF_LEN];
    aucTmpBuf[0] = (uint8)(JOIN_CONTRACT_ID) >> 8;
    aucTmpBuf[1] = (uint8)(JOIN_CONTRACT_ID);      
    memcpy(aucTmpBuf + 2, g_stDMO.m_auc128BitAddr, 16);
    //double the index
    memcpy(aucTmpBuf + 18, aucTmpBuf, 18);
    
    memcpy(aucTmpBuf + 36, g_stDMO.m_aucSysMng128BitAddr, 16);  
    aucTmpBuf[52] = 0x03; // b1b0 = priority; b3= include contract flag;
    
    NLME_SetRow( NLME_ATRBT_ID_CONTRACT_TABLE, 
                0, 
                aucTmpBuf,
                CONTRACT_BUF_LEN );   
    
    //add a route to the SM
    memcpy(aucTmpBuf, g_stDMO.m_aucSysMng128BitAddr, 16);
    //double the index
    memcpy(aucTmpBuf + 16, g_stDMO.m_aucSysMng128BitAddr, 16);
    memcpy(aucTmpBuf + 32, g_stDMO.m_aucSysMng128BitAddr, 16);
    aucTmpBuf[48] = 1;    // m_ucNWK_HopLimit = 1
    aucTmpBuf[49] = 1;    // m_bOutgoingInterface = 1
    
    NLME_SetRow( NLME_ATRBT_ID_ROUTE_TABLE, 
                0, 
                aucTmpBuf,
                ROUTE_BUF_LEN); 
    
    return SFC_SUCCESS;
 #endif   
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Updates indexed attributes for the join process
/// @param  none
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 NLMO_updateJoinSMIBS(void)
{
uint8 aucTmpBuf[CONTRACT_BUF_LEN];    //the maximum size for the used buffer

#ifndef BACKBONE_SUPPORT
    //delete the ATT for advertiser router
    DLMO_SMIB_ENTRY stEntry;
    memcpy(aucTmpBuf, c_aLocalLinkIpv6Prefix, 8);
    
    // search neighbor table EUI64 address of router
    if( SFC_SUCCESS == DLME_GetSMIBRequest(DL_NEIGH, g_unDAUXRouterAddr, &stEntry) )
    {
        DLME_CopyReversedEUI64Addr(aucTmpBuf + 8, stEntry.m_stSmib.m_stNeighbor.m_aEUI64);
        NLME_DeleteRow(NLME_ATRBT_ID_ATT_TABLE, 0, aucTmpBuf, 16 );
    }
#endif

    //for the BBR no needed to change the existing route to SM  
    
    //delete the fake join contract
    //the real contract with the SM was added when JoinContractResponse was proceeded
    aucTmpBuf[0] = (uint8)(JOIN_CONTRACT_ID) >> 8;
    aucTmpBuf[1] = (uint8)(JOIN_CONTRACT_ID);      
    memcpy(aucTmpBuf + 2, c_aLocalLinkIpv6Prefix, 8);
    memcpy(aucTmpBuf + 10, c_oEUI64BE, 8);
    NLME_DeleteRow(NLME_ATRBT_ID_CONTRACT_TABLE, 0, aucTmpBuf, 18 );

    //add the real ATT table with SM
    memcpy(aucTmpBuf, g_stDMO.m_aucSysMng128BitAddr, 16);
    memcpy(aucTmpBuf + 16, aucTmpBuf, 16);
    aucTmpBuf[32] = g_stDMO.m_unSysMngShortAddr >> 8;
    aucTmpBuf[33] = (uint8)g_stDMO.m_unSysMngShortAddr;
            
    return NLME_SetRow( NLME_ATRBT_ID_ATT_TABLE, 
                        0, 
                        aucTmpBuf,
                        ATT_BUF_LEN );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC
/// @brief  Reinitializes all device layers and sets the device on discovery state
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
__monitor void DMAP_DLMO_ResetStack( uint8 p_ucReason )
{
  //PROVISION_Init();
  ETH_Init();
  UDP_Init();
  ClearListMacOfIP();
  
  MONITOR_ENTER();
 // PHY_INIT(1);
  PHY_Disable_Request();
  ASLDE_Init();
  DLME_Init(); 
  TLME_Init();
  NLDE_Init();
  SLME_Init();  
  DLL_Init();
  
  DMAP_Init();  

  MLDE_SetDiscoveryState(DEVICE_DISCOVERY);
  
  UAP_OnJoinReset();
  

#ifndef g_ucProvisioned  
  g_ucProvisioned = FALSE;
#endif  

  MONITOR_EXIT();  
  
// reset NTP timer  
#warning Check eth.c code around g_unNTPReqCounter
  g_unNTPReqCounter = 0;
  
#ifndef RESET_TO_FACTORY
  WCITXT_TX_MSG stMsg;
  
  stMsg.m_ucMsgType = WCITXT_MSG_TO_BBR_AS_ERR;
  stMsg.m_u.m_stErr.m_ucSeverity = LOG_INFO;
  stMsg.m_u.m_stErr.m_ucLevel = LOG_M_APP;
  stMsg.m_u.m_stErr.m_ucErrorCode = 0;
  stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Stack reset [%u]", (unsigned long)p_ucReason );

  // add to queue
  WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg );  
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Checks if backof counter has reached target value
/// @param  none
/// @return none
/// @remarks
///      Access level: user level

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Checks the timeouts based on second accuracy 
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_CheckSecondCounter(void)
{
#ifndef BACKBONE_SUPPORT
  UpdateBannedRouterStatus();
  
  DMAP_CheckNeighborLink();
  
  if( g_unLastJoinCnt )
      g_unLastJoinCnt--;
#endif
  
  if( g_ulRadioSleepCounter )
      g_ulRadioSleepCounter--;
  
  if( g_stJoinRetryCntrl.m_unJoinTimeout )  
      g_stJoinRetryCntrl.m_unJoinTimeout--;

  if( UDO_STATE_APPLYING == g_ucUDOState )
  {
    if( g_ulUDOCutoverTime <= MLSM_GetCrtTaiSec() )
    {
#if   (DEVICE_TYPE == DEV_TYPE_MC13225) 
   #warning  "TODO: DMAP_CheckSecondCounter activate the new FW"   
//CallBootl trebuie sa faca o modificare in flash intern care sa indice 
//firmware-le activ in eeprom pentru ca bootloaderul ruleaza la fiecare start al
//crow-ului si incarca fnctie de jmperul de isa/hart
    CallBootl();
    //DeviceReset();
            
#elif (DEVICE_TYPE == DEV_TYPE_AP91SAM7X512)        
      //TBD: activate the new FW 
      SPI_FlushEEPROM();    //just in case  
      CallBootl();
#elif (DEVICE_TYPE == DEV_TYPE_MSP430F2618)
      
      SPI_ReadEEPROM( "", 0, 0 ); // perfrom flush eeprom
      SPI1_RequestEEPROMAccess();
       WDT_Disable();
       __disable_interrupt();
      //give control to the Boot Loader module - the address of start is stored at 0x4000
      PF_SWAP pfSwapEntry = (PF_SWAP)(*(uint16*)SWAP_ENTRY_POINT_ADDR);
      pfSwapEntry();
      
#else 
    #warning "unknown device:activate the new FW discarded"
#endif    
	}
#ifdef BACKBONE_SUPPORT
    else
    {
      //be sure that SM Link timeout will not cancel the firmware upgrade process
      if(!g_ucSMLinkTimeout)
        g_ucSMLinkTimeout = SM_LINK_TIMEOUT;
    }
  }
  DMAP_DMO_CheckSMLink();
#else
  }
#endif  //#ifdef BACKBONE_SUPPORT
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Generates a Security join request buffer
/// @param  p_pucBuf - the request buffer
/// @return data size
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint16 DMO_generateSecJoinReqBuffer(uint8* p_pucBuf)
{
    //TO DO - need to generate the unique device challenge  

#if  (DEVICE_TYPE != DEV_TYPE_MC13225)               
    ((uint16*)g_aulNewNodeChallenge)[0] =  g_unRandValue;
    ((uint16*)g_aulNewNodeChallenge)[1] =  rand();
    ((uint16*)g_aulNewNodeChallenge)[2] =  TMR_Get250msOffset();
#else
    g_aulNewNodeChallenge[0] =  g_unRandValue;
    g_aulNewNodeChallenge[1] =  g_unRandValue;
#endif
/*
    g_aulNewNodeChallenge[2] =  MLSM_GetCrtTaiSec();
    
    memcpy(p_pucBuf, c_oEUI64BE, 8);
  
    memcpy(p_pucBuf + 8, g_aulNewNodeChallenge, 16);
    
    //add the Key Info field
    #warning "need to be specified the content coding of the Key_Info"
    p_pucBuf[24] = 0x00;
    
    //add the algorihm identifier field
    p_pucBuf[25] = 0x01;     //only the AES_CCM symmetric key algorithm must used 
                             //b7..b4 - public key algorithm and options
                             //b3..b0 - symmetric key algorithm and mode
    
    AES_Crypt_User(g_aJoinAppKey, (uint8*)g_aulNewNodeChallenge, p_pucBuf, 26, p_pucBuf + 26, 0);
    
    return (26 + 4);
    */
    
        g_aulNewNodeChallenge[2] =  MLSM_GetCrtTaiSec();
    memcpy(p_pucBuf, c_oEUI64BE, 8);                    // EUI 64
     
    p_pucBuf[8]    = g_unDllSubnetId >> 8;
    p_pucBuf[9]    = g_unDllSubnetId;
    p_pucBuf[10]   =  c_ucCommSWMajorVer;
    p_pucBuf[11]   =  c_ucCommSWMinorVer;

    memcpy(p_pucBuf + 12, g_aulNewNodeChallenge, 16);
    
    //add the algorihm identifier field
    p_pucBuf[28] = 0x01;     //only the AES_CCM symmetric key algorithm must used 
                             //b7..b4 - public key algorithm and options
                             //b3..b0 - symmetric key algorithm and mode
    AES_Crypt_User(g_aJoinAppKey, (uint8*)g_aulNewNodeChallenge, p_pucBuf, 25+4, p_pucBuf + 25+4, 0, MIC_SIZE);
    return (25 + 4 + 4);
    
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Validates join security response
/// @param  p_pExecRsp - pointer to the response service structure 
/// @return None
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMO_checkSecJoinResponse(EXEC_RSP_SRVC* p_pExecRsp)
{
    if( SFC_VALUE_LIMITED == p_pExecRsp->m_ucSFC )
    {
        //the advertisement router not support another device to join
        g_stJoinRetryCntrl.m_unJoinTimeout = 0;     //force rejoin process
        return;
    }
    
    if( SFC_SUCCESS != p_pExecRsp->m_ucSFC )
        return;     //wait timeout then rejoin
    
    if( p_pExecRsp->m_unLen < SEC_JOIN_RESP_SIZE )
        return;
    
    uint8 aucHashEntry[MAX_HMAC_INPUT_SIZE];
    uint8 CombinedSecLevel =0;
    
    memcpy(aucHashEntry, p_pExecRsp->p_pRspData, 16);       //Security Manager Challenge
    memcpy(aucHashEntry + 16, g_aulNewNodeChallenge, 16);   //New Device Challenge
    memcpy(aucHashEntry + 32, c_oEUI64BE, 8);
    memcpy(aucHashEntry + 40, c_oSecManagerEUI64BE, 8);
    memcpy(aucHashEntry + 48, p_pExecRsp->p_pRspData + 32, SEC_JOIN_RESP_SIZE - 32);
    
    if( SFC_SUCCESS == Keyed_Hash_MAC(g_aJoinAppKey, aucHashEntry, MAX_HMAC_INPUT_SIZE) )
    {
        //validate Hash_B field
        if( !memcmp(p_pExecRsp->p_pRspData + 16, aucHashEntry, 16) )
        {
            //retain the System Manager Challenge
            memcpy(g_aucSecMngChallenge, p_pExecRsp->p_pRspData, 16);
            
            //compute the Master Key
    #warning " for Hash_B field validation the Device Challenge can be placed before SM Challenge? "
            memcpy(aucHashEntry, g_aulNewNodeChallenge, 16);        //New Device Challenge
            memcpy(aucHashEntry + 16, p_pExecRsp->p_pRspData, 16);  //Security Manager Challenge
            //the Device and SecMngr EUI64s not overwrited inside "aucHashEntry"
        
            Keyed_Hash_MAC(g_aJoinAppKey, aucHashEntry, 48);    //the first 16 bytes from "aucHashEntry" represent the Master Key
        
            //decrypt the DLL Key using the Master Key
#warning " not specified the nonce generation "
            //MIC is not transmitted inside response - Encryption with MIC_SIZE = 4
            uint8 ucEncKeyOffset = SEC_JOIN_RESP_SIZE - 32;
            
            //for pure decryption no result validation needed 
            AES_Decrypt_User( aucHashEntry,              //Master_Key - first 16 bytes
                              (uint8*) g_aulNewNodeChallenge,     //first 13 bytes 
                              NULL, 0,                   // No authentication
                              p_pExecRsp->p_pRspData + ucEncKeyOffset,
                              32 + MIC_SIZE, MIC_SIZE);
            
            CombinedSecLevel = *(p_pExecRsp->p_pRspData + 32);
            //add the DLL
            uint8* pucKeyPolicy = p_pExecRsp->p_pRspData + ucEncKeyOffset - 7;  //2*3 bytes(Master Key, DLL and Session key policies) + 1 byte(DLL KeyID) 
            uint32 ulLifeTime;
            
            //add the Master Key
            ulLifeTime = (((uint16)(pucKeyPolicy[0])) << 8) | pucKeyPolicy[1];
            ulLifeTime *= 1800;
            
            SLME_SetKey(    NULL, // p_pucPeerIPv6Address, 
                            0,  // p_ucUdpPorts,
                            0,  // p_ucKeyID  - hardcoded
                            aucHashEntry,  // p_pucKey, 
                            c_oSecManagerEUI64BE,  // p_pucIssuerEUI64, 
                            0, // p_ulValidNotBefore
                            0, // ulLifeTime, // p_ulSoftLifetime,
                            0, // ulLifeTime*2, // p_ulHardLifetime,
                            SLM_KEY_USAGE_MASTER, // p_ucUsage, 
                            0, // p_ucPolicy -> need correct policy
                        CombinedSecLevel & 0x03);
            
            //add the DLL Key
            ulLifeTime = (((uint16)(pucKeyPolicy[2])) << 8) | pucKeyPolicy[3];
            ulLifeTime *= 1800;
            SLME_SetKey(    NULL, // p_pucPeerIPv6Address, 
                            0,  // p_ucUdpPorts,
                            pucKeyPolicy[6],  // p_ucKeyID,
                            pucKeyPolicy + 7,  // p_pucKey, 
                            NULL,  // p_pucIssuerEUI64, 
                            0, // p_ulValidNotBefore
                            ulLifeTime, // p_ulSoftLifetime,
                            ulLifeTime*2, // p_ulHardLifetime,
                            SLM_KEY_USAGE_DLL, // p_ucUsage, 
                        0, // p_ucPolicy -> need correct policy,
                        (CombinedSecLevel >> 5) & 0x07);
            
            //add the SM Session key
            const uint8 * pKey =  pucKeyPolicy + 7 + 16;
            memset( aucHashEntry, 0, 16 ); 
            ulLifeTime = ((uint16)(pucKeyPolicy[4])) << 8 | pucKeyPolicy[5];
        //if( ulLifeTime || memcmp( pKey, aucHashEntry, 16 ) ) // TL encrypted if ulLifeTime || key <> 0
                ulLifeTime *= 1800;                
                
                SLME_SetKey(    g_stDMO.m_aucSysMng128BitAddr, // p_pucPeerIPv6Address, 
                                (uint8)UAP_SMAP_ID & 0x0F,  // p_ucUdpPorts,   //dest port - SM's SM_UAP 
                                0,  // p_ucKeyID,
                                pKey,  // p_pucKey, 
                                NULL,  // p_pucIssuerEUI64, 
                                0, // p_ulValidNotBefore
                                ulLifeTime, // p_ulSoftLifetime,
                                ulLifeTime*2, // p_ulHardLifetime,
                                SLM_KEY_USAGE_SESSION, // p_ucUsage, 
                                0, // p_ucPolicy -> need correct policy
                            (CombinedSecLevel >> 2) & 0x07);
        g_ucJoinStatus = DEVICE_SEND_SM_JOIN_REQ;
    }
  }    
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Generates a System Manager join request buffer
/// @param  p_pBuf - the request buffer
/// @return data size
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint16 DMO_generateSMJoinReqBuffer(uint8* p_pBuf)
{
  uint8* pucTemp = p_pBuf;
  uint8 ucStrLen = 0;
  
  memcpy(p_pBuf, c_oEUI64BE, 8);
  p_pBuf += 8;

  *(p_pBuf++) = g_unDllSubnetId >> 8;
  *(p_pBuf++) = g_unDllSubnetId;
  
  //Device Role Capability
  *(p_pBuf++) = g_stDPO.m_unDeviceRole >> 8;
  *(p_pBuf++) = g_stDPO.m_unDeviceRole;  
  
  *(p_pBuf++) = sizeof(g_stDMO.m_aucTagName);
  memcpy(p_pBuf, g_stDMO.m_aucTagName, sizeof(g_stDMO.m_aucTagName));  
  p_pBuf += sizeof(g_stDMO.m_aucTagName);
  
  *(p_pBuf++) = c_ucCommSWMajorVer;
  *(p_pBuf++) = c_ucCommSWMinorVer;
  
  ucStrLen = DMAP_GetVisibleStringLength( c_aucSWRevInfo, sizeof(c_aucSWRevInfo) );
  *(p_pBuf++) = ucStrLen;
  memcpy(p_pBuf, c_aucSWRevInfo, ucStrLen );  
  p_pBuf += ucStrLen;

// when Device Capability inside the SM Join Request
  uint16 unSize;
  DLMO_ReadDeviceCapability(DL_DEV_CAPABILITY, &unSize, p_pBuf); 
  p_pBuf += unSize;    
 
  *(p_pBuf++) = 0;  //DL_CONSTRAINTS;
   return p_pBuf - pucTemp;
  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Validates join SM response
/// @param  p_pExecRsp - pointer to the response service structure 
/// @return None
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMO_checkSMJoinResponse(EXEC_RSP_SRVC* p_pExecRsp)
{
    //TO DO - wait specs to clarify if authentication with MIC32 needed    
    
    if( SFC_VALUE_LIMITED == p_pExecRsp->m_ucSFC )
    {
        //the advertisement router not support another device to join
        g_stJoinRetryCntrl.m_unJoinTimeout = 0;     //force rejoin process
        return;
    }
    
    if( SFC_SUCCESS != p_pExecRsp->m_ucSFC )
        return; //wait timeout then rejoin
        
    if( p_pExecRsp->m_unLen < 46 )
        return;
    
    memcpy(g_stDMO.m_auc128BitAddr, p_pExecRsp->p_pRspData, 16);
    p_pExecRsp->p_pRspData += 16;
    
#ifndef BACKBONE_SUPPORT
    //the device short address will be updated later 
    s_unDevShortAddr = (uint16)p_pExecRsp->p_pRspData[0] << 8 | p_pExecRsp->p_pRspData[1];
#else
    g_stDMO.m_unShortAddr = (uint16)p_pExecRsp->p_pRspData[0] << 8 | p_pExecRsp->p_pRspData[1];
#endif    
    g_stDMO.m_unAssignedDevRole = (uint16)p_pExecRsp->p_pRspData[2] << 8 | p_pExecRsp->p_pRspData[3];
    p_pExecRsp->p_pRspData += 4;
    
    memcpy(g_stDMO.m_aucSysMng128BitAddr, p_pExecRsp->p_pRspData, 16);
    p_pExecRsp->p_pRspData += 16;
    
    g_stDMO.m_unSysMngShortAddr = (uint16)p_pExecRsp->p_pRspData[0] << 8 | p_pExecRsp->p_pRspData[1];
    p_pExecRsp->p_pRspData += 2;
    
    memcpy(g_stDMO.m_unSysMngEUI64, p_pExecRsp->p_pRspData, 8);

    //update the SM Session Key's issuer
    SLME_UpdateJoinSessionsKeys( g_stDMO.m_unSysMngEUI64, g_stDMO.m_aucSysMng128BitAddr );
    
    g_ucJoinStatus = DEVICE_SEND_SM_CONTR_REQ;
}


/*===========================[ DMO implementation ]==========================*/
#if defined(ROUTING_SUPPORT) || defined(BACKBONE_SUPPORT)
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Advertising router's DMO generates a join request to the DSMO
/// @param  p_pExecReq - execute request structure
/// @param  p_pdtf - APDU structure
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 DMAP_DMO_forwardJoinReq( EXEC_REQ_SRVC * p_pExecReq,
                               APDU_IDTF *     p_pIdtf)
{
    if ( ASLDE_GetAPDUFreeSpace() > MIN_JOIN_FREE_SPACE 
        && (DEVICE_JOINED == g_ucJoinStatus) 
            && g_ucDevInfoEntryNo < MAX_SIMULTANEOUS_JOIN_NO ) 
    {  
        EXEC_REQ_SRVC stFwExecReq; 
        
        
        switch( p_pExecReq->m_ucMethID )
        {
            case DMO_PROXY_SEC_SYM_REQ:
                //for forward security join request
                stFwExecReq.m_unDstOID = SM_PSMO_OBJ_ID;
                stFwExecReq.m_ucMethID = PSMO_SECURITY_JOIN_REQ;
                break;
            case DMO_PROXY_SM_JOIN_REQ:
                stFwExecReq.m_unDstOID = SM_DMSO_OBJ_ID;
                stFwExecReq.m_ucMethID = DMSO_JOIN_REQUEST;
                break;
            case DMO_PROXY_SM_CONTR_REQ:
                stFwExecReq.m_unDstOID = SM_DMSO_OBJ_ID;
                stFwExecReq.m_ucMethID = DMSO_CONTRACT_REQUEST;
                break;
        }
        
        stFwExecReq.m_unSrcOID = DMAP_DMO_OBJ_ID;
        stFwExecReq.p_pReqData = p_pExecReq->p_pReqData;
        stFwExecReq.m_unLen    = p_pExecReq->m_unLen;      
        
        if (SFC_SUCCESS == ASLSRVC_AddGenericObject( &stFwExecReq,
                                                    SRVC_EXEC_REQ,
                                                    0,               // priority
                                                    UAP_DMAP_ID,     // SrcTSAPID
                                                    UAP_SMAP_ID,     // DstTSAPID
                                                    0,
                                                    NULL,            // EUI64 address
                                                    g_unSysMngContractID,
                                                    0 ) // p_unBinSize
            )
        {
            uint8 ucFwReqID = ASLSRVC_GetLastReqID();
            
            DMAP_DMO_saveNewDevJoinInfo(p_pIdtf, 
                                        p_pExecReq->m_ucReqID,
                                        ucFwReqID); 
            return SFC_SUCCESS;
        }
    }
    
    return SFC_INSUFFICIENT_DEVICE_RESOURCES;      
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Forwards a join response to a new joining device
/// @param  p_pExecRsp - pointer to the execute response
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_DMO_forwardJoinResp ( EXEC_RSP_SRVC * p_pExecRsp )
{
  uint8  aEUI64[8];
  uint8  ucPriority;
  uint8  ucOriginalReqID;
  
  if ( SFC_SUCCESS == DMAP_DMO_getNewDevJoinInfo( p_pExecRsp->m_ucReqID, aEUI64, &ucPriority, &ucOriginalReqID ) )
  {      
      // to save stack space, alter the SM response instead of declaring a new execute response
      p_pExecRsp->m_ucReqID  = ucOriginalReqID;
      p_pExecRsp->m_unSrcOID = DMAP_DMO_OBJ_ID;
      p_pExecRsp->m_unDstOID = DMAP_DMO_OBJ_ID;  
      
      ASLSRVC_AddGenericObject( p_pExecRsp,
                                SRVC_EXEC_RSP,
                                ucPriority,
                                UAP_DMAP_ID,       // SrcTSAPID
                                UAP_DMAP_ID,       // DstTSAPID
                                0,
                                aEUI64,  // dest EUI64 addr
                                0,                      // no contract
                                0  );     // p_unBinSize
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Stores data of a new device that attempts a join request through this router
/// @param  p_pIdtf - APDU structure
/// @param  p_ucReqID - request identifier
/// @param  p_ucFwReqID - forward request identifier
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_DMO_saveNewDevJoinInfo( APDU_IDTF * p_pIdtf,
                                  uint8       p_ucReqID,
                                  uint8       p_ucFwReqID)
{
  // this check is done also outside the function
  if ( g_ucDevInfoEntryNo >= (uint8)MAX_SIMULTANEOUS_JOIN_NO )
      return;      
  
  g_stDevInfo[g_ucDevInfoEntryNo].m_ucJoinReqID = p_ucReqID;
  g_stDevInfo[g_ucDevInfoEntryNo].m_ucFwReqID   = p_ucFwReqID;
  g_stDevInfo[g_ucDevInfoEntryNo].m_ucPriority  = p_pIdtf->m_ucPriorityAndFlags;
  g_stDevInfo[g_ucDevInfoEntryNo].m_ucTTL       = APP_QUE_TTL_SEC;
  
  memcpy(g_stDevInfo[g_ucDevInfoEntryNo].m_aucAddr,
         p_pIdtf->m_aucAddr,
         8);
  
  g_ucDevInfoEntryNo++;         
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Retrieves new device's join information
/// @param  p_ucFwReqID - forward request identifier
/// @param  p_pEUI64 - EUI64 address
/// @param  p_pPriority - priority
/// @param  p_pReqID - request identifier
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
  static uint8 DMAP_DMO_getNewDevJoinInfo( uint8   p_ucFwReqID,
                                           uint8 * p_pEUI64,
                                           uint8 * p_pPriority,
                                           uint8 * p_pReqID )
{  
  for (uint8 ucIdx = 0; ucIdx < g_ucDevInfoEntryNo; ucIdx++)
  {
      if ( p_ucFwReqID == g_stDevInfo[ucIdx].m_ucFwReqID )
      {
          memcpy(p_pEUI64, g_stDevInfo[ucIdx].m_aucAddr, 8);
          
          *p_pPriority = g_stDevInfo[ucIdx].m_ucPriority;                    
          *p_pReqID = g_stDevInfo[ucIdx].m_ucJoinReqID;          
          
          g_ucDevInfoEntryNo--;
          
          memmove(g_stDevInfo+ucIdx,
                  g_stDevInfo+ucIdx+1,
                  (g_ucDevInfoEntryNo-ucIdx) * sizeof(*g_stDevInfo)) ;
          
          
          return SFC_SUCCESS;
      }
  }
  
  return SFC_ELEMENT_NOT_FOUND;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Checks new device's time to live and removes it if time expired
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_DMO_CheckNewDevInfoTTL(void)
{  
  for (uint8 ucIdx = 0; ucIdx < g_ucDevInfoEntryNo; ucIdx++)
  {
      if ( g_stDevInfo[ucIdx].m_ucTTL )
      {
          g_stDevInfo[ucIdx].m_ucTTL--;
      }
      else        
      {          
          g_ucDevInfoEntryNo--;
          
          memmove(g_stDevInfo+ucIdx,
                  g_stDevInfo+ucIdx+1,
                  (g_ucDevInfoEntryNo-ucIdx) * sizeof(*g_stDevInfo)) ;
          
      }
  }
}
 
#endif // ROUTING_SUPPORT || BACKBONE_SUPPORT


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Initiates a join request for a non routing device
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_startJoin(void)
{
  DLL_MIB_ADV_JOIN_INFO stJoinInfo;  
  
  if( SFC_SUCCESS != DLME_GetMIBRequest(DL_ADV_JOIN_INFO, &stJoinInfo) )
      return;
  
  EXEC_REQ_SRVC stExecReq;
  uint8         aucReqParams[MAX_PARAM_SIZE]; // todo: check this size
    
  stExecReq.m_unSrcOID = DMAP_DMO_OBJ_ID;
  stExecReq.m_unDstOID = DMAP_DMO_OBJ_ID;
  stExecReq.m_ucMethID = DMO_PROXY_SEC_SYM_REQ;  
  stExecReq.p_pReqData = aucReqParams;
  stExecReq.m_unLen    = DMO_generateSecJoinReqBuffer( stExecReq.p_pReqData );    
   
  ASLSRVC_AddGenericObject(  &stExecReq,
                             SRVC_EXEC_REQ,
                             0,                 // priority
                             UAP_DMAP_ID,   // SrcTSAPID
                             UAP_DMAP_ID,   // DstTSAPID
                             0,
                             NULL,              // dest EUI64 address
                             JOIN_CONTRACT_ID,   // ContractID
                             0  ); // p_unBinSize

  g_stJoinRetryCntrl.m_unJoinTimeout = 1 << (stJoinInfo.m_mfDllJoinBackTimeout & 0x0F);  
  g_ucJoinStatus = DEVICE_SECURITY_JOIN_REQ_SENT;  
}

//////////////////////////////////////TLMO Object///////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Executes a transport layer management object method
/// @param  p_unMethID  - method identifier
/// @param  p_unReqSize - request buffer size
/// @param  p_pReqBuf   - request buffer containing method parameters
/// @param  p_pRspSize  - pointer to uint16 where to pu response size
/// @param  p_pRspBuf  - pointer to response buffer (otput data)
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 TLMO_execute(uint8    p_ucMethID,
                   uint16   p_unReqSize,
                   uint8 *  p_pReqBuf,
                   uint16 * p_pRspSize,
                   uint8 *  p_pRspBuf)
{
  uint8 ucSFC = SFC_SUCCESS; 
  *p_pRspSize = 0;
  
  uint8 * pStart = p_pRspBuf;
  
  if (!p_ucMethID || p_ucMethID >= TLME_METHOD_NO)
      return SFC_INVALID_METHOD;
  
  if (p_unReqSize != c_aTLMEMethParamLen[p_ucMethID-1])
      return SFC_INVALID_SIZE;     
  
  *(p_pRspBuf++) = 0; // SUCCESS.... TL editor is not aware that execute service already has a SFC
  
  switch(p_ucMethID)
  {
  case TLME_RESET_METHID: 
      TLME_Reset(*p_pReqBuf); 
      break;
    
  case TLME_HALT_METHID: 
    {
        uint16 unPortNr = (((uint16)*(p_pReqBuf+16)) << 8) | *(p_pReqBuf+17);
        TLME_Halt( p_pReqBuf, unPortNr ); 
        break;
    }
      
  case TLME_PORT_RANGE_INFO_METHID:
    {
        TLME_PORT_RANGE_INFO stPortRangeInfo;  
        TLME_GetPortRangeInfo( p_pReqBuf, &stPortRangeInfo );
        
        p_pRspBuf = DMAP_InsertUint16( p_pRspBuf, stPortRangeInfo.m_unNbActivePorts);
        p_pRspBuf = DMAP_InsertUint16( p_pRspBuf, stPortRangeInfo.m_unFirstActivePort);
        p_pRspBuf = DMAP_InsertUint16( p_pRspBuf, stPortRangeInfo.m_unLastActivePort);        
        break;          
    }
    
  case TLME_GET_PORT_INFO_METHID:
  case TLME_GET_NEXT_PORT_INFO_METHID:  
    {
        TLME_PORT_INFO stPortInfo;  
        uint16 unPorNo = (((uint16)*(p_pReqBuf+16)) << 8) | *(p_pReqBuf+17);
        
        uint8 ucStatus;
        
        if (p_ucMethID == TLME_GET_PORT_INFO_METHID)
        {
            ucStatus = TLME_GetPortInfo( p_pRspBuf, unPorNo, &stPortInfo);
        }
        else // if (p_ucMethID == TLME_GET_NEXT_PORT_INFO_METHID)
        {
            ucStatus = TLME_GetNextPortInfo( p_pRspBuf, unPorNo, &stPortInfo);
        }
        
        if (SFC_SUCCESS == ucStatus)
        {
            p_pRspBuf = DMAP_InsertUint16( p_pRspBuf, unPorNo );
            p_pRspBuf = DMAP_InsertUint32( p_pRspBuf, stPortInfo.m_unUID );

#ifdef BACKBONE_SUPPORT            
            *(p_pRspBuf++) = 0; // not compressed  
#else
            *(p_pRspBuf++) = 1; // compressed any time at device level 
#endif            

            p_pRspBuf = DMAP_InsertUint32( p_pRspBuf, stPortInfo.m_aTPDUCount[TLME_PORT_TPDU_IN_OK] );
            p_pRspBuf = DMAP_InsertUint32( p_pRspBuf, stPortInfo.m_aTPDUCount[TLME_PORT_TPDU_IN] );
            p_pRspBuf = DMAP_InsertUint32( p_pRspBuf, stPortInfo.m_aTPDUCount[TLME_PORT_TPDU_OUT_OK] );
            p_pRspBuf = DMAP_InsertUint32( p_pRspBuf, stPortInfo.m_aTPDUCount[TLME_PORT_TPDU_OUT] );
        }
        else
        {
            *pStart = 1; // FAIL
        }
        break;     
    }  
  }
  
  *p_pRspSize = p_pRspBuf - pStart;
  
  return ucSFC;
  
}



//////////////////////////////////////NLMO Object///////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Executes network layer management object methods
/// @param  p_unMethID - method identifier
/// @param  p_unReqSize - request buffer size
/// @param  p_pReqBuf - request buffer containing method parameters
/// @param  p_pRspSize - response buffer size
/// @param  p_pRspBuf - response buffer
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 NLMO_execute(uint8    p_ucMethID,
                   uint16   p_unReqSize,
                   uint8 *  p_pReqBuf,
                   uint16 * p_pRspSize,
                   uint8 *  p_pRspBuf)
{
  uint8 ucSFC;
  #warning " Change: ASL works with uint16 length. DMAP works with uint8 length"
  uint8 ucRspLen = *p_pRspSize;
  
  if ( p_unReqSize < 2 )
      return SFC_INVALID_SIZE;
  
  uint16 unAttrID = ((uint16)p_pReqBuf[0] << 8) | p_pReqBuf[1];
  p_pReqBuf += 2;
  p_unReqSize -= 2;
  
  uint32 ulSchedTAI;
  if( (NLMO_GET_ROW_RT != p_ucMethID) 
      && (NLMO_GET_ROW_CONTRACT_TBL != p_ucMethID) 
      && (NLMO_GET_ROW_ATT != p_ucMethID) )
  {
      if ( p_unReqSize < 4 )
          return SFC_INVALID_SIZE;
      
      ulSchedTAI = ((uint32)p_pReqBuf[0] << 24) |
                   ((uint32)p_pReqBuf[1] << 16) |
                   ((uint32)p_pReqBuf[2] << 8) |
                   p_pReqBuf[3];
      p_pReqBuf += 4;
      p_unReqSize -= 4;
  } 
  
  *p_pRspSize = 0;
  
  switch(p_ucMethID)
  {        
  case NLMO_GET_ROW_RT:
  case NLMO_GET_ROW_CONTRACT_TBL:
  case NLMO_GET_ROW_ATT:    
      ucSFC = NLME_GetRow( unAttrID,
                           p_pReqBuf,
                           p_unReqSize,    
                           p_pRspBuf,  
                           &ucRspLen);                 
      *p_pRspSize = ucRspLen;
      break;
      
  case NLMO_SET_ROW_RT:
  case NLMO_SET_ROW_CONTRACT_TBL:
  case NLMO_SET_ROW_ATT:    
      ucSFC = NLME_SetRow( unAttrID,
                           ulSchedTAI,
                           p_pReqBuf,
                           p_unReqSize);     
      break;     
  
  case NLMO_DEL_ROW_RT:  
  case NLMO_DEL_ROW_CONTRACT_TBL:  
  case NLMO_DEL_ROW_ATT:      
      ucSFC = NLME_DeleteRow( unAttrID,
                              ulSchedTAI,
                              p_pReqBuf,
                              p_unReqSize);       
      break;

  default: 
      ucSFC = SFC_INVALID_METHOD;
      break;
  }
  
  return ucSFC;
}


#ifndef BACKBONE_SUPPORT
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Ban an advertise router for a time period
/// @param  p_unDAUXRouterAddr - short address of router
/// @param  p_unBannedPeriod - ban time in seconds
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void AddBannedAdvRouter(SHORT_ADDR p_unDAUXRouterAddr, uint16 p_unBannedPeriod)
{
  // add at begin (not from interrupt)
  memmove( g_astBannedRouters+1, g_astBannedRouters, sizeof(g_astBannedRouters) - sizeof(g_astBannedRouters[0]) );
  g_astBannedRouters[0].m_unRouterAddr = p_unDAUXRouterAddr;
  g_astBannedRouters[0].m_unBannedPeriod = p_unBannedPeriod;
  
  if( g_ucBannedRoutNo < MAX_BANNED_ROUTER_NO )
  {
    g_ucBannedRoutNo++;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Updates time for banned routers
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
static void UpdateBannedRouterStatus()
{
  uint8 ucIdx = 0;
  
  for( ucIdx = 0; ucIdx < g_ucBannedRoutNo; ucIdx++ )
  {
    if( g_astBannedRouters[ ucIdx ].m_unBannedPeriod )
    {
        g_astBannedRouters[ ucIdx ].m_unBannedPeriod --;
    }
  }
  
  if( ucIdx && !g_astBannedRouters[ ucIdx-1 ].m_unBannedPeriod ) // last banned entry expired
  {
      g_ucBannedRoutNo--;      
  }
}
#endif  //#ifndef BACKBONE_SUPPORT
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Logs the size of the DLL queue, of the NLME tables and of the security key table    
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void  DMAP_LogStatus(void)
{  
  #if defined(BACKBONE_SUPPORT) && (DEVICE_TYPE == DEV_TYPE_AP91SAM7X512) 

extern uint8           g_ucSLMEActiveKeysNo;
extern uint8           g_ucSLMEKeysNo;
  
  static uint8 s_aStatusTable[SMIB_IDX_NO+6];
  uint8 aStatusTable[sizeof(s_aStatusTable)];
  
  uint8 ucDllMsgNo = g_pDllMsgQueueEnd - g_aDllMsgQueue;
    
  memcpy( aStatusTable, g_stDll.m_aSMIBTableRecNo, SMIB_IDX_NO+1 ); 
  
  aStatusTable[SMIB_IDX_NO+1] = ucDllMsgNo;
  aStatusTable[SMIB_IDX_NO+2] = g_stNlme.m_ucContractNo;
  aStatusTable[SMIB_IDX_NO+3] = g_stNlme.m_ucAddrTransNo;
  
  aStatusTable[SMIB_IDX_NO+4] = g_stNlme.m_ucRoutesNo;
  aStatusTable[SMIB_IDX_NO+5] = g_ucSLMEKeysNo;
      
  if( memcmp( aStatusTable, s_aStatusTable, sizeof(s_aStatusTable) ) )
  {
      memcpy( s_aStatusTable, aStatusTable, sizeof(s_aStatusTable) );
      
      Log(LOG_DEBUG,LOG_M_MAP,MAPLOG_Status
                  , SMIB_IDX_NO+1, s_aStatusTable                  
                  , 3, s_aStatusTable + SMIB_IDX_NO+1
                  , 2, s_aStatusTable + SMIB_IDX_NO+4
                  );
  }
  #endif
}

#if defined(BACKBONE_SUPPORT) && (DEVICE_TYPE == DEV_TYPE_MC13225)
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @author NIVIS LLC, Ion Ticus
    /// @brief  Parses the management commands received over the serial interface by the backbone
    /// @return none
    /// @remarks
    ///      Access level: user level
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    uint8 DMAP_ParseLocalMng( const uint8 * p_pMngCmd, uint8 p_ucMngCmdLen )
    {
        switch( *p_pMngCmd )
        {
        case LOCAL_MNG_RESTARTED_SUBCMD:
                if (p_ucMngCmdLen == 2)
                {
                    g_ucProvisioned = FALSE;
                    return 1;
                }
                break;
                
        case LOCAL_MNG_PROVISIONING_SUBCMD:
                if( p_ucMngCmdLen >= 3
                            + sizeof(c_oEUI64BE)
                            + sizeof(g_stFilterBitMask)
                            + sizeof(g_stFilterTargetID)
                            + sizeof(g_aJoinAppKey)
                            + sizeof(c_oSecManagerEUI64BE)
                            + sizeof(g_stDMO.m_aucSysMng128BitAddr)
                            + sizeof(g_stDMO.m_auc128BitAddr) ) 
                {                    
                    p_pMngCmd += 1+2; // cmd + format
                    memcpy( c_oEUI64BE, p_pMngCmd, sizeof(c_oEUI64BE) ); 
                    p_pMngCmd += sizeof(c_oEUI64BE);
                    
                    g_stFilterBitMask  = ( ((uint16)p_pMngCmd[0]) << 8 ) | p_pMngCmd[1]; 
                    p_pMngCmd += 2;
                    
                    g_stFilterTargetID = ( ((uint16)p_pMngCmd[0]) << 8 ) | p_pMngCmd[1]; 
                    p_pMngCmd += 2;
                    
                    memcpy( g_aJoinAppKey, p_pMngCmd, sizeof(g_aJoinAppKey) ); 
                    p_pMngCmd += sizeof(g_aJoinAppKey);
                    
                    memcpy( c_oSecManagerEUI64BE, p_pMngCmd, sizeof(c_oSecManagerEUI64BE) ); 
                    p_pMngCmd += sizeof(c_oSecManagerEUI64BE);
                    
                    memcpy( g_stDMO.m_aucSysMng128BitAddr, p_pMngCmd, sizeof(g_stDMO.m_aucSysMng128BitAddr) ); 
                    p_pMngCmd += sizeof(g_stDMO.m_aucSysMng128BitAddr);
                    
                    memcpy( g_stDMO.m_auc128BitAddr, p_pMngCmd, sizeof(g_stDMO.m_auc128BitAddr) );      
                    p_pMngCmd += sizeof(g_stDMO.m_auc128BitAddr);
                                        
                    DLME_CopyReversedEUI64Addr( c_oEUI64LE, c_oEUI64BE );
                    
                    DMAP_DLMO_ResetStack(6);
                    
                    g_ucProvisioned = TRUE;                    
                    return 1;
                }
                break;    
        
        case LOCAL_MNG_ROUTES_SUBCMD:
                p_ucMngCmdLen -= 2;
                if ( p_ucMngCmdLen < sizeof(g_stNlme.m_aToEthRoutes) )
                {
                    g_stNlme.m_ucDefaultIsDLL = !p_pMngCmd[1]; // 0 means BBR, 1 means ETH 
                    g_stNlme.m_ucToEthRoutesNo = p_ucMngCmdLen/2;
                    memcpy(g_stNlme.m_aToEthRoutes, p_pMngCmd+2, p_ucMngCmdLen);
                    return 1;
                }               
                break;
        case LOCAL_MNG_ALERT_SUBCMD:
          {
              ALERT stAlert;
              typedef struct
              {
                uint8 m_uaObjPort[2];
                uint8 m_uaObjId[2];
                uint8 m_uaTime[6];
                uint8 m_ucAlertId;
                uint8 m_ucClass;
                uint8 m_ucDirection;
                uint8 m_ucCategory;
                uint8 m_ucType;
                uint8 m_ucPriority;
              } BBR_ALERT;

              BBR_ALERT * pBBRAlert = (BBR_ALERT*)(p_pMngCmd+1);
              stAlert.m_unDetObjTLPort = ((uint16)(pBBRAlert->m_uaObjPort[0])<< 8) | pBBRAlert->m_uaObjPort[1];
              stAlert.m_unDetObjID = ((uint16)(pBBRAlert->m_uaObjId[0])<< 8) | pBBRAlert->m_uaObjId[1]; 
              stAlert.m_ucClass = pBBRAlert->m_ucClass; 
              stAlert.m_ucDirection = pBBRAlert->m_ucDirection; 
              stAlert.m_ucCategory = pBBRAlert->m_ucCategory; 
              stAlert.m_ucType = pBBRAlert->m_ucType;                      
              stAlert.m_ucPriority = pBBRAlert->m_ucPriority;                      
              stAlert.m_unSize = p_ucMngCmdLen - 1 - sizeof(BBR_ALERT); 
              
              if( stAlert.m_unSize < 128 )
              {
                  ARMO_AddAlertToQueue( &stAlert, (uint8*)(pBBRAlert+1) );
              }
          }
        }
        
        return 0;
    }
#endif //#if defined(BACKBONE_SUPPORT) && (DEVICE_TYPE == DEV_TYPE_MC13225)
