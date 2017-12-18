////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Ion Ticus
/// Date:         December 2007
/// Description:  Implements Network Layer Data Entity of ISA100 standard
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include "nlme.h"
#include "nlde.h"
#include "dlde.h"
#include "mlde.h"
#include "mlsm.h"
#include "tlde.h"
#include "sfc.h"
#include "slme.h"

#ifdef UT_ACTIVED  // unit testing support
  #if( (UT_ACTIVED == UT_NL_ONLY) || (UT_ACTIVED == UT_NL_TL) )
      #include "../UnitTests/unit_test_NLDE.h"
      UINT_TEST_NLDE_STRUCT g_stUnitTestNLDE;
  #endif
#endif // UT_ACTIVED


#define FRAG1_H_SIZE                      4
#define FRAG2_H_SIZE                      5
#define HC1_BASIC_H_SIZE                  1
#define HC1_NO_CONTR_HOPS_DEFAULT_H_SIZE  2
#define HC1_CONTR_HOPS_DEFAULT_H_SIZE     5

#define HC1_DISPATCH_BASIC          0x01 //b00000001
#define HC1_DISPATCH_NO_CONTRACT    0x7C //b01111100 //  011 + 111HH01110111 = 011111HH + 01110111
#define HC1_DISPATCH_WITH_CONTRACT  0x6C //b01101100 //  011 + 011HH01110111 = 011011HH + 01110111
#define HC1_DISPATCH_FRAG1          0xC0 //b11000000
#define HC1_DISPATCH_FRAG2          0xE0 //b11100000

#define HC1_DISPATCH_FRAG_MASK      0xF8 //b11111000
#define HC1_DISPATCH_CONTRACT_MASK  0xEC //b11101100
#define HC1_DISPATCH_HOPS_MASK      0x03 //b00000011

#define HC1_ENCODING_LIMIT          0x77 //b01110111  011 + 011HH01110111 = 011011HH + 01110111



typedef union
{
    struct
    {
        uint8 m_ucHC1Dispatch;
    } m_stBasic;
    struct
    {
        uint8 m_ucHC1Dispatch;
        uint8 m_ucHC1Encoding;
        uint8 m_aFlowLabel[3];
        uint8 m_ucHopLimit;
    } m_stContractEnabled;
    struct
    {
        uint8 m_ucHC1Dispatch;
        uint8 m_ucHC1Encoding;
        uint8 m_ucHopLimit;
    } m_stNoContractEnabled;
} NWK_HEADER_HC1;

typedef struct
{
    uint8           m_aDatagramSize[2];
    uint8           m_aDatagramTag[2];
    NWK_HEADER_HC1 m_stHC1;
} NWK_HEADER_FRAG1;

typedef struct
{
    uint8           m_aDatagramSize[2];
    uint8           m_aDatagramTag[2];
    uint8           m_ucDatagramOffset;
    NWK_HEADER_HC1 m_stHC1;
} NWK_HEADER_FRAG2;

typedef union
{
    NWK_HEADER_HC1    m_stHC1;
    NWK_HEADER_FRAG1  m_stFrag1;
    NWK_HEADER_FRAG2  m_stFrag2;
    
    uint8 m_ucHC1Dispatch;
    uint8 m_aRowData[DLL_MAX_DSDU_SIZE_DEFAULT];
} DLL_PACKET;



void NLDE_dispatchRxMsg( const uint8 * p_pSrcDstAddr,
                         uint8         p_ucSrcDstAddrLen,
                         uint8       * p_pTLPayload,
                         uint16        p_unTLPayloadLen,
                         uint8         p_ucPriorityFlags
#if defined(BACKBONE_SUPPORT)
                         , uint16        p_unContractId
#endif                         
                         );

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Dorin Pavel, Ion Ticus
/// @brief  It is invoked by the Data-Link Layer to communicate to the Network Layer 
///         the result of a previously issued transmit request.
/// @param  p_oHandle       - application level handler (same as on request)
/// @param  p_ucLocalStatus - confirmation status
/// @return none
/// @remarks
///      Access level: Interrupt or User level\n
///      Context: Must be preceded by a transmit request action
////////////////////////////////////////////////////////////////////////////////////////////////////
void DLDE_Data_Confirm (HANDLE p_hHandle, uint8 p_ucLocalStatus)
{
  if( g_ucLoggingLevel < DLL_LOGGING_LEVEL )
    return;
      
  //add the DLL message sent confirmation    
  g_stWciRcvRfMsg.m_ucMsgType = WCITXT_MSG_TO_BBR_AS_CFM;
  g_stWciRcvRfMsg.m_u.m_stDllCfm.ulTxMsgID = p_hHandle;
  g_stWciRcvRfMsg.m_u.m_stDllCfm.ucTxStatus = p_ucLocalStatus; 
      
  WCI_AddMessage( sizeof(g_stWciRcvRfMsg), (const uint8*)&g_stWciRcvRfMsg );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Dorin Pavel, Ion Ticus
/// @brief  It is invoked by DLL Layer to notify the Network Layer of a
///         successfully received payload addressed to the device.
/// @param  p_ucSrcDstAddrLen - The source addressing length (4 or 8)
/// @param  p_pSrcDstAddr     - Network source address + destination address (if length is 4)
/// @param  p_ucPriority      - Priority of the payload
/// @param  p_ucPayloadLength - Payload length
/// @param  p_pPayload        - Payload. Number of octets as per p_ucPayloadLength
/// @return none
/// @remarks
///      Access level: Interrupt\n
///      Context: When a new packet data was received and processed by DLL
////////////////////////////////////////////////////////////////////////////////////////////////////
void DLDE_Data_Indicate ( uint8        p_ucSrcDstAddrLen,
                         const uint8 * p_pSrcDstAddr,
                         uint8         p_ucPayloadLength,
			             void *        p_pPayload )
{
  uint8 ucPkHeaderLen;
  const DLL_PACKET * pDllPk = p_pPayload;
  
  // HC1 -> sigle packet datagram
  if(    pDllPk->m_ucHC1Dispatch ==  HC1_DISPATCH_BASIC 
     || (pDllPk->m_ucHC1Dispatch & HC1_DISPATCH_CONTRACT_MASK) == (HC1_DISPATCH_NO_CONTRACT & HC1_DISPATCH_CONTRACT_MASK) ) 
  {
      if( pDllPk->m_ucHC1Dispatch == HC1_DISPATCH_BASIC )
      {
          ucPkHeaderLen = HC1_BASIC_H_SIZE;
      }
      else
      {
          if( (pDllPk->m_ucHC1Dispatch & (~HC1_DISPATCH_HOPS_MASK)) == HC1_DISPATCH_NO_CONTRACT )
          {
              ucPkHeaderLen = HC1_NO_CONTR_HOPS_DEFAULT_H_SIZE;
          }
          else
          {
              ucPkHeaderLen = HC1_CONTR_HOPS_DEFAULT_H_SIZE;
          }
          if( (pDllPk->m_ucHC1Dispatch & HC1_DISPATCH_HOPS_MASK) == 0 )
          {
              ucPkHeaderLen++;
          }
      }

  }
  //  5 bits   11 bits	     16 bits
  //  11000     Datagram_size   Datagram_tag
  //  first fragment datagram
  else if( (pDllPk->m_ucHC1Dispatch & HC1_DISPATCH_FRAG_MASK) == HC1_DISPATCH_FRAG1 ) 
  {
      if( p_ucPayloadLength > FRAG1_H_SIZE+HC1_BASIC_H_SIZE )
      {
          uint16 unDatagramSize = (((uint16)pDllPk->m_stFrag2.m_aDatagramSize[0] & 0x07) << 8)
                                  + pDllPk->m_stFrag2.m_aDatagramSize[1];

          if( unDatagramSize <= MAX_DATAGRAM_SIZE )
          {            
              if( pDllPk->m_stFrag1.m_stHC1.m_stBasic.m_ucHC1Dispatch == HC1_DISPATCH_BASIC  )
              {         
                  ucPkHeaderLen = FRAG1_H_SIZE + HC1_BASIC_H_SIZE;
              }
              else  if((pDllPk->m_stFrag1.m_stHC1.m_stBasic.m_ucHC1Dispatch & HC1_DISPATCH_CONTRACT_MASK) == (HC1_DISPATCH_NO_CONTRACT & HC1_DISPATCH_CONTRACT_MASK) )
              {
                  if( (pDllPk->m_stFrag1.m_stHC1.m_stBasic.m_ucHC1Dispatch & (~HC1_DISPATCH_HOPS_MASK)) == HC1_DISPATCH_NO_CONTRACT )
                  {
                      ucPkHeaderLen = FRAG1_H_SIZE+HC1_NO_CONTR_HOPS_DEFAULT_H_SIZE;
                  }
                  else
                  {
                      ucPkHeaderLen = FRAG1_H_SIZE+HC1_CONTR_HOPS_DEFAULT_H_SIZE;
                  }
                  if( (pDllPk->m_stFrag1.m_stHC1.m_stBasic.m_ucHC1Dispatch & HC1_DISPATCH_HOPS_MASK) == 0 )
                  {
                      ucPkHeaderLen++;
                  }
              }
              else // not UDP or not 16 bit address type
              {
                  return;
              }
          }
      }
  }
        //  5 bits   11 bits	     16 bits
        // 11100     Datagram_size   Datagram_tag
  else if( (pDllPk->m_ucHC1Dispatch & HC1_DISPATCH_FRAG_MASK) == HC1_DISPATCH_FRAG2 ) // next fragment datagram
  {      
      ucPkHeaderLen = FRAG2_H_SIZE;
  }  

  if( p_ucPayloadLength >= ucPkHeaderLen ) // has space and valid packet size
  {
      g_stWciRcvRfMsg.m_u.m_stRf.m_stNL.m_ucLen = ucPkHeaderLen;
      memcpy( g_stWciRcvRfMsg.m_u.m_stRf.m_stNL.m_aBuff, p_pPayload, ucPkHeaderLen );
      
      if( pDllPk->m_ucHC1Dispatch == HC1_DISPATCH_BASIC )
      {
          ucPkHeaderLen = 0;
          *(uint8*)p_pPayload = UDP_ENCODING_ENCRYPTED;
      }
          
      NLDE_DATA_Indication( p_ucSrcDstAddrLen,
                            p_pSrcDstAddr,
                            (uint8*)p_pPayload+ucPkHeaderLen, 
                            p_ucPayloadLength - ucPkHeaderLen );
  }  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Initializes the contract and route tables. Resets the current packet and multi-packet 
///         structures
/// @param  none
/// @return none
/// @remarks
///      Access level: User Level
////////////////////////////////////////////////////////////////////////////////////////////////////
void NLDE_Init(void)
{  
  NLME_Init();
}

