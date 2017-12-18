/*************************************************************************
* File: ut_main_spi_eeprom.c
* Author: Dorin Pavel, Nivis LLC
* SPI1 handling for communication with Honeywell EEPROM, unit test
*************************************************************************/

#ifdef UT_ACTIVED  // unit testing support must be off


#include <string.h>
#include <stdio.h>

#include "bcm.h"
#include "digitals.h"
#include "global.h"
#include "rtc.h"
#include "spi1_eeprom.h"
#include "wdt.h"


///////////////////////////////////////////////////////////////////////////////////
// Name: MAIN Function
///////////////////////////////////////////////////////////////////////////////////

uint8 i=0,
      j=0,
      g_ucArray1[100],
      g_ucArray2[100];


int main()
{ 
  //__disable_interrupt();
  //WDT_Init();
  BCM_INIT();
  INIT_Digitals();
  RTC_INIT();
  SPI1_eeprom_init();

  __enable_interrupt();
  
  memset (g_ucArray1, 0, sizeof(g_ucArray1)); 
  memset (g_ucArray2, 0, sizeof(g_ucArray2));

  //--------------------------------------------------------------------
  // Main Loop
  //--------------------------------------------------------------------
  for (;;)
  {

      //------------------------------------------
      // 10ms Tasks
      //------------------------------------------
      if( g_uc10msFlag )
      {
          g_uc10msFlag=0;
        
          /******** EEPROM SPI tests **********/        
          
          for(i=0; i<sizeof(g_ucArray1); i++)
          {  
            if(j%2)
              g_ucArray1[i]=i;
            else
              g_ucArray1[i]=sizeof(g_ucArray1)-i;
          }
          
          for(i=0; i<sizeof(g_ucArray1); i++)
            g_ucArray2[i]=g_ucArray1[i];
          
          //write
          SPI_eeprom_write(0x0000, g_ucArray1, sizeof(g_ucArray1));
          
          
          for(i=0; i<sizeof(g_ucArray1); i++)
            g_ucArray1[i]=0;
          
          // read
          SPI_eeprom_read(g_ucArray1, 0x0000, sizeof(g_ucArray1));
          
          if ( memcmp( g_ucArray1, g_ucArray2, sizeof(g_ucArray1) ) )
            while(1);  
          
          
          if (0xff > j) j++;
          else j=0;
        
      }
        
      WDT_Refresh();
  }
}

#endif // #ifndef UT_ACTIVED -> unit testing support must be off
