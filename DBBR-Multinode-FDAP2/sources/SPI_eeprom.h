/*************************************************************************
* File: spi_eeprom.h
* Author: Nivis LLC, Ion Ticus
* SPI0 handling for communication with NX25P40 serial flash
*************************************************************************/

#ifndef SPI_EEPROM_H_INCLUDED
#define SPI_EEPROM_H_INCLUDED 

#include "typedef.h"
#include "digitals.h"

#define EEP_FW_CODE_ADDR            0x2000       //start address of the firmware code inside EEPROM 
#define EEP_SERIAL_NUMBER_ADDR      0x0000





void SPI_InitEEPROM (void);
void SPI_ReadEEPROM (uint8 *p_pucDest, uint32 p_ulEepAddr, uint16 p_unSize);
void SPI_FlushEEPROM (void); // must be SPI1_RequestEEPROMAccess() called before that function
void SPI_EraseEEPROMSector( uint8 p_ucSectorNo  );

// the sector must be erased before !!!
void SPI_WriteEEPROM (const uint8 *p_pucSrc, uint32 p_ulEepAddr, uint16 p_unSize);



#endif  // SPI_EEPROM_H_INCLUDED


