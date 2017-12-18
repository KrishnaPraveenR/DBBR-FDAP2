////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Ion Ticus
/// Date:         January 2008
/// Description:  This file holds definitions of the Network Layer Data Entity
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _NIVIS_NLDE_H_
#define _NIVIS_NLDE_H_

#include "../typedef.h"
#include "config.h"

#define HC_UDP_FULL_COMPRESS        0xF0 // b11110000 - full compress 
#define HC_UDP_PARTIAL_COMPRESS     0xE0 // b11100000 - checksum is present elided

#define NLDE_PRIORITY_MASK          0x0F

#define HANDLE_NL_BIG_MSG 0xF000
#define HANDLE_NL_BBR     0xF001

typedef struct
{
    uint8   m_ucVersionAndTrafficClass;
    uint8   m_aFlowLabel[3];
    uint8   m_aPayloadSize[2];
    uint8   m_ucNextHeader;
    uint8   m_ucHopLimit;
    uint8   m_aIpv6SrcAddr[16];
    uint8   m_aIpv6DstAddr[16];
    uint8   m_aSrcPort[2];
    uint8   m_aDstPort[2];
    uint8   m_aUdpLength[2];
    uint8   m_aUdpCheckSum[2];    
} NWK_IPV6_UDP_HDR;

void DLDE_Data_Indicate ( uint8         p_ucSrcDstAddrLen,
                         const uint8 * p_pSrcDstAddr,
                         uint8         p_ucPayloadLength,
			             void *  p_pPayload );
void NLDE_Init(void);
void DLDE_Data_Confirm (HANDLE p_hHandle, uint8 p_ucLocalStatus);

 // -------- Callback functions from DLL to Network level ----------------------------------------

#endif // _NIVIS_NLDE_H_ 

