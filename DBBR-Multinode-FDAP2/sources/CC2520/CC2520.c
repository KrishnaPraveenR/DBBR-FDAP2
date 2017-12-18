//=========================================================================================
// Name:        CC2520.c
// Author:      Gourav Sharma
// Date:        February 2008
// Description: CC2520 Driver
// Changes:
// Revisions:
//=========================================================================================


#include "../global.h"
#include "../digitals.h"
#include "../spi1.h"
#include "../itc.h"
#include "../ISA100/phy.h"
#include "../ISA100/provision.h"
#include "../spi0.h"
#include "../timers.h"
#include "../wdt.h"
#include "../ISA100/tmr_util.h"

#include "CC2520.h"
#include "CC2520_macros.h"
#include "../spi_pa_dac.h"

#include <string.h>

extern uint16 g_unTxSFDFraction;          // save the time of transmitted packet's SFD (from slot start) in fraction counts

#ifdef BBR2_HW


unsigned int g_unCC2520SecCtrl_0 = 0x01C4; // 01 1100 0100
unsigned int g_unCC2520SecCtrl_1 = 0x01C4;

uint8 g_ucAlarms_Indicator = 0;   

uint8 eActiveTransmitter; 
uint8 g_ucRadioOperationSts; 
uint8 g_ucOldRadioOperationSts, g_ucOldActiveTransmitter;

#ifdef LOW_POWER_DEVICE
  unsigned char g_ucModemState = CC2520_MODEM_STATE_POWERDOWN;
#endif
  
void CC2520_WaitUntilStatusBitClear( uint8 p_ucBitMask, uint8 Radio1or2 );
void CC2520_WaitUntilStatusBitSet( uint8 p_ucBitMask, uint8 Radio1or2 );

//==================================================================================================

//////////////////////////////////////////////////////////////////////
// Function: Radio1_Init
// Author: Gourav Sharma
// Description: Initialize CC2520 Radio1 on SPI1
//                - Turn ON the voltage regulator
//                - Reset CC2520
//                - Turn ON the crystal oscilator (crystal will stay ON forever)
//                - Write all necessary registers and settings
// Parameters:
//
//////////////////////////////////////////////////////////////////////  
void Radio1_Init(uint8 ucHardReset)
{
    volatile unsigned char ucstatusRegister; 
    volatile uint8 uctmpCntr = 0;  
    uint16 unCounter;   
    
    // set High Gain Mode
    SET_GPIO_HI(CC2520_1_HGM);

    SPI1_Init();

    if (ucHardReset)
    {
    do{
      CC2520_1_MACRO_SELECT();
      SPI1_WriteByte( CC2520_SXOSCON );
      SPI1_ReadByte( ucstatusRegister ); // get status
      CC2520_1_MACRO_RELEASE();
      uctmpCntr++;      
      for (unCounter=0; unCounter<2000; unCounter++);  //3msec timeout  (2000x100)    
    }while(   ( uctmpCntr < 100 && ( ucstatusRegister == 0xFF ) )                        \
            ||(!(ucstatusRegister & CC2520_XOSC16M_STABLE) )                             \
          );
    }
    
    for(volatile uint8 tempp = 0; tempp<100;tempp++); // Let the chip come in to a stable state     
        
    if( (uctmpCntr >= 100 ) && ( ucstatusRegister == 0xFF ) )        //Radio removed or not working gives 0xFF when StatusReg is read            
    {
       g_ucAlarms_Indicator |= RADIO_1_FAIL;
    }
    else 
    {      
      g_ucAlarms_Indicator &= ~(RADIO_1_FAIL);
    }    

    CC2520_1_MACRO_SELECT();    
     SPI1_WriteByte( CC2520_SRES ); // Reset the device using the soft inst.
    CC2520_1_MACRO_RELEASE();
    
    uctmpCntr = 0;  //3msec counter initialization

    do{
      CC2520_1_MACRO_SELECT();
      SPI1_WriteByte( CC2520_SXOSCON );
      SPI1_ReadByte(ucstatusRegister ); // get status
      CC2520_1_MACRO_RELEASE();
      uctmpCntr++;          
      for (unCounter=0; unCounter<2000; unCounter++);  //3msec timeout  (2000x100)    
    }while(   ( uctmpCntr < 100 && ( ucstatusRegister == 0xFF ) )                        \
            ||(!(ucstatusRegister & CC2520_XOSC16M_STABLE) )                             \
          );           
        
    for(volatile uint8 tempp = 0; tempp<100;tempp++); // Let the chip come in to a stable state     
    
    if( (uctmpCntr >= 100 ) && ( ucstatusRegister == 0xFF ) )        //Radio removed or not working gives 0xFF when St      
    {
       g_ucAlarms_Indicator |= RADIO_1_FAIL;
       return;
    }
    else 
    {
      g_ucAlarms_Indicator &= ~(RADIO_1_FAIL);
    }            

    CC2520_1_MACRO_SELECT();

    // Set the TXPower control register, PA Power Level to 0 Dbm
    CC2520_1_MACRO_SETMEM(CC2520_TXPOWER, CC2520_TXPOWER_PA_POWER_0);

    // Set CCA Control register , as per the reccomended value in the datasheet
    CC2520_1_MACRO_SETMEM(CC2520_CCACTRL0, 0x04);   //-72dBm at CC2520 Radio 

     // Configure Modem Control Register 0 (MDMCTRL0 0x046)
    CC2520_1_MACRO_SETMEM(CC2520_MDMCTRL0, 0x85);

    // Configure Modem Control Register 1 (MDMCTRL1 0x47)
    CC2520_1_MACRO_SETMEM(CC2520_MDMCTRL1, CC2520_MDMCTRL1_CORR_THR_20);       // Set the correlation threshold = 20

    // Set Rx Control register , as per the reccomended value in the datasheet
    CC2520_1_MACRO_SETMEM(CC2520_RXCTRL, 0x3F);

    // Set FS Control register , as per the reccomended value in the datasheet
    CC2520_1_MACRO_SETMEM(CC2520_FSCTRL, 0x5A);

    // Set FS Cal register , as per the reccomended value in the datasheet
    CC2520_1_MACRO_SETMEM(CC2520_FSCAL1, 0x2B);

    // Set AGC Control loop register , as per the reccomended value in the datasheet
    CC2520_1_MACRO_SETMEM(CC2520_AGCCTRL1, 0x16);

    // Set ADC Performance register , as per the reccomended value in the datasheet
    CC2520_1_MACRO_SETMEM(CC2520_ADCTEST0, 0x10);

    // Set ADC Performance register , as per the reccomended value in the datasheet
    CC2520_1_MACRO_SETMEM(CC2520_ADCTEST1, 0x0E);

    // Set ADC Performance register , as per the reccomended value in the datasheet
    CC2520_1_MACRO_SETMEM(CC2520_ADCTEST2, 0x03);

    // Disable Frame Filtering
    CC2520_1_MACRO_SETMEM(CC2520_FRMFILT0, 0x00);

    //  Set CCA mode and hysteresis value in CCA control Register
//    CC2520_1_MACRO_SETMEM(CC2520_CCACTRL1, temp);

    // Set Frame handling to disable Auto CRC Check on start up
    CC2520_1_MACRO_SETMEM (CC2520_FRMCTRL0, 0x00);

    // Set FIFOP Threshold value
    CC2520_1_MACRO_SETMEM (CC2520_FIFOPCTRL, PHY_MIN_HDR_SIZE + 2); // Set the FIFOP threshold

    // Set Tx Control register, ???? Check if it is required right now
    CC2520_1_MACRO_SETMEM(CC2520_TXCTRL, 0xC1);

    CC2520_1_MACRO_SETMEM(CC2520_EXTCLOCK, 0x00);
    
    CC2520_1_MACRO_RELEASE();  
           
}

//////////////////////////////////////////////////////////////////////
// Function: Radio2_Init
// Author: Gourav Sharma
// Description: Initialize CC2520 Radio2 on SPI0
//                - Turn ON the voltage regulator
//                - Reset CC2520
//                - Turn ON the crystal oscilator (crystal will stay ON forever)
//                - Write all necessary registers and settings
// Parameters:
//
//////////////////////////////////////////////////////////////////////  
void Radio2_Init(uint8 ucHardReset)
{
   /* volatile unsigned char ucstatusRegister;
    volatile uint8 uctmpCntr = 0; 
    uint16 unCounter=0;
    
    // set High Gain Mode
    SET_GPIO_HI(CC2520_2_HGM);

    SPI0_Init();

    if (ucHardReset)
    {
    do{
      CC2520_2_MACRO_SELECT();
      SPI0_Radio2_WriteByte( CC2520_SXOSCON );
      SPI0_Radio2_ReadByte( ucstatusRegister ); // get status
      CC2520_2_MACRO_RELEASE();
      uctmpCntr++;      
      for (unCounter=0; unCounter<2000; unCounter++);  //Datasheet says LPM2 to Active mode is 0.3msec - we set max timout of 3msec (2000x100)    
    }while(   ( uctmpCntr < 100 && ( ucstatusRegister == 0xFF ) )                        \
            ||(!(ucstatusRegister & CC2520_XOSC16M_STABLE) )                             \
          );  
    }
    
    if( (uctmpCntr >= 100 ) && ( ucstatusRegister == 0xFF ) )       //Radio removed or not working gives 0xFF when StatusReg is read
    {
       g_ucAlarms_Indicator |= RADIO_2_FAIL;
    }
    else 
    {
      g_ucAlarms_Indicator &= ~(RADIO_2_FAIL);
    }    

     CC2520_2_MACRO_SELECT();
     SPI0_Radio2_WriteByte( CC2520_SRES ); // Reset the device using the soft inst.
     CC2520_2_MACRO_RELEASE();

     uctmpCntr = 0;   //3msec counter initialization

    do{
      CC2520_2_MACRO_SELECT();
      SPI0_Radio2_WriteByte( CC2520_SXOSCON );
      SPI0_Radio2_ReadByte(ucstatusRegister ); // get status
      CC2520_2_MACRO_RELEASE();
      uctmpCntr++;          
      for (unCounter=0; unCounter<2000; unCounter++);  //Datasheet says LPM2 to Active mode is 0.3msec - we set max timout of 3msec (2000x100)    
    }while(   ( uctmpCntr < 100 && ( ucstatusRegister == 0xFF ) )                        \
            ||(!(ucstatusRegister & CC2520_XOSC16M_STABLE) )                             \
          );              
        
    for(volatile uint8 tempp = 0; tempp<100;tempp++); // Let the chip come in to a stable state     
    
    if( (uctmpCntr >= 100 ) &&( ucstatusRegister == 0xFF ) )        //Radio removed or not working gives 0xFF when St            
    {
       g_ucAlarms_Indicator |= RADIO_2_FAIL;
    }
    else 
    {
      g_ucAlarms_Indicator &= ~(RADIO_2_FAIL);
    }

    // Set the TXPower control register, PA Power Level to 0 Dbm
    CC2520_2_MACRO_SETMEM(CC2520_TXPOWER, CC2520_TXPOWER_PA_POWER_0);

    // Set CCA Control register , as per the reccomended value in the datasheet
    CC2520_2_MACRO_SETMEM(CC2520_CCACTRL0, 0x04);  //-72dBm at CC2520 Radio 

     // Configure Modem Control Register 0 (MDMCTRL0 0x046)
    CC2520_2_MACRO_SETMEM(CC2520_MDMCTRL0, 0x85);

    // Configure Modem Control Register 1 (MDMCTRL1 0x47)
    CC2520_2_MACRO_SETMEM(CC2520_MDMCTRL1, CC2520_MDMCTRL1_CORR_THR_20);       // Set the correlation threshold = 20

    // Set Rx Control register , as per the reccomended value in the datasheet
    CC2520_2_MACRO_SETMEM(CC2520_RXCTRL, 0x3F);

    // Set FS Control register , as per the reccomended value in the datasheet
    CC2520_2_MACRO_SETMEM(CC2520_FSCTRL, 0x5A);

    // Set FS Cal register , as per the reccomended value in the datasheet
    CC2520_2_MACRO_SETMEM(CC2520_FSCAL1, 0x2B);

    // Set AGC Control loop register , as per the reccomended value in the datasheet
    CC2520_2_MACRO_SETMEM(CC2520_AGCCTRL1, 0x16);

    // Set ADC Performance register , as per the reccomended value in the datasheet
    CC2520_2_MACRO_SETMEM(CC2520_ADCTEST0, 0x10);

    // Set ADC Performance register , as per the reccomended value in the datasheet
    CC2520_2_MACRO_SETMEM(CC2520_ADCTEST1, 0x0E);

    // Set ADC Performance register , as per the reccomended value in the datasheet
    CC2520_2_MACRO_SETMEM(CC2520_ADCTEST2, 0x03);

    // Disable Frame Filtering
    CC2520_2_MACRO_SETMEM(CC2520_FRMFILT0, 0x00);

    //  Set CCA mode and hysteresis value in CCA control Register
//    CC2520_2_MACRO_SETMEM(CC2520_CCACTRL1, temp);

    // Set Frame handling to disable Auto CRC Check on start up
    CC2520_2_MACRO_SETMEM (CC2520_FRMCTRL0, 0x00);

    // Set FIFOP Threshold value
    CC2520_2_MACRO_SETMEM (CC2520_FIFOPCTRL, PHY_MIN_HDR_SIZE + 2); // Set the FIFOP threshold

    // Set Tx Control register, ???? Check if it is required right now
    CC2520_2_MACRO_SETMEM(CC2520_TXCTRL, 0xC1);

    CC2520_2_MACRO_SETMEM(CC2520_EXTCLOCK, 0x00);
    
    CC2520_2_MACRO_RELEASE();
   */ 
}
//////////////////////////////////////////////////////////////////////
// Function: CC2520_Init
// Author: Gourav Sharma
// Description: Initialize CC2520
//                - Turn ON the voltage regulator
//                - Reset CC2520
//                - Turn ON the crystal oscilator (crystal will stay ON forever)
//                - Write all necessary registers and settings
// Parameters: Select the radio to be initialized Radio1 or 2 or both
// Note: Radio 1 is taken to be the primary radio for communication
//////////////////////////////////////////////////////////////////////
void CC2520_Init(uint8 Radio1or2, uint8 p_ucGlowLED)
{
  volatile uint32 i = MCK/1000; //1msec 
  volatile uint32 j, k;
  
  eActiveTransmitter = CC2520_NO_RADIO; 
  g_ucRadioOperationSts = CC2520_NO_RADIO; 

  AT91C_BASE_AIC->AIC_IDCR = (1L << AT91C_ID_PIOB); // disable RF interrupt
 
  SET_GPIO_AS_OUTPUT(CC2520_RESET | CC2520_1_HGM);     
  SET_GPIO_AS_OUTPUT(CC2520_1_POWERUP);   
  SET_GPIO_AS_OUTPUT(CC2520_2_HGM);     
  SET_GPIO_AS_OUTPUT(CC2520_2_POWERUP);
 
  // power up both the RF circuits, no harm even though they are not present
  SET_GPIO_HI(CC2520_1_POWERUP);
  SET_GPIO_HI(CC2520_2_POWERUP);

  // Reset radio chip, hold the reset line for 1 msec for proper radio reset
  if (p_ucGlowLED) // reset only on rejoin/start
  {
  SET_GPIO_LO(CC2520_RESET);
  while(--i); 
  SET_GPIO_HI(CC2520_RESET);
  }
  
  if (CC2520_RADIO_1 == Radio1or2 ||
      CC2520_BOTH_RADIOS == Radio1or2)
  {
    // for user visual indication
    LED2_OFF();
    LED3_OFF();
    for( j = 0; (j < 10) && p_ucGlowLED; j++ )
    {
      LED1_OFF();
      for (k=0; k<200000; k++);
  
      LED1_ON();
      for (k=0; k<200000; k++);
      FEED_WDT();
    }
    
    // Initialise Radio 1
    MONITOR_ENTER();
    Radio1_Init(p_ucGlowLED);
    LED1_OFF();
    MONITOR_EXIT();    
    
    if(!(g_ucAlarms_Indicator & RADIO_1_FAIL))
    {
    // Set Frame handling to enable Auto CRC Check
    CC2520_1_MACRO_SELECT();
    CC2520_1_MACRO_SETMEM (CC2520_FRMCTRL0, CC2520_FRMCTRL0_AUTOCRC);
    CC2520_1_MACRO_RELEASE();

      eActiveTransmitter = CC2520_RADIO_1;      
      g_ucRadioOperationSts = CC2520_RADIO_1;      
    
    // Set the Security Control Register
    //  -> AES disabled, RX and TX encryption keys selected as Key0.
    g_unCC2520SecCtrl_0 = 0x0000
         | CC2520_SECCTRL0_NO_SECURITY       // Security Mode: 0=Disabled, 1=CBC-MAC, 2=CTR, 3=CCM
         | CC2520_SECCTRL0_SEC_M_4_BYTES     // Nr of bytes in authentication field for CBC-MAC, encoded as (M-2)/2
         //| CC2520_SECCTRL0_SEC_RXKEYSEL    // RX Key select: 0=Key0, 1=Key1
         //| CC2520_SECCTRL0_SEC_TXKEYSEL    // TX Key select: 0=Key0, 1=Key1
         | CC2520_SECCTRL0_SEC_SAKEYSEL      // Stand Alone Key select: 0=Key0, 1=Key1
         | CC2520_SECCTRL0_SEC_CBC_HEAD      // Defines what to use for the first byte in CBC-MAC (does not apply to CBC-MAC part of CCM)
         ;    
  
    }
  }
  
  /*if ((CC2520_RADIO_2 == Radio1or2 ||
      CC2520_BOTH_RADIOS == Radio1or2) &&
      SPI0_FLASH_RESERVED != eSPI0_CurrentAccess)
  {    
    // for user visual indication
    LED1_OFF();
    LED3_OFF();
    for( j = 0; (j < 10) && p_ucGlowLED; j++ )
    {
      LED2_OFF();
      for (k=0; k<200000; k++);
  
      LED2_ON();
      for (k=0; k<200000; k++);
      FEED_WDT();
    }
    
    // Initialise Radio 2 only if SPI0 is not used by SPI flash dirver
    MONITOR_ENTER();
    Radio2_Init(p_ucGlowLED);
    LED2_OFF();
    MONITOR_EXIT();    
    
    if( !(g_ucAlarms_Indicator & RADIO_2_FAIL) )
    {      
    g_ucRadioOperationSts |= CC2520_RADIO_2;
    }
    
    if (CC2520_RADIO_2 == Radio1or2 || (g_ucAlarms_Indicator & RADIO_1_FAIL))  //if Radio 1 has failed or Radio 2 is the configured Radio
    {
      // Set Frame handling to enable Auto CRC Check
      CC2520_2_MACRO_SELECT();
      CC2520_2_MACRO_SETMEM (CC2520_FRMCTRL0, CC2520_FRMCTRL0_AUTOCRC);
      CC2520_2_MACRO_RELEASE();
      
      eActiveTransmitter = CC2520_RADIO_2;      //only 1 Radio can be the active transmitter at a time
    }
    
    // Set the Security Control Register
    //  -> AES disabled, RX and TX encryption keys selected as Key0.
    g_unCC2520SecCtrl_1 = 0x0000
         | CC2520_SECCTRL0_NO_SECURITY       // Security Mode: 0=Disabled, 1=CBC-MAC, 2=CTR, 3=CCM
         | CC2520_SECCTRL0_SEC_M_4_BYTES     // Nr of bytes in authentication field for CBC-MAC, encoded as (M-2)/2
         //| CC2520_SECCTRL0_SEC_RXKEYSEL    // RX Key select: 0=Key0, 1=Key1
         //| CC2520_SECCTRL0_SEC_TXKEYSEL    // TX Key select: 0=Key0, 1=Key1
         | CC2520_SECCTRL0_SEC_SAKEYSEL      // Stand Alone Key select: 0=Key0, 1=Key1
         | CC2520_SECCTRL0_SEC_CBC_HEAD      // Defines what to use for the first byte in CBC-MAC (does not apply to CBC-MAC part of CCM)
         ;    
  }*/
  
  // Set the Transmit power to default 20dBm
  SPI_PA_DAC_Set(TxPowerLevel_GetDACValue(DEFAULT_DBM_POWER_LEVEL));
  
  // fifo  on positive edge
  // fifop on positive edge
  // sfd on negative edge

  // enable RF pins interrupts
  AT91C_BASE_PMC->PMC_PCER = (1L << AT91C_ID_PIOB);

  AT91C_BASE_AIC->AIC_ICCR = (1L << AT91C_ID_PIOB);
  AT91C_BASE_AIC->AIC_IECR = (1L << AT91C_ID_PIOB); // enable RF interrupt
}

//==================================================================================================


//////////////////////////////////////////////////////////////////////
// Function: CC2520_SetModemState
// Author: Rares Ivan
// Purpose: Set CC2520 Modem state. It is called by both Radio 1 and Radio2 individually,
//          except the PowerUp option, which initializes both the radios.
// Parameters: newModemState, Select Radio 1 or Radio 2 or both
// Returns:
//////////////////////////////////////////////////////////////////////
void CC2520_SetModemState(unsigned char newModemState)
{
  uint16 temp;
  
#ifdef LOW_POWER_DEVICE
  if( newModemState >= CC2520_MODEM_STATE_IDLE ) // device on
  {
      if( g_ucModemState <= CC2520_MODEM_STATE_POWERUP )
      {
          BCM_SpeedUp();

          if( g_ucModemState == CC2520_MODEM_STATE_POWERDOWN ) // device was off
          {
            if (CC2520_RADIO_1 & g_ucRadioOperationSts)
            {
              do{
                CC2520_1_MACRO_SELECT();
                CC2520_1_MACRO_STROBE(CC2520_SXOSCON);
                CC2520_1_MACRO_GET_STATUS(temp);
                CC2520_1_MACRO_RELEASE();
              }
              while ( !(temp & (CC2520_XOSC16M_STABLE)) );
              
              temp = 0;
            }
              
          }
      }
  }
  g_ucModemState = newModemState;
#endif
  
  if (g_ucRadioOperationSts & CC2520_RADIO_1)
  {
    CC2520_1_MACRO_SELECT();
  }
  
  if (g_ucRadioOperationSts & CC2520_RADIO_2 && SPI0_FLASH_RESERVED != eSPI0_CurrentAccess)
  {
    CC2520_2_MACRO_SELECT();
  }
  
  switch(newModemState)
  {
    case CC2520_MODEM_STATE_POWERDOWN:
   
          if (g_ucRadioOperationSts & CC2520_RADIO_1)
          {
            CC2520_WaitUntilStatusBitClear(CC2520_TX_ACTIVE, CC2520_RADIO_1); // Wait for TX to finish            
            CC2520_1_MACRO_STROBE(CC2520_SRFOFF);
            CC2520_1_MACRO_STROBE(CC2520_SXOSCOFF);
          }

          CC2520_1_MACRO_OFF_ALL_IRQS();
          CC2520_1_MACRO_CLEAR_ALL_IRQS();            
          CC2520_2_MACRO_OFF_ALL_IRQS();
          CC2520_2_MACRO_CLEAR_ALL_IRQS();               
          break;

    case CC2520_MODEM_STATE_POWERUP:
          // Initialise the Radio Here
          PHY_INIT(0);
          // no break here
    case CC2520_MODEM_STATE_IDLE:
      
          if (g_ucRadioOperationSts & CC2520_RADIO_1)
          {
            CC2520_WaitUntilStatusBitClear(CC2520_TX_ACTIVE, CC2520_RADIO_1); // Wait for TX to finish            
            CC2520_1_MACRO_SELECT();
            CC2520_1_MACRO_STROBE(CC2520_SRFOFF);
          }
          
          CC2520_1_MACRO_OFF_ALL_IRQS();
          CC2520_1_MACRO_CLEAR_ALL_IRQS();
          CC2520_2_MACRO_OFF_ALL_IRQS();
          CC2520_2_MACRO_CLEAR_ALL_IRQS();
          CC2520_1_VCTRLPA_OFF();
          CC2520_1_SHDNLNA_OFF();
          CC2520_2_VCTRLPA_OFF();
          CC2520_2_SHDNLNA_OFF();
          break;

    case CC2520_MODEM_STATE_RX:

          CC2520_1_VCTRLPA_OFF();
          CC2520_2_VCTRLPA_OFF();

          if (g_ucRadioOperationSts & CC2520_RADIO_1)
          {
            CC2520_WaitUntilStatusBitClear(CC2520_TX_ACTIVE, CC2520_RADIO_1); // Wait for TX to finish            
            CC2520_1_MACRO_GETMEM(CC2520_RXBUF, temp);     // Dummy read for CC2520 to set FIFOP pin inactive
            CC2520_1_MACRO_FLUSH_RXFIFO();
            CC2520_1_SHDNLNA_ON();
            CC2520_1_MACRO_ON_RX_IRQS();
            CC2520_1_MACRO_STROBE(CC2520_SRXON);
          }
          
          break;
    case CC2520_MODEM_STATE_TX_NO_CCA:
          
          CC2520_1_SHDNLNA_OFF();
          CC2520_2_SHDNLNA_OFF();
          CC2520_1_MACRO_OFF_ALL_IRQS();
          CC2520_1_MACRO_CLEAR_ALL_IRQS();
          CC2520_2_MACRO_OFF_ALL_IRQS();
          CC2520_2_MACRO_CLEAR_ALL_IRQS();
          {
            CC2520_2_VCTRLPA_OFF();
            CC2520_1_VCTRLPA_ON();
            CC2520_1_MACRO_STROBE(CC2520_STXON);
            CC2520_1_MACRO_ON_TX_IRQS();
          }
          //Timestamp = Current time + 6 bytes wait + 5 bytes preamble (total 11 bytes of delay)
          g_unTxSFDFraction = TMR0_TO_FRACTION(TMR_GetSlotOffset()+11);
          break;

    default:
      
          if (g_ucRadioOperationSts & CC2520_RADIO_1)
          {
            CC2520_WaitUntilStatusBitClear(CC2520_TX_ACTIVE, CC2520_RADIO_1); // Wait for TX to finish            
            CC2520_1_MACRO_STROBE(CC2520_SRFOFF);
          }
          
          if (g_ucRadioOperationSts & CC2520_RADIO_2 && SPI0_FLASH_RESERVED != eSPI0_CurrentAccess)
          {
            CC2520_WaitUntilStatusBitClear(CC2520_TX_ACTIVE, CC2520_RADIO_2); // Wait for TX to finish            
            CC2520_2_MACRO_STROBE(CC2520_SRFOFF);
          }
          
          CC2520_1_VCTRLPA_OFF();
          CC2520_1_SHDNLNA_OFF();
          CC2520_2_VCTRLPA_OFF();
          CC2520_2_SHDNLNA_OFF();
  } // end of switch statement
  
  if (g_ucRadioOperationSts & CC2520_RADIO_1)
  {
    CC2520_1_MACRO_RELEASE();
  }
  
  if (g_ucRadioOperationSts & CC2520_RADIO_2 && SPI0_FLASH_RESERVED != eSPI0_CurrentAccess)
  {
    CC2520_2_MACRO_RELEASE();
  }
}

void CC2520_WaitUntilStatusBitClear( uint8 p_ucBitMask, uint8 Radio1or2 )
{
    uint8  ucStatus;
    uint16 unTmpCnt = 200; // do not wait for more than 500usec (do-while loop takes 2.2usec)
    do
    {
      if (Radio1or2 == CC2520_RADIO_1)
      {
        CC2520_1_MACRO_GET_STATUS(ucStatus);
      }

      unTmpCnt--;
      if( !unTmpCnt )
      {
          Log(LOG_ERROR,LOG_M_SYS,SYSOP_GENERAL, 1, "1", 1, &p_ucBitMask, 1, &ucStatus );
          break;
      }
    }
    while( ucStatus & p_ucBitMask );
}

void CC2520_WaitUntilStatusBitSet( uint8 p_ucBitMask, uint8 Radio1or2  )
{
    uint8 ucStatus;
    uint32 unTmpCnt = 100000;
    do
    {
      if (Radio1or2 == CC2520_RADIO_1)
      {
        CC2520_1_MACRO_GET_STATUS(ucStatus);
      }
      
      if( !unTmpCnt-- )
      {
          Log(LOG_ERROR,LOG_M_SYS,SYSOP_GENERAL, 1, "2", 1, &p_ucBitMask, 1, &ucStatus );
          break;
      }
    }
    while(!(ucStatus & p_ucBitMask));
}

#endif


