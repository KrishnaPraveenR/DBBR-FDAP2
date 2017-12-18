/***************************************************************************************************
* Name:         spi0.h
* Author:       Nivis LLC,
* Date:         Aug, 2008
* Description:  This header file is provided ...
*               This file holds definitions of the ...
* Changes:
* Revisions:
****************************************************************************************************/
#ifndef _NIVIS_SPI0_H_
#define _NIVIS_SPI0_H_

   #include "digitals.h"

//  #define SPI0_WaitEndTx(void)       while ( !( AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_ENDTX) )
  #define SPI0_WaitEndTx(void)       while ( !( AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_TXEMPTY) )
  #define SPI0_CkIfCanTx(void)       while ( !(  AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_TDRE) )

// attention !!, next macros are not {} protected so be carefull when use it
  #define SPI0_WriteByte(p_ucByte)   SPI0_CkIfCanTx(); AT91C_BASE_SPI0->SPI_TDR = p_ucByte
  #define SPI0_ReadByte(p_ucDummy, p_ucSpiRead)   \
          SPI0_WriteByte(p_ucDummy); \
          SPI0_WaitEndTx(); \
          while(!(AT91C_BASE_SPI0->SPI_SR & AT91C_SPI_RDRF));\
          p_ucSpiRead = (unsigned char)AT91C_BASE_SPI0->SPI_RDR

#ifdef BBR2_HW

   enum eSPI0_CurrentAccessType 
   {
       SPI0_NONE_RESERVED        = 0,
       SPI0_RADIO2_RESERVED      = 1,
       SPI0_FLASH_RESERVED       = 2,
       SPI0_THERMOMETER_RESERVED = 3,
       SPI0_UNDEFINED_RESERVED   = 4
   };
   extern enum eSPI0_CurrentAccessType eSPI0_CurrentAccess;
   #define SPI0_Radio2_WriteByte( p_ulByte ) SPI0_WriteByte( (p_ulByte) | (1L << 16) )

   #define SPI0_Radio2_ReadByte(p_ucSpiRead) SPI0_Radio2_WriteByte(CC2520_SNOP | (1L << 16)); \
                                          SPI0_WaitEndTx(); \
                                          p_ucSpiRead = (unsigned char)AT91C_BASE_SPI0->SPI_RDR
   #define CC2520_2_MACRO_SELECT()   ; // Not Required now
   #define CC2520_2_MACRO_RELEASE() _NOP(); _NOP(); _NOP(); _NOP(); _NOP(); \
                                    AT91C_BASE_SPI0->SPI_CR = AT91C_SPI_LASTXFER;
#pragma inline 
void CC2520_2_MACRO_RESELECT(void) 
{
    CC2520_2_MACRO_RELEASE(); 
    _NOP(); // added 5 tics delay
    _NOP();
    _NOP(); 
    _NOP(); 
    _NOP(); 
}
#endif
//  #define EEPROM_SELECT()  SET_GPIO_LO( EEPROM_SPI_SELECT )
//  #define PA_DAC_SELECT()  SET_GPIO_LO( PA_DAC_SPI_SELECT )
//  #define THERM_SELECT()   SET_GPIO_LO( THERM_SPI_SELECT )
  
//  #define SPI0_SLAVES_RELEASE()   SET_GPIO_HI( EEPROM_SPI_SELECT | PA_DAC_SPI_SELECT | THERM_SPI_SELECT )
//  #define EEPROM_RELEASE()  SPI0_SLAVES_RELEASE()
//  #define PA_DAC_RELEASE()  SPI0_SLAVES_RELEASE()
//  #define THERM_RELEASE()   SPI0_SLAVES_RELEASE()


  void SPI0_Init(void);
  #ifdef BBR2_HW
  void SPI0_Radio2_WriteBuff( const unsigned char * p_pSPI0Msg,  unsigned char p_ucMsgLen );
  void SPI0_Radio2_WriteBuffReverse( const unsigned char * p_pSPI0Msg,  unsigned char p_ucMsgLen );
  void SPI0_Radio2_ReadBuff( unsigned char* p_pSPI0Msg, unsigned char p_ucMsgLen );
  void SPI0_Radio2_ReadBuffReverse( unsigned char* p_pSPI0Msg, unsigned char p_ucMsgLen );  
  #endif


#endif // SPI0

