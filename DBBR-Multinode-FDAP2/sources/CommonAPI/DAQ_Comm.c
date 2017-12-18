// $Id: DAQ_Comm.c 6912 2009-03-19 08:46:55Z ion.ticus@NIVISATL $
/*************************************************************************
 * File: DAQ_Comm.c
 * Author: John Chapman, Nivis LLC
 * Revised by: $Author: John.Chapman@NIVISATL $
 * Revision: $Revision: 6912 $
 * Description: Methods for communicating/updating data with
 *              the DAQ Processor
 *************************************************************************/
#include "../global.h"
#include "DAQ_Comm.h"
#include "../ISA100/aslde.h"
#include "../ISA100/tlde.h"
#include "../ISA100/dmap.h"
#include "../ISA100/uap.h"
#include "../ISA100/dmap_udo.h"
#include "../ISA100/mlsm.h"

#if ( _UAP_TYPE != 0 )

#include "Common_API.h"

void ReadTaiSeconds(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);

#define HEARTBEAT_TICS_20ms    0
#define HEARTBEAT_TICS_50ms    0
#define HEARTBEAT_TICS_100ms   0
#define HEARTBEAT_TICS_500ms   2
#define HEARTBEAT_TICS_1000ms  4
#define HEARTBEAT_TICS_60000ms (4*60)

static const uint16 HeartBeatFreqTable[NUM_SUPPORTED_HEARTBEAT_FREQ] =
{
    HEARTBEAT_TICS_500ms,    // Not used
    HEARTBEAT_TICS_20ms,     // HEARTBEAT_20ms,
    HEARTBEAT_TICS_50ms,     // HEARTBEAT_50ms,
    HEARTBEAT_TICS_100ms,    // HEARTBEAT_100ms,
    HEARTBEAT_TICS_500ms,    // HEARTBEAT_500ms,
    HEARTBEAT_TICS_1000ms,  // HEARTBEAT_1000ms,
    HEARTBEAT_TICS_60000ms  // HEARTBEAT_60000ms,
};

uint8 g_ucPowerType;
uint8 g_ucPowerLevel;

uint8 g_ucUapId;
uint16 g_unHeartBeatTick = 0;
uint16 g_unHeartBeatLimit = HEARTBEAT_TICS_500ms;

const DMAP_FCT_STRUCT c_aIsa100ReadWriteTable[ISA100_CONTRACT_REQUEST - ISA100_ITEMS] =
{
    { 0, 0                              , DMAP_EmptyReadFunc     , NULL },     // ISA100_LINK_STATUS
    { 0, 0                              , DMAP_EmptyReadFunc     , NULL },     // ISA100_LINK_QUALITY
    { ATTR_CONST(g_ucJoinStatus)        , DMAP_ReadUint8         , NULL },     // ISA100_JOIN_STATUS
    { 0, 0                              , DMAP_EmptyReadFunc     , NULL },     // ISA100_ACTIVE_SECURITY_LEVEL
    { 0, sizeof(uint32)                 , ReadTaiSeconds         , NULL },     // ISA100_GET_TAI_TIME
    { ATTR_CONST(g_ucPowerType)         , DMAP_ReadUint8         , DMAP_WriteUint8 },        // ISA100_POWER_TYPE
    { ATTR_CONST(g_ucPowerLevel)        , DMAP_ReadUint8         , DMAP_WriteUint8 },        // ISA100_POWER_LEVEL
    { ATTR_CONST(g_ucUapId)             , DMAP_ReadUint8         , DMAP_WriteUint8 },// ISA100_UAP_IDS
    { 0, 0                              , DMAP_EmptyReadFunc     , NULL }     // ISA100_CONTRACT_REQUEST
};

///////////////////////////////////////////////////////////////////////////////////
// Name: DAQ_Init
// Description: SPI communication handler
// Parameters: none
// Retunn: none
///////////////////////////////////////////////////////////////////////////////////
void DAQ_Init(void)
{    
#if ( _UAP_TYPE == UAP_TYPE_ISA100_API )
    g_ucUapId = 0;

    SendMessage(STACK_SPECIFIC, ISA100_GET_INITIAL_INFO, API_REQ, 0, 0, NULL);
#else // UAP_TYPE_SIMPLE_API
    g_ucUapId = UAP_APP1_ID;
#endif    
    
}

///////////////////////////////////////////////////////////////////////////////////
// Name: DAQ_UpdateHeartBeatFreq
// Description: Update the frequency at which the heartbeat is sent to the application
//              processor
// Parameters: NewFrequency: the newly requested frequency.
// Return: none
///////////////////////////////////////////////////////////////////////////////////
uint8 DAQ_UpdateHeartBeatFreq(enum HEARTBEAT_FREQ p_eNewFrequency)
{
    if (p_eNewFrequency > MIN_HEARTBEAT_FREQ)
        return SFC_FAILURE;
    if (p_eNewFrequency < MAX_HEARTBEAT_FREQ)
        return SFC_FAILURE;

    g_unHeartBeatLimit = HeartBeatFreqTable[ p_eNewFrequency ];

    return SFC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////
// Name: DAQ_Handler
// Description: SPI communication handler
// Parameters: none
// Retunn: none
///////////////////////////////////////////////////////////////////////////////////
void DAQ_Handler(void)
{   
    uint8 aMsgBuff[MAX_API_BUFFER_SIZE];
    ApiMessageFrame * pMsgFrame = (ApiMessageFrame *)aMsgBuff;

    // RX packets
    uint8 ucPacketSize = API_PHY_Read(aMsgBuff, sizeof(aMsgBuff));
    if ( ucPacketSize == (pMsgFrame->MessageSize + sizeof(ApiMessageFrame)) ) 
    {        
        // parse response
        API_OnRcvMsg( pMsgFrame );
    }
    
#if ( _UAP_TYPE == UAP_TYPE_ISA100_API )    
    if( !DAQ_IsReady() && !g_unTxQueueSize )
    {
        DAQ_Init();
        return;
    }
    // TX packets
    APDU_IDTF stApduIdtf;
    uint8 *pApdu = ASLDE_GetMyAPDU( g_ucUapId, &stApduIdtf);
    if( pApdu )
    {
        // Send APDU to DAQ.
        memcpy(aMsgBuff, &stApduIdtf, sizeof(APDU_IDTF));
        
        if (sizeof(aMsgBuff)-sizeof(APDU_IDTF) > stApduIdtf.m_unDataLen){
            memcpy(&aMsgBuff[sizeof(APDU_IDTF)], pApdu, stApduIdtf.m_unDataLen);
        }
        else {
            // Message size is larger than can be sent to the application processor.
            // TODO: Flag this error somehow.
        }

        if( NO_ERR == SendMessage(STACK_SPECIFIC, 
                        ISA100_TL_APDU_INDICATE, 
                        API_REQ, 
                        0, 
                        sizeof(APDU_IDTF)+stApduIdtf.m_unDataLen, 
                        aMsgBuff) )
        {
            ASLDE_DeleteAPDU( pApdu );
        }
    }
#endif

    // Normal Heartbeat freqency.
    uint16 unHeartBeatLimit;    
    
    if( EXT_WAKEUP_IS_ON() )        unHeartBeatLimit = 2; // = 1;
    else if( g_unTxQueueSize )      unHeartBeatLimit = 2;
    else                            unHeartBeatLimit = g_unHeartBeatLimit;

    if( (++g_unHeartBeatTick) >= unHeartBeatLimit ) 
    {
        API_PHY_Write( NULL, 0 ); // send pending messsage or API_HEARTBEAT if case
    }
}
///////////////////////////////////////////////////////////////////////////////////
// Name: ReadTaiSeconds
// Description: Helper function to read current time
// Parameters: none
// Retunn: none
///////////////////////////////////////////////////////////////////////////////////
void ReadTaiSeconds(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize)
{
    uint32 CurrentTime = MLSM_GetCrtTaiSec();

    *(p_pBuf++) = (CurrentTime >> 24) & 0xFF;
    *(p_pBuf++) = (CurrentTime >> 16) & 0xFF;
    *(p_pBuf++) = (CurrentTime >> 8)  & 0xFF;
    *(p_pBuf++) = (CurrentTime)       & 0xFF;
}

#endif // _UAP_TYPE != 0
