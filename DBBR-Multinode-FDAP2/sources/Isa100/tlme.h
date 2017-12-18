////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Adrian Simionescu 
/// Date:         January 2008
/// Description:  This file holds definitions of transport layer management entity from ISA100 standard
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _NIVIS_TLME_H_
#define _NIVIS_TLME_H_

#include "config.h"

#define MAX_TSDU_SIZE    88   // Valid value set: 13 – 65535  

typedef uint32 TPDU_COUNTER;

typedef enum {
  TLME_ILLEGAL_USE_OF_PORT = 0,
  TLME_TPDU_ON_UNREG_PORT, 
  TLME_TPDU_OUT_OF_SEC_POL  
} TLME_ALERT_TYPE;

#define TLME_ALERT_TYPE_NO (TLME_TPDU_OUT_OF_SEC_POL+1)

typedef struct
{
  uint16 m_unNbActivePorts;   
  uint16 m_unFirstActivePort;
  uint16 m_unLastActivePort;
} TLME_PORT_RANGE_INFO; 

typedef enum
{
    TLME_PORT_TPDU_IN_OK,
    TLME_PORT_TPDU_IN,
    TLME_PORT_TPDU_OUT_OK,
    TLME_PORT_TPDU_OUT,

    TLME_PORT_ACTION_NO
} PORT_ACTION;

typedef enum 
{
    TLME_ATRBT_MAX_TSDU_SIZE = 1,
    TLME_ATRBT_MAX_NB_OF_PORTS,
    TLME_ATRBT_PORT_TPDU_IN,            // TLME_ATRBT_PORT_TPDU_IN
    TLME_ATRBT_PORT_TPDU_IN_REJECTED,   // TLME_ATRBT_PORT_TPDU_IN_REJECTED
    TLME_ATRBT_PORT_TPDU_IN_OK,         // TLME_ATRBT_PORT_TSDU_OUT
    TLME_ATRBT_PORT_TPDU_OUT,           // TLME_ATRBT_PORT_TSDU_IN
    TLME_ATRBT_PORT_TPDU_OUT_REJECTED,  // TLME_ATRBT_PORT_TSDU_IN_REJECTED
    TLME_ATRBT_PORT_TPDU_OUT_OK,        // TLME_ATRBT_PORT_TPDU_OUT
    TLME_ATRBT_ILLEGAL_USE_OF_PORT,
    TLME_ATRBT_TPDU_ON_UNREG_PORT,
    TLME_ATRBT_TPDU_OUT_OF_SEC_POL,
    
    TLME_ATRBT_ID_NO        
} TLME_ATTRIBUTE_ID;

typedef enum
{
  TLME_RESET_METHID = 1,
  TLME_HALT_METHID,
  TLME_PORT_RANGE_INFO_METHID,
  TLME_GET_PORT_INFO_METHID,
  TLME_GET_NEXT_PORT_INFO_METHID,
    
  TLME_METHOD_NO    
} TLME_METHOD_IDS;


typedef struct
{
  uint8 m_aIpv6Addr[16];
  uint16 m_unPortNb;   
  uint32 m_unUID;
  TPDU_COUNTER m_aTPDUCount[TLME_PORT_ACTION_NO];
} TLME_PORT_INFO; 

typedef struct
{
    uint16 m_unMaxTSDUsize;
    uint16 m_unMaxNbOfPorts;
    
    TPDU_COUNTER m_aTPDUCount[TLME_PORT_ACTION_NO];        
    uint8        m_aAlertDesc[ TLME_ALERT_TYPE_NO ];    
    
    uint8 m_ucNbOfPorts;
    TLME_PORT_INFO m_aPortInfo[MAX_NB_OF_PORTS];    
} TLME_STRUCT;


extern TLME_STRUCT g_stTlme;

void TLME_Init(void);
void TLME_Reset( const uint8 p_ucForced );
void TLME_Halt( const uint8* p_pDeviceAddr, uint16 p_unPortNr );
void TLME_SetPortInfo( const uint8* p_pDeviceAddr, uint16 p_unPortNb, PORT_ACTION p_ucAction );
uint8 TLME_GetPortInfo( const uint8* p_pDeviceAddr, uint16 p_unPortNb, TLME_PORT_INFO * p_pPortInfo );
uint8 TLME_GetNextPortInfo( const uint8* p_pDeviceAddr, uint16 p_unPortNb, TLME_PORT_INFO * p_pPortInfo );
void  TLME_GetPortRangeInfo( const uint8* p_pDeviceAddr, TLME_PORT_RANGE_INFO * p_pPortRange );
void  TLME_Alert( TLME_ALERT_TYPE p_unAlertType, const uint8 * p_pData, uint16 p_unDataSize );

uint8 TLME_SetRow( uint8         p_ucAttributeID, 
                   uint32        p_ulTaiCutover, 
                   const uint8 * p_pData,
                   uint8         p_ucDataLen );

uint8 TLME_GetRow( uint8         p_ucAttributeID, 
                   const uint8 * p_pIdxData,
                   uint8         p_ucIdxDataLen,
                   uint8       * p_pData,
                   uint8       * p_pDataLen );

#endif // _NIVIS_TLME_H_

