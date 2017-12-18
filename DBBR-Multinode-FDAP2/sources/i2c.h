#ifndef _NIVIS_I2C_H_
#define _NIVIS_I2C_H_

#include "global.h"
    
// EEPROM Page Size (hardware specific)
#define EEPROM_PAGE_SIZE    16
#define EEPROM_MEM_SIZE     512

//-------------------------------------------
// Variables and Functions declarations
void I2C_Init(void);

uint16 i2c_WriteEEPROM(uint16 p_nAddress, const uint8 * p_pchSrc, uint16 p_nSize);
uint16 i2c_ReadEEPROM (uint16 p_nAddress, uint8 * p_pchDst, uint16 p_nSize);
    
#define ReadEEPROM          i2c_ReadEEPROM
#define WriteEEPROM         i2c_WriteEEPROM
    
#endif // _NIVIS_I2C_H_
