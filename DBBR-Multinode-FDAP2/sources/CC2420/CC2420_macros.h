//=========================================================================================
// Name:        CC2420.c
// Author:      Rares Ivan
// Date:        February 2008
// Description: CC2420 Driver
// Changes:     
// Revisions:   
//=========================================================================================
#ifdef BBR1_HW  
#ifndef CC2420_MACROS_H
#define CC2420_MACROS_H

#include "../global.h"
#include "../spi1.h"

//  SPIx Macros
//      cmd = command strobe
//      ra = register address
//      rv = register value


//=========================================================================================================
// Read, Write byte(s), word(s), comands
//=========================================================================================================

//-------------------------------------------------
// Macro: CC2420_MACRO_STROBE
//-------------------------------------------------
// attention !!, next macros are not {} protected so be carefull when use it
#define CC2420_MACRO_STROBE(cmd)                        SPI1_WriteByte(cmd);SPI1_WaitEndTx()


//-------------------------------------------------
// Macro: CC2420_MACRO_SETREG/CC2420_MACRO_GETREG
//-------------------------------------------------
#define CC2420_MACRO_SETREG(regAddr,regValue)     SPI1_WriteByte(regAddr); \
                                                  SPI1_WriteByte( (unsigned char)( (regValue) >> 8 ) ); \
                                                  SPI1_WriteByte( (unsigned char)( regValue ) ); \
                                                  SPI1_WaitEndTx()

#define CC2420_MACRO_GETREG(regAddr,regValue)     { unsigned int spiVal;            \
                                                    SPI1_WriteByte((regAddr)|0x40); \
                                                    SPI1_ReadByte( spiVal );     \
                                                    SPI1_ReadByte( regValue );   \
                                                    regValue |=  (spiVal << 8);     \
                                                  } 

//=========================================================================================================
// Specific comands/configuration
//=========================================================================================================

//-------------------------------------------------
// Macro: CC2420_MACRO_GET_STATUS
//-------------------------------------------------
#define CC2420_MACRO_GET_STATUS(rv) SPI1_ReadByte(rv)

#define CC2420_MACRO_GET_PA(pa) CC2420_MACRO_GETREG(CC2420_TXCTRL, pa); pa &= 0x001F
#define CC2420_MACRO_SET_PA(pa) CC2420_MACRO_SETREG(CC2420_TXCTRL, ((unsigned int)(0xA0E0 | (pa & 0x001F))) )

//-------------------------------------------------
// Macro: CC2420_MACRO_SET_RFCH
//  Note: LOCK_THR = 1 is also added (0x4000).
//-------------------------------------------------
//#define CC2420_MACRO_GET_RFCH_0TO15(ch)  CC2420_MACRO_GETREG(CC2420_FSCTRL, ch); ch = (ch - 0x4000 - 357) / 5
//#define CC2420_MACRO_GET_RFCH_11TO26(ch) CC2420_MACRO_GETREG(CC2420_FSCTRL, ch); ch = ((ch - 0x4000 - 357) / 5) + 11
#define CC2420_MACRO_SET_RFCH_0TO15(ch)  CC2420_MACRO_SETREG(CC2420_FSCTRL, ( (unsigned int)(ch * 5 + 357 + 0x4000) ) )
//#define CC2420_MACRO_SET_RFCH_11TO26(ch) SPI1_SETWORD(CC2420_FSCTRL, ( (unsigned int)((ch - 11) * 5 + 357 + 0x4000) ) )

//=========================================================================================================
// RF TX and RX
//=========================================================================================================
//#define CC2420_MACRO_WAIT4TX(status)                 do{ CC2420_MACRO_GET_STATUS(status); }while(status & (1L <<CC2420_TX_ACTIVE))
//#define CC2420_MACRO_ENABLE_RX_GET_STATUS(status)    CC2420_MACRO_STROBE(CC2420_SRXON); CC2420_MACRO_STROBE_GET_STATUS(CC2420_SFLUSHRX, status); CC2420_FIFOP_IRQON()
//#define CC2420_MACRO_ENABLE_RX()                     CC2420_MACRO_STROBE(CC2420_SRXON); CC2420_MACRO_STROBE(CC2420_SFLUSHRX); CC2420_FIFOP_IRQON()
//#define CC2420_MACRO_ENABLE_TX()                     CC2420_MACRO_STROBE(CC2420_STXON)
//#define CC2420_MACRO_ENABLE_TX_GET_STATUS(status)    CC2420_MACRO_STROBE(CC2420_STXON); CC2420_MACRO_GET_STATUS(status)
//#define CC2420_MACRO_DISABLE_RXTX_GET_STATUS(status) CC2420_WAIT4TX(status); CC2420_MACRO_STROBE_GET_STATUS(CC2420_SRFOFF, status); CC2420_FIFOP_IRQOFF()
//#define CC2420_MACRO_DISABLE_RXTX()                  CC2420_MACRO_STROBE(CC2420_SRFOFF); CC2420_FIFOP_IRQOFF()


//=========================================================================================================
// FIFO
//=========================================================================================================

//  FIFO access
//      p = pointer to the byte array to be read/written
//      c = the number of bytes to read/write
//      b = single data byte
//      a = address

//-------------------------------------------------
// Macro: CC2420 Macros to write the TXFIFO and the RXFIXO
//
// Note: TXFIFO is write-only, but it may be read back using RAM access.
//       RXFIFO is read/write but writing should be done only for debugging or security operations.
//       The Crystal Oscilator must be running when accesing the FIFOs.
//       When writing to TXFIFO, the status register is output for each new byte on SO. Use this to detect TXFIFO underflow.
//       FIFP and FIFOP pins only apply to RXFIFO. TXFIFO has its underflow flag in the status byte.
//
// !!! FIFO access can only be terminated by setting CSn pin high (release CC2420) once it has been started !!!
//
//-------------------------------------------------
#define CC2420_MACRO_WRITE_TXFIFO(p, c)            {SPI1_WRITEB( CC2420_TXFIFO ); SPI1_WriteBuff(p, c);}
#define CC2420_MACRO_WRITE_TXFIFO_WITH_FLUSH(p, c) {CC2420_MACRO_STROBE(CC2420_SFLUSHTX); CC2420_MACRO_WRITE_TXFIFO(p, c);}
#define CC2420_MACRO_WRITE_RXFIFO(p, c)            {SPI1_WRITEB( CC2420_RXFIFO ); SPI1_WriteBuff(p, c);}
#define CC2420_MACRO_WRITE_RXFIFO_WITH_FLUSH(p, c) {CC2420_MACRO_STROBE(CC2420_SFLUSHRX); CC2420_MACRO_WRITE_RXFIFO(p, c);}

//-------------------------------------------------
// Macro: CC2420 Macros to read the RXFIFO, TXFIFO
//-------------------------------------------------
//#define CC2420_MACRO_READ_RXFIFO(p, c, i)         { SPI1_WRITEB(CC2420_RXFIFO | 0x40); for (i = 0; i < (c); i++) { if ( !CC2420_FIFO_STATUS() ) break; SPI1_WRITEB_READB(0x00, ( (unsigned char*)(p) )[i] ); }
  #define CC2420_MACRO_READ_RXFIFO_NO_WAIT(p, c)  SPI1_READBYTES_FROM_ADDR(CC2420_RXFIFO, p, c)
//#define CC2420_MACRO_READ_RXFIFO_FRAME(p, i)      SPI1_WRITEB(CC2420_RXFIFO + 0x40); SPI1_WRITEB_READB(0x00, ( (unsigned char*)(p) )[0] ); for (i = 1; i < p[0] + 1; i++) { if ( !CC2420_FIFO_STATUS() ) break; SPI1_WRITEB_READB(0x00, ( (unsigned char*)(p) )[i] ); }
#define CC2420_MACRO_READ_RXFIFO_LEN(b)           SPI1_WriteByte( CC2420_RXFIFO|0x40 ); SPI1_ReadByte( b )

//#define CC2420_MACRO_READ_RXFIFO_GARBAGE(c, i)    SPI1_WRITEB(CC2420_RXFIFO + 0x40); for ( i = 0; ((i < (c)) && CC2420_FIFO_STATUS() ); i++) { SPI1_DUMMY_WRITEB_READB(); }

//-------------------------------------------------
// Macro: CC2420 Macros to FLUSH the RXFIFO and the TXFIFO
//-------------------------------------------------
#define CC2420_MACRO_FLUSH_RXFIFO() CC2420_MACRO_STROBE(CC2420_SFLUSHRX); CC2420_MACRO_STROBE(CC2420_SFLUSHRX)
#define CC2420_MACRO_FLUSH_TXFIFO() CC2420_MACRO_STROBE(CC2420_SFLUSHTX)



//=========================================================================================================
// RAM
//
// Note: CC2420 has 368 byte internal RAM accessible through SPI. Multiple readings/writtings possible by 
//       sending the 2 bytes address once (address is automatically incremented).
//       Crystal Oscilator must be running when accesing the RAM.
//       RAM is divided into 3 banks:
//         1. TXFIFO - bank 0, 128 bytes
//         2. RXFIFO - bank 1, 128 bytes
//         3. SECURITY - bank 2, 112 bytes
//
// !!! RAM memory (including registers) is retained during power down mode but NOT when power supply is 
//     turned off (ex. by disabling the VREG_EN voltage regulator) !!!
//
// !!! RAM access can only be terminated by setting CSn pin high (release CC2420) once it has been started !!!
//
//=========================================================================================================

//  CC2420 RAM access (big or little-endian order)
//      p = pointer to the variable to be written
//      a = the CC2420 RAM address
//      c = the number of bytes to write
//      n = counter variable which is used in for/while loops (unsigned char)

//-------------------------------------------------
// Macro: CC2420 Macros to write the RAM with data Little/Big Endian
//-------------------------------------------------

#define CC2420_MACRO_SET_WRITE_ADDR(a) SPI1_WriteByte(0x80 | ((a) & 0x7F)); SPI1_WriteByte(((a) >> 1) & 0xC0)

#define CC2420_MACRO_WRITE_RAM_LITTLE_E(p,a,c) { CC2420_MACRO_SET_WRITE_ADDR(a); SPI1_WriteBuff(p, c); }
#define CC2420_MACRO_WRITE_RAM_BIG_E(p,a,c)    { CC2420_MACRO_SET_WRITE_ADDR(a); SPI1_WriteBuffReverse(p, c); }
#define CC2420_MACRO_WRITE_NONCE(addr,b0,nonce,b14,b15) {\
                    CC2420_MACRO_SET_WRITE_ADDR(addr); \
                    SPI1_WriteByte(b15); \
                    SPI1_WriteByte(b14); \
                    SPI1_WriteBuffReverse(nonce,13); \
                    SPI1_WriteByte(b0); \
                    SPI1_WaitEndTx(); \
        }

//-------------------------------------------------
// Macro: CC2420_MACRO_READ_RAM_LITTLE_E (Little Endian)
//-------------------------------------------------
#define CC2420_MACRO_SET_READ_ADDR(a) SPI1_WriteByte(0x80 | ((a) & 0x7F)); SPI1_WriteByte((((a) >> 1) & 0xC0) | 0x20); SPI1_WaitEndTx()

#define CC2420_MACRO_READ_RAM_LITTLE_E(p,a,c) {CC2420_MACRO_SET_READ_ADDR(a); SPI1_ReadBuff(p, c); }
#define CC2420_MACRO_READ_RAM_BIG_E(p,a,c)    {CC2420_MACRO_SET_READ_ADDR(a); SPI1_ReadBuffReverse(p,c); }


//=========================================================================================================
// AES
//=========================================================================================================


//-------------------------------------------------
// Macro: CC2420 Macro to Read, Write Encryption KEY0
//   CC2420RAM_KEY0 address is 0x100 and is 128-bit long (to 0x10F)
//-------------------------------------------------
#define CC2420_MACRO_SET_ENCKEY0(p) CC2420_MACRO_WRITE_RAM_BIG_E(p, CC2420RAM_KEY0, 16)

//-------------------------------------------------
// Macro: CC2420 Macro to Read, Write Encryption KEY1
//   CC2420RAM_KEY1 address is 0x130 and is 128-bit long (to 0x13F)
//-------------------------------------------------
#define CC2420_MACRO_SET_ENCKEY1(p) CC2420_MACRO_WRITE_RAM_BIG_E(p, CC2420RAM_KEY1, 16)

//-------------------------------------------------
// Macro: CC2420 Macro to Select the Encryption Key for TX/RX/StandAlone modes
//-------------------------------------------------
//#define CC2420_MACRO_STANDALONE_SELECT_KEY0(rv)  CC2420_MACRO_REG_CLEARBIT(CC2420_SECCTRL0,rv, CC2420_SECCTRL0_SEC_SAKEYSEL)
//#define CC2420_MACRO_STANDALONE_SELECT_KEY1(rv)  CC2420_MACRO_REG_SETBIT(  CC2420_SECCTRL0,rv, CC2420_SECCTRL0_SEC_SAKEYSEL)
//#define CC2420_MACRO_RX_SELECT_KEY0(rv)          CC2420_MACRO_REG_CLEARBIT(CC2420_SECCTRL0,rv, CC2420_SECCTRL0_SEC_RXKEYSEL)
//#define CC2420_MACRO_RX_SELECT_KEY1(rv)          CC2420_MACRO_REG_SETBIT(  CC2420_SECCTRL0,rv, CC2420_SECCTRL0_SEC_RXKEYSEL)
//#define CC2420_MACRO_TX_SELECT_KEY0(rv)          CC2420_MACRO_REG_CLEARBIT(CC2420_SECCTRL0,rv, CC2420_SECCTRL0_SEC_TXKEYSEL)
//#define CC2420_MACRO_TX_SELECT_KEY1(rv)          CC2420_MACRO_REG_SETBIT(  CC2420_SECCTRL0,rv, CC2420_SECCTRL0_SEC_TXKEYSEL)

//#define CC2420_MACRO_STANDALONE_SELECT_KEY0() { g_unCC2420SecCtrl_0 &= ~CC2420_SECCTRL0_SEC_SAKEYSEL;  SPI1_SETWORD(CC2420_SECCTRL0,g_unCC2420SecCtrl_0); }
//#define CC2420_MACRO_STANDALONE_SELECT_KEY1() { g_unCC2420SecCtrl_0 |=  CC2420_SECCTRL0_SEC_SAKEYSEL;  SPI1_SETWORD(CC2420_SECCTRL0,g_unCC2420SecCtrl_0); }
//#define CC2420_MACRO_RX_SELECT_KEY0()         { g_unCC2420SecCtrl_0 &= ~CC2420_SECCTRL0_SEC_RXKEYSEL;  SPI1_SETWORD(CC2420_SECCTRL0,g_unCC2420SecCtrl_0); }
//#define CC2420_MACRO_RX_SELECT_KEY1()         { g_unCC2420SecCtrl_0 |=  CC2420_SECCTRL0_SEC_RXKEYSEL;  SPI1_SETWORD(CC2420_SECCTRL0,g_unCC2420SecCtrl_0); }
//#define CC2420_MACRO_TX_SELECT_KEY0()         { g_unCC2420SecCtrl_0 &= ~CC2420_SECCTRL0_SEC_TXKEYSEL;  SPI1_SETWORD(CC2420_SECCTRL0,g_unCC2420SecCtrl_0); }
//#define CC2420_MACRO_TX_SELECT_KEY1()         { g_unCC2420SecCtrl_0 |=  CC2420_SECCTRL0_SEC_TXKEYSEL;  SPI1_SETWORD(CC2420_SECCTRL0,g_unCC2420SecCtrl_0); }

#define CC2420_MACRO_SELECT_KEY0(kMask) { g_unCC2420SecCtrl_0 &= ~(kMask);  CC2420_MACRO_SETREG(CC2420_SECCTRL0,g_unCC2420SecCtrl_0); }
#define CC2420_MACRO_SELECT_KEY1(kMask) { g_unCC2420SecCtrl_0 |=  (kMask);  CC2420_MACRO_SETREG(CC2420_SECCTRL0,g_unCC2420SecCtrl_0); }

//-------------------------------------------------
// Macro: CC2420 Macro to Turn On/Off Encryption and to set Encryption mode
//-------------------------------------------------
#define CC2420_MACRO_SET_AES_MODE(aesMode)    { g_unCC2420SecCtrl_0 = (g_unCC2420SecCtrl_0 & 0xFFFC) | aesMode; CC2420_MACRO_SETREG(CC2420_SECCTRL0, g_unCC2420SecCtrl_0); }
#define CC2420_MACRO_GET_AES_MODE(rv)         { rv = g_unCC2420SecCtrl_0 & 0x0003;} 

//-------------------------------------------------
// Macro: CC2420 Macros to configure the AES RX/TX Plaintext number of bytes
//-------------------------------------------------
#define CC2420_MACRO_SET_AES_RX_PLAINTEXT(aesPlainLen) { g_unCC2420SecCtrl_1 = (g_unCC2420SecCtrl_1 & 0xFF00) | (aesPlainLen); CC2420_MACRO_SETREG(CC2420_SECCTRL1, g_unCC2420SecCtrl_1); }
#define CC2420_MACRO_SET_AES_TX_PLAINTEXT(aesPlainLen) { g_unCC2420SecCtrl_1 = (g_unCC2420SecCtrl_1 & 0xFF) | ((unsigned int)(aesPlainLen) << 8); CC2420_MACRO_SETREG(CC2420_SECCTRL1, g_unCC2420SecCtrl_1); }
#define CC2420_MACRO_GET_AES_RX_PLAINTEXT(rv)          { rv = g_unCC2420SecCtrl_1 & 0x00FF;}
#define CC2420_MACRO_GET_AES_TX_PLAINTEXT(rv)          { rv = g_unCC2420SecCtrl_1 >> 8;}

//-------------------------------------------------
// Macro: CC2420 Macros to set/get the RX/TX nonce
//-------------------------------------------------
#define CC2420_MACRO_SET_RX_NONCE(b0,nonce,b14,b15) CC2420_MACRO_WRITE_NONCE(CC2420RAM_RXNONCE,b0,nonce,b14,b15)
#define CC2420_MACRO_SET_TX_NONCE(b0,nonce,b14,b15) CC2420_MACRO_WRITE_NONCE(CC2420RAM_TXNONCE,b0,nonce,b14,b15)

#define CC2420_MACRO_GET_RX_NONCE(p) CC2420_MACRO_READ_RAM_BIG_E(p, CC2420RAM_RXNONCE, 16)
#define CC2420_MACRO_GET_TX_NONCE(p) CC2420_MACRO_READ_RAM_BIG_E(p, CC2420RAM_TXNONCE, 16)

//-------------------------------------------------
// Macro: CC2420 Macros to set/get MIC length
//-------------------------------------------------
#define CC2420_MACRO_GET_MIC_LEN(rv)
#define CC2420_MACRO_SET_MIC_LEN(rv) { g_unCC2420SecCtrl_0 = (g_unCC2420SecCtrl_0 & 0xFFE3) | (rv<<2); CC2420_MACRO_SETREG(CC2420_SECCTRL0, g_unCC2420SecCtrl_0); }

//-------------------------------------------------
// Macro: CC2420 Macro to:
//   Read/Write from/to the Stand-Alone Encryption Buffer (SABUF 16 bytes)
//-------------------------------------------------
#define CC2420_MACRO_GET_SABUF(p)  CC2420_MACRO_READ_RAM_LITTLE_E( p, CC2420RAM_SABUF, 16)
#define CC2420_MACRO_SET_SABUF(p)  CC2420_MACRO_WRITE_RAM_LITTLE_E(p, CC2420RAM_SABUF, 16)




//=========================================================================================================
// Other
//=========================================================================================================

//-------------------------------------------------
// Macro: CC2420 Macro Status Register flags
//-------------------------------------------------
#define CC2420_MACRO_WAIT4ENC()         CC2420_WaitUntilStatusBitClear( 1 << CC2420_ENC_BUSY )
#define CC2420_MACRO_WAIT4OSC()         CC2420_WaitUntilStatusBitSet( 1 << CC2420_XOSC16M_STABLE )

//-------------------------------------------------
// Macro: CC2420 Macro to Soft-Reset
//-------------------------------------------------
#define CC2420_MACRO_SOFT_RESET() CC2420_MACRO_SETREG(CC2420_MAIN, 0x0000); CC2420_MACRO_SETREG(CC2420_MAIN, 0xF800)


#endif
#endif