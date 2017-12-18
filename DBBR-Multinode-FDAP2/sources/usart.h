/***************************************************************************************************
* Name:         USART.h
* Author:       
* Date:         September, 2008
* Description:  This header file provide ...
* Changes:
* Revisions:
****************************************************************************************************/

#ifndef _NIVIS_USART_H_
#define _NIVIS_USART_H_

#pragma pack(1)
    typedef struct 
    {
      uint16 m_unFormatVersion;                   // 0x0080 
      uint16 m_unSubnetID;                        // 0x0082
      uint16 m_unFilterBitMask;                   // 0x0084
      uint16 m_unFilterTargetID;                  // 0x0086             
      uint8  m_aAppJoinKey[16];                   // 0x0088 
      uint8  m_aDllJoinKey[16];                   // 0x0098
      uint8  m_aProvisionKey[16];                 // 0x00A8    
      uint8  m_aSecMngrEUI64[8];                  // 0x00B8
      uint8  m_aSysMngrIPv6[16];                  // 0x00C0
      uint8  m_aIpv6BBR[16];                      // 0x00D0
      uint8  m_aSysMngrIPv4[4];                   // 0x00E0       redundant data (last 4 bytes from corresponding IPv6); the space allocated remains
      
      uint8  m_aReserved0[12];                    // 0x00E4       reserved0 data : 12 bytes
      
      int16  m_nCrtUTCDrift;                      // 0x00F0
      uint8  m_aNextTimeUTCDrift[4];              // 0x00F2
      int16  m_nNextUTCDrift;                     // 0x00F6
      
      uint32 m_ulInitialTAISec;                   // 0x00F8                          
      uint8  m_ucPALevel;                         // 0x00FC     
      uint8  m_aReserved1[3];                     // 0x00FD       reserved1 data : 3 bytes
      
      
      uint8  m_aIPv4BBR[4];                       // 0x0100       redundant data (last 4 bytes from corresponding IPv6); the space allocated remains
      uint8  m_aIPv4BBRMask[4];                   // 0x0104      
      uint8  m_aIPv4GWY[4];                       // 0x0108
      
      uint8  m_aIP4LOG[4];                        // 0x010C
      uint8  m_aPort4LOG[2];                      // 0x0110
      uint8  m_ucVerboseLOG[NUMBER_OF_MODULES];   // 0x0112
      
#ifdef WCI_SUPPORT 
      uint8  m_aIP4LOGAck[4];                        // 0x0122
      uint8  m_aPort4LOGAck[2];                      // 0x0126
      uint8  m_ucVerboseLOGAck[NUMBER_OF_MODULES];   // 0x0128
                                                     // 0x0138
#endif
    } BBR_UART_PROVISIONING;
#pragma pack()
                                       

void USART0_Init(void);
void USART0_Provision(void);


#endif // _NIVIS_USART_H_
