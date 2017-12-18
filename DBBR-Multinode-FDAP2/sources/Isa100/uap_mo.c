////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Mircea Vlasin
/// Date:         June 2009
/// Description:  This file implements UAP Management Object
/// Changes:      Created
/// Revisions:    1.0
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "uap_mo.h"
#include "tlde.h"
#include "uap.h"
#include "aslsrvc.h"

#if ( _UAP_TYPE == UAP_TYPE_SIMPLE_API )

//#define UAPMO_CONTRACTS_NO      MAX_CONTRACT_NO - 2 //-2 the contracts with SM
const uint8 g_ucMaxUAPRetries = DEFAULT_UAP_RETRY_NO;

static void UAP_GetNumAperiodicContracts(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);
static void UAP_GetAperiodicCorrespondentsTable(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);
static void UAP_GetAperiodicContracts(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);
static void UAP_GetObjectTable(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);

static const uint8 g_pucUAPVersion[] = { 'U', 'A', 'P', ' ', 'v', '1', '.', '0', '.', '1'};
uint8 g_ucUAPState = UAP_ACTIVE;
uint8 g_ucUAPCommand = 0;

//will be computed at read request(read only attributes) 
//uint8 g_ucUAPContractsNo = 0;
//static UNSCHEDULED_CORRESPONDENT g_pstUAPCorrespondentTable[UAPMO_CONTRACTS_NO]; 
//static COMMUNICATION_CONTRACT_DATA g_pstUAPContractTable[UAPMO_CONTRACTS_NO]; 

static const uint8 g_ucUAPObjectsNo = UAP_OBJ_NO;

static const OBJECT_ID_AND_TYPE g_pstUAPObjectTable[UAP_OBJ_NO] =
{
    { UAP_MO_OBJ_ID,      UAP_MO_OBJ_TYPE,    0,  0 },
    { UAP_DATA_OBJ_ID,    UAP_DATA_OBJ_TYPE,  0,  0 },
    { UAP_CO_OBJ_ID,      UAP_CO_OBJ_TYPE,    0,  0 },
//    { UAP_DISP_OBJ_ID,    UAP_DISP_OBJ_TYPE,  0,  0 }
};
static const uint16 g_unUAPStaticRevision = 1;


const DMAP_FCT_STRUCT c_aUapMoAttributes[UAPMO_ATTR_NO] = {
   { 0,   0,                                            DMAP_EmptyReadFunc,             NULL },
   { ATTR_CONST(g_pucUAPVersion),                       DMAP_ReadVisibleString,         NULL },
   { ATTR_CONST(g_ucUAPState),                          DMAP_ReadUint8,                 NULL },
   { ATTR_CONST(g_ucUAPCommand),                        DMAP_ReadUint8,                 DMAP_WriteUint8 },
   { ATTR_CONST(g_ucMaxUAPRetries),                     DMAP_ReadUint8,                 NULL },
   { 0,            0,                                   UAP_GetNumAperiodicContracts,   NULL },
   { 0,            0,                                   UAP_GetAperiodicCorrespondentsTable,      NULL },
   { 0,            0,                                   UAP_GetAperiodicContracts,      NULL },
   { ATTR_CONST(g_ucUAPObjectsNo),                      DMAP_ReadUint8,                 NULL },
   { ATTR_CONST(g_pstUAPObjectTable),                   UAP_GetObjectTable,             NULL },
   { ATTR_CONST(g_unUAPStaticRevision),                 DMAP_ReadUint16,                NULL }
};

/**********************************
 * UAP_processUapmoExecuteRequest
 **********************************/
void UAP_processUapmoExecuteRequest(EXEC_REQ_SRVC * p_pExecReq,
                                      APDU_IDTF * p_pIdtf)
{
    // if Contract
    //   execute method
    //   Respond
    // else
    //   Request contract
    //   Respond no contract
}

/************************************
 * Local functions
 ************************************/

/**********************************
 * UAP_GetNumAperiodicContracts
 **********************************/
static void UAP_GetNumAperiodicContracts(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize)
{
    DMO_CONTRACT_ATTRIBUTE * pContract = g_stDMO.m_aContractTbl;
    uint16 unNumAperiodicContracts = 0;
    
    for( ; pContract < g_stDMO.m_aContractTbl + g_stDMO.m_ucContractNo; pContract++ )
    {
        if( SRVC_APERIODIC_COMM == pContract->m_ucServiceType &&
            (ISA100_START_PORTS + UAP_APP1_ID) == pContract->m_unSrcTLPort)
        {
            unNumAperiodicContracts++;
        }
    }    
    
    *(p_pBuf++) = unNumAperiodicContracts >> 8;
    *(p_pBuf++) = unNumAperiodicContracts;
    
    *p_ucSize = 2;
}

/**********************************
 * UAP_GetAperiodicCoresspondentsTable
 **********************************/
static void UAP_GetAperiodicCorrespondentsTable(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize)
{
    DMO_CONTRACT_ATTRIBUTE * pContract = g_stDMO.m_aContractTbl;
        
    *p_ucSize = 0;
    
    for( ; pContract < g_stDMO.m_aContractTbl + g_stDMO.m_ucContractNo; pContract++ )
    {
        if( SRVC_APERIODIC_COMM == pContract->m_ucServiceType &&
            (ISA100_START_PORTS + UAP_APP1_ID) == pContract->m_unSrcTLPort)
        {
            memcpy(p_pBuf, pContract->m_aDstAddr128, sizeof(pContract->m_aDstAddr128));
            p_pBuf += 16;
            *(p_pBuf++) = pContract->m_unDstTLPort >> 8;
            *(p_pBuf++) = pContract->m_unDstTLPort;
            *p_ucSize += 18;
        }
    }
}

/**********************************
 * UAP_GetAperiodicContracts
 **********************************/
static void UAP_GetAperiodicContracts(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize)
{
    DMO_CONTRACT_ATTRIBUTE * pContract = g_stDMO.m_aContractTbl;
        
    *p_ucSize = 0;
    
    for( ; pContract < g_stDMO.m_aContractTbl + g_stDMO.m_ucContractNo; pContract++ )
    {
        if( SRVC_APERIODIC_COMM == pContract->m_ucServiceType &&
            (ISA100_START_PORTS + UAP_APP1_ID) == pContract->m_unSrcTLPort)
        {
            *(p_pBuf++) = pContract->m_unContractID >> 8;
            *(p_pBuf++) = pContract->m_unContractID;
            *(p_pBuf++) = pContract->m_ucContractStatus;

#warning  "No available phase for the unscheduled/aperiodic communication contract"
            *(p_pBuf++) = 0; //pContract->m_stBandwidth.m_stPeriodic.m_ucPhase;
            *(p_pBuf++) = 0;
            *p_ucSize += 5;
        }
    }
}

/**********************************
 * UAP_GetObjectTable
 **********************************/
static void UAP_GetObjectTable(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize)
{
    *p_ucSize = 0;
    
    for( uint8 ucIdx = 0; ucIdx < UAP_OBJ_NO; ucIdx++ )
    {
        *(p_pBuf++) = g_pstUAPObjectTable[ucIdx].m_unObjectId >> 8;
        *(p_pBuf++) = g_pstUAPObjectTable[ucIdx].m_unObjectId;
        memcpy(p_pBuf, &g_pstUAPObjectTable[ucIdx].m_ucObjectType, 3);
        p_pBuf += 3;
        *p_ucSize += 5;
    }
}

#endif // _UAP_TYPE == 0
