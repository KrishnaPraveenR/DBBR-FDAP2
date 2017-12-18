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

//-----------------------------------------------------------------------------
//         Headers
//-----------------------------------------------------------------------------
//#include <board.h>
#include "emac.h"
#include "../at91sam7x512.h"
//#include <utility/trace.h>
//#include <utility/assert.h>
#include <string.h>
#include "./bcm5221/bcm5221_define.h"

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/// The buffer addresses written into the descriptors must be aligned so the
/// last few bits are zero.  These bits have special meaning for the EMAC
/// peripheral and cannot be used as part of the address.
#define EMAC_ADDRESS_MASK   ((unsigned int)0xFFFFFFFC)
#define EMAC_LENGTH_FRAME   ((unsigned int)0x0FFF)    /// Length of frame mask

// receive buffer descriptor bits
#define EMAC_RX_OWNERSHIP_BIT   (1UL <<  0)
#define EMAC_RX_WRAP_BIT        (1UL <<  1)
#define EMAC_RX_SOF_BIT         (1UL << 14)
#define EMAC_RX_EOF_BIT         (1UL << 15)

// Transmit buffer descriptor bits
#define EMAC_TX_LAST_BUFFER_BIT (1UL << 15)
#define EMAC_TX_WRAP_BIT        (1UL << 30)
#define EMAC_TX_USED_BIT        (1UL << 31)

//-----------------------------------------------------------------------------
// Circular buffer management
//-----------------------------------------------------------------------------
// Return count in buffer
#define CIRC_CNT(head,tail,size) (((head) - (tail)) & ((size)-1))

// Return space available, 0..size-1
// We always leave one free char as a completely full buffer
// has head == tail, which is the same as empty
#define CIRC_SPACE(head,tail,size) CIRC_CNT((tail),((head)+1),(size))

// Return count up to the end of the buffer.
// Carefully avoid accessing head and tail more than once,
// so they can change underneath us without returning inconsistent results
#define CIRC_CNT_TO_END(head,tail,size) \
   ({int end = (size) - (tail); \
     int n = ((head) + end) & ((size)-1); \
     n < end ? n : end;})

// Return space available up to the end of the buffer
#define CIRC_SPACE_TO_END(head,tail,size) \
   ({int end = (size) - 1 - (head); \
     int n = (end + (tail)) & ((size)-1); \
     n <= end ? n : end+1;})

// Increment head or tail
#define CIRC_INC(headortail,size) \
        headortail++;             \
        if(headortail >= size) {  \
            headortail = 0;       \
        }

#define CIRC_EMPTY(circ)     ((circ)->head == (circ)->tail)
#define CIRC_CLEAR(circ)     ((circ)->head = (circ)->tail = 0)


//------------------------------------------------------------------------------
//      Structures
//------------------------------------------------------------------------------
#ifdef __ICCARM__          // IAR
#pragma pack(4)            // IAR
#define __attribute__(...) // IAR
#endif                     // IAR
/// Describes the type and attribute of Receive Transfer descriptor.
typedef struct _EmacRxTDescriptor {
    unsigned int addr;
    unsigned int status;
} __attribute__((packed, aligned(8))) EmacRxTDescriptor, *PEmacRxTDescriptor;

/// Describes the type and attribute of Transmit Transfer descriptor.
typedef struct _EmacTxTDescriptor {
    unsigned int addr;
    unsigned int status;
} __attribute__((packed, aligned(8))) EmacTxTDescriptor, *PEmacTxTDescriptor;
#ifdef __ICCARM__          // IAR
#pragma pack()             // IAR
#endif                     // IAR

/// Descriptors for RX (required aligned by 8)
typedef struct {
   volatile EmacRxTDescriptor td[RX_BUFFERS];
   unsigned short idx;
} RxTd;

/// Descriptors for TX (required aligned by 8)
typedef struct {
   volatile EmacTxTDescriptor td[TX_BUFFERS];
   unsigned short head;            /// Circular buffer head pointer incremented by the upper layer (buffer to be sent)
   unsigned short tail;            /// Circular buffer head pointer incremented by the IT handler (buffer sent)
} TxTd;

//------------------------------------------------------------------------------
//         Internal variables
//------------------------------------------------------------------------------
// Receive Transfer Descriptor buffer
#ifdef __ICCARM__          // IAR
#pragma data_alignment=8   // IAR
#endif                     // IAR
static volatile RxTd rxTd;
// Transmit Transfer Descriptor buffer
#ifdef __ICCARM__          // IAR
#pragma data_alignment=8   // IAR
#endif                     // IAR
static volatile TxTd txTd;
/// Send Buffer
// Section 3.6 of AMBA 2.0 spec states that burst should not cross 1K Boundaries.
// Receive buffer manager writes are burst of 2 words => 3 lsb bits of the address shall be set to 0
#ifdef __ICCARM__          // IAR
#pragma data_alignment=8   // IAR
#endif                     // IAR
static volatile unsigned char pTxBuffer[TX_BUFFERS * EMAC_TX_UNITSIZE] __attribute__((aligned(8)));

#ifdef __ICCARM__          // IAR
#pragma data_alignment=8   // IAR
#endif                     // IAR
/// Receive Buffer
static volatile unsigned char pRxBuffer[RX_BUFFERS * EMAC_RX_UNITSIZE] __attribute__((aligned(8)));

//-----------------------------------------------------------------------------
//         Internal functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// Wait PHY operation complete.
/// Return 1 if the operation completed successfully.
/// May be need to re-implemented to reduce CPU load.
/// \param retry: the retry times, 0 to wait forever until complete.
//-----------------------------------------------------------------------------
static unsigned char EMAC_WaitPhy( void )
{
    unsigned int retry_count = 0;

    while((AT91C_BASE_EMAC->EMAC_NSR & AT91C_EMAC_IDLE) == 0) {

        // Timeout check
        retry_count++;
        if(retry_count >= BCM5221_RETRY_MAX) {

            //"E: Wait PHY time out\n\r"
            return 0;
        }
    }

    return 1;
}

//-----------------------------------------------------------------------------
//         Exported functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//          PHY management functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// Read PHY register.
/// Return 1 if successfully, 0 if timeout.
/// \param PhyAddress PHY Address
/// \param Address Register Address
/// \param pValue Pointer to a 32 bit location to store read data
/// \param retry The retry times, 0 to wait forever until complete.
//-----------------------------------------------------------------------------
unsigned char EMAC_ReadPhy(unsigned char Address,
                           unsigned int *pValue)
{
    AT91C_BASE_EMAC->EMAC_MAN = (AT91C_EMAC_SOF & (0x01 << 30))
                              | (AT91C_EMAC_CODE & (2 << 16))
                              | (AT91C_EMAC_RW & (2 << 28))
                              | (AT91C_EMAC_PHYA & ((BCM5221_PHY_ADDR & 0x1f) << 23))
                              | (AT91C_EMAC_REGA & (Address << 18));

    if ( EMAC_WaitPhy() == 0 ) {

        //"TimeOut EMAC_ReadPhy\n\r"
        return 0;
    }
    *pValue = ( AT91C_BASE_EMAC->EMAC_MAN & 0x0000ffff );
    return 1;
}

//-----------------------------------------------------------------------------
/// Write PHY register
/// Return 1 if successfully, 0 if timeout.
/// \param PhyAddress PHY Address
/// \param Address Register Address
/// \param Value Data to write ( Actually 16 bit data )
/// \param retry The retry times, 0 to wait forever until complete.
//-----------------------------------------------------------------------------
unsigned char EMAC_WritePhy(unsigned char Address,
                            unsigned int  Value)
{
    AT91C_BASE_EMAC->EMAC_MAN = (AT91C_EMAC_SOF & (0x01 << 30))
                              | (AT91C_EMAC_CODE & (2 << 16))
                              | (AT91C_EMAC_RW & (1 << 28))
                              | (AT91C_EMAC_PHYA & ((BCM5221_PHY_ADDR & 0x1f) << 23))
                              | (AT91C_EMAC_REGA & (Address << 18))
                              | (AT91C_EMAC_DATA & Value) ;
    if ( EMAC_WaitPhy() == 0 ) {

        //"TimeOut EMAC_WritePhy\n\r"
        return 0;
    }
    return 1;
}

//-----------------------------------------------------------------------------
/// Setup the EMAC for the link : speed 100M/10M and Full/Half duplex
/// \param speed        Link speed, 0 for 10M, 1 for 100M
/// \param fullduplex   1 for Full Duplex mode
//-----------------------------------------------------------------------------
void EMAC_SetLinkSpeed(unsigned char speed, unsigned char fullduplex)
{
    unsigned int ncfgr;

    ncfgr = AT91C_BASE_EMAC->EMAC_NCFGR;
    ncfgr &= ~(AT91C_EMAC_SPD | AT91C_EMAC_FD);
    if (speed) {

        ncfgr |= AT91C_EMAC_SPD;
    }
    if (fullduplex) {

        ncfgr |= AT91C_EMAC_FD;
    }
    AT91C_BASE_EMAC->EMAC_NCFGR = ncfgr;
}



//-----------------------------------------------------------------------------
//          EMAC functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// EMAC Interrupt handler
//-----------------------------------------------------------------------------
void EMAC_Handler(void)
{
    volatile EmacTxTDescriptor *pTxTd;
    unsigned int isr;
    unsigned int rsr;
    unsigned int tsr;
    //unsigned int rxStatusFlag;
    unsigned int txStatusFlag;

    //trace_LOG(trace_DEBUG, "EMAC_Handler\n\r");
    isr = AT91C_BASE_EMAC->EMAC_ISR & AT91C_BASE_EMAC->EMAC_IMR;
    rsr = AT91C_BASE_EMAC->EMAC_RSR;
    tsr = AT91C_BASE_EMAC->EMAC_TSR;

    // RX packet
    if ((isr & AT91C_EMAC_RCOMP) || (rsr & AT91C_EMAC_REC)) {
        // Clear status
        AT91C_BASE_EMAC->EMAC_RSR = AT91C_EMAC_REC | AT91C_EMAC_OVR | AT91C_EMAC_BNA;
    }

    // TX packet
    if ((isr & AT91C_EMAC_TCOMP) || (tsr & AT91C_EMAC_COMP)) {

        txStatusFlag = AT91C_EMAC_COMP;
//        EmacStatistics.tx_comp ++;

        // A frame transmitted
        // Check RLE
        if (tsr & AT91C_EMAC_RLES) {
            txStatusFlag |= AT91C_EMAC_RLES;
//            EmacStatistics.tx_errors++;
        }
        // Check COL
        if (tsr & AT91C_EMAC_COL) {
            txStatusFlag |= AT91C_EMAC_COL;
//            EmacStatistics.collisions++;
        }
        // Check BEX
        if (tsr & AT91C_EMAC_BEX) {
            txStatusFlag |= AT91C_EMAC_BEX;
//            EmacStatistics.tx_exausts++;
        }
        // Check UND
        if (tsr & AT91C_EMAC_UND) {
            txStatusFlag |= AT91C_EMAC_UND;
//            EmacStatistics.tx_underruns++;
        }
        // Clear status
        AT91C_BASE_EMAC->EMAC_TSR |= txStatusFlag;

        // Check the buffers
        while (CIRC_CNT(txTd.head, txTd.tail, TX_BUFFERS)) {
            pTxTd = txTd.td + txTd.tail;

            // Exit if buffer has not been sent yet
            if ((pTxTd->status & EMAC_TX_USED_BIT) == 0) {
                //trace_LOG(trace_DEBUG, "buffer has not been sent")
                break;
            }
            
            {
            CIRC_INC( txTd.tail, TX_BUFFERS );
            }
        }
    }
}

//-----------------------------------------------------------------------------
/// Initialize the EMAC with the emac controller address
/// \param id     HW ID for power management
/// \param pTxWakeUpfct Thresold TX Wakeup Callback
/// \param pRxfct       RX Wakeup Callback
/// \param pMacAddress  Mac Address
/// \param enableCAF    enable AT91C_EMAC_CAF if needed by application
/// \param enableNBC    AT91C_EMAC_NBC if needed by application
//-----------------------------------------------------------------------------
void EMAC_Init( unsigned char id, const unsigned char *pMacAddress,
                unsigned char enableCAF, unsigned char enableNBC )
{
    int Index;
    unsigned int Address;

    //"EMAC_Init\n\r"

    // Power ON
    AT91C_BASE_PMC->PMC_PCER = 1 << id;

    // Disable TX & RX and more
    AT91C_BASE_EMAC->EMAC_NCR = 0;

    // disable
    AT91C_BASE_EMAC->EMAC_IDR = ~0;

    rxTd.idx = 0;
    CIRC_CLEAR(&txTd);

    // Setup the RX descriptors.
    for(Index = 0; Index < RX_BUFFERS; Index++) {

        Address = (unsigned int)(&(pRxBuffer[Index * EMAC_RX_UNITSIZE]));
        // Remove EMAC_RX_OWNERSHIP_BIT and EMAC_RX_WRAP_BIT
        rxTd.td[Index].addr = Address & EMAC_ADDRESS_MASK;
        rxTd.td[Index].status = 0;
    }
    rxTd.td[RX_BUFFERS - 1].addr |= EMAC_RX_WRAP_BIT;

    // Setup the TX descriptors.
    for(Index = 0; Index < TX_BUFFERS; Index++) {

        Address = (unsigned int)(&(pTxBuffer[Index * EMAC_TX_UNITSIZE]));
        txTd.td[Index].addr = Address;
        txTd.td[Index].status = EMAC_TX_USED_BIT;
    }
    txTd.td[TX_BUFFERS - 1].status = EMAC_TX_USED_BIT | EMAC_TX_WRAP_BIT;

    // Set the MAC address
    if( pMacAddress != (unsigned char *)0 ) {
        AT91C_BASE_EMAC->EMAC_SA1L = ( ((unsigned int)pMacAddress[3] << 24)
                                     | ((unsigned int)pMacAddress[2] << 16)
                                     | ((unsigned int)pMacAddress[1] << 8 )
                                     |                pMacAddress[0] );

        AT91C_BASE_EMAC->EMAC_SA1H = ( ((unsigned int)pMacAddress[5] << 8 )
                                     |                pMacAddress[4] );
    }
    // Now setup the descriptors
    // Receive Buffer Queue Pointer Register
    AT91C_BASE_EMAC->EMAC_RBQP = (unsigned int) (rxTd.td);
    // Transmit Buffer Queue Pointer Register
    AT91C_BASE_EMAC->EMAC_TBQP = (unsigned int) (txTd.td);

    AT91C_BASE_EMAC->EMAC_NCR = AT91C_EMAC_CLRSTAT;

    // Clear all status bits in the receive status register.
    AT91C_BASE_EMAC->EMAC_RSR = (AT91C_EMAC_OVR | AT91C_EMAC_REC | AT91C_EMAC_BNA);

    // Clear all status bits in the transmit status register
    AT91C_BASE_EMAC->EMAC_TSR = ( AT91C_EMAC_UBR | AT91C_EMAC_COL | AT91C_EMAC_RLES
                                | AT91C_EMAC_BEX | AT91C_EMAC_COMP
                                | AT91C_EMAC_UND );

    // Clear interrupts
    AT91C_BASE_EMAC->EMAC_ISR;

    // Enable the copy of data into the buffers
    // ignore broadcasts, and don't copy FCS.
    AT91C_BASE_EMAC->EMAC_NCFGR |= (AT91C_EMAC_DRFCS | AT91C_EMAC_PAE);

    if( enableCAF == EMAC_CAF_ENABLE ) {
        AT91C_BASE_EMAC->EMAC_NCFGR |= AT91C_EMAC_CAF;
    }
    if( enableNBC == EMAC_NBC_ENABLE ) {
        AT91C_BASE_EMAC->EMAC_NCFGR |= AT91C_EMAC_NBC;
    }

    // Enable Rx and Tx, plus the stats register.
    AT91C_BASE_EMAC->EMAC_NCR |= (AT91C_EMAC_TE | AT91C_EMAC_RE | AT91C_EMAC_WESTAT);

    // Setup the interrupts for TX (and errors)
    AT91C_BASE_EMAC->EMAC_IER = AT91C_EMAC_RCOMP
                              //| AT91C_EMAC_RXUBR
                              //| AT91C_EMAC_TUNDR
                              //| AT91C_EMAC_RLEX
                              //| AT91C_EMAC_TXERR
                              //| AT91C_EMAC_TCOMP
                              //| AT91C_EMAC_ROVR
                              //| AT91C_EMAC_HRESP
                              ;
}


//-----------------------------------------------------------------------------
/// Send a packet with EMAC.
/// If the packet size is larger than transfer buffer size error returned.
/// \param buffer   The buffer to be send
/// \param size     The size of buffer to be send
/// \return         OK, Busy or invalid packet
//-----------------------------------------------------------------------------
unsigned char EMAC_Send(void *pBuffer,
                        unsigned int size )
{
    volatile EmacTxTDescriptor *pTxTd;

    //trace_LOG(trace_DEBUG, "EMAC_Send\n\r");

    // Check parameter
    if (size > EMAC_TX_UNITSIZE) {

        //"-E- EMAC driver does not split send packets."
        //" It can send %d bytes max in one packet (%u bytes requested)\n\r", EMAC_TX_UNITSIZE, size
        return EMAC_TX_INVALID_PACKET;
    }

    // If no free TxTd, buffer can't be sent, schedule the wakeup callback
    if( CIRC_SPACE(txTd.head, txTd.tail, TX_BUFFERS) == 0) {
        //trace_LOG(trace_DEBUG, " CIRC_SPACE head = %d tail = %d\n\r", txTd.head, txTd.tail);
        return EMAC_TX_BUFFER_BUSY;
    }
        //trace_LOG(trace_DEBUG, " NOW head = %d tail = %d\n\r", txTd.head, txTd.tail);

    // Pointers to the current TxTd
    pTxTd = txTd.td + txTd.head;

    // Setup/Copy data to transmition buffer
    if (pBuffer && size) {
        // Driver manage the ring buffer
        memcpy((void *)pTxTd->addr, pBuffer, size);
    }

    // Update TD status
    // The buffer size defined is length of ethernet frame
    // so it's always the last buffer of the frame.
    if (txTd.head == TX_BUFFERS-1) {
        pTxTd->status =
            (size & EMAC_LENGTH_FRAME) | EMAC_TX_LAST_BUFFER_BIT | EMAC_TX_WRAP_BIT;
    }
    else {
        pTxTd->status = (size & EMAC_LENGTH_FRAME) | EMAC_TX_LAST_BUFFER_BIT;
    }

    CIRC_INC(txTd.head, TX_BUFFERS)

    // Tx packets count
//    EmacStatistics.tx_packets++;

    // Now start to transmit if it is not already done
    AT91C_BASE_EMAC->EMAC_NCR |= AT91C_EMAC_TSTART;

    return EMAC_TX_OK;
}

//-----------------------------------------------------------------------------
/// Receive a packet with EMAC
/// If not enough buffer for the packet, the remaining data is lost but right
/// frame length is returned.
/// \param pFrame           Buffer to store the frame
/// \param frameSize        Size of the frame
/// \param pRcvSize         Received size
/// \return                 OK, no data, or frame too small
//-----------------------------------------------------------------------------
unsigned char EMAC_Poll(unsigned char *pFrame,
                        unsigned short frameSize,
                        unsigned short *pRcvSize)
{
    unsigned short bufferLength;
    unsigned short   tmpFrameSize=0;
    unsigned char  *pTmpFrame=0;
    unsigned short   tmpIdx = rxTd.idx;
    volatile EmacRxTDescriptor *pRxTd = rxTd.td + rxTd.idx;

    char isFrame = 0;
    // Set the default return value
    *pRcvSize = 0;

    // Process received RxTd
    while ((pRxTd->addr & EMAC_RX_OWNERSHIP_BIT) == EMAC_RX_OWNERSHIP_BIT) {

        // A start of frame has been received, discard previous fragments
        if ((pRxTd->status & EMAC_RX_SOF_BIT) == EMAC_RX_SOF_BIT) {
            // Skip previous fragment
            while (tmpIdx != rxTd.idx) {
                pRxTd = rxTd.td + rxTd.idx;
                pRxTd->addr &= ~(EMAC_RX_OWNERSHIP_BIT);
                CIRC_INC(rxTd.idx, RX_BUFFERS);
            }
            // Reset the temporary frame pointer
            pTmpFrame = pFrame;
            tmpFrameSize = 0;
            // Start to gather buffers in a frame
            isFrame = 1;
        }

        // Increment the pointer
        CIRC_INC(tmpIdx, RX_BUFFERS);

        // Copy data in the frame buffer
        if (isFrame) {
            if (tmpIdx == rxTd.idx) {
                // "I: no EOF (Invalid of buffers too small)\n\r"

                do {

                    pRxTd = rxTd.td + rxTd.idx;
                    pRxTd->addr &= ~(EMAC_RX_OWNERSHIP_BIT);
                    CIRC_INC(rxTd.idx, RX_BUFFERS);
                } while(tmpIdx != rxTd.idx);
                return EMAC_RX_NO_DATA;
            }
            // Copy the buffer into the application frame
            bufferLength = EMAC_RX_UNITSIZE;
            if ((tmpFrameSize + bufferLength) > frameSize) {
                bufferLength = frameSize - tmpFrameSize;
            }

            memcpy(pTmpFrame, (void*)(pRxTd->addr & EMAC_ADDRESS_MASK), bufferLength);
            pTmpFrame += bufferLength;
            tmpFrameSize += bufferLength;

            // An end of frame has been received, return the data
            if ((pRxTd->status & EMAC_RX_EOF_BIT) == EMAC_RX_EOF_BIT) {
                // Frame size from the EMAC
                *pRcvSize = (pRxTd->status & EMAC_LENGTH_FRAME);

                // Application frame buffer is too small all data have not been copied
                if (tmpFrameSize < *pRcvSize) {
                    //printf("size req %u size allocated %u\n\r", *pRcvSize, frameSize);
                    while (rxTd.idx != tmpIdx) {
                        pRxTd = rxTd.td + rxTd.idx;
                        pRxTd->addr &= ~(EMAC_RX_OWNERSHIP_BIT);
                        CIRC_INC(rxTd.idx, RX_BUFFERS);
                    }
                    return EMAC_RX_FRAME_SIZE_TOO_SMALL;
                }

                //trace_LOG(trace_INFO, "packet %d-%u (%u)\n\r", rxTd.idx, tmpIdx, *pRcvSize);
                // All data have been copied in the application frame buffer => release TD
                while (rxTd.idx != tmpIdx) {
                    pRxTd = rxTd.td + rxTd.idx;
                    pRxTd->addr &= ~(EMAC_RX_OWNERSHIP_BIT);
                    CIRC_INC(rxTd.idx, RX_BUFFERS);
                }
//                EmacStatistics.rx_packets++;
                return EMAC_RX_OK;
            }
        }

        // SOF has not been detected, skip the fragment
        else {
           pRxTd->addr &= ~(EMAC_RX_OWNERSHIP_BIT);
           rxTd.idx = tmpIdx;
        }

        // Process the next buffer
        pRxTd = rxTd.td + tmpIdx;
    }

    //trace_LOG(trace_DEBUG, "E");
    return EMAC_RX_NO_DATA;
}

//-----------------------------------------------------------------------------
/// Not sure what is the use 
//-----------------------------------------------------------------------------
unsigned char ItCanGo (void)
{
  return (txTd.tail==0);
}

void EMAC_ClearTxBuff (void)
{  
volatile EmacTxTDescriptor *pTxTd;

  // Check the buffers
        while (CIRC_CNT(txTd.head, txTd.tail, TX_BUFFERS)) {
            pTxTd = txTd.td + txTd.tail;

            // Exit if buffer has not been sent yet
            if ((pTxTd->status & EMAC_TX_USED_BIT) == 0) {
                //trace_LOG(trace_DEBUG, "buffer has not been sent")
                break;
            }
            CIRC_INC( txTd.tail, TX_BUFFERS );
        }
}

