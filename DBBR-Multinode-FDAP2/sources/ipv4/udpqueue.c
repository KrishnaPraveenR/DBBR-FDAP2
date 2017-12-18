/***************************************************************************************************
* Name:         udpqueue.c
* Author:       Nivis LLC, Ion Ticus
* Date:         Sep, 2008
* Description:  This file is provided implmentation of UDP circular queue
* Changes:
* Revisions:
****************************************************************************************************/

#include "udpqueue.h"

UDP_QUEUE g_stUDPOut;

void UDPQ_Init(void)
{
    g_stUDPOut.m_unLen = 0;
    g_stUDPOut.m_pStart = g_stUDPOut.m_pBuf;  
}

uint8 * UDPQ_setData( uint8 * p_pQData, const uint8 * p_pSrc, const uint8 * p_pRollover, uint16 p_unSize);

////////////////////////////////////////////////////////////////////////////////////////////////////
/// UDPQ_PushMsg2
/// @author NIVIS LLC, Ion Ticus
/// @brief  add a message in que.
/// @remarks
///      Access level: user level 
///      Context: 
////////////////////////////////////////////////////////////////////////////////////////////////////
uint16 UDPQ_PushOutMsg( const NWK_IPV6_UDP_HDR * p_pHdr, const uint8 * p_pMsg, uint16 p_unMsgLen  )
{     
    if( g_stUDPOut.m_unLen + p_unMsgLen <= UDPQUEUE_BUFFER_SIZE - sizeof(NWK_IPV6_UDP_HDR) - 2 )
    {
        uint8 * pEnd = g_stUDPOut.m_pStart + g_stUDPOut.m_unLen;
        const uint8 * pRollover = g_stUDPOut.m_pBuf + UDPQUEUE_BUFFER_SIZE;
        
        p_unMsgLen += sizeof(NWK_IPV6_UDP_HDR);        

        if( pEnd >= pRollover ) pEnd -= UDPQUEUE_BUFFER_SIZE;
        *(pEnd++) = (uint8)(p_unMsgLen);               
        if( pEnd >= pRollover ) pEnd -= UDPQUEUE_BUFFER_SIZE;
        *(pEnd++) = (uint8)((p_unMsgLen) >> 8);
        
        UDPQ_setData( pEnd, (const uint8 *)p_pHdr, pRollover, sizeof(NWK_IPV6_UDP_HDR) );        
        UDPQ_setData( pEnd + sizeof(NWK_IPV6_UDP_HDR), p_pMsg, pRollover, p_unMsgLen-sizeof(NWK_IPV6_UDP_HDR) );        
        
        g_stUDPOut.m_unLen += p_unMsgLen+2;
        return 1;
    }

    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// UDPQ_GetOutMsg
/// @author NIVIS LLC, Ion Ticus
/// @brief  get first message from out queue
/// @remarks
///      Access level: user level 
///      Context: 
////////////////////////////////////////////////////////////////////////////////////////////////////
uint16 UDPQ_GetOutMsg( uint8 * p_pMsg )
{
    uint16 unMsgLen = 0;
    if( g_stUDPOut.m_unLen >= 2 )
    {
        uint8 * pStart = g_stUDPOut.m_pStart;
        uint16 unIdx;

        if( pStart >= (g_stUDPOut.m_pBuf+UDPQUEUE_BUFFER_SIZE) ) pStart -= UDPQUEUE_BUFFER_SIZE;
        unMsgLen = *(pStart++); 
        if( pStart >= (g_stUDPOut.m_pBuf+UDPQUEUE_BUFFER_SIZE) ) pStart -= UDPQUEUE_BUFFER_SIZE;
        unMsgLen |= ((uint16)(*(pStart++))) << 8; 
        
        unIdx = unMsgLen; 
        
        while( unIdx -- )
        {
              if( pStart >= (g_stUDPOut.m_pBuf+UDPQUEUE_BUFFER_SIZE) ) pStart -= UDPQUEUE_BUFFER_SIZE;              
              *(p_pMsg++) = *(pStart++);          
        }
    }

    return unMsgLen;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// UDPQ_DelOutMsg
/// @author NIVIS LLC, Ion Ticus
/// @brief  delete first message from out queue
/// @remarks
///      Access level: user level 
///      Context: 
////////////////////////////////////////////////////////////////////////////////////////////////////
void UDPQ_DelOutMsg( uint16 p_unMsgLen )
{    
    p_unMsgLen += 2;
    if( g_stUDPOut.m_unLen >= p_unMsgLen )
    {        
        uint8 * pStart = g_stUDPOut.m_pStart + p_unMsgLen;

        if( pStart >= (g_stUDPOut.m_pBuf+UDPQUEUE_BUFFER_SIZE) ) pStart -= UDPQUEUE_BUFFER_SIZE;
        
        g_stUDPOut.m_pStart = pStart;
        g_stUDPOut.m_unLen -= p_unMsgLen;
    }
}


uint8 * UDPQ_setData( uint8 * p_pQData, const uint8 * p_pSrc, const uint8 * p_pRollover, uint16 p_unSize)
{
    while( p_unSize -- )
    {
          if( p_pQData >= p_pRollover ) 
          {
              p_pQData -= UDPQUEUE_BUFFER_SIZE; 
          }
          
          *(p_pQData++) = *(p_pSrc++);          
    }
    
    return p_pQData;
}
