//=========================================================================================
// Name:        CC2420.h
// Author:      Rares Ivan
// Date:        February 2008
// Description: CC2420 Driver
// Changes:     
// Revisions:   
//=========================================================================================

#ifdef BBR1_HW
#ifndef CC2420_H
#define CC2420_H

#include "../digitals.h"

//-------------------------------------------------------------------------------------------------------
//	CC2420 register constants
#define CC2420_SNOP             0x00
#define CC2420_SXOSCON          0x01
#define CC2420_STXCAL           0x02
#define CC2420_SRXON            0x03
#define CC2420_STXON            0x04
#define CC2420_STXONCCA         0x05
#define CC2420_SRFOFF           0x06
#define CC2420_SXOSCOFF         0x07
#define CC2420_SFLUSHRX         0x08
#define CC2420_SFLUSHTX         0x09
#define CC2420_SACK             0x0A
#define CC2420_SACKPEND         0x0B
#define CC2420_SRXDEC           0x0C
#define CC2420_STXENC           0x0D
#define CC2420_SAES             0x0E

#define CC2420_MAIN             0x10
#define CC2420_MDMCTRL0         0x11
#define CC2420_MDMCTRL1         0x12
#define CC2420_RSSI             0x13
#define CC2420_SYNCWORD         0x14
#define CC2420_TXCTRL           0x15
#define CC2420_RXCTRL0          0x16
#define CC2420_RXCTRL1          0x17
#define CC2420_FSCTRL           0x18
#define CC2420_SECCTRL0         0x19
#define CC2420_SECCTRL1         0x1A
#define CC2420_BATTMON          0x1B
#define CC2420_IOCFG0           0x1C
#define CC2420_IOCFG1           0x1D
#define CC2420_MANFIDL          0x1E
#define CC2420_MANFIDH          0x1F
#define CC2420_FSMTC            0x20
#define CC2420_MANAND           0x21
#define CC2420_MANOR            0x22
#define CC2420_AGCCTRL          0x23
#define CC2420_AGCTST0          0x24
#define CC2420_AGCTST1          0x25
#define CC2420_AGCTST2          0x26
#define CC2420_FSTST0           0x27
#define CC2420_FSTST1           0x28
#define CC2420_FSTST2           0x29
#define CC2420_FSTST3           0x2A
#define CC2420_RXBPFTST         0x2B
#define CC2420_FSMSTATE         0x2C
#define CC2420_ADCTST           0x2D
#define CC2420_DACTST           0x2E
#define CC2420_TOPTST           0x2F
#define CC2420_RESERVED         0x30

#define CC2420_TXFIFO           0x3E
#define CC2420_RXFIFO           0x3F
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Memory

// Sizes
#define CC2420_RAM_SIZE			368
#define CC2420_FIFO_SIZE		128

// Addresses
#define CC2420RAM_TXFIFO		0x000
#define CC2420RAM_RXFIFO		0x080
#define CC2420RAM_KEY0			0x100
#define CC2420RAM_RXNONCE		0x110
#define CC2420RAM_SABUF			0x120
#define CC2420RAM_KEY1			0x130
#define CC2420RAM_TXNONCE		0x140
#define CC2420RAM_CBCSTATE		0x150
#define CC2420RAM_IEEEADDR		0x160
#define CC2420RAM_PANID			0x168
#define CC2420RAM_SHORTADDR		0x16A
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Status byte
#define CC2420_RSSI_VALID		1
#define CC2420_LOCK			    2
#define CC2420_TX_ACTIVE		3
#define CC2420_ENC_BUSY			4
#define CC2420_TX_UNDERFLOW		5
#define CC2420_XOSC16M_STABLE	6

//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
// SECCTRL0.BitMask
#define CC2420_SECCTRL0_NO_SECURITY         0x0000
#define CC2420_SECCTRL0_CBC_MAC             0x0001
#define CC2420_SECCTRL0_CTR                 0x0002
#define CC2420_SECCTRL0_CCM                 0x0003

#define CC2420_SECCTRL0_SEC_M_0_BYTES        BIN16(00000000,00000000)
#define CC2420_SECCTRL0_SEC_M_4_BYTES        BIN16(00000000,00000100)
#define CC2420_SECCTRL0_SEC_M_6_BYTES        BIN16(00000000,00001000)
#define CC2420_SECCTRL0_SEC_M_8_BYTES        BIN16(00000000,00001100)
#define CC2420_SECCTRL0_SEC_M_10_BYTES       BIN16(00000000,00010000)
#define CC2420_SECCTRL0_SEC_M_12_BYTES       BIN16(00000000,00010100)
#define CC2420_SECCTRL0_SEC_M_14_BYTES       BIN16(00000000,00011000)
#define CC2420_SECCTRL0_SEC_M_16_BYTES       BIN16(00000000,00011100)

#define CC2420_SECCTRL0_RXKEYSEL0_MASK           0x0000  // Bit 5
#define CC2420_SECCTRL0_RXKEYSEL1_MASK           0x0020  // Bit 5

#define CC2420_SECCTRL0_TXKEYSEL0_MASK           0x0000  // Bit 6
#define CC2420_SECCTRL0_TXKEYSEL1_MASK           0x0040  // Bit 6

#define CC2420_SECCTRL0_SEC_CBC_HEAD_MASK        0x0100  // Bit 8
#define CC2420_SECCTRL0_RXFIFO_PROTECTION_MASK   0x0200  // Bit 9

// SECCTRL0.Bits
#define CC2420_SECCTRL0_SEC_MODE_BIT0       BIT0
#define CC2420_SECCTRL0_SEC_MODE_BIT1       BIT1
#define CC2420_SECCTRL0_SEC_M_BIT0          BIT2
#define CC2420_SECCTRL0_SEC_M_BIT1          BIT3
#define CC2420_SECCTRL0_SEC_M_BIT2          BIT4
#define CC2420_SECCTRL0_SEC_RXKEYSEL        BIT5
#define CC2420_SECCTRL0_SEC_TXKEYSEL        BIT6
#define CC2420_SECCTRL0_SEC_SAKEYSEL        BIT7
#define CC2420_SECCTRL0_SEC_CBC_HEAD        BIT8
#define CC2420_SECCTRL0_RXFIFO_PROTECTION   BIT9

// MDMCTRL0 (0x11) - Modem Control Register 0
// MDMCTRL0.Bits
#define CC2420_MDMCTRL0_PREAMBLE_LENGTH0    BIT0  // See CC2420 page 64. The number of preamble bytes (2 zero-symbols) to be sent in TX mode prior to the SYNCWORD, encoded in steps of 2.
#define CC2420_MDMCTRL0_PREAMBLE_LENGTH1    BIT1  // -- // --
#define CC2420_MDMCTRL0_PREAMBLE_LENGTH2    BIT2  // -- // --
#define CC2420_MDMCTRL0_PREAMBLE_LENGTH3    BIT3  // -- // --
#define CC2420_MDMCTRL0_AUTOACK             BIT4  // If AUTOACK is set, all packets accepted by address recognition with the acknowledge request flag set and a valid CRC are acknowledged 12 symbol periods after being received.
#define CC2420_MDMCTRL0_AUTOCRC             BIT5  // In packet mode a CRC-16 (ITU-T) is calculated and is transmitted after the last data byte in TX. In RX CRC is calculated and checked for validity.
#define CC2420_MDMCTRL0_CCA_MODE0           BIT6  // See CC2420 page 64 for CCA Modes
#define CC2420_MDMCTRL0_CCA_MODE1           BIT7  // -- // --
#define CC2420_MDMCTRL0_CCA_HYST0           BIT8  // CCA Hysteresis in dB, values 0 through 7 dB
#define CC2420_MDMCTRL0_CCA_HYST1           BIT9  // -- // --
#define CC2420_MDMCTRL0_CCA_HYST2           BIT10 // -- // --
#define CC2420_MDMCTRL0_ADR_DECODE          BIT11 // Hardware Address decode enable = 1, disabled = 0.
#define CC2420_MDMCTRL0_PAN_COORDINATOR     BIT12 // Should be set high when the device is a PAN Coordinator. Used for filtering packets with no destination address, as specified in section 7.5.6.2 in 802.15.4, D18
#define CC2420_MDMCTRL0_RESERVED_FRAME_MODE BIT13 // Mode for accepting reserved IEE 802.15.4 frame types when address recognition is enabled (MDMCTRL0.ADR_DECODE = 1)
#define CC2420_MDMCTRL0_RESERVED0           BIT14 // Reserved, write as 0
#define CC2420_MDMCTRL0_RESERVED1           BIT15 // -- // --
// MDMCTRL0.Bits.Mask
#define CC2420_MDMCTRL0_PREAMBLE_LENGTH_1   BIN16(00000000,00000000)  // 1 leading zero bytes (not recommended)
#define CC2420_MDMCTRL0_PREAMBLE_LENGTH_2   BIN16(00000000,00000001)  // 2 leading zero bytes (not recommended)
#define CC2420_MDMCTRL0_PREAMBLE_LENGTH_3   BIN16(00000000,00000010)  // 3 leading zero bytes (IEEE 802.15.4)
#define CC2420_MDMCTRL0_PREAMBLE_LENGTH_4   BIN16(00000000,00000011)  // 4 leading zero bytes 
// ...
#define CC2420_MDMCTRL0_PREAMBLE_LENGTH_16  BIN16(00000000,00001111)  // 16 leading zero bytes
#define CC2420_MDMCTRL0_CCA_MODE_0          BIN16(00000000,00000000)  // Reserved
#define CC2420_MDMCTRL0_CCA_MODE_1          BIN16(00000000,01000000)  // CCA=1 when RSSI_VAL < CCA_THR - CCA_HYST, CCA=0 when RSSI_VAL >= CCA_THR
#define CC2420_MDMCTRL0_CCA_MODE_2          BIN16(00000000,10000000)  // CCA=1 when not receiving valid IEEE 802.15.4 data, CCA=0 otherwise
#define CC2420_MDMCTRL0_CCA_MODE_3          BIN16(00000000,11000000)  // CCA=1 when RSSI_VAL < CCA_THR - CCA_HYST and not receiving valid IEEE 802.15.4 data. CCA=0 when RSSI_VAL >= CCA_THR or receiving a packet
#define CC2420_MDMCTRL0_CCA_HYST_0dB        BIN16(00000000,00000000)  // CCA Hysteresis 0 dB
#define CC2420_MDMCTRL0_CCA_HYST_1dB        BIN16(00000001,00000000)  // CCA Hysteresis 1 dB
#define CC2420_MDMCTRL0_CCA_HYST_2dB        BIN16(00000010,00000000)  // CCA Hysteresis 2 dB
#define CC2420_MDMCTRL0_CCA_HYST_3dB        BIN16(00000011,00000000)  // CCA Hysteresis 3 dB
#define CC2420_MDMCTRL0_CCA_HYST_4dB        BIN16(00000100,00000000)  // CCA Hysteresis 4 dB
#define CC2420_MDMCTRL0_CCA_HYST_5dB        BIN16(00000101,00000000)  // CCA Hysteresis 5 dB
#define CC2420_MDMCTRL0_CCA_HYST_6dB        BIN16(00000110,00000000)  // CCA Hysteresis 6 dB
#define CC2420_MDMCTRL0_CCA_HYST_7dB        BIN16(00000111,00000000)  // CCA Hysteresis 7 dB
#define CC2420_MDMCTRL0_RESERVED            BIN16(00000000,00000000)  // Reserved, write as 0

// MDMCTRL1 (0x12) - Modem Control Register 1
// MDMCTRL1.Bits
#define CC2420_MDMCTRL1_RX_MODE0            BIT0  // Set test modes for RX
#define CC2420_MDMCTRL1_RX_MODE1            BIT1  // -- // --
#define CC2420_MDMCTRL1_TX_MODE0            BIT2  // Set test modes for TX
#define CC2420_MDMCTRL1_TX_MODE1            BIT3  // -- // --
#define CC2420_MDMCTRL1_MODULATION_MODE     BIT4  // Set one of two RF modulation modes for RX / TX. 0 : IEEE 802.15.4 compliant mode, 1 : Reversed phase, non-IEEE compliant (could be used to set up a system which will not receive 802.15.4 packets)
#define CC2420_MDMCTRL1_DEMOD_AVG_MODE      BIT5  // Frequency offset average filter behaviour. 0 : Lock frequency offset filter after preamble match, 1 : Continuously update frequency offset filter.
#define CC2420_MDMCTRL1_CORR_THR0           BIT6  // Demodulator correlator threshold value, required before SFD search. Note that on early CC2420 versions the reset value was 0.
#define CC2420_MDMCTRL1_CORR_THR1           BIT7  // -- // --
#define CC2420_MDMCTRL1_CORR_THR2           BIT8  // -- // --
#define CC2420_MDMCTRL1_CORR_THR3           BIT9  // -- // --
#define CC2420_MDMCTRL1_CORR_THR4           BIT10 // -- // --
#define CC2420_MDMCTRL1_RESERVED0           BIT11 // Reserved, write as 0
#define CC2420_MDMCTRL1_RESERVED1           BIT12 // -- // --
#define CC2420_MDMCTRL1_RESERVED2           BIT13 // -- // --
#define CC2420_MDMCTRL1_RESERVED3           BIT14 // -- // --
#define CC2420_MDMCTRL1_RESERVED4           BIT15 // -- // --
// MDMCTRL1.Bits.Mask
#define CC2420_MDMCTRL1_RX_MODE_NORMAL      BIN16(00000000,00000000)
#define CC2420_MDMCTRL1_RX_MODE_SERIAL      BIN16(00000000,00000001)
#define CC2420_MDMCTRL1_RX_MODE_LOOPING     BIN16(00000000,00000010)
#define CC2420_MDMCTRL1_RX_MODE_RESERVED    BIN16(00000000,00000011)
#define CC2420_MDMCTRL1_TX_MODE_NORMAL      BIN16(00000000,00000000)
#define CC2420_MDMCTRL1_TX_MODE_SERIAL      BIN16(00000000,00000100)
#define CC2420_MDMCTRL1_TX_MODE_LOOPING     BIN16(00000000,00001000)
#define CC2420_MDMCTRL1_TX_MODE_RESERVED    BIN16(00000000,00001100)
#define CC2420_MDMCTRL1_CORR_THR_20         BIN16(00000101,00000000)  // 0x14 = 20
#define CC2420_MDMCTRL1_RESERVED            BIN16(00000000,00000000)  // Reserved, write as 0

// TXCTRL (0x15) - Transmit Control Register
// TXCTRL.Bits.Mask
// TXCTRL.Bits.Mask
#define CC2420_TXCTRL_PA_LEVEL_MAX          BIN16(00000000,00011111)    // Output PA level (~0 dBm)
#define CC2420_TXCTRL_PA_LEVEL_DEFAULT      CC2420_TXCTRL_PA_LEVEL_MAX
#define CC2420_TXCTRL_RESERVED              BIN16(00000000,00100000)    // Reserved, write as 1
#define CC2420_TXCTRL_PA_CURRENT_MINUS_3    BIN16(00000000,00000000)    // -3 Current Adjustment. Current programming of the PA.
#define CC2420_TXCTRL_PA_CURRENT_MINUS_2    BIN16(00000000,01000000)    // -2 Current Adjustment. Current programming of the PA.
#define CC2420_TXCTRL_PA_CURRENT_MINUS_1    BIN16(00000000,10000000)    // -1 Current Adjustment. Current programming of the PA.
#define CC2420_TXCTRL_PA_CURRENT_NOMINAL    BIN16(00000000,11000000)    //  0 Current Adjustment - Nominal setting. Current programming of the PA.
#define CC2420_TXCTRL_PA_CURRENT_PLUS_1     BIN16(00000001,00000000)    // +1 Current Adjustment. Current programming of the PA.
#define CC2420_TXCTRL_PA_CURRENT_PLUS_2     BIN16(00000001,01000000)    // +2 Current Adjustment. Current programming of the PA.
#define CC2420_TXCTRL_PA_CURRENT_PLUS_3     BIN16(00000001,10000000)    // +3 Current Adjustment. Current programming of the PA.
#define CC2420_TXCTRL_PA_CURRENT_PLUS_4     BIN16(00000001,11000000)    // +4 Current Adjustment. Current programming of the PA.
#define CC2420_TXCTRL_TXMIXCURRENT_1720uA   BIN16(00000000,00000000)    // 0: 1.72 mA. Transmit mixers current.
#define CC2420_TXCTRL_TXMIXCURRENT_1880uA   BIN16(00000010,00000000)    // 1: 1.88 mA. Transmit mixers current.
#define CC2420_TXCTRL_TXMIXCURRENT_2050uA   BIN16(00000100,00000000)    // 2: 2.05 mA. Transmit mixers current.
#define CC2420_TXCTRL_TXMIXCURRENT_2210uA   BIN16(00000110,00000000)    // 3: 2.21 mA. Transmit mixers current.
#define CC2420_TXCTRL_TXMIX_CAP_ARRAY_0     BIN16(00000000,00000000)    // 0. Selects varactor array settings in the transmit mixers.
#define CC2420_TXCTRL_TX_TURNAROUND_128us   BIN16(00000000,00000000)    // 0 :  8 symbol periods (128 us). Sets the wait time after STXON before transmission is started.
#define CC2420_TXCTRL_TX_TURNAROUND_192us   BIN16(00100000,00000000)    // 1 : 12 symbol periods (192 us). Sets the wait time after STXON before transmission is started.
#define CC2420_TXCTRL_TXMIXBUF_CUR_690uA    BIN16(00000000,00000000)    // 0: 690uA. TX mixer buffer bias current.
#define CC2420_TXCTRL_TXMIXBUF_CUR_980uA    BIN16(01000000,00000000)    // 1: 980uA. TX mixer buffer bias current.
#define CC2420_TXCTRL_TXMIXBUF_CUR_1160uA   BIN16(10000000,00000000)    // 2: 1.16mA - Nominal setting. TX mixer buffer bias current.
#define CC2420_TXCTRL_TXMIXBUF_CUR_1440uA   BIN16(11000000,00000000)    // 3: 1.44mA. TX mixer buffer bias current.
#define CC2420_TXCTRL_TXMIXBUF_CUR_DEFAULT  CC2420_TXCTRL_TXMIXBUF_1160uA

// RSSI (0x13) - RSSI and CCA Status and Control Register
// RSSI.Bits
//   RSSI_VAL  BIT7:BIT0   // RSSI estimate on a logarithmic scale, signed number on 2’s complement.
//   CCA_THR   BIT15:BIT8  // Clear Channel Assessment threshold value, signed number on 2’s complement for comparison with the RSSI.

// SYNCWORD (0x14) - Sync Word
// SYNCWORD.Bits
//   SYNCWORD  BIT15:BIT0  // Synchronisation word.

//-------------------------------------------------------------------------------------------------------
// RSSI to Energy Detection conversion
// RSSI_OFFSET defines the RSSI level where the PLME.ED generates a zero-value
#define RSSI_OFFSET -38
#define RSSI_2_ED(rssi)   ((rssi) < RSSI_OFFSET ? 0 : ((rssi) - (RSSI_OFFSET)))
#define ED_2_LQI(ed) (((ed) > 63 ? 255 : ((ed) << 2)))
//-------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------
// CC2420 Frame Format:
//
//  +------------------+------------+---------------------+-------------------+-------------+
//  | Preamble 4 bytes | SFD 1 byte | Frame Length 1 byte | Payload 125 bytes | FCS 2 bytes |
//  +------------------+------------+---------------------+-------------------+-------------+
//                                  | Packet Length 128 bytes                               |
//                                  +-------------------------------------------------------+
//
//----------------------------------------------------------------------------
#define FRAME_LEN_LENGTH          1   // 1rst byte in RX/TX FIFOs is Frame Length
#define FCS_LENGTH                2   // Frame Check Sequence 2 bytes added by hardware at TX time
#define MAX_FRAME_PAYLOAD_LENGTH  125 // Max Frame length without the FCS
#define MAX_FRAME_LENGTH          MAX_FRAME_PAYLOAD_LENGTH + FCS_LENGTH
#define MAX_BUFFER_LENGTH         FRAME_LEN_LENGTH + MAX_FRAME_LENGTH

// CC2420 Radio Modem states
enum CC2420_MODEM_STATE
{
  CC2420_MODEM_STATE_POWERDOWN = 0,
  CC2420_MODEM_STATE_POWERUP,
  CC2420_MODEM_STATE_IDLE,
  CC2420_MODEM_STATE_RX,
  CC2420_MODEM_STATE_RX_CONTINUOUS,
//CC2420_MODEM_STATE_TX_WITH_CCA, // do that manually
  CC2420_MODEM_STATE_TX_NO_CCA,
  CC2420_MODEM_STATE_TX_CONTINUOUS_MOD,
  CC2420_MODEM_STATE_TX_CONTINUOUS_NOMOD,
  CC2420_MODEM_STATE_CCA,
  CC2420_MODEM_STATE_TX_MSG
};

// CC2420 AES Encryption Engine mode
enum CC2420_AES_MODE
{
  AES_MODE_NONE   = CC2420_SECCTRL0_NO_SECURITY,
  AES_MODE_CBCMAC = CC2420_SECCTRL0_CBC_MAC,
  AES_MODE_CTR    = CC2420_SECCTRL0_CTR,
  AES_MODE_CCM    = CC2420_SECCTRL0_CCM,
  AES_MODE_ERROR  = CC2420_SECCTRL0_CCM + 1
};
// CC2420 AES Encryption Keys
enum CC2420_AES_KEYS
{
  AES_KEY0 = 0,
  AES_KEY1
};

#define CC2420_CCA_STATUS()   GET_GPIO_STATUS( CC2420_CCA )
#define CC2420_FIFO_STATUS()  GET_GPIO_STATUS( CC2420_FIFO )
#define CC2420_FIFOP_STATUS() GET_GPIO_STATUS( CC2420_FIFOP )
#define CC2420_SFD_STATUS()   GET_GPIO_STATUS( CC2420_SFD )

#define CC2420_FIFO_IRQ_STATUS(x)  ( (x) & (uint32)CC2420_FIFO )
#define CC2420_FIFOP_IRQ_STATUS(x) ( (x) & (uint32)CC2420_FIFOP )
#define CC2420_SFD_IRQ_STATUS(x)   ( (x) & (uint32)CC2420_SFD )

#define CC2420_FIFO_IRQOFF()  AT91C_BASE_PIOB->PIO_IDR = (uint32)(CC2420_FIFO) 
#define CC2420_FIFOP_IRQOFF() AT91C_BASE_PIOB->PIO_IDR = (uint32)(CC2420_FIFOP) 
#define CC2420_SFD_IRQOFF()   AT91C_BASE_PIOB->PIO_IDR = (uint32)(CC2420_SFD) 

#define CC2420_VCTRLPA_OFF()  SET_GPIO_LO( CC2420_VCTRLPA ) 
#define CC2420_VCTRLPA_ON()   SET_GPIO_HI( CC2420_VCTRLPA )
#define CC2420_SHDNLNA_OFF()  SET_GPIO_LO( CC2420_SHDNLNA ) 
#define CC2420_SHDNLNA_ON()   SET_GPIO_HI( CC2420_SHDNLNA )

#define CC2420_MACRO_CLEAR_ALL_IRQS()    (void)(AT91C_BASE_PIOB->PIO_ISR)
#define CC2420_MACRO_OFF_ALL_IRQS()      AT91C_BASE_PIOB->PIO_IDR = (uint32)(CC2420_FIFO | CC2420_FIFOP | CC2420_SFD)
#define CC2420_MACRO_ON_RX_IRQS()        {(void)(AT91C_BASE_PIOB->PIO_ISR); AT91C_BASE_PIOB->PIO_IER = (uint32)(CC2420_FIFO | CC2420_FIFOP | CC2420_SFD);}
#define CC2420_MACRO_ON_TX_IRQS()        {(void)(AT91C_BASE_PIOB->PIO_ISR); AT91C_BASE_PIOB->PIO_IER = (uint32)(CC2420_SFD);}

void CC2420_Init();

void CC2420_SetModemState(unsigned char newModemState);

extern unsigned int g_unCC2420SecCtrl_0;
extern unsigned int g_unCC2420SecCtrl_1; 

#ifdef LOW_POWER_DEVICE
  extern unsigned char g_ucModemState;
#endif  

void CC2420_WaitUntilStatusBitClear( uint8 p_ucBitMask );
void CC2420_WaitUntilStatusBitSet( uint8 p_ucBitMask );

#define CC2420_IsBusyChannel() ( CC2420_CCA_STATUS() == 0 )

#endif
#endif 