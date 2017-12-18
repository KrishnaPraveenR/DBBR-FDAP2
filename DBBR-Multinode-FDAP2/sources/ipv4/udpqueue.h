/***************************************************************************************************
* Name:         udpqueue.h
* Author:       Nivis LLC, Ion Ticus
* Date:         Sep, 2008
* Description:  This file is provided implmentation of UDP circular queue
* Changes:
* Revisions:
****************************************************************************************************/
#ifndef _NIVIS_UDPQUEUE_H_
#define _NIVIS_UDPQUEUE_H_

#include "../typedef.h"
#include "../global.h"
#include "udp.h"

#define UDPQUEUE_BUFFER_SIZE (UDPMSG_MAX_SIZE + 200)

typedef struct
{
    uint16  m_unLen;
    uint8 * m_pStart;
    uint8   m_pBuf[UDPQUEUE_BUFFER_SIZE];
} UDP_QUEUE;


extern UDP_QUEUE g_stUDPOut;

void UDPQ_Init(void);
uint16 UDPQ_PushOutMsg( const NWK_IPV6_UDP_HDR * p_pHdr, const uint8 * p_pMsg, uint16 p_unMsgLen );
uint16 UDPQ_GetOutMsg( uint8 * p_pMsg );
void UDPQ_DelOutMsg( uint16 p_unMsgLen );


#endif // _NIVIS_UDPQUEUE_H_

