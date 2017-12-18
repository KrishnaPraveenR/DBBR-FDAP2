/*************************************************************************
* File: spi_eeprom.c
* Author: Nivis LLC, Ion Ticus
* SPI0 handling for communication with NX25P40 serial flash
*************************************************************************/

#include <string.h>

#include "spi_eeprom.h"
#include "spi0.h"
#include "timers.h"

#ifndef SPI_PAGE_SIZE
#  define SPI_PAGE_SIZE 256
#endif

// NX25P40 instruction set 
#define WRSR  0x01 // Write Status Register    
#define WRITE 0x02 // Write Data to Memory Array 
#define READ  0x03 // Read Data from Memory Array 
#define WRDI  0x04 // Reset Write Enable Latch  
#define RDSR  0x05 // Read Status Register  
#define WREN  0x06 // Set Write Enable Latch  
#define ERASE 0xD8 // Sector erase

#define __low_WriteByte( p_ulByte ) SPI0_WriteByte( (p_ulByte) | (0L << 16) )

void __low_WriteLastByte( uint8 p_ucByte );

static  void SPI_writeEEPROMPage(uint32 p_ulEepAddr, const uint8* p_pucSrc, uint16 p_unSize);
///////////////////////////////////////////////////////////////////////////////////
// Name: SPI_InitEEPROM
// Description: SPI Initialisation for communication with the EEPROM
// Parameters: none 
// Retunn: none
///////////////////////////////////////////////////////////////////////////////////
void SPI_InitEEPROM(void)
{     
//    EEPROM_WP_OFF();  // WP=1 
        
    SPI_FlushEEPROM();
        
    __low_WriteLastByte( WREN );
    
    __low_WriteByte( WRSR );  // Write Status Register
    __low_WriteLastByte( 0x00 ); // no protection
}

///////////////////////////////////////////////////////////////////////////////////
// Name: SPI_ReadEEPROM
// Description: Read an array of bytes from EEPROM
// Parameters: p_pucDest = ptr. to destination array 
//             p_unEepAddr = EEPROM start address 
//             p_unSize = number of bytes to be read, must be >=1 !!! 
// Return: none
///////////////////////////////////////////////////////////////////////////////////
void SPI_ReadEEPROM (uint8 *p_pucDest, uint32 p_ulEepAddr, uint16 p_unSize)
{  
  SPI_FlushEEPROM();
  
  if( p_unSize )
  {      
        __low_WriteByte( READ);   // read command
        __low_WriteByte( (uint8)(p_ulEepAddr >> 16) );   // 24 bit address
        __low_WriteByte( (uint8)(p_ulEepAddr >> 8) ); // hi byte address        
        __low_WriteByte( (uint8)p_ulEepAddr); // lo byte address
                
        while( --p_unSize )
        {    
            __low_WriteByte(0x00); // dummy write in order to start read
            SPI0_WaitEndTx();            
            *(p_pucDest++) = (unsigned char)AT91C_BASE_SPI0->SPI_RDR;            
        }         
        __low_WriteLastByte( 0x00 ); // dummy write in order to start read
        *p_pucDest = (unsigned char)AT91C_BASE_SPI0->SPI_RDR;            
  }
}


///////////////////////////////////////////////////////////////////////////////////
// Name: SPI_WriteEEPROM
// Description: Write an array of bytes to EEPROM
// Parameters: p_unEepAddr = ptr. to destination (EEPROM address)
//             p_pucSrc = ptr. to array to be written 
//             p_unSize = number of bytes to be written 
// Return: none
//
// ATTENTION: the sector must be erased before !!!
///////////////////////////////////////////////////////////////////////////////////
void SPI_WriteEEPROM (const uint8 *p_pucSrc, uint32 p_ulEepAddr, uint16 p_unSize)
{
  while ( p_unSize )
  {
    uint16 unWriteLen = SPI_PAGE_SIZE - ( p_ulEepAddr % SPI_PAGE_SIZE );        

    if(unWriteLen > p_unSize)
      unWriteLen = p_unSize;

    SPI_writeEEPROMPage( p_ulEepAddr, p_pucSrc, unWriteLen );
    
    p_ulEepAddr += unWriteLen;
    p_pucSrc += unWriteLen;
    
    p_unSize -= unWriteLen;    
  } 
}

///////////////////////////////////////////////////////////////////////////////////
// Name: SPI_EraseEEPROMSector
// Description: Erase a sector (64 kB)
// Parameters: p_ulSectorAddress = EEPROM address
// Return: none
///////////////////////////////////////////////////////////////////////////////////
void SPI_EraseEEPROMSector( uint8 p_ucSectorNo  )
{
     SPI_FlushEEPROM();
     
    __low_WriteLastByte( WREN ); //Write enable
    __low_WriteByte( ERASE ); //Sector erase
    __low_WriteByte( p_ucSectorNo );   // 24 bit address
    __low_WriteByte( 0);   // hi byte address  
    __low_WriteLastByte( 0 );        // low byte address
}


///////////////////////////////////////////////////////////////////////////////////
// Name: SPI_writeEEPROMPage
// Description: Write an EEPROM page (max. 128 bytes)
// Parameters: p_unEepAddr = EEPROM address
//             p_pucSrc = ptr. to array to be written 
//             p_unSize = number of bytes to be written 
// Return: none
///////////////////////////////////////////////////////////////////////////////////
static void SPI_writeEEPROMPage(uint32 p_ulEepAddr, const uint8* p_pucSrc, uint16 p_unSize)
{
    SPI_FlushEEPROM();
    
    if( p_unSize )
    {                
        // write page
        __low_WriteLastByte( WREN ); //Write enable

        __low_WriteByte( WRITE );              // write data command      
        __low_WriteByte( (uint8)(p_ulEepAddr >> 16) );   // 24 bit address
        __low_WriteByte( (uint8)(p_ulEepAddr >> 8) );   // hi byte address  
        __low_WriteByte( (uint8)(p_ulEepAddr) );        // low byte address

        
        while( --p_unSize )
        {
              __low_WriteByte( *(p_pucSrc++) ); 
        }
        __low_WriteLastByte( *p_pucSrc ); 
    }    
}

///////////////////////////////////////////////////////////////////////////////////
// Name: SPI_FlushEEPROM
// Description: Check EEPROM status register until it is not busy (write is done)
// Parameters: none  
// Return: 0
///////////////////////////////////////////////////////////////////////////////////
void SPI_FlushEEPROM (void)
{
  do
  {
      __low_WriteByte( RDSR ); // read eeprom status
      __low_WriteLastByte( 0x00 );       
  }  while( (unsigned char)AT91C_BASE_SPI0->SPI_RDR & 0x01 ); // continue if still busy   
} 
 
void __low_WriteLastByte( uint8 p_ucByte )
{
      volatile uint16 i = MCK_DIV_FOR_10MHZ/2+1;
      
    __low_WriteByte( p_ucByte | AT91C_SPI_LASTXFER );
      SPI0_WaitEndTx();
      while(--i) // at least 100 ns delay
        ;  
}    
