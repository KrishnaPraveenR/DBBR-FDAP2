/***************************************************************************************************
* Name:         digitals.h
* Author:       Nivis LLC, Ion Ticus
* Date:         Aug 2008
* Description:  This header file is provided ...
*               This file holds definitions of the ...
* Changes:
* Revisions:
****************************************************************************************************/
#ifndef _NIVIS_DIGITALS_H_
#define _NIVIS_DIGITALS_H_

#include "typedef.h"
#include "global.h"
#include "at91sam7x512.h"
#include "system.h"

#define EEPROM_WR_PROTECT   AT91C_PIO_PA3
#define EEPROM_SPI_SELECT   AT91C_PA12_SPI0_NPCS0
#define PA_DAC_SPI_SELECT   AT91C_PA13_SPI0_NPCS1
#define THERM_SPI_SELECT    AT91C_PA14_SPI0_NPCS2

#define CC2420_SPI_SELECT   AT91C_PA21_SPI1_NPCS0
#define LED_1               AT91C_PIO_PA29
#define LED_2               AT91C_PIO_PA30
#ifdef BBR1_HW
#define LED_3               AT91C_PIO_PA19      
#endif
#ifdef BBR2_HW
#define LED_3               AT91C_PIO_PA2
#endif

#define PORTB_FLAGS 0x100000000

#define I2C_EEPROM_WP       (AT91C_PIO_PB30 | PORTB_FLAGS)


#ifdef BBR1_HW 
#define CC2420_SPI_SELECT   AT91C_PA21_SPI1_NPCS0
#define CC2420_SHDNLNA      (AT91C_PIO_PB19 | PORTB_FLAGS)
#define CC2420_VCTRLPA      (AT91C_PIO_PB20 | PORTB_FLAGS)
#define CC2420_CCA          (AT91C_PIO_PB21 | PORTB_FLAGS)
#define CC2420_FIFO         (AT91C_PIO_PB22 | PORTB_FLAGS)
#define CC2420_FIFOP        (AT91C_PIO_PB23 | PORTB_FLAGS)
#define CC2420_SFD          (AT91C_PIO_PB27 | PORTB_FLAGS)
#endif

#ifdef BBR2_HW
#define CC2520_1_SPI_SELECT   AT91C_PA21_SPI1_NPCS0
#define CC2520_2_SPI_SELECT   AT91C_PA13_SPI0_NPCS1
#define CC2520_1_SHDNLNA      (AT91C_PIO_PA4)
#define CC2520_1_VCTRLPA      (AT91C_PIO_PA8)
#define CC2520_2_SHDNLNA      (AT91C_PIO_PA19)
#define CC2520_2_VCTRLPA      (AT91C_PIO_PA25)
#define CC2520_1_POWERUP      (AT91C_PIO_PA7)   
#define CC2520_2_POWERUP      (AT91C_PIO_PA9)    
#define CC2520_1_CCA          (AT91C_PIO_PB21 | PORTB_FLAGS)
#define CC2520_1_FIFO         (AT91C_PIO_PB22 | PORTB_FLAGS)
#define CC2520_1_FIFOP        (AT91C_PIO_PB23 | PORTB_FLAGS)
#define CC2520_1_SFD          (AT91C_PIO_PB27 | PORTB_FLAGS)
#define CC2520_2_CCA          (AT91C_PIO_PB29 | PORTB_FLAGS)
#define CC2520_2_FIFO         (AT91C_PIO_PB19 | PORTB_FLAGS)
#define CC2520_2_FIFOP        (AT91C_PIO_PB20 | PORTB_FLAGS)
#define CC2520_2_SFD          (AT91C_PIO_PB30 | PORTB_FLAGS)
#define CC2520_1_HGM          (AT91C_PIO_PB24 | PORTB_FLAGS)   
#define CC2520_2_HGM          (AT91C_PIO_PB25 | PORTB_FLAGS)    
#define CC2520_RESET          (AT91C_PIO_PB12 | PORTB_FLAGS) 
#define FACTORY_DIAG_JUMPER   (AT91C_PIO_PA26)
#endif

/////////////////////////////////

#define SET_GPIOx_AS_OUTPUT( BASE, GPIO )  { BASE->PIO_PER = (uint32)(GPIO); BASE->PIO_OER = (uint32)(GPIO); }
#define SET_GPIOx_AS_INPUT( BASE, GPIO )   { BASE->PIO_ODR = (uint32)(GPIO); BASE->PIO_PER = (uint32)(GPIO); }

#define SET_GPIO_AS_OUTPUT( GPIO ) if((GPIO) & PORTB_FLAGS) SET_GPIOx_AS_OUTPUT(AT91C_BASE_PIOB,GPIO) else SET_GPIOx_AS_OUTPUT(AT91C_BASE_PIOA,GPIO) 
#define SET_GPIO_AS_INPUT( GPIO )  if((GPIO) & PORTB_FLAGS) SET_GPIOx_AS_INPUT(AT91C_BASE_PIOB,GPIO)  else SET_GPIOx_AS_INPUT(AT91C_BASE_PIOA,GPIO) 

#define SET_GPIO_HI( GPIO )  if((GPIO) & PORTB_FLAGS) AT91C_BASE_PIOB->PIO_SODR = (uint32)(GPIO); else AT91C_BASE_PIOA->PIO_SODR = (uint32)(GPIO) 
#define SET_GPIO_LO( GPIO )  if((GPIO) & PORTB_FLAGS) AT91C_BASE_PIOB->PIO_CODR = (uint32)(GPIO); else AT91C_BASE_PIOA->PIO_CODR = (uint32)(GPIO) 

#define GET_GPIO_STATUS( GPIO ) ((GPIO) & PORTB_FLAGS ? AT91C_BASE_PIOB->PIO_PDSR & (uint32)(GPIO) : AT91C_BASE_PIOA->PIO_PDSR & (uint32)(GPIO)) 

#ifdef BBR1_HW
// LED 1
#define LED1_ON()     SET_GPIO_LO( LED_1 )
#define LED1_OFF()    SET_GPIO_HI( LED_1 )

// LED 2
#define LED2_ON()     SET_GPIO_LO( LED_2 )
#define LED2_OFF()    SET_GPIO_HI( LED_2 )

// LED 3
#define LED3_ON()     SET_GPIO_LO( LED_3 )
#define LED3_OFF()    SET_GPIO_HI( LED_3 )

#define IS_LED1_ON()  GET_GPIO_STATUS( LED_1 )?0:1
#define IS_LED2_ON()  GET_GPIO_STATUS( LED_2 )?0:1
#define IS_LED3_ON()  GET_GPIO_STATUS( LED_3 )?0:1
#endif

#ifdef BBR2_HW   ///666
#define LED1_ON()     SET_GPIO_HI( LED_1 ); SET_GPIO_LO( LED_2 )
#define LED1_OFF()    SET_GPIO_LO( LED_1 ); SET_GPIO_LO( LED_2 )
#define LED2_ON()     SET_GPIO_HI( LED_2 ); SET_GPIO_LO( LED_1 )
#define LED2_OFF()    SET_GPIO_LO( LED_2 ); SET_GPIO_LO( LED_1 )
#define LED3_ON()     SET_GPIO_LO( LED_3 ); 
#define LED3_OFF()    SET_GPIO_HI( LED_3 ); 
#define IS_LED1_ON()  GET_GPIO_STATUS( LED_1 )
#define IS_LED2_ON()  GET_GPIO_STATUS( LED_2 )
#define IS_LED3_ON()  GET_GPIO_STATUS( LED_3 )?0:1
#endif
#define DEBUG0_ON()   //LED1_OFF()  
#define DEBUG0_OFF()  //LED1_ON()  

#define DEBUG1_ON()   LED3_OFF()  
#define DEBUG1_OFF()  LED3_ON()  

#define DEBUG2_ON()   //LED1_OFF()  
#define DEBUG2_OFF()  //LED1_ON()  

#define DEBUG3_ON()   LED1_OFF()  
#define DEBUG3_OFF()  LED1_ON()  

#define DEBUG4_ON()   LED2_OFF()  
#define DEBUG4_OFF()  LED2_ON()  



#define EEPROM_WRITE_ENABLE()  SET_GPIO_HI( EEPROM_WR_PROTECT )
#define EEPROM_WRITE_DISABLE() SET_GPIO_LO( EEPROM_WR_PROTECT )

#define I2C_EEPROM_WRITE_ENABLE()  SET_GPIO_LO( I2C_EEPROM_WP )
#define I2C_EEPROM_WRITE_DISABLE() SET_GPIO_HI( I2C_EEPROM_WP )

void Digitals_Init(void);

#define DbgError(n) 


#endif // _NIVIS_DIGITALS_H_ 

