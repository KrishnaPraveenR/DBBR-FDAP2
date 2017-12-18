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
#include "bcm5221.h"
#include "bcm5221_define.h"
//#include <pio/pio.h>
//#include <rstc/rstc.h>
#include "../emac.h"
//#include <utility/trace.h>
//#include <utility/assert.h>
#include "../../at91sam7x512.h"
#include "../../timers.h"
#include "../../wdt.h"

//-----------------------------------------------------------------------------
//         Definitions
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
/// Dump all the useful registers
/// \param pDm          Pointer to the Dm9161 instance
//-----------------------------------------------------------------------------
static void BCM5221_DumpRegisters(void)
{
    unsigned int value;

    AT91C_BASE_EMAC->EMAC_NCR |= AT91C_EMAC_MPE;//EMAC_EnableMdio();

    EMAC_ReadPhy(BCM5221_CTRL, &value);
    //trace_LOG(trace_INFO, " _CTRL   : 0x%X\n\r", value);
    EMAC_ReadPhy(BCM5221_STTS, &value);
    //trace_LOG(trace_INFO, " _STTS   : 0x%X\n\r", value);
    EMAC_ReadPhy(BCM5221_ANAR, &value);
    //trace_LOG(trace_INFO, " _ANAR   : 0x%X\n\r", value);
    EMAC_ReadPhy( BCM5221_ANLPAR, &value);
    //trace_LOG(trace_INFO, " _ANLPAR : 0x%X\n\r", value);
    EMAC_ReadPhy( BCM5221_ANER, &value);
    //trace_LOG(trace_INFO, " _ANER   : 0x%X\n\r", value);
    EMAC_ReadPhy( BCM5221_AXCTRL, &value);
    //trace_LOG(trace_INFO, " _AXCTRL   : 0x%X\n\r", value);
    EMAC_ReadPhy( BCM5221_AXSTTS, &value);
    //trace_LOG(trace_INFO, " _AXSTTS  : 0x%X\n\r", value);
    EMAC_ReadPhy( BCM5221_RCVCNT, &value);
    //trace_LOG(trace_INFO, " _RCVCNT: 0x%X\n\r", value);
    EMAC_ReadPhy( BCM5221_FCCNT, &value);
    //trace_LOG(trace_INFO, " _FCCNT  : 0x%X\n\r", value);
    EMAC_ReadPhy( BCM5221_DSCCNT, &value);
    //trace_LOG(trace_INFO, " _DSCCNT: 0x%X\n\r", value);
    EMAC_ReadPhy( BCM5221_PTEST, &value);
    //trace_LOG(trace_INFO, " _PTEST  : 0x%X\n\r", value);
    EMAC_ReadPhy( BCM5221_AXCTTS, &value);
    //trace_LOG(trace_INFO, " _AXCTTS   : 0x%X\n\r", value);

    AT91C_BASE_EMAC->EMAC_NCR &= ~AT91C_EMAC_MPE;//EMAC_DisableMdio();
}

//-----------------------------------------------------------------------------
//         Exported functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// Do a HW initialize to the PHY ( via RSTC ) and setup clocks & PIOs
/// This should be called only once to initialize the PHY pre-settings.
/// The PHY address is reset status of CRS,RXD[3:0] (the emacPins' pullups).
/// The COL pin is used to select MII mode on reset (pulled up for Reduced MII)
/// The RXDV pin is used to select test mode on reset (pulled up for test mode)
/// The above pins should be predefined for corresponding settings in resetPins
/// The EMAC peripheral pins are configured after the reset done.
/// Return 1 if RESET OK, 0 if timeout.
/// \param pDm         Pointer to the Dm9161 instance
/// \param mck         Main clock setting to initialize clock
/// \param resetPins   Pointer to list of PIOs to configure before HW RESET
///                       (for PHY power on reset configuration latch)
/// \param nbResetPins Number of PIO items that should be configured
/// \param emacPins    Pointer to list of PIOs for the EMAC interface
/// \param nbEmacPins  Number of PIO items that should be configured
//-----------------------------------------------------------------------------
void BCM5221_InitPhy (void)
{
    // Configure EMAC runtime pins
    {
#warning  "TO DO: #define here"
      AT91C_BASE_PIOB->PIO_IDR = 0x03FFFF;
      AT91C_BASE_PIOB->PIO_PPUDR = 0x03FFFF;
      AT91C_BASE_PIOB->PIO_ASR = 0x03FFFF;
      AT91C_BASE_PIOB->PIO_PDR = 0x03FFFF;
    }
    //set mdc clk
    AT91C_BASE_EMAC->EMAC_NCFGR = (AT91C_BASE_EMAC->EMAC_NCFGR &
                                  (~AT91C_EMAC_CLK)) | AT91C_EMAC_CLK_HCLK_32; // MDC clock = MCK/32
      
    AT91C_BASE_EMAC->EMAC_NCR |= AT91C_EMAC_MPE;
    AT91C_BASE_EMAC->EMAC_USRIO = AT91C_EMAC_CLKEN;
      
    // Configure LED Mode to 3
    EMAC_WritePhy(BCM5221_TEST, 0x80);
    EMAC_WritePhy(BCM5221_AXM4, 0x00);
    EMAC_WritePhy(BCM5221_TEST, 0x00);
    
    //BCM5221_AutoNegotiate();       
}

//-----------------------------------------------------------------------------
/// Issue a Auto Negotiation of the PHY
/// Return 1 if successfully, 0 if timeout.
/// \param pDm   Pointer to the Dm9161 instance
//-----------------------------------------------------------------------------
unsigned char BCM5221_AutoNegotiate(void)
{
    unsigned int value =0;
    unsigned int phyAnar;
    unsigned int phyAnalpar;
    unsigned int retryCount= 0;

    // Setup control register
    EMAC_ReadPhy(BCM5221_CTRL, &value);
    value &= ~BCM5221_AUTONEG;   // Remove autonegotiation enable
    value &= ~(BCM5221_LOOPBACK|BCM5221_POWER_DOWN);
    value |=  BCM5221_ISOLATE;   // Electrically isolate PHY
    EMAC_WritePhy(BCM5221_CTRL, value);

    // Set the Auto_negotiation Advertisement Register
    // MII advertising for Next page
    // 100BaseTxFD and HD, 10BaseTFD and HD, IEEE 802.3
    phyAnar = BCM5221_NP | BCM5221_TX_FDX | BCM5221_TX_HDX |
              BCM5221_10_FDX | BCM5221_10_HDX | BCM5221_AN_IEEE_802_3;
    EMAC_WritePhy(BCM5221_ANAR, phyAnar);

    // Read & modify control register
    EMAC_ReadPhy(BCM5221_CTRL, &value);
    value |= BCM5221_SPEED_SELECT | BCM5221_AUTONEG | BCM5221_DUPLEX_MODE;
    EMAC_WritePhy(BCM5221_CTRL, value);

    // Restart Auto_negotiation
    value |=  BCM5221_RESTART_AUTONEG;
    value &= ~BCM5221_ISOLATE;
    EMAC_WritePhy(BCM5221_CTRL, value);

    // Check AutoNegotiate complete
    while (1) {

        EMAC_ReadPhy(BCM5221_STTS, &value);
        // Done successfully
        if (value & BCM5221_AUTONEG_COMP) {
            break;
        }
        // Timeout check
        if (++ retryCount >= BCM5221_RETRY_MAX) {
            //AT91C_BASE_EMAC->EMAC_NCR &= ~AT91C_EMAC_MPE;//EMAC_DisableMdio();
            //return 0;
          break;
        }
        FEED_WDT();
    }

    // Get the AutoNeg Link partner base page
    EMAC_ReadPhy(BCM5221_ANLPAR, &phyAnalpar);

    // Setup the EMAC link speed
    if ((phyAnar & phyAnalpar) & BCM5221_TX_FDX) {
        // set MII for 100BaseTX and Full Duplex
        EMAC_SetLinkSpeed(1, 1);
    }
    else if ((phyAnar & phyAnalpar) & BCM5221_10_FDX) {
        // set MII for 10BaseT and Full Duplex
        EMAC_SetLinkSpeed(0, 1);
    }
    else if ((phyAnar & phyAnalpar) & BCM5221_TX_HDX) {
        // set MII for 100BaseTX and half Duplex
        EMAC_SetLinkSpeed(1, 0);
    }
    else if ((phyAnar & phyAnalpar) & BCM5221_10_HDX) {
        // set MII for 10BaseT and half Duplex
        EMAC_SetLinkSpeed(0, 0);
    }

    // Setup EMAC mode
    AT91C_BASE_EMAC->EMAC_USRIO = AT91C_EMAC_CLKEN;//EMAC_EnableMII();
    //AT91C_BASE_EMAC->EMAC_USRIO = AT91C_EMAC_CLKEN | AT91C_EMAC_RMII;//EMAC_EnableRMII()
    return 1;
}


//-----------------------------------------------------------------------------
/// Get Link Active Status
/// Return 1 if link is active, 0 if link is down.
/// \param : None
//-----------------------------------------------------------------------------
unsigned char BCM5221_IsLinkActive(void)
{
    unsigned int value;
    
    EMAC_ReadPhy(BCM5221_STTS, &value);
    
    // Done successfully
    if (value & BCM5221_LINK_STATUS) 
    {
        return 1;
    }
    else
    {
      return 0;
    }
}