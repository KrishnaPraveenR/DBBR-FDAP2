/***************************************************************************************************
 * Name:         Common_API.c
 * Author:       NIVIS LLC, John Chapman and Kedar Kapoor
 * Date:         July 2008
 * Description:  This file has the Common Radio routines for the RF Modem side of the common API.
 *               This also is the API for the single processor solution.  This routine will call
 *               into the serial port to grab data.
 * Changes:
 * Revisions:
 ***************************************************************************************************/

#include "Common_API.h"
#include "../global.h"
#include "DAQ_Comm.h"
#include "../ISA100/aslde.h"
#include "../ISA100/mlsm.h"
#include "../ISA100/dmap.h"
#include "../ISA100/dmap_dmo.h"
#include "../ISA100/uap_data.h"

#include <string.h>

#if ( _UAP_TYPE != 0 )


void ReadTaiSeconds(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);
uint8 API_setDPO( uint8 * pRcvData, uint8 ucMessageSize );


static enum API_STATE s_ApiState = STATE_ACTIVE;

static const uint16 HwPlatform = API_PLATFORM;
static const uint16 ApiVersion = API_VERSION;
static const uint16 MaxApiBufferSize = MAX_API_BUFFER_SIZE;
static const uint8  MaxSpiSpeed = MAX_SPI_SPEED;
static const uint8  MaxUartSpeed = MAX_UART_SPEED;

void API_onRcvDataPassTrough( ApiMessageFrame * p_pMsgFrame );
void API_onRcvPowerManagement( ApiMessageFrame * p_pMsgFrame );
void API_onRcvStackSpecific( ApiMessageFrame * p_pMsgFrame );
void API_onRcvApiCommands( ApiMessageFrame * p_pMsgFrame );
void API_onRcvACK( ApiMessageFrame * p_pMsgFrame );
void API_onRcvNACK( ApiMessageFrame * p_pMsgFrame );

typedef void (*API_FCT_OnRcv)(ApiMessageFrame * p_pMsgFrame);

typedef struct 
{
    uint8   m_aDstAddr128[16];
    uint16  m_unDstTLPort;
    uint16  m_unSrcTLPort;
    uint32  m_ulContractLife;
    uint8   m_ucSrvcType;
    uint8   m_ucPriority;
    uint16  m_unMaxAPDUSize; 
    uint8   m_ucReliability;                              
    DMO_CONTRACT_BANDWIDTH  m_stBandwidth;
} UAP_CONTR_REQ;


const API_FCT_OnRcv c_aAPIOnRcvFct[NUM_ITEMS_MSG_CLASS-1] = 
{
    API_onRcvDataPassTrough,
    API_onRcvPowerManagement,
    API_onRcvStackSpecific,
    API_onRcvApiCommands,
    API_onRcvACK,
    API_onRcvNACK
};

const DMAP_FCT_STRUCT c_aApiReadWriteTable[NUM_ITEMS_API_COMMAND_TYPES] =
{
    { 0, 0                             , DMAP_EmptyReadFunc     , NULL },     // just for protection; attributeID will match index in this table
    { ATTR_CONST(HwPlatform)           , DMAP_ReadUint16        , NULL },     // API_HW_PLATFORM
    { ATTR_CONST(ApiVersion)           , DMAP_ReadUint16        , NULL },     // API_FW_VERSION
    { ATTR_CONST(MaxApiBufferSize)     , DMAP_ReadUint16        , NULL },     // API_MAX_BUFFER_SIZE
    { ATTR_CONST(MaxSpiSpeed)          , DMAP_ReadUint8         , NULL },     // API_MAX_SPI_SPEED
    { 0, 0                             , DMAP_EmptyReadFunc     , NULL },     // API_UPDATE_SPI_SPEED
    { ATTR_CONST(MaxUartSpeed)         , DMAP_ReadUint8         , NULL },     // API_MAX_UART_SPEED
    { 0, 0                             , DMAP_EmptyReadFunc     , NULL },     // API_UPDATE_UART_SPEED
    { 0, 0                             , DMAP_EmptyReadFunc     , NULL },     // API_HEARTBEAT_FREQ
    { 0, 0                             , DMAP_EmptyReadFunc     , NULL }     // API_HEARTBEAT
};


/****************
 * SendMessage
 ****************/
enum SEND_ERROR_CODE SendMessage(enum MSG_CLASS p_MsgClass,
                                 uint8 p_MsgType,
                                 uint8 p_ucIsRsp,
                                 uint8 p_ucMSG_ID, 
                                 uint8 p_BuffSize,
                           const uint8 *p_pMsgBuff)
{    
    static uint8 s_ucMsgId;
    uint8 anPacketBuffer[MAX_API_BUFFER_SIZE];
    ApiMessageFrame *MessageFrame = (ApiMessageFrame *)anPacketBuffer;

    if (p_BuffSize > MAX_API_BUFFER_SIZE) {
        return ER_UNSUPPORTED_DATA_SIZE;
    }

    if( !p_ucIsRsp )
    {
        p_ucMSG_ID = (s_ucMsgId++) | 0x80;
    }
    
    memset(anPacketBuffer, 0, MAX_API_BUFFER_SIZE);

    MessageFrame->MessageHeader.MessageClass = p_MsgClass;
    MessageFrame->MessageType = p_MsgType;
    MessageFrame->MessageHeader.m_bIsRsp = p_ucIsRsp;    
    MessageFrame->MessageID = p_ucMSG_ID;        
    MessageFrame->MessageSize = p_BuffSize;
    if (p_pMsgBuff != NULL) 
    {
        memcpy(anPacketBuffer+sizeof(ApiMessageFrame), p_pMsgBuff, MessageFrame->MessageSize);
    }

    if (SUCCESS != API_PHY_Write(anPacketBuffer, MessageFrame->MessageSize + sizeof(ApiMessageFrame))){
        return ER_QUEUE_FULL;
    }
    
    return NO_ERR;
}

void API_onRcvDataPassTrough( ApiMessageFrame * p_pMsgFrame )
{
#if ( _UAP_TYPE == UAP_TYPE_SIMPLE_API )
    uint8 * pRcvData = (uint8 *)(p_pMsgFrame + 1); // skip the header       
    
    switch (p_pMsgFrame->MessageType)
    {
        case READ_DATA_RESP:
            while(p_pMsgFrame->MessageSize >= 5 )
            {
                if(pRcvData[0] >= DIGITAL_DATA_ATTR_ID_OFFSET)
                {
                    UAPDATA_SetDigitalVal( UAP_DATA_DIGITAL1 + pRcvData[0] - DIGITAL_DATA_ATTR_ID_OFFSET, pRcvData[4] );                    
                }
                else
                {
                    float fTmp;
                    DMAP_WriteUint32( &fTmp,  pRcvData + 1, 4 );
                    
                    UAPDATA_SetAnalogVal(UAP_DATA_ANALOG1 + pRcvData[0] - ANALOG_DATA_ATTR_ID_OFFSET, fTmp );
                }
                
                p_pMsgFrame->MessageSize -= 5;
                pRcvData += 5;                
            }
            break;
        default:;    
    }
#endif  //  _UAP_TYPE == UAP_TYPE_SIMPLE_API  
}

void API_onRcvPowerManagement( ApiMessageFrame * p_pMsgFrame )
{
  SendMessage(API_NACK, NACK_UNSUPPORTED_FEATURE, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
}

void API_onRcvStackSpecific( ApiMessageFrame * p_pMsgFrame )
{
    uint8   aDataBuffer[DAQ_BUFFER_SIZE];
    uint8   ucMessageSize;
    uint8 * pRcvData = (uint8 *)(p_pMsgFrame + 1); // skip the header
    
    switch (p_pMsgFrame->MessageType)
    {
    // Generic Read/Write attributes
    case ISA100_GET_TAI_TIME:
    case ISA100_JOIN_STATUS:
    case ISA100_POWER_TYPE:
    case ISA100_POWER_LEVEL:
        {
            const DMAP_FCT_STRUCT * pFct = c_aIsa100ReadWriteTable + p_pMsgFrame->MessageType-ISA100_ITEMS-1;
            ucMessageSize = pFct->m_ucSize;
            pFct->m_pReadFct( pFct->m_pValue, aDataBuffer, &ucMessageSize);
            SendMessage(STACK_SPECIFIC, p_pMsgFrame->MessageType, API_RSP, p_pMsgFrame->MessageID, ucMessageSize, aDataBuffer );
        }
        break;

    case ISA100_UAP_IDS:
        {
            const DMAP_FCT_STRUCT * pFct = c_aIsa100ReadWriteTable + p_pMsgFrame->MessageType-ISA100_ITEMS-1;
            ucMessageSize = pFct->m_ucSize;
            if( pFct->m_pWriteFct )
            {
                pFct->m_pWriteFct( pFct->m_pValue, pRcvData, ucMessageSize);
                SendMessage(STACK_SPECIFIC, p_pMsgFrame->MessageType, API_RSP, p_pMsgFrame->MessageID, ucMessageSize, pRcvData);
            }
        }
        break;
        
#if ( _UAP_TYPE == UAP_TYPE_ISA100_API )
    case ISA100_CONTRACT_REQUEST:
        {
            uint8 ucResponse;
            UAP_CONTR_REQ stContract;
            const uint8 * pData = pRcvData+2;
            
            stContract.m_ucSrvcType = *(pData++);
            pData = DMAP_ExtractUint16( pData, &stContract.m_unSrcTLPort );
            memcpy( stContract.m_aDstAddr128, pData, 16 );
            pData += 16;
            pData = DMAP_ExtractUint16( pData, &stContract.m_unDstTLPort );

            pData ++; // skip negociation 
  
            pData = DMAP_ExtractUint32(pData, &stContract.m_ulContractLife );
            stContract.m_ucPriority = *(pData++);
 
            pData = DMAP_ExtractUint16( pData, &stContract.m_unMaxAPDUSize );  

            stContract.m_ucReliability = *(pData++);
            
            pData = DMO_ExtractBandwidthInfo( pData, &stContract.m_stBandwidth, stContract.m_ucSrvcType );
            
            if( (pData - pRcvData) == p_pMsgFrame->MessageSize ) 
            {
                ucResponse = DMO_RequestNewContract(
                                                    stContract.m_aDstAddr128, 
                                                    stContract.m_unDstTLPort, 
                                                    stContract.m_unSrcTLPort,
                                                    stContract.m_ulContractLife,
                                                    stContract.m_ucSrvcType,
                                                    stContract.m_ucPriority,
                                                    stContract.m_unMaxAPDUSize,
                                                    stContract.m_ucReliability,
                                                   &stContract.m_stBandwidth );
            }
            else
            {
                ucResponse = SFC_INVALID_SIZE;
            }
              
            if (SFC_SUCCESS == ucResponse) 
            {
                SendMessage(API_ACK, DATA_OK, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
            }
            else if (SFC_DUPLICATE == ucResponse) // contract already exists, build a fake contract response 
            {
                uint8 aContrRsp[32];
                uint8 * pucTemp = aContrRsp;
                DMO_CONTRACT_ATTRIBUTE *pContract = DMO_GetContract( stContract.m_aDstAddr128, 
                                                                    stContract.m_unDstTLPort, 
                                                                    stContract.m_unSrcTLPort, 
                                                                    stContract.m_ucSrvcType);
                *(pucTemp++) = pRcvData[0]; // req ID
                *(pucTemp++) = 0;           // success with immediate effect
    
                 pucTemp = DMAP_InsertUint16(pucTemp, pContract->m_unContractID);   
                *(pucTemp++) = pContract->m_ucServiceType; 
                pucTemp = DMAP_InsertUint32(pucTemp, pContract->m_ulAssignedExpTime);
                *(pucTemp++) = pContract->m_ucPriority;   
                pucTemp = DMAP_InsertUint16(pucTemp, pContract->m_unAssignedMaxNSDUSize);   
                *(pucTemp++) = pContract->m_ucReliability;
  
                pucTemp = DMO_InsertBandwidthInfo(  pucTemp, 
                                                     &pContract->m_stBandwidth, 
                                                     pContract->m_ucServiceType );     
  
                SendMessage(STACK_SPECIFIC, ISA100_NOTIFY_ADD_CONTRACT, API_RSP, p_pMsgFrame->MessageID, pucTemp - aContrRsp, aContrRsp );
            }
            else 
            {
                SendMessage(API_NACK, NACK_STACK_ERROR, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
            }
        }
        break;

    case ISA100_TL_APDU_REQUEST:      
        ucMessageSize = p_pMsgFrame->MessageSize;
      
        if (  ( ucMessageSize > 5 )
           && ( SFC_SUCCESS == ASLSRVC_AddGenericObject(
                                 pRcvData + 5,
                                 pRcvData[2],   // ServiceType
                                 0,             // priority
                                 pRcvData[0],   // SrcTSAPID
                                 pRcvData[1],   // DstTSAPID
                                 0,
                                 NULL,                      // EUI64 addr
                                 pRcvData[3] | (((uint16)(pRcvData[4])) << 8) ,// contract ID
                                 ucMessageSize-5 ) ) )
        {
            SendMessage(API_ACK, DATA_OK, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
        }
        else 
        {
            SendMessage(API_NACK, NACK_STACK_ERROR, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
        }
        break;
        
    case ISA100_CONTRACT_TERMINATE:
        break;

    // Custom/Specialized attributes
    case ISA100_GET_INITIAL_INFO: // power type, power level, uap ID, routing type
        g_ucUapId = pRcvData[2];    
        
        #if defined (ROUTING_SUPPORT)
        g_stDPO.m_unDeviceRole = (pRcvData[3] ? 0x03: 0x01);
        #endif
        
//        SendMessage(API_ACK, DATA_OK, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
        break;

    case ISA100_SET_DPO:
        if( SFC_SUCCESS == API_setDPO( pRcvData, ucMessageSize ) )
        {
            SendMessage(API_ACK, DATA_OK, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
        }
        else
        {
            SendMessage(API_NACK, NACK_STACK_ERROR, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
        }

        break;
#endif // _UAP_TYPE == UAP_TYPE_ISA100_API
        
    case ISA100_SECURITY_LEVEL:
        aDataBuffer[0] = GetDLSecurityLevel();
        aDataBuffer[1] = GetTLSecurityLevel();
        SendMessage(STACK_SPECIFIC, ISA100_SECURITY_LEVEL, TRUE, p_pMsgFrame->MessageID, 2, aDataBuffer);
        break;
      
    case ISA100_LINK_QUALITY:
    case ISA100_LINK_STATUS:
    default:
        SendMessage(API_NACK, NACK_UNSUPPORTED_FEATURE, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
        break;
    }  
}

void API_onRcvApiCommands( ApiMessageFrame * p_pMsgFrame )
{
    uint8 ucMessgeSize;
    uint8 * pRcvData = (uint8*)(p_pMsgFrame+1);
    switch (p_pMsgFrame->MessageType)
    {
    case API_HW_PLATFORM:
    case API_FW_VERSION:
    case API_MAX_BUFFER_SIZE:
    case API_MAX_SPI_SPEED:
    case API_MAX_UART_SPEED:
        ucMessgeSize = c_aApiReadWriteTable[p_pMsgFrame->MessageType].m_ucSize;
        c_aApiReadWriteTable[p_pMsgFrame->MessageType].m_pReadFct(
                         c_aApiReadWriteTable[p_pMsgFrame->MessageType].m_pValue,
                         pRcvData, // read after msg header
                         &ucMessgeSize);
        SendMessage(API_COMMANDS, p_pMsgFrame->MessageType, API_RSP, p_pMsgFrame->MessageID, ucMessgeSize, pRcvData );
        break;

    case API_UPDATE_SPI_SPEED:
        if (API_PHY_SetSpeed((enum SPI_SPEED)pRcvData[0]) == SFC_SUCCESS){
            SendMessage(API_ACK, API_CHANGE_ACCEPTED, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
        }
        else {
            SendMessage(API_NACK, NACK_API_COMMAND_ERROR, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
        }
        break;

    case API_HEARTBEAT_FREQ:
        if (DAQ_UpdateHeartBeatFreq((enum HEARTBEAT_FREQ)pRcvData[0]) == SFC_SUCCESS){
            SendMessage(API_ACK, API_CHANGE_ACCEPTED, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
        }
        else {
            SendMessage(API_NACK, NACK_API_COMMAND_ERROR, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
        }
        break;

    case API_UPDATE_UART_SPEED:
        SendMessage(API_NACK, NACK_UNSUPPORTED_FEATURE, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
        break;

    default:
        SendMessage(API_NACK, NACK_API_COMMAND_ERROR, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
        break;
    }
}

void API_onRcvACK( ApiMessageFrame * p_pMsgFrame )
{
}

void API_onRcvNACK( ApiMessageFrame * p_pMsgFrame )
{
}

/****************
 * GetMessage
 ****************/
void API_OnRcvMsg( ApiMessageFrame * p_pMsgFrame )
{
    if (s_ApiState == STATE_ACTIVE )
    {
        uint8 ucMsgClass = p_pMsgFrame->MessageHeader.MessageClass-1;
        if( ucMsgClass < NUM_ITEMS_MSG_CLASS-1 )
        {
            c_aAPIOnRcvFct[ ucMsgClass ]( p_pMsgFrame );
        }
        else
        {
            SendMessage(API_NACK, NACK_UNSUPPORTED_FEATURE, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
        }
    }
    else
    {
        SendMessage(API_NACK, NACK_API_ERROR, API_RSP, p_pMsgFrame->MessageID, 0, NULL);
    }
}

uint8 API_setDPO( uint8 * pRcvData, uint8 ucMessageSize )
{
    GENERIC_ASL_SRVC  stGenSrvc;
    uint16 unSize;
    uint8  aBuff[16];
        
    const uint8 * pAPDU = pRcvData;
    
    while ( pAPDU = ASLSRVC_GetGenericObject( pAPDU,
                                        ucMessageSize - (pAPDU - pRcvData),
                                        &stGenSrvc,
                                        NULL)
          )
    {
      if( stGenSrvc.m_stSRVC.m_stWriteReq.m_unDstOID == DMAP_DPO_OBJ_ID ) // object id is on same pos for structures
      {
          if( stGenSrvc.m_ucType == SRVC_WRITE_REQ )
          {
              if( SFC_SUCCESS == DPO_Write( stGenSrvc.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_unAttrID, 
                                                   stGenSrvc.m_stSRVC.m_stExecReq.m_unLen, 
                                                   stGenSrvc.m_stSRVC.m_stExecReq.p_pReqData ) 
                 )
              {
                  continue;
              }
          }
          else if ( stGenSrvc.m_ucType == SRVC_EXEC_REQ )
          {
              if( SFC_SUCCESS == DPO_Execute( stGenSrvc.m_stSRVC.m_stExecReq.m_ucMethID,
                                                     stGenSrvc.m_stSRVC.m_stExecReq.m_unLen,
                                                     stGenSrvc.m_stSRVC.m_stExecReq.p_pReqData, 
                                                     &unSize,
                                                     aBuff )
                 )
              {
                  continue;
              }
          }
      }
      
      return SFC_FAILURE; 
    }
    
    return SFC_SUCCESS;
}

#endif // _UAP_TYPE
