/*************************************************************************
* File: spi_pa_dac.h
* Author: Nivis LLC, Ion Ticus
* SPI handling for communication with PA DAC 
*************************************************************************/

#ifndef SPI_PA_DAC_H
#define SPI_PA_DAC_H

#include "typedef.h"
#define MAX_PA_LEVEL                0
#define NOMINAL_PA_LEVEL            (-1)
#define MIN_PA_LEVEL                (-25)
#define DEFAULT_PA_LEVEL            NOMINAL_PA_LEVEL
#define INTEGRAL_OMNI_ANTENNA   (-1)
#define REMOTE_OMNI_ANTENNA     (-3)
#define REMOTE_SECTOR_ANTENNA   (-10)
#define MAX_DBM_POWER_LEVEL         20
#define NOMINAL_DBM_POWER_LEVEL     16
#define MIN_DBM_POWER_LEVEL         (-5)
#define DEFAULT_DBM_POWER_LEVEL     NOMINAL_DBM_POWER_LEVEL
#define TX_POWER_LEVEL_ARRAY_SIZE  (MAX_DBM_POWER_LEVEL -  MIN_DBM_POWER_LEVEL) + 1
void SPI_InitDAC (void);
void SPI_PA_DAC_On(void);
void SPI_PA_DAC_Set (uint16 DAC_Value);

uint8 TxPowerLevel_GetDACValue(int8 dBm);
uint8 PA_Level_Validate(int8 Dbm);
void PA_Level_Set(int8 Dbm, int8 CurrentPowerLevel);
int8 PA_Level_Get(void);
int8 PA_Level_GetDefault(void);
#define SPI_InitDAC() SPI_PA_DAC_On()
#define TxPowerLevel_SetDACValue(DAC_Value) SPI_PA_DAC_Set(DAC_Value)

#endif  // SPI_PA_DAC_H


