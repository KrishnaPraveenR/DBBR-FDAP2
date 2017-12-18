/***************************************************************************************************
* Name:         spi1.c
* Author:       Nivis LLC,
* Date:         Aug, 2008
* Description:  This header file is provided ...
*               This file holds definitions of the ...
* Changes:
* Revisions:
****************************************************************************************************/

#include "spi1.h"
#include "digitals.h"
#include "timers.h"


void SPI1_Init(void)
{    
    AT91C_BASE_SPI1->SPI_CR = AT91C_SPI_SPIDIS; //Disable SPI1 
    AT91C_BASE_SPI1->SPI_CR = AT91C_SPI_SWRST; //Reset the SPI. A software-triggered hardware reset of SPI. SPI is in slave mode
    
   #ifdef BBR1_HW 

//    CC2420_MACRO_RELEASE();
    
    AT91C_BASE_PIOA->PIO_PDR = 0
//                              | AT91C_PA21_SPI1_NPCS0  // Disables the PIO from controlling the corresponding pin 21 , SPI 1 Peripheral Chip Select 0
                              | AT91C_PA22_SPI1_SPCK   // Disables the PIO from controlling the corresponding pin 22 , SPI 1 Serial Clock
                              | AT91C_PA23_SPI1_MOSI   // Disables the PIO from controlling the corresponding pin 23 , SPI 1 Master Out Slave
                              | AT91C_PA24_SPI1_MISO   // Disables the PIO from controlling the corresponding pin 24 , SPI 1 Master In Slave
                              ;                   
   
    
    AT91C_BASE_PIOA->PIO_BSR = 0
//                              | AT91C_PA21_SPI1_NPCS0 // Assigns the I/O line to the peripheral B function, SPI 1 Peripheral Chip Select 0
                              | AT91C_PA22_SPI1_SPCK  // Assigns the I/O line to the peripheral B function, SPI 1 Serial Clock
                              | AT91C_PA23_SPI1_MOSI  // Assigns the I/O line to the peripheral B function, SPI 1 Master Out Slave
                              | AT91C_PA24_SPI1_MISO  // Assigns the I/O line to the peripheral B function, SPI 1 Master In Slave
                              ;                   
    
    
    
  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_SPI1);
  AT91C_BASE_SPI1->SPI_CR = AT91C_SPI_SPIEN; //Enable de SPI1 to transfer and receive data
  
  AT91C_BASE_SPI1->SPI_MR = 0
                           | AT91C_SPI_MSTR      // SPI Mode Register: MSTR=1, SPI1 is in Master Mode
//                           | AT91C_SPI_PS        // SPI Mode Register: PS=0, Fixed Peripheral Select 
//                           | AT91C_SPI_PCSDEC    // SPI Mode Register: PCSDEC=0, The chips selects are directly connected to a peripheral device
                           | AT91C_SPI_MODFDIS   // SPI Mode Register: MODFDIS=1, Mode Fault Detection is disabled
//                           | AT91C_SPI_LLB       // SPI Mode Register: LLB=0, Local Loopback path disabled  
//                   | AT91C_SPI_PCS       // SPI Mode Register:  PCS=0xF , No peripheral is selected
//                           | (0x0E << 16)        // NPCS0 line is active low, CC2420 is selected
                           ;
 
  AT91C_BASE_SPI1->SPI_CSR[0] = 0  //  CPOL=0 and NCPHA=1 ?!?!?!? 
//                    | AT91C_SPI_CPOL   // Chip Select Register[0] : CPOL=1;
                               | AT91C_SPI_NCPHA  // Chip Select Register[0] : NCPHA=1;
                               | AT91C_SPI_BITS_8 // Chip Select Register[0] : 8 bits per transfer
                               | (MCK_DIV_FOR_10MHZ << 8)         // Chip Select Register[0] : Serial clock baud rate equals MCK / 8
                               | AT91C_SPI_CSAAT  // Chip Select Register[0] : Chip Select Active After Transfer 
                               | (0x0 << 16 )     // Delay before SPCK is 1/2 the SPCK clock period , 54.25 ns
                               ;    
   #endif
   #ifdef BBR2_HW
    AT91C_BASE_PIOA->PIO_PDR = 0
                              | AT91C_PA21_SPI1_NPCS0  // Disables the PIO from controlling the corresponding pin 21 , SPI 1 Peripheral Chip Select 0
                              | AT91C_PA22_SPI1_SPCK   // Disables the PIO from controlling the corresponding pin 22 , SPI 1 Serial Clock
                              | AT91C_PA23_SPI1_MOSI   // Disables the PIO from controlling the corresponding pin 23 , SPI 1 Master Out Slave
                              | AT91C_PA24_SPI1_MISO   // Disables the PIO from controlling the corresponding pin 24 , SPI 1 Master In Slave
                              ;                   
    AT91C_BASE_PIOA->PIO_BSR = 0
                              | AT91C_PA21_SPI1_NPCS0 // Assigns the I/O line to the peripheral B function, SPI 1 Peripheral Chip Select 0
                              | AT91C_PA22_SPI1_SPCK  // Assigns the I/O line to the peripheral B function, SPI 1 Serial Clock
                              | AT91C_PA23_SPI1_MOSI  // Assigns the I/O line to the peripheral B function, SPI 1 Master Out Slave
                              | AT91C_PA24_SPI1_MISO  // Assigns the I/O line to the peripheral B function, SPI 1 Master In Slave
                              ;     
  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_SPI1);
  AT91C_BASE_SPI1->SPI_MR = 0
                           | AT91C_SPI_MSTR      // SPI Mode Register: MSTR=1, SPI1 is in Master Mode
                           | AT91C_SPI_MODFDIS   // SPI Mode Register: MODFDIS=1, Mode Fault Detection is disabled
                             ;
    AT91C_BASE_SPI1->SPI_CSR[0] = 0  //  CPOL=0 and NCPHA=1 ?!?!?!? 
                               | AT91C_SPI_NCPHA  // Chip Select Register[0] : NCPHA=1;
                               | AT91C_SPI_BITS_8 // Chip Select Register[0] : 8 bits per transfer
                               | (9 << 8)         // Chip Select Register[0] : Serial clock baud rate equals MCK / 8 (around 6 MHZ)
                               | AT91C_SPI_CSAAT  // Chip Select Register[0] : Chip Select Active After Transfer 
                               | (0x04 << 16 )     // Delay before SPCK is 1/2 the SPCK clock period , Value/MCK = 72ns
                           ;    
  AT91C_BASE_SPI1->SPI_CR = AT91C_SPI_SPIEN; //Enable de SPI1 to transfer and receive data
   #endif
  
}


void SPI1_WriteBuff( const unsigned char * p_pSPIMsg,  unsigned char p_ucMsgLen )
{
  // Slave SPI device should be selected before calling this functions
   while (p_ucMsgLen--)
  {  
     SPI1_WriteByte(*(p_pSPIMsg++)) ;             
  }
  
  SPI1_WaitEndTx();   
}

void SPI1_WriteBuffReverse( const unsigned char * p_pSPIMsg,  unsigned char p_ucMsgLen )
{
  // Slave SPI device should be selected before calling this functions
  
  p_pSPIMsg += p_ucMsgLen-1; 
  while (p_ucMsgLen--)
  {  
    SPI1_WriteByte(*(p_pSPIMsg--)) ;              
  }

  SPI1_WaitEndTx();   
}

void SPI1_ReadBuff( unsigned char* p_pSPIMsg, unsigned char p_ucMsgLen )
{ 
  if( p_ucMsgLen )
  {
      SPI1_WriteByte(0x00); // start read cicle  
      
      while (--p_ucMsgLen)    
      {  
         SPI1_WaitEndTx();
         *p_pSPIMsg = (unsigned char)AT91C_BASE_SPI1->SPI_RDR;            
         AT91C_BASE_SPI1->SPI_TDR = 0x00; // start new read cycle                
         p_pSPIMsg++;                            
      } 
      SPI1_WaitEndTx();
      *p_pSPIMsg = (unsigned char)AT91C_BASE_SPI1->SPI_RDR;   
  }
}


void SPI1_ReadBuffReverse( unsigned char* p_pSPIMsg, unsigned char p_ucMsgLen )
{        
  SPI1_WriteByte(0x00); // start read cicle  
  p_pSPIMsg += p_ucMsgLen-1;
  
  while (--p_ucMsgLen)    
  {  
    SPI1_WaitEndTx();
    *p_pSPIMsg = (unsigned char)AT91C_BASE_SPI1->SPI_RDR;                                                        
    AT91C_BASE_SPI1->SPI_TDR = 0x00;  // start new read cycle      
    p_pSPIMsg--;                            
  }    
  SPI1_WaitEndTx();
  *p_pSPIMsg = (unsigned char)AT91C_BASE_SPI1->SPI_RDR;   
}
