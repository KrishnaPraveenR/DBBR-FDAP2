/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support 
 * ----------------------------------------------------------------------------
 * Copyright (c) 2008, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

// peripherals/emac/emac.h

#ifndef EMAC_H
#define EMAC_H

//-----------------------------------------------------------------------------
/// \dir
/// !Purpose
///
///     Definition of methods and structures for using EMAC
///     
/// !Usage
///
/// -# Initialize EMAC with EMAC_Init.
/// -# Setup EMAC with EMAC_SetupTx, EMAC_SetupRx, EMAC_SetupMacAddress
///    and EMAC_SetupStack. 
/// -# Drive the EMAC status machine by EMAC_Task.
/// -# EMAC_GetStatus give EMAC status machine current status 
/// -# Send a packet to network with EMAC_SendPacket.
/// -# Get a packet from network with EMAC_GetPacket.
///
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//         Headers
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//         Definitions
//-----------------------------------------------------------------------------
/// Number of buffer for RX, be carreful: MUST be 2^n
#define RX_BUFFERS  16
/// Number of buffer for TX, be carreful: MUST be 2^n
//#define TX_BUFFERS   8
#define TX_BUFFERS   2

/// Buffer Size
#define EMAC_RX_UNITSIZE            128     /// Fixed size for RX buffer
#define EMAC_TX_UNITSIZE            1518    /// Size for ETH frame length

// The MAC can support frame lengths up to 1536 bytes.
#define EMAC_FRAME_LENTGH_MAX       1536


//-----------------------------------------------------------------------------
//         Types
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//         PHY Exported functions
//-----------------------------------------------------------------------------

extern unsigned char EMAC_ReadPhy(unsigned char Address,
                                  unsigned int *pValue);

extern unsigned char EMAC_WritePhy(unsigned char Address,
                                   unsigned int Value);

extern void EMAC_SetLinkSpeed(unsigned char speed,
                              unsigned char fullduplex);

//-----------------------------------------------------------------------------
//         EMAC Exported functions
//-----------------------------------------------------------------------------
/// Callback used by send function
typedef void (*EMAC_TxCallback)(unsigned int status);
typedef void (*EMAC_RxCallback)(unsigned int status);
typedef void (*EMAC_WakeupCallback)(void);

extern void EMAC_Init( unsigned char id, const unsigned char *pMacAddress,
                unsigned char enableCAF, unsigned char enableNBC );
#define EMAC_CAF_DISABLE  0
#define EMAC_CAF_ENABLE   1
#define EMAC_NBC_DISABLE  0
#define EMAC_NBC_ENABLE   1

extern void EMAC_Handler(void);

extern unsigned char EMAC_Send(void *pBuffer, 
                               unsigned int size);
/// Return for EMAC_Send function
#define EMAC_TX_OK                     0
#define EMAC_TX_BUFFER_BUSY            1
#define EMAC_TX_INVALID_PACKET         2


extern unsigned char EMAC_Poll(unsigned char *pFrame,
                               unsigned short frameSize,
                               unsigned short *pRcvSize);
/// Return for EMAC_Poll function
#define EMAC_RX_OK                   0
#define EMAC_RX_NO_DATA              1
#define EMAC_RX_FRAME_SIZE_TOO_SMALL 2

extern unsigned char ItCanGo (void);
void EMAC_ClearTxBuff (void);

#endif // #ifndef EMAC_H

