
/***************************************************************************************************
* Name:         usart.c
* Author:       NIVIS LLC, Dorin Muica
* Date:         September, 2008
* Description:  This file is provided ...
*               This file holds USART0 implementation (support at start of program the provisioning data)
* Changes:
* Revisions:
****************************************************************************************************/

#include "typedef.h"
#include "usart.h"
#include "at91sam7x512.h"
#include "wdt.h"
#include "i2c.h"
#include "string.h"
#include "./logger/err.h"
#include "./isa100/provision.h"

#define RS232_INIT_TIMEOUT    5000000   // 3 sec delay
#define RS232_CMD_TIMEOUT     5*RS232_INIT_TIMEOUT 

#ifdef BBR1_HW
inline uint8 USART0_readByte(void) { return AT91C_BASE_US0->US_RHR & 0xFF; }
inline uint8 USART0_isByteReceived(void) { return AT91C_BASE_US0->US_CSR & AT91C_US_RXRDY; }
#endif

#ifdef BBR2_HW //666
inline uint8 USART0_readByte(void) { return AT91C_BASE_DBGU->DBGU_RHR & 0xFF; }
inline uint8 USART0_isByteReceived(void) { return AT91C_BASE_DBGU->DBGU_CSR & AT91C_US_RXRDY; }
#endif

//inline uint8 USART0_readByte(void) { return AT91C_BASE_US0->US_RHR & 0xFF; }
//inline uint8 USART0_isByteReceived(void) { return AT91C_BASE_US0->US_CSR & AT91C_US_RXRDY; }



inline void USART0_writeByte( uint8 p_ucByte)   
{
#ifdef BBR1_HW  
        AT91C_BASE_US0->US_THR = p_ucByte; 
        while ( !( AT91C_BASE_US0->US_CSR & AT91C_US_TXEMPTY) );
#endif         
#ifdef BBR2_HW
        AT91C_BASE_DBGU->DBGU_THR = p_ucByte; 
        while ( !( AT91C_BASE_DBGU->DBGU_CSR & AT91C_US_TXEMPTY) );
#endif
}

inline void USART0_disable(void) 
{
#ifdef BBR1_HW    
    AT91C_BASE_US0->US_CR = AT91C_US_TXDIS | AT91C_US_RXDIS ; 
    AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_US0);
#endif
#ifdef BBR2_HW
    AT91C_BASE_DBGU->DBGU_CR = AT91C_US_TXDIS | AT91C_US_RXDIS ; 
#endif
}


unsigned int provisionParse(const unsigned char *p_pucCmd, unsigned int p_unCnt );
unsigned int extractBinary( const unsigned char *p_pucCmd, unsigned char *p_pucBin, unsigned int p_unBinLen );

static const unsigned char c_strError[] = "ERROR";

void USART0_Init(void)
{
  AT91C_BASE_PIOA->PIO_PDR = 0  // Disables the PIO from controlling the corresponding pins
                               | AT91C_PA0_RXD0    // USART 0 Receive Data
                               | AT91C_PA1_TXD0    // USART 0 Transmit Data
                               | AT91C_PA27_DRXD   // DBGU Debug Receive Data
                               | AT91C_PA28_DTXD   // DBGU Debug Transmit Data
                                 ;
  
  AT91C_BASE_PIOA->PIO_ASR = 0 // Assigns the I/O line to the Peripheral A function
                               | AT91C_PA0_RXD0    // USART 0 Receive Data
                               | AT91C_PA1_TXD0    // USART 0 Transmit Data
                               | AT91C_PA27_DRXD   // DBGU Debug Receive Data
                               | AT91C_PA28_DTXD   // DBGU Debug Transmit Data
                                 ;
#ifdef BBR1_HW  
  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_US0);  // Enables USART0 peripheral clock
  
  AT91C_BASE_US0->US_CR = 0 
//                            | AT91C_US_RSTRX    // bit 2, Reset Receiver
//                            | AT91C_US_RSTTX    // bit 3, Reset Transmitter
                            | AT91C_US_RXEN       // bit 4, Receiver Enable
//                            | AT91C_US_RXDIS    // bit 5, Receiver Disable
                            | AT91C_US_TXEN       // bit 6, Transmitter Enable
//                            | AT91C_US_TXDIS    // bit 7, Transmitter Disable
                            | AT91C_US_RSTSTA     // bit 8, Reset Status Bits
//                            | AT91C_US_STTBRK   // bit 9, Start Break
//                            | AT91C_US_STPBRK   // bit 10, Stop Break
//                            | AT91C_US_STTTO    // bit 11, Start Time-out 
//                            | AT91C_US_SENDA    // bit 12, Send Address
//                            | AT91C_US_RSTIT    // bit 13, Reset Iterations
//                            | AT91C_US_RSTNACK  // bit 14, Reset Non Acknowledge
//                            | AT91C_US_RETTO    // bit 15, Rearm Time-out
//                            | AT91C_US_DTREN    // bit 16, Data Terminal ready Enable
//                            | AT91C_US_DTRDIS   // bit 17, Data Terminal ready Disable
//                            | AT91C_US_RTSEN    // bit 18, Request to Send enable
//                            | AT91C_US_RTSDIS   // bit 19, Request to Send Disable  
                              ;

  AT91C_BASE_US0->US_MR = 0 
                // USART_MODE
                            | AT91C_US_USMODE_NORMAL        // (0x0) (USART) Normal
//                            | AT91C_US_USMODE_RS485       // (0x1) (USART) RS485
//                            | AT91C_US_USMODE_HWHSH       // (0x2) (USART) Hardware Handshaking
//                            | AT91C_US_USMODE_MODEM       // (0x3) (USART) Modem  
//                            | AT91C_US_USMODE_ISO7816_0   // (0x4) (USART) ISO7816 protocol: T = 0 
//                            | AT91C_US_USMODE_ISO7816_1   // (0x6) (USART) ISO7816 protocol: T = 1  
//                            | AT91C_US_USMODE_IRDA        // (0x8) (USART) IrDA  
//                            | AT91C_US_USMODE_SWHSH       // (0xC) (USART) Software Handshaking 
                 // Clock Selection
                            | AT91C_US_CLKS_CLOCK    // MCK
//                            | AT91C_US_CLKS_FDIV1  // MCK/DIV (DIV=8)
//                            | AT91C_US_CLKS_EXT    // SCK  
                 // Character Length
//                             | AT91C_US_CHRL_5_BITS  // Character Length: 5 bits
//                             | AT91C_US_CHRL_6_BITS  // Character Length: 6 bits
//                             | AT91C_US_CHRL_7_BITS  // Character Length: 7 bits
                             | AT91C_US_CHRL_8_BITS    // Character Length: 8 bits
                  // Synchronous Mode Select
//                             | AT91C_US_SYNC  // (USART) Synchronous Mode Select
                   // Parity type 
                              | (0x04 << 9) // No parity
                   // Number of stop bits
                              | AT91C_US_NBSTOP_1_BIT  // 1 stop bit
//                              | AT91C_US_NBSTOP_15_BIT // Asynchronous (SYNC=0) 1.5 stop bits Synchronous (SYNC=1) reserved  
//                              | AT91C_US_NBSTOP_2_BIT  // 2 stop bits 
                    // Channel Mode  
                              | AT91C_US_CHMODE_NORMAL    // Normal Mode
//                               | AT91C_US_CHMODE_AUTO    // Automatic Echo 
//                               | AT91C_US_CHMODE_LOCAL   // Local loopback 
//                               | AT91C_US_CHMODE_REMOTE  // Remote Loopback
 
                    // Bit order
//                               | AT91C_US_MSBF // Most significant bit is sent/received first
                     // 9-bit character length
//                               | AT91C_US_MODE9 9-bit character length 
                     // Clock Output Select
//                               | AT91C_US_CKLO //The USART drives the SCK pin if USCLKS doesn't select the external clock SCK
                     // Oversampling mode
                                | AT91C_US_OVER
                     // Inhibit Non Acknowledge
//                               | AT91C_US_INACK
                     // Disable Successive NACK             
//                               | AT91C_US_DSNACK
                     // MAX_ITERATION 
//                               | AT91C_US_MAX_ITER 
                      // Infrared receive line filter
//                               | AT91C_US_FILTER                                         
                              ;
              
 AT91C_BASE_US0->US_BRGR = 0x2D0; // CD=( 3 * 18 432 000 hz )/(9600 BAUD *8) for OVER=1 (Oversampling mode =8x)
#endif
#ifdef BBR2_HW
  AT91C_BASE_PMC->PMC_PCER = ( 1 << AT91C_ID_PIOA );
 
  AT91C_BASE_DBGU->DBGU_MR  = AT91C_US_USMODE_NORMAL    |  // normal mode
                              AT91C_US_PAR_NONE         |  // no parity
                              AT91C_US_CHMODE_NORMAL;      // normal (no loopback that is) mode
  AT91C_BASE_DBGU->DBGU_CR  = //AT91C_US_RSTRX |              // reset receiver
                              AT91C_US_RXEN |              // receiver disabled
                              AT91C_US_TXEN |               // transmitter disabled
                              AT91C_US_RSTSTA ;
  AT91C_BASE_DBGU->DBGU_BRGR = 360; 
#endif
}


void USART0_writeBytes( const unsigned char *p_pucBuff, unsigned int p_unLen )
{
    while( p_unLen-- )
    {
        USART0_writeByte( *(p_pucBuff++) );
    }
}

///////////////////////////////////////////////////////////////////////////////////
// Name: USART0_Provision
// Author: 
// Description: Write provisioning commands from console to global buffer
//              Delay time : 3 seconds
// Parameters: none
//          
// Return: none
//
/////////////////////////////////////////////////////////////////////////////////// 

void USART0_Provision(void)
{
  unsigned int unCnt = RS232_INIT_TIMEOUT;
  unsigned int j=0;
  unsigned char ucByteReceived;
  unsigned char ucFirstTime = 1;
  unsigned char aBuf[1024];
  
  USART0_Init();
    
  USART0_writeBytes("\r\n>",3); 
  
  do
  {
    FEED_WDT();
    if( USART0_isByteReceived() )
    {
      ucByteReceived = USART0_readByte();
      if(  ucByteReceived == '\r' )
      {
          if( ucFirstTime )
          {
              unCnt = RS232_CMD_TIMEOUT;
              ucFirstTime = 0;
          }

          if( j )
          {
              USART0_writeBytes("\r\n",2);   
              
              aBuf[j] = 0;
              unCnt = provisionParse(aBuf, unCnt );
              j=0;
          }            
          USART0_writeBytes("\r\n>",3);   
      }
      else if(j < (sizeof(aBuf)-1) )
      {
          if( ucByteReceived != '\n' )
          {
              aBuf[j++]=ucByteReceived;          
          }
      }
    }
  }
  while( unCnt-- );
  
  USART0_writeByte('.');   
  USART0_disable();
}

const unsigned char c_aHex[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
void USART0_writeHexBytes( const unsigned char *p_pucBuff, unsigned int p_unLen )
{
    
    USART0_writeByte(':');
    while( p_unLen-- )
    {
      
        USART0_writeByte( c_aHex[(*p_pucBuff) >> 4] );
        USART0_writeByte( c_aHex[(*p_pucBuff) & 0x0F] );
        p_pucBuff ++;
    }
}

void USART0_writeHexBytesLE( const unsigned char *p_pucBuff, unsigned int p_unLen )
{
    
    USART0_writeByte(':');
    p_pucBuff += p_unLen;
    while( p_unLen-- )
    {
        p_pucBuff --;
      
        USART0_writeByte( c_aHex[(*p_pucBuff) >> 4] );
        USART0_writeByte( c_aHex[(*p_pucBuff) & 0x0F] );
    }
}

unsigned int provisionParse(const unsigned char *p_pucCmd, unsigned int p_unCnt )
{
    static const unsigned char p[] = "CMD:E - exit, S - signature, M - manufacturing, P - provisioning, ? - read data";
//    static const unsigned char c_strM[] = "ver:EUI64:VRef:MaxPA:CristalTrim\r\nM";
//    static const unsigned char c_strP[] = "\r\nnver:subnetId:subnetMask:joinK:dllJoinK:ipv6SMAddr:ipv4Addr:ipv4Mask:ipv4GW\r\nP";
    static const unsigned char c_strM[] = "M";
    static const unsigned char c_strP[] = "\r\nP";

    
    struct 
    {
        uint16 m_unFormatVersion;            // 0x0000
        uint8  m_aMAC[8];                    // 0x0002
        uint16 m_unVRef;                     // 0x000A
        uint8  m_ucMaxPA;                    // 0x000C
        uint8  m_ucCristal;                  // 0x000D
    } c_stManufacturing;
    
    BBR_UART_PROVISIONING c_stProvisioning;
    
    switch( p_pucCmd[0] )
    {
    case 'E':  return 0;
    case 'S':
                if( p_pucCmd[1] == 'B' && p_pucCmd[2] == 'B' && p_pucCmd[3] == 'R' )
                {
                    return 0xFFFFFFFF;
                }
                USART0_writeBytes(c_strError, sizeof(c_strError)-1 );
                break;
    case '?':
                i2c_ReadEEPROM(0x00, (uint8*)&c_stManufacturing, sizeof(c_stManufacturing) );
                i2c_ReadEEPROM(0x80, (uint8*)&c_stProvisioning, sizeof(c_stProvisioning) );
                
                USART0_writeBytes( c_strM, sizeof(c_strM)-1 ); 
                USART0_writeHexBytes( (uint8*)&c_stManufacturing.m_unFormatVersion, sizeof(c_stManufacturing.m_unFormatVersion) );
                USART0_writeHexBytes( (uint8*)c_stManufacturing.m_aMAC, sizeof(c_stManufacturing.m_aMAC) );
                USART0_writeHexBytes( (uint8*)&c_stManufacturing.m_unVRef, sizeof(c_stManufacturing.m_unVRef) );
                USART0_writeHexBytes( (uint8*)&c_stManufacturing.m_ucMaxPA, sizeof(c_stManufacturing.m_ucMaxPA) );
                USART0_writeHexBytes( (uint8*)&c_stManufacturing.m_ucCristal, sizeof(c_stManufacturing.m_ucCristal) );
                
                USART0_writeBytes( c_strP, sizeof(c_strP)-1 );                 
                
#pragma diag_suppress=Pa039   
                
                USART0_writeHexBytes( (uint8*)&c_stProvisioning.m_unFormatVersion, sizeof(c_stProvisioning.m_unFormatVersion) );                
                
                USART0_writeHexBytesLE( (uint8*)&c_stProvisioning.m_unSubnetID, sizeof(c_stProvisioning.m_unSubnetID) );
                USART0_writeHexBytesLE( (uint8*)&c_stProvisioning.m_unFilterBitMask, sizeof(c_stProvisioning.m_unFilterBitMask) );
                USART0_writeHexBytesLE( (uint8*)&c_stProvisioning.m_unFilterTargetID, sizeof(c_stProvisioning.m_unFilterTargetID) );

                USART0_writeBytes( ":x:x:x", 6 );
//                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_aAppJoinKey, sizeof(c_stProvisioning.m_aAppJoinKey) );
//                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_aDllJoinKey, sizeof(c_stProvisioning.m_aDllJoinKey) );
//                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_aProvisionKey, sizeof(c_stProvisioning.m_aProvisionKey) );
                
                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_aSecMngrEUI64, sizeof(c_stProvisioning.m_aSecMngrEUI64) ); 
                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_aSysMngrIPv6, sizeof(c_stProvisioning.m_aSysMngrIPv6) );
                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_aIpv6BBR, sizeof(c_stProvisioning.m_aIpv6BBR) );
                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_aSysMngrIPv4, sizeof(c_stProvisioning.m_aSysMngrIPv4) );  // redundant data
                
                USART0_writeBytes( ":x", 2 );  // reserved0 data
//                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_aReserved0, sizeof(c_stProvisioning.m_aReserved0) );
                
                USART0_writeHexBytesLE( (uint8*)&c_stProvisioning.m_nCrtUTCDrift, sizeof(c_stProvisioning.m_nCrtUTCDrift) );
                USART0_writeHexBytesLE( (uint8*)c_stProvisioning.m_aNextTimeUTCDrift, sizeof(c_stProvisioning.m_aNextTimeUTCDrift) );
                USART0_writeHexBytesLE( (uint8*)&c_stProvisioning.m_nNextUTCDrift, sizeof(c_stProvisioning.m_nNextUTCDrift) );
                
                USART0_writeBytes( ":x", 2 );  // reserved1 data
//                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_aReserved1, sizeof(c_stProvisioning.m_aReserved1) );                
                
                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_aIPv4BBR, sizeof(c_stProvisioning.m_aIPv4BBR) );  // redundant data
                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_aIPv4BBRMask, sizeof(c_stProvisioning.m_aIPv4BBRMask) );
                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_aIPv4GWY, sizeof(c_stProvisioning.m_aIPv4GWY) );
                
                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_aIP4LOG, sizeof(c_stProvisioning.m_aIP4LOG) );
                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_aPort4LOG, sizeof(c_stProvisioning.m_aPort4LOG) );
                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_ucVerboseLOG, sizeof(c_stProvisioning.m_ucVerboseLOG) );          
#ifdef WCI_SUPPORT                
                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_aIP4LOGAck, sizeof(c_stProvisioning.m_aIP4LOGAck) );
                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_aPort4LOGAck, sizeof(c_stProvisioning.m_aPort4LOGAck) );
                USART0_writeHexBytes( (uint8*)c_stProvisioning.m_ucVerboseLOGAck, sizeof(c_stProvisioning.m_ucVerboseLOGAck) );          
#endif // WCI_SUPPORT                
#pragma diag_default=Pa039 
                
                break;
    case 'M':
                if( extractBinary( p_pucCmd+1, (uint8*)&c_stManufacturing, sizeof(c_stManufacturing) ) )
                {
                    i2c_WriteEEPROM(0x00, (const uint8*)&c_stManufacturing, sizeof(c_stManufacturing) );
                }
                break;
    case 'P':
                if( extractBinary( p_pucCmd+1, (uint8*)&c_stProvisioning, sizeof(c_stProvisioning) ) )
                {
                    c_stProvisioning.m_unSubnetID = __swap_bytes( c_stProvisioning.m_unSubnetID );
                    c_stProvisioning.m_unFilterBitMask = __swap_bytes( c_stProvisioning.m_unFilterBitMask );
                    c_stProvisioning.m_unFilterTargetID = __swap_bytes( c_stProvisioning.m_unFilterTargetID );
                    c_stProvisioning.m_nCrtUTCDrift = __swap_bytes( c_stProvisioning.m_nCrtUTCDrift );
                    c_stProvisioning.m_nNextUTCDrift = __swap_bytes( c_stProvisioning.m_nNextUTCDrift );  // not used; kept for compatibility with windows app
                      
                    //set default the custom StartTAI parameter
                    c_stProvisioning.m_ulInitialTAISec = 0x00000000;
                    c_stProvisioning.m_ucPALevel = CHIPCON_DEFAULT_PA_LEVEL;
                        
                    i2c_WriteEEPROM(0x80, (const uint8*)&c_stProvisioning, sizeof(c_stProvisioning) );
                }      
                break;
    default:
                USART0_writeBytes( p, sizeof(p)-1 );
  }     
  
  return p_unCnt;
}

const unsigned char * extractHex( const unsigned char *p_pucCmd, unsigned char *p_pucBin )
{
    do
    {
        if( *p_pucCmd >='0' && *p_pucCmd <='9' )
        {
            *p_pucBin = (*(p_pucCmd++) - '0') << 4;
            break;
        }
        else if( *p_pucCmd >='A' && *p_pucCmd <='F' )
        {
            *p_pucBin = (*(p_pucCmd++) - 'A' + 10) << 4;
            break;
        }
        else if( *(p_pucCmd++) == 0 )
            return NULL;
    }
    while(1);
    
    if( *p_pucCmd >='0' && *p_pucCmd <='9' )
    {
        *p_pucBin |= (*(p_pucCmd++) - '0');
    }
    else if( *p_pucCmd >='A' && *p_pucCmd <='F' )
    {
        *p_pucBin |= (*(p_pucCmd++) - 'A' + 10);
    }
    else
        return NULL;
    
    return p_pucCmd;
}
          
unsigned int extractBinary( const unsigned char *p_pucCmd, unsigned char *p_pucBin, unsigned int p_unBinLen )
{
    while( p_unBinLen-- )
    {
        p_pucCmd = extractHex( p_pucCmd, p_pucBin++ );
        if( !p_pucCmd )
        {
            USART0_writeBytes(c_strError, sizeof(c_strError)-1 );
            return 0;
        }
    }
    return 1;
}

