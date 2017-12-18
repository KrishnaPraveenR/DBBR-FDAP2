/***************************************************************************************************
* Name:         UDP.h
* Author:       Nivis LLC,
* Date:         Aug, 2008
* Description:  This header file provide API of light UDP implmentation
* Changes:
* Revisions:
****************************************************************************************************/
#ifndef _NIVIS_UDP_H_
#define _NIVIS_UDP_H_

    #include "../isa100/nlde.h"

    #define UDPMSG_MAX_SIZE   1280
    #define UDP_IPV4_HDR_SIZE 20

    #define NTP_VS_TAI_OFFSET (1830297600) // seconds between NTP time and TAI time (01/01/1900 -> 01/01/1958)

    typedef struct
    {
        uint8  m_aSettings[4];
        uint32 m_ulRootDelay;
        uint32 m_ulRootDispertion;
        uint32 m_ulRefID;
        uint8  m_aRefTime[8];
        uint8  m_aOrigTime[8];
        uint8  m_aRcvTime[8];
        uint8  m_aTxTime[8];                
    }NTP_MESSAGE;

    void UDP_Init( void );
    
    void   UDP_ParseRXTxtMessage( uint8 * p_pTxtMsg, uint16 p_unUdpMsgLen );
    void   UDP_ParseNTPMessage( const NTP_MESSAGE * p_pNtpMsg );
    void   UDP_BuildNTPRequestMsg( NTP_MESSAGE * p_pNtpMsg );

#endif // _NIVIS_UDP_H_

