//=========================================================================================
// Name:        CC2520_macros.h
// Author:      Gourav Sharma
// Date:        December 2009
// Description: CC2520 Driver
// Changes:     
// Revisions:   
//=========================================================================================

#ifndef CC2520_MACROS_H
#define CC2520_MACROS_H

#include "../global.h"
#include "../spi1.h"
#include "../spi0.h"
#include "cc2520.h"

#ifdef BBR2_HW // Include this file only if hardware is BBR2 HW

//  SPIx Macros
//      cmd = command strobe
//      ra = register address
//      rv = register value


//=========================================================================================================
// Read, Write byte(s), word(s), comands
//=========================================================================================================
#define CC2520_SECCTRL0_NO_SECURITY      0x0000
#define CC2520_SECCTRL0_CBC_MAC          0x0001
#define CC2520_SECCTRL0_CTR              0x0002
#define CC2520_SECCTRL0_CCM              0x0003

#define CC2520_SECCTRL0_SEC_M_4_BYTES    BIN16(00000000,00000100)

#define CC2520_SECCTRL0_SEC_SAKEYSEL     BIT7
#define CC2520_SECCTRL0_SEC_CBC_HEAD     BIT8


// ==================================================================================================================
// ============================= Radio 1 Related Macro Definitions ==================================================
// ==================================================================================================================
//-------------------------------------------------
// Macro: CC2520_1_MACRO_STROBE
//-------------------------------------------------
// attention !!, next macros are not {} protected so be carefull when use it
#define CC2520_1_MACRO_STROBE(cmd)      CC2520_1_MACRO_RESELECT();\
                                        SPI1_WriteByte(cmd);\
                                        SPI1_WaitEndTx()
                                                        

//#define CC2520_MACRO_STROBE_GET_STATUS(cmd, statusByte) SPI1_WRITEB_READB(cmd, statusByte)

//-------------------------------------------------
// Macro: CC2520_1_MACRO_SETREG/CC2520_1_MACRO_GETREG
// Note: These Macros can only be used for addressses 
// less than 0x40 only
//-------------------------------------------------
#define CC2520_1_MACRO_SETREG(regAddr,regValue)   CC2520_1_MACRO_RESELECT();\
                                                  SPI1_WriteByte(CC2520_REGWR | regAddr); \
                                                  SPI1_WriteByte( (unsigned char)( regValue ) ); \
                                                  SPI1_WaitEndTx()

#define CC2520_1_MACRO_GETREG(regAddr,regValue)   CC2520_1_MACRO_RESELECT();\
                                                  SPI1_WriteByte(CC2520_REGRD | regAddr); \
                                                  SPI1_ReadByte( regValue );

//-------------------------------------------------
// Macro: CC2520_1_MACRO_SETMEM/CC2520_1_MACRO_GETMEM
// Note: These call are meant to be used only for 
// addresses in the range 0x000 to ox0XX. It cannot 
// be used for whole Radio memory.
//-------------------------------------------------
#define CC2520_1_MACRO_SETMEM(memAddr,value)      CC2520_1_MACRO_RESELECT();\
                                                  SPI1_WriteByte(CC2520_MEMWR); \
                                                  SPI1_WriteByte(memAddr); \
                                                  SPI1_WriteByte( (unsigned char)( value ) ); \
                                                  SPI1_WaitEndTx()


#define CC2520_1_MACRO_GETMEM(memAddr,regValue)   CC2520_1_MACRO_RESELECT();\
                                                  SPI1_WriteByte(CC2520_MEMRD); \
                                                  SPI1_WriteByte(memAddr); \
                                                  SPI1_ReadByte( regValue );

//=========================================================================================================
// Specific comands/configuration
//=========================================================================================================
#define CC2520_1_MACRO_SET_RFCH_0TO15(ch)  CC2520_1_MACRO_SETREG(CC2520_FREQCTRL, ((unsigned char)(ch * 5 + 11) & 0x7F))
#define CC2520_1_MACRO_GET_RFCH_0TO15(ch)  CC2520_1_MACRO_GETREG(CC2520_FREQCTRL, ch);  
//-------------------------------------------------
// Macro: CC2520_1_MACRO_GET_STATUS
//-------------------------------------------------
#define CC2520_1_MACRO_GET_STATUS(rv) SPI1_ReadByte(rv)

#define CC2520_1_MACRO_GET_PA(pa) CC2520_1_MACRO_GETMEM(CC2520_TXPOWER, pa)
#define CC2520_1_MACRO_SET_PA(pa) CC2520_1_MACRO_SETMEM(CC2520_TXPOWER, (unsigned char)(pa & 0xFF))

//-------------------------------------------------
// Macro: CC2520 Macros to FLUSH the RXFIFO and the TXFIFO
//-------------------------------------------------
#define CC2520_1_MACRO_FLUSH_RXFIFO() CC2520_1_MACRO_STROBE(CC2520_SFLUSHRX); CC2520_1_MACRO_STROBE(CC2520_SFLUSHRX)
#define CC2520_1_MACRO_FLUSH_TXFIFO() CC2520_1_MACRO_STROBE(CC2520_SFLUSHTX); CC2520_1_MACRO_STROBE(CC2520_SFLUSHTX);

//-------------------------------------------------
// Macro: CC2520 Macros to get the length of receive FIFO
//-------------------------------------------------
#define CC2520_1_MACRO_READ_RXFIFO_LEN(b)   CC2520_1_MACRO_GETREG(CC2520_RXFIRST, b)

//-------------------------------------------------
// Macro: CC2520 Macros to Read and Write RAM data Little/Big Endian
// Note: Address should be a valid address to call this MACRO
//-------------------------------------------------
#define CC2520_1_MACRO_READ_RAM_LITTLE_E(ptr,addr,cnt) {SPI1_WriteByte((unsigned char)(CC2520_MEMRD | ((0x0F00 & addr)>>8))); \
                                                        SPI1_WriteByte((unsigned char)(addr & 0x00FF)); \
                                                        SPI1_ReadBuff(ptr, cnt); \
                                                        }

#define CC2520_1_MACRO_READ_RAM_BIG_E(ptr,addr,cnt)    {SPI1_WriteByte((unsigned char)(CC2520_MEMRD | ((0x0F00 & addr)>>8))); \
                                                        SPI1_WriteByte((unsigned char)(addr & 0x00FF)); \
                                                        SPI1_ReadBuffReverse(ptr,cnt); }


#define CC2520_1_MACRO_WRITE_RAM_LITTLE_E(ptr,addr,cnt) {SPI1_WriteByte((unsigned char)(CC2520_MEMWR | ((0x0F00 & addr)>>8))); \
                                                         SPI1_WriteByte((unsigned char)(addr & 0x00FF)); \
                                                         SPI1_WriteBuff(ptr, cnt); }

#define CC2520_1_MACRO_WRITE_RAM_BIG_E(ptr,addr,cnt)   {SPI1_WriteByte((unsigned char)(CC2520_MEMWR | ((0x0F00 & addr)>>8))); \
                                                        SPI1_WriteByte((unsigned char)(addr & 0x00FF)); \
                                                        SPI1_WriteBuffReverse(ptr, cnt); }

//--------------------------------------------------------
// Macro: CC2520 Macro to write data in to the Tx Buffer
//--------------------------------------------------------
#define CC2520_1_LOAD_TX_BUFFER(msgptr, len)   { CC2520_1_MACRO_RESELECT();\
                                                 SPI1_WriteByte((unsigned char)(CC2520_TXBUF));\
                                                 SPI1_WriteByte((unsigned char)(len + 2));  \
                                                 SPI1_WriteBuff(msgptr, len);\
                                               }

#define CC2520_1_LOAD_RX_BUFFER(msgptr, len)   { CC2520_1_MACRO_RESELECT();\
                                                 SPI1_WriteByte((unsigned char)(CC2520_MEMWR | ((0x0F00 & CC2520_RX_DECRYP_BUFF)>>8))); \
                                                 SPI1_WriteByte((unsigned char)(CC2520_RX_DECRYP_BUFF & 0x00FF)); \
                                                 SPI1_WriteBuff(msgptr, len); \
                                               }

//-------------------------------------------------
// Macro: CC2520 Macro to:
//   Read/Write from/to the Stand-Alone Encryption Buffer (SABUF 16 bytes)
//-------------------------------------------------
#define CC2520_1_MACRO_GET_SABUF(p)  CC2520_1_MACRO_READ_RAM_LITTLE_E( p, CC2520_SA_BUFF, 16)
#define CC2520_1_MACRO_SET_SABUF(p)  CC2520_1_MACRO_WRITE_RAM_LITTLE_E(p, CC2520_SA_BUFF, 16)

//------------------------------------------------------------------------
// Macro: CC2520 Macro to Write Encryption KEY0 and  KEY1
//   CC2520RAM_KEY0 address is user defined and is 128-bit long
//------------------------------------------------------------------------

#define CC2520_1_MACRO_SET_ENCKEY0(p)    CC2520_1_MACRO_WRITE_RAM_BIG_E(p, CC2520_RAM_KEY0, 16)
#define CC2520_1_MACRO_SET_ENCKEY1(p)    CC2520_1_MACRO_WRITE_RAM_BIG_E(p, CC2520_RAM_KEY1, 16)


#define CC2520_1_MACRO_WRITE_NONCE(addr,b0,nonce,b14,b15) {\
                    SPI1_WriteByte((unsigned char)(CC2520_MEMWR | ((0x0F00 & addr)>>8))); \
                    SPI1_WriteByte((unsigned char)(addr & 0x00FF)); \
                    SPI1_WriteByte((unsigned char)b15); \
                    SPI1_WriteByte((unsigned char)b14); \
                    SPI1_WriteBuffReverse(nonce,13); \
                    SPI1_WriteByte((unsigned char)b0); \
                    SPI1_WaitEndTx(); \
                    }

//-------------------------------------------------
// Macro: CC2520 Macros to set/get the RX/TX nonce
//-------------------------------------------------
#define CC2520_1_MACRO_SET_RX_NONCE(b0,nonce,b14,b15)  CC2520_1_MACRO_WRITE_NONCE(CC2520_RX_NONCE,b0,nonce,b14,b15)
#define CC2520_1_MACRO_SET_TX_NONCE(b0,nonce,b14,b15)  CC2520_1_MACRO_WRITE_NONCE(CC2520_TX_NONCE,b0,nonce,b14,b15)

#define CC2520_1_MACRO_GET_RX_NONCE(p) CC2520_1_MACRO_READ_RAM_BIG_E(p, CC2520_RX_NONCE, 16)
#define CC2520_1_MACRO_GET_TX_NONCE(p) CC2520_1_MACRO_READ_RAM_BIG_E(p, CC2520_TX_NONCE, 16)

//-------------------------------------------------
// Macro: CC2520 Macros to store/read the AES RX/TX Plaintext number of bytes
//-------------------------------------------------
#define CC2520_1_MACRO_SET_AES_RX_PLAINTEXT(aesPlainLen) { g_unCC2520SecCtrl_1 = (g_unCC2520SecCtrl_1 & 0xFF00) | (aesPlainLen); }
#define CC2520_1_MACRO_SET_AES_TX_PLAINTEXT(aesPlainLen) { g_unCC2520SecCtrl_1 = (g_unCC2520SecCtrl_1 & 0xFF) | ((unsigned int)(aesPlainLen) << 8);}
#define CC2520_1_MACRO_GET_AES_RX_PLAINTEXT(rv)          { rv = g_unCC2520SecCtrl_1 & 0x00FF;}
#define CC2520_1_MACRO_GET_AES_TX_PLAINTEXT(rv)          { rv = g_unCC2520SecCtrl_1 >> 8;}

//--------------------------------------------------------------------------
// Macro: CC2520 Macros to Encrypt and Decrypt various buffers 
// i.e. Rx, Tx and StandAlone buffer
// Arguments:
// cmd     -> Encryption Command with Priority Bit
// key     -> Address of the key
// c       -> No. of byets to be used for encryption
// nonce   -> Address where the Nonce is located
// inaddr  -> Start Address of the data to be Encrypted + Authenticated
// outaddr -> Address where to store the Encrypted + Authenticated data
// f       -> Number of bytes to be Authenticated
// m       -> Number of MIC Bytes to be generated
//--------------------------------------------------------------------------
#define CC2520_1_MACRO_CRYPT(cmd, key, c, nonce, inaddr, outaddr, f, m)     { SPI1_WriteByte(cmd);      \
                                                                              SPI1_WriteByte(key);      \
                                                                              SPI1_WriteByte(c&0x7F); \
                                                                              SPI1_WriteByte(nonce);    \
                                                                              SPI1_WriteByte((uint8)((((inaddr) & 0x0F00)>>4) | (((outaddr) & 0x0F00)>>8)));\
                                                                              SPI1_WriteByte((uint8)((inaddr) & 0x00FF));   \
                                                                              SPI1_WriteByte((uint8)((outaddr) & 0x00FF));  \
                                                                              SPI1_WriteByte(f & 0x7F); \
                                                                              SPI1_WriteByte(m & 0x03); \
                                                                              SPI1_WaitEndTx(); }

#define CC2520_1_MACRO_DECRYPT_RXFIFO(EncLen, MicSize)   {  unsigned char plainTxt; \
         CC2520_1_MACRO_GET_AES_RX_PLAINTEXT(plainTxt); \
         CC2520_1_MACRO_CRYPT(CC2520_UCCM_HIGH_P, (uint8)(CC2520_RAM_KEY0>>4), EncLen, (uint8)(CC2520_RX_NONCE>>4), CC2520_RAM_RXBUF+1, 0,plainTxt, MicSize); \
                                          }        

#define CC2520_1_MACRO_DECRYPT_ACKFIFO()   {  unsigned char plainTxt; \
         CC2520_1_MACRO_GET_AES_RX_PLAINTEXT(plainTxt); \
         CC2520_1_MACRO_CRYPT(CC2520_UCCM_HIGH_P, (uint8)(CC2520_RAM_KEY0>>4), 0, (uint8)(CC2520_RX_NONCE>>4), CC2520_RX_DECRYP_BUFF, 0,plainTxt,1); \
                                          }

#define CC2520_1_MACRO_ENCRYPT_TXFIFO(encLen,MicSize)   {  unsigned char plainTxt;\
                                           CC2520_1_MACRO_GET_AES_TX_PLAINTEXT(plainTxt);\
                                           CC2520_1_MACRO_CRYPT(CC2520_CCM_HIGH_P, \
                                                             (uint8)(CC2520_RAM_KEY0>>4),\
                                                              encLen, (uint8)(CC2520_TX_NONCE>>4), \
                                                              CC2520_RAM_TXBUF + 1, 0, \
                                                              plainTxt, \
                                                              MicSize);\
                                         }

#define CC2520_1_MACRO_ENCRYPT_SA_BUFFER() { SPI1_WriteByte(CC2520_ECBO_HIGH_P);\
                                             SPI1_WriteByte((uint8)(CC2520_RAM_KEY1>>4));\
                                             SPI1_WriteByte((uint8)((CC2520_SA_BUFF & 0x0F00)>>8));\
                                             SPI1_WriteByte((uint8)(CC2520_SA_BUFF & 0x00FF));\
                                             SPI1_WaitEndTx(); \
                                           }

//-------------------------------------------------------------
// Macro: CC2520 Macro to set and get current Encryption mode
//-------------------------------------------------------------
#define CC2520_1_MACRO_SET_AES_MODE(aesMode)    { g_unCC2520SecCtrl_0 = (g_unCC2520SecCtrl_0 & 0xFFFC) | aesMode; }
#define CC2520_1_MACRO_GET_AES_MODE(rv)         { rv = g_unCC2520SecCtrl_0 & 0x0003;} 
// ------------------------------------------------------------------------------------------------------------------

// ==================================================================================================================
// ============================= Radio 2 Related Macro Definitions ==================================================
// ==================================================================================================================
//-------------------------------------------------
// Macro: CC2520_2_MACRO_STROBE
//-------------------------------------------------
// attention !!, next macros are not {} protected so be carefull when use it
#define CC2520_2_MACRO_STROBE(cmd)      CC2520_2_MACRO_RESELECT();\
                                        SPI0_Radio2_WriteByte(cmd);\
                                        SPI0_WaitEndTx()
                                                        
//-------------------------------------------------
// Macro: CC2520_2_MACRO_SETREG/CC2520_2_MACRO_GETREG
// Note: These Macros can only be used for addressses 
// less than 0x40 only
//-------------------------------------------------
#define CC2520_2_MACRO_SETREG(regAddr,regValue)   CC2520_2_MACRO_RESELECT();\
                                                  SPI0_Radio2_WriteByte(CC2520_REGWR | regAddr); \
                                                  SPI0_Radio2_WriteByte( (unsigned char)( regValue ) ); \
                                                  SPI0_WaitEndTx()

#define CC2520_2_MACRO_GETREG(regAddr,regValue)   CC2520_2_MACRO_RESELECT();\
                                                  SPI0_Radio2_WriteByte(CC2520_REGRD | regAddr); \
                                                  SPI0_Radio2_ReadByte( regValue );

//-------------------------------------------------
// Macro: CC2520_2_MACRO_SETMEM/CC2520_2_MACRO_GETMEM
// Note: These call are meant to be used only for 
// addresses in the range 0x000 to ox0XX. It cannot 
// be used for whole Radio memory.
//-------------------------------------------------
#define CC2520_2_MACRO_SETMEM(memAddr,value)      CC2520_2_MACRO_RESELECT();\
                                                  SPI0_Radio2_WriteByte(CC2520_MEMWR); \
                                                  SPI0_Radio2_WriteByte(memAddr); \
                                                  SPI0_Radio2_WriteByte( (unsigned char)( value ) ); \
                                                  SPI0_WaitEndTx()


#define CC2520_2_MACRO_GETMEM(memAddr,regValue)   CC2520_2_MACRO_RESELECT();\
                                                  SPI0_Radio2_WriteByte(CC2520_MEMRD); \
                                                  SPI0_Radio2_WriteByte(memAddr); \
                                                  SPI0_Radio2_ReadByte( regValue );

//=========================================================================================================
// Specific comands/configuration
//=========================================================================================================
#define CC2520_2_MACRO_SET_RFCH_0TO15(ch)  CC2520_2_MACRO_SETREG(CC2520_FREQCTRL, ((unsigned char)(ch * 5 + 11) & 0x7F))
#define CC2520_2_MACRO_GET_RFCH_0TO15(ch)  CC2520_2_MACRO_GETREG(CC2520_FREQCTRL, ch);  
//-------------------------------------------------
// Macro: CC2520_2_MACRO_GET_STATUS
//-------------------------------------------------
#define CC2520_2_MACRO_GET_STATUS(rv) SPI0_Radio2_ReadByte(rv)

#define CC2520_2_MACRO_GET_PA(pa) CC2520_2_MACRO_GETMEM(CC2520_TXPOWER, pa)
#define CC2520_2_MACRO_SET_PA(pa) CC2520_2_MACRO_SETMEM(CC2520_TXPOWER, (unsigned char)(pa & 0xFF))

//-------------------------------------------------
// Macro: CC2520 Macros to FLUSH the RXFIFO and the TXFIFO
//-------------------------------------------------
#define CC2520_2_MACRO_FLUSH_RXFIFO() CC2520_2_MACRO_STROBE(CC2520_SFLUSHRX); CC2520_2_MACRO_STROBE(CC2520_SFLUSHRX)
#define CC2520_2_MACRO_FLUSH_TXFIFO() CC2520_2_MACRO_STROBE(CC2520_SFLUSHTX); CC2520_2_MACRO_STROBE(CC2520_SFLUSHTX);

//-------------------------------------------------
// Macro: CC2520 Macros to get the length of receive FIFO
//-------------------------------------------------
#define CC2520_2_MACRO_READ_RXFIFO_LEN(b)   CC2520_2_MACRO_GETREG(CC2520_RXFIRST, b)

//-------------------------------------------------
// Macro: CC2520 Macros to Read and Write RAM data Little/Big Endian
// Note: Address should be a valid address to call this MACRO
//-------------------------------------------------
#define CC2520_2_MACRO_READ_RAM_LITTLE_E(ptr,addr,cnt) {SPI0_Radio2_WriteByte((unsigned char)(CC2520_MEMRD | ((0x0F00 & addr)>>8))); \
                                                        SPI0_Radio2_WriteByte((unsigned char)(addr & 0x00FF)); \
                                                        SPI0_Radio2_ReadBuff(ptr, cnt); \
                                                        }

#define CC2520_2_MACRO_READ_RAM_BIG_E(ptr,addr,cnt)    {SPI0_Radio2_WriteByte((unsigned char)(CC2520_MEMRD | ((0x0F00 & addr)>>8))); \
                                                        SPI0_Radio2_WriteByte((unsigned char)(addr & 0x00FF)); \
                                                        SPI0_Radio2_ReadBuffReverse(ptr,cnt); }


#define CC2520_2_MACRO_WRITE_RAM_LITTLE_E(ptr,addr,cnt) {SPI0_Radio2_WriteByte((unsigned char)(CC2520_MEMWR | ((0x0F00 & addr)>>8))); \
                                                         SPI0_Radio2_WriteByte((unsigned char)(addr & 0x00FF)); \
                                                         SPI0_Radio2_WriteBuff(ptr, cnt); }

#define CC2520_2_MACRO_WRITE_RAM_BIG_E(ptr,addr,cnt)   {SPI0_Radio2_WriteByte((unsigned char)(CC2520_MEMWR | ((0x0F00 & addr)>>8))); \
                                                        SPI0_Radio2_WriteByte((unsigned char)(addr & 0x00FF)); \
                                                        SPI0_Radio2_WriteBuffReverse(ptr, cnt); }

//--------------------------------------------------------
// Macro: CC2520 Macro to write data in to the Tx Buffer
//--------------------------------------------------------
#define CC2520_2_LOAD_TX_BUFFER(msgptr, len)   { CC2520_2_MACRO_RESELECT();\
                                                 SPI0_Radio2_WriteByte((unsigned char)(CC2520_TXBUF));\
                                                 SPI0_Radio2_WriteByte((unsigned char)(len + 2));  \
                                                 SPI0_Radio2_WriteBuff(msgptr, len);\
                                               }

#define CC2520_2_LOAD_RX_BUFFER(msgptr, len)   { CC2520_2_MACRO_RESELECT();\
                                                 SPI0_Radio2_WriteByte((unsigned char)(CC2520_MEMWR | ((0x0F00 & CC2520_RX_DECRYP_BUFF)>>8))); \
                                                 SPI0_Radio2_WriteByte((unsigned char)(CC2520_RX_DECRYP_BUFF & 0x00FF)); \
                                                 SPI0_Radio2_WriteBuff(msgptr, len); \
                                               }

//-------------------------------------------------
// Macro: CC2520 Macro to:
//   Read/Write from/to the Stand-Alone Encryption Buffer (SABUF 16 bytes)
//-------------------------------------------------
#define CC2520_2_MACRO_GET_SABUF(p)  CC2520_2_MACRO_READ_RAM_LITTLE_E( p, CC2520_SA_BUFF, 16)
#define CC2520_2_MACRO_SET_SABUF(p)  CC2520_2_MACRO_WRITE_RAM_LITTLE_E(p, CC2520_SA_BUFF, 16)

//------------------------------------------------------------------------
// Macro: CC2520 Macro to Write Encryption KEY0 and  KEY1
//   CC2520RAM_KEY0 address is user defined and is 128-bit long
//------------------------------------------------------------------------

#define CC2520_2_MACRO_SET_ENCKEY0(p)    CC2520_2_MACRO_WRITE_RAM_BIG_E(p, CC2520_RAM_KEY0, 16)
#define CC2520_2_MACRO_SET_ENCKEY1(p)    CC2520_2_MACRO_WRITE_RAM_BIG_E(p, CC2520_RAM_KEY1, 16)


#define CC2520_2_MACRO_WRITE_NONCE(addr,b0,nonce,b14,b15) {\
                    SPI0_Radio2_WriteByte((unsigned char)(CC2520_MEMWR | ((0x0F00 & addr)>>8))); \
                    SPI0_Radio2_WriteByte((unsigned char)(addr & 0x00FF)); \
                    SPI0_Radio2_WriteByte((unsigned char)b15); \
                    SPI0_Radio2_WriteByte((unsigned char)b14); \
                    SPI0_Radio2_WriteBuffReverse(nonce,13); \
                    SPI0_Radio2_WriteByte((unsigned char)b0); \
                    SPI0_WaitEndTx(); \
                    }

//-------------------------------------------------
// Macro: CC2520 Macros to set/get the RX/TX nonce
//-------------------------------------------------
#define CC2520_2_MACRO_SET_RX_NONCE(b0,nonce,b14,b15)  CC2520_2_MACRO_WRITE_NONCE(CC2520_RX_NONCE,b0,nonce,b14,b15)
#define CC2520_2_MACRO_SET_TX_NONCE(b0,nonce,b14,b15)  CC2520_2_MACRO_WRITE_NONCE(CC2520_TX_NONCE,b0,nonce,b14,b15)

#define CC2520_2_MACRO_GET_RX_NONCE(p) CC2520_2_MACRO_READ_RAM_BIG_E(p, CC2520_RX_NONCE, 16)
#define CC2520_2_MACRO_GET_TX_NONCE(p) CC2520_2_MACRO_READ_RAM_BIG_E(p, CC2520_TX_NONCE, 16)

//-------------------------------------------------
// Macro: CC2520 Macros to store/read the AES RX/TX Plaintext number of bytes
//-------------------------------------------------
#define CC2520_2_MACRO_SET_AES_RX_PLAINTEXT(aesPlainLen) { g_unCC2520SecCtrl_1 = (g_unCC2520SecCtrl_1 & 0xFF00) | (aesPlainLen); }
#define CC2520_2_MACRO_SET_AES_TX_PLAINTEXT(aesPlainLen) { g_unCC2520SecCtrl_1 = (g_unCC2520SecCtrl_1 & 0xFF) | ((unsigned int)(aesPlainLen) << 8);}
#define CC2520_2_MACRO_GET_AES_RX_PLAINTEXT(rv)          { rv = g_unCC2520SecCtrl_1 & 0x00FF;}
#define CC2520_2_MACRO_GET_AES_TX_PLAINTEXT(rv)          { rv = g_unCC2520SecCtrl_1 >> 8;}

//--------------------------------------------------------------------------
// Macro: CC2520 Macros to Encrypt and Decrypt various buffers 
// i.e. Rx, Tx and StandAlone buffer
// Arguments:
// cmd     -> Encryption Command with Priority Bit
// key     -> Address of the key
// c       -> No. of byets to be used for encryption
// nonce   -> Address where the Nonce is located
// inaddr  -> Start Address of the data to be Encrypted + Authenticated
// outaddr -> Address where to store the Encrypted + Authenticated data
// f       -> Number of bytes to be Authenticated
// m       -> Number of MIC Bytes to be generated
//--------------------------------------------------------------------------
#define CC2520_2_MACRO_CRYPT(cmd, key, c, nonce, inaddr, outaddr, f, m)     { SPI0_Radio2_WriteByte(cmd);      \
                                                                              SPI0_Radio2_WriteByte(key);      \
                                                                              SPI0_Radio2_WriteByte(c&0x7F); \
                                                                              SPI0_Radio2_WriteByte(nonce);    \
                                                                              SPI0_Radio2_WriteByte((uint8)((((inaddr) & 0x0F00)>>4) | (((outaddr) & 0x0F00)>>8)));\
                                                                              SPI0_Radio2_WriteByte((uint8)((inaddr) & 0x00FF));   \
                                                                              SPI0_Radio2_WriteByte((uint8)((outaddr) & 0x00FF));  \
                                                                              SPI0_Radio2_WriteByte(f & 0x7F); \
                                                                              SPI0_Radio2_WriteByte(m & 0x03); \
                                                                              SPI0_WaitEndTx(); }

#define CC2520_2_MACRO_DECRYPT_RXFIFO(EncLen, MicSize)   {  unsigned char plainTxt; \
         CC2520_2_MACRO_GET_AES_RX_PLAINTEXT(plainTxt); \
         CC2520_2_MACRO_CRYPT(CC2520_UCCM_HIGH_P, (uint8)(CC2520_RAM_KEY0>>4), EncLen, (uint8)(CC2520_RX_NONCE>>4), CC2520_RAM_RXBUF+1, 0,plainTxt, MicSize); \
                                          }        

#define CC2520_2_MACRO_DECRYPT_ACKFIFO()   {  unsigned char plainTxt; \
         CC2520_2_MACRO_GET_AES_RX_PLAINTEXT(plainTxt); \
         CC2520_2_MACRO_CRYPT(CC2520_UCCM_HIGH_P, (uint8)(CC2520_RAM_KEY0>>4), 0, (uint8)(CC2520_RX_NONCE>>4), CC2520_RX_DECRYP_BUFF, 0,plainTxt,1); \
                                          }        

#define CC2520_2_MACRO_ENCRYPT_TXFIFO(encLen,MicSize)   {  unsigned char plainTxt;\
                                           CC2520_2_MACRO_GET_AES_TX_PLAINTEXT(plainTxt);\
                                           CC2520_2_MACRO_CRYPT(CC2520_CCM_HIGH_P, \
                                                             (uint8)(CC2520_RAM_KEY0>>4),\
                                                              encLen, (uint8)(CC2520_TX_NONCE>>4), \
                                                              CC2520_RAM_TXBUF + 1, 0, \
                                                              plainTxt, \
                                                              MicSize);\
                                         }

#define CC2520_2_MACRO_ENCRYPT_SA_BUFFER() { SPI0_Radio2_WriteByte(CC2520_ECBO_HIGH_P);\
                                             SPI0_Radio2_WriteByte((uint8)(CC2520_RAM_KEY1>>4));\
                                             SPI0_Radio2_WriteByte((uint8)((CC2520_SA_BUFF & 0x0F00)>>8));\
                                             SPI0_Radio2_WriteByte((uint8)(CC2520_SA_BUFF & 0x00FF));\
                                             SPI0_WaitEndTx(); \
                                           }

//-------------------------------------------------------------
// Macro: CC2520 Macro to set and get current Encryption mode
//-------------------------------------------------------------
#define CC2520_2_MACRO_SET_AES_MODE(aesMode)    { g_unCC2520SecCtrl_0 = (g_unCC2520SecCtrl_0 & 0xFFFC) | aesMode; }
#define CC2520_2_MACRO_GET_AES_MODE(rv)         { rv = g_unCC2520SecCtrl_0 & 0x0003;} 
//------------------------------------------------------------------------------------------------------------------------

#endif // BBR2_HW

#endif
