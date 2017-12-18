/***************************************************************************************************
* Name:         digitals.c
* Author:       Marius Vilvoi
* Date:         October 2007
* Description:  This file is provided ...
*               This file holds definitions of the ...
* Changes:
* Revisions:
****************************************************************************************************/
#include "digitals.h"
#ifdef BBR1_HW
#include "cc2420\cc2420.h"
#endif
#ifdef BBR2_HW
#include "cc2520\cc2520.h"
#endif

void Digitals_Init(void)
{  
  #ifdef  BBR1_HW
  // can set  more pins because all are on PIOA 
  SET_GPIO_HI( LED_1 | LED_2 | LED_3 | CC2420_SPI_SELECT | EEPROM_SPI_SELECT | PA_DAC_SPI_SELECT | THERM_SPI_SELECT | EEPROM_WR_PROTECT );
  SET_GPIO_AS_OUTPUT( LED_1 | LED_2 | LED_3 | CC2420_SPI_SELECT | EEPROM_SPI_SELECT | PA_DAC_SPI_SELECT | THERM_SPI_SELECT | EEPROM_WR_PROTECT);
  

  CC2420_VCTRLPA_OFF();
  CC2420_SHDNLNA_OFF();

  I2C_EEPROM_WRITE_DISABLE();
  
  // can set  more pins because all are on PIOB 
  SET_GPIO_AS_INPUT( CC2420_CCA | CC2420_FIFO | CC2420_FIFOP | CC2420_SFD);
  SET_GPIO_AS_OUTPUT( CC2420_SHDNLNA | CC2420_VCTRLPA | I2C_EEPROM_WP);
  #endif  
  #ifdef  BBR2_HW
  SET_GPIO_HI( LED_1 | LED_2 | LED_3 | EEPROM_SPI_SELECT | THERM_SPI_SELECT | EEPROM_WR_PROTECT );
  SET_GPIO_AS_OUTPUT( LED_1 | LED_2 | LED_3 | EEPROM_SPI_SELECT | THERM_SPI_SELECT | EEPROM_WR_PROTECT);
  CC2520_1_VCTRLPA_OFF();
  CC2520_1_SHDNLNA_OFF();
  CC2520_2_VCTRLPA_OFF();
  CC2520_2_SHDNLNA_OFF();
  SET_GPIO_AS_INPUT( CC2520_1_CCA | CC2520_1_FIFO | CC2520_1_FIFOP | CC2520_1_SFD | CC2520_2_CCA | CC2520_2_FIFO | CC2520_2_FIFOP | CC2520_2_SFD);
  SET_GPIO_AS_OUTPUT( CC2520_1_SHDNLNA | CC2520_1_VCTRLPA | CC2520_2_SHDNLNA | CC2520_2_VCTRLPA);
  SET_GPIO_AS_INPUT(FACTORY_DIAG_JUMPER);
  #endif
}
