//!#############################################################################
//
//!           HONEYWELL CONFIDENTIAL & PROPRIETARY
//
//  This work contains Honeywell Inc. confidential and proprietary
//  information.  Disclosure, use or reproduction outside of Honeywell
//  is prohibited except as authorized in writing.
//
//  This unpublished work is protected by the laws of the United States
//  and other countries.  If publication occurs, the following notice
//  shall apply:
//
//!    COPYRIGHT (C) 2009 HONEYWELL INC.  ALL RIGHTS RESERVED.
//
//!#############################################################################
//##############################################################################

#ifndef CC2520_H
#define CC2520_H                   // Only include this header file once

#include "../digitals.h"

#ifdef BBR2_HW // Include this file only if HW is for BBR2 Board

#define CC2520_NO_RADIO               (0)
#define CC2520_RADIO_1                (1)
#define CC2520_RADIO_2                (2)
#define CC2520_BOTH_RADIOS            (3)

/***************************************************************************************************
 *      CC2520 STROBE, CONTROL AND STATUS REGSITERS                                             *
 ***************************************************************************************************
*/ 

// FREG definitions (BSET/BCLR supported)
#define CC2520_FRMFILT0                0x00
#define CC2520_FRMFILT1                0x01
#define CC2520_SRCMATCH                0x02
#define CC2520_SRCSHORTEN0             0x04
#define CC2520_SRCSHORTEN1             0x05
#define CC2520_SRCSHORTEN2             0x06
#define CC2520_SRCEXTEN0               0x08
#define CC2520_SRCEXTEN1               0x09
#define CC2520_SRCEXTEN2               0x0A
#define CC2520_FRMCTRL0                0x0C
#define CC2520_FRMCTRL1                0x0D
#define CC2520_RXENABLE0               0x0E
#define CC2520_RXENABLE1               0x0F
#define CC2520_EXCFLAG0                0x10
#define CC2520_EXCFLAG1                0x11
#define CC2520_EXCFLAG2                0x12
#define CC2520_EXCMASKA0               0x14
#define CC2520_EXCMASKA1               0x15
#define CC2520_EXCMASKA2               0x16
#define CC2520_EXCMASKB0               0x18
#define CC2520_EXCMASKB1               0x19
#define CC2520_EXCMASKB2               0x1A
#define CC2520_EXCBINDX0               0x1C
#define CC2520_EXCBINDX1               0x1D
#define CC2520_EXCBINDY0               0x1E
#define CC2520_EXCBINDY1               0x1F
#define CC2520_GPIOCTRL0               0x20
#define CC2520_GPIOCTRL1               0x21
#define CC2520_GPIOCTRL2               0x22
#define CC2520_GPIOCTRL3               0x23
#define CC2520_GPIOCTRL4               0x24
#define CC2520_GPIOCTRL5               0x25
#define CC2520_GPIOPOLARITY            0x26
#define CC2520_GPIOCTRL                0x28
#define CC2520_DPUCON                  0x2A
#define CC2520_DPUSTAT                 0x2C
#define CC2520_FREQCTRL                0x2E
#define CC2520_FREQTUNE                0x2F
#define CC2520_TXPOWER                 0x30
#define CC2520_TXCTRL                  0x31
#define CC2520_FSMSTAT0                0x32
#define CC2520_FSMSTAT1                0x33
#define CC2520_FIFOPCTRL               0x34
#define CC2520_FSMCTRL                 0x35
#define CC2520_CCACTRL0                0x36
#define CC2520_CCACTRL1                0x37
#define CC2520_RSSI                    0x38
#define CC2520_RSSISTAT                0x39
#define CC2520_TXFIFO_BUF              0x3A
#define CC2520_RXFIRST                 0x3C
#define CC2520_RXFIFOCNT               0x3E
#define CC2520_TXFIFOCNT               0x3F

// SREG definitions (BSET/BCLR unsupported)
#define CC2520_CHIPID                  0x40
#define CC2520_VERSION                 0x42
#define CC2520_EXTCLOCK                0x44
#define CC2520_MDMCTRL0                0x46
#define CC2520_MDMCTRL1                0x47
#define CC2520_FREQEST                 0x48
#define CC2520_RXCTRL                  0x4A
#define CC2520_FSCTRL                  0x4C
#define CC2520_FSCAL0                  0x4E
#define CC2520_FSCAL1                  0x4F
#define CC2520_FSCAL2                  0x50
#define CC2520_FSCAL3                  0x51
#define CC2520_AGCCTRL0                0x52
#define CC2520_AGCCTRL1                0x53
#define CC2520_AGCCTRL2                0x54
#define CC2520_AGCCTRL3                0x55
#define CC2520_ADCTEST0                0x56
#define CC2520_ADCTEST1                0x57
#define CC2520_ADCTEST2                0x58
#define CC2520_MDMTEST0                0x5A
#define CC2520_MDMTEST1                0x5B
#define CC2520_DACTEST0                0x5C
#define CC2520_DACTEST1                0x5D
#define CC2520_ATEST                   0x5E
#define CC2520_DACTEST2                0x5F
#define CC2520_PTEST0                  0x60
#define CC2520_PTEST1                  0x61
#define CC2520_RESERVED                0x62
#define CC2520_DPUBIST                 0x7A
#define CC2520_ACTBIST                 0x7C
#define CC2520_RAMBIST                 0x7E


// RAM size definitions
#define CC2520_RAM_START_ADDR          0x100
#define CC2520_RAM_SIZE                640

// RX and TX buffer definitions
#define CC2520_RAM_TXBUF       (0x0100)
#define CC2520_RAM_RXBUF       (0x0180)

// Commands
#define CC2520_SNOP             0x00  // No Operation 
#define CC2520_SRES             0x0F
#define CC2520_REGRD            0x80  // Register Read, Or this with the Reg Address
#define CC2520_REGWR            0xC0  // Register Write, Or this with the Reg Address

#define CC2520_MEMRD            0x10  // Memory Read
#define CC2520_MEMWR            0x20  // Memory Write
#define CC2520_RXBUF            0x30  // Single Byte access to the RX buffer.
#define CC2520_TXBUF            0x3A  // Single Byte access to the TX buffer.

#define CC2520_SXOSCON          0x40  // Turn on the crystal oscillator (set XOSC16M_PD = 0 and BIAS_PD = 0)
#define CC2520_STXCAL           0x41  // Enable and calibrate frequency synthesizer for TX
#define CC2520_SRXON            0x42  // Enable RX
#define CC2520_STXON            0x43  // Enable TX after calibration
#define CC2520_STXONCCA         0x44  // Enable Transmission on CCA
#define CC2520_SRFOFF           0x45  // Disable RX/TX and frequency synthesizer
#define CC2520_SXOSCOFF         0x46  // Turn off the crystal oscillator and RF 
#define CC2520_SFLUSHRX         0x47  // Flush the RX FIFO buffer and reset the demodulator
#define CC2520_SFLUSHTX         0x48  // Flush the TX FIFO buffer
#define CC2520_SACK             0x49  // Send acknowledge frame, with pending field cleared
#define CC2520_SACKPEND         0x4A  // Send acknowledge frame, with pending field set
#define CC2520_CCM_HIGH_P       0x69  // Command CCM with high priority
#define CC2520_UCCM_HIGH_P      0x6B  // Command UCCM with high priority
#define CC2520_ECBO_HIGH_P      0x73  // Command ECBO with high priority
/***************************************************************************************************
 *              CC2520 RAM MEMORY SPACE                                                *
 ***************************************************************************************************
*/ 

// Sizes
#define CC2520_FIFO_SIZE    128

/**************************************************************************************************
 Security Register Configuration
 Note: There are no specific registers defined in CC2520 memory map. In turn General memory is 
       provided that can be used to store security fucntion specific data (i.e. Keys, NONCE etc.)      
***************************************************************************************************
*/
#define CC2520_SA_BUFF      0x200   // MAX 128 Bytes
#define CC2520_RAM_KEY0     0x280   // 16 Bytes, This key is used for Tx and Rx buffers
#define CC2520_RAM_KEY1     0x290   // 16 Bytes, This key is used for Stand Alone Buffer
#define CC2520_RX_NONCE     0x2A0   // 16 Bytes
#define CC2520_TX_NONCE     0x2B0   // 16 Bytes
#define CC2520_RX_DECRYP_BUFF     0x2C0   // 128 Bytes

/***************************************************************************************************
 *  Bit definitions: The Register Bit names are represented by their bit number (0-15)             *
 ***************************************************************************************************
*/

// CC2520 status byte
#define CC2520_XOSC16M_STABLE           0x80   // Indicates whether the 16 MHz oscillator is running or not
#define CC2520_TX_ACTIVE                0x02   // Indicates whether RF transmission is active
#define CC2520_RSSI_VALID		0x40   // Indicates the RSSI validity status
#define CC2520_RX_ACTIVE		0x01   // Indicates whethre RF reception is active

#define CC2520_EXCEP_TX_UNDERFLOW       0x08   // Tx Underflow bit flag

// CC2520 FSMSTAT0/1 register bits
#define CC2520_FSMSTAT_FSM_STATE_BV     0x003F
#define CC2520_FSMSTAT_CAL_RUNNING_BV   0x0040
#define CC2520_FSMSTAT_CAL_DONE_BV      0x0080
#define CC2520_FSMSTAT_RX_ACTIVE_BV     0x0100
#define CC2520_FSMSTAT_TX_ACTIVE_BV     0x0200
#define CC2520_FSMSTAT_LOCK_STATUS_BV   0x0400
#define CC2520_FSMSTAT_SAMPLED_CCA_BV   0x0800
#define CC2520_FSMSTAT_CCA_BV           0x1000
#define CC2520_FSMSTAT_SFD_BV           0x2000
#define CC2520_FSMSTAT_FIFOP_BV         0x4000
#define CC2520_FSMSTAT_FIFO_BV          0x8000

// DPUSTAT register bits
#define CC2520_AUTHSTAT_H     0x08
#define CC2520_AUTHSTAT_L     0x04
#define CC2520_DPUH_ACTIVE    0x02
#define CC2520_DPUL_ACTIVE    0x01

// MDMCTRL0 (0x11) - Modem Control Register 0
// MDMCTRL0.Bits
#define CC2520_MDMCTRL0_TX_FILTER           BIT0  // 0 - Normal TX filtering, 1 - Enable extra filtering
#define CC2520_MDMCTRL0_PREAMBLE_LENGTH0    BIT1  // See CC2520 page 64. The number of preamble bytes (2 zero-symbols) to be sent in TX mode prior to the SYNCWORD, encoded in steps of 2.
#define CC2520_MDMCTRL0_PREAMBLE_LENGTH1    BIT2  // -- // --
#define CC2520_MDMCTRL0_PREAMBLE_LENGTH2    BIT3  // -- // --
#define CC2520_MDMCTRL0_PREAMBLE_LENGTH3    BIT4  // -- // --

// MDMCTRL0.Bits.Mask
#define CC2520_MDMCTRL0_PREAMBLE_LENGTH_1   BIN8(00000000)  // 2 leading zero bytes (not recommended)
#define CC2520_MDMCTRL0_PREAMBLE_LENGTH_2   BIN8(00000010)  // 3 leading zero bytes (not recommended)
#define CC2520_MDMCTRL0_PREAMBLE_LENGTH_3   BIN8(00000100)  // 4 leading zero bytes (IEEE 802.15.4)
// ...
#define CC2520_MDMCTRL0_PREAMBLE_LENGTH_16  BIN8(00011110)  // 17 leading zero bytes
#define CC2520_MDMCTRL0_DEM_NUM_ZEROS_1     BIN8(01000000)
#define CC2520_MDMCTRL0_DEM_NUM_ZEROS_2     BIN8(10000000)
#define CC2520_MDMCTRL0_DEM_NUM_ZEROS_3     BIN8(11000000)

// FRMCTRL0 (0x11) - Frame Control Register 0
#define CC2520_FRMCTRL0_AUTOCRC             0x40  // In packet mode a CRC-16 (ITU-T) is calculated and is transmitted after the last data byte in TX. In RX CRC is calculated and checked for validity.

// CCACTRL1 Register
#define CC2520_CCACTRL1_CCA_MODE_0          BIN8(00000000)  // Reserved
#define CC2520_CCACTRL1_CCA_MODE_1          BIN8(01000000)  // CCA=1 when RSSI_VAL < CCA_THR - CCA_HYST, CCA=0 when RSSI_VAL >= CCA_THR
#define CC2520_CCACTRL1_CCA_MODE_2          BIN8(00001000)  // CCA=1 when not receiving valid IEEE 802.15.4 data, CCA=0 otherwise
#define CC2520_CCACTRL1_CCA_MODE_3          BIN8(00011000)  // CCA=1 when RSSI_VAL < CCA_THR - CCA_HYST and not receiving valid IEEE 802.15.4 data. CCA=0 when RSSI_VAL >= CCA_THR or receiving a packet
#define CC2520_CCACTRL1_CCA_HYST_0dB        BIN8(00000000)  // CCA Hysteresis 0 dB
#define CC2520_CCACTRL1_CCA_HYST_1dB        BIN8(00000001)  // CCA Hysteresis 1 dB
#define CC2520_CCACTRL1_CCA_HYST_2dB        BIN8(00000010)  // CCA Hysteresis 2 dB
#define CC2520_CCACTRL1_CCA_HYST_3dB        BIN8(00000011)  // CCA Hysteresis 3 dB
#define CC2520_CCACTRL1_CCA_HYST_4dB        BIN8(00000100)  // CCA Hysteresis 4 dB
#define CC2520_CCACTRL1_CCA_HYST_5dB        BIN8(00000101)  // CCA Hysteresis 5 dB
#define CC2520_CCACTRL1_CCA_HYST_6dB        BIN8(00000110)  // CCA Hysteresis 6 dB
#define CC2520_CCACTRL1_CCA_HYST_7dB        BIN8(00000111)  // CCA Hysteresis 7 dB

// Sync Word threshold as per the default value
#define CC2520_MDMCTRL1_CORR_THR_20  0x14

// PA Power
#define CC2520_TXPOWER_PA_POWER_0 0x32

// Transmit Control Register
#define TX_TURNAROUND   13

#define CC2520_RSSI_OFFSET               76  // dBm (from data sheet)
#define CC2520_RSSI_OFFSET_LNA_HIGHGAIN  ( CC2520_RSSI_OFFSET + 8 )
#define CC2520_RSSI_OFFSET_LNA_LOWGAIN   ( CC2520_RSSI_OFFSET - 5 )
#define RSSI_OFFSET (int8)( CC2520_RSSI_OFFSET + CC2520_RSSI_OFFSET_LNA_HIGHGAIN)

// Receive control register 1
#define RXBPF_LOCUR     13
#define RXBPF_MIDCUR    12
#define LOW_LOWGAIN     11
#define MED_LOWGAIN     10
#define HIGH_HGM       9
#define MED_HGM        8

// Frequency Synthesizer Control and Status
#define CAL_DONE      13
#define CAL_RUNNING     12
#define LOCK_LENGTH     11
#define LOCK_STATUS     10

// Security Control Register 0
#define RXFIFO_PROTECTION  9
#define SEC_CBC_HEAD     8
#define SEC_SAKEYSEL     7
#define SEC_TXKEYSEL     6
#define SEC_RXKEYSEL     5

// Battery Monitor Control register
#define BATT_OK        6
#define BATTMON_EN       5

// I/O Configuration Register 0
#define BCN_ACCEPT      11
#define FIFO_POLARITY   10
#define FIFOP_POLARITY     9
#define SFD_POLARITY     8
#define CCA_POLARITY     7

// Manual signal AND override register
#define VGA_RESET_N     15
#define BIAS_PD       14
#define BALUN_CTRL      13
#define RXTX        12
#define PRE_PD        11
#define PA_N_PD       10
#define PA_P_PD        9
#define DAC_LPF_PD       8
#define XOSC16M_PD       7
#define RXBPF_CAL_PD     6
#define CHP_PD         5
#define FS_PD        4
#define ADC_PD         3
#define VGA_PD         2
#define RXBPF_PD       1
#define LNAMIX_PD      0

// Manual signal OR override register
#define VGA_RESET_N     15
#define BIAS_PD       14
#define BALUN_CTRL      13
#define RXTX        12
#define PRE_PD        11
#define PA_N_PD       10
#define PA_P_PD        9
#define DAC_LPF_PD       8
#define XOSC16M_PD       7
#define RXBPF_CAL_PD     6
#define CHP_PD         5
#define FS_PD        4
#define ADC_PD         3
#define VGA_PD         2
#define RXBPF_PD       1
#define LNAMIX_PD      0

// AGC Control
#define VGA_GAIN_OE     11

// AGC Test Register 1
#define AGC_BLANK_MODE      14
#define PEAKDET_CUR_BOOST   13

// Frequency Synthesizer Test Register 0
#define VCO_ARRAY_SETTLE_LONG 11
#define VCO_ARRAY_OE      10

// Frequency Synthesizer Test Register 1
#define VCO_TX_NOCAL      15
#define VCO_ARRAY_CAL_LONG    14
#define VC_DAC_EN        3

// Frequency Synthesizer Test Register 2
#define VCO_CURRENT_OE      12

// Frequency Synthesizer Test Register 3
#define CHP_CAL_DISABLE     15
#define CHP_CURRENT_OE      14
#define CHP_TEST_UP       13
#define CHP_TEST_DN       12
#define CHP_DISABLE       11
#define PD_DELAY        10

// Receiver Bandpass Filters Test Register
#define RXBPF_CAP_OE      14

// ADC Test Register
#define ADC_CLOCK_DISABLE   15

// Top Level Test Register
#define RAM_BIST_RUN       7
#define TEST_BATTMON_EN      6
#define VC_IN_TEST_EN      5  
#define ATESTMOD_PD        4
///////////////////////////////////////////////////////////////////////////////


#define CC2520_1_CCA_STATUS()   GET_GPIO_STATUS( CC2520_1_CCA )
#define CC2520_1_FIFO_STATUS()  GET_GPIO_STATUS( CC2520_1_FIFO )
#define CC2520_1_FIFOP_STATUS() GET_GPIO_STATUS( CC2520_1_FIFOP )
#define CC2520_1_SFD_STATUS()   GET_GPIO_STATUS( CC2520_1_SFD )

#define CC2520_2_CCA_STATUS()   GET_GPIO_STATUS( CC2520_2_CCA )
#define CC2520_2_FIFO_STATUS()  GET_GPIO_STATUS( CC2520_2_FIFO )
#define CC2520_2_FIFOP_STATUS() GET_GPIO_STATUS( CC2520_2_FIFOP )
#define CC2520_2_SFD_STATUS()   GET_GPIO_STATUS( CC2520_2_SFD )

#define CC2520_1_FIFO_IRQ_STATUS(x)  ( (x) & (uint32)CC2520_1_FIFO )
#define CC2520_1_FIFOP_IRQ_STATUS(x) ( (x) & (uint32)CC2520_1_FIFOP )
#define CC2520_1_SFD_IRQ_STATUS(x)   ( (x) & (uint32)CC2520_1_SFD )

#define CC2520_2_FIFO_IRQ_STATUS(x)  ( (x) & (uint32)CC2520_2_FIFO )
#define CC2520_2_FIFOP_IRQ_STATUS(x) ( (x) & (uint32)CC2520_2_FIFOP )
#define CC2520_2_SFD_IRQ_STATUS(x)   ( (x) & (uint32)CC2520_2_SFD )

#define CC2520_1_FIFO_IRQOFF()  AT91C_BASE_PIOB->PIO_IDR = (uint32)(CC2520_1_FIFO) 
#define CC2520_1_FIFOP_IRQOFF() AT91C_BASE_PIOB->PIO_IDR = (uint32)(CC2520_1_FIFOP) 
#define CC2520_1_SFD_IRQOFF()   AT91C_BASE_PIOB->PIO_IDR = (uint32)(CC2520_1_SFD) 

#define CC2520_2_FIFO_IRQOFF()  AT91C_BASE_PIOB->PIO_IDR = (uint32)(CC2520_2_FIFO) 
#define CC2520_2_FIFOP_IRQOFF() AT91C_BASE_PIOB->PIO_IDR = (uint32)(CC2520_2_FIFOP) 
#define CC2520_2_SFD_IRQOFF()   AT91C_BASE_PIOB->PIO_IDR = (uint32)(CC2520_2_SFD) 

#define CC2520_1_VCTRLPA_OFF()  SET_GPIO_LO( CC2520_1_VCTRLPA ) 
#define CC2520_1_VCTRLPA_ON()   SET_GPIO_HI( CC2520_1_VCTRLPA )
#define CC2520_1_SHDNLNA_OFF()  SET_GPIO_LO( CC2520_1_SHDNLNA ) 
#define CC2520_1_SHDNLNA_ON()   SET_GPIO_HI( CC2520_1_SHDNLNA )

#define CC2520_2_VCTRLPA_OFF()  SET_GPIO_LO( CC2520_2_VCTRLPA ) 
#define CC2520_2_VCTRLPA_ON()   SET_GPIO_HI( CC2520_2_VCTRLPA )
#define CC2520_2_SHDNLNA_OFF()  SET_GPIO_LO( CC2520_2_SHDNLNA ) 
#define CC2520_2_SHDNLNA_ON()   SET_GPIO_HI( CC2520_2_SHDNLNA )

#define CC2520_1_MACRO_CLEAR_ALL_IRQS()    (void)(AT91C_BASE_PIOB->PIO_ISR)
#define CC2520_1_MACRO_OFF_ALL_IRQS()      AT91C_BASE_PIOB->PIO_IDR = (uint32)(CC2520_1_FIFO | CC2520_1_FIFOP | CC2520_1_SFD)
#define CC2520_1_MACRO_ON_RX_IRQS()        {(void)(AT91C_BASE_PIOB->PIO_ISR); AT91C_BASE_PIOB->PIO_IER = (uint32)(CC2520_1_FIFO | CC2520_1_FIFOP | CC2520_1_SFD);} 
#define CC2520_1_MACRO_ON_TX_IRQS()        {(void)(AT91C_BASE_PIOB->PIO_ISR); AT91C_BASE_PIOB->PIO_IER = (uint32)(CC2520_1_SFD);}

#define CC2520_2_MACRO_CLEAR_ALL_IRQS()    (void)(AT91C_BASE_PIOB->PIO_ISR)
#define CC2520_2_MACRO_OFF_ALL_IRQS()      AT91C_BASE_PIOB->PIO_IDR = (uint32)(CC2520_2_FIFO | CC2520_2_FIFOP | CC2520_2_SFD)
#define CC2520_2_MACRO_ON_RX_IRQS()        {(void)(AT91C_BASE_PIOB->PIO_ISR); AT91C_BASE_PIOB->PIO_IER = (uint32)(CC2520_2_FIFO | CC2520_2_FIFOP | CC2520_2_SFD);} 
#define CC2520_2_MACRO_ON_TX_IRQS()        {(void)(AT91C_BASE_PIOB->PIO_ISR); AT91C_BASE_PIOB->PIO_IER = (uint32)(CC2520_2_SFD);}

// CC2520 Radio Modem states
enum CC2520_MODEM_STATE
{
  CC2520_MODEM_STATE_POWERDOWN = 0,
  CC2520_MODEM_STATE_POWERUP,
  CC2520_MODEM_STATE_IDLE,
  CC2520_MODEM_STATE_RX,
  CC2520_MODEM_STATE_RX_CONTINUOUS,
//CC2520_MODEM_STATE_TX_WITH_CCA, // do that manually
  CC2520_MODEM_STATE_TX_NO_CCA,
  CC2520_MODEM_STATE_TX_CONTINUOUS_MOD,
  CC2520_MODEM_STATE_TX_CONTINUOUS_NOMOD,
  CC2520_MODEM_STATE_CCA,
  CC2520_MODEM_STATE_TX_MSG
};

typedef enum RadioDataIntegrity
{
  BOTH_RADIO_CORRECT = 0,
  ONLY_RADIO_1_ERROR = 1,
  ONLY_RADIO_2_ERROR = 2,
  BOTH_RADIO_ERROR   = 3
}eRadioDataIntegrity;

void CC2520_Init(uint8 Radio1or2, uint8 p_ucGlowLED);

void CC2520_SetModemState(unsigned char newModemState);

extern unsigned int g_unCC2520SecCtrl_0; 
extern unsigned int g_unCC2520SecCtrl_1; 
extern uint8 eActiveTransmitter; 
extern uint8 g_ucRadioOperationSts;
extern uint8 g_ucOldRadioOperationSts, g_ucOldActiveTransmitter;



#define RADIO_1_FAIL    BIT4
#define RADIO_2_FAIL    BIT5

#define MAX_DBM_POWER_LEVEL         20
#define NOMINAL_DBM_POWER_LEVEL     16
#define MIN_DBM_POWER_LEVEL         (-5)
#define DEFAULT_DBM_POWER_LEVEL     NOMINAL_DBM_POWER_LEVEL



#ifdef LOW_POWER_DEVICE
  extern unsigned char g_ucModemState;
#endif  

#define CC2520_IsBusyChannel() ((CC2520_RADIO_2 == eActiveTransmitter)?(CC2520_2_CCA_STATUS() == 0): (CC2520_1_CCA_STATUS() == 0))

#endif // BBR2_HW
#endif // CC2520_H
