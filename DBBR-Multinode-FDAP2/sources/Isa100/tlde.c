////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Ion Ticus
/// Date:         January 2008
/// Description:  Implements transport layer data entity from ISA100 standard
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>

#include "tlde.h"
#include "tlme.h"
#include "slme.h"
#include "nlde.h"
#include "nlme.h"
#include "aslsrvc.h"
#include "mlde.h"
#include "mlsm.h"
#include "dmap.h"
#include "provision.h"
#include "../asm.h"
#include "slme.h"

#if (DEVICE_TYPE == DEV_TYPE_MC13225)
    #include "uart_link.h"
#endif

#ifdef UT_ACTIVED  // unit testing support
  #if( (UT_ACTIVED == UT_TL_ONLY) || (UT_ACTIVED == UT_NL_TL) )
      #include "../UnitTests/unit_test_TLDE.h"
      UINT_TEST_TLDE_STRUCT g_stUnitTestTLDE;
  #endif
#endif // UT_ACTIVED

#if DUPLICATE_ARRAY_SIZE > 0
    uint8  g_ucDuplicateIdx; 
    uint32 g_aDuplicateHistArray[DUPLICATE_ARRAY_SIZE]; 
    uint8  TLDE_IsDuplicate( const uint8 * p_pMIC );
#else // no duplicate detection support
    #define TLDE_IsDuplicate(...) 0
#endif

#if ( _UAP_TYPE != 0 )
    extern uint8 g_ucUapId;
#endif
    
  typedef struct
  {
      uint8 m_aIpv6SrcAddr[16];    
      uint8 m_aIpv6DstAddr[16];    
      uint8 m_aPadding[3]; 
      uint8 m_ucNextHdr; // 17
      uint8 m_aSrcPort[2];    
      uint8 m_aDstPort[2];   
      uint8 m_aTLSecurityHdr[4];
  } IPV6_PSEUDO_HDR; // encrypted will avoid check sum since is part of MIC

  typedef union
  {
      uint32            m_ulAligned;
      IPV6_PSEUDO_HDR   m_st;
  } IPV6_ALIGNED_PSEUDO_HDR;
  
#pragma pack(1)
  typedef struct
  {
      uint8 m_ucUdpEncoding;    
      uint8 m_ucUdpPorts;    
      uint8 m_aUdpCkSum[2];        
      uint8 m_ucSecurityCtrl;    // must be SECURITY_CTRL_ENC_NONE
      uint8 m_ucTimeAndCounter;    
      uint8 m_ucCounter;    
  } TL_HDR_PLAIN; // plain request mandatory check sum
  
  typedef struct
  {
      uint8 m_ucUdpEncoding;    
      uint8 m_ucUdpPorts;    
      uint8 m_ucSecurityCtrl;   // must be SECURITY_ENC_MIC_32  
      uint8 m_ucTimeAndCounter;    
      uint8 m_ucCounter;    
  } TL_HDR_ENC_SINGLE_KEY; // encrypted will avoid check sum since is part of MIC

  typedef struct
  {
      uint8 m_ucUdpEncoding;    
      uint8 m_ucUdpPorts;    
      uint8 m_ucSecurityCtrl;   // must be SECURITY_CTRL_ENC_MIC_32  
      uint8 m_ucKeyIdx;    
      uint8 m_ucTimeAndCounter;    
      uint8 m_ucCounter;    
  } TL_HDR_ENC_MORE_KEYS; // encrypted will avoid check sum since is part of MIC
  
  typedef union
  {
      TL_HDR_PLAIN          m_stPlain; 
      TL_HDR_ENC_SINGLE_KEY m_stEncOneKey; 
      TL_HDR_ENC_MORE_KEYS  m_stEncMoreKeys; 
  } TL_HDR;
  
#pragma pack()

#define TLDE_TIME_MAX_DIFF 2
  
  
const uint8 c_aLocalLinkIpv6Prefix[8] = {0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8 c_aIpv6ConstPseudoHdr[8] = { 0, 0, 0, UDP_PROTOCOL_CODE, 0xF0, 0xB0, 0xF0, 0xB0 };
//uint8 g_ucMsgIncrement;
EUI64_ADDR g_aucScriptServerEUI64;

uint8 TLDE_decryptTLPayload( const SLME_KEY * p_pKey,
                             uint32   p_ulIssueTime,
                             const IPV6_PSEUDO_HDR * p_pAuth,
                             uint16                  p_unSecHeaderLen,
                             uint16   p_unTLDataLen,
                             void *   p_pTLData );

uint8 TLDE_encryptTLPayload( uint16                  p_unAppDataLen, 
                            void *                  p_pAppData, 
                            const IPV6_PSEUDO_HDR * p_pAuth, 
                            uint16                  p_unSecHeaderLen,
                            const SLME_KEY *        p_pKey, 
                            uint32                  p_ulCrtTai );

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  It is used by the Application Layer to send an packet to one device
/// @param  p_pEUI64DestAddr     - final EUI64 destination
/// @param  p_unContractID       - contract ID
/// @param  p_ucPriorityAndFlags - message priority + DE + ECN
/// @param  p_unAppDataLen  - APP data length
/// @param  p_pAppData      - APP data buffer
/// @param  p_appHandle     - app level handler
/// @param  p_uc4BitSrcPort - source port (16 bit port supported only)
/// @param  p_uc4BitDstPort - destination port (16 bit port supported only)
/// @return none
/// @remarks
///      Access level: user level\n
///      Context: After calling of that function a NLDE_DATA_Confirm has to be received.\n
///      Obs: !!! p_pAppData can be altered by TLDE_DATA_Request and must have size at least TL_MAX_PAYLOAD+16\n
///      On future p_uc4BitSrcPort must be maped via a TSAP id to a port (something like socket id) \n
//       but that is not clear specified on ISA100
////////////////////////////////////////////////////////////////////////////////////////////////////
void TLDE_DATA_Request(   const uint8* p_pEUI64DestAddr,
                          uint16       p_unContractID,
                          uint8        p_ucPriorityAndFlags,
                          uint16       p_unAppDataLen,
                          void *       p_pAppData,
                          HANDLE       p_appHandle,
                          uint8        p_uc4BitSrcPort,
                          uint8        p_uc4BitDstPort )
{  
      WCI_Log( LOG_M_TL, TLOG_DataReq, sizeof(p_unContractID), &p_unContractID,
                                       sizeof(p_ucPriorityAndFlags), &p_ucPriorityAndFlags,
                                       (uint8)p_unAppDataLen,  (uint8 *)p_pAppData,
                                       sizeof(p_appHandle), &p_appHandle,
                                       sizeof(p_uc4BitSrcPort), &p_uc4BitSrcPort,
                                       sizeof(p_uc4BitDstPort), &p_uc4BitDstPort );
 
      // send to WCI script server
      
      WCITXT_TX_MSG stMsg;
      
      stMsg.m_ucMsgType = WCITXT_MSG_TO_BBR_AS_APP;
      stMsg.m_u.m_stCfg.m_stApp.m_ucLen = p_unAppDataLen;
      memcpy(stMsg.m_u.m_stCfg.m_stApp.m_aBuff, p_pAppData, p_unAppDataLen );
      
      // add to queue
      WCI_AddMessage( sizeof(stMsg.m_u.m_stCfg)+4, (const uint8*)&stMsg );
      
      NLDE_DATA_Confirm( p_appHandle, SFC_SUCCESS );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  It is used by the Application Layer to send an packet to one device
/// @param  p_pEUI64DestAddr     - final EUI64 destination
/// @param  p_unContractID       - contract ID
/// @param  p_ucPriorityAndFlags - message priority + DE + ECN
/// @param  p_unAppDataLen  - APP data length
/// @param  p_pAppData      - APP data buffer
/// @param  p_appHandle     - app level handler
/// @param  p_uc4BitSrcPort - source port (16 bit port supported only)
/// @param  p_uc4BitDstPort - destination port (16 bit port supported only)
/// @return none
/// @remarks
///      Access level: user level\n
///      Context: After calling of that function a NLDE_DATA_Confirm has to be received.\n
///      Obs: !!! p_pAppData can be altered by TLDE_DATA_Request and must have size at least TL_MAX_PAYLOAD+16\n
///      On future p_uc4BitSrcPort must be maped via a TSAP id to a port (something like socket id) \n
//       but that is not clear specified on ISA100
////////////////////////////////////////////////////////////////////////////////////////////////////
uint16 TLDE_BuildPaiload( WCITXT_RCV_MSG * p_pWciMsg ,
                          uint8        * p_pRspPayload )
{
  TL_HDR                  stHdr;
  IPV6_ALIGNED_PSEUDO_HDR stPseudoHdr;
  
  uint16 unHdrLen;
  uint8 ucKeysNo;
  const SLME_KEY * pKey = NULL;
  
  memcpy( stPseudoHdr.m_st.m_aPadding, c_aIpv6ConstPseudoHdr, sizeof(c_aIpv6ConstPseudoHdr) ) ;
  stPseudoHdr.m_st.m_aSrcPort[1] = p_pWciMsg->m_u.m_stRf.m_aIPv6SrcPort[1];
  stPseudoHdr.m_st.m_aDstPort[1] = p_pWciMsg->m_u.m_stRf.m_aIPv6DstPort[1];

  stHdr.m_stPlain.m_ucUdpPorts =    (p_pWciMsg->m_u.m_stRf.m_aIPv6SrcPort[1] << 4) 
                                  | (p_pWciMsg->m_u.m_stRf.m_aIPv6DstPort[1] & 0x0F); // UDP compresed hdr
  
  memcpy( stPseudoHdr.m_st.m_aIpv6DstAddr, p_pWciMsg->m_u.m_stRf.m_aIPv6DstAddr, 16 );
  memcpy( stPseudoHdr.m_st.m_aIpv6SrcAddr, p_pWciMsg->m_u.m_stRf.m_aIPv6SrcAddr, 16 );
  
      
  if(  p_pWciMsg->m_u.m_stRf.m_ucTLEncryption != SECURITY_NONE )
  {
        //the BBR is not Endpoint, so the SrcPort for a SessionKey is represented by the DUT Port  
        pKey = SLME_FindTxKey( stPseudoHdr.m_st.m_aIpv6DstAddr, 
                               (p_pWciMsg->m_u.m_stRf.m_aIPv6DstPort[1] << 4) | (p_pWciMsg->m_u.m_stRf.m_aIPv6SrcPort[1] & 0x0F), 
                               &ucKeysNo );
        if( !pKey )
        {
//                TLDE_DATA_Confirm( p_appHandle, SFC_NO_KEY );
            WCITXT_TX_MSG stMsg;
            
            stMsg.m_ucMsgType = WCITXT_MSG_TO_BBR_AS_ERR;
            stMsg.m_u.m_stErr.m_ucSeverity = LOG_ERROR;
            stMsg.m_u.m_stErr.m_ucLevel = LOG_M_TL;
            
            stMsg.m_u.m_stErr.m_ucErrorCode = 2;
            stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "Tx Session Key Not Found" );
            
            // add to queue
            WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg );
            return 0;
        }
  }


  uint8 *  p_pAppData = p_pWciMsg->m_stApp.m_aBuff;
  uint16   p_unAppDataLen = p_pWciMsg->m_stApp.m_ucLen;
  
  uint32 ulCrtTai = MLSM_GetCrtTaiSec();
  uint8  ucTimeAndCounter = (uint8)((ulCrtTai << 2) | g_stTAI.m_uc250msStep); // add 250ms time sense
  
  g_ucMsgIncrement++;    
          
  if( !pKey ) // not encryption
  {
      stHdr.m_stPlain.m_ucUdpEncoding = UDP_ENCODING_PLAIN;
      stPseudoHdr.m_st.m_aTLSecurityHdr[0] = stHdr.m_stPlain.m_ucSecurityCtrl = SECURITY_NONE;
      stPseudoHdr.m_st.m_aTLSecurityHdr[1] = stHdr.m_stPlain.m_ucTimeAndCounter =  ucTimeAndCounter;
      stPseudoHdr.m_st.m_aTLSecurityHdr[2] = stHdr.m_stPlain.m_ucCounter = g_ucMsgIncrement;          
      stPseudoHdr.m_st.m_aTLSecurityHdr[3] = (p_unAppDataLen ? *((uint8*)p_pAppData) : 0 );        
            
      uint32 ulUdpCkSum = p_unAppDataLen + 3 + 8; // security + mic + UDP header length
      // make happy check sum over UDP and add twice udp payload size
      *(uint16*)stPseudoHdr.m_st.m_aPadding = __swap_bytes(ulUdpCkSum);      
      ulUdpCkSum = IcmpInterimChksum( (const uint8*)&stPseudoHdr.m_st, sizeof(stPseudoHdr.m_st), ulUdpCkSum );
            
      if( p_unAppDataLen > 1 )
      {      
          ulUdpCkSum = IcmpInterimChksum( p_pAppData +1 , p_unAppDataLen - 1 , ulUdpCkSum );
      }
            
      ulUdpCkSum = IcmpGetFinalCksum( ulUdpCkSum );
      
      stHdr.m_stPlain.m_aUdpCkSum[0] = ulUdpCkSum >> 8;
      stHdr.m_stPlain.m_aUdpCkSum[1] = ulUdpCkSum & 0xFF;
      
      if( g_ucWrongUDPCSCount )
      {
          g_ucWrongUDPCSCount--;
          //generate wrong UDP Checksum
          stHdr.m_stPlain.m_aUdpCkSum[0] += 1;
      }
      unHdrLen = sizeof( stHdr.m_stPlain );
  }
  else // encryption
  {
      stHdr.m_stPlain.m_ucUdpEncoding = UDP_ENCODING_ENCRYPTED;
      
      if( ucKeysNo > 1 )
      {
          //stHdr.m_stEncMoreKeys.m_ucSecurityCtrl = SECURITY_CTRL_ENC_MIC_32;
          stHdr.m_stEncMoreKeys.m_ucSecurityCtrl = (pKey->m_ucSecurityCtrl | (KEY_ID_MODE << KEY_ID_MODE_OFFSET));
          stHdr.m_stEncMoreKeys.m_ucKeyIdx = pKey->m_ucKeyID;    
          stHdr.m_stEncMoreKeys.m_ucTimeAndCounter = ucTimeAndCounter; // add 250ms time sense
          stHdr.m_stEncMoreKeys.m_ucCounter = g_ucMsgIncrement;              
            
          unHdrLen = sizeof( stHdr.m_stEncMoreKeys );
      }
      else
      {
          stHdr.m_stEncOneKey.m_ucSecurityCtrl = pKey->m_ucSecurityCtrl;
          stHdr.m_stEncOneKey.m_ucTimeAndCounter = ucTimeAndCounter; // add 250ms time sense
          stHdr.m_stEncOneKey.m_ucCounter = g_ucMsgIncrement;              
            
          unHdrLen = sizeof( stHdr.m_stEncOneKey );
      }
      
      memcpy( stPseudoHdr.m_st.m_aTLSecurityHdr, &stHdr.m_stEncMoreKeys.m_ucSecurityCtrl, unHdrLen-2 );
      
      TLDE_encryptTLPayload(p_unAppDataLen, p_pAppData, &stPseudoHdr.m_st, unHdrLen, pKey, ulCrtTai );

      p_unAppDataLen += GetMicSize(TMIC,pKey->m_ucSecurityCtrl); // add MIC_32 size
  }
  
  memcpy( p_pRspPayload, &stHdr, unHdrLen );
  memcpy( p_pRspPayload + unHdrLen, p_pAppData, p_unAppDataLen );
  return (unHdrLen+p_unAppDataLen);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  It is invoked by Network Layer to notify about result code of a data request
/// @param  p_hTLHandle - transport layer handler (used to find correspondent request)
/// @param  p_ucStatus  - request status
/// @return none
/// @remarks
///      Access level: Interrupt level\n
///      Context: After any NLDE_DATA_Request
////////////////////////////////////////////////////////////////////////////////////////////////////
void NLDE_DATA_Confirm (  HANDLE       p_hTLHandle,
                          uint8        p_ucStatus )
{
  WCI_Log( LOG_M_NL, NLOG_Confirm, sizeof(p_hTLHandle), &p_hTLHandle, sizeof(p_ucStatus), &p_ucStatus );

#if defined( BACKBONE_SUPPORT ) 
  #if (DEVICE_TYPE == DEV_TYPE_AP91SAM7X512)
    if( p_hTLHandle == HANDLE_NL_BBR ) // from ETH
    {
        return;
    }
  #elif (DEVICE_TYPE == DEV_TYPE_MC13225)
      if( (p_hTLHandle & 0x8000) == 0 ) // from ETH
      {
          uint8 aConfMsg[3];
          aConfMsg[0] = (p_hTLHandle >> 8);
          aConfMsg[1] = (uint8)p_hTLHandle;
          aConfMsg[2] = p_ucStatus;
  
          UART_LINK_AddNwkConf( aConfMsg );
          return;
      }
    p_hTLHandle &= 0x7FFF;
  #endif      
#endif // BACKBONE_SUPPORT
        
  TLDE_DATA_Confirm ( p_hTLHandle, p_ucStatus );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  It is invoked by Network Layer to notify transport layer about new packet
/// @param  p_pSrcAddr  - The IPv6 or EUI64 source address (if first byte is 0xFF -> last 8 bytes are EUI64)  
/// @param  p_unTLDataLen - network layer payload length of received message
/// @param  p_pTLData     - network layer payload data of received message
/// @param  m_ucPriorityAndFlags - message priority + DE + ECN
/// @return none
/// @remarks
///      Access level: User level\n
///      Obs: p_pTLData will be altered on this function
////////////////////////////////////////////////////////////////////////////////////////////////////
#define MAX_MIC_SIZE 16
void NLDE_DATA_Indication ( uint8         p_ucSrcDstAddrLen,
                         const uint8 * p_pSrcDstAddr,
                         void * p_pTLData , uint16 p_unTLDataLen )
{
  uint8  ucTimeTraveling;
  const SLME_KEY * pKey = NULL;
  uint32 ulCrtTime;
  //unsigned char micSize = GetMicSize(TMIC);
  //uint8 work[ MAX_TSDU_SIZE + 6 + MAX_MIC_SIZE ]; // TL_HDR size + MIC
 
  IPV6_ALIGNED_PSEUDO_HDR stPseudoHdr;
  
  WCITXT_TX_MSG stMsg;
  stMsg.m_ucMsgType = WCITXT_MSG_TO_BBR_AS_ERR;
  stMsg.m_u.m_stErr.m_ucSeverity = LOG_ERROR;
  
  memcpy( stPseudoHdr.m_st.m_aPadding, c_aIpv6ConstPseudoHdr, sizeof(c_aIpv6ConstPseudoHdr) ) ;
  stPseudoHdr.m_st.m_aSrcPort[1] |= ((TL_HDR*)p_pTLData)->m_stPlain.m_ucUdpPorts >> 4;
  stPseudoHdr.m_st.m_aDstPort[1] |= ((TL_HDR*)p_pTLData)->m_stPlain.m_ucUdpPorts & 0x0F;;
    
   RefreshKeyTable();
  if(  p_ucSrcDstAddrLen == 8 ) // MAC address -> join request -> use local address
  {
      memcpy( stPseudoHdr.m_st.m_aIpv6SrcAddr,   c_aLocalLinkIpv6Prefix, 8 );
      memcpy( stPseudoHdr.m_st.m_aIpv6SrcAddr+8, p_pSrcDstAddr, 8 );
      memcpy( stPseudoHdr.m_st.m_aIpv6DstAddr,   c_aLocalLinkIpv6Prefix, 8 ); 
      memcpy( stPseudoHdr.m_st.m_aIpv6DstAddr+8, c_oEUI64BE, 8 ); 
  }
  else   
  {
      NLME_ADDR_TRANS_ATTRIBUTES * pAtt = NLME_FindATTByShortAddr ( p_pSrcDstAddr );
      
      if( !pAtt )
      {
          stMsg.m_u.m_stErr.m_ucLevel = LOG_M_NL;
          stMsg.m_u.m_stErr.m_ucErrorCode = 1;
          stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "No ATT for Source Address" );
          
          // add to queue
          WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg );            
          return;
      }
       
      memcpy( stPseudoHdr.m_st.m_aIpv6SrcAddr, pAtt->m_aIPV6Addr, 16 );
      
      pAtt = NLME_FindATTByShortAddr ( p_pSrcDstAddr + 2 );
      
      if( !pAtt )
      {
          stMsg.m_u.m_stErr.m_ucLevel = LOG_M_NL;
          stMsg.m_u.m_stErr.m_ucErrorCode = 2;
          stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "No ATT for Destination Address" );
          
          // add to queue
          WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg );            

          return;
      }
      
      memcpy( stPseudoHdr.m_st.m_aIpv6DstAddr, pAtt->m_aIPV6Addr, 16 );
    
  }
  
  ulCrtTime = MLSM_GetCrtTaiSec() + TLDE_TIME_MAX_DIFF;
  ucTimeTraveling = ( ulCrtTime & 0x3F);


  if( ((TL_HDR*)p_pTLData)->m_stPlain.m_ucUdpEncoding == UDP_ENCODING_PLAIN )
  {
      if( p_unTLDataLen <= sizeof(((TL_HDR*)p_pTLData)->m_stPlain) ) 
          return;

      g_stWciRcvRfMsg.m_u.m_stRf.m_stTL.m_ucLen = sizeof(((TL_HDR*)p_pTLData)->m_stPlain);
      memcpy( g_stWciRcvRfMsg.m_u.m_stRf.m_stTL.m_aBuff, p_pTLData, g_stWciRcvRfMsg.m_u.m_stRf.m_stTL.m_ucLen );
      
      if( p_ucSrcDstAddrLen != 8  ) // valid IPv6 addr -> not MAC addr
      {
          if( g_stDSMO.m_ucTLSecurityLevel != SECURITY_NONE ) // BBR mandatory cut UDP checksum when receive encrypted message on compressed TL header 
          {
              stMsg.m_u.m_stErr.m_ucLevel = LOG_M_TL;
              stMsg.m_u.m_stErr.m_ucErrorCode = 3;
              stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "UDP_Checksum + TL_MIC combination not accepted" );
              
              // add to queue
              WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg );            
              
              return;
          }
      }
      
      ucTimeTraveling -= ((TL_HDR*)p_pTLData)->m_stPlain.m_ucTimeAndCounter >> 2;      
      if( ucTimeTraveling & 0x80 )
      {
          ucTimeTraveling += 0x40;
      }
      
      
      // escape control + ports + check sum but add UDP header length      
      uint32 ulUdpCkSum = p_unTLDataLen - 4 + 8 ;
     *(uint16*)stPseudoHdr.m_st.m_aPadding = __swap_bytes(ulUdpCkSum); 
     
      ulUdpCkSum = IcmpInterimChksum( (const uint8*)&stPseudoHdr.m_st, sizeof(stPseudoHdr.m_st)-sizeof(stPseudoHdr.m_st.m_aTLSecurityHdr), ulUdpCkSum );
      
      // compute the checksum on NL payload but escape UDP hdr (control byte, ports, and checksum bytes)
      ulUdpCkSum = IcmpInterimChksum( &((TL_HDR*)p_pTLData)->m_stPlain.m_ucSecurityCtrl, p_unTLDataLen-4, ulUdpCkSum );
      
      ulUdpCkSum = IcmpGetFinalCksum( ulUdpCkSum );
      
      if(     ((TL_HDR*)p_pTLData)->m_stPlain.m_aUdpCkSum[0] != (ulUdpCkSum >> 8)
          ||  ((TL_HDR*)p_pTLData)->m_stPlain.m_aUdpCkSum[1] != (ulUdpCkSum & 0xFF) )
      {
          stMsg.m_u.m_stErr.m_ucLevel = LOG_M_TL;
          stMsg.m_u.m_stErr.m_ucErrorCode = 4;
          stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "Invalid UDP Checksum" );
          
          // add to queue
          WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg );
          return; // invlaid udp check sum
      }
      
      p_pTLData = ((uint8*)p_pTLData) + sizeof( ((TL_HDR*)p_pTLData)->m_stPlain );
      p_unTLDataLen -= sizeof( ((TL_HDR*)p_pTLData)->m_stPlain );
  }
  else if( ((TL_HDR*)p_pTLData)->m_stPlain.m_ucUdpEncoding == UDP_ENCODING_ENCRYPTED )
  {      
      uint8  ucKeyPorts = ((TL_HDR*)p_pTLData)->m_stPlain.m_ucUdpPorts;
      
      //the BBR is not Endpoint, so no Source/Destination ports must not be interchanged 
      //ucKeyPorts = (ucKeyPorts >> 4) | (ucKeyPorts << 4);
      
      uint16 unHeaderLen;
      
      //if( ((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucSecurityCtrl == SECURITY_ENC_MIC_32 ) ||
      if((((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucSecurityCtrl == SECURITY_ENC_MIC_32 ) ||
         (((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucSecurityCtrl == SECURITY_ENC_MIC_64 ) ||
         (((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucSecurityCtrl == SECURITY_ENC_MIC_128 )||
         (((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucSecurityCtrl == SECURITY_MIC_32 )     ||
         (((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucSecurityCtrl == SECURITY_MIC_64 )     ||
         (((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucSecurityCtrl == SECURITY_MIC_128 ))
      {
          unHeaderLen = sizeof(((TL_HDR*)p_pTLData)->m_stEncOneKey);                    
          ucTimeTraveling -= ((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucTimeAndCounter >> 2;
          
          uint8 ucTmp;
          pKey = SLME_FindTxKey( stPseudoHdr.m_st.m_aIpv6SrcAddr, ucKeyPorts, &ucTmp );
      }
      //else if( ((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_ucSecurityCtrl == SECURITY_CTRL_ENC_MIC_32 )
      else if((((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucSecurityCtrl == SECURITY_CTRL_ENC_MIC_32 ) ||
             (((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucSecurityCtrl == SECURITY_CTRL_ENC_MIC_64 )  ||
             (((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucSecurityCtrl == SECURITY_CTRL_ENC_MIC_128 )      ||
             (((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucSecurityCtrl == SECURITY_CTRL_MIC_32 )      ||
             (((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucSecurityCtrl == SECURITY_CTRL_MIC_64 )      ||
             (((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucSecurityCtrl == SECURITY_CTRL_MIC_128 ))
      {
          unHeaderLen = sizeof(((TL_HDR*)p_pTLData)->m_stEncMoreKeys);
          
          ucTimeTraveling -= ((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_ucTimeAndCounter >> 2;      
          
          pKey = SLME_FindKey( stPseudoHdr.m_st.m_aIpv6SrcAddr, 
                              ucKeyPorts, 
                              ((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_ucKeyIdx,
                              SLM_KEY_USAGE_SESSION);
      }
      else
      {
          stMsg.m_u.m_stErr.m_ucLevel = LOG_M_TL;
          stMsg.m_u.m_stErr.m_ucErrorCode = 5;
          stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "Invalid Security Control Field" );
          
          // add to queue
          WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg );
          return;
      }
      
      if( !pKey ) // key not found
      {
          stMsg.m_u.m_stErr.m_ucLevel = LOG_M_TL;
          stMsg.m_u.m_stErr.m_ucErrorCode = 6;
          stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "RX Session Key Not Found" );
          
          // add to queue
          WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg );
          return;
      }

      if( p_unTLDataLen <= (unHeaderLen+GetMicSize(TMIC,pKey->m_ucSecurityCtrl)) ) //  +MIC
      {  
          stMsg.m_u.m_stErr.m_ucLevel = LOG_M_TL;
          stMsg.m_u.m_stErr.m_ucErrorCode = 7;
          stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "Invalid TL Header Size" );
          
          // add to queue
          WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg );
          return;      
      }
      
      g_stWciRcvRfMsg.m_u.m_stRf.m_stTL.m_ucLen = unHeaderLen + 4;
      memcpy( g_stWciRcvRfMsg.m_u.m_stRf.m_stTL.m_aBuff, p_pTLData, g_stWciRcvRfMsg.m_u.m_stRf.m_stTL.m_ucLen );

      if( ucTimeTraveling & 0x80 )
      {
          ucTimeTraveling += 0x40;
      }
      
      memcpy( stPseudoHdr.m_st.m_aTLSecurityHdr, &((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_ucSecurityCtrl, unHeaderLen-2 );      
      memset( g_stWciRcvAckMsg.m_TMIC, 0, MAX_TMIC_SIZE );
      if( AES_SUCCESS != TLDE_decryptTLPayload( pKey, ulCrtTime-ucTimeTraveling, &stPseudoHdr.m_st, unHeaderLen, p_unTLDataLen, p_pTLData ) )
      {
          stMsg.m_u.m_stErr.m_ucLevel = LOG_M_TL;
          stMsg.m_u.m_stErr.m_ucErrorCode = 8;
          stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "Invalid TL MIC" );
          
          // add to queue
          WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg );
          return; // invalid MIC -> discard it
      }

      p_pTLData = ((uint8*)p_pTLData) + unHeaderLen;
      p_unTLDataLen -= unHeaderLen+4;
  }
  else
  {
      stMsg.m_u.m_stErr.m_ucLevel = LOG_M_TL;
      stMsg.m_u.m_stErr.m_ucErrorCode = 9;
      stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "Invalid UDP Header Encoding" );
      
      // add to queue
      WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg );
      return;
  }
   
  if( g_ucLoggingLevel >= DLL_LOGGING_LEVEL )
  {
    //add to queue the sent DLL ACK
    g_stWciRcvAckMsg.m_ucMsgType = WCITXT_MSG_TO_BBR_AS_TX_ACK;
    WCI_AddMessage( sizeof(g_stWciRcvAckMsg), (const uint8*)&g_stWciRcvAckMsg );
    //clear the ACK DLL header
    g_stWciRcvAckMsg.m_u.m_stDllAck.m_stDL.m_ucLen = 0;
  }
  
  //add to queue the received RF message
  g_stWciRcvRfMsg.m_ucMsgType = WCITXT_MSG_TO_BBR_AS_RF;
  g_stWciRcvRfMsg.m_u.m_stRf.m_stApp.m_ucLen = p_unTLDataLen;
  memcpy(g_stWciRcvRfMsg.m_u.m_stRf.m_stApp.m_aBuff, p_pTLData, p_unTLDataLen );
  memset( g_stWciRcvAckMsg.m_MMIC, 0, MAX_DMIC_SIZE );  
  WCI_AddMessage( sizeof(g_stWciRcvRfMsg), (const uint8*)&g_stWciRcvRfMsg );  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  decrypt transport layer payload in same buffer
/// @param  p_pKey        - key entry
/// @param  p_ulIssueTime - time when message was issued on remote source
/// @param  p_pAuth       - auth data (IPv6 pseudo header + security header)
/// @param  p_unTLDataLen - transport layer data length
/// @param  p_pTLData     - transport layer data buffer
/// @return none
/// @remarks
///      Access level: User level\n
///      Obs: !! use 1.2k stack !! p_pTLData will be altered on this function
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 TLDE_decryptTLPayload( const SLME_KEY * p_pKey,
                             uint32   p_ulIssueTime,
                             const IPV6_PSEUDO_HDR * p_pAuth,
                             uint16                  p_unSecHeaderLen,
                             uint16   p_unTLDataLen,
                             void *   p_pTLData )
{
    uint8  aNonce[13];
    uint16 unAuthLen = sizeof(IPV6_PSEUDO_HDR) - 4 + p_unSecHeaderLen - 2; //
    uint8 Val = 0;
    unsigned int   p_unToDecryptLen = p_unTLDataLen -  p_unSecHeaderLen;
    uint8 work[150];
    
    memcpy(aNonce, p_pKey->m_aIssuerEUI64, 8);    
    aNonce[8] = (uint8)(p_ulIssueTime >> 14);
    aNonce[9] = (uint8)(p_ulIssueTime >> 6);
    aNonce[10] = ((uint8*)p_pAuth)[unAuthLen-2];
    aNonce[11] = ((uint8*)p_pAuth)[unAuthLen-1];
    aNonce[12] = 0xFF; 
    
    switch(p_pKey->m_ucSecurityCtrl)
    {  
      case SECURITY_MIC_32:
      case SECURITY_MIC_64:    
      case SECURITY_MIC_128:    
         memcpy(&work[0],p_pAuth,unAuthLen);
         memcpy(&work[0]+unAuthLen,((uint8*)p_pTLData) + p_unSecHeaderLen,p_unToDecryptLen);	
         unAuthLen += (p_unToDecryptLen - GetMicSize(TMIC,p_pKey->m_ucSecurityCtrl));
         p_unToDecryptLen = GetMicSize(TMIC,p_pKey->m_ucSecurityCtrl);
        Val =  AES_Decrypt_User( p_pKey->m_aKey,aNonce,	work ,unAuthLen, &work[unAuthLen],p_unToDecryptLen , GetMicSize(TMIC,p_pKey->m_ucSecurityCtrl));
      break;
      
       case SECURITY_ENC_MIC_32:    
       case SECURITY_ENC_MIC_64:    
       case SECURITY_ENC_MIC_128: 
	// skip UDP section from AUTH because that section can be altered by BBR
        Val = AES_Decrypt_User( p_pKey->m_aKey,
                                     aNonce,
                                    (uint8*)p_pAuth,
                                    unAuthLen,
                                    ((uint8*)p_pTLData) + p_unSecHeaderLen,
                                    p_unToDecryptLen, GetMicSize(TMIC,p_pKey->m_ucSecurityCtrl) );
      break; 
    }

  return Val;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  encrypt transport layer payload in same buffer
/// @param  p_unAppDataLen  - application layer buffer
/// @param  p_pAppData      - application layer data buffer
/// @param  p_pAuth         - auth data (IPv6 pseudo header + security header)
/// @param  p_pKey          - encryption key
/// @param  p_ulCrtTai      - current TAI
/// @return none
/// @remarks
///      Access level: User level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 TLDE_encryptTLPayload( uint16 p_unAppDataLen, void * p_pAppData, const IPV6_PSEUDO_HDR * p_pAuth, uint16 p_unSecHeaderLen, const SLME_KEY * p_pKey, uint32 p_ulCrtTai )
{
    uint8 ret = AES_SUCCESS;

    uint8 ucNullMIC[16] = {0x00, 0x00, 0x00, 0x00, 
                           0x00, 0x00, 0x00, 0x00, 
                           0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0x00, 0x00};
    uint8  aNonce[13];
    uint16 unAuthLen = sizeof(IPV6_PSEUDO_HDR) - 4 + p_unSecHeaderLen - 2;
    
    uint8  work[150];
    
    memcpy(aNonce, g_aucScriptServerEUI64, 8);
    aNonce[8]  = (uint8)(p_ulCrtTai >> 14);
    aNonce[9]  = (uint8)(p_ulCrtTai >> 6);
    aNonce[10] = ((uint8*)p_pAuth)[unAuthLen-2];
    aNonce[11] = ((uint8*)p_pAuth)[unAuthLen-1];
    aNonce[12] = 0xFF; 
  
    switch(p_pKey->m_ucSecurityCtrl)
    {  
        case SECURITY_MIC_32:    
        case SECURITY_MIC_64:    
        case SECURITY_MIC_128:    
	  memset(&work[0],0,sizeof(work));
          memcpy(&work[0],p_pAuth,unAuthLen);
	  memcpy(&work[0]+unAuthLen,p_pAppData,p_unAppDataLen);	
	  unAuthLen +=  p_unAppDataLen;
          AES_Crypt_User( p_pKey->m_aKey, aNonce, (uint8 *)work,unAuthLen,(uint8*)work + unAuthLen, 0, GetMicSize(TMIC,p_pKey->m_ucSecurityCtrl));
 	  memcpy(((uint8*)p_pAppData) + p_unAppDataLen, &work[0]+unAuthLen, GetMicSize(TMIC,p_pKey->m_ucSecurityCtrl)); 
          
        break;

	  case SECURITY_ENC_MIC_32:    
	  case SECURITY_ENC_MIC_64:    
          case SECURITY_ENC_MIC_128: 
          
            // skip UDP section from AUTH because that section can be altered by BBR
            AES_Crypt_User( p_pKey->m_aKey,  aNonce, (const uint8 *) p_pAuth,unAuthLen, p_pAppData, p_unAppDataLen, GetMicSize(TMIC,p_pKey->m_ucSecurityCtrl) );
    	  break;
    } 

    memset( g_stWciTxRfMsg.m_TMIC,0,MAX_TMIC_SIZE);
    memcpy( g_stWciTxRfMsg.m_TMIC, (uint8*)p_pAppData+p_unAppDataLen, GetMicSize(TMIC,p_pKey->m_ucSecurityCtrl));
    // check if encryption is success/failure by checking MIC for zero value
    if (p_unAppDataLen && !memcmp(((uint8 *)p_pAppData+p_unAppDataLen), ucNullMIC, GetMicSize(TMIC,p_pKey->m_ucSecurityCtrl)))
    {
      ret = AES_ERROR;
    }
return ret;
}

#if DUPLICATE_ARRAY_SIZE > 0

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @author NIVIS LLC, Ion Ticus
    /// @brief  Check if message is duplicate at trasnport layer (on security enable only) 
    /// @param  p_pMIC          - message MIC
    /// @return 1 if is duplicate, 0 if not
    /// @remarks
    ///      Access level: User level\n
    ///       Note: may be a low chance to detect false duplicate but can take that risk since the RAM is a big concern\n
    ///       As an alternative keep time generation + IPV6 (6+16 = 22 bytes instead of 4 bytes)
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    uint8  TLDE_IsDuplicate( const uint8 * p_pMIC ) 
    {
        uint8  ucIdx = 0;
        uint32 ulMic;

        memcpy( &ulMic, p_pMIC, 4 );

        for( ; ucIdx < DUPLICATE_ARRAY_SIZE; ucIdx ++ )
        {
           if( ulMic == g_aDuplicateHistArray[ucIdx] )
               return 1;
        }

        if( (++g_ucDuplicateIdx) >= DUPLICATE_ARRAY_SIZE )
            g_ucDuplicateIdx = 0;

        g_aDuplicateHistArray[g_ucDuplicateIdx] = ulMic;
        return 0;
    }


#endif // no duplicate detection support
