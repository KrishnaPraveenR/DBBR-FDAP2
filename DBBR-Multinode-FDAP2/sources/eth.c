/***************************************************************************************************
* Name:         eth.c
* Author:       Nivis LLC,
* Date:         Aug, 2008
* Description:  This header file provide API of implmentation of ETH controller
* Changes:
* Revisions:
****************************************************************************************************/
#include <string.h>

#include "global.h"
#include "eth.h"
#include "timers.h"
#include "ipv4/udp.h"
#include "ipv4/udpqueue.h"
#include "ethernet/emac.h"
#include "ethernet/IPlookup.h"
#include "ethernet/bcm5221/bcm5221.h"
#include "ethernet/bcm5221/bcm5221_define.h"
#include "isa100/provision.h"
#include "isa100/nlme.h"
#include "isa100/provision.h"
#include "logger/logger.h"
#include "logger/logger_ack.h"
#include "text/text_convertor.h"
#include "wdt.h"
/// Ethernet header structure
#pragma pack(2)
typedef struct _EthHdr
{
    unsigned char       et_dest[6]; /// Destination node
    unsigned char       et_src[6];  /// Source node
    unsigned short      et_protlen; /// Protocol or length
} EthHeader, *PEthHeader;  // GCC

/// ARP header structure
typedef struct _ArpHdr
{
    unsigned short      ar_hrd;     /// Format of hardware address
    unsigned short      ar_pro;     /// Format of protocol address
    unsigned char       ar_hln;     /// Length of hardware address
    unsigned char       ar_pln;     /// Length of protocol address
    unsigned short      ar_op;      /// Operation
    unsigned char       ar_sha[6];  /// Sender hardware address
    unsigned char       ar_spa[4];  /// Sender protocol address
    unsigned char       ar_tha[6];  /// Target hardware address
    unsigned char       ar_tpa[4];  /// Target protocol address
} ArpHeader, *PArpHeader;  // GCC

/// ICMP echo header structure
typedef struct _IcmpEchoHdr {
    unsigned char       type;       /// type of message
    unsigned char       code;       /// type subcode
    unsigned short      cksum;      /// ones complement cksum of struct
    unsigned short      id;         /// identifier
    unsigned short      seq;        /// sequence number
} IcmpEchoHeader, *PIcmpEchoHeader;    // GCC

/// UDP echo header structure
typedef struct _UdpHdr {
    unsigned short      udp_src;    /// UDP source port
    unsigned short      udp_dst;    /// UDP destination port
    unsigned short      udp_len;    /// Length of UDP packet
    unsigned short      udp_xsum;   /// Checksum    
} UdpHeader, *PUdpHeader;    // GCC

typedef union _IPdata {
    IcmpEchoHeader imp_hdr;
    UdpHeader      udp_hdr;
} IpData, *PIpData;    // GCC

/// IP Header structure
typedef struct _IPheader {
    unsigned char       ip_hl_v;    /// header length and version
    unsigned char       ip_tos;     /// type of service
    unsigned short      ip_len;     /// total length
    unsigned short      ip_id;      /// identification
    unsigned short      ip_off;     /// fragment offset field
    unsigned char       ip_ttl;     /// time to live
    unsigned char       ip_p;       /// protocol
    unsigned short      ip_sum;     /// checksum
    unsigned char       ip_src[4];  /// Source IP address
    unsigned char       ip_dst[4];  /// Destination IP address
    IpData              ip_data;
} IpHeader, *PIpHeader;    // GCC


typedef struct _EthMsg {
    EthHeader     m_stEthHdr;
    union
    {
       ArpHeader m_stArpHdr;
       IpHeader m_stIpHdr;
    } m_uEthPk;
} EthMsg;    // GCC

#pragma pack()

  /// ARP OP codes


#define ETH_HDR_LEN sizeof( EthHeader )
#define SWAP16(x) ((uint16)(x) << 8 | (uint16)(x) >> 8)

uint8 c_aETH_MAC[6];

#define ETH_PROT_IP             0x0800 // 2048  (0x0800) IPv4
#define ETH_PROT_ARP            0x0806 // 2054  (0x0806) ARP

#define ETH_PROT_IP_LE          0x0008 // litle endian
#define ETH_PROT_ARP_LE         0x0608 // litle endian

#define ARP_REQUEST             0x0001
#define ARP_REPLY               0x0002

#define ARP_REQUEST_LE      0x0100 // litle endian
#define ARP_REPLY_LE        0x0200 // litle endian

/// IP protocoles
// http://www.iana.org/assignments/protocol-numbers
#define IP_PROT_ICMP            1
#define IP_PROT_IP              4
#define IP_PROT_TCP             6
#define IP_PROT_UDP             17

 /// ICMP types
// http://www.iana.org/assignments/icmp-parameters
#define ICMP_ECHO_REPLY         0x00 // Echo reply (used to ping)
                            // 1 and 2  Reserved
#define ICMP_DEST_UNREACHABLE   0x03 // Destination Unreachable
#define ICMP_SOURCE_QUENCH      0x04 // Source Quench
#define ICMP_REDIR_MESSAGE      0x05 // Redirect Message
#define ICMP_ALT_HOST_ADD       0x06 // Alternate Host Address
                            //  0x07    Reserved
#define ICMP_ECHO_REQUEST       0x08 // Echo Request
#define ICMP_ROUTER_ADV         0x09 // Router Advertisement
#define ICMP_ROUTER_SOL         0x0A // Router Solicitation
#define ICMP_TIME_EXC           0x0B // Time Exceeded
#define ICMP_PARAM_PB           0x0C // Parameter Problem: Bad IP header
#define ICMP_TIMESTAMP          0x0D // Timestamp
#define ICMP_TIMESTAMP_REP      0x0E // Timestamp Reply
#define ICMP_INFO_REQ           0x0F // Information Request
#define ICMP_INFO_REPLY         0x10 // Information Reply
#define ICMP_ADD_MASK_REQ       0x11 // Address Mask Request
#define ICMP_ADD_MASK_REP       0x12 // Address Mask Reply
                            //  0x13    Reserved for security
                            //  0X14 through 0x1D Reserved for robustness experiment
#define ICMP_TRACEROUTE         0x1E // Traceroute
#define ICMP_DAT_CONV_ERROR     0x1F // Datagram Conversion Error
#define ICMP_MOB_HOST_RED       0x20 // Mobile Host Redirect
#define ICMP_W_A_Y              0x21 // Where-Are-You (originally meant for IPv6)
#define ICMP_H_I_A              0x22 // Here-I-Am (originally meant for IPv6)
#define ICMP_MOB_REG_REQ        0x23 // Mobile Registration Request
#define ICMP_MOB_REG_REP        0x24 // Mobile Registration Reply
#define ICMP_DOM_NAME_REQ       0x25 // Domain Name Request
#define ICMP_DOM_NAME_REP       0x26 // Domain Name Reply
#define ICMP_SKIP_ALGO_PROT     0x27 // SKIP Algorithm Discovery Protocol, Simple Key-Management for Internet Protocol
#define ICMP_PHOTURIS           0x28 // Photuris, Security failures
#define ICMP_EXP_MOBIL          0x29 // ICMP for experimental mobility protocols such as Seamoby [RFC4065]


#define NTP_REQUEST_TIMEOUT 200 // 2s
#define ARP_REQUEST_TIMEOUT 16 // ^ 2 // 150 - 160 ms
#define ARP_RETRIES_NO      4 

    #define  UDP_DATA_OFFSET ( sizeof(EthHeader) + sizeof(IpHeader) ) // IpHeader contains also ARP and UDP headers (8 bytes both) 

uint16 g_unArpRequestCounter;
uint16 g_unNTPReqCounter;

static uint8 ETH_sendMsg( EthMsg * p_pEthMsg, uint16 p_unEthMsgLen, const uint8 * p_pDstMAC );
static void arp_process_packet( EthMsg * p_pEthMsg, unsigned int p_unEthMsgLen);
static void ip_process_packet(EthMsg * p_pEthMsg, unsigned int p_unEthMsgLen);
static uint16 IcmpChksum(const uint8 *p_pData, uint16 p_unDataLen);

volatile int v =0;
void ETH_Init(void)
{   
    volatile uint32 j, k;
    uint8 linkFlag =0;
    
    LED2_OFF();
    for( j = 0; j < 5; j++ )
    {
      LED1_OFF();
      LED3_OFF(); //666
      for (k=0; k<200000; k++);
  
      LED1_ON();
      LED3_ON(); //666
      for (k=0; k<200000; k++);
      FEED_WDT();
    }
  
    EMAC_Init(AT91C_ID_EMAC, c_aETH_MAC, EMAC_CAF_DISABLE, EMAC_NBC_DISABLE);
    BCM5221_InitPhy();  
   
    LED1_OFF();
    LED2_OFF();
    LED3_OFF(); //666
  
    // Ethernet Hardware Initialization Delay
    FEED_WDT();
    for (volatile uint32 Index = 0; Index < 2000000; Index++);
    FEED_WDT();
    
    // Dummy read initially
    BCM5221_IsLinkActive();
    for (volatile uint32 Index = 0; Index < 2000000; Index++);
    FEED_WDT();

    // Dummy read initially
   linkFlag =  BCM5221_IsLinkActive();
    
    while(!linkFlag)
    {      
        FEED_WDT();
        LED1_ON();
        LED3_ON(); // 666 
        LED2_ON();
        for (volatile uint32 Index = 0; Index < 200000; Index++);
        linkFlag = BCM5221_IsLinkActive();
    }
    /*for(j = 0; j < 5; j++ )
    {
      LED2_OFF();
     LED3_OFF(); //666
      for (k=0; k<200000; k++);
  
      LED2_ON();
      LED3_ON(); //666
      for (k=0; k<200000; k++);
      FEED_WDT();
    }*/
    LED1_OFF();
    LED2_OFF();
    LED3_OFF();    // 666
    

}

void ETH_Inc10msCounter()
{ 
    if( g_unArpRequestCounter ) 
        g_unArpRequestCounter++; 
    
    if( !g_ulStartTAISec )
    {
        //connection with NTP server expected
        g_unNTPReqCounter ++;
    }
}

void ETH_Interrupt(void)
{
    if( (AT91C_BASE_EMAC->EMAC_ISR & AT91C_EMAC_RCOMP) || (AT91C_BASE_EMAC->EMAC_RSR & AT91C_EMAC_REC) )
    {
        AT91C_BASE_EMAC->EMAC_RSR = AT91C_EMAC_REC | AT91C_EMAC_OVR | AT91C_EMAC_BNA;
        TMR_SetEmacTimestamp();
    }
    
    if( (AT91C_BASE_EMAC->EMAC_ISR & AT91C_EMAC_TCOMP) || (AT91C_BASE_EMAC->EMAC_TSR & AT91C_EMAC_COMP) )
    {
        AT91C_BASE_EMAC->EMAC_TSR = AT91C_EMAC_COMP | AT91C_EMAC_RLES | AT91C_EMAC_COL | AT91C_EMAC_BEX | AT91C_EMAC_UND;
    }
    
    
    
    AT91C_BASE_AIC->AIC_ICCR = (1L << AT91C_ID_EMAC); // clear EMAC interrupt flag
}

const uint8 MACFF[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const uint8 ARP_FRAME_FREZE[] = {
                      0x00, 0x01, //ar_hrd
                      0x08, 0x00, //ar_pro
                      6,          //hln
                      4,          //pln
                      0x00, 0x01 //ar_op
                                          };
void ARP_RequestMAC(EthMsg * p_pEthMsg, uint32 p_ulIP)
{
    p_pEthMsg->m_stEthHdr.et_protlen = ETH_PROT_ARP_LE;
    memcpy(&(p_pEthMsg->m_uEthPk.m_stArpHdr.ar_hrd), ARP_FRAME_FREZE, sizeof(ARP_FRAME_FREZE));
    memcpy(p_pEthMsg->m_uEthPk.m_stArpHdr.ar_sha, c_aETH_MAC, 6); 
    memcpy(p_pEthMsg->m_uEthPk.m_stArpHdr.ar_spa, g_aIPv4Address, 4);
    memset(p_pEthMsg->m_uEthPk.m_stArpHdr.ar_tha,0,46-18);
//    memcpy(p_pEthMsg->m_uEthPk.m_stArpHdr.ar_tha, MACFF, 6); 
    if( (p_ulIP & c_ulMASK) != (c_ulIP & c_ulMASK)) {
        p_ulIP = c_ulGWY;  
    }
    memcpy(p_pEthMsg->m_uEthPk.m_stArpHdr.ar_tpa, &p_ulIP, 4);
  
  ETH_sendMsg(p_pEthMsg, 60, MACFF); 
}

void UDP_BuidIpv4Hdr(EthMsg * p_pEthMsg, uint32 p_theIP, uint16 unEthMsgLen, uint16 p_unSrcPort, uint16 p_unDstPort )
{
    static uint16 s_unDataGramID = 0;
    
    unEthMsgLen += 20 + 8;
    
    // IP header
    p_pEthMsg->m_stEthHdr.et_protlen = ETH_PROT_IP_LE;
    p_pEthMsg->m_uEthPk.m_stIpHdr.ip_hl_v = 0x45; // header length and version
    p_pEthMsg->m_uEthPk.m_stIpHdr.ip_tos = 0x00; // type of service
    p_pEthMsg->m_uEthPk.m_stIpHdr.ip_len = SWAP16(unEthMsgLen); // total length
    p_pEthMsg->m_uEthPk.m_stIpHdr.ip_id = ++s_unDataGramID; // identification
    p_pEthMsg->m_uEthPk.m_stIpHdr.ip_off = 0; // fragment offset field
    p_pEthMsg->m_uEthPk.m_stIpHdr.ip_ttl = 64; // time to live
    p_pEthMsg->m_uEthPk.m_stIpHdr.ip_p = 17; // protocol //UDP // 1 (ICMPv4), 2 (IGMPv4), 6 (TCP), and 17 (UDP). 
    p_pEthMsg->m_uEthPk.m_stIpHdr.ip_sum = 0; // checksum
    memcpy(p_pEthMsg->m_uEthPk.m_stIpHdr.ip_src, g_aIPv4Address, 4); // Source IP address
    memcpy(p_pEthMsg->m_uEthPk.m_stIpHdr.ip_dst, &p_theIP, 4);  /// Destination IP address
    
    //calculating check sum IP
    p_pEthMsg->m_uEthPk.m_stIpHdr.ip_sum = IcmpChksum((uint8 *)&p_pEthMsg->m_uEthPk.m_stIpHdr, 20);
    
    unEthMsgLen -= 20;
    
    // UDP header
    p_pEthMsg->m_uEthPk.m_stIpHdr.ip_data.udp_hdr.udp_src = p_unSrcPort; //UDP source port
    p_pEthMsg->m_uEthPk.m_stIpHdr.ip_data.udp_hdr.udp_dst = p_unDstPort; //UDP destination port
    p_pEthMsg->m_uEthPk.m_stIpHdr.ip_data.udp_hdr.udp_len = SWAP16(unEthMsgLen);              // Length of UDP packet
    p_pEthMsg->m_uEthPk.m_stIpHdr.ip_data.udp_hdr.udp_xsum = 0;                       // Checksum
}

void ETH_Task(void)
{
    unsigned char aEthMsg[UDP_DATA_OFFSET + UDPMSG_MAX_SIZE];
    uint16 unEthMsgLen, unLogEthMsgLen=0;
    uint32 ulIpv4;
    
    EthMsg * pEthMsg = (EthMsg *)aEthMsg;
            
    // TX task
        
    if( g_unNTPReqCounter < NTP_REQUEST_TIMEOUT ) 
    {
        NWK_IPV6_UDP_HDR * pIpv6Hdr = (NWK_IPV6_UDP_HDR *)(pEthMsg+1);
        
        unEthMsgLen = UDPQ_GetOutMsg( (uint8*)pIpv6Hdr );        
        if( unEthMsgLen )
        {   
            NLME_ROUTE_ATTRIBUTES * pRoute = NLME_FindDestinationRoute( pIpv6Hdr->m_aIpv6DstAddr );
            if( pRoute /*&& pRoute-> m_bOutgoingInterface*/ ) // not found or not ETH
            {
                // populate hops no from IPv6 packet based on route
                pIpv6Hdr->m_ucHopLimit = pRoute->m_ucNWK_HopLimit;
                uint16 unDstPort;
                memcpy( &ulIpv4, pRoute->m_aNextHopAddress + 12, 4 );
                memcpy( &unDstPort, pRoute->m_aNextHopAddress + 10, 2 ); //dest port from IPv6.destIP 

                UDP_BuidIpv4Hdr( pEthMsg, ulIpv4, unEthMsgLen, 
                                c_unPort,               //source port from IPv6.IP 
                                unDstPort          
                                );        
            }
            else
            {    
                 Log( LOG_ERROR, LOG_M_NL, NLOG_FindRoute, 16, pIpv6Hdr->m_aIpv6DstAddr );
                 UDPQ_DelOutMsg( unEthMsgLen );
                 unEthMsgLen = 0;
            }
        }
        else
        {
#ifdef WCI_SUPPORT
            const uint8 * pWciMsg = WCI_GetHeadMessage();
            
            if( pWciMsg )
            {                
                unLogEthMsgLen = WCITXT_ConvertMsg2Txt(  WCI_GetMessageId(),
                                                         (WCITXT_TX_MSG*)(pWciMsg+4), 
                                                         (uint8*)(pEthMsg+1) );
                                
                ulIpv4 = c_ulIP4LOGAck;
                
                UDP_BuidIpv4Hdr( pEthMsg, ulIpv4, unLogEthMsgLen, 
                              c_usPort4LOGAck, c_usPort4LOGAck );
            }
            else 
#endif // WCI_SUPPORT             
            {  
                //if there is no message then send a logmsg if present
                unLogEthMsgLen = LOGQ_GetOutMsg( (uint8*)pIpv6Hdr );
                if( unLogEthMsgLen )
                { 
                    ulIpv4 = c_ulIP4LOG;
                    UDP_BuidIpv4Hdr( pEthMsg, ulIpv4, unLogEthMsgLen, 
                                    c_usPort4LOG, c_usPort4LOG );        
                  
                }
            }
        }
    }
    else // NTP request time
    {
        unEthMsgLen = sizeof(NTP_MESSAGE);
        memcpy( &ulIpv4, g_stDMO.m_aucSysMng128BitAddr + 12, 4 );
        
         UDP_BuildNTPRequestMsg( (NTP_MESSAGE*)(pEthMsg+1));                 
         UDP_BuidIpv4Hdr( pEthMsg, ulIpv4, unEthMsgLen, SWAP16(123), SWAP16(123) );     
    }

    
    if( unEthMsgLen ) // have to send an UDP message
    {  
        const uint8 * pDstMac = GetMacOfIP( ulIpv4 );
                
        if( pDstMac )
        {
            g_unArpRequestCounter = 0;
          
            if( ETH_sendMsg( pEthMsg, sizeof(*pEthMsg) + unEthMsgLen, pDstMac ) != EMAC_TX_BUFFER_BUSY )
            {
                if( g_unNTPReqCounter < NTP_REQUEST_TIMEOUT )
                {
                    UDPQ_DelOutMsg( unEthMsgLen );
                }
                else
                {
                    g_unNTPReqCounter = 0;
                }
            }
        }
        else if( ! (g_unArpRequestCounter & (ARP_REQUEST_TIMEOUT-1)) ) // not an ARP request on progress
        {
              ARP_RequestMAC(pEthMsg, ulIpv4);             
              // exceed the no of req, discard the message
              if(  (++g_unArpRequestCounter) > (ARP_REQUEST_TIMEOUT * ARP_RETRIES_NO) ) 
              {
                  g_unArpRequestCounter = 0;
                  if( g_unNTPReqCounter < NTP_REQUEST_TIMEOUT )
                  {
                      UDPQ_DelOutMsg( unEthMsgLen );
                  }
              }
        }    
    }
    else if(unLogEthMsgLen) // have to send an LOG message
    {
        const uint8 * pDstMac = GetMacOfIP( ulIpv4 );
    
        if( pDstMac )
        {
            if( ETH_sendMsg( pEthMsg, sizeof(*pEthMsg) + unLogEthMsgLen, pDstMac ) != EMAC_TX_BUFFER_BUSY )
            {
                WCI_DeleteHeadMessage();
            }
        }
        else
        {
              ARP_RequestMAC(pEthMsg, ulIpv4);          
        }    
    }

    // RX task    
    while( EMAC_Poll( aEthMsg, sizeof(aEthMsg), &unEthMsgLen) == EMAC_RX_OK )
    {
        // extract eth header  
        switch( pEthMsg->m_stEthHdr.et_protlen )
        {
        case ETH_PROT_ARP_LE: arp_process_packet(pEthMsg, unEthMsgLen); break;           
        case ETH_PROT_IP_LE:  ip_process_packet(pEthMsg, unEthMsgLen); break;
        }                
    }
}


static uint8 ETH_sendMsg( EthMsg * p_pEthMsg, uint16 p_unEthMsgLen, const uint8 * p_pDstMAC )
{    
    memcpy( p_pEthMsg->m_stEthHdr.et_dest, p_pDstMAC, 6 ); // load dst mac first since can be on same buffer
    memcpy( p_pEthMsg->m_stEthHdr.et_src, c_aETH_MAC, 6 );
    
    EMAC_ClearTxBuff();
    
    return EMAC_Send( (uint8*)p_pEthMsg, p_unEthMsgLen );
}

//-----------------------------------------------------------------------------
/// Process the received ARP packet
/// Just change address and send it back
/// \param pData The data to process
/// \param size The data size
//-----------------------------------------------------------------------------
static void arp_process_packet( EthMsg * p_pEthMsg, unsigned int p_unEthMsgLen)
{
    
    if (  *(uint16*)(p_pEthMsg->m_uEthPk.m_stArpHdr.ar_tpa+2) == *(uint16*)(g_aIPv4Address+2) 
        && *(uint16*)(p_pEthMsg->m_uEthPk.m_stArpHdr.ar_tpa)   == *(uint16*)(g_aIPv4Address) ) 
    {
        switch( p_pEthMsg->m_uEthPk.m_stArpHdr.ar_op )
        {
        case ARP_REQUEST_LE: // request for my IP
              // ARP REPLY operation
              p_pEthMsg->m_uEthPk.m_stArpHdr.ar_op =  ARP_REPLY_LE;
              
              // swap sender IP address and target IP address
              memcpy( p_pEthMsg->m_uEthPk.m_stArpHdr.ar_tha, p_pEthMsg->m_uEthPk.m_stArpHdr.ar_sha, 10 ); // MAC + IP
              
              memcpy( p_pEthMsg->m_uEthPk.m_stArpHdr.ar_sha, c_aETH_MAC, 6 );
              memcpy( p_pEthMsg->m_uEthPk.m_stArpHdr.ar_spa, g_aIPv4Address, 4 );                      
              
              ETH_sendMsg( p_pEthMsg, p_unEthMsgLen, p_pEthMsg->m_stEthHdr.et_src );
              break;
    
        case ARP_REPLY_LE: // reply for my request
              AddMacOfIP( (uint16*)p_pEthMsg->m_uEthPk.m_stArpHdr.ar_spa, (uint16*)p_pEthMsg->m_uEthPk.m_stArpHdr.ar_sha );              
              break;
        }
    }
}

//-----------------------------------------------------------------------------
/// Process the received IP packet
/// Just change address and send it back
/// \param pData The data to process
/// \param size The data size
//-----------------------------------------------------------------------------
static void ip_process_packet(EthMsg * p_pEthMsg, unsigned int p_unEthMsgLen)
{
    if( (p_pEthMsg->m_uEthPk.m_stIpHdr.ip_hl_v & 0xF0) != 0x40 ) // ipv4 version only
        return;
    
    uint16 unLen = (p_pEthMsg->m_uEthPk.m_stIpHdr.ip_hl_v & 0x0F) * 4;
    
    if( unLen + sizeof(EthHeader) + sizeof(UdpHeader) > p_unEthMsgLen ) // invlaid hdr len (icmp and udp headers has same size)
        return;
    
    PIpData pIpData = (PIpData)((char *)&p_pEthMsg->m_uEthPk.m_stIpHdr + unLen );
    
    switch( p_pEthMsg->m_uEthPk.m_stIpHdr.ip_p )
    {
    case IP_PROT_UDP:
      
        // chect my IP
        if(  *(uint16*)(p_pEthMsg->m_uEthPk.m_stIpHdr.ip_dst+2) == *(uint16*)(g_aIPv4Address+2)
          && *(uint16*)p_pEthMsg->m_uEthPk.m_stIpHdr.ip_dst == *(uint16*)g_aIPv4Address )
        {
            // ntp packet from SM          
            if( pIpData->udp_hdr.udp_dst == SWAP16(123)             // NTP port
               && *(uint16*)(p_pEthMsg->m_uEthPk.m_stIpHdr.ip_src+2) == 
                            *(uint16*)(g_stDMO.m_aucSysMng128BitAddr + 14) 
               && *(uint16*)(p_pEthMsg->m_uEthPk.m_stIpHdr.ip_src) == 
                            *(uint16*)(g_stDMO.m_aucSysMng128BitAddr + 12) )
            {
                UDP_ParseNTPMessage( (NTP_MESSAGE*)((uint8*)pIpData+sizeof(UdpHeader)) );
            }
            //ISA100 port from IPv6 obsolates-> // ISA100 ports -> 0xF0B0 ... 0xF0BF
            else  if( pIpData->udp_hdr.udp_dst == c_unPort )
            {           
            #ifdef RESET_TO_FACTORY
                //in order to not hardcode the SS IPv4 and Port
                memcpy( (uint8*)&c_ulIP4LOGAck, p_pEthMsg->m_uEthPk.m_stIpHdr.ip_src, sizeof(c_ulIP4LOGAck) );
            #endif
                    
                UDP_ParseRXTxtMessage( (uint8*)pIpData+sizeof(UdpHeader), 
                                        p_unEthMsgLen - unLen - sizeof(EthHeader) - sizeof(UdpHeader) );
            }
#ifdef WCI_SUPPORT
            //LOGAck port
            else if ( pIpData->udp_hdr.udp_dst == c_usPort4LOGAck) 
            {
              //UDP_ParseLOGAckMessage();
              LOGAckQ_AckOutMsg( (unsigned char *)((unsigned)pIpData+sizeof(UdpHeader)-1) );
            }
#endif // WCI_SUPPORT
        }
        break;
        
    case IP_PROT_ICMP: // icmp -> implement ping echo only
          // if ICMP_ECHO_REQUEST ==> resp = ICMP_ECHO_REPLY
          if(pIpData->imp_hdr.type == ICMP_ECHO_REQUEST) 
          {
              pIpData->imp_hdr.type = ICMP_ECHO_REPLY;
              pIpData->imp_hdr.code = 0;
              pIpData->imp_hdr.cksum = 0;

              // Checksum of the ICMP Message

              pIpData->imp_hdr.cksum = IcmpChksum((uint8 *)pIpData, p_unEthMsgLen - unLen - sizeof(EthHeader) );
              // Swap IP Dest address and IP Source address
              
              memcpy( p_pEthMsg->m_uEthPk.m_stIpHdr.ip_dst, p_pEthMsg->m_uEthPk.m_stIpHdr.ip_src, 4 );
              memcpy( p_pEthMsg->m_uEthPk.m_stIpHdr.ip_src, g_aIPv4Address, 4 );
              ETH_sendMsg( p_pEthMsg, p_unEthMsgLen, p_pEthMsg->m_stEthHdr.et_src );
          }
          break;
    }
}

//-----------------------------------------------------------------------------
/// Process & return the ICMP checksum
/// \param p Pointer to the buffer to process
/// \param len The length of the bufferred data
//-----------------------------------------------------------------------------
uint16 IcmpChksum(const uint8 *p_pData, uint16 p_unDataLen)
{
    uint32 t = 0;   

    // p_pData is 2 bytes aligned
    for (  ;p_unDataLen > 1 ; p_unDataLen -= 2 ) 
    {        
        t += *(uint16*)p_pData;        
        p_pData += 2;
    }
    
    if( p_unDataLen )
    {
        t += *p_pData;                  
    }

    uint16 unResult = ~((uint16)t + (uint16)(t >> 16)); 
    return unResult;
}
