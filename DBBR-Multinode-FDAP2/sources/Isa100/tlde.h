////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Ion Ticus
/// Date:         January 2008
/// Description:  This file holds definitions of transport layer data entity from ISA100 standard
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _NIVIS_TLDE_H_
#define _NIVIS_TLDE_H_

#include "tlme.h"
#include "nlde.h"


#include "../text/text_convertor.h"

#define ISA100_START_PORTS 0xF0B0
#define ISA100_DMAP_PORT  (ISA100_START_PORTS)
#define ISA100_APP_PORT   (ISA100_START_PORTS+1)

#define UDP_ENCODING_PLAIN     0xF3 //b11110011 // check sum present, 4 bit ports
#define UDP_ENCODING_ENCRYPTED 0xF7 //b11110111 // check sum elided, 4 bit ports

#define UDP_PROTOCOL_CODE      17

extern const uint8 c_aLocalLinkIpv6Prefix[8];
extern EUI64_ADDR g_aucScriptServerEUI64;

#define TLDE_DISCARD_ELIGIBILITY_MASK BIT4
#define TLDE_ECN_MASK                 (BIT5 | BIT6) 
#define TLDE_PRIORITY_MASK            (BIT0 | BIT1)

void TLDE_DATA_Request(   const uint8* p_pEUI64DestAddr,
                          uint16       p_unContractID,
                          uint8        p_ucPriorityAndFlags,
                          uint16       p_unAppDataLen,
                          void *       p_pAppData,
                          HANDLE       p_appHandle,
                          uint8        p_uc4BitSrcPort,
                          uint8        p_uc4BitDstPort );

// -------- Callback functions from Network level to Transport Level -----------------------------


void NLDE_DATA_Confirm (  HANDLE       p_hTLHandle,
                          uint8        p_ucStatus );

void NLDE_DATA_Indication ( uint8         p_ucSrcDstAddrLen,
                         const uint8 * p_pSrcDstAddr,
                         void * p_pTLData , uint16 p_unTLDataLen );


uint16 TLDE_BuildPaiload( WCITXT_RCV_MSG * p_pWciMsg ,
                          uint8        * p_pRspPayload );


// -------- Callback functions from Transport Level to Application Layer -----------------------------

#endif // _NIVIS_TLDE_H_ */

