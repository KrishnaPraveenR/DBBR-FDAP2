//=========================================================================================
// Name:        CC2420.c
// Author:      Rares Ivan
// Date:        February 2008
// Description: CC2420 Driver
// Changes:     
// Revisions:   
//=========================================================================================

#ifdef BBR1_HW
#include "../global.h"
#include "../digitals.h"
#include "../spi1.h"
#include "../itc.h"
#include "../ISA100/phy.h"
#include "../ISA100/provision.h"

#include "cc2420.h"
#include "cc2420_macros.h"
#include "cc2420_aes.h"

#include <string.h>

//unsigned int g_unCC2420SecCtrl_0 = 0x03C4; // 11 1100 0100 
  unsigned int g_unCC2420SecCtrl_0 = 0x01C4; // 01 1100 0100 
  unsigned int g_unCC2420SecCtrl_1 = 0x0000; 
#ifdef LOW_POWER_DEVICE
  unsigned char g_ucModemState = CC2420_MODEM_STATE_POWERDOWN;
#endif  

//==================================================================================================


//////////////////////////////////////////////////////////////////////
// Function: CC2420_Init
// Author: Rares Ivan
// Description: Initialize CC2420
//                - Turn ON the voltage regulator
//                - Reset CC2420
//                - Turn ON the crystal oscilator (crystal will stay ON forever)
//                - Write all necessary registers and settings
// Parameters: 
//
//////////////////////////////////////////////////////////////////////
void CC2420_Init()
{
    unsigned int temp;
    unsigned char statusRegister;

    AT91C_BASE_AIC->AIC_IDCR = (1L << AT91C_ID_PIOB); // disable RF interrupt
    
    SPI1_Init();   
            
    do{ 
      CC2420_MACRO_SELECT();
      SPI1_WriteByte( CC2420_SXOSCON );
      SPI1_ReadByte( statusRegister ); // get status
      CC2420_MACRO_RELEASE();
    } 
    while ( !(statusRegister & (1 << CC2420_XOSC16M_STABLE)) );
    
    
     CC2420_MACRO_SELECT();
     
     CC2420_MACRO_SOFT_RESET();
     
     SPI1_WriteByte( CC2420_SXOSCON );
     
    // Rares: For my tests, the CC2420 works with the default settings
    //        TBD the settings needed for ISA100
    
    // Configure MANAND (register 0x21) - Manual signal AND override register1
    //CC2420_MACRO_SETREG(CC2420_MANAND, 0xBFFF ); //BFFF - Bias PD 
    //CC2420_MACRO_GETREG(CC2420_MANAND, temp);    //BFFF - Bias PD

    // Configure Modem Control Register 0 (MDMCTRL0 0x11)
    temp = 0x0000
           | CC2420_MDMCTRL0_PREAMBLE_LENGTH_3     // Preamble Length [3:0]. See CC2420 page 64. The number of preamble bytes (2 zero-symbols) to be sent in TX mode prior to the SYNCWORD, encoded in steps of 2.
           //| CC2420_MDMCTRL0_AUTOACK             // If AUTOACK is set, all packets accepted by address recognition with the acknowledge request flag set and a valid CRC are acknowledged 12 symbol periods after being received.
           | CC2420_MDMCTRL0_AUTOCRC               // In packet mode a CRC-16 (ITU-T) is calculated and is transmitted after the last data byte in TX. In RX CRC is calculated and checked for validity.
           | CC2420_MDMCTRL0_CCA_MODE_3            // CCA Mode [7:6]. See CC2420 page 64 for CCA Modes
           | CC2420_MDMCTRL0_CCA_HYST_2dB          // CCA Hysteresis in dB, values 0 through 7 dB
           //| CC2420_MDMCTRL0_ADR_DECODE          // Hardware Address decode enable = 1, disabled = 0.
           //| CC2420_MDMCTRL0_PAN_COORDINATOR     // Should be set high when the device is a PAN Coordinator. Used for filtering packets with no destination address, as specified in section 7.5.6.2 in 802.15.4, D18
           //| CC2420_MDMCTRL0_RESERVED_FRAME_MODE // Mode for accepting reserved IEE 802.15.4 frame types when address recognition is enabled (MDMCTRL0.ADR_DECODE = 1)
           //| CC2420_MDMCTRL0_RESERVED            // Reserved, write as 0
           ;
    
    CC2420_MACRO_SETREG(CC2420_MDMCTRL0, temp);

    
    // Configure Modem Control Register 1 (MDMCTRL1 0x12)
    temp = 0x0000    
           | CC2420_MDMCTRL1_RX_MODE_NORMAL     // RX Test Mode
           | CC2420_MDMCTRL1_TX_MODE_NORMAL     // TX Test Mode
           //| CC2420_MDMCTRL1_MODULATION_MODE  // Set one of two RF modulation modes for RX / TX. 0 : IEEE 802.15.4 compliant mode, 1 : Reversed phase, non-IEEE compliant (could be used to set up a system which will not receive 802.15.4 packets)
           //| CC2420_MDMCTRL1_DEMOD_AVG_MODE   // Frequency offset average filter behaviour. 0 : Lock frequency offset filter after preamble match, 1 : Continuously update frequency offset filter.
           | CC2420_MDMCTRL1_CORR_THR_20        // Demodulator correlator threshold value, required before SFD search. Note that on early CC2420 versions the reset value was 0.
           //| CC2420_MDMCTRL1_RESERVED         // Reserved, write as 0
           ;
    CC2420_MACRO_SETREG(CC2420_MDMCTRL1, temp);       // 0x0500 Normal mode. Set the correlation threshold = 20
    //CC2420_MACRO_SETREG(CC2420_MDMCTRL1, 0x050C);   // CW Modulation mode.(For Power measurement)
    //CC2420_MACRO_SETREG(CC2420_MDMCTRL1, 0x0508);   // CW Unmodulation mode (For frequency accuracy, not for power measurement)

    temp = 0x0000
           | CC2420_TXCTRL_PA_LEVEL_MAX        // Output PA level (~0 dBm)
           | CC2420_TXCTRL_RESERVED            // Reserved, write as 1
           | CC2420_TXCTRL_PA_CURRENT_NOMINAL  // Current programming of the PA.
           | CC2420_TXCTRL_TXMIXCURRENT_1720uA // Transmit mixers current.
           | CC2420_TXCTRL_TXMIX_CAP_ARRAY_0   // Selects varactor array settings in the transmit mixers.
           | CC2420_TXCTRL_TX_TURNAROUND_192us // Sets the wait time after STXON before transmission is started.
           | CC2420_TXCTRL_TXMIXBUF_CUR_1160uA // TX mixer buffer bias current.
           ;
    CC2420_MACRO_SETREG(CC2420_TXCTRL,   temp); // 0xA0FF

    //CC2420_MACRO_SETREG(CC2420_RSSI,   0xE080); // 1110.0000,1000.0000 -> 0xE080 is default value at reset -> CCA_THR [15:8] = -32 = ~ -77dBm, RSSI_VAL [7:0] = -128, as per datasheet
    CC2420_MACRO_SETREG(CC2420_RXCTRL1,  0x2A56); // 0010.1010,0101.0110. All settings default, except RXBPF_LOCUR Bit 13 which is set to 1 as per datasheet recommendation.
    CC2420_MACRO_SETREG(CC2420_IOCFG0,   PHY_MIN_HDR_SIZE+2); // Set the FIFOP threshold to 10 bytes

    // Set the Security Control Register (0x19)
    //  -> AES disabled, RX and TX encryption keys selected as Key0.
    g_unCC2420SecCtrl_0 = 0x0000
           | CC2420_SECCTRL0_NO_SECURITY       // Security Mode: 0=Disabled, 1=CBC-MAC, 2=CTR, 3=CCM
           | CC2420_SECCTRL0_SEC_M_4_BYTES     // Nr of bytes in authentication field for CBC-MAC, encoded as (M-2)/2
           //| CC2420_SECCTRL0_SEC_RXKEYSEL    // RX Key select: 0=Key0, 1=Key1
           //| CC2420_SECCTRL0_SEC_TXKEYSEL    // TX Key select: 0=Key0, 1=Key1
           //| CC2420_SECCTRL0_SEC_SAKEYSEL    // Stand Alone Key select: 0=Key0, 1=Key1
           | CC2420_SECCTRL0_SEC_CBC_HEAD      // Defines what to use for the first byte in CBC-MAC (does not apply to CBC-MAC part of CCM)
           | CC2420_SECCTRL0_RXFIFO_PROTECTION // Protection enable of the RXFIFO
           ;
    
    CC2420_MACRO_SETREG(CC2420_SECCTRL0, g_unCC2420SecCtrl_0);
        
    //CC2420_MACRO_SETREG(CC2420_DACTST,   0x1800);  // DAC Test Register: CW unmodulation mode
    //CC2420_MACRO_SETREG(CC2420_DACTST,   0x0000);  // DAC Test Register -> Default: Normal mode & CW modulation mode
  
    //CC2420_MACRO_WAIT4OSC(statusRegister);

    //set the PA level
    CC2420_MACRO_SET_PA(g_ucPALevel);
    
    CC2420_MACRO_SET_RFCH_0TO15(0);
    
    // Select KEY1 as stand alone working key
    CC2420_MACRO_SELECT_KEY1( CC2420_SECCTRL0_SEC_SAKEYSEL );

    // Select KEY0 as current decryption key
    CC2420_MACRO_SELECT_KEY0( CC2420_SECCTRL0_SEC_RXKEYSEL | CC2420_SECCTRL0_SEC_TXKEYSEL );
    
    CC2420_MACRO_RELEASE();

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
// Function: CC2420_SetModemState
// Author: Rares Ivan
// Purpose: Set CC2420 Modem state
// Parameters:newModemState
// Returns:
//////////////////////////////////////////////////////////////////////
void CC2420_SetModemState(unsigned char newModemState)
{
  uint16 temp;  
  
#ifdef LOW_POWER_DEVICE
  if( newModemState >= CC2420_MODEM_STATE_IDLE ) // device on
  {
      if( g_ucModemState <= CC2420_MODEM_STATE_POWERUP )
      {
          BCM_SpeedUp();
          
          if( g_ucModemState == CC2420_MODEM_STATE_POWERDOWN ) // device was off
          {
              do{ 
                CC2420_MACRO_SELECT();
                CC2420_MACRO_STROBE(CC2420_SXOSCON); 
                CC2420_MACRO_GET_STATUS(temp); 
                CC2420_MACRO_RELEASE();
              } 
              while ( !(temp & BitMask(CC2420_XOSC16M_STABLE)) );
          }
      }
  }
  g_ucModemState = newModemState;
#endif
  
  CC2420_MACRO_SELECT();
  switch(newModemState)
  {
    case CC2420_MODEM_STATE_POWERDOWN: 
                CC2420_WaitUntilStatusBitClear( 1 << CC2420_TX_ACTIVE ); // Wait for TX to finish        
                CC2420_MACRO_STROBE(CC2420_SRFOFF);
                CC2420_MACRO_STROBE(CC2420_SXOSCOFF);
                CC2420_MACRO_OFF_ALL_IRQS();
                CC2420_MACRO_CLEAR_ALL_IRQS();
//                CC2420_VREG_OFF();
//                CC2420_VCTRLPA_OFF();
//                CC2420_SHDNLNA_OFF();
                break;
                
    case CC2420_MODEM_STATE_POWERUP:
                CC2420_Init();                
                CC2420_MACRO_SELECT();
                
                // no break here
    case CC2420_MODEM_STATE_IDLE:
                CC2420_WaitUntilStatusBitClear( 1 << CC2420_TX_ACTIVE ); // Wait for TX to finish        
                CC2420_MACRO_STROBE(CC2420_SRFOFF);
                CC2420_MACRO_OFF_ALL_IRQS();
                CC2420_MACRO_CLEAR_ALL_IRQS();
                CC2420_VCTRLPA_OFF();
                CC2420_SHDNLNA_OFF();
                break;
                
    case CC2420_MODEM_STATE_RX:
                CC2420_WaitUntilStatusBitClear( 1 << CC2420_TX_ACTIVE ); // Wait for TX to finish        
                CC2420_VCTRLPA_OFF();
                                
                CC2420_MACRO_GETREG(CC2420_RXFIFO, temp); // Dummy read for CC2420 to set FIFOP pin inactive

                CC2420_MACRO_RESELECT();  
                
                CC2420_MACRO_FLUSH_RXFIFO();                
                
                CC2420_SHDNLNA_ON();
                CC2420_MACRO_ON_RX_IRQS();                
                CC2420_MACRO_STROBE(CC2420_SRXON); 
                break;
/*                
	case CC2420_MODEM_STATE_TX_WITH_CCA:
                CC2420_SHDNLNA_OFF();
                CC2420_VCTRLPA_ON();
                CC2420_FIFOP_IRQOFF();
                CC2420_SFD_FALLING_IRQON();   // SFD Falling for end of packet
		CC2420_MACRO_STROBE(CC2420_STXONCCA);
		break;
*/                
        case CC2420_MODEM_STATE_TX_NO_CCA:
                CC2420_SHDNLNA_OFF();
                CC2420_VCTRLPA_ON();
                CC2420_MACRO_ON_TX_IRQS();
		CC2420_MACRO_STROBE(CC2420_STXON);
		break;
                
     default: 
                CC2420_WaitUntilStatusBitClear( 1 << CC2420_TX_ACTIVE ); // Wait for TX to finish        
                CC2420_MACRO_STROBE(CC2420_SRFOFF);
                CC2420_VCTRLPA_OFF();
                CC2420_SHDNLNA_OFF();
  }
  
  CC2420_MACRO_RELEASE();  
}

void CC2420_WaitUntilStatusBitClear( uint8 p_ucBitMask )
{
    uint8  ucStatus;
    uint32 unTmpCnt = 100000;
    do
    { 
      CC2420_MACRO_GET_STATUS(ucStatus); 
      if( !unTmpCnt-- )
      {
          Log(LOG_ERROR,LOG_M_SYS,SYSOP_GENERAL, 1, "1", 1, &p_ucBitMask, 1, &ucStatus );
          break;
      }      
    }    
    while( ucStatus & p_ucBitMask );
}

void CC2420_WaitUntilStatusBitSet( uint8 p_ucBitMask )
{
    uint8 ucStatus;
    uint32 unTmpCnt = 100000;
    do
    { 
      CC2420_MACRO_GET_STATUS(ucStatus); 
      if( !unTmpCnt-- )
      {
          Log(LOG_ERROR,LOG_M_SYS,SYSOP_GENERAL, 1, "2", 1, &p_ucBitMask, 1, &ucStatus );
          break;
      }      
    }    
    while( !(ucStatus & p_ucBitMask) );
}





#endif