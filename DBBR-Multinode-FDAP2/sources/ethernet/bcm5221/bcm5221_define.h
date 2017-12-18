
#ifndef _BCM5221_DEFINE_H
#define _BCM5221_DEFINE_H

// BROADCOM PHYSICAL LAYER TRANSCEIVER BCM5221

//-----------------------------------------------------------------------------
///         Definitions
//-----------------------------------------------------------------------------

/// Registers
#define BCM5221_CTRL        0   // Basic Mode Control Register
#define BCM5221_STTS        1   // Basic Mode Status Register
#define BCM5221_PHYIDH      2   // PHY Idendifier Register 1
#define BCM5221_PHYIDL      3   // PHY Idendifier Register 2
#define BCM5221_ANAR        4   // Auto_Negotiation Advertisement Register
#define BCM5221_ANLPAR      5   // Auto_negotiation Link Partner Ability Register
#define BCM5221_ANER        6   // Auto-negotiation Expansion Register
#define BCM5221_NXT         7   // Next Page Register
#define BCM5221_LPNXT       8   // LP Next Page Register
#define BCM5221_AXCTRL     16   // 100Base-X Aux Control Register
#define BCM5221_AXSTTS     17   // 100Base-X Aux Status Register
#define BCM5221_RCVCNT     18   // 100Base-X Rcv Error Counter Register
#define BCM5221_FCCNT      19   // 100Base-X False Carrier Counter Register
#define BCM5221_DSCCNT     20   // 100Base-X Disconnect Counter Register
#define BCM5221_PTEST      23   // PTEST
#define BCM5221_AXCTTS     24   // Auxiliary Control /Status Register
#define BCM5221_AXSTSS     25   // Auxiliary Status Summary Register
#define BCM5221_INT        26   // Interrupt Register
#define BCM5221_AXM2       27   // Auxiliary Mode 2 Register
#define BCM5221_AXERTS     28   // 10Base-T Aux Error & General Status Register
#define BCM5221_AXMOD      29   // Auxiliary Mode Register
#define BCM5221_AXMLT      30   // Auxiliary Multi-PHY Register
#define BCM5221_TEST       31   // Broadcom Test Register

/// Shadow Registers
#define BCM5221_AXM4        26   // Auxiliary Mode 4 Register
#define BCM5221_AXS2        27   // Auxiliary Status 2 Register
#define BCM5221_AXS3        28   // Auxiliary Status 3 Register
#define BCM5221_AXM3        29   // Auxiliary Mode 3 Register
#define BCM5221_AXS4        30   // Auxiliary Status 4 Register


// Basic Mode Control Register
// Bit definitions: BCM5221_CTRL
#define BCM5221_RESET            (1 << 15) // 1=PHY Reset; 0=Normal Operation
#define BCM5221_LOOPBACK         (1 << 14) // 1=Loopback Enabled; 0=Normal Operation
#define BCM5221_SPEED_SELECT     (1 << 13) // 1=100Mbps; 0=10Mbps
#define BCM5221_AUTONEG          (1 << 12) // Auto-negotiation Enable
#define BCM5221_POWER_DOWN       (1 << 11) // 1=Power down 0=Normal operation
#define BCM5221_ISOLATE          (1 << 10) // 1=Isolates 0=Normal operation
#define BCM5221_RESTART_AUTONEG  (1 << 9)  // 1=Restart auto-negotiation 0=Normal operation
#define BCM5221_DUPLEX_MODE      (1 << 8)  // 1=Full duplex operation 0=Half duplex operation
#define BCM5221_COLLISION_TEST   (1 << 7)  // 1=Collision test enabled 0=Normal operation
//      Reserved                  6 to 0   // Read as 0, ignore on write

// Basic Mode Status Register 
// Bit definitions: BCM5221_STTS
#define BCM5221_100BASE_T4        (1 << 15) // 100BASE-T4 Capable
#define BCM5221_100BASE_TX_FD     (1 << 14) // 100BASE-TX Full Duplex Capable
#define BCM5221_100BASE_TX_HD     (1 << 13) // 100BASE-TX Half Duplex Capable
#define BCM5221_10BASE_T_FD       (1 << 12) // 10BASE-T Full Duplex Capable
#define BCM5221_10BASE_T_HD       (1 << 11) // 10BASE-T Half Duplex Capable
//      Reserved                   10 to 7  // Read as 0, ignore on write
#define BCM5221_MF_PREAMB_SUPPR   (1 << 6)  // MII Frame Preamble Suppression
#define BCM5221_AUTONEG_COMP      (1 << 5)  // Auto-negotiation Complete
#define BCM5221_REMOTE_FAULT      (1 << 4)  // Remote Fault
#define BCM5221_AUTONEG_ABILITY   (1 << 3)  // Auto Configuration Ability
#define BCM5221_LINK_STATUS       (1 << 2)  // Link Status
#define BCM5221_JABBER_DETECT     (1 << 1)  // Jabber Detect
#define BCM5221_EXTEND_CAPAB      (1 << 0)  // Extended Capability

// PHY ID Identifier Register
// definitions: BCM5221_PHYIDx
// not used #define DM9161_PHYID1_OUI         0x606E   // OUI: Organizationally Unique Identifier
#define BCM5221_LSB_MASK            0x3F
// not used #define DM9161_ID             0x0181b8a0
#define BCM5221_OUI_MSB           0x0040 //sau 0041
#define BCM5221_OUI_LSB             0x18

// Auto-negotiation Advertisement Register (ANAR)
// Auto-negotiation Link Partner Ability Register (ANLPAR)
// Bit definitions: BCM5221_ANAR, BCM5221_ANLPAR
#define BCM5221_NP               (1 << 15) // Next page Indication
#define BCM5221_ACK              (1 << 14) // Acknowledge (only ANLPAR)
#define BCM5221_RF               (1 << 13) // Remote Fault
//      Reserved                 12 to 11  // Write as 0, ignore on read
#define BCM5221_PAUSE            (1 << 10) // 1=Pause Operation For Full Duplex
#define BCM5221_T4               (1 << 9)  // 1=Advertise T4 Capability
#define BCM5221_TX_FDX           (1 << 8)  // 1=Advertise 100BASE-X Full Duplex Support
#define BCM5221_TX_HDX           (1 << 7)  // 1=Advertise 100BASE-X 1/2 Duplex
#define BCM5221_10_FDX           (1 << 6)  // 1=Advertise 10BASE-T Full Duplex Support
#define BCM5221_10_HDX           (1 << 5)  // 1=Advertise 10BASE-T 1/2 Duplex
//      Selector                  4 to 0   // Protocol Selection Bits
#define BCM5221_AN_IEEE_802_3      0x0001

// Auto-negotiation Expansion Register (ANER)
// Bit definitions: BCM5221_ANER
//      Reserved                 15 to 5  // Read as 0, ignore on write
#define BCM5221_PDF              (1 << 4) // Local Device Parallel Detection Fault
#define BCM5221_LP_NP_ABLE       (1 << 3) // Link Partner Next Page Able
#define BCM5221_NP_ABLE          (1 << 2) // Local Device Next Page Able
#define BCM5221_PAGE_RX          (1 << 1) // New Page Received
#define BCM5221_LP_AN_ABLE       (1 << 0) // Link Partner Auto-negotiation Able

// Next Page Register
// LP Next Page Register
// Bit definitions: BCM5221_NXT, BCM5221_LPNXT
#define BCM5221_NPAGE           (1 << 15) // 1=Additional Next Page(s) Will Follow
//      Reserved                      14  // Reserved
#define BCM5221_MPAGE           (1 << 13) // Message Page
#define BCM5221_ACK2            (1 << 12) // ?? todo
#define BCM5221_TGGL            (1 << 11) // ?? todo
//      Code Field              10 to 0   // Message /Unformatted Code Field

// 100Base-X Aux Control Register
// Bit definitions: BCM5221_AXCTRL
//      Reserved                 15 to 14  // Read/ mofify /write to preserve
#define BCM5221_TX_DISABLE       (1 << 13) // 1=Transmitter Disabled in PHY
//      Reserved                 12 to 11  // Read/ mofify /write to preserve
#define BCM5221_BP4B5B           (1 << 10) // Bypass 4B5B Encoding and 5B4B Decoding
#define BCM5221_BP_SCR           (1 << 9 ) // Bypass Scrambler/Descrambler Function
#define BCM5221_BP_NRZI          (1 << 8 ) // Bypass NRZI Function
#define BCM5221_BP_ALIGN         (1 << 7 ) // Bypass Symbol Alignment Function
#define BCM5221_BP_WANDER        (1 << 6 ) // Baseline Wander Correction Disable/Enable
#define BCM5221_EN_FEF           (1 << 5 ) // Far End Fault Enable
//      Reserved                 4 to 3    // Read as 0, ignore on write
#define BCM5221_EN_FIFO          (1 << 2 ) // Extended FIFO Enable
#define BCM5221_EN_RMII          (1 << 1 ) // RMII Out-Of-Band Enable
//      Reserved                       1   // Read/ mofify /write to preserve

// 100Base-X Aux Status Register
// Bit definitions: BCM5221_AXSTTS   
//      Reserved                 15 to 12 // Read/ mofify /write to preserve
#define BCM5221_RMII_ERR        (1 << 11) // 1=Error detected
#define BCM5221_FXTX            (1 << 10) // 1=100BASE-FX mode 0=100BASE-TX or 10BASE-T mode
#define BCM5221_LOCK            (1 << 9 ) // 1=Descrambler locked
#define BCM5221_LINK_100        (1 << 8 ) // 1=Link Pass
#define BCM5221_FAULT           (1 << 7 ) // 1=Remote fault detected
//      Reserved                      6   // Read/ mofify /write to preserve
#define BCM5221_FALSE_CD        (1 << 5 ) // 1=False carrier detected since last read
#define BCM5221_BAD_ESD         (1 << 4 ) // 1=ESD error detected since last read
#define BCM5221_RX_ERR          (1 << 3 ) // 1=Receive error detected since last read
#define BCM5221_TX_ERR          (1 << 2 ) // 1=Transmit error detected since last read
#define BCM5221_LOCK_ERR        (1 << 1 ) // 1=Lock error detected since last read
#define BCM5221_MLT3_ERR        (1 << 0 ) // 1=MLT3 code error detected since last read

// 100Base-X Rcv Error Counter Register
// Bit definitions: BCM5221_RCVCNT  
//

// 100Base-X False Carrier Counter Register
// Bit definitions: BCM5221_FCCNT
//

// 100Base-X Disconnect Counter Register
// Bit definitions: BCM5221_DSCCNT 
#define BCM5221_FAST_RX          (1 << 15) // 1=In extended FIFO mode, detect fast receive data
#define BCM5221_SLOW_RX          (1 << 14) // 1=In extended FIFO mode, detect slow receive data
//      Reserved                 13 to 0   // Read/ mofify /write to preserve

// Auxiliary Control /Status Register
// Bit definitions: BCM5221_AXCTTS
#define BCM5221_NO_JABBER        (1 << 15) // 1=Jabber function disabled
#define BCM5221_F_LINK           (1 << 14) // 1=Force 10BASE-T link pass
//      Reserved                 13 to 8   // Read/ mofify /write to preserve
#define BCM5221_HSQ              (1 << 7 ) //* These two bits define the squelch mode
#define BCM5221_LSQ              (1 << 6 ) //* of the 10BASE-T Carrier Sense mechanism
//      Edge Rate                5 to 4    // DAC output edge rate
#define BCM5221_ER_1ns       0x0
#define BCM5221_ER_2ns       0x1
#define BCM5221_ER_3ns       0x2
#define BCM5221_ER_4ns       0x3
#define BCM5221_AUTO_N           (1 << 3 ) // 1=Auto-negotiation activated
#define BCM5221_FSPEED           (1 << 2 ) // 1=Speed forced to 100BASE-X 0=Speed forced to 10BASE-T
#define BCM5221_SPEED            (1 << 1 ) // 1=100BASE-X 0=10BASE-T
#define BCM5221_DPLX             (1 << 0 ) // 1=Full-duplex active 0=Full-duplex not active

// Auxiliary Status Summary Register
// Bit definitions: BCM5221_AXSTSS
#define BCM5221_ANEG             (1 << 15) // 1=Auto-negotiation process completed
#define BCM5221_ANEG_A           (1 << 14) // 1=Auto-negotiation completed acknowledge state
#define BCM5221_ANEG_D           (1 << 13) // 1=Auto-negotiation acknowledge detected
#define BCM5221_ANEG_AD          (1 << 12) // 1=Auto-negotiation for link partner Ability Detect
#define BCM5221_ANEG_P           (1 << 11) // BCM5221 and link partner pause operation bit
//      Auto-Negotiation HCD     10 to 8   // 
//                        000 = No highest common denominator
//                        001 = 10BASE-T
//                        010 = 10BASE-T Full-Duplex
//                        011 = 100BASE-TX
//                        100 = 100BASE-T4
//                        101 = 100BASE-TX Full-Duplex
//                        11x = Undefined
#define BCM5221_PD_FAULT         (1 << 7 ) // 1=Parallel detection fault
#define BCM5221_R_FAULT          (1 << 6 ) // 1=Link partner remote fault
#define BCM5221_PAGE_R           (1 << 5 ) // 1=New page has been received
#define BCM5221_R_ANA            (1 << 4 ) // 1=Link partner is auto-negotiation capable
#define BCM5221_SPEED_           (1 << 3 ) // 1=100 Mbps 0=10 Mbps
#define BCM5221_LINK             (1 << 2 ) // 1=Link is up (link pass state)
#define BCM5221_E_ANA            (1 << 1 ) // 1=Auto-negotiation enabled
#define BCM5221_JABBER           (1 << 0 ) // 1=Jabber condition detected

// Interrupt Register 
// Bit definitions: BCM5221_INT
#define BCM5221_FDX_LED          (1 << 15) // 1=Enable Full-duplex LED output on G7/34
#define BCM5221_INT_E            (1 << 14) // Interrupt enable
//      Reserved                 13 to 12  // Read/ mofify /write to preserve
#define BCM5221_FDX_MASK         (1 << 11) // Full-Duplex interrupt mask
#define BCM5221_SPD_MASK         (1 << 10) // SPEED interrupt mask
#define BCM5221_LNK_MASK         (1 << 9 ) // LINK interrupt mask
#define BCM5221_INT_MASK         (1 << 8 ) // Master interrupt mask
//      Reserved                 7 to 4    // Read/ mofify /write to preserve
#define BCM5221_FDX_CNG          (1 << 3 ) // Duplex change interrupt
#define BCM5221_SPD_CNG          (1 << 2 ) // Speed change interrupt
#define BCM5221_LNK_CNG          (1 << 1 ) // Link change interrupt
#define BCM5221_INT_STTS         (1 << 0 ) // Interrupt status

// Auxiliary Mode 2 Register
// Bit definitions: BCM5221_AXM2
//      Reserved                 15 to 12  // Read/ mofify /write to preserve
#define BCM5221_E_DBC            (1 << 11) // 1=Enable 10BT Dribble Bit Correct
#define BCM5221_E_TRM            (1 << 10) // 1=Enable Token Ring Mode
#define BCM5221_E_HST            (1 << 9 ) // 1=Enable HSTR FIFO
//      Reserved                 8         // Read/ mofify /write to preserve
#define BCM5221_E_EMD            (1 << 7 ) // 1=Enable Block 10BT Echo Mode
#define BCM5221_E_TLM            (1 << 6 ) // 1=Enable Traffic Meter LED Mode
#define BCM5221_LED_ON           (1 << 5 ) // 1=ON  Activity LED Force On
//      Reserved                 4 to 3    // Read/ mofify /write to preserve
#define BCM5221_LED_ACT          (1 << 2 ) // 1=Enable Activity/Link LED Mode
#define BCM5221_E_QPD            (1 << 1 ) // 1=Enable Qual Parallel Detect Mode
//      Reserved                 0         // Read/ mofify /write to preserve

// 10Base-T Aux Error & General Status Register
// Bit definitions:  BCM5221_AXERTS
//      Reserved                 15 to 14  // Read/ mofify /write to preserve
#define BCM5221_MDIX_S           (1 << 13) // MDIX Status
#define BCM5221_MDIX_MS          (1 << 12) // MDIX Manual Swap 1=Force MDIX
#define BCM5221_MDIX_D           (1 << 11) // 1=Disable HP Auto-MDIX
#define BCM5221_MCE              (1 << 10) // 1=Manchester code error (10BASE-T)
#define BCM5221_EOFD             (1 << 9 ) // 1=EOF detection error (10BASE-T)
//      Reserved                 8 to 4    // Read/ mofify /write to preserve
#define BCM5221_A_ANA            (1 << 3 ) // 1=Auto-negotiation activated
#define BCM5221_SPEED_F          (1 << 2 ) // 1=Speed forced to 100BASE-X
#define BCM5221_SPEED_I          (1 << 1 ) // 1=100BASE-X
#define BCM5221_DPLX             (1 << 0 ) // 1=Full-duplex active

// Auxiliary Mode Register
// Bit definitions: BCM5221_AXMOD
//      Reserved                 15 to 5   // Read/ mofify /write to preserve
#define BCM5221_LED_D            (1 << 4 ) // 1=Disable XMT/RCV activity LED outputs
#define BCM5221_LLED_D           (1 << 3 ) // 1=Disable link LED output
//      Reserved                 2         // Read/ mofify /write to preserve
#define BCM5221_TXEN             (1 << 1 ) // 1=Enable block TXEN mode
//      Reserved                 0         // Read/ mofify /write to preserve

// Auxiliary Multi-PHY Register
// Bit definitions: BCM5221_AXMLT 
#define BCM5221_HCD_TX_FDX        (1 << 15) // 1=Auto-negotiation result is 100BASE-TX fullduplex
#define BCM5221_HCD_T4            (1 << 14) // 1=Auto-negotiation result is 100BASE-T4
#define BCM5221_HCD_TX            (1 << 13) // 1=Auto-negotiation result is 100BASE-TX
#define BCM5221_HCD_10BASE_T_FDX  (1 << 12) // 1=Auto-negotiation result is 10BASE-T full-duplex
#define BCM5221_HCD_10BASE_T      (1 << 11) // 1=Auto-negotiation result is 10BASE-T
//      Reserved                  10 to 9   // Read/ mofify /write to preserve
#define BCM5221_RAN               (1 << 8 ) // 1=Restart auto-negotiation process
#define BCM5221_ANC               (1 << 7 ) // 1=Auto-negotiation process completed
#define BCM5221_ACK_C             (1 << 6 ) // 1=Auto-negotiation acknowledge completed
#define BCM5221_ACK_D             (1 << 5 ) // 1=Auto-negotiation acknowledge detected
#define BCM5221_AD                (1 << 4 ) // Ability Detect 1=Auto-negotiation waiting for LP ability
#define BCM5221_SPRI              (1 << 3 ) // 1=Super isolate mode
//      Reserved                  2         // Read/ mofify /write to preserve
#define BCM5221_10SM              (1 << 1 ) // 1=Enable 10BASE-T serial mode
//      Reserved                  0         // Read/ mofify /write to preserve


// Broadcom Test Register
// Bit definitions: BCM5221_TEST
//      Reserved                 15 to 8   // Read/ mofify /write to preserve
#define BCM5221_SHDW_E           (1 << 7 ) // 1=Enable shadow registers
//      Reserved                 6 to 0    // Read/ mofify /write to preserve

// Auxiliary Mode 4 Register
// Bit definitions: BCM5221_AXM4
//      Reserved                 15 to 6   // Read/ mofify /write to preserve
//      Force LED                5 to 4    // 
//                        01 = Force all LED status to on 0 state
//                        10 = Force all LED status to off 1 state
//      Reserved                 3         // Read/ mofify /write to preserve
#define BCM5221_E_LPC            (1 << 2 ) // 1=Enables clock during low power modes
#define BCM5221_E_LPM            (1 << 1 ) // 1=Forces the 5221 to enter the low power mode
#define BCM5221_E_IDDQ           (1 << 0 ) // 1=Causes the BCM5221 to go to IDDQ mode

// Auxiliary Status 2 Register
// Bit definitions: BCM5221_AXS2
#define BCM5221_MLT3              (1 << 15) // 1=MLT3 Detected
//      Cable Length 100X         14 to 12  // The BCM5221 shows the cable length in 20 meters steps
//      ADC Peak Amplitude        11 to 6   // A to D peak amplitude seen
#define BCM5221_E_APD             (1 << 5 ) // 1=Enable auto power down mode
#define BCM5221_APD_SLP           (1 << 4 ) // 0=2.5 sec sleep, 1=5.0 sec sleep before wake-up
//      APD Wake-up Timer         3 to 0    // A to D peak amplitude seen

// Auxiliary Status 3 Register
// Bit definitions: BCM5221_AXS3
//      Noise                     15 to 8  // Current mean square error value, valid only if link is established
//      Reserved                  7 to 4   // Read/ mofify /write to preserve
//      FIFO Consumption          3 to 0   // Currently utilized number of nibbles in the receive FIFO

// Auxiliary Mode 3 Register
// Bit definitions: BCM5221_AXM3 
//      Reserved                  15 to 4   // Read/ mofify /write to preserve
//      FIFO Size Select          3 to 0    // Currently selected receive FIFO Size

// Auxiliary Status 4 Register
// Bit definitions: BCM5221_AXS4

#define BCM5221_RETRY_MAX             100000
#ifdef BBR2_HW  //666
#define BCM5221_PHY_ADDR              0
#endif
#ifdef BBR1_HW
#define BCM5221_PHY_ADDR              1
#endif

/*#define BCM5221_PHY_ADDR              1
#define LED3_OFF()
#define LED2_OFF()
#define LED1_OFF()
#define LED3_ON()
#define LED2_ON()
#define LED1_ON()
#define USART0_isByteReceived()    0
#define USART0_readByte()           
*/

#endif // #ifndef _BCM5221_DEFINE_H

