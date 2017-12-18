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



#ifndef _DM9161_H
#define _DM9161_H

//-----------------------------------------------------------------------------
//         Headers
//-----------------------------------------------------------------------------
//#include <pio/pio.h>

//-----------------------------------------------------------------------------
//         Definitions
//-----------------------------------------------------------------------------

/// The reset length setting for external reset configuration
#define BCM5221_RESET_LENGTH         0xD

//-----------------------------------------------------------------------------
//         Types
//-----------------------------------------------------------------------------

/// The DM9161 instance
typedef struct _Bcm5221 {

    /// The retry & timeout settings
    unsigned int retryMax;

    /// PHY address ( pre-defined by pins on reset )
    unsigned char phyAddress;

} Bcm5221, *pBcm5221;

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------

extern void BCM5221_InitPhy (void);

extern unsigned char BCM5221_AutoNegotiate(void);

extern unsigned char BCM5221_Send(Bcm5221 *pBcm,
                                 void *pBuffer,
                                 unsigned int size);

extern unsigned int BCM5221_Poll(Bcm5221 *pBcm,
                                unsigned char *pBuffer,
                                unsigned int size);

unsigned char BCM5221_IsLinkActive(void);

#endif // #ifndef _DM9161_H

