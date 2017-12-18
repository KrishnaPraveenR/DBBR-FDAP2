/*************************************************************************
* File: global.h
* Author: Nivis LLC
* Global stuff
*************************************************************************/

#ifndef _NIVIS_GLOBAL_H_
#define _NIVIS_GLOBAL_H_

#include "at91sam7x512.h"
#include "typedef.h"

#include "logger/logger.h"

//=============================================================================

// first digit is major version number, last two is minor version number (v 0.40)
#define VERSION               "0001"

// build number (used for internal releases only)
#define BUILD                 "0001"

//=============================================================================

#define OSCILLATOR_FREQUENCY    32768
//#define MASTER_CLOCK            (4L * 1024*1024) // 4MHz
#define MASTER_CLOCK            (8L * 1024*1024) // 8MHz
#define MILLISECOND2LOOP(x)     ((unsigned int)((float)x*(float)(MASTER_CLOCK/4000)))

#define DELAY125ms              65535

#define ACTIVED     1
#define NONE        0

//=============================================================================
#ifdef LOW_POWER_DEVICE
  #define PREDICTIVE_SCHEDULE_LINK
  #define _BATTERY_OPERATED_DEVICE_
#endif

#define WDG_MODE             ACTIVED
#define I2C_MODE             NONE
#define USART0_MODE          NONE
#define USART1_MODE          ACTIVED  // Used for [Backbone] or [GE Demo Valprobe 9600 8N1] or ..
#define SPI0_MODE            ACTIVED  // Used for CC2420

#define SPI1_MODE            ACTIVED  // SPI access to EEPROM and acquisition board F449
#define RTC_MODE             ACTIVED
#define BCM_MODE             ACTIVED
#define ADC12_MODE           NONE
#define DAC12_MODE           NONE     // PA can be controlled with DAC12's CH 0. Make sure PA_DAC_MODE is also ACTIVED

// ACTIVED = Control the RF Output Power by controlling the PA with DAC12 CH 0 (recommended)
// NONE = Control the RF Output Power by simply turning OFF or ON the PA (PA ON = full power)
#define PA_DAC_MODE          NONE


#define DEBUG_MODE           NONE  // Debug messages are printed to serial port
//#define DEBUG_MODE           1  // Debug output pins actived
//#define DEBUG_MODE           2  // Debug output pins actived
//#define DEBUG_MODE           3  // Debug output pins actived
//#define DEBUG_MODE           4  // Debug output pins actived
//#define DEBUG_MODE           5  // Debug output pins actived

//=============================================================================


#define MAX(X, Y) ( (X) < (Y) ? (Y) : (X) )

#define IRQ_ON()  __enable_interrupt()
#define IRQ_OFF() __disable_interrupt()

#define Load2BytesInNO(p_pDst,p_pSrc) { \
                             ((unsigned char *)(p_pDst))[0] = ((unsigned char *)(p_pSrc))[1];\
                             ((unsigned char *)(p_pDst))[1] = ((unsigned char *)(p_pSrc))[0];\
                             }

#define Load4BytesInNO(p_pDst,p_pSrc) { \
                             ((unsigned char *)(p_pDst))[0] = ((unsigned char *)(p_pSrc))[3];\
                             ((unsigned char *)(p_pDst))[1] = ((unsigned char *)(p_pSrc))[2];\
                             ((unsigned char *)(p_pDst))[2] = ((unsigned char *)(p_pSrc))[1];\
                             ((unsigned char *)(p_pDst))[3] = ((unsigned char *)(p_pSrc))[0];\
                             }

///////////////////////////////////////////////////////////////////////////////
//    MACROS: some usefull macros
///////////////////////////////////////////////////////////////////////////////

// "Convert" a numeric literal into a hex constant
//     Note: 8-bit constants max value 0x11111111 = 0xFF = 255, always fits in unsigned long
#define HEX__(n) 0x##n##LU

// 8-bit conversion function
#define BIN8__(x) ((x&0x0000000FLU)?1:0) + ((x&0x000000F0LU)?2:0) + ((x&0x00000F00LU)?4:0) + ((x&0x0000F000LU)?8:0) \
                 +((x&0x000F0000LU)?16:0) + ((x&0x00F00000LU)?32:0) + ((x&0x0F000000LU)?64:0) + ((x&0xF0000000LU)?128:0)

// Example usage:
//    BIN8(01110101) = 117;
//    BIN16(11001000,01000111) = 51271;
//    BIN32(10000000,11111111,11000011,11000111) = 2164245447;
//
// Up to 8-bit binary constants
#define BIN8(d) ((unsigned char)BIN8__(HEX__(d)))
// Up to 16-bit binary constants, MSB first
#define BIN16(dmsb,dlsb)  (( (unsigned short)BIN8(dmsb)<< 8) + BIN8(dlsb))
// Up to 32-bit binary constants, MSB first
#define BIN32(dmsb,db2,db3,dlsb) (((unsigned long)BIN8(dmsb) << 24) + ((unsigned long)BIN8(db2) << 16) + ((unsigned long)BIN8(db3) << 8) + BIN8(dlsb))


typedef void (*FCT_TYPE_RUN_STATE)();

extern const uint16 g_aunCCITTCrcTable[256];
#define COMPUTE_CRC(unAcumulator, ucBuffElem) (unAcumulator << 8) ^ g_aunCCITTCrcTable[(unAcumulator >> 8) ^ ucBuffElem ]
uint16 ComputeCCITTCRC(const uint8 *p_pucBuf, uint8 p_ucLen);

uint16 IcmpGetFinalCksum( uint32 p_ulPrevCheckSum );
uint32 IcmpInterimChksum(const uint8 *p_pData, uint16 p_unDataLen, uint32 p_ulPrevCheckSum );

void Delay(unsigned int p_nLoopsWait);

#define __swap_bytes( p_unToSwap ) ((((uint16)(p_unToSwap)) >> 8 ) | (((uint16)(p_unToSwap)) << 8 ))

#endif /* _NIVIS_GLOBAL_H_ */
