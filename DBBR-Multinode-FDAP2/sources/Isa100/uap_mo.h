////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Mircea Vlasin 
/// Date:         June 2009
/// Description:  This file holds the definitions of the MO object of the UAP
/// Changes:      Created
/// Revisions:    1.0
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NIVIS_UAP_MO_H_
#define _NIVIS_UAP_MO_H_

#include "../typedef.h"
#include "aslde.h"
#include "dmap_utils.h"

enum
{
    UAPMO_RESERVED,
    UAPMO_VERSION,
    UAPMO_STATE,
    UAPMO_COMMAND,
    UAPMO_MAXRETRIES,
    UAPMO_NUM_UNSCHED_COORESPONDENT,
    UAPMO_TABLE_UNSCHED_COORESPONDENT,
    UAPMO_TABLE_CONTRACT_UNSCHED_COORESPONDENT,
    UAPMO_NUM_OBJECTS,
    UAPMO_TABLE_OBJECTS,
    UAPMO_STATIC_REVISION,

    UAPMO_ATTR_NO                  
}; // UAPMO_ATTRIBUTES

enum UAP_STATES {
    UAP_INACTIVE,
    UAP_ACTIVE,
    UAP_FAILED
};

typedef struct
{
    IPV6_ADDR m_pucIPv6Addr;
    uint16    m_unTLPort;
} UNSCHEDULED_CORRESPONDENT;

typedef struct
{
    uint16 m_unObjectId;
    uint8  m_ucObjectType;
    uint8  m_ucObjSubType;
    uint8  m_ucVendorSubType;
}OBJECT_ID_AND_TYPE;

typedef struct{
    uint16  m_unContractID;
    uint8   m_ucContractStatus;
    uint16  m_unActualPhase;
}COMMUNICATION_CONTRACT_DATA;


#define DEFAULT_UAP_RETRY_NO    3

#if ( _UAP_TYPE == UAP_TYPE_SIMPLE_API )
    extern const uint8 g_ucMaxUAPRetries;
    extern const DMAP_FCT_STRUCT c_aUapMoAttributes[UAPMO_ATTR_NO];

    #define UAPMO_Read(p_unAttrID,p_punBufferSize,p_pucRspBuffer) \
                DMAP_ReadAttr(p_unAttrID,p_punBufferSize,p_pucRspBuffer,c_aUapMoAttributes,UAPMO_ATTR_NO)
    
    #define UAPMO_Write(p_unAttrID,p_ucBufferSize,p_pucBuffer)   \
                DMAP_WriteAttr(p_unAttrID,p_ucBufferSize,p_pucBuffer,c_aUapMoAttributes,UAPMO_ATTR_NO)

#else
    #define g_ucMaxUAPRetries DEFAULT_UAP_RETRY_NO
    
    #define UAPMO_Read(...) SFC_INVALID_OBJ_ID
    #define UAPMO_Write(...) SFC_INVALID_OBJ_ID
#endif // _UAP_TYPE == 0
    
#endif //_NIVIS_UAP_MO_H_
