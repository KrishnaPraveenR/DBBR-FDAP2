////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Rares Ivan
/// Date:         February 2008
/// Description:  ISA100 PHY for Honeywell Raptor Leaf Node radio platform
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "sfc.h"
#include "phy.h"
#include "mlde.h"
#include "mlsm.h"
#include "tmr_util.h"

volatile unsigned char g_ucXVRStatus = PHY_DISABLED;                   // state machine

uint8 g_ucLastRFChannel = 0xFF;
uint8 g_ucLastRSSI;


#ifdef LOW_POWER_DEVICE
  unsigned char g_ucAESUserOnProgress;
  #define SET_AES_USER_ON_PROGRESS(x) g_ucAESUserOnProgress = (x)
#else
  #define SET_AES_USER_ON_PROGRESS(x) 
  
#endif
  
const uint8 * g_pPrevKey = NULL;

// Rares : Note that TXFIFO/RXFIFO are 128 bytes len.
//           Frame format at PHY level is:
//            [ 1 byte frame len][Paylod up to 125 bytes][2 bytes FCS]
//
//           RX Frame:
//            [ 1 byte frame len][Paylod up to 125 bytes][1 byte RSSI ][1 bytes: Bit7 CRCOK, Bit6:Bit0 CORRELATION]
//
//           TX Frame:
//            [ 1 byte frame len][Paylod up to 125 bytes][1 byte RSSI ][2 bytes FCS]
//
//           When Authentication in enabled, the MIC should be taken in consideration


unsigned int  g_unTimestampSFDReceived;
uint16 g_unRxAckSFDOffsetFract;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Rares Ivan
/// @brief  Execute a time triggered action based on modem status
/// @param  none
/// @return none
/// @remarks
///      Access level: Interrupt Level\n
///      Context: This function is called from TimerA interrupt TIMERA1_Handler()
////////////////////////////////////////////////////////////////////////////////////////////////////
void PHY_OnTimeTriggeredAction(void)
{
  unsigned char statusReg;

#ifdef RX_TX_LINK_COEXISTENCE  
  if( g_pCandidateHashEntry && (g_ucXVRStatus == PHY_RX_IN_PROGRESS) ) // timeout during reception -> a RX/TX slot
  {
      CC2420_SetModemState(CC2420_MODEM_STATE_IDLE); // prepare the modem for TX 
    
      g_pCrtHashEntry = g_pCandidateHashEntry;
      g_ucDllTdmaStat = MLSM_TX_FRAME;
      
      g_ucXVRStatus = (g_aDllTimeslotsTable[g_pCrtHashEntry->m_ucTemplateIdx].m_stTimeslot.m_ucInfo & DLL_MASK_TSCCA  ? 
                                    PHY_TX_WITH_CCA_START_DO : 
                                    PHY_TX_START_DO );
  }
#endif
  
  switch(g_ucXVRStatus)
  {
    case PHY_DISABLED:
        break;

    case PHY_IDLE:      
        PHY_SetToIdle();
        break;

    case PHY_CCA_START_DO:
        //g_ucXVRStatus = PHY_CCA_IN_PROGRESS;
        //CC2420_SetModemState(CC2420_MODEM_STATE_RX);
        break;

    case PHY_TX_START_DO:
        #if (DEBUG_MODE >= 3)
          DEBUG1_ON();    // test - start modem action
          DEBUG1_OFF();
          DEBUG1_ON();    // test - start modem action
          DEBUG1_OFF();
          DEBUG1_ON();    // test - start modem action
          DEBUG1_OFF();
        #endif

        g_ucXVRStatus = PHY_TX_IN_PROGRESS;
        CC2420_SetModemState(CC2420_MODEM_STATE_TX_NO_CCA);
        
//        if( g_ucDllTdmaStat == MLSM_TX_ACK || g_ucDllTdmaStat == MLSM_TX_NACK )
//        {
//            CC2420_LoadRXBuffer( g_aucDllRxTxBuf,  g_stDllRxHdrExt.m_ucSecurityPos+g_stDllMsgHrds.m_ucPayloadLen + MIC_SIZE );
//        }
        break;

    case PHY_TX_WITH_CCA_START_DO:
        #if (DEBUG_MODE >= 3)
          DEBUG1_ON();    // test - start modem action
          DEBUG1_OFF();
          DEBUG1_ON();    // test - start modem action
          DEBUG1_OFF();
          DEBUG1_ON();    // test - start modem action
          
        #endif

        g_ucXVRStatus = PHY_TX_IN_PROGRESS;       
        
        CC2420_SHDNLNA_ON();        // use CCA manually 
        CC2420_MACRO_SELECT();
        
        CC2420_MACRO_STROBE(CC2420_SRXON);                // Set Transceiver to RX mode                                                               
        do                                                // Loop read Status Byte until RSSI becomes valid (RX must be ON for at least 8 symbols)
        { 
          CC2420_MACRO_GET_STATUS(statusReg); 
        } 
        while ( !(statusReg & (1 << CC2420_RSSI_VALID)) );                
        
        CC2420_MACRO_RELEASE();        
        
        if ( PHY_IsBusyChannel() )
        {
          PHY_DataConfirm(TRANCEIVER_BUSY); 

#ifdef CUSTOM_RX_AFTER_CCA
        g_ucDllTdmaStat = MLSM_RX_FRAME;
        
        #if (DEBUG_MODE >= 3)
            DEBUG1_ON();    // test - start modem action
            DEBUG1_OFF();
          #endif
  
          g_ucXVRStatus = PHY_RX_IN_PROGRESS;
          CC2420_SetModemState(CC2420_MODEM_STATE_RX);
          break;          
#endif          
        }
        else
        {
          CC2420_SetModemState(CC2420_MODEM_STATE_TX_NO_CCA);
        }     
        
        #if (DEBUG_MODE >= 3)
          DEBUG1_OFF();
        #endif
        
        // ... from point 1 to point 2A or 2B = takes 260 uSec !
        break;

    case PHY_RX_START_DO:
        #if (DEBUG_MODE >= 3)
          DEBUG1_ON();    // test - start modem action
          DEBUG1_OFF();
        #endif

        g_ucXVRStatus = PHY_RX_IN_PROGRESS;
        CC2420_SetModemState(CC2420_MODEM_STATE_RX);
        break;        
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Rares Ivan
/// @brief  Disables the Transceiver (PHY_DISABLE)
/// @param  p_ucDisableLevel must be PHY_DISABLED or PHY_IDLE
/// @return none
/// @remarks
///      Access level: Interrupt Level
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef LOW_POWER_DEVICE  
  void PHY_Disable_RequestExt( unsigned char p_ucDisableLevel )
  {    
    if( g_ucAESUserOnProgress ) // don't allows shut down during AES user operation or if not predictive code
    {
        p_ucDisableLevel = PHY_IDLE;
    }
    
    if( p_ucDisableLevel < g_ucXVRStatus ) // minimize the consuption sense 
    {
      TMR_DisablePendingActions();
      
      // Put CC2420 in Idle mode, PA off, LNA off
      if( p_ucDisableLevel == PHY_IDLE )
      {
          CC2420_SetModemState(CC2420_MODEM_STATE_IDLE);
      }
      else
      {
          CC2420_SetModemState(CC2420_MODEM_STATE_POWERDOWN);
      }
        
      g_ucXVRStatus = p_ucDisableLevel;
  
      #if (DEBUG_MODE >= 1)
        DEBUG3_OFF();    // test - Rx/Tx off
        DEBUG4_OFF();
      #endif
    }
  }
#else
  void PHY_Disable_RequestExt( void )
  {
    if( g_ucXVRStatus != PHY_IDLE )
    {
      TMR_DisablePendingActions();
      
      // Put CC2420 in Idle mode, PA off, LNA off
      CC2420_SetModemState(CC2420_MODEM_STATE_IDLE);
  
      g_ucXVRStatus = PHY_IDLE;
  
      #if (DEBUG_MODE >= 1)
        DEBUG3_OFF();    // test - Rx/Tx off
        DEBUG4_OFF();
      #endif
    }
    
  }
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Rares Ivan
/// @brief  Disables the Transceiver (PHY_DISABLE)
/// @param  none
/// @return none
/// @remarks
///      Access level: User Level
////////////////////////////////////////////////////////////////////////////////////////////////////
void PHY_Reset(void)
{
    // Put CC2420 in Idle mode, PA off, LNA off
    CC2420_SetModemState(CC2420_MODEM_STATE_POWERDOWN);
    _NOP();
    _NOP();
    CC2420_SetModemState(CC2420_MODEM_STATE_POWERUP);

    CC2420_MACRO_OFF_ALL_IRQS();
    CC2420_MACRO_CLEAR_ALL_IRQS();

    g_ucXVRStatus = PHY_IDLE;
    
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Rares Ivan
/// @brief  Enables the transceiver in receive mode on the desired frequency channel (p_ucChannel)
///         the receive sequence will begin after minimum of X uS
/// @param  p_ucChannel - rx channel
/// @param  p_unDelay   - time after which to perform the action (timerA ticks)
/// @remarks
///      Access level: Interrupt Level
////////////////////////////////////////////////////////////////////////////////////////////////////
void PHY_RX_Request ( unsigned char p_ucChannel, unsigned char p_ucRelativeFlag,  unsigned int p_unDelay )
{
    PHY_Disable_Request(PHY_IDLE);
    
    // Set RF Channel (channel is given as a channel number 0..15)    
    if( g_ucLastRFChannel != p_ucChannel )
    {
          CC2420_MACRO_SELECT();
          CC2420_MACRO_SET_RFCH_0TO15(p_ucChannel);
          CC2420_MACRO_RELEASE();
          
          g_ucLastRFChannel = p_ucChannel;
    }

    // Set the status -> this statud will be used when the PHY_OnTimeTriggeredAction() is called from timer interrupt
    // Based on this status, the next action is decided
    g_ucXVRStatus = PHY_RX_START_DO;

    // Schedule a delayed action
    ExecAtXTics( p_ucRelativeFlag, p_unDelay );

    // Set CC2420 in RX mode
    //CC2420_SetModemState(CC2420_MODEM_STATE_RX); --> CC2420 modem is set in RX by the Timer_A interrupt after the scheduled p_unDelay elapsed

    #if (DEBUG_MODE >= 2)
    //test - signal the number of radio channel used for reception
      while(p_ucChannel--)
      {
          DEBUG3_ON();
          DEBUG3_OFF();
      }
      DEBUG3_ON(); //test - start reception
    #endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Rares Ivan
/// @brief  Enable transceiver in transmit mode on desired frequency channel (PHY_ENABLE_TX)
///                (the transmit sequence will begin after minimum of X uS
/// @param  p_ucChannel - tx channel
/// @param  p_unDelay   - uS Delay
/// @param  p_ucCCAFlag - 0 without CCA ; 1 with CCA
/// @param  p_pucData   - pointer to tx data
/// @param  p_ucLen     - tx data len (max 125 bytes)
/// @remarks
///      Access level: Interrupt Level\n
///      Obs: For any TX sequence the RF transimted bytes (symbols) are :\n
///                            Preamble                        = 4 Bytes = 8 symbols = 128 uS\n
///                            Start of Frame Delimiter (SFD)  = 1 bytes = 2 symbols = 32 uS\n
///                            PHR (frame len)                 = 1 bytes = 2 symbols = 32 uS\n
///                            PHY message ... ()              = max 125 bytes\n
///                            FCS (CRC)                       = 2 bytes = 4 symbols = 64 uS\n
///           FOR TX with CCA delays :      = X us minimum = delay\n
///                                              = X uS = CCA len\n
///                                              = X uS = from CCA back to TX front (TX seq as described above)\n
///           FOR TX without CCA delays :   = X us = delay TX front (TX seq as described above)
////////////////////////////////////////////////////////////////////////////////////////////////////
void PHY_TX_Request2( unsigned char p_ucChannel, unsigned char p_ucRelativeFlag, unsigned int p_unDelay, unsigned char p_ucCCAFlag )
{
  PHY_Disable_Request(PHY_IDLE);

  if( g_ucLastRFChannel != p_ucChannel )
  {
       // Set RF Channel
      CC2420_MACRO_SELECT();    
      CC2420_MACRO_SET_RFCH_0TO15(p_ucChannel);  // If channel is given as a channel number 0..15. LOCK_THR = 1 is also added.    
      CC2420_MACRO_RELEASE();
      
      g_ucLastRFChannel = p_ucChannel;
  }

  // Set the status -> this statud will be used when the PHY_OnTimeTriggeredAction() is called from timer interrupt
  // Based on this status, the next action is decided
  g_ucXVRStatus = (p_ucCCAFlag ? PHY_TX_WITH_CCA_START_DO : PHY_TX_START_DO);
  
  // Schedule a delayed action
  ExecAtXTics( p_ucRelativeFlag, p_unDelay );

  #if (DEBUG_MODE >= 2)
    //test - signal the number of radio channel used for transmission
    while(p_ucChannel--)
    {
        DEBUG4_ON();
        DEBUG4_OFF();
    }
    DEBUG4_ON(); //test - start transmission
  #endif
}



///////////////////////////////////////////////////////////////////////////////////
// Name:          PHY_MNG_Request
// Author:        Rares Ivan
// Description:   Implements the WirelessHART primitive :
//
//
// Parameters:
//                unsigned char p_ucService =
//                unsigned char * p_pucData =
// Return:        none
// Obs:
//      Access level:
//      Context:
///////////////////////////////////////////////////////////////////////////////////
void PHY_MNG_Request ( unsigned char p_ucService, unsigned char * p_pucData)
{
  unsigned int temp;

  switch ( p_ucService )
  {
    case PHY_MNG_RESET :
      PHY_Reset();
      // To discuss !!!
      //PHY_MNG_Confirm(PHY_MNG_RESET, PHY_MNG_SUCCES, NULL);
      break;

    case PHY_MNG_IDLE :
      PHY_Disable_Request(PHY_IDLE);
      break;

    case PHY_MNG_READ_TX_PWR_LVL :
      // Read PA power level (in range [0x00..0x1F] unsigned char)
      CC2420_MACRO_SELECT();
      CC2420_MACRO_GET_PA(temp);
      CC2420_MACRO_RELEASE();
      p_pucData[0] = (unsigned char)temp;
      break;

    case PHY_MNG_WRITE_TX_PWR_LVL :
      // New PA power level should be in range [0x00..0x1F] unsigned char
      CC2420_MACRO_SELECT();
      CC2420_MACRO_SET_PA( ((unsigned char)p_pucData[0]) );
      CC2420_MACRO_RELEASE();
      break;

    case PHY_MNG_WRITE_RX_OVERFLOW_LVL :
      // Rares: The CC2420 RXFIFO can only contain a maximum if 128 bytes at a given time. This may be divided between
      //        multiple frames as long as the total number of bytes is 128 or less.
      //
      //        If we want to have an indication when the number of unread bytes in RXFIFO excedes a threshold, the
      //        CC2420's IOCFG0.FIFOP_THR threshold may be used.
      //
      //        I recommend leaving this threshold set to maximum value (127)
      //
      /*
      CC2420_MACRO_SELECT():
        CC2420_MACRO_GETREG(CC2420_IOCFG0, temp);
        temp = (temp & 0xFF80) | ((unsigned char)p_pucData[0]);
        CC2420_MACRO_SETREG(CC2420_IOCFG0, temp);
      CC2420_MACRO_RELEASE();
      */
      break;
  }
}

//==========================================================================================================
// PHY Security Module (AES)
//==========================================================================================================

// See #define PHY_AES_*() functions in PHY.h

//==========================================================================================================

//////////////////////////////////////////////////////////////////////
// Function: RF_MODEM_SIGNALS_HANDLER
// Author: Rares Ivan
// Description: IRQ Handler for CC2420 modem signals
//////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Rares Ivan
/// @brief  IRQ Handler for CC2420 modem signals
/// @param  none
/// @return none
////////////////////////////////////////////////////////////////////////////////////////////////////
void RF_Interrupt(void)
{
  unsigned long ulPIOIsr = AT91C_BASE_PIOB->PIO_ISR; // PIO interrupt status register
  ulPIOIsr &= AT91C_BASE_PIOB->PIO_IMR; 

  if( !ulPIOIsr ) // not expected interrupt?, escape it
  {
      return;
  }
  
  if( g_ucXVRStatus == PHY_RX_IN_PROGRESS )
  {    
      // FIFO Pin Interrupt = packet begin
      if ( CC2420_FIFO_IRQ_STATUS( ulPIOIsr ) )
      {       
          g_unTimestampSFDReceived = TMR_Get250msOffset()-USEC_TO_TMR0(32);  // A new frame is comming. LEN of new frame was received
          
          if( MLSM_RX_ACK == g_ucDllTdmaStat  )
          {
              g_unRxAckSFDOffsetFract = TMR0_TO_FRACTION(g_unTimestampSFDReceived - g_unRxAckSFDOffsetFract);
          }
          
          #if (DEBUG_MODE >= 2)
              DEBUG3_OFF();  DEBUG3_ON(); 
          #endif
                    
          CC2420_FIFO_IRQOFF();
//          CC2420_FIFO_CLEAR_IRQ();  // Clear IRQ Flag          
      }
      else if (ulPIOIsr == (uint32)CC2420_SFD && CC2420_SFD_STATUS() ) // if interrupt was generated by SFD from 0 to 1, ignore it
      {
          return;
      }
      
      #if (DEBUG_MODE >= 3)
          DEBUG3_OFF();  DEBUG3_ON(); 
      #endif

      if(!MLSM_CanReceiveMsg())
      {
          PHY_ErrorIndicate( RX_GENERAL_ERROR, NULL );
          return;
      }
            
      // packet start received          
      // FIFOP Pin Interrupt = threshold (if PHY_MIN_HDR_SIZE bytes received or packet received if less than PHY_MIN_HDR_SIZE)
       if ( CC2420_FIFOP_IRQ_STATUS(ulPIOIsr) )
       {       
                    
      #if (DEBUG_MODE >= 2)
          DEBUG3_OFF();  DEBUG3_ON(); 
          DEBUG3_OFF();  DEBUG3_ON(); 
      #endif
          
          CC2420_FIFOP_IRQOFF(); 
//          CC2420_FIFOP_CLEAR_IRQ(); // Clear IRQ Flag
          
          // read also the length
          CC2420_MACRO_SELECT();
              CC2420_MACRO_GETREG( CC2420_RSSI, g_ucLastRSSI ); // read RSSI
              CC2420_MACRO_READ_RAM_LITTLE_E( (unsigned char*)g_aucDllRxTxBuf, (unsigned int)0x080, 1+PHY_MIN_HDR_SIZE+2 ); // + RSSI and CRC
          CC2420_MACRO_RELEASE();

          uint8 ucRxLen = g_aucDllRxTxBuf[0] - 2; // remove the RSSI and CRC from len

          // Rares: 802.15.4 frame len contains Payload max 125 bytes + 2 bytes FCS = max 127 bytes
          // Check the length of the incomming frame 6 <= len <= 127
          if(  (unsigned char)(ucRxLen-6) > (MAX_PHY_PAYLOAD_LEN-6)) // error on receiving
          {       
              PHY_ErrorIndicate(RX_CRC_ERROR, NULL);
              return;
          }
          
          g_aucDllRxTxBuf[0] = ucRxLen;
          if( !PHY_HeaderIndicate(g_aucDllRxTxBuf) )
          {
              PHY_ErrorIndicate(RX_CRC_ERROR, NULL);
              return;
          }
      }
            
      // SFD Pin Interrupt = packet end
      if ( CC2420_SFD_IRQ_STATUS(ulPIOIsr) )
      {               
          #if (DEBUG_MODE >= 2)
              DEBUG3_OFF();                                                     // test - end reception
          #endif
  
          PHY_Disable_Request(PHY_IDLE); // stop reception                    
                        
          // Read data from RX FIFO
          if( g_aucDllRxTxBuf[0] > PHY_MIN_HDR_SIZE ) // still have data on RX buffer
          {                        
              CC2420_MACRO_SELECT();
              CC2420_MACRO_READ_RAM_LITTLE_E( (unsigned char*)g_aucDllRxTxBuf+1+PHY_MIN_HDR_SIZE+2, 
                                             (unsigned int)0x080+1+PHY_MIN_HDR_SIZE+2, 
                                             g_aucDllRxTxBuf[0]-PHY_MIN_HDR_SIZE );
              CC2420_MACRO_RELEASE();
          }
  
          // Set the RSSI, CRCOK and CORR fields of the just received message
          // Note:
          //   In RX mode, if AUTOCRC is ON, the last 2 bytes (FCS) of the buffer are replaced by hardware with:
          //   [ 1 byte RSSI (signed) ][ 1 byte (Bit7=CRC OK, Bit6..Bit0=Correlation value (unsigned)) ]
          if ( g_aucDllRxTxBuf[  g_aucDllRxTxBuf[0] + 2 ] < 0x80 ) // CRC Error !
          {
              PHY_ErrorIndicate(RX_CRC_ERROR, NULL);
              return;
          }
  
          #if (DEBUG_MODE >= 2)
              DEBUG3_ON();                                                      // test - start indicate
          #endif
          
          memcpy( g_stWciRcvRfMsg.m_u.m_stRf.m_stDL.m_aBuff, g_aucDllRxTxBuf + 1, g_aucDllRxTxBuf[0] );
          PHY_DataIndicate( g_aucDllRxTxBuf );          
                    
          #if (DEBUG_MODE >= 2)
              DEBUG3_OFF();                                                     // test - end endicate
          #endif              
      }
  }
  else if ( g_ucXVRStatus == PHY_TX_IN_PROGRESS )
  {
      unsigned char rv;
      // Read Status byte from CC2420 (SPI)	
      CC2420_MACRO_SELECT();
        CC2420_MACRO_GET_STATUS(rv);
      CC2420_MACRO_RELEASE();

      //-----------------------------------------------------------------------------------------
      //
      // Check if TX Underflow
      //
      // Note: TX Underflow happens when too few bytes are written to the TXFIFO (the given frame
      //       len was higher than the nr of bytes written after).
      //       TX Underflow should NOT happen -> Upper layers must assure that frame len is correct
      //
      if ( (rv & (1 << CC2420_TX_UNDERFLOW)) )
      {
          // ERROR ! TX Underflow

          // Flush the TXFIFO buffer. This is also clearing the TX Underflow flag
          CC2420_MACRO_SELECT();
            CC2420_MACRO_FLUSH_TXFIFO();
            CC2420_MACRO_STROBE(CC2420_SRFOFF);        // Turn Off the RF (CC2420 goes to Idle state)
          CC2420_MACRO_RELEASE();

          PHY_Disable_Request(PHY_IDLE);
          PHY_DataConfirm(TX_BUFFER_UNDERFLOW);
          return;
      }
      //-----------------------------------------------------------------------------------------

      //-----------------------------------------------------------------------------------------
      // Is TX Action complete ?
      //
      //-----------------------------------------------------------------------------------------
      //
      // SFD Interrupt
      //
      // In TX mode, the SFD pin goes HI when SFD is detected,
      // and goes LO after last byte of frame is transmitted (FCS included)
      // It marks the completion of transmission.
      //
      if ( CC2420_SFD_IRQ_STATUS(ulPIOIsr) && !CC2420_SFD_STATUS() )
      {
            PHY_Disable_Request(PHY_IDLE);
            
            if( g_ucDllTdmaStat == MLSM_TX_ACK )
            {
                if( MLDE_DecryptRxMessage() ) // decrypt performs internaly the time settings
                {
                    DLDE_HandleRxMessage( g_aucDllRxTxBuf );
                }
            }
            else if( g_ucDllTdmaStat == MLSM_TX_NACK )
            {
                MLDE_DecryptRxMessage(); // decrypt performs internaly the time settings
            }
            else
            {
                //for RxACK SFD logging
                g_unRxAckSFDOffsetFract = TMR_Get250msOffset();

                PHY_DataConfirm(PHY_DATA_SUCCES );
            }
      }
  }
}

void PHY_AES_SetKeyInterrupt(const unsigned char* p_ucKey )
{  
  if( g_pPrevKey != p_ucKey )
  {
    g_pPrevKey = p_ucKey;
    
    CC2420_MACRO_SELECT();  
    
    // Select KEY0 as current decryption key
    CC2420_MACRO_SET_ENCKEY0(p_ucKey);   // Load the key in RAM by overwriting KEY0 location      
        
    CC2420_MACRO_RELEASE();
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Set decrypt nonce
/// @param  p_ucNonce   - pointer to RX nonce
/// @return none
/// @remarks
///      Access level: Interrupt Level
///      Context: 
////////////////////////////////////////////////////////////////////////////////////////////////////
void PHY_AES_SetDecryptNonce( const unsigned char* p_ucNonce )
{
  CC2420_MACRO_SELECT();
    CC2420_MACRO_SET_RX_NONCE(0x09,p_ucNonce,0x00,0x01);            // Load the Nonce in RAM to RXNONCE  
  CC2420_MACRO_RELEASE();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Set decrypt RX plain text length
/// @param  p_ucPlainRx - RX plain length
/// @return none
/// @remarks
///      Access level: Interrupt Level
////////////////////////////////////////////////////////////////////////////////////////////////////
void PHY_AES_SetDecryptPlainLen( unsigned char p_ucPlainRx )
{
  CC2420_MACRO_SELECT();
    CC2420_MACRO_SET_AES_RX_PLAINTEXT(p_ucPlainRx); // Set the # of plaintext bytes between frame len and 1rst byte to decrypt      
  CC2420_MACRO_RELEASE();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Set encrypt nonce
/// @param  p_ucNonce   - pointer to TX nonce
/// @param  p_ucPlainRx - TX plain length
/// @return none
/// @remarks
///      Access level: Interrupt Level
////////////////////////////////////////////////////////////////////////////////////////////////////
void PHY_AES_SetEncryptNonce( const unsigned char* p_ucNonce, unsigned char p_ucPlainTx )
{
  CC2420_MACRO_SELECT();
    CC2420_MACRO_SET_AES_TX_PLAINTEXT(p_ucPlainTx); // Set the # of plaintext bytes between frame len and 1rst byte to decrypt      
    CC2420_MACRO_SET_TX_NONCE(0x09,p_ucNonce,0x00,0x01);            // Load the Nonce in RAM to TXNONCE  
  CC2420_MACRO_RELEASE();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Rares Ivan
/// @brief  Decrypt a given buffer, using a given key
/// @param  p_ucKey         
/// @param  p_pucNonce      
/// @param  p_pucToAuthBuf 
/// @param  p_unToAuthOnlyLen
/// @param  p_pucToDecryptBuff
/// @param  p_unToDecryptLen  
/// @return AES_ERROR if fail, AES_SUCCESS if success
/// @remarks
///      Access level: -
///      Obs: Decrypted data replaces the original data in source buffer\n
///           Decryption is done in RXFIFO ! Any pre-existing data in RXFIFO will be lost !\n
///           Decryption can be done only during RF IDLE times.
////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned char  PHY_AES_DecryptUser( const unsigned char* p_ucKey,
                                    const unsigned char* p_pucNonce,
                                    const unsigned char* p_pucToAuthBuf,
                                    unsigned int   p_unToAuthOnlyLen,
                                    unsigned char* p_pucToDecryptBuff,
                                    unsigned int   p_unToDecryptLen,
                                    unsigned char  micSize 
                                 )
{
  if (p_unToDecryptLen < micSize )
    return AES_ERROR;
  
  p_unToDecryptLen -= micSize;
  
  unsigned int i;
  uint8  Todelete = 0;
  
  struct 
  {
      unsigned int m_unBlockLen;
      uint8  m_aInput[16] ;
      uint8  m_aCipherText[16] ;
      uint8  m_aA0[16]    ;
  } st; // put that format to be aligned
  
  
  SET_AES_USER_ON_PROGRESS( 1 );  
  PHY_SetToIdle(); // wake up if modem is down
  
    CC2420_SetKeySA( p_ucKey );

    // building B0 = Flags || Nonce N(13) || l(m)         
    st.m_aA0[0]  = (p_unToAuthOnlyLen ? 0x49 : 0x09); 
    memcpy( st.m_aA0+1, p_pucNonce, 13 );
    *(uint16*)(st.m_aA0+14) = __swap_bytes( p_unToDecryptLen );
    
     if(micSize == 8)   { st.m_aA0[0] = 0x59;  }
     if(micSize == 16)  { st.m_aA0[0] = 0x79;  }
    // encrypt the block using the expandedKey    
    CC2420_ComputeAES(  st.m_aA0,  st.m_aInput );     
    
    // building B1, B2, B3,...
    *(uint16*)(st.m_aInput) ^= __swap_bytes( p_unToAuthOnlyLen );
      
    st.m_unBlockLen = ( p_unToAuthOnlyLen > 14 ? 14 : p_unToAuthOnlyLen);
    for (i = 0; i < st.m_unBlockLen; i++)
    {
        st.m_aInput[i+2] ^= *(p_pucToAuthBuf++);
    }
    
    CC2420_ComputeAES( st.m_aInput, st.m_aInput );
   
    p_unToAuthOnlyLen -= st.m_unBlockLen;
    
    while(  p_unToAuthOnlyLen )
    { 
         st.m_unBlockLen = ( p_unToAuthOnlyLen > 16 ? 16 : p_unToAuthOnlyLen);
         p_unToAuthOnlyLen -= st.m_unBlockLen;
         // xor-ing with B
         for (i = 0; i < st.m_unBlockLen; i++)
         {
            st.m_aInput[i] ^= *(p_pucToAuthBuf++);
         }
           
         // encrypt the block using the expandedKey    
         CC2420_ComputeAES( st.m_aInput, st.m_aInput );
    }
        
    st.m_aA0[0]  = 0x01;
    *(uint16*)(st.m_aA0+14) = 0x0100; // 0x00 0x01 (network order)
        
    while(  p_unToDecryptLen )
    {
      st.m_unBlockLen = ( p_unToDecryptLen > 16 ? 16 : p_unToDecryptLen);
      p_unToDecryptLen -= st.m_unBlockLen;
      
      // encrypt the block using the expandedKey 
      CC2420_ComputeAES( st.m_aA0, st.m_aCipherText );
      
      // xor-ing with M
      for (i = 0; i < st.m_unBlockLen; i++)
      {
           uint8 ucTmp = *p_pucToDecryptBuff ^ st.m_aCipherText[i];
           st.m_aInput[i] ^= ucTmp;
           *(p_pucToDecryptBuff++) = ucTmp;
      }

      CC2420_ComputeAES( st.m_aInput, st.m_aInput ); 
      
      st.m_aA0[15] ++;
    }
    
    // building A0 
    st.m_aA0[15] = 0;       
    
    // building block S0
    CC2420_ComputeAES( st.m_aA0, st.m_aCipherText );
                 
    SET_AES_USER_ON_PROGRESS( 0 );  

// The encrypted authentication tag U is the result of XOR-ing the string consisting of the leftmost M octets of S0 and the authentication tag T
    

    
    for( i = 0; i < micSize; i++)
    {
      Todelete = (*(p_pucToDecryptBuff++) ^ st.m_aCipherText[i]); 
      if( st.m_aInput[i] !=  Todelete )
       {
          return AES_ERROR;
       }
     }
    
 return AES_SUCCESS;
} 

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Rares Ivan
/// @brief  Encrypt a given buffer, using a given key
/// @param  p_ucKey         
/// @param  p_pucNonce      
/// @param  p_pucToAuthBuf 
/// @param  p_unToAuthOnlyLen
/// @param  p_pucToEncryptBuff
/// @param  p_unToEncryptLen  
/// @return AES_ERROR if fail, AES_SUCCESS if success
/// @remarks
///      Access level: -
///      Obs: Encrypted data replaces the original data in source buffer\n
///           Encryption is done in TXFIFO ! Any pre-existing data in TXFIFO will be lost !\n
///           Encryption can be done only during RF IDLE times.
////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned char PHY_AES_EncryptUser( const unsigned char* p_ucKey,
                                   const unsigned char* p_pucNonce,
                                   const unsigned char* p_pucToAuthBuf,
                                   unsigned int   p_unToAuthOnlyLen,
                                   unsigned char* p_pucToEncryptBuff,
                                   unsigned int   p_unToEncryptLen,
                                   unsigned char micSize
                                 )
{  
    unsigned int i;
    
    struct 
    {
        unsigned int  m_unBlockLen;
        uint8         m_aInput[16] ;
        uint8         m_aCipherText[16] ;
        uint8         m_aA0[16]    ;
    } st; // put that format to be aligned
            
    SET_AES_USER_ON_PROGRESS( 1 );  
    PHY_SetToIdle(); // wake up if modem is down
    
    CC2420_SetKeySA( p_ucKey );
    
    // building B0 = Flags || Nonce N(13) || l(m)         
    st.m_aA0[0]  = (p_unToAuthOnlyLen ? 0x49 : 0x09); 
    memcpy( st.m_aA0+1, p_pucNonce, 13 );
    *(uint16*)(st.m_aA0+14) = __swap_bytes( p_unToEncryptLen );

    if(micSize == 8)   {      st.m_aA0[0] = 0x59;   }
    if(micSize == 16)  { st.m_aA0[0] = 0x79;  }
    // encrypt the block using the expandedKey    
    CC2420_ComputeAES(  st.m_aA0,  st.m_aInput );     
    
    // building B1, B2, B3,...
    *(uint16*)(st.m_aInput) ^= __swap_bytes( p_unToAuthOnlyLen );
      
    st.m_unBlockLen = ( p_unToAuthOnlyLen > 14 ? 14 : p_unToAuthOnlyLen);
    for (i = 0; i < st.m_unBlockLen; i++)
    {
        st.m_aInput[i+2] ^= *(p_pucToAuthBuf++);
    }
    
    CC2420_ComputeAES( st.m_aInput, st.m_aInput );
   
    p_unToAuthOnlyLen -= st.m_unBlockLen;
    
    while(  p_unToAuthOnlyLen )
    { 
         st.m_unBlockLen = ( p_unToAuthOnlyLen > 16 ? 16 : p_unToAuthOnlyLen);
         p_unToAuthOnlyLen -= st.m_unBlockLen;
         // xor-ing with B
         for (i = 0; i < st.m_unBlockLen; i++)
         {
            st.m_aInput[i] ^= *(p_pucToAuthBuf++);
         }
           
         // encrypt the block using the expandedKey    
         CC2420_ComputeAES( st.m_aInput, st.m_aInput );
    }
    
    
    // start encryption (CCM)
     
// The authentication tag T is the result of omitting all but the leftmost M octets of aInput

    // building M1, M2, M3,...
    st.m_aA0[0]  = 0x01;
    *(uint16*)(st.m_aA0+14) = 0x0100; // 0x00 0x01 (network order)
     
    while(  p_unToEncryptLen )
    {
         st.m_unBlockLen = ( p_unToEncryptLen > 16 ? 16 : p_unToEncryptLen);
         p_unToEncryptLen -= st.m_unBlockLen;
           
         // encrypt the block using the expandedKey    
         CC2420_ComputeAES( st.m_aA0, st.m_aCipherText );
         
         // xor-ing with B
         for (i = 0; i < st.m_unBlockLen; i++)
         {
            st.m_aInput[i] ^= *(p_pucToEncryptBuff);
            *(p_pucToEncryptBuff++) ^= st.m_aCipherText[i];
         }
         
         // compute MIC
           CC2420_ComputeAES( st.m_aInput, st.m_aInput ); 
         
         st.m_aA0[15] ++;       
    }
    
// The string Ciphertext is the result of omitting all but the leftmost l(m) octets of the string C1 || … || Ct.

    // building A0 
    st.m_aA0[15] = 0;       
    
    // building block S0
     CC2420_ComputeAES( st.m_aA0, st.m_aCipherText );

// The encrypted authentication tag U is the result of XOR-ing the string consisting of the leftmost M octets of S0 and the authentication tag T
    for ( i = 0; i < micSize; i++)
    {
      *(p_pucToEncryptBuff++) = st.m_aInput[i] ^ st.m_aCipherText[i];
    }
     
// Output the right-concatenation c of the encrypted message Ciphertext ( = *(p_pucToEncrypt++) ^ aCipherText[i]) and the encrypted authentication tag U (= aInput[i] ^ aCipherText[i])
    
    SET_AES_USER_ON_PROGRESS( 0 );  
    
    return AES_SUCCESS;
}



#ifdef LOW_POWER_DEVICE
  __monitor void PHY_SetToIdle(void)
  {
      MONITOR_ENTER();
      if( g_ucModemState == CC2420_MODEM_STATE_POWERDOWN )
      {
          g_ucXVRStatus = 0xFF;           // 
          PHY_Disable_Request(PHY_IDLE);
      }
      MONITOR_EXIT();
  }

  void PHY_WakeUpRequest( void )
  {
    g_ucXVRStatus = PHY_IDLE;
    ExecAtXTics( ABSOLUTE_DELAY, g_unDllTMR2SlotLength - FRACTION_TO_TMR2(3*1024) ); // 3 ms for wake up
  }
#endif    


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Implementation of the Matyas-Meyer-Oseas hash function with 16 byte block length 
/// @param  p_pucInputBuff - the input buffer with length in bytes not smaller than 16 bytes  
/// @param  p_unInputBuffLen - length in bytes of the input buffer
/// @return SFC_FAILURE if fail, SFC_SUCCESS if success 
/// @remarks
///      Access level: User\n
///      Obs: result data replaces the first "MAX_HMAC_INPUT_SIZE" bytes from original source buffer
uint8 Crypto_Hash_Function( const uint8 * p_pucKey,
                            uint8  p_ucXOR,
                            uint8* p_pucInputBuff,
                            uint16 p_unInputBuffLen
                              )
{  
  struct
  {
      uint8*        m_pucBuffRes;
      uint8         m_aucInput[HASH_DATA_BLOCK_LEN];
      uint8         m_aucOutput[HASH_DATA_BLOCK_LEN];
  } st; // used to be aligned
  
  unsigned int  unIdx;
  
  uint16        unInitialLen = p_unInputBuffLen+HASH_DATA_BLOCK_LEN;
  uint8         ucNextBlock = 0;
  uint8         ucLastBlock = 0;  

  st.m_pucBuffRes = p_pucInputBuff;
  
  //the key for the first algorithm iteration need to be full 0 
  memset(st.m_aucOutput, 0x00, HASH_DATA_BLOCK_LEN);
  
  SET_AES_USER_ON_PROGRESS( 1 );  
  PHY_SetToIdle(); // wake up if modem is down

  while(!ucLastBlock)
  {
    CC2420_SetKeySA( st.m_aucOutput );

    if( p_pucKey ) // first block is the key XOR p_ucXOR
    {
        for(unIdx = 0; unIdx<HASH_DATA_BLOCK_LEN; unIdx++)
        {
            st.m_aucInput[unIdx] = p_pucKey[unIdx] ^ p_ucXOR;
        }
        p_pucKey = NULL;
    }
    else if( p_unInputBuffLen >= HASH_DATA_BLOCK_LEN )
    {
        memcpy(st.m_aucInput, p_pucInputBuff, HASH_DATA_BLOCK_LEN);  
        
        p_unInputBuffLen -= HASH_DATA_BLOCK_LEN;
        p_pucInputBuff += HASH_DATA_BLOCK_LEN;
    }  
    else
    {
        memset(st.m_aucInput, 0x00, HASH_DATA_BLOCK_LEN);
        st.m_aucInput[p_unInputBuffLen] = 0x80;   //the bit7 of the padding's first byte must be "1"
        
        if( !p_unInputBuffLen )
        {
            //last iteration
            ucLastBlock = 1;   
        }
        else
        {
            //+1 - concatenation of the input buffer with first byte 0x80
            //+2 - adding the last 2 bytes representing the buffer length info - particular case for 16 bytes data block len
            if( HASH_DATA_BLOCK_LEN - 3 >= p_unInputBuffLen)
            {
                memcpy(st.m_aucInput, p_pucInputBuff, p_unInputBuffLen);
                //last iteration
                ucLastBlock = 1;
            }
            else if( !ucNextBlock && (HASH_DATA_BLOCK_LEN - 2 <= p_unInputBuffLen) )
            {
                memcpy(st.m_aucInput, p_pucInputBuff, p_unInputBuffLen);
                ucNextBlock = 1;
            }
            else
            {
                st.m_aucInput[p_unInputBuffLen] = 0x00;  
                ucLastBlock = 1;      
            }
        }
        
        if( ucLastBlock )
        {
            //it's the last block
            st.m_aucInput[HASH_DATA_BLOCK_LEN - 2] = unInitialLen >> 5;  //the MSB of bits number inside the input buffer
            st.m_aucInput[HASH_DATA_BLOCK_LEN - 1] = unInitialLen << 3;  //the LSB of bits number inside the input buffer
        }
    }
    
    // encrypt the block using the expandedKey    
    CC2420_ComputeAES(st.m_aucInput, st.m_aucOutput);
    
    // xor-ing with Me
    for (unIdx = 0; unIdx < HASH_DATA_BLOCK_LEN; unIdx += sizeof(unsigned int) )
    {
      *(unsigned int*)(st.m_aucOutput+unIdx) ^= *(unsigned int*)(st.m_aucInput+unIdx);
    }
  }
  
  SET_AES_USER_ON_PROGRESS( 0 );  
  
    //save the result data over the original buffer
  memcpy(st.m_pucBuffRes, st.m_aucOutput, HASH_DATA_BLOCK_LEN);
  return SFC_SUCCESS;
}

uint8 Keyed_Hash_MAC(const uint8* p_pucKey, 
                  uint8* p_pucInputBuff,
                  uint16 p_unInputBuffLen
                  )
{
  //the result having HASH_DATA_BLOCK_LEN length will overwrite the input buffer
  if( p_unInputBuffLen < HASH_DATA_BLOCK_LEN )
    return SFC_FAILURE;
  
  
  Crypto_Hash_Function(p_pucKey, 0x36, p_pucInputBuff, p_unInputBuffLen );      
  Crypto_Hash_Function(p_pucKey, 0x5C, p_pucInputBuff, HASH_DATA_BLOCK_LEN );  
  
  return SFC_SUCCESS;
}

uint8 PHY_IsRXOnChannel(uint8 p_ucChannel )
{
    return (g_ucLastRFChannel == p_ucChannel && g_ucXVRStatus == PHY_RX_IN_PROGRESS);
}

