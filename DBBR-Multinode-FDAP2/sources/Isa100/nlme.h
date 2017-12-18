////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Adrian Simionescu
/// Date:         January 2008
/// Description:  This file holds definitions of the Network Layer Management Entity
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _NIVIS_NLME_H_
#define _NIVIS_NLME_H_

#include "config.h"
#include "dmap_dmo.h"


typedef enum {
  NLME_ATRBT_ID_BBR_CAPABLE = 1,
  NLME_ATRBT_ID_DL_CAPABLE,
  NLME_ATRBT_ID_SHORT_ADDR,
  NLME_ATRBT_ID_IPV6_ADDR,
  NLME_ATRBT_ID_ROUTE_TABLE,
  NLME_ATRBT_ID_DEFAULT_ROUTE_FLAG,
  NLME_ATRBT_ID_DEFAULT_ROUTE_ID,
  NLME_ATRBT_ID_CONTRACT_TABLE,
  NLME_ATRBT_ID_ATT_TABLE,
  NLME_ATRBT_ID_MAX_NSDU_SIZE,
  NLME_ATRBT_ID_FRAG_REASSEMBLY_TIMEOUT,
  NLME_ATRBT_ID_FRAG_DATAGRAM_TAG,
  
  NLME_ATRBT_ID_ROUTE_TBL_META,
  NLME_ATRBT_ID_CONTRACT_TBL_META,
  NLME_ATRBT_ID_ATT_TBL_META,
  
  NLME_ATRBT_ID_ALERT_DESC,
  
  NLME_ATRBT_ID_NO
} NLME_ATTRIBUTE_ID;

typedef enum {
  NLME_ALERT_DEST_UNREACHABLE = 1,
  NLME_ALERT_FRAG_ERROR, 
  NLME_ALERT_REASS_TIMEOUT, 
  NLME_ALERT_HOP_LIMIT_REACHED, 
  NLME_ALERT_HEADER_ERROR, 
  NLME_ALERT_NO_ROUTE, 
  NLME_ALERT_OUT_OF_MEM, 
  NLME_ALERT_NPDU_LEN_TOO_LONG 
} NLME_ALERT_TYPE;

typedef struct
{
    uint8 * m_pValue;
    uint8   m_ucSize;
    uint8   m_ucIndexSize;
    uint8   m_ucIsReadOnly;
} NLME_ATTRIBUTES_PARAMS;

typedef struct
{   
  IPV6_ADDR     m_aDestAddress; 
  IPV6_ADDR     m_aNextHopAddress;
  uint8         m_ucNWK_HopLimit;                        // Default value : 64
  uint8         m_bOutgoingInterface;                 // Valid values: 0-DL, 1-Backbone
}NLME_ROUTE_ATTRIBUTES;

typedef struct
{  
  uint16          m_unContractID;
//  IPV6_ADDR       m_aSourceAddress; not used since I'm the source all time
  IPV6_ADDR       m_aDestAddress; 
  uint8           m_bUnused : 5;             
  uint8           m_bIncludeContractFlag : 1;               // 0 = Don't Include, 1 = Include
  uint8           m_bPriority : 2;                          // Valid values: 00, 01, 10, 11
}NLME_CONTRACT_ATTRIBUTES;

typedef struct
{   
  IPV6_ADDR     m_aIPV6Addr;
  uint8         m_aShortAddress[2];
} NLME_ADDR_TRANS_ATTRIBUTES;

typedef struct
{
  uint16 m_unFragDatagramTag;      
  uint8  m_ucFragReassemblyTimeout;                     // Valid value set: 1 - 64   
  uint8  m_ucEnableDefaultRoute;
  uint8  m_aDefaultRouteEntry[16];                         // Index into the RouteTable that is the default route 

  uint8  m_ucContractNo;
  uint8  m_ucAddrTransNo;
    
#if (MAX_ROUTES_NO > 0)
  uint8  m_ucRoutesNo;
  NLME_ROUTE_ATTRIBUTES       m_aRoutesTable[MAX_ROUTES_NO];
#endif  
  
  NLME_CONTRACT_ATTRIBUTES    m_aContractTable[MAX_CONTRACT_NO];
  NLME_ADDR_TRANS_ATTRIBUTES  m_aAddrTransTable[MAX_ADDR_TRANSLATIONS_NO];
  
#if(DEVICE_TYPE == DEV_TYPE_MC13225) && defined( BACKBONE_SUPPORT )
  uint8   m_ucToEthRoutesNo;
  uint8   m_ucDefaultIsDLL;
  uint16  m_aToEthRoutes[32];
#endif
  
  uint8  m_ucAlertDesc;
  
} NLME_ATRIBUTES;



    void NLME_Init( void );

    uint8 NLME_SetRow( uint8         p_ucAttributeID, 
                       uint32        p_ulTaiCutover, 
                       const uint8 * p_pData,
                       uint8         p_ucDataLen );
    
    uint8 NLME_GetRow( uint8         p_ucAttributeID, 
                       const uint8 * p_pIdxData,
                       uint8         p_ucIdxDataLen,
                       uint8       * p_pData,
                       uint8       * p_pDataLen );
    
    uint8 NLME_DeleteRow( uint8         p_ucAttributeID, 
                          uint32        p_ulTaiCutover, 
                          const uint8 * p_pIdxData,
                          uint8         p_ucIdxDataLen );
    
    NLME_CONTRACT_ATTRIBUTES * NLME_FindContract(  uint16 p_unContractID );
    uint8 NLME_AddDmoContract( const DMO_CONTRACT_ATTRIBUTE * p_pDmoContract );
    void NLME_Alert( uint8 p_ucAlertValue, const uint8 * p_pNpdu, uint8 p_ucNpduLen );
    
#if (MAX_ROUTES_NO > 0) // NL routing support
    NLME_ROUTE_ATTRIBUTES * NLME_FindDestinationRoute( uint8*   p_ucDestAddress );
#else     // not NL routing support
    #define NLME_FindDestinationRoute(...) NULL
#endif // (MAX_ROUTES_NO > 0)
    
    NLME_ADDR_TRANS_ATTRIBUTES * NLME_FindATTByShortAddr( const uint8 * p_pShortAddr );
    NLME_ADDR_TRANS_ATTRIBUTES * NLME_FindATT( const uint8 * p_pIpv6Addr );
                            
extern NLME_ATRIBUTES g_stNlme;

  #define g_unShortAddr   g_stDMO.m_unShortAddr
  #define g_aIPv6Address  g_stDMO.m_auc128BitAddr

#if defined (BACKBONE_SUPPORT) 
  uint8 NLME_IsInSameSubnet( uint16 p_unDestAddr );
#else
  #define NLME_IsInSameSubnet(...) 1 // always true on device
#endif

#endif // _NIVIS_NLME_H_

