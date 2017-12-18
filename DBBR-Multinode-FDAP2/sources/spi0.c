/***************************************************************************************************
* Name:         spi0.c
* Author:       Nivis LLC,
* Date:         Aug, 2008
* Description:  This header file is provided ...
*               This file holds definitions of the ...
* Changes:
* Revisions:
****************************************************************************************************/

#include "spi0.h"
#include "timers.h"

#ifdef BBR2_HW
    enum eSPI0_CurrentAccessType     eSPI0_CurrentAccess = SPI0_NONE_RESERVED;
#endif
void SPI0_Init(void)
{
    AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SPIDIS; // Disable SPI 
    AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SWRST;  // Reset the SPI. A software-triggered hardware reset of SPI. SPI is in slave mode  
   
    AT91C_BASE_PIOA->PIO_ASR = 0
                              | AT91C_PA12_SPI0_NPCS0 // EEPROM (flash)
                              | AT91C_PA13_SPI0_NPCS1 // pa dac
                              | AT91C_PA14_SPI0_NPCS2 // therm 
                              | AT91C_PA16_SPI0_MISO
                              | AT91C_PA17_SPI0_MOSI
                              | AT91C_PA18_SPI0_SPCK
                              ;                   
    
    AT91C_BASE_PIOA->PIO_PDR = 0
                              | AT91C_PA12_SPI0_NPCS0 // EEPROM (flash)
                              | AT91C_PA13_SPI0_NPCS1 // pa dac
                              | AT91C_PA14_SPI0_NPCS2 // therm
                              | AT91C_PA16_SPI0_MISO
                              | AT91C_PA17_SPI0_MOSI
                              | AT91C_PA18_SPI0_SPCK                          
                              ;                   
    
   AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_SPI0);
   AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_SPIEN; // Enable de SPI0 to transfer and receive data
  
  
   AT91C_BASE_SPI0->SPI_MR = 0
                              | AT91C_SPI_MSTR
                              | AT91C_SPI_PS_VARIABLE  // PS=1
//                              | AT91C_SPI_PCSDEC
                              | AT91C_SPI_MODFDIS
//                              | AT91C_SPI_PCS       // is used only if PS=0
                              | (MCK_DIV_FOR_10MHZ << 24) // (SPI) Delay Between Chip Selects (108 ns)
                              ;

   AT91C_BASE_SPI0->SPI_CSR[0] = 0
//                        | AT91C_SPI_CPOL            // CPOL=1;
                        | AT91C_SPI_NCPHA             // NCPHA=1;
                        | AT91C_SPI_BITS_8            // 8 bits per transfer
                        | AT91C_SPI_CSAAT             // Chip Select Active After Transfer
                        | (MCK_DIV_FOR_10MHZ << 8)    // Serial clock baud rate equals MCK/6 => 9,216 Mhz
                        | (MCK_DIV_FOR_10MHZ << 16)   // (SPI) Delay Before SPCK (108 ns)
//                        | (MCK_DIV_FOR_10MHZ << 24) // (SPI) Delay Between Consecutive Transfers
                          ;  
   #ifdef BBR1_HW
   AT91C_BASE_SPI0->SPI_CSR[1] = 0
//                        | AT91C_SPI_CPOL           // CPOL=1;
//                        | AT91C_SPI_NCPHA          // NCPHA=1;
                        |  AT91C_SPI_BITS_16         // 16 bits per transfer
                        | AT91C_SPI_CSAAT            // Chip Select Active After Transfer
                        | (MCK_DIV_FOR_10MHZ <<  8)  // Serial clock baud rate equals MCK/6 => 9,216 Mhz
                        | (MCK_DIV_FOR_10MHZ << 16)  // (SPI) Delay Before SPCK (108 ns)
//                        | (1 << 24)  // (SPI) Delay Between Consecutive Transfers (578 ns)
                        ;  
   #endif
   #ifdef BBR2_HW
   AT91C_BASE_SPI0->SPI_CSR[1] = 0  //  CPOL=0 and NCPHA=1 ?!?!?!? 
                               | AT91C_SPI_NCPHA  // NCPHA=1;
                               | AT91C_SPI_BITS_8 // 8 bits per transfer
                               | (9 << 8)         // Serial clock baud rate equals MCK / 8 (around 6 MHZ)
                               | AT91C_SPI_CSAAT  // Chip Select Active After Transfer 
                               | (0x04 << 16 )    // Delay before SPCK is 1/2 the SPCK clock period , value/MCK = 72ns
                           ;    
   eSPI0_CurrentAccess = SPI0_NONE_RESERVED;
   #endif
  
    AT91C_BASE_SPI0->SPI_CSR[2] = 0
//                        | AT91C_SPI_CPOL          // CPOL=1;
                        | AT91C_SPI_NCPHA           // NCPHA=1;
                        | AT91C_SPI_CSAAT           // Chip Select Active After Transfer
                        | AT91C_SPI_BITS_8          // 8 bits per transfer
                        | (MCK_DIV_FOR_10MHZ << 8)  // Serial clock baud rate equals MCK/6 => 9,216 Mhz
                        | (MCK_DIV_FOR_10MHZ << 16) // (SPI) Delay Before SPCK (108 ns)
                        | (0 << 24)                 // (SPI) Delay Between Consecutive Transfers
                        ;
} 
#ifdef BBR2_HW
void SPI0_Radio2_WriteBuff( const unsigned char * p_pSPIMsg,  unsigned char p_ucMsgLen )
{
   while (p_ucMsgLen--)
  {  
     SPI0_Radio2_WriteByte(*(p_pSPIMsg++)) ;             
  }
  SPI0_WaitEndTx();   
}
void SPI0_Radio2_WriteBuffReverse( const unsigned char * p_pSPIMsg,  unsigned char p_ucMsgLen )
{
  p_pSPIMsg += p_ucMsgLen-1; 
  while (p_ucMsgLen--)
  {  
    SPI0_Radio2_WriteByte(*(p_pSPIMsg--)) ;              
  }
  SPI0_WaitEndTx();   
}
void SPI0_Radio2_ReadBuff( unsigned char* p_pSPIMsg, unsigned char p_ucMsgLen )
{ 
  if( p_ucMsgLen )
  {
      SPI0_Radio2_WriteByte(0x00); // start read cicle  
      while (--p_ucMsgLen)    
      {  
         SPI0_WaitEndTx();
         *p_pSPIMsg = (unsigned char)AT91C_BASE_SPI0->SPI_RDR;            
         AT91C_BASE_SPI0->SPI_TDR = (0x00 | (1L << 16)); // start new read cycle                
         p_pSPIMsg++;                            
      } 
      SPI0_WaitEndTx();
      *p_pSPIMsg = (unsigned char)AT91C_BASE_SPI0->SPI_RDR;   
  }
}
void SPI0_Radio2_ReadBuffReverse( unsigned char* p_pSPIMsg, unsigned char p_ucMsgLen )
{        
  SPI0_Radio2_WriteByte(0x00); // start read cicle  
  p_pSPIMsg += p_ucMsgLen-1;
  while (--p_ucMsgLen)    
  {  
    SPI0_WaitEndTx();
    *p_pSPIMsg = (unsigned char)AT91C_BASE_SPI0->SPI_RDR;                                                        
    AT91C_BASE_SPI0->SPI_TDR = (0x00 | (1L << 16));  // start new read cycle      
    p_pSPIMsg--;                            
  }    
  SPI0_WaitEndTx();
  *p_pSPIMsg = (unsigned char)AT91C_BASE_SPI0->SPI_RDR;   
}
#endif
