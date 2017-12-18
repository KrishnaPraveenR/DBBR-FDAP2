/*************************************************************************
* File: spi_pa_dac.c
* Author: Nivis LLC, Dorin Muica
* SPI handling for communication with PA DAC 
*************************************************************************/

#include "spi_pa_dac.h"
#include "spi0.h"

#ifdef BBR1_HW
#include "CC2420/cc2420.h"
#include "CC2420/cc2420_macros.h"
#endif

#ifdef BBR2_HW
#include "CC2520/cc2520.h"
#include "CC2520/CC2520_macros.h"
#endif

#define SPI0_DAC_ZERO_VOLTS_CMD 0x0000
#define SPI0_DAC_2_5_VOLTS_CMD  0x0C10     // 2.4878 volts


// The iNode DAC is 8-bits wide and there are 3 different antennas

const uint8 DBM_TO_DAC_INTEGRAL_OMNI_ANTENNA[TX_POWER_LEVEL_ARRAY_SIZE] =
{
  // dBm       DAC Value
  /* -5 */          89,
  /* -4 */          90,
  /* -3 */          91,
  /* -2 */          92,
  /* -1 */          93,
  /*  0 */          94,
  /*  1 */          95,
  /*  2 */          96,
  /*  3 */          98,
  /*  4 */          99,
  /*  5 */         101,
  /*  6 */         104,
  /*  7 */         106,
  /*  8 */         109,
  /*  9 */         112,
  /* 10 */         116,
  /* 11 */         119,
  /* 12 */         123,
  /* 13 */         128,
  /* 14 */         133,
  /* 15 */         139,
  /* 16 */         146,
  /* 17 */         153,
  /* 18 */         162,
  /* 19 */         174,
  /* 20 */         193     
};

const uint8 DBM_TO_DAC_REMOTE_OMNI_ANTENNA[TX_POWER_LEVEL_ARRAY_SIZE] =
{
  // dBm       DAC Value
  /* -5 */          92,
  /* -4 */          93,
  /* -3 */          94,
  /* -2 */          95,
  /* -1 */          97,
  /*  0 */          98,
  /*  1 */          99,
  /*  2 */         101,
  /*  3 */         103,
  /*  4 */         105,
  /*  5 */         107,
  /*  6 */         109,
  /*  7 */         112,
  /*  8 */         115,
  /*  9 */         118,
  /* 10 */         121,
  /* 11 */         126,
  /* 12 */         131,
  /* 13 */         136,
  /* 14 */         143,
  /* 15 */         151,
  /* 16 */         163,
  /* 17 */         176,
  /* 18 */         190,
  /* 19 */         193,
  /* 20 */         193     
};

const uint8 DBM_TO_DAC_REMOTE_SECTOR_ANTENNA[TX_POWER_LEVEL_ARRAY_SIZE] =
{
  // dBm       DAC Value
  /* -5 */          98,
  /* -4 */          99,
  /* -3 */         100,
  /* -2 */         102,
  /* -1 */         103,
  /*  0 */         104,
  /*  1 */         106,
  /*  2 */         108,
  /*  3 */         110,
  /*  4 */         112,
  /*  5 */         115,
  /*  6 */         119,
  /*  7 */         122,
  /*  8 */         127,
  /*  9 */         134,
  /* 10 */         142,
  /* 11 */         155,
  /* 12 */         186,
  /* 13 */         193,
  /* 14 */         193,
  /* 15 */         193,
  /* 16 */         193,
  /* 17 */         193,
  /* 18 */         193,
  /* 19 */         193,
  /* 20 */         193     
};

#ifdef BBR2_HW
/* for cc2520-2591 combo chip
dBm    DAC
0xFF 	20
0x8D 	18
0xF9 	17
0xF0    16  
0xE0 	15
0xA0    14  
0x6C 	13
0x2C    11  
0x79 	10
0x19 	9
0x49 	5
0x69 	4
0x45 	3
0x29 	1
0x03    -1  
0x09 	-2
0x01    -8  
*/
const uint8 DBM_TO_DAC_CC2520_2591[TX_POWER_LEVEL_ARRAY_SIZE] =
{
  // dBm       DAC Value
  /* -5 */          3,
  /* -4 */          3,
  /* -3 */          3,
  /* -2 */          9,
  /* -1 */          9,
  /*  0 */          41,
  /*  1 */          41,
  /*  2 */          69,
  /*  3 */          73,
  /*  4 */          73,
  /*  5 */          73,
  /*  6 */          121,
  /*  7 */          121,
  /*  8 */          121,
  /*  9 */          44,
  /* 10 */          108,
  /* 11 */          108,
  /* 12 */          160,
  /* 13 */          160,
  /* 14 */          224,
  /* 15 */          240,
  /* 16 */          249,
  /* 17 */          249,
  /* 18 */          255,
  /* 19 */          255,
  /* 20 */          255     
};
#endif

#define TX_PA_LEVEL_ARRAY_SIZE  ( MAX_PA_LEVEL -  MIN_PA_LEVEL) + 1
#define INVALID_TXCTRL          0xFFFF

const uint16 DBM_TO_TXCTRL_REGISTER[TX_PA_LEVEL_ARRAY_SIZE] =
{
  //    See CC2420 datasheet table 9 for more information
  //
  //    Only 8 values are valid.  However, sizing the array to the maximum 
  //    provides straight forward source code.
  //
  //    Ultimately a value from this table gets stored into PA_Level_RegisterSetting.

  //    From MLMO_IN.c, the following values are used in the iNode.
  //    #define INTEGRAL_OMNI           (-1)
  //    #define REMOTE_OMNI             (-3)
  //    #define REMOTE_SECTOR           (-10)


  //
  //
  // dBm          Value for TXCTRL register
  //
  /* -25 */       INVALID_TXCTRL,
  /* -24 */       INVALID_TXCTRL,     
  /* -23 */       INVALID_TXCTRL,     
  /* -22 */       INVALID_TXCTRL,     
  /* -21 */       INVALID_TXCTRL,     
  /* -20 */       INVALID_TXCTRL,     
  /* -19 */       INVALID_TXCTRL,
  /* -18 */       INVALID_TXCTRL,
  /* -17 */       INVALID_TXCTRL,
  /* -16 */       INVALID_TXCTRL,
  /* -15 */       INVALID_TXCTRL,
  /* -14 */       INVALID_TXCTRL,
  /* -13 */       INVALID_TXCTRL,
  /* -12 */       INVALID_TXCTRL,
  /* -11 */       INVALID_TXCTRL,
  /* -10 */       0xEB,                   // REMOTE_SECTOR
  /*  -9 */       INVALID_TXCTRL,
  /*  -8 */       INVALID_TXCTRL,
  /*  -7 */       INVALID_TXCTRL,
  /*  -6 */       INVALID_TXCTRL,
  /*  -5 */       INVALID_TXCTRL,
  /*  -4 */       INVALID_TXCTRL,
  /*  -3 */       0xF7,                   // REMOTE_OMNI
  /*  -2 */       INVALID_TXCTRL,
  /*  -1 */       0xFB,                   // INTEGRAL_OMNI
  /*   0 */       0xFF
};


// 16 bit HW transfer on channel 1
#define __DAC_16_BITS_CMD( p_ulWord ) SPI0_WriteByte( (p_ulWord) | (1L << 16) | AT91C_SPI_LASTXFER )
  
int8    PA_Level_InDbm = INTEGRAL_OMNI_ANTENNA;
 

void SPI_PA_DAC_Set (uint16 DAC_Value)
{
#ifdef BBR1_HW
  DAC_Value = (DAC_Value << 4);
  __DAC_16_BITS_CMD( DAC_Value  );
#endif
  
#ifdef BBR2_HW
  if (g_ucRadioOperationSts & CC2520_RADIO_1)
  {
    CC2520_1_MACRO_SELECT();
    CC2520_1_MACRO_SET_PA(DAC_Value);
    CC2520_1_MACRO_RELEASE();
  }

#endif
}

void SPI_PA_DAC_On (void)
{
  __DAC_16_BITS_CMD( SPI0_DAC_2_5_VOLTS_CMD  );
}

void SPI_PA_DAC_Off (void)
{
  __DAC_16_BITS_CMD( SPI0_DAC_ZERO_VOLTS_CMD  );
}

uint8 TxPowerLevel_GetDACValue(int8 dBm)
{
#ifdef BBR1_HW
  int8 AntennaType;
    
  AntennaType = PA_Level_Get( );
  dBm = (dBm - MIN_DBM_POWER_LEVEL);
  if( (dBm >= 0)  && (dBm <= (MAX_DBM_POWER_LEVEL- MIN_DBM_POWER_LEVEL)) )
  {
    if( REMOTE_SECTOR_ANTENNA == AntennaType )
    {
      return( DBM_TO_DAC_REMOTE_SECTOR_ANTENNA[dBm] );
    }
    else if( REMOTE_OMNI_ANTENNA == AntennaType )
    {
      return( DBM_TO_DAC_REMOTE_OMNI_ANTENNA[dBm] );
    }
    else
    {// INTEGRAL_OMNI_ANTENNA
      return( DBM_TO_DAC_INTEGRAL_OMNI_ANTENNA[dBm] );
    }
  }
  else if( dBm < 0 )
  {
    if( REMOTE_SECTOR_ANTENNA == AntennaType )
    {
      return( DBM_TO_DAC_REMOTE_SECTOR_ANTENNA[MIN_DBM_POWER_LEVEL-MIN_DBM_POWER_LEVEL] );
    }
    else if( REMOTE_OMNI_ANTENNA == AntennaType )
    {
      return( DBM_TO_DAC_REMOTE_OMNI_ANTENNA[MIN_DBM_POWER_LEVEL-MIN_DBM_POWER_LEVEL] );
    }
    else
    {// INTEGRAL_OMNI_ANTENNA
      return( DBM_TO_DAC_INTEGRAL_OMNI_ANTENNA[MIN_DBM_POWER_LEVEL-MIN_DBM_POWER_LEVEL] );
    }
  }
  else
  {
    if( REMOTE_SECTOR_ANTENNA == AntennaType )
    {
      return( DBM_TO_DAC_REMOTE_SECTOR_ANTENNA[MAX_DBM_POWER_LEVEL-MIN_DBM_POWER_LEVEL] );
    }
    else if( REMOTE_OMNI_ANTENNA == AntennaType )
    {
      return( DBM_TO_DAC_REMOTE_OMNI_ANTENNA[MAX_DBM_POWER_LEVEL-MIN_DBM_POWER_LEVEL] );
    }
    else
    {// INTEGRAL_OMNI_ANTENNA
      return( DBM_TO_DAC_INTEGRAL_OMNI_ANTENNA[MAX_DBM_POWER_LEVEL-MIN_DBM_POWER_LEVEL] );
    }
  }
#endif
  
#ifdef BBR2_HW
  dBm = (dBm - MIN_DBM_POWER_LEVEL);
  if( (dBm >= 0)  && (dBm <= (MAX_DBM_POWER_LEVEL- MIN_DBM_POWER_LEVEL)) )
  {
      return( DBM_TO_DAC_CC2520_2591[dBm] );
  }
  else if( dBm < 0 )
  {
      return( DBM_TO_DAC_CC2520_2591[MAX_DBM_POWER_LEVEL-MIN_DBM_POWER_LEVEL] );
  }
  else
  {
      return( DBM_TO_DAC_CC2520_2591[MAX_DBM_POWER_LEVEL-MAX_DBM_POWER_LEVEL] );
  }
#endif  
}

//##############################################################################
uint8 PA_Level_Validate(int8 Dbm)
//-
//+ PublicDescription:
//   Validate the PA_Level
//-
//+ PrivateDescription:
//-
//+ Arguments:
//-
//+ ReturnValues: uint8 - set to one when valid (zero otherwise).
//-
//+ Restrictions:
//
//-
//##############################################################################
{
  if( (Dbm >= MIN_PA_LEVEL)  && (Dbm <= MAX_PA_LEVEL) )
  {
      if( DBM_TO_TXCTRL_REGISTER[Dbm - MIN_PA_LEVEL] != INVALID_TXCTRL )
      {
          return( 1 );                                      
      }
  }                                                     
  return( 0 );                                          
}                                                         
                                                          
//##############################################################################
void PA_Level_Set(int8 Dbm, int8 CurrentPowerLevel)                             
//-                                                       
//+ PublicDescription:                                    
//   Sets the PA Level                                    
//-                                                       
//+ PrivateDescription:                                   
//-                                                       
//+ Arguments:                                            
//-                                                       
//+ ReturnValues: None.                                   
//-                                                       
//+ Restrictions:                                          
//                                                        
//-                                                       
//##############################################################################
{       
#ifdef BBR1_HW
  if( PA_Level_Validate( Dbm ) )
  {
    // Index lookup allows for negative DBM values.
    CC2420_MACRO_SET_PA(DBM_TO_TXCTRL_REGISTER[Dbm - MIN_PA_LEVEL]);
    PA_Level_InDbm = Dbm;

    // Reset Radio power level because PA level is set for different
    // antenna types and different types required different power settings to arrive
    // at the correct output power (dBm).
    TxPowerLevel_SetDACValue(TxPowerLevel_GetDACValue(CurrentPowerLevel));
  }
#endif
}

//##############################################################################
int8 PA_Level_Get(void)
//-
//+ PublicDescription:
//   Gets the PA Level  (in Dbm)
//-
//+ PrivateDescription:
//-
//+ Arguments:
//-
//+ ReturnValues: int8 - Current power level (in Dbm).
//-
//+ Restrictions:
//
//-
//##############################################################################
{
  return( PA_Level_InDbm );
}

//##############################################################################
int8 PA_Level_GetDefault(void)
//-
//+ PublicDescription:
//   Gets the default PA Level (in Dbm)
//-
//+ PrivateDescription:
//-
//+ Arguments:
//-
//+ ReturnValues: int8 - Default power level (in Dbm).
//-
//+ Restrictions:
//
//-
//##############################################################################
{
  return( DEFAULT_PA_LEVEL );
}

