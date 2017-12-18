/***************************************************************************************************
* Name:         spi1.h
* Author:       Nivis LLC,
* Date:         Aug, 2008
* Description:  This header file is provided ...
*               This file holds definitions of the ...
* Changes:
* Revisions:
****************************************************************************************************/
#ifndef _NIVIS_SPI1_H_
#define _NIVIS_SPI1_H_

   #include "digitals.h"
   #include "system.h"

  #define SPI1_WaitEndTx(void)       while ( !( AT91C_BASE_SPI1->SPI_SR & AT91C_SPI_TXEMPTY) )
  #define SPI1_CkIfCanTx(void)       while ( !(  AT91C_BASE_SPI1->SPI_SR & AT91C_SPI_TDRE) )

// attention !!, next macros are not {} protected so be carefull when use it

/*
  inline void SPI1_WriteByte( uint8 p_ucByte ) 
  { 
    SPI1_CkIfCanTx(); 
    AT91C_BASE_SPI1->SPI_TDR = p_ucByte; 
  }
*/
  #define SPI1_WriteByte(p_ucByte)   SPI1_CkIfCanTx(); AT91C_BASE_SPI1->SPI_TDR = p_ucByte

  #ifdef BBR1_HW
  #define SPI1_ReadByte(p_ucSpiRead)   \
          SPI1_WriteByte(CC2420_SNOP); \
          SPI1_WaitEndTx(); \
          p_ucSpiRead = (unsigned char)AT91C_BASE_SPI1->SPI_RDR
  #endif

  #ifdef BBR2_HW
  #define SPI1_ReadByte(p_ucSpiRead)   \
          SPI1_WriteByte(CC2520_SNOP); \
          SPI1_WaitEndTx(); \
          while(!(AT91C_BASE_SPI1->SPI_SR & AT91C_SPI_RDRF));\
          p_ucSpiRead = (unsigned char)AT91C_BASE_SPI1->SPI_RDR
  #endif

#ifdef BBR1_HW
   #define CC2420_MACRO_SELECT()  SET_GPIO_LO( CC2420_SPI_SELECT )
   #define CC2420_MACRO_RELEASE() SET_GPIO_HI( CC2420_SPI_SELECT )
  
#pragma inline 
void CC2420_MACRO_RESELECT(void) 
{
    CC2420_MACRO_RELEASE(); 
//    _NOP(); // add 4 clock tics delay
//    _NOP();
//    _NOP(); 
    _NOP(); 
    CC2420_MACRO_SELECT();
}

#endif
#ifdef BBR2_HW
   #define CC2520_1_MACRO_SELECT()  ; // Not Required now
   #define CC2520_1_MACRO_RELEASE()  _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); \
                                     AT91C_BASE_SPI1->SPI_CR = AT91C_SPI_LASTXFER;
#pragma inline 
void CC2520_1_MACRO_RESELECT(void) 
{
    CC2520_1_MACRO_RELEASE(); 
    _NOP(); // added 5 tics delay
    _NOP();
    _NOP(); 
    _NOP(); 
    _NOP(); 
}
#endif
   void SPI1_Init(void);

    void SPI1_WriteBuff( const unsigned char * p_pSPI0Msg,  unsigned char p_ucMsgLen );
    void SPI1_WriteBuffReverse( const unsigned char * p_pSPI0Msg,  unsigned char p_ucMsgLen );
    void SPI1_ReadBuff( unsigned char* p_pSPI0Msg, unsigned char p_ucMsgLen );
    void SPI1_ReadBuffReverse( unsigned char* p_pSPI0Msg, unsigned char p_ucMsgLen );


#endif // SPI1

