/***************************************************************************************************
* Name:         UDP.h
* Author:       Nivis LLC,
* Date:         Aug, 2008
* Description:  This header file provide API of light UDP implmentation
* Changes:
* Revisions:
****************************************************************************************************/
#include <string.h>
#include <stdio.h>

#include "../global.h"
#include "../isa100/nlme.h"
#include "../isa100/tlde.h"
#include "../isa100/slme.h"
#include "../isa100/mlsm.h"
#include "../isa100/aslsrvc.h"
#include "../isa100/dmap.h"
#include "../asm.h"
#include "../i2c.h"
#include "../text/text_convertor.h"
#include "udp.h"
#include "udpqueue.h"

EUI64_ADDR g_aucTestedNodeEUI64;     //needed to validate the TestedNode Sec Join Confirm
static uint8 g_aulTestedNodeChallenge[16];  //needed to validate the TestedNode Sec Join Confirm
SLME_KEY g_stNewKey;
uint8 keyReqArr[100];
uint8 KeyUpdateFlag = 0;

static __no_init uint8 g_aucMySecMngChallenge[16];

#define MASTER_KEY_ID       0   //hardcoded on device

uint16 UDP_GetMessage( uint8 * p_pMsg );

void UDP_Init( void )
{
    UDPQ_Init();
}


uint8 UDP_UpdateMIC( EXEC_REQ_SRVC* p_pstExecReq, EXEC_RSP_SRVC* p_pstExecResp, int inpLen) 
{
  uint8 aucHashEntry[100];
  
  memset(aucHashEntry,0,100);
  
  memcpy(aucHashEntry,p_pstExecResp->p_pRspData,inpLen);
  memcpy((aucHashEntry + inpLen), g_aucTestedNodeEUI64 ,sizeof(g_aucTestedNodeEUI64));
  memcpy((aucHashEntry + inpLen + sizeof(g_aucTestedNodeEUI64)), g_aulTestedNodeChallenge ,sizeof(g_aulTestedNodeChallenge));
  
  Keyed_Hash_MAC(g_aJoinAppKey,   aucHashEntry, inpLen + sizeof(g_aucTestedNodeEUI64)+ sizeof(g_aulTestedNodeChallenge));
  memcpy(p_pstExecResp->p_pRspData + inpLen ,aucHashEntry, 4);
  
  return 1;

}

uint8 UDP_UpdateSecJoinResponse( EXEC_REQ_SRVC* p_pstExecReq, EXEC_RSP_SRVC* p_pstExecResp )
{
    uint8 aucHashEntry[MAX_HMAC_INPUT_SIZE];
    uint8 p_pucPeerIPv6Address[16] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0};
    uint8 CombinedSecLevel =0;
  
    memcpy( g_aucTestedNodeEUI64, p_pstExecReq->p_pReqData, sizeof(g_aucTestedNodeEUI64) );
    memcpy( g_aulTestedNodeChallenge, p_pstExecReq->p_pReqData + 12, sizeof(g_aulTestedNodeChallenge) );
    memcpy( g_aucMySecMngChallenge, p_pstExecResp->p_pRspData, sizeof(g_aucMySecMngChallenge) );
  
    memcpy(&p_pucPeerIPv6Address[8], &g_aucTestedNodeEUI64[0] ,8);
    
    // validation of SecJoinRequest MIC
    if( AES_SUCCESS !=  AES_Decrypt_User( g_aJoinAppKey,              
                                          (uint8*)g_aulTestedNodeChallenge,     //first 13 bytes 
                                         p_pstExecReq->p_pReqData, 
                                          p_pstExecReq->m_unLen - MIC_SIZE,                   
                                          p_pstExecReq->p_pReqData + p_pstExecReq->m_unLen - MIC_SIZE,
                                          MIC_SIZE, MIC_SIZE) )
    {
      return SFC_FAILURE;
    }
    
    //generate the master key
    //memcpy(aucHashEntry, g_aulTestedNodeChallenge, sizeof(g_aulTestedNodeChallenge) );  //Tested Device Challenge
    //memcpy(aucHashEntry + 16, g_aucMySecMngChallenge, sizeof(g_aucMySecMngChallenge));  //Emulated Security Manager Challenge
    //memcpy(aucHashEntry + 32, g_aucTestedNodeEUI64, sizeof(g_aucTestedNodeEUI64));
    //memcpy(aucHashEntry + 40, c_oSecManagerEUI64BE, sizeof(c_oSecManagerEUI64BE));
    
    
      memcpy(aucHashEntry, g_aucTestedNodeEUI64, 8);
      memcpy(aucHashEntry +  8, c_oSecManagerEUI64BE, 8);
      memcpy(aucHashEntry + 16, g_aulTestedNodeChallenge, 16);   //New Device Challenge
      memcpy(aucHashEntry + 32, g_aucMySecMngChallenge, 16);  //Security Manager Challenge
    
    
    
    Keyed_Hash_MAC(g_aJoinAppKey, aucHashEntry, 48);    //the first 16 bytes from "aucHashEntry" represent the Master Key
        
    //encrypt the DLL and SMSession keys
    uint8 ucEncKeyOffset = SEC_JOIN_RESP_SIZE - 32;

    uint8* pucKeyPolicy = p_pstExecResp->p_pRspData + ucEncKeyOffset - 7;  //2*3 bytes(Master Key, DLL and Session key policies) + 1 byte(DLL KeyID) 
    uint32 ulLifeTime;
        
    //add the Master Key - use later for update security fields inside New Key method   
    ulLifeTime = (((uint16)(pucKeyPolicy[0])) << 8) | pucKeyPolicy[1];
    ulLifeTime *= 1800;

    CombinedSecLevel = *(p_pstExecResp->p_pRspData + 32);
    SLME_SetKey(    NULL, // p_pucPeerIPv6Address - will be update later with DUT real IPv6  
                    0, // p_ucUdpPorts,
                    MASTER_KEY_ID, // p_ucKeyID  - hardcoded
                    aucHashEntry,  // p_pucKey, 
                    c_oSecManagerEUI64BE,  // p_pucIssuerEUI64, 
                    0, // p_ulValidNotBefore
                    ulLifeTime,   // p_ulSoftLifetime
                    ulLifeTime*2, // p_ulHardLifetime
                    SLM_KEY_USAGE_MASTER, // p_ucUsage, 
                    0, // p_ucPolicy -> need correct policy
                    CombinedSecLevel & 0x03);
    
    ulLifeTime = ((uint16)(pucKeyPolicy[4])) << 8 | pucKeyPolicy[5];
    
  //  if( ulLifeTime || memcmp( p_pstExecResp->p_pRspData + ucEncKeyOffset + 16, c_aucInvalidKey, 16 ) ) // TL encrypted if ulLifeTime <> 0 || key <> 0
    {
        ulLifeTime *= 1800;
        
        // add the Session Key - use for TL security support
        SLME_SetKey( p_pucPeerIPv6Address,// - will be update later with DUT real IPv6
                    (uint8)UAP_SMAP_ID & 0x0F,  // p_ucUdpPorts,   //dest port - SM's SM_UAP 
                    0,  // p_ucKeyID - hardcoded,
                    p_pstExecResp->p_pRspData + ucEncKeyOffset + 16,  // p_pucKey, 
                    g_aucTestedNodeEUI64,  // p_pucIssuerEUI64, 
                    0, // p_ulValidNotBefore
                    ulLifeTime,   // p_ulSoftLifetime
                    ulLifeTime*2, // p_ulHardLifetime
                    SLM_KEY_USAGE_SESSION, // p_ucUsage, 
                    0, // p_ucPolicy -> need correct policy
                    (CombinedSecLevel >> 2) & 0x07    
                      );
    }


    AES_Crypt_User(aucHashEntry,    //master key 
                   (uint8*)g_aucMySecMngChallenge,
                   p_pstExecResp->p_pRspData + ucEncKeyOffset, 
                   0,   //pure encryption
                   p_pstExecResp->p_pRspData + ucEncKeyOffset,
                   32, MIC_SIZE);
    
    //generate the 128_Bit_Response_To_New_Device_Hash_B field
    memcpy(aucHashEntry, g_aucMySecMngChallenge, sizeof(g_aucMySecMngChallenge));        //Security Manager Challenge
    memcpy(aucHashEntry + 16, g_aulTestedNodeChallenge, 16);    //Tested Device Challenge
    memcpy(aucHashEntry + 32, g_aucTestedNodeEUI64, 8);
    memcpy(aucHashEntry + 40, c_oSecManagerEUI64BE, 8);
    
    memcpy(aucHashEntry + 48, p_pstExecResp->p_pRspData + 32, 8 ); // Combined Security Level + HardLife Span ( Master/DL/Session)
    memcpy(aucHashEntry + 56, p_pstExecResp->p_pRspData + 40, 32);  // Encry. DL Key + Encry Sys Mng Session Key

    if( SFC_SUCCESS == Keyed_Hash_MAC(g_aJoinAppKey, aucHashEntry, MAX_HMAC_INPUT_SIZE) )
    {
        //inside original response overwrite Hash_B field (initially is full 0x00)
        memcpy(p_pstExecResp->p_pRspData + 16, aucHashEntry, 16);    
    }
    
    return SFC_SUCCESS;
}


uint8 aaKey[16];
uint8 aaNon[13];
uint8 aaAuthLen =0;
uint8 aaEncrLen =0;

uint8 UDP_UpdateNewKeyRequest( EXEC_REQ_SRVC* p_pstExecReq, const uint8* p_pucPeerIPv6Address )
{
    //uint8 aucMasterKey[16] = {0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01};
    //uint8 aucMasterKey[16] = {0x07,0xBA,0x16,0x7D,0xC2,0x1E,0xB2,0x57,0x97,0xE6,0xAE,0x49,0xE1,0x3F,0xF5,0x23};
    
    WCITXT_TX_MSG stMsg;
    unsigned char *tempAddr;
    unsigned int pulTaiSec=0;
    unsigned short punTaiFract=0;
    stMsg.m_ucMsgType = WCITXT_MSG_TO_BBR_AS_ERR;
    stMsg.m_u.m_stErr.m_ucSeverity = LOG_ERROR;
    stMsg.m_u.m_stErr.m_ucLevel = LOG_M_APP;
        
    //check the key usage
    uint8 ucKeyUsage = (p_pstExecReq->p_pReqData[0] & POLICY_KEY_USAGE_MASK) >> POLICY_KEY_USAGE_OFF;

    uint8 ucReqSize = DSMO_MAX_NEW_KEY_SIZE_NO_MMIC ;//+ GetMicSize(MASTER_KEY_MIC,0);
    
    if( SLM_KEY_USAGE_DLL == ucKeyUsage || SLM_KEY_USAGE_MASTER == ucKeyUsage)
    {
        ucReqSize -= 28;  //RemoteIPv6, RemoteEUI64, SourcePort + RemotePort   
    }    
            
    //check the granularity
    const uint8* pucPolicy = p_pstExecReq->p_pReqData + 1;
    uint32 ulValidNotBeforeSec;
    uint32 ulHardLifeTimeSec;
            
    switch( p_pstExecReq->p_pReqData[0] & POLICY_KEY_GRAN_MASK )
    {
        case 0x01:      //minute
            ucReqSize -= 1; 
            pucPolicy = DMAP_ExtractUint32( pucPolicy, &ulValidNotBeforeSec);
            ulValidNotBeforeSec *= 60;
                    
            ulHardLifeTimeSec = ((uint32)*(pucPolicy++) << 16);
            ulHardLifeTimeSec |= ((uint32)*(pucPolicy++) << 8);
            ulHardLifeTimeSec |= *(pucPolicy++);
            ulHardLifeTimeSec *= 60;
            break;
        case 0x02:      //hour
            ucReqSize -= 2; 
            pucPolicy = DMAP_ExtractUint32( pucPolicy, &ulValidNotBeforeSec);
                    
            ulHardLifeTimeSec = ((uint32)*(pucPolicy++) << 8);
            ulHardLifeTimeSec |= *(pucPolicy++);
            ulHardLifeTimeSec *= 3600;
            break;      
        case 0x03:      //day
            ucReqSize -= 2; 
            pucPolicy = DMAP_ExtractUint32( pucPolicy, &ulValidNotBeforeSec);
                    
            ulHardLifeTimeSec = ((uint32)*(pucPolicy++) << 8);
            ulHardLifeTimeSec |= *(pucPolicy++);
            ulHardLifeTimeSec *= 86400;

            break;      
        //case 0x00:    
        //    pucPolicy = DMAP_ExtractUint32( pucPolicy, &ulValidNotBeforeSec);
        //    pucPolicy = DMAP_ExtractUint32( pucPolicy, &ulHardLifeTimeSec);  
        //    break;
        case 0x00:    //seconds
            tempAddr  = (unsigned char *)pucPolicy;
            pucPolicy = DMAP_ExtractUint32( pucPolicy, &ulValidNotBeforeSec);
            MLSM_GetCrtTaiTime(&pulTaiSec, &punTaiFract);
            if(ulValidNotBeforeSec == 0xFFFFFFFF) // Trick to get the HLT 
            {
              *tempAddr =       0;
              *(tempAddr + 1) = 0;
              *(tempAddr + 2) = 0;
              *(tempAddr + 3) = 1;
              *(tempAddr + 4) = (unsigned char)((pulTaiSec + 1800) >> 24);
              *(tempAddr + 5) = (unsigned char)((pulTaiSec + 1800)>> 16);
              *(tempAddr + 6) = (unsigned char)((pulTaiSec + 1800)>> 8);
              *(tempAddr + 7) = (unsigned char)((pulTaiSec + 1800));
              DMAP_ExtractUint32( &tempAddr[4], &ulHardLifeTimeSec);
              ulValidNotBeforeSec = 0x01;
              pucPolicy+=4;
            }
            else
            pucPolicy = DMAP_ExtractUint32( pucPolicy, &ulHardLifeTimeSec);  
            break;
    }
    uint8 NewKeySecLevel = (uint8)(*pucPolicy >> 5);
    
    uint8* pucStaticData = p_pstExecReq->p_pReqData + ucReqSize - (24); //+ GetMicSize(MASTER_KEY_MIC,0)); 
    const SLME_KEY * pKey = SLME_GetMasterKey( p_pucPeerIPv6Address, pucStaticData[2] );
    
    ucReqSize += GetMicSize(MASTER_KEY_MIC,pKey->m_ucSecurityCtrl);
      
    if( p_pstExecReq->m_unLen < ucReqSize)
    {
        stMsg.m_u.m_stErr.m_ucErrorCode = 0;
        stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "invalid size" );

        // add to queue
        WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg ); 
        return 0;
    }        
             
   
            
    union
    {
      uint32 m_ulAligned;
      uint8  m_aucNonce[13]; 
    } stAlignedNonce;
    
    //check if clear specified that last nonce byte should be 0xFF
    stAlignedNonce.m_aucNonce[12] = 0xFF;
    memcpy(stAlignedNonce.m_aucNonce, c_oSecManagerEUI64BE, 8);
    memcpy(stAlignedNonce.m_aucNonce + 8, pucStaticData + 3, 4);
   
   // if(( SECURITY_CTRL_ENC_MIC_32  == (pucStaticData[1] & (SEC_LEVEL_MASK | SEC_KEY_ID_MODE_MASK)))||
   //    ( SECURITY_CTRL_ENC_MIC_64  == (pucStaticData[1] & (SEC_LEVEL_MASK | SEC_KEY_ID_MODE_MASK)))||
   //    ( SECURITY_CTRL_ENC_MIC_128 == (pucStaticData[1] & (SEC_LEVEL_MASK | SEC_KEY_ID_MODE_MASK))))
    {
        //only if Auth+Enc+MIC32

        if( pKey )
        {
            //a paired key(Master Key or Session Key) will be automatically added also on DBBR
            //the paired Key will be added after the New Key request is sent to DUT, explicitly to prevent using the new added session key by the DBBR(so far as DUT not already have it)   

            //if( SLM_KEY_USAGE_MASTER == ucKeyUsage || SLM_KEY_USAGE_SESSION == ucKeyUsage )
            //{
                memcpy(g_stNewKey.m_aPeerIPv6Address, p_pucPeerIPv6Address, sizeof(g_stNewKey.m_aPeerIPv6Address));
                if(ucKeyUsage == SLM_KEY_USAGE_SESSION){  // in case of TL save the ports
                  g_stNewKey.m_ucUdpPorts = ((pucPolicy[2] << 4) | (pucPolicy[28] & 0x0F));
                }else{
                  g_stNewKey.m_ucUdpPorts = 0;
                }
                g_stNewKey.m_ucKeyID = pucStaticData[7];
                g_stNewKey.m_ucUsage = ucKeyUsage;
                g_stNewKey.m_ucPolicy = pucPolicy[0];
                g_stNewKey.m_ulValidNotBefore = ulValidNotBeforeSec;
                g_stNewKey.m_ulHardLifetime = ulHardLifeTimeSec; 
                memcpy(g_stNewKey.m_aKey, pucStaticData + 8, sizeof(g_stNewKey.m_aKey));
                memcpy(g_stNewKey.m_aIssuerEUI64, pKey->m_aIssuerEUI64, sizeof(g_stNewKey.m_aIssuerEUI64));
                g_stNewKey.m_ucSecurityCtrl = NewKeySecLevel;
            //}
            
            memset(aaKey,0,16);
            memset(aaNon,0,13);
            memcpy(aaKey, pKey->m_aKey, 16);
            memcpy(aaNon, stAlignedNonce.m_aucNonce,13);
            aaAuthLen = ucReqSize - (16 + GetMicSize(MASTER_KEY_MIC,pKey->m_ucSecurityCtrl));
            aaEncrLen = 16;
           
            AES_Crypt_User(pKey->m_aKey, 
                           stAlignedNonce.m_aucNonce, 
                           p_pstExecReq->p_pReqData, 
                           ucReqSize - (16 + GetMicSize(MASTER_KEY_MIC,pKey->m_ucSecurityCtrl)),    //Enc Key + MIC32, 
                           p_pstExecReq->p_pReqData + ucReqSize - (16 + GetMicSize(MASTER_KEY_MIC,pKey->m_ucSecurityCtrl)),
                           16,  GetMicSize(MASTER_KEY_MIC,pKey->m_ucSecurityCtrl));
            
            KeyUpdateFlag = DSMO_NEW_KEY;    
            return 1;
        }
        else
        {
            stMsg.m_u.m_stErr.m_ucErrorCode = 1;
            stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "No Master Key" );

            // add to queue
            WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg ); 
            return 0;    
        }
    }
    /*else
    {
        stMsg.m_u.m_stErr.m_ucErrorCode = 2;
        stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "Invalid Security Control" );
        
        // add to queue
        WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg ); 
        return 0;
    } */
}

uint8 UDP_UpdateDeleteKeyRequest(EXEC_REQ_SRVC* p_pstExecReq, const uint8* p_pucPeerIPv6Address)
{
    WCITXT_TX_MSG stMsg;
  
    stMsg.m_ucMsgType = WCITXT_MSG_TO_BBR_AS_ERR;
    stMsg.m_u.m_stErr.m_ucSeverity = LOG_ERROR;
    stMsg.m_u.m_stErr.m_ucLevel = LOG_M_APP;

    uint8 ucReqSize = DSMO_MAX_DEL_KEY_SIZE_NO_MMIC;// + GetMicSize(MASTER_KEY_MIC,pstKey->m_ucSecurityCtrl);
    
    //const SLME_KEY * pstKey = SLME_GetMasterKey( p_pucPeerIPv6Address,  p_pstExecReq->p_pReqData[1] );
                           
    if( SLM_KEY_USAGE_SESSION != p_pstExecReq->p_pReqData[0] )
    {
       ucReqSize -= 2+16+2;
    }
    
    uint8 MKeyID    = p_pstExecReq->p_pReqData[ucReqSize - 5]; 
    const SLME_KEY * pstKey = SLME_GetMasterKey( p_pucPeerIPv6Address, MKeyID);
    ucReqSize += GetMicSize(MASTER_KEY_MIC,pstKey->m_ucSecurityCtrl);
    
    //KeyUsage(1 byte) + MasterKeyId(1 byte) + Key Id(1 byte) + KeyUdpPorts(4 byte) + IPv6PeerAddr(16bytes) + NonceSubstring(4 bytes) + MIC(4 bytes)
    if( p_pstExecReq->m_unLen < ucReqSize )
    {
        stMsg.m_u.m_stErr.m_ucErrorCode = 3;
        stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "Invalid Size" );

        // add to queue
        WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg ); 
        return 0;
    }
    
    if( pstKey )
    {
        union
        {
            uint32 m_ulAligned;
            uint8  m_aucNonce[13]; 
        } stAlignedNonce;
        
        //check if clear specified that last nonce byte should be 0xFF
        stAlignedNonce.m_aucNonce[12] = 0xFF;
        
        memcpy(stAlignedNonce.m_aucNonce, c_oSecManagerEUI64BE, 8);
        //memcpy(stAlignedNonce.m_aucNonce + 8, p_pstExecReq->p_pReqData + ucReqSize - 8, 4);
        memcpy(stAlignedNonce.m_aucNonce + 8, p_pstExecReq->p_pReqData + ucReqSize - (4+GetMicSize(MASTER_KEY_MIC,pstKey->m_ucSecurityCtrl)), 4);
        memcpy(&keyReqArr[0], &p_pstExecReq->p_pReqData[0], p_pstExecReq->m_unLen);
        AES_Crypt_User( pstKey->m_aKey,
                        stAlignedNonce.m_aucNonce,      
                        p_pstExecReq->p_pReqData, 
                        ucReqSize - GetMicSize(MASTER_KEY_MIC,pstKey->m_ucSecurityCtrl),                      
                        p_pstExecReq->p_pReqData + (ucReqSize - GetMicSize(MASTER_KEY_MIC,pstKey->m_ucSecurityCtrl)),
                        0, GetMicSize(MASTER_KEY_MIC,pstKey->m_ucSecurityCtrl)); //just authentication
    }
    else
    {
        stMsg.m_u.m_stErr.m_ucErrorCode = 4;
        stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "No Master Key" );
        
        // add to queue
        WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg ); 
        return 0;
    }
    KeyUpdateFlag = DSMO_DELETE_KEY;    
    return 1;
}


uint8 UDP_UpdateKeyRequest(EXEC_REQ_SRVC* p_pstExecReq, const uint8* p_pucPeerIPv6Address)
{
    WCITXT_TX_MSG stMsg;
  
    stMsg.m_ucMsgType = WCITXT_MSG_TO_BBR_AS_ERR;
    stMsg.m_u.m_stErr.m_ucSeverity = LOG_ERROR;
    stMsg.m_u.m_stErr.m_ucLevel = LOG_M_APP;
    uint8 ucKeyUsage = p_pstExecReq->p_pReqData[0];
    
    uint8 ucReqSize = DSMO_KEY_POLIC_UPD_SIZE_NO_MMIC;
                           
    if( SLM_KEY_USAGE_DLL == ucKeyUsage || SLM_KEY_USAGE_MASTER == ucKeyUsage)
    {
        ucReqSize -= 20;  //RemoteIPv6, RemoteEUI64, SourcePort + RemotePort   
    }
    
    uint8 MKeyID    = p_pstExecReq->p_pReqData[ucReqSize - 5]; 
    const SLME_KEY * pstKey = SLME_GetMasterKey( p_pucPeerIPv6Address, MKeyID);
    ucReqSize += GetMicSize(MASTER_KEY_MIC,pstKey->m_ucSecurityCtrl);
    
    if( p_pstExecReq->m_unLen < ucReqSize )
    {
        stMsg.m_u.m_stErr.m_ucErrorCode = 3;
        stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "Invalid size.." );

        // add to queue
        WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg ); 
        return 0;
    }
    
    if( pstKey )
    {
        union
        {
            uint32 m_ulAligned;
            uint8  m_aucNonce[13]; 
        } stAlignedNonce;
        
        //check if clear specified that last nonce byte should be 0xFF
        stAlignedNonce.m_aucNonce[12] = 0xFF;
        
        memset(&keyReqArr[0], 0, sizeof(keyReqArr));
        memcpy(&keyReqArr[0],  &p_pstExecReq->p_pReqData[0], ucReqSize);
        memcpy(stAlignedNonce.m_aucNonce, c_oSecManagerEUI64BE, 8);
        memcpy(stAlignedNonce.m_aucNonce + 8, p_pstExecReq->p_pReqData + ucReqSize - (4+GetMicSize(MASTER_KEY_MIC,pstKey->m_ucSecurityCtrl)), 4);

        AES_Crypt_User( pstKey->m_aKey,
                        stAlignedNonce.m_aucNonce,      
                        p_pstExecReq->p_pReqData, 
                        ucReqSize - GetMicSize(MASTER_KEY_MIC,pstKey->m_ucSecurityCtrl),                      
                        p_pstExecReq->p_pReqData + (ucReqSize - GetMicSize(MASTER_KEY_MIC,pstKey->m_ucSecurityCtrl)),
                        0, GetMicSize(MASTER_KEY_MIC,pstKey->m_ucSecurityCtrl)); //just authentication
    } 
    else
    {
       stMsg.m_u.m_stErr.m_ucErrorCode = 4;
        stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "No Master Key" );
        
        // add to queue
        WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg ); 
        return 0;
    }
    KeyUpdateFlag = DSMO_KEY_POLICY_UPDATE;
    return 1;
}

uint8 UDP_UpdateRfMsg( WCITXT_RCV_MSG * p_pRcvMsg )
{
    if( p_pRcvMsg->m_stApp.m_ucLen )
    {
        static GENERIC_ASL_SRVC stGenSrvc;
        
        if( ASLSRVC_GetGenericObject( p_pRcvMsg->m_stApp.m_aBuff,
                                  p_pRcvMsg->m_stApp.m_ucLen,
                                  &stGenSrvc,
                                  NULL ) )
        {
            switch ( stGenSrvc.m_ucType )
            {
                case SRVC_EXEC_RSP: 
                {
                    GENERIC_ASL_SRVC stOrigGenSrvc;
                    //m_stOrigAPDU contains the original request
                    if( ASLSRVC_GetGenericObject( p_pRcvMsg->m_u.m_stRsp.m_stOrigAPDU.m_aBuff,
                                  p_pRcvMsg->m_u.m_stRsp.m_stOrigAPDU.m_ucLen,
                                  &stOrigGenSrvc,
                                  NULL ) )
                    {
                        if( SRVC_EXEC_REQ == stOrigGenSrvc.m_ucType )
                        {
                            if( (DMAP_DMO_OBJ_ID == stOrigGenSrvc.m_stSRVC.m_stExecReq.m_unDstOID && DMO_PROXY_SEC_SYM_REQ == stOrigGenSrvc.m_stSRVC.m_stExecReq.m_ucMethID) ||
                                 SM_PSMO_OBJ_ID == stOrigGenSrvc.m_stSRVC.m_stExecReq.m_unDstOID && PSMO_SECURITY_JOIN_REQ == stOrigGenSrvc.m_stSRVC.m_stExecReq.m_ucMethID)
                            {
                                //Symetric Join Security Response
                                if( SFC_SUCCESS != UDP_UpdateSecJoinResponse(&stOrigGenSrvc.m_stSRVC.m_stExecReq, &stGenSrvc.m_stSRVC.m_stExecRsp ) )
                                {
                                    WCITXT_TX_MSG stMsg;
  
                                    stMsg.m_ucMsgType = WCITXT_MSG_TO_BBR_AS_ERR;
                                    stMsg.m_u.m_stErr.m_ucSeverity = LOG_ERROR;
                                    stMsg.m_u.m_stErr.m_ucLevel = LOG_M_APP;
                                    
                                    stMsg.m_u.m_stErr.m_ucErrorCode = 0;
                                    stMsg.m_u.m_stErr.m_stDesc.m_ucLen = sprintf( stMsg.m_u.m_stErr.m_stDesc.m_aBuff, "Reason: %s", "INVALID MIC" );

                                    // add to queue 
                                    WCI_AddMessage( sizeof(stMsg.m_u.m_stErr)+4, (const uint8*)&stMsg );
                                    return 0;   //not send the SecJoinResponse
                                }
                            } 
                            if( (DMAP_DMO_OBJ_ID == stOrigGenSrvc.m_stSRVC.m_stExecReq.m_unDstOID && DMO_PROXY_SM_JOIN_REQ == stOrigGenSrvc.m_stSRVC.m_stExecReq.m_ucMethID) ||
                                (SM_DMSO_OBJ_ID == stOrigGenSrvc.m_stSRVC.m_stExecReq.m_unDstOID && DMSO_JOIN_REQUEST == stOrigGenSrvc.m_stSRVC.m_stExecReq.m_ucMethID) )
                            {
                              
                                //update the Script Server EUI64                                 
                                memcpy( g_aucScriptServerEUI64, stGenSrvc.m_stSRVC.m_stExecRsp.p_pRspData + 38, sizeof(g_aucScriptServerEUI64) ); 
                                
                                //Symetric SM Join Response - need to update the previous added keys(MasterKey + SessionKey)
                                //for Master Key the "m_aPeerIPv6Address" is used as index to identify the Master Key of the DUT
                                SLME_UpdateJoinSessionsKeys( stOrigGenSrvc.m_stSRVC.m_stExecReq.p_pReqData, stGenSrvc.m_stSRVC.m_stExecRsp.p_pRspData );
                              
                              UDP_UpdateMIC(&stOrigGenSrvc.m_stSRVC.m_stExecReq, &stGenSrvc.m_stSRVC.m_stExecRsp,48);
                            }
                            if( (DMAP_DMO_OBJ_ID == stOrigGenSrvc.m_stSRVC.m_stExecReq.m_unDstOID && DMO_PROXY_SM_CONTR_REQ == stOrigGenSrvc.m_stSRVC.m_stExecReq.m_ucMethID) ||
                                (SM_DMSO_OBJ_ID == stOrigGenSrvc.m_stSRVC.m_stExecReq.m_unDstOID && DMSO_CONTRACT_REQUEST == stOrigGenSrvc.m_stSRVC.m_stExecReq.m_ucMethID) )
                            {
                                 UDP_UpdateMIC(&stOrigGenSrvc.m_stSRVC.m_stExecReq, &stGenSrvc.m_stSRVC.m_stExecRsp,28);
                            }
                        }
                        
                        #ifdef WCI_SUPPORT 
                        //update also the Execute Response ReqId field(always compressed form of the ObjectId representation)
                        p_pRcvMsg->m_stApp.m_aBuff[2] = stOrigGenSrvc.m_stSRVC.m_stExecReq.m_ucReqID;
                        #endif
                    }
                    break;
                }
                case SRVC_EXEC_REQ:
                    if( DMAP_DSMO_OBJ_ID == stGenSrvc.m_stSRVC.m_stExecReq.m_unDstOID )
                        {
                            switch( stGenSrvc.m_stSRVC.m_stExecReq.m_ucMethID )
                            {
                                case DSMO_NEW_KEY:
                                    return UDP_UpdateNewKeyRequest(&stGenSrvc.m_stSRVC.m_stExecReq, p_pRcvMsg->m_u.m_stRf.m_aIPv6DstAddr);
                                case DSMO_DELETE_KEY:
                                    return UDP_UpdateDeleteKeyRequest(&stGenSrvc.m_stSRVC.m_stExecReq, p_pRcvMsg->m_u.m_stRf.m_aIPv6DstAddr);
                                case DSMO_KEY_POLICY_UPDATE:
                                    return UDP_UpdateKeyRequest(&stGenSrvc.m_stSRVC.m_stExecReq, p_pRcvMsg->m_u.m_stRf.m_aIPv6DstAddr);
                                default:
                                    break;
                            }
                        }
                    break;
            }
        }
    }
    return 1;
}
uint8 UDP_AddRfMsgToDll( WCITXT_RCV_MSG * p_pRcvMsg )
{
    uint8  pDLPayload[128];
    uint16 unDLPayloadLen;
    
    // build NL
    unDLPayloadLen = p_pRcvMsg->m_u.m_stRf.m_stNL.m_ucLen;
        
    if( p_pRcvMsg->m_u.m_stRf.m_stNL.m_aBuff[0] == HC_UDP_FULL_COMPRESS ) // if full compress, discard first byte
    {
       unDLPayloadLen --;
    }
    // build TL
    uint16 unTLPayload = TLDE_BuildPaiload( p_pRcvMsg, pDLPayload + unDLPayloadLen );
    if( !unTLPayload )
    {
        return 0;
    }
    
            
        
            // add to queue
        
        
    
    
    
                      
            //choose the key to be updated
     
                            
              
      
    
    unDLPayloadLen += unTLPayload;
    
    memcpy( pDLPayload, p_pRcvMsg->m_u.m_stRf.m_stNL.m_aBuff, p_pRcvMsg->m_u.m_stRf.m_stNL.m_ucLen );    
    
    if( !p_pRcvMsg->m_u.m_stRf.m_stDL.m_ucLen )
    {  
        NLME_ADDR_TRANS_ATTRIBUTES * pAttEntry;
        uint8 aDstAddr[4];        
        
        pAttEntry = NLME_FindATT( p_pRcvMsg->m_u.m_stRf.m_aIPv6DstAddr );
        if( !pAttEntry )
        {
            return 0;
        }
              
        aDstAddr[0] = pAttEntry->m_aShortAddress[0];
        aDstAddr[1] = pAttEntry->m_aShortAddress[1];
        
        pAttEntry = NLME_FindATT( p_pRcvMsg->m_u.m_stRf.m_aIPv6SrcAddr );
        if( !pAttEntry )
        {
            return 0;
        }
        aDstAddr[2] = pAttEntry->m_aShortAddress[0];
        aDstAddr[3] = pAttEntry->m_aShortAddress[1];
                      
        DLDE_Data_Request(4,
                          aDstAddr,
                          p_pRcvMsg->m_u.m_stRf.m_ucPriority | (p_pRcvMsg->m_u.m_stRf.m_ucDiscardEligible << 4) | (p_pRcvMsg->m_u.m_stRf.m_ucECN << 5),  // p_ucPriorityAndFlags
                          p_pRcvMsg->m_u.m_stRf.m_unContractID, // p_unContractID,
                          unDLPayloadLen, // p_ucPayloadLength
                          pDLPayload, // p_pPayload
                          p_pRcvMsg->m_ulMsgNo);  // p_hHandle
    }
    else
    {
        DLDE_Data_Request(8,
                          p_pRcvMsg->m_u.m_stRf.m_stDL.m_aBuff,
                          p_pRcvMsg->m_u.m_stRf.m_ucPriority | (p_pRcvMsg->m_u.m_stRf.m_ucDiscardEligible << 4) | (p_pRcvMsg->m_u.m_stRf.m_ucECN << 5),  // p_ucPriorityAndFlags
                          p_pRcvMsg->m_u.m_stRf.m_unContractID, // p_unContractID,
                          unDLPayloadLen, // p_ucPayloadLength
                          pDLPayload, // p_pPayload
                          p_pRcvMsg->m_ulMsgNo);  // p_hHandle        
    }
//--------------------------------------------   
// if there was a key to be deleted..
  
//---------------------------------------------
    return 1;
}


void UDP_ParseRXTxtMessage( uint8 * p_pTxtMsg, uint16 p_unUdpMsgLen )
{
    p_pTxtMsg[ p_unUdpMsgLen ] = 0;
    
    WCITXT_RCV_MSG stRcvMsg;
    
    if( WCITXT_ConvertTxt2Msg( (char*)p_pTxtMsg, &stRcvMsg ) )
    {
        switch(stRcvMsg.m_ucMsgType)
        {
        case WCITXT_MSG_TO_APP: 
          TLDE_DATA_Indication( g_stDMO.m_aucSysMng128BitAddr,
                               UAP_SMAP_ID,
                               0,
                               stRcvMsg.m_stApp.m_ucLen,
                               stRcvMsg.m_stApp.m_aBuff,                               
                               UAP_DMAP_ID,
                               0 );
          break;
          
        case WCITXT_MSG_TO_RF_AS_RSP:             
        case WCITXT_MSG_TO_RF: 
             if( !UDP_UpdateRfMsg( &stRcvMsg ) )
                  break;

              // add to DLL message queue
              UDP_AddRfMsgToDll( &stRcvMsg );
              break;
        }
    }
}

void   UDP_ParseNTPMessage( const NTP_MESSAGE * p_pNtpMsg )
{
  
    if( g_ulStartTAISec )
    {
        //no NTP needed    
        return;
    }
    
    if( /*((p_pNtpMsg->m_aSettings[0] & 0xC0) != 0xC0) && */ //  server clock synchronized
        ((p_pNtpMsg->m_aSettings[0] & 0x3F) == ((0x04 << 3) | 0x04) ) ) // server reply (version 4 only)
    {
        uint32 ulSec =    ((uint32)(p_pNtpMsg->m_aTxTime[0]) << 24) 
                        | ((uint32)(p_pNtpMsg->m_aTxTime[1]) << 16) 
                        | ((uint32)(p_pNtpMsg->m_aTxTime[2]) << 8) 
                        | ((uint32)(p_pNtpMsg->m_aTxTime[3])); 
                          
        uint32 ulSecFrac = ((uint32)(p_pNtpMsg->m_aTxTime[4]) << 12) 
                        | ((uint32)(p_pNtpMsg->m_aTxTime[5]) << 4) 
                        | ((uint32)(p_pNtpMsg->m_aTxTime[6]) >> 4); 
        
        ulSec -= (uint32)NTP_VS_TAI_OFFSET;
        if( (g_stDMO.m_unCrtUTCDrift != g_stDMO.m_unNextUTCDrift) && (ulSec + g_stDMO.m_unNextUTCDrift) >= g_stDMO.m_ulNextDriftTAI )
        {
            g_stDMO.m_ulNextDriftTAI = 0xFFFFFFFF;
            g_stDMO.m_unCrtUTCDrift = g_stDMO.m_unNextUTCDrift;
            i2c_WriteEEPROM( 0xF0, (uint8*)&g_stDMO.m_unCrtUTCDrift, 2 );            
        }
        else
        {        
            MLSM_SetAbsoluteTAI(  ulSec + g_stDMO.m_unCrtUTCDrift, ulSecFrac );
        }
    }
}

void   UDP_BuildNTPRequestMsg( NTP_MESSAGE * p_pNtpMsg )
{        
    memset( p_pNtpMsg, 0, sizeof(*p_pNtpMsg) );
    p_pNtpMsg->m_aSettings[0] = (0x04 << 3) | 0x03; // version 4, client request
}
