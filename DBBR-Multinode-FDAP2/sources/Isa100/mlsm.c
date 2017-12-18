/***************************************************************************************************
* Name:         mlsm.c
* Author:       Nivis LLC, Doinel Alban
* Date:         January 2008
* Description:  MAC Extension Layer Data Entity
* Changes:
* Revisions:
****************************************************************************************************/

#include "tmr_util.h"
#include "mlde.h"
#include "mlsm.h"
#include "dmap.h"
#include "sfc.h"


#define TX_JITTER_USEC 100


#define DAUX_SYNC_MIN_SECONDS         4
//#define MODM_DEEP_SLEEP_WAKE_UP_TIME  3 // counted on slots tics (maybe must be dependend of slot length

#define ADV_RX_250MS_STEPS_NO     6   //reception period/channel used by joining device to receive one Advertisment
                                      //1.5 seconds per channel(channel sequence D having 3 channels length)

#define RX_RSSI_SIGNAL_LEVEL_REFERENCE     (int8)-77     //CC2420 CCA Threshold is set to -77 dBm
uint8 g_ucResetPhyChip = 0;
uint16 g_unDllTMR2SlotLength;
uint16 unClockDrift =0;
uint8  s_ucPrevCorrection;
uint16 g_unTxSFDFraction;          // save the time of transmitted packet's SFD (from slot start) in fraction counts
uint16 g_unDllTimeslotLength;   // 2^(-20) sec; same value for all active superframes
#if  (DEVICE_TYPE != DEV_TYPE_MC13225)
  uint16 g_unRandValue;           //a pre-generated random value for interrupt using
#endif
  
extern uint8 g_ucAdvTimeSync;
TAI_STRUCT  g_stTAI;
SLOT_STRUCT g_stSlot;

//global variables used by DLL state machine
uint8  g_ucDllTdmaStat = 0;
uint8  g_ucDllCrtCh;                // current used radio channel
uint8  g_ucDllRestoreSfOffset;      // =1 when superframe offsets must be recalculated from TAI cutover

// global variables for neighbor diagnostic purposes
uint8  g_ucSlotExecutionStatus;     // DPDU received, TxSuccess, TxFailed, etc

uint8 g_ucIdleLinkIdx;    // used to identify an idle link that needs to be activated on DAUX (after ack)
int16 g_nClockDriftTMR;
uint8 g_ucClockDriftStatus;

#ifndef BACKBONE_SUPPORT
  void activateDiscoveryRxMode(void);  
  static void MLSM_updateClockDiagnostics(int32 p_lBias, uint8 p_NeighIdx);
#endif // not BACKBONE_SUPPORT
static uint8 MLSM_getClockUpdateApproval( uint8 p_ucNeighborIdx );
  
static void MLSM_saveStatistics(void);  
static void MLSM_updateSignalStatus(uint8 p_NeighIdx);
static void MLSM_checkSfIdleTimers(void);
static uint8 MLSM_getCrtChannel(void);
static void MLSM_checkECNandNACKTimers(void);
static void MLSM_chekIdleLinkTimers(void);
void MLSM_refreshOffsetForAllSf (void);

//------------------ MAC State Machine functions -----------------------------------

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Doinel Alban
/// @brief  Extracts radio channel from specified channel hopping sequence and index
/// @param  p_pChTable - pointer to m_aSequence[] into a DLL_SMIB_CHANNEL struct
/// @param  p_ucChIdx  - channel index
/// @return channel number
/// @remarks  
///      Access level: Interrupt level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 MLSM_getChFromTable( const uint8 * p_pChTable, uint8 p_ucChIdx )
{
    uint8 ucCh = p_pChTable[p_ucChIdx / 2];
    if((p_ucChIdx & 0x01) == 0) // first channel value is on the lower nibble
    {
        return (ucCh & 0x0F);
    }
    return (ucCh >> 4);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Doinel Alban
/// @brief  Selects the channel to be used in the current slot
/// @param  none
/// @return channel number
/// @remarks  
///      Access level: Interrupt level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 MLSM_getCrtChannel( void )
{
  DLL_SMIB_SUPERFRAME * pSf = &g_aDllSuperframesTable[g_pCrtHashEntry->m_ucSuperframeIdx].m_stSuperframe;
  uint8 ucCh = pSf->m_ucCrtChOffset; // ChOffset
  
  if (g_aDllLinksTable[g_pCrtHashEntry->m_ucLinkIdx].m_stLink.m_mfTypeOptions & DLL_MASK_LNKCH)
  {   
      // used link has an additional channel offset
      ucCh += g_aDllLinksTable[g_pCrtHashEntry->m_ucLinkIdx].m_stLink.m_ucChOffset;
      ucCh %= pSf->m_ucChSeqLen;  // m_ucSequenceSize is 1-16 , not 0-15!!!
  }
  
  return MLSM_GetChFromTable( pSf->m_aChSeq, ucCh );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Increments/decrements idle timer of superframes.
/// @param  none
/// @return none
/// @remarks  
///      Access level: Interrupt level\n
///      Context: This function must be called once/second
////////////////////////////////////////////////////////////////////////////////////////////////////
void MLSM_checkSfIdleTimers(void)
{
  unsigned int unIdx = g_ucDllSuperframesNo;
  DLL_SMIB_ENTRY_SUPERFRAME * pSf = g_aDllSuperframesTable;
  
  for (; unIdx; unIdx--, pSf++) // for all current superframes
  { 
      if( pSf->m_stSuperframe.m_ucInfo & DLL_MASK_SFIDLE )
      {
          if ( pSf->m_stSuperframe.m_lIdleTimer > 0 )
          {
              pSf->m_stSuperframe.m_lIdleTimer--;
              
              if( 0 == pSf->m_stSuperframe.m_lIdleTimer )
              {
                  g_ucHashRefreshFlag = 1; // need to recompute the hash table   
              }
          }
          else if( pSf->m_stSuperframe.m_lIdleTimer < -1 )
          {
              pSf->m_stSuperframe.m_lIdleTimer++;
              
              if( -1 == pSf->m_stSuperframe.m_lIdleTimer )
              {
                  g_ucHashRefreshFlag = 1; // need to recompute the hash table   
              }
          }
      }   
  } 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Dorin Pavel, Eduard Erdei
/// @brief  This implements the incremental approach of computing current 
///              superframe offsets; the incremental update approach will not work 
///              when there is a change in any field used in the base calculation,
///              or in fields in other attributes (dlmo11a.Ch[] in general, and 
///              dlmo11a.Link[] for SfType=2) that are used in the base calculation
/// @param  p_pSuperFrame - pointer to a superframe entry
/// @return none
/// @remarks  
///      Access level: Interrupt level
////////////////////////////////////////////////////////////////////////////////////////////////////
void MLSM_incSuperframe (DLL_SMIB_SUPERFRAME * p_pSuperFrame )
{
  // todo: implmenet SfType 1..3
  
  // increment SfOffset
  if( (++ (p_pSuperFrame->m_unCrtOffset) ) >= p_pSuperFrame->m_unSfPeriod )
  {
      p_pSuperFrame->m_unCrtOffset = 0;
  }

  if ( p_pSuperFrame->m_unChRate > 1)
  {
      if( (++ (p_pSuperFrame->m_unCrtSlowHopOffset) ) < p_pSuperFrame->m_unChRate )
      {
          return;
      }
      p_pSuperFrame->m_unCrtSlowHopOffset = 0;
  }

   if( (++ (p_pSuperFrame->m_ucCrtChOffset)) >= p_pSuperFrame->m_ucChSeqLen )
  {
      p_pSuperFrame->m_ucCrtChOffset = 0;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Dorin Pavel, Eduard Erdei
/// @brief  Implements the non-incremental approach of the current superframe 
///              offsets calculations. Updates m_unCrtOffset, m_ucCrtChOffset, 
///              for a certain superframe. Calculaton is made from TAI=0 or soon 
///              thereafter, using SfBirth
/// @param  p_pSuperFrame - pointer to a superframe entry
/// @return none
/// @remarks  
///      Access level: Interrupt level\n
///      Context: Called from timer interrupt at slot beginning, only when something 
///               has changed in the DLL settings (channels, superframes, etc)
////////////////////////////////////////////////////////////////////////////////////////////////////
static void MLSM_rebuildSuperframe (DLL_SMIB_ENTRY_SUPERFRAME * p_pSuperFrame )
{
    // todo: implement sfType=1..3 (only sfType=0 implemented)
    uint32 ulValue;
    
    // assume that the "first" superframe cycle started at ASN=0 TAI=0 (sfBirth=0) 
    // the initial superframe's offsets at TAI=0 are the same as a few seconds ago; these seconds are computed as:
    ulValue = g_ulDllTaiSeconds % p_pSuperFrame->m_stSuperframe.m_unSfPeriod;     
    
    // compute the number of timeslots that fits into the previously calculated seconds + 1 to 3 quarter seconds
    ulValue = ((ulValue * 4) + g_stTAI.m_uc250msStep) * g_stSlot.m_ucsMax250mSlotNo;
    
    // add the number of timeslots within the current quarter second
    ulValue += g_stSlot.m_uc250msSlotNo;
    
    // compute current offset considering also the SfBirth, that was ignored until now
    // consider m_unSfBirth < m_unSfPeriod (don't make sense otherwise)
    ulValue += p_pSuperFrame->m_stSuperframe.m_unSfPeriod; // just for protection because a substraction will follow
    ulValue -= p_pSuperFrame->m_stSuperframe.m_unSfBirth;
    p_pSuperFrame->m_stSuperframe.m_unCrtOffset = ulValue % p_pSuperFrame->m_stSuperframe.m_unSfPeriod;
    
    // compute the current channel offset, using the same approach as above
    uint32 ulHopSeqPeriod = p_pSuperFrame->m_stSuperframe.m_ucChSeqLen;     
    if( p_pSuperFrame->m_stSuperframe.m_unChRate > 1 )
    {
        ulHopSeqPeriod *= p_pSuperFrame->m_stSuperframe.m_unChRate; // hop seq period expressed in timeslot numbers   
    }

    ulValue = g_ulDllTaiSeconds % ulHopSeqPeriod;
    ulValue = ((ulValue * 4) + g_stTAI.m_uc250msStep) * g_stSlot.m_ucsMax250mSlotNo;
    ulValue += g_stSlot.m_uc250msSlotNo;

    // consider m_ucChBirth < m_ucChLen * m_unChRate (don't make sense otherwise)
    ulValue += ulHopSeqPeriod; 
    ulValue -= p_pSuperFrame->m_stSuperframe.m_ucChBirth; 
    p_pSuperFrame->m_stSuperframe.m_ucCrtChOffset = ulValue % ulHopSeqPeriod;
    
    if (p_pSuperFrame->m_stSuperframe.m_unChRate > 1) // slow hopping
    {              
        p_pSuperFrame->m_stSuperframe.m_unCrtSlowHopOffset = ulValue % p_pSuperFrame->m_stSuperframe.m_unChRate;
        p_pSuperFrame->m_stSuperframe.m_ucCrtChOffset /= p_pSuperFrame->m_stSuperframe.m_unChRate;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Dorin Pavel
/// @brief  Refresh SfOffset, SfSlowOffset, SfChOffset and SfIdleTimer for all superframe entries
/// @param  none
/// @return none
/// @remarks  
///      Access level: Interrupt level\n
///      Context: Called from timer interrupt at slot beginning
////////////////////////////////////////////////////////////////////////////////////////////////////
void MLSM_refreshOffsetForAllSf (void)
{
  unsigned int unIdx = g_ucDllSuperframesNo;
  DLL_SMIB_ENTRY_SUPERFRAME * pSf = g_aDllSuperframesTable;

  if (!g_ucDllRestoreSfOffset)
  {
      for (; unIdx; unIdx--, pSf++ )
      { // for all current superframes
          MLSM_incSuperframe( &pSf->m_stSuperframe );
      }
  }
  else
  {
      g_ucDllRestoreSfOffset=0;

      for (; unIdx; unIdx--, pSf++ )
      {
          MLSM_rebuildSuperframe( pSf );
      }
  }
}

uint16 g_unTxAckCount, g_unTxNakCount, g_unTxCount;

//------------------------------------- TAI timebase functions --------------------------------

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Doinel Alban 
/// @brief  Performs the necessary updates at the begginning of a new slot 
/// @param  p_unFirstSlot - flag if is first slot on 250ms interval
/// @return none
/// @remarks  
///      Access level: Interrupt level
////////////////////////////////////////////////////////////////////////////////////////////////////
void MLSM_OnTimeSlotStart( unsigned int p_unFirstSlot )
{
    g_ucIdleLinkIdx  = 0xFF; 
    uint8 ucFrameLen;
    
   #if (DEBUG_MODE >= 4)
      DEBUG4_ON();// test - timeslot start
      DEBUG4_OFF();
   #endif
      
    switch(g_ucDllTdmaStat)
    {
        case MLSM_RX_ACK:   // RX_ACK timeout (from previously timeslot)                      
        case MLSM_TX_FRAME: // TX timeout - maybe HW problem or maybe send too late ...              
            DLDE_AckInterpreter( SFC_TIMEOUT, NULL ); // called for both MLSM_RX_ACK and MLSM_TX_FRAME
            // for update msg queue retry counters           
            // todo: only for unicast transmission; save info for neighbor diagnostics
            g_ucSlotExecutionStatus = SLOT_STS_TX_ACK_TIMEOUT;
        
        //  case MLSM_TX_ACK:   // TX timeout - maybe HW problem or maybe send too late ...
        //              PHY_Disable_Request();
        //              g_ucDllTdmaStat = MLSM_START_SLOT;
        //              break;
        
        //  case MLSM_RX_FRAME: // RX_FRAME timeout (from previously timeslot)
        //              if(g_ucChDiagEnabled && g_stChDiag[g_ucDllCrtCh].m_ucTxCCABackoff < 0xff )
        //              {
        //                  if( PHY_IsBusyChannel() )
        //                    g_stChDiag[g_ucDllCrtCh].m_ucTxCCABackoff++;
        //              }
        //TODO: decide what to do if the receiver is still on from the previous RX slot
        //   (slow hopping? with DllTstRespectBoundary=0 and a SFD already received)
    }

    // save the statistics info from previous timeslot
    MLSM_saveStatistics();
    
    if( p_unFirstSlot )
    {
        DLME_ParseMsgQueue();
        if( g_ucHashRefreshFlag )
        {
            g_ucHashRefreshFlag = 0;
            
    #ifdef PREDICTIVE_SCHEDULE_LINK  
            g_stNextLink.m_unSlotsUntilWakeUp = 0;
    #endif      
            DLME_RebuildHashTable();
            PHY_Disable_Request(PHY_IDLE);
        }
                
        if( 0 == g_stTAI.m_uc250msStep )// start a new sec
        {
            MLSM_checkSfIdleTimers();   // performs increment/decrement of sf idle timer
            DLDE_CheckDLLQueueTTL();    // performs cleaning of DLL queue entries that have stayed for too long time on the queue
        }    
      
        if( g_stSlot.m_uc250msSlotNo != (g_stSlot.m_ucsMax250mSlotNo-1) ) // need realign
        {
            // rebuild slots
            g_ucDllRestoreSfOffset = 1;
        }
        g_stSlot.m_uc250msSlotNo = 0;
    }
    
    MLSM_refreshOffsetForAllSf();   // refresh SfChOffset, SfOffset, SfSlowOffset, SfIdleTimer        
    
    // clean slot variables (from that point ACK / NACK are not used)
    g_ucDllECNStatus = 0;
    
    uint16 unDelay;
    uint8  ucSlotType;
            
  
#ifdef PREDICTIVE_SCHEDULE_LINK  
    g_stNextLink.m_ucSlotAtComputedTime = g_stSlot.m_uc250msSlotNo;
    
    if (g_stNextLink.m_unSlotsUntilWakeUp)
    {
        g_stNextLink.m_unSlotsUntilWakeUp--;
    }       
    
    if( g_stNextLink.m_unSlotsUntilWakeUp > 1 )
    {
        g_ucDllTdmaStat = MLSM_START_SLOT;
        
        PHY_Disable_Request(PHY_IDLE);
        PHY_Disable_Request(PHY_DISABLED);
        return; // keep modem on sleep mode
    }    
    
    if( !g_ulRadioSleepCounter ) 
    {
        g_stNextLink.m_unSlotsUntilWakeUp = 0;
        ucSlotType = DLDE_ScheduleLink();
    }
    else
    {
        g_stNextLink.m_unSlotsUntilWakeUp = g_stSlot.m_ucsMax250mSlotNo;
        ucSlotType = SFC_NO_LINK;
    }
    if( ucSlotType == SFC_NO_LINK ) // not link on current slot
    {
        g_ucDllTdmaStat = MLSM_START_SLOT;
        
        PHY_Disable_Request(PHY_IDLE);
        
        if(g_stNextLink.m_unSlotsUntilWakeUp > 1) 
        {            
            PHY_Disable_Request(PHY_DISABLED);
            return; // false alarm, keep modem on sleep mode
        }
        
        PHY_WakeUpRequest(); // wake up next time
        return;          
    }                  
    // wake up modem if necessary
    if( g_ucModemState == CC2420_MODEM_STATE_POWERDOWN ) // speed up the PHY_SetToIdle()
    {
        g_ucXVRStatus = 0xFF;           // 
        PHY_Disable_Request(PHY_IDLE);
    }
#else 
    
    if( !g_ulRadioSleepCounter ) 
    {
        ucSlotType = DLDE_ScheduleLink();
    }
    else
    {
        ucSlotType = SFC_NO_LINK;
    }
    
#endif
    
    // find out the radio channel to be used during this slot
    g_ucDllCrtCh = MLSM_getCrtChannel();
        
    if( (ucSlotType == SFC_RX_LINK) && (g_ucDllTdmaStat == MLSM_RX_FRAME) && PHY_IsRXOnChannel(g_ucDllCrtCh) )
    {
        return; // RX on same channel, stay on it
    }
    
    g_ucDllTdmaStat = MLSM_START_SLOT;        
    PHY_Disable_Request(PHY_IDLE);

    if( !((~g_unIdleChannels & c_stCapability.m_unChannelMap ) & (1 << g_ucDllCrtCh)) )
    {
        return;
    }
    
#ifdef RX_TX_LINK_COEXISTENCE
     g_pCandidateHashEntry = NULL;
#endif
    
    if( ucSlotType == SFC_TX_LINK )
    {
        DLL_TIMESLOT_TRANSMIT * pCrtTemplate;
        uint16 unRandomDelay = 0;
        
        pCrtTemplate = &(g_aDllTimeslotsTable[g_pCrtHashEntry->m_ucTemplateIdx].m_stTimeslot.m_utTemplate.m_stTx);
        
#ifdef RANDOM_TX_START
        unRandomDelay = pCrtTemplate->m_unXmitLatest - pCrtTemplate->m_unXmitEarliest - 2 * USEC_TO_FRACTION( TX_JITTER_USEC ) - 1;
        if( (unRandomDelay < 0x8000) // valid interval
            && ((g_pCrtHashEntry->m_ucLinkType & DLL_MASK_LNKDISC) != (DLL_LNKDISC_BURST << DLL_ROT_LNKDISC)) ) // not burst advertise link
        {
            unRandomDelay = g_unRandValue % (unRandomDelay+1);
        }
        else
        {        
            unRandomDelay = 0;
        }
#endif        
        
        g_unTxSFDFraction = pCrtTemplate->m_unXmitEarliest + USEC_TO_FRACTION( TX_JITTER_USEC ) + unRandomDelay; // add the jitter
        
        ucFrameLen = MLDE_OutgoingFrame();
                
        if(ucFrameLen > PHY_BUFFERS_LENGTH)
        { // error
            SET_DebugLastAck( SFC_INVALID_SIZE );  
            DLDE_AckInterpreter( SFC_INVALID_SIZE, NULL );                     
            return;
        }
        
#ifdef RX_TX_LINK_COEXISTENCE        
        if(  unRandomDelay &&
            ( g_aDllLinksTable[g_pCrtHashEntry->m_ucLinkIdx].m_stLink.m_mfLinkType & (DLL_MASK_LNKTX |  DLL_MASK_LNKRX)) 
                                                                                  == (DLL_MASK_LNKTX |  DLL_MASK_LNKRX)  ) // RX&TX link
        {            
            DLL_HASH_STRUCT *pRxHashEntry = g_pCrtHashEntry+1;
            for( ; pRxHashEntry < g_stDll.m_aHashTable+g_stDll.m_ucHashTableSize; pRxHashEntry++ )
            {
                if( pRxHashEntry->m_ucLinkIdx == g_pCrtHashEntry->m_ucLinkIdx ) // correspondent has entry founded
                {
                    g_pCandidateHashEntry = pRxHashEntry;
                }
            }            
        }
#endif        
        // calculates the delay until start CCA/transmission and check if not too late
        unDelay = TMR_GetSlotOffset(); // + KTX until TxRequest!!!  // TMRA steps
                
        if( unDelay >=  pCrtTemplate->m_unXmitLatestTMR2 - USEC_TO_TMR2(TX_JITTER_USEC) ) // too late, next slot maybe
        {
            SET_DebugLastAck( SFC_TOO_LATE );            
            DLDE_AckInterpreter( SFC_TIMEOUT, NULL );            
            
            return;
        }
        
        
        WCI_Log( LOG_M_SYS, PLOG_DataReq, sizeof(g_ucDllCrtCh), &g_ucDllCrtCh, 
                                           sizeof(p_unDelay), &p_unDelay, 
                                           g_aucDllRxTxBuf, ucFrameLen );
        
        g_unTxCount++;
        
        // set CCA/transmission -to be started at the specified moment
        PHY_TX_Request(g_ucDllCrtCh, 
                       ABSOLUTE_DELAY, 
                       pCrtTemplate->m_unXmitEarliestTMR2 + USEC_TO_TMR2(TX_JITTER_USEC) + FRACTION_TO_TMR2(unRandomDelay), 
                       g_aDllTimeslotsTable[g_pCrtHashEntry->m_ucTemplateIdx].m_stTimeslot.m_ucInfo & DLL_MASK_TSCCA, 
                       g_aucDllRxTxBuf, 
                       ucFrameLen);
        
        g_ucDllTdmaStat = MLSM_TX_FRAME;
        
#ifdef RX_TX_LINK_COEXISTENCE        
        if( g_pCandidateHashEntry && (g_ucXVRStatus != PHY_TX_IN_PROGRESS) ) // RX before TX 
        {
            DLL_HASH_STRUCT *pRxHashEntry = g_pCandidateHashEntry;
            g_pCandidateHashEntry = g_pCrtHashEntry;
            g_pCrtHashEntry = pRxHashEntry;
            
            g_ucXVRStatus = PHY_RX_START_DO; // set as RX until TX happend
            PHY_OnTimeTriggeredAction();            
            
            g_ucDllTdmaStat = MLSM_RX_FRAME;            
        }
#endif    
    }
    else if ( ucSlotType == SFC_RX_LINK ) // RX link
    {
        DLL_TIMESLOT_RECEIVE * pCrtTemplate;
        
        pCrtTemplate = &(g_aDllTimeslotsTable[g_pCrtHashEntry->m_ucTemplateIdx].m_stTimeslot.m_utTemplate.m_stRx);
        
        
        unDelay = TMR_GetSlotOffset(); // + KRX until RxRequest!!!  // TMR2 steps -not so important for Rx, anyway...
        if( unDelay >= pCrtTemplate->m_unTimeoutTMR2 ) // too late, next slot maybe ...
        {
            return;
        }
        uint16 unRxTime = 0; //pCrtTemplate->m_unEnableReceiverTMR2;

#ifdef _BATTERY_OPERATED_DEVICE_        
	if( g_stTAI.m_ucClkInterogationStatus == MLSM_CLK_INTEROGATION_ON )
        {
            unRxTime = 0; // search for clock, try the full range of timeslot
        }        
	else if( g_aDllSuperframesTable[g_pCrtHashEntry->m_ucSuperframeIdx].m_stSuperframe.m_unChRate <= 1 ) // for not slow hopping superframe
        {
            TIMERB_StartRxAbandonTimer(pCrtTemplate->m_unTimeoutTMR2 +  TMR0_TO_TMR2(USEC_TO_TMR0(1000))); // prepare timerb to generate interrupt
        }
#endif
        // set reception -to be started at the specified moment
        PHY_RX_Request(g_ucDllCrtCh, ABSOLUTE_DELAY, unRxTime);
        g_ucDllTdmaStat = MLSM_RX_FRAME;
    }
    
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Doinel Alban, Ion Ticus
/// @brief  Performs the necessary operations at the start of a new quarter of second
///         (implicitly at the start of a new second)
/// @param  none
/// @return none
/// @remarks  
///      Access level: Interrupt level
////////////////////////////////////////////////////////////////////////////////////////////////////
void MLSM_On250ms( void )
{
   int16  nCorrection;
   
#ifndef BACKBONE_SUPPORT      
   nCorrection = g_stTAI.m_nCorrection;
#else
   nCorrection = 0;
#endif
   
   #if (DEBUG_MODE >= 1)
      DEBUG1_ON();// test - enter 250ms interrupt
   #endif

   MLSM_checkECNandNACKTimers();
   
   MLSM_chekIdleLinkTimers();

   if( (++g_stTAI.m_uc250msStep) >= 4 )// start a new sec
   {
      g_stTAI.m_uc250msStep = 0;
      g_ulDllTaiSeconds ++;

#ifndef BACKBONE_SUPPORT      
      if( g_stTAI.m_ucSecondsFromLastSync < 255 )
         g_stTAI.m_ucSecondsFromLastSync ++;
      
      if (g_stTAI.m_unClkTimeout)
      {
          g_stTAI.m_unClkTimeout--; 
      }
      // else do nothing; clock corrections from secondary sources will be accepted
      
      if (g_stTAI.m_unSrcClkTimeout)
      {
          g_stTAI.m_unSrcClkTimeout--;
      }
      else if (g_stTAI.m_ucClkInterogationStatus == MLSM_CLK_INTEROGATION_OFF)
      {
          // the maximum number of seconds that the DL can safely operate 
          // without a clock update has expired. Now the dll must actively 
          // interrogate a clock source for a time update; 
          g_stTAI.m_unSrcClkTimeout         = 2 * DMO_DEF_MAX_RETRY_TOUT_INT;           
          g_stTAI.m_ucClkInterogationStatus = MLSM_CLK_INTEROGATION_ON;
      }
      else //  (g_stTAI.m_ucClkInterogationStatus == MLSM_CLK_INTEROGATION_ON)
      {
          // expired also the waiting for actively interogation
          g_stTAI.m_ucClkInterogationStatus    = MLSM_CLK_INTEROGATION_TIMEOUT;
      }
   }
   
   if( nCorrection ) // correction
   {
       if( nCorrection > 0 )
       {
            if( nCorrection > MAX_TMR_250MS_CORRECTION )
            {
                nCorrection = MAX_TMR_250MS_CORRECTION;
            }
       }
       else // ( g_stTAI.m_cCorrection < 0 )
       {
            nCorrection = -nCorrection;
            if( nCorrection > MAX_TMR_250MS_CORRECTION )
            {
                nCorrection = MAX_TMR_250MS_CORRECTION;
            }

            while( TMR_Get250msOffset() < nCorrection )
                ;

            nCorrection = -nCorrection;
       }

       TMR_AddCounter( (uint16)nCorrection );
       
       g_stTAI.m_nCorrection -= nCorrection;
#else
       nCorrection = g_stTAI.m_nCorrection;    
       g_stTAI.m_nCorrection = 0;
#endif 
   }


  
#ifdef BACKBONE_SUPPORT      
  if( DRIFT_APPLY_ASAP == g_ucClockDriftStatus )
  {
     nCorrection = g_nClockDriftTMR;
     
     //TO DO - update if necessary current slot inside 250ms interval 
     g_ucClockDriftStatus = DRIFT_APPLIED;
  }    
  TIMER_Set250msCorrection( nCorrection );     
#endif
  
  MLSM_OnTimeSlotStart( 1 );

  #if (DEBUG_MODE >= 1)
    DEBUG1_OFF();// test - enter 250ms interrupt
  #endif
}

#ifndef BACKBONE_SUPPORT

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /// @author NIVIS LLC, Doinel Alban, Eduard Erdei
  /// @brief  Performes the clock corrections based on the correction included inside ACK  
  /// @param  p_unFraction - the value in 2 ^^ -20. Reports an offset between
  ///                                SFD reception  and nominal start time of the current timeslot
  /// @param  p_ucTimeslotOffset  - indicates the current timeslot in the acknowledger's
  ///                                timebase
  /// @return none
  /// @remarks  
  ///      Access level: Interrupt level\n
  ///      Context: Called by the DLDE_AckInterpreter routine
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  void  MLSM_ClockCorrection(uint16 p_unFraction,
                             uint16 p_unTimeslotOffset)
  {
    if( !g_unTxSFDFraction )
      return;

    #if (DEBUG_MODE >= 4)
      DEBUG4_ON();    // test - clock correction made
      DEBUG4_OFF();
    #endif      
      
    if (SFC_SUCCESS != MLSM_getClockUpdateApproval( g_ucTxNeighborIdx ) )
        return;      
          
    // this will be sufficient if no slot correction is necessary
    g_stTAI.m_ucSecondsFromLastSync = 0; 
    g_stTAI.m_nCorrection = FRACTION_TO_TMR0( p_unFraction ) -  FRACTION_TO_TMR0(g_unTxSFDFraction);
        
    if(p_unTimeslotOffset != 0xFFFF) // valid timeslot offset
    {       
      uint32 unCrtFraction;
      uint16 unRxTimeElapsed = TMR_GetSlotOffset() - g_unTimestampSFDReceived;
      if( unRxTimeElapsed & 0x8000 )
      {
          unRxTimeElapsed += TMR2_TO_TMR0( g_unDllTMR2SlotLength );
      }
      
      unCrtFraction = p_unFraction + unRxTimeElapsed;
      if( unCrtFraction >= TMR2_TO_TMR0( g_unDllTMR2SlotLength ) )
      {
          unCrtFraction -= TMR2_TO_TMR0( g_unDllTMR2SlotLength );
          p_unTimeslotOffset ++;     
          if( p_unTimeslotOffset >= g_aDllSuperframesTable[g_pCrtHashEntry->m_ucSuperframeIdx].m_stSuperframe.m_unChRate )
          {
              p_unTimeslotOffset = 0;
          }
      }
      
      // slow hopping, check if the devices are in different slots
      int16 nSlowDiff = MLDE_GetSlowOffsetDiff( p_unTimeslotOffset - g_aDllSuperframesTable[g_pCrtHashEntry->m_ucSuperframeIdx].m_stSuperframe.m_unCrtSlowHopOffset );
      
      if( nSlowDiff ) // not same slot, slot correction is necessary
      {            
          uint16  unSlotsOnSec =  g_stSlot.m_ucsMax250mSlotNo * 4;    
          uint16 unNewSlotsOnSec =  g_stSlot.m_ucsMax250mSlotNo * g_stTAI.m_uc250msStep + g_stSlot.m_uc250msSlotNo + nSlowDiff;    
          
          while( unNewSlotsOnSec & 0x8000 ) // negative value
          {
              g_ulDllTaiSeconds --;
              unNewSlotsOnSec += unSlotsOnSec;
          }
          
          g_ulDllTaiSeconds += unNewSlotsOnSec / unSlotsOnSec;
          unNewSlotsOnSec %= unSlotsOnSec;
          
          g_stTAI.m_uc250msStep = unNewSlotsOnSec / g_stSlot.m_ucsMax250mSlotNo;
          g_stSlot.m_uc250msSlotNo = unNewSlotsOnSec % g_stSlot.m_ucsMax250mSlotNo;
                
          g_stTAI.m_nCorrection = 0;
          g_ucDllRestoreSfOffset = 1;          
    

          //optimization in order to not accumulate 32 kHz roundoff error
          g_unTMRStartSlotOffset = FRACTION_TO_TMR0( g_stSlot.m_uc250msSlotNo * (uint32)g_unDllTimeslotLength );
          TACCR1 = g_unTMRStartSlotOffset + g_unDllTMR2SlotLength;              
      }
      
      TMR_SetCounter(g_unTMRStartSlotOffset + unCrtFraction );         
    }
    else
    {
        MLSM_updateClockDiagnostics((int32)TMR0_TO_FRACTION(g_stTAI.m_nCorrection), g_ucTxNeighborIdx); 
    }
  }

#endif // not BACKBONE_SUPPORT

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /// @author NIVIS LLC, Doinel Alban
  /// @brief  Sets current DLL TAI time by using the time informations from received DAUX header
  /// @param  p_pstTimeSync -contains TAI at the beginning of received packet
  ///              and the timeslot length value
  /// @return 1 if success, 0 if not
  /// @remarks  
  ///      Access level: interrupt level
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  void MLSM_SetCrtTaiTime(DLL_ADV_TIME_SYNC *p_pstTimeSync)
  {
    uint16 unTMR0Cntr = TMR_Get250msOffset()+1; // +1 is to cover next computations time
    uint16 unFract2Offset =  TMR0_TO_FRACTION2(unTMR0Cntr) - TMR0_TO_FRACTION2(g_unTimestampSFDReceived); // delay from current time and SFD received
    unFract2Offset += p_pstTimeSync->m_unDllTaiFraction;
    if( unFract2Offset >= 0x8000 ) // dummy check since is not expected to be true
    {
        p_pstTimeSync->m_ulDllTaiSecond --;
        unFract2Offset -= 0x8000;
    }
    
    if(g_ucDiscoveryState == DEVICE_JOINED)
    {
        
        // ignore if no clock correction on ack for mor than a specified time (120 seconds)
        // ignore if is not clock source 2 bytes address source
        // ignore if clock correction on advertise from a device 8 times failed clock source
        // todo: m_stNeighbor.m_ucInfo & DLL_MASK_NEICLKSRC may specify as secondary clock source
        if(  ( g_stDllMsgHrds.m_ucNeighborIdx == 0xFF )
            || !(g_aDllNeighborsTable[g_stDllMsgHrds.m_ucNeighborIdx].m_stNeighbor.m_ucInfo & DLL_MASK_NEICLKSRC)
            || !g_aDllNeighborsTable[g_stDllMsgHrds.m_ucNeighborIdx].m_stNeighbor.m_ucCommHistory   
              )
        {
            return;
        }
        
        uint32 ulSecDiff = g_ulDllTaiSeconds - p_pstTimeSync->m_ulDllTaiSecond + 1;
        if( ulSecDiff <= 2 ) // a sec drift max
        {
            uint32 ulCorrection = TMR_CLK_250MS * ((ulSecDiff-1)*4 + g_stTAI.m_uc250msStep);
            ulCorrection += unTMR0Cntr;
            ulCorrection -= FRACTION2_TO_TMR0(unFract2Offset);
            if( (ulCorrection + 0x8000) < 0xFFFF ) // valid correction
            {
                if (SFC_SUCCESS != MLSM_getClockUpdateApproval( g_stDllMsgHrds.m_ucNeighborIdx ) )
                {
                    return;
                }
                                            
                g_stTAI.m_nCorrection = (int16)-ulCorrection;
                
                #if (DEBUG_MODE >= 4)
                  DEBUG4_ON();    // test - clock correction made
                  DEBUG4_OFF();
                #endif
            }
        }
    }    
  }



////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Doinel Alban
/// @brief  Gets the current TAI time (seconds only) from the DLL
/// @param  none
/// @return current TAI (seconds number)
/// @remarks  
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
__monitor __arm uint32 MLSM_GetCrtTaiSec(void)
{ // __monitor to guarantee a 4-byte memory access (g_ulDllTaiSeconds is written from interrupt)
  return g_ulDllTaiSeconds;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Gets the nominal slot start time in miliseconds(2^-10 sec)
/// @param  none
/// @return  nominal slot start time in miliseconds
/// @remarks  
///      Access level: interrupt level
////////////////////////////////////////////////////////////////////////////////////////////////////
__arm uint32 MLSM_GetSlotStartMs(void)
{
    uint32 ulSlotStartMs = (g_ulDllTaiSeconds << 10);
        
    //miliseconds inside the passed quarters of second from the current second
    ulSlotStartMs += ((uint32)g_stTAI.m_uc250msStep << 8);
    
    //miliseconds offset inside the current quarter of second
     ulSlotStartMs += ((uint32)g_stSlot.m_uc250msSlotNo * g_unDllTimeslotLength) >> 10;
     
    
    return ulSlotStartMs;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin, Ion Ticus
/// @brief  Gets the nominal slot start time in miliseconds(2^-10 sec) on  big endian order
/// @param  p_nSlotDiff - difference in timeslots for slow hopping
/// @return  nominal slot start time in miliseconds on big endian order
/// @remarks  
///      Access level: interrupt level
////////////////////////////////////////////////////////////////////////////////////////////////////
__arm uint32 MLSM_GetSlotStartMsBE(int p_nSlotDiff)
{
    //count the time as no of quarter of seconds
    uint32 ul250msStepsNo = (g_ulDllTaiSeconds << 2) + g_stTAI.m_uc250msStep;
    unsigned int unCrtSlotNo = g_stSlot.m_uc250msSlotNo + p_nSlotDiff;
                    
    if( unCrtSlotNo >= g_stSlot.m_ucsMax250mSlotNo )
    {
        if( unCrtSlotNo & 0x8000 ) // negative
        {
            unCrtSlotNo = -unCrtSlotNo-1;
            
            ul250msStepsNo -= 1 + (unCrtSlotNo / g_stSlot.m_ucsMax250mSlotNo);
            unCrtSlotNo = g_stSlot.m_ucsMax250mSlotNo - 1 - (unCrtSlotNo % g_stSlot.m_ucsMax250mSlotNo);
        }
        else // positive
        {
            ul250msStepsNo += unCrtSlotNo / g_stSlot.m_ucsMax250mSlotNo;
            unCrtSlotNo %= g_stSlot.m_ucsMax250mSlotNo;
        }
    }
    
    uint32 ulSlotStartMs = (ul250msStepsNo << 8) + (((uint32)unCrtSlotNo * g_unDllTimeslotLength) >> 10); 

    //rotate the long value
    uint32 ulTemp = ulSlotStartMs ^ ((ulSlotStartMs << 16) | (ulSlotStartMs >> 16));
    ulTemp &= ~0xff0000;
    ulSlotStartMs = (ulSlotStartMs << 24) | (ulSlotStartMs >> 8);
    return ulSlotStartMs ^ (ulTemp >> 8);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Doinel Alban
/// @brief  Gets the precise current TAI time from the DLL, by reading the system timer
/// @param  p_pulTaiSec - (out) seconds number
/// @param  p_punTaiFract - (out) fractional time in units of 2^(-15) seconds
/// @return none
/// @remarks  
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
__monitor void MLSM_GetCrtTaiTime(uint32 *p_pulTaiSec, uint16 *p_punTaiFract)
{
  MONITOR_ENTER();
  
  *p_pulTaiSec = g_ulDllTaiSeconds;
  *p_punTaiFract = ((uint16)g_stTAI.m_uc250msStep << 13) + TMR0_TO_FRACTION2( TMR_Get250msOffset() );
  
  MONITOR_EXIT();
}

// ------------------------------------- PHY layer functions -----------------------------


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Doinel Alban
/// @brief  Callback function. It is called either by PHY_TX_Request or by MACA_Interrupt
///                after a transmission was requested.
/// @param  p_ucStatus  = PHY_DATA_SUCCES, TRANCEIVER_BUSY, PHY_DATA_OTH_ERROR (CCA)
/// @return none
/// @remarks  
///      Access level: interrupt
////////////////////////////////////////////////////////////////////////////////////////////////////
void PHY_DataConfirm (uint8 p_ucStatus)
{
    WCI_Log( LOG_M_SYS, PLOG_Confirm, sizeof(p_ucStatus), &p_ucStatus );
    
    if( p_ucStatus != PHY_DATA_SUCCES ) // on error
    {
        PHY_Disable_Request(PHY_IDLE);
        if( g_ucDllTdmaStat == MLSM_TX_FRAME ) // TX error
        {
            if (p_ucStatus == TRANCEIVER_BUSY)  // CCA - channel busy
            {
                g_ucSlotExecutionStatus = SLOT_STS_TXCCA_BACKOFF;
                if( g_pCrtHashEntry->m_ucLinkType & DLL_MASK_LNKEXP )
                {
                    MLSM_UpdateShareLinkExpBackoff(TX_WITH_NO_ACK);
                }
            }
            // else; TX_BUFFER_UNDERFLOW
            SET_DebugLastAck( SFC_FUNCTION_PROCESS_ERROR );
            DLDE_AckInterpreter( SFC_TIMEOUT, NULL );            
        }
    }
    else // on success
    {
        if(g_ucDllTdmaStat == MLSM_TX_FRAME) // data frame was just sent
        {
            if( g_stDllMsgHrds.m_ucDHDR & DLL_MASK_NEED_ACK ) // ACK expected
            {
                uint16 unDelay;
                DLL_SMIB_TIMESLOT* pCrtTemplate = &g_aDllTimeslotsTable[g_pCrtHashEntry->m_ucTemplateIdx].m_stTimeslot;
                
                if( pCrtTemplate->m_ucInfo & DLL_MASK_TSACKREF )
                {
                    //negative offset from end of timeslots
//                    unDelay = g_unDllTMR2SlotLength 
//                                - pCrtTemplate->m_utTemplate.m_stTx.m_unAckEarliestTMR2;                
                    unDelay = 0; // safe option but not very much battery optimization
                }
                else //offset from end of incoming DPDU
                {                
                    // unDelay = TMR_GetSlotOffset() + pCrtTemplate->m_utTemplate.m_stTx.m_unAckEarliestTMR2-1; // -1 because computation time
                    unDelay = 0; // safe option but not very much battery optimization
                }
                
                PHY_RX_Request(g_ucDllCrtCh, ABSOLUTE_DELAY, unDelay);
                
                g_ucDllTdmaStat = MLSM_RX_ACK;   // for now, the timeout will be processed at the next slot beginning!!!!!
                return;            
            }
            else // no ACK expected
            {
                DLDE_AckInterpreter( (BROADCAST_DEST << 8) | SFC_SUCCESS, 0);
                
                if( g_ucLoggingLevel >= EXTENDED_LOGGING_LEVEL )
                {
                    g_stWciRcvRfMsg.m_ucMsgType = WCITXT_MSG_TO_BBR_AS_CFM;
                    g_stWciRcvRfMsg.m_u.m_stDllCfm.ulTxMsgID = 0xFFFF;
                    g_stWciRcvRfMsg.m_u.m_stDllCfm.ucTxStatus = 0;
                    g_stWciRcvRfMsg.m_u.m_stDllCfm.ucTxRetryCount = 0;
                    g_stWciRcvRfMsg.m_unDelayMilliSec = 0;
                    
                    WCI_AddMessage( sizeof(g_stWciRcvRfMsg), (const uint8*)&g_stWciRcvRfMsg );
                }
            }
        }
    }
    #ifdef PREDICTIVE_SCHEDULE_LINK
        g_stNextLink.m_unSlotsUntilWakeUp = 1;
    #endif
    g_ucDllTdmaStat = MLSM_START_SLOT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Doinel Alban
/// @brief  Callback function. It is called by MACA_Interrupt after a data frame
///                or ACK packet was successfully received.
/// @param  p_pRxBuff   = 1 byte length + received packet
/// @return none
/// @remarks  
///      Access level: MACA interrupt\n
///      Context: transceiver is in IDLE state
////////////////////////////////////////////////////////////////////////////////////////////////////
void PHY_DataIndicate( uint8 * p_pRxBuff )
{
  uint8  ucIncomingFrmStat;
  volatile int aa=10;
  switch(g_ucDllTdmaStat)
  {
  case MLSM_RX_FRAME:             // -packet reception      
      // prepare clock correction to be included into ACK [us]
       if( g_ucNoSendACK )  //888
      {
        aa = 11;
      }
      ucIncomingFrmStat = MLDE_IncomingFrame(p_pRxBuff);   // check the received packet            
      if( g_ucNoSendACK )
      {
        //no ACK/NACK will be sent
        ucIncomingFrmStat = DLL_NO_ACK_SEND_MSG_BROADCAST;
      }
          
      switch(ucIncomingFrmStat)
      {
        case DLL_NO_ACK_SEND_MSG_BROADCAST: // 1 - not send ACK (broadcast); The use of broadcast in the standard is limited to DL-layer operations          
            //DLDE_HandleRxMessage(p_pRxBuff); 
            MLSM_updateSignalStatus(g_stDllMsgHrds.m_ucNeighborIdx);
            g_ucDllTdmaStat = MLSM_START_SLOT;

            //add to queue the received RF broadcast message
            if( g_ucLoggingLevel >= DLL_LOGGING_LEVEL )
            {
                g_stWciRcvRfMsg.m_ucMsgType = WCITXT_MSG_TO_BBR_AS_RF;
                g_stWciRcvRfMsg.m_u.m_stRf.m_stTL.m_ucLen = 0;
                g_stWciRcvRfMsg.m_u.m_stRf.m_stNL.m_ucLen = 0;
                WCI_AddMessage( sizeof(g_stWciRcvRfMsg), (const uint8*)&g_stWciRcvRfMsg );            
            }
            g_ucNoSendACK = 0;  //to be sure
            break;
          
        case DLL_NO_ACK_SEND_MSG_DAUX:      // 2 - not send ACK, process received packet (discovery state, DAUX - advertise)                
            DMAP_DLMO_IndicateDAUX( g_stDllMsgHrds.m_stPeerAddr.m_unShort );
            MLSM_updateSignalStatus(g_stDllMsgHrds.m_ucNeighborIdx);
            g_ucDllTdmaStat = MLSM_START_SLOT;            
            break;

        case DLL_DISCARD_MSG:               // 0 - not send ACK, discard received packet
            g_ucDllTdmaStat = MLSM_START_SLOT;            
            break;
        
        case DLL_DISCARD_MSG_NOT_FOR_ME: // 7 - not send ACK, check new discovered neighbor, discard packet          
            g_ucDllTdmaStat = MLSM_START_SLOT;                                   
            break;

        case DLL_ACK_SEND_MSG:              // 3 - send ACK, process received packet          
        case DLL_NAK_DISCARD_MSG:           // 5 - send NACK, discard received packet
        {
            uint16 unDelay;
            DLL_SMIB_TIMESLOT* pCrtTemplate = &g_aDllTimeslotsTable[g_pCrtHashEntry->m_ucTemplateIdx].m_stTimeslot;
           
            if( pCrtTemplate->m_ucInfo & DLL_MASK_TSACKREF )
            {
                //negative offset from end of timeslots
                unDelay = g_unDllTMR2SlotLength 
                           - pCrtTemplate->m_utTemplate.m_stRx.m_unAckEarliestTMR2;                
            }
            else //offset from end of incoming DPDU
            {   
                // g_stDllRxHdrExt.m_ucSecurityPos is DLL hdr length (include packet length) 
                uint16  unPaketLength = g_stDllRxHdrExt.m_ucSecurityPos + g_stDllMsgHrds.m_ucPayloadLen + MIC_SIZE+2;
                unDelay = PHY_GetLastRXSFDTmrOffset();
                if(  unDelay & 0x8000 ) // previous slot 
                {
                    unDelay += TMR2_TO_TMR0( g_unDllTMR2SlotLength );
                }
                unDelay = TMR0_TO_TMR2( unDelay  + USEC_TO_TMR0(unPaketLength*32) ) 
                          + pCrtTemplate->m_utTemplate.m_stRx.m_unAckEarliestTMR2;                
            }
            if( unDelay & 0x8000 ) // late ACK
            {
                unDelay = 0; // still try
            }

            WCI_Log( LOG_M_SYS, PLOG_DataReq, sizeof(g_ucDllCrtCh), &g_ucDllCrtCh, 
                                               sizeof(unDelay), &unDelay, 
                                               g_ucAckLen, g_oAckPdu ); // g_ucAckLen is without MIC on CHIPCON modem
            
            g_ucDllTdmaStat = (ucIncomingFrmStat == DLL_ACK_SEND_MSG ? MLSM_TX_ACK : MLSM_TX_NACK);
            
            if (g_ucDllTdmaStat == MLSM_TX_ACK)
            {
              g_unTxAckCount++;
            }
            else
            {
              g_unTxNakCount++;
            }
            
            PHY_TX_Request(g_ucDllCrtCh, ABSOLUTE_DELAY, unDelay, 0, g_oAckPdu, g_ucAckLen); 
            
            if( DLL_NAK_DISCARD_MSG == ucIncomingFrmStat )
            {
                if( g_ucLoggingLevel >= DLL_LOGGING_LEVEL )
                {
                    //add to queue the sent DLL NACK/ACK - ACK based on a DLL request(clock synchro or EUI64 request)
                    g_stWciRcvAckMsg.m_ucMsgType = WCITXT_MSG_TO_BBR_AS_TX_ACK;
                    WCI_AddMessage( sizeof(g_stWciRcvAckMsg), (const uint8*)&g_stWciRcvAckMsg );
                    //clear the ACK DLL header
                    g_stWciRcvAckMsg.m_u.m_stDllAck.m_stDL.m_ucLen = 0;
                }
            }
            
            //update the average RSSI level value and signal status
            MLSM_updateSignalStatus(g_stDllMsgHrds.m_ucNeighborIdx);
            return;
        }
    }
  break;
  case MLSM_RX_ACK:               // -ACK reception (after frame transmission)
      SET_DebugLastAck( SFC_ACK_RECEIVED );
      DLDE_AckInterpreter( (UNICAST_DEST << 8) | SFC_SUCCESS, p_pRxBuff);
      MLSM_updateSignalStatus(g_ucTxNeighborIdx);
#ifdef CUSTOM_RX_AFTER_ACK 
      // DUO cast support, non standard since ISA100 says "periodically activation" without any other explanation     
      {
          DLL_SMIB_TIMESLOT* pCrtTemplate = &g_aDllTimeslotsTable[g_pCrtHashEntry->m_ucTemplateIdx].m_stTimeslot;
          if( pCrtTemplate->m_ucInfo & DLL_MASK_TSKEEP )
          {
              PHY_RX_Request( g_ucLastRFChannel, RELATIVE_DELAY, 0 );  
              g_ucDllTdmaStat = MLSM_RX_FRAME;
              return;
          }
      }
#endif      
      g_ucDllTdmaStat = MLSM_START_SLOT;
      break;

//    case MLSM_START_SLOT:           // -waiting for a new timeslot start
//      PHY_ReleaseRXBuff();
      //TODO: slow hopping period - receiver active beyond the slot boundary ???
//      break;
  }
    
#ifndef BACKBONE_SUPPORT
  if( g_ucDiscoveryState == DEVICE_DISCOVERY ) // a message received, but still on discover, continue RX
  {
      activateDiscoveryRxMode();
  }
  else
  {
    #ifdef PREDICTIVE_SCHEDULE_LINK
      g_stNextLink.m_unSlotsUntilWakeUp = 1;
    #endif
  }
#endif

}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Callback function. 
/// @param  p_pRxBuff composed as 1 byte length + packet     
/// @return 0 if error, 1 if need to continue to receive the packet
/// @remarks  
///      Access level: interrupt\n
///      Context: When PHY_MIN_HDR_SIZE bytes arrived or end of RX (first happend)
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 PHY_HeaderIndicate( uint8 * p_pRxBuff )
{    
  if(g_ucDllTdmaStat == MLSM_RX_FRAME) // -packet reception
  {
      if( (p_pRxBuff[1] == 0x01) || (p_pRxBuff[1] == (0x01 | DLL_MHR_PAN_COMPRESS_MASK)) )
      {      
          return MLDE_IncomingHeader( p_pRxBuff );
      }
  }
  
  return (p_pRxBuff[1] == 0x01); // ack accept only that data frame control 
}


uint8 MLSM_CanReceiveMsg(void)
{
  return (g_ucDllTdmaStat == MLSM_RX_FRAME || g_ucDllTdmaStat == MLSM_RX_ACK);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Doinel Alban
/// @brief  Callback function. Informs the MLDE when PHY error occurs.
/// @param  p_ucStatus
/// @param  p_pucData
/// @return none
/// @remarks  
///      Access level: Interrupt level
////////////////////////////////////////////////////////////////////////////////////////////////////
void PHY_ErrorIndicate(uint8 p_ucStatus, uint8 *p_pucData)
{
//  switch (p_ucStatus)
//  {
//    //case TRANCEIVER_BUSY:
//    case RX_BUFFER_OVERFLOW:
//    case RX_CRC_ERROR:
//    //case PACKET_INCOMPLETE:
//    //case OTH_ERROR:
//    //case RX_GENERAL_ERROR:
//    default:
//      break;
//  }
  PHY_Disable_Request(PHY_IDLE);
  if(g_ucDllTdmaStat == MLSM_RX_ACK)
  {
      // save statistics info: consider this error as TxFailed: no ACK/NACK received
      // todo: filter only unicast destinations
      g_ucSlotExecutionStatus = SLOT_STS_TX_ACK_TIMEOUT;
      SET_DebugLastAck( SFC_INTERNAL_ERROR );
      DLDE_AckInterpreter( SFC_TIMEOUT, NULL );      
  }
  g_ucDllTdmaStat = MLSM_START_SLOT;
  
#ifndef BACKBONE_SUPPORT
  if( g_ucDiscoveryState == DEVICE_DISCOVERY )
  {
      activateDiscoveryRxMode();
  }
  else
  {
  #ifdef PREDICTIVE_SCHEDULE_LINK
      g_stNextLink.m_unSlotsUntilWakeUp = 1;
  #endif
  }
#endif
}


//////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, 
/// @brief  Initializes the MLSM module
/// @param  none
/// @return none
/// @remarks  
///      Access level: User mode
////////////////////////////////////////////////////////////////////////////////////////////////////
void MLSM_Init()
{
  g_stSlot.m_uc250msSlotNo = 0xFF;
  g_stSlot.m_ucsMax250mSlotNo = 25;
  g_stTAI.m_uc250msStep = 0;
  g_stTAI.m_nCorrection = 0;
  g_stTAI.m_ucSecondsFromLastSync = 0;
#ifndef BACKBONE_SUPPORT 
  g_stTAI.m_unSrcClkTimeout = g_unClockExpire;
  g_stTAI.m_unClkTimeout = g_unClockStale; // clock stale in 1 sec units
  g_stTAI.m_ucClkInterogationStatus = MLSM_CLK_INTEROGATION_OFF;
#endif
  
  // T0 interrupt was set at 250ms by TMR0_INIT()
  if( g_ulStartTAISec )
  {
      g_ulDllTaiSeconds = g_ulStartTAISec;
      TMR_SetCounter( 0 );
  }
  else
  {
      g_ulDllTaiSeconds = 0;  
  }

  g_ucDiscoveryState = DEVICE_DISCOVERY;
  g_ucDllTdmaStat = MLSM_START_SLOT;
  
  g_ucSlotExecutionStatus = SLOT_STS_NONE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, 
/// @brief  Set the length of the used timeslot in 2^(-20) sec and also in timer ticks 
/// @param  p_unTimeslotLen - length of timeslot in 2^(-20) sec
/// @return none
/// @remarks  
///      Access level: User mode and interrupt mode 
////////////////////////////////////////////////////////////////////////////////////////////////////
void MLSM_SetSlotLength( uint16 p_unTimeslotLen  )
{
  g_unDllTimeslotLength = p_unTimeslotLen; // recommended (not mandatory) to be aligned to 2 ^^ 15
  g_unDllTMR2SlotLength = FRACTION_TO_TMR2( p_unTimeslotLen );
  g_stSlot.m_uc250msSlotNo = 0xFF; // wait for end of 250 ms
  g_stSlot.m_ucsMax250mSlotNo  = (1L<<18)/p_unTimeslotLen;
  // enable TACCR1 interrupt???
}


#ifdef BACKBONE_SUPPORT
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @author NIVIS LLC, Doinel Alban, Ion Ticus
    /// @brief  Set the backbone's absolute TAI, based on the value received from the master clock source
    /// @param  p_ulSec
    /// @param  p_unSecFract2
    /// @return none
    /// @remarks  
    ///      Access level: Interrupt level
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void MLSM_SetAbsoluteTAI( uint32 p_ulSec, uint32 p_ulFractSec )
    {
         static uint8 s_ucPrevCorrection;
         
         if (g_ucAdvTimeSync && g_ucDiscoveryState == DEVICE_JOINED)
           return;
         p_ulFractSec = FRACTION_TO_TMR0( p_ulFractSec ); // transform on clock tics

#if defined(at91sam7x512)
         p_ulFractSec -= TMR_GetEmacElapsedTime();
         if( p_ulFractSec & 0x80000000 ) // negative value
         {
            p_ulFractSec += TMR_CLK_250MS*4;
            p_ulSec--;
         }
#endif
         
         //update NTP clock based on the preconfigured clock drift
         //the external clock drift is limited to +/- 32 mSec 
         if( DRIFT_APPLIED == g_ucClockDriftStatus )
         {
            p_ulFractSec += g_nClockDriftTMR;
         
            if( p_ulFractSec & 0x80000000 ) // negative value
            {
                p_ulFractSec += TMR_CLK_250MS*4;
                p_ulSec--;
            }
            else if( p_ulFractSec >= TMR_CLK_250MS*4 )
            {
                p_ulFractSec -= TMR_CLK_250MS*4;
                p_ulSec++;
            }
         }
         
         if(g_ucDiscoveryState == DEVICE_JOINED)
         {
            s_ucPrevCorrection <<= 1;
            if(  g_ulDllTaiSeconds > p_ulSec ) // negative correction needed
            {
                s_ucPrevCorrection |= 1;
            }
            else if(  g_ulDllTaiSeconds == p_ulSec ) 
            {
                if( g_stTAI.m_uc250msStep * TMR_CLK_250MS + TMR_Get250msOffset() > p_ulFractSec ) // negative correction needed
                {
                    s_ucPrevCorrection |= 1;
                }
            }
            
            s_ucPrevCorrection &= 0x0F;
            
            if( s_ucPrevCorrection == 0 )
            {
                g_stTAI.m_nCorrection = MAX_TMR_BBR_CORRECTION; 
            }
            else if(  s_ucPrevCorrection  == 0x0F ) // 4 consecutive in one direction 
            {
                g_stTAI.m_nCorrection = -MAX_TMR_BBR_CORRECTION;
            }              
         }
         else if(g_ucDiscoveryState == DEVICE_DISCOVERY)
         {
           // TBD : need to notify the DMAP for new clock ... must rewrite that section
            g_stSlot.m_uc250msSlotNo = 0xFF;

            g_ulDllTaiSeconds = p_ulSec;
            g_stTAI.m_uc250msStep = p_ulFractSec / TMR_CLK_250MS;
            g_stTAI.m_nCorrection = 0;            

            TMR_SetCounter( p_ulFractSec % TMR_CLK_250MS);
            
            s_ucPrevCorrection = 1; // forced 8 consecutive comparations
            if (g_ucJoinStatus == DEVICE_JOINED)
            {
              g_ucDiscoveryState = DEVICE_JOINED;
            }
         }
        #if (DEBUG_MODE >= 1)
         DEBUG1_ON();
         DEBUG1_OFF();

         DEBUG1_ON();
         DEBUG1_OFF();
        #endif
    }

#endif // BACKBONE_SUPPORT


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin, Eduard Erdei
/// @brief  Updates the RF Signal status needed for the acquisition board and for the EMA average
///         RSSI/neighbor statistics
/// @param  p_NeighIdx - neighbor index in neighbor table
/// @return None
/// @remarks
///      Access level: Interrupt
////////////////////////////////////////////////////////////////////////////////////////////////////
void MLSM_updateSignalStatus(uint8 p_NeighIdx)
{    
  // update the EMA average RSSI for a particular neighbor
  if (p_NeighIdx < sizeof(g_aDllNeighborsTable)/sizeof(g_aDllNeighborsTable[0]) )
  {
      DLL_SMIB_NEIGHBOR_DIAG * pstDiag = g_aDllNeighborsTable[p_NeighIdx].m_stNeighbor.m_pstDiag;
      
      // collect RSSI info only if required
      // RSSI shall be reported as a signed 8-bit integer, reflecting an estimate of received signal strength in dBm. 
      // RSSI reports shall be biased by +64 dBm to give an effective range of -192 dBm to +63 dBm.
      // Biasing operation is performed only when reporting RSSI to the System Manager
      if ( pstDiag && (pstDiag->m_ucDiagFlags & DLL_MASK_NEIDIAG_LINK) )
      {          
          // check if first time update
          int16 nLastRSSI = GET_LAST_RSSI();
          if ( pstDiag->m_ucStatisticsInfo & DLL_MASK_RSSIINFO )
          {
              uint8 ucSmoothAlpha;
              
              if (nLastRSSI > g_stSmoothFactors.m_nRSSIThresh)
              {
                  ucSmoothAlpha = g_stSmoothFactors.m_ucRSSIAlphaHigh;
              }
              else
              {
                  ucSmoothAlpha = g_stSmoothFactors.m_ucRSSIAlphaLow;
              }             
              
              // NewRssi = OldRssi + (p_pCrtSignalLevel - OldRssi) * smoothAlpha / 100
              pstDiag->m_cRSSI = (int8)((nLastRSSI * ucSmoothAlpha + ((int16)100-ucSmoothAlpha) * pstDiag->m_cRSSI) / 100);  
              
              if( g_ucLastLQI > g_stSmoothFactors.m_nRSQIThresh )
              {
                  ucSmoothAlpha = g_stSmoothFactors.m_ucRSQIAlphaHigh;
              }
              else
              {
                  ucSmoothAlpha = g_stSmoothFactors.m_ucRSQIAlphaLow;
              }
              
               pstDiag->m_ucRSQI = ((uint16)g_ucLastLQI * ucSmoothAlpha + ((uint16)100-ucSmoothAlpha) * pstDiag->m_ucRSQI)/100;
          }
          else
          {
              // first time update; EMA level = current level;
              pstDiag->m_cRSSI = nLastRSSI;
              pstDiag->m_ucRSQI = g_ucLastLQI;
              // signal that EMA level has been initialized
              pstDiag->m_ucStatisticsInfo |= DLL_MASK_RSSIINFO;
          }
      }               
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef BACKBONE_SUPPORT
void activateDiscoveryRxMode(void)
{
  PHY_RX_Request( g_ucLastRFChannel, RELATIVE_DELAY, 0 );  
  g_ucDllTdmaStat = MLSM_RX_FRAME;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Updates the EMA average of clock bias statistics of a neighbor (clock source or recipient)
/// @param  p_lBias - clock correction (signed int13 in 2^-20 seconds)
/// @param  p_NeighIdx - neighbor index
/// @return None
/// @remarks
///      Access level: Interrupt
////////////////////////////////////////////////////////////////////////////////////////////////////
void MLSM_updateClockDiagnostics( int32 p_lBias, uint8 p_NeighIdx)
{  
  if (p_NeighIdx < sizeof(g_aDllNeighborsTable)/sizeof(g_aDllNeighborsTable[0]) )
  {  
      // collect clock info only if required
      DLL_SMIB_NEIGHBOR_DIAG * pstDiag = g_aDllNeighborsTable[p_NeighIdx].m_stNeighbor.m_pstDiag;
      if( pstDiag )
      {          
          // check if the EMA level is initialized
          if( pstDiag->m_ucStatisticsInfo & DLL_MASK_CLOCKCORRINFO )
          {  
              uint8 ucSmoothAlpha = g_stSmoothFactors.m_ucClkBiasAlphaLow;  
              if (p_lBias > g_stSmoothFactors.m_nClkBiasThresh)
              {
                  ucSmoothAlpha = g_stSmoothFactors.m_ucClkBiasAlphaHigh;
              }                    
              pstDiag->m_nClockSigma = (p_lBias * ucSmoothAlpha + (100-ucSmoothAlpha) * pstDiag->m_nClockSigma)/100;
          }
          else 
          {   
              // first update of the EMA. It must be initialized with the current correction;
              pstDiag->m_nClockSigma = p_lBias;
              // signal that the clock EMA has been initialized
              pstDiag->m_ucStatisticsInfo |= DLL_MASK_CLOCKCORRINFO;
          }        
          
          if( g_aDllNeighborsTable[p_NeighIdx].m_stNeighbor.m_ucInfo & DLL_MASK_NEIDIAG_CLOCK )
          {
              if ( pstDiag->m_stDiag.m_unClkCount < 0x7FFF)
              {
                  // increment the clock count diagnostic counter
                  pstDiag->m_stDiag.m_unClkCount++; 
              }
              
              pstDiag->m_stDiag.m_nClkBias = pstDiag->m_nClockSigma;             
          }          
      }
  }
}
#endif // not BACKBONE_SUPPORT

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Approves a clock update if necessary. Resets the clock timeout counter
/// @param  p_ucNeighborIdx - index of the clock source in the neighbor table
/// @return service feedback code
/// @remarks
///      Access level: Interrupt level\n
///      Context: Called if an acknowledge with clock correction info is received or an advertise.
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 MLSM_getClockUpdateApproval( uint8 p_ucNeighborIdx )
{
    #if (DEBUG_MODE >= 4)
        DEBUG4_ON();    // test - clock correction attempt
        DEBUG4_OFF();
    #endif
                  
    if ( p_ucNeighborIdx >= g_ucDllNeighborsNo ) // just in case: check for invalid index 
        return SFC_FAILURE;
    
    if ((g_aDllNeighborsTable[p_ucNeighborIdx].m_stNeighbor.m_ucInfo & DLL_MASK_NEICLKSRC) == 0x80)
    {
        // reset preferred clock timeout counter
        // secondary clock source
       return SFC_SUCCESS;
        }
        //  else  clock timeout expired. accept this correction (from a secondary clock source)
    else 
    {
        return SFC_FAILURE; // the clock correction info is not from a valid clock source 
    }      
    
    
}
 

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Inserts a new candidate neighbor in the new nighbor discovery table
/// @param  p_unShortAddr - short address of the new neighbor
/// @return None
/// @remarks
///      Access level: Interrupt/user
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 MLSM_AddNewCandidate(uint16 p_unShortAddr )
{
  unsigned int unIdx;
  
  for ( unIdx = 0; unIdx < g_stCandidates.m_ucCandidateNo; unIdx++ )
  {
      if ( g_stCandidates.m_unCandidateAddr[unIdx] == p_unShortAddr )
      {
        uint8 ucSmoothAlpha;
          if( g_ucLastLQI > g_stSmoothFactors.m_nRSQIThresh )
          {
              ucSmoothAlpha = g_stSmoothFactors.m_ucRSQIAlphaHigh;
          }
          else
          {
              ucSmoothAlpha = g_stSmoothFactors.m_ucRSQIAlphaLow;
          }
          g_stCandidates.m_cRSSI[unIdx] = GET_LAST_RSSI();          
          g_stCandidates.m_ucRSQI[unIdx] = (uint8) ( ( ( ((uint16)(g_ucLastLQI)) * ucSmoothAlpha) +       \
                                             ( ((uint16)(100-ucSmoothAlpha)) * g_stCandidates.m_ucRSQI[unIdx]))/100);
          g_stCandidates.m_unClockDrift[unIdx] = unClockDrift;
          return SFC_DUPLICATE; // duplicate entry found.
      }
  }
  
  if( unIdx < DLL_MAX_CANDIDATE_NO )
  {
      g_stCandidates.m_unCandidateAddr[unIdx] = p_unShortAddr;
      g_stCandidates.m_cRSSI[unIdx] = GET_LAST_RSSI();      
      g_stCandidates.m_ucRSQI[unIdx] = g_ucLastLQI;
      g_stCandidates.m_unClockDrift[unIdx] = unClockDrift;
      g_stCandidates.m_ucCandidateNo = unIdx+1;
      return SFC_SUCCESS;
  }
  
  return SFC_INSUFFICIENT_DEVICE_RESOURCES;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Decrements the ECN, NACK and alarm timeouts in the neighbor tables
/// @param  none
/// @return none
/// @remarks
///      Access level: Interrupt level
///      Context: 
////////////////////////////////////////////////////////////////////////////////////////////////////
void MLSM_checkECNandNACKTimers(void)
{
    unsigned int unIdx;
    DLL_SMIB_ENTRY_NEIGHBOR * pNeighbour = g_aDllNeighborsTable;
    
    for( unIdx = g_ucDllNeighborsNo; unIdx ; unIdx-- )
    {
        if( pNeighbour->m_stNeighbor.m_ucECNIgnoreTmr )
        {
            pNeighbour->m_stNeighbor.m_ucECNIgnoreTmr--;
        }          
        if( pNeighbour->m_stNeighbor.m_unNACKBackoffTmr )
        {
            pNeighbour->m_stNeighbor.m_unNACKBackoffTmr--;
        }
        if( pNeighbour->m_stNeighbor.m_ucBacklogLinkTimer )
        {
            pNeighbour->m_stNeighbor.m_ucBacklogLinkTimer--;
        }
        pNeighbour++;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Decrements the idle link in the link tables
/// @param  none
/// @return none
/// @remarks
///      Access level: Interrupt level
///      Context: 
////////////////////////////////////////////////////////////////////////////////////////////////////
void MLSM_chekIdleLinkTimers(void)
{  
  if( g_stDll.m_ucIdleLinksFlag )
  {
      g_stDll.m_ucIdleLinksFlag = 0;
      for( unsigned int unIdx = 0; unIdx < g_ucDllLinksNo; unIdx++ )
      {
          if( g_aDllLinksTable[unIdx].m_stLink.m_ucActiveTimer )
          {
              g_stDll.m_ucIdleLinksFlag = 1;
              g_aDllLinksTable[unIdx].m_stLink.m_ucActiveTimer--;
              
              if( g_aDllLinksTable[unIdx].m_stLink.m_ucActiveTimer == 0 )
              {
                  g_ucHashRefreshFlag = 1; // need to recompute the hash table because 
              }                            // we have found a link that has to go back in idle state   
          }
      }
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Saves the status of the previous slot into the neighbor statistics table
/// @param  none
/// @return none
/// @remarks
///      Access level: Interrupt level\n
///      Context: Called if an acknowledge with clock correction info is received or an advertise.
////////////////////////////////////////////////////////////////////////////////////////////////////
void MLSM_saveStatistics(void)
{  
  // if a message has been received succesfuly and the rxNeighIndex is valid
  if (g_ucSlotExecutionStatus == SLOT_STS_RX_SUCCESS )
  {
      if( g_stDllMsgHrds.m_ucNeighborIdx < sizeof(g_aDllNeighborsTable)/sizeof(g_aDllNeighborsTable[0]) )
      {
          DLL_SMIB_NEIGHBOR_DIAG * pDiag = g_aDllNeighborsTable[g_stDllMsgHrds.m_ucNeighborIdx].m_stNeighbor.m_pstDiag;
          if ( pDiag && (g_aDllNeighborsTable[g_stDllMsgHrds.m_ucNeighborIdx].m_stNeighbor.m_ucInfo & DLL_MASK_NEIDIAG_LINK)
                  /*&& (pDiag->m_stDiag.m_unRxDPDUNo < 0x7FFF)*/ )
          {          
              pDiag->m_stDiag.m_unRxDPDUNo++;
          }
      }
  }
  // else, a tx slot was executed in the previous slot
  else if (g_ucSlotExecutionStatus != SLOT_STS_NONE)
  {
      // collect channel diagnsotics
      g_stChDiag.m_astCh[g_ucDllCrtCh].m_unUnicastTxCtr++; // increment attempted unicast transmission number
      
      if (g_ucSlotExecutionStatus == SLOT_STS_TXCCA_BACKOFF)
      {
          g_stChDiag.m_astCh[g_ucDllCrtCh].m_unCCABackoffCtr++;
      }
      else if (g_ucSlotExecutionStatus == SLOT_STS_TX_ACK_TIMEOUT)
      {
          g_stChDiag.m_astCh[g_ucDllCrtCh].m_unNoAckCtr++;
      }
    
      // collect per-neighbor diagnostics if necessary
      if (g_ucTxNeighborIdx < sizeof(g_aDllNeighborsTable)/sizeof(g_aDllNeighborsTable[0]))
      {
          DLL_SMIB_NEIGHBOR_DIAG * pDiag = g_aDllNeighborsTable[g_ucTxNeighborIdx].m_stNeighbor.m_pstDiag;
          if( pDiag && (g_aDllNeighborsTable[g_ucTxNeighborIdx].m_stNeighbor.m_ucInfo & DLL_MASK_NEIDIAG_LINK) )
          {
              //if (pDiag->m_stDiag.m_unUnicastTxCtr < 0x7FFF)
              //{
                    pDiag->m_stDiag.m_unUnicastTxCtr++;
              //}
              
              switch ( g_ucSlotExecutionStatus )
              {
              case SLOT_STS_TX_SUCCESS: 
                  //if (pDiag->m_stDiag.m_unTxSuccesNo < 0x7FFF)
                  //{
                      pDiag->m_stDiag.m_unTxSuccesNo++;                  
                  //}
                  break;
                  
              case SLOT_STS_NACK:
                  //if (pDiag->m_stDiag.m_unTxNackNo < 0x7FFF)
                  //{
                      pDiag->m_stDiag.m_unTxNackNo++;
                  //}
                  break;
                    
              case SLOT_STS_TX_ACK_TIMEOUT:
                  //if (pDiag->m_stDiag.m_unTxFailedNo < 0x7FFF)
                  //{
                      pDiag->m_stDiag.m_unTxFailedNo++;
                  //}
                  break;
                  
              case SLOT_STS_TXCCA_BACKOFF:
                  //if (pDiag->m_stDiag.m_unTxCCABackoffNo < 0x7FFF)
                  //{
                      pDiag->m_stDiag.m_unTxCCABackoffNo++;
                  //}
                  break;            
              }
          }
      }  
  }
  
  // reset slot execution status
  g_ucSlotExecutionStatus = SLOT_STS_NONE;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Updates the shared link parameters related to the exponential backoff support   
/// @param  p_ucReason - reason of calling the function 
/// @return service feedback code
/// @remarks
///      Access level: Interrupt
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 MLSM_UpdateShareLinkExpBackoff(uint8 p_ucReason)
{
    if( g_ucTxNeighborIdx >= sizeof(g_aDllNeighborsTable)/sizeof(g_aDllNeighborsTable[0]) )
        return SFC_FAILURE;
    
    DLL_SMIB_NEIGHBOR* pstCrtNeighbor = &g_aDllNeighborsTable[g_ucTxNeighborIdx].m_stNeighbor;
    
    switch(p_ucReason)
    {
        default:
        case TX_WITH_ACK:
            pstCrtNeighbor->m_ucExpBackoff = 0;
            break;
        
        case TX_WITH_NO_ACK:
            if(pstCrtNeighbor->m_ucExpBackoff < g_ucMaxBackoffExp)
            {
                pstCrtNeighbor->m_ucExpBackoff++;
            }
            pstCrtNeighbor->m_unBackoffCnt = g_unRandValue & ((1 << pstCrtNeighbor->m_ucExpBackoff) - 1);
        
            break;
        
        case SKIP_LINK:
            if( pstCrtNeighbor->m_unBackoffCnt )    
                pstCrtNeighbor->m_unBackoffCnt --;
            else
                return SFC_FAILURE;
    }
    
    return SFC_SUCCESS;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author Honeywell
/// @brief  Is a valid clock source available for this node
/// @param  None
/// @return service feedback code
/// @remarks
///      Access level: User level\n
///      Context: Called by UDP to find out whether a DL clock source is available, if not
///               the UDP syncs the clock from NTP.
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 MLSM_IsClockSourceAvailable( void )
{
    unsigned int unIdx;
    for( unIdx = 0;  unIdx < g_ucDllNeighborsNo ; unIdx++ )
    {
      if( ((g_aDllNeighborsTable[unIdx].m_stNeighbor.m_ucInfo & DLL_MASK_NEICLKSRC) == 0x80) ||
          ((g_aDllNeighborsTable[unIdx].m_stNeighbor.m_ucInfo & DLL_MASK_NEICLKSRC) == 0x40) )
      {
        return SFC_SUCCESS;
      }
    }
    return SFC_FAILURE;
}
