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

#ifndef _NIVIS_PHY_H_
#define _NIVIS_PHY_H_

#include "../global.h"
#include "../digitals.h"
// CC2420
#include "../CC2420/CC2420.h"
#include "../CC2420/CC2420_Macros.h"
#include "../CC2420/CC2420_AES.h"

#define PHY_BUFFERS_LENGTH          ( 128 )
#define MIC_SIZE 4
#define PHY_MIN_HDR_SIZE            (15+1+2) // MHR + DHDR + first 2 bytes from DMXHR

#define PHY_INIT() CC2420_Init()

extern volatile unsigned char g_ucXVRStatus;
extern uint8 g_ucLastRFChannel;
extern uint8 g_ucLastRSSI;

// P = RSSI_VAL + RSSI_OFFSET [dBm]
// where the RSSI_OFFSET is found empirically during system development
// from the front end gain. RSSI_OFFSET is approximately –45.
// include also the Raptor LNA RSSI offset which is -15
#define GET_LAST_RSSI() ( (int8)g_ucLastRSSI - 45 - 15 )

// " TODO: need a correct estimation of LQI for the CC2420 Chipcon Modem"
#define g_ucLastLQI  (g_ucLastRSSI + 0x80)   

// ----------- PHY CHANNELS ------------
#define PHY_CHAN_11               (unsigned char)  0
#define PHY_CHAN_12               (unsigned char)  1
#define PHY_CHAN_13               (unsigned char)  2
#define PHY_CHAN_14               (unsigned char)  3
#define PHY_CHAN_15               (unsigned char)  4
#define PHY_CHAN_16               (unsigned char)  5
#define PHY_CHAN_17               (unsigned char)  6
#define PHY_CHAN_18               (unsigned char)  7
#define PHY_CHAN_19               (unsigned char)  8
#define PHY_CHAN_20               (unsigned char)  9
#define PHY_CHAN_21               (unsigned char) 10
#define PHY_CHAN_22               (unsigned char) 11
#define PHY_CHAN_23               (unsigned char) 12
#define PHY_CHAN_24               (unsigned char) 13
#define PHY_CHAN_25               (unsigned char) 14
#define PHY_CHAN_26               (unsigned char) 15


// ------- PHY ENABLE REQUEST PARAMETERS -----------
#define PHY_DISABLE               (unsigned char) 0
#define PHY_ENABLE_TX             (unsigned char) 1
#define PHY_ENABLE_RX             (unsigned char) 2
// ------- ENABLE CONFIRM CODES
#define PHY_DISABLED_SUCCES       (unsigned char) 0
#define PHY_TX_ENABLED_SUCCES     (unsigned char) 1
#define PHY_TX_ENABLED_ERROR      (unsigned char) 2
#define PHY_RX_ENABLED_SUCCES     (unsigned char) 3
#define PHY_RX_ENABLED_ERROR      (unsigned char) 4


// ------- CCA REQUEST VOID   -----------
// ------- CCA CONFIRM CODES
#define PHY_CCA_CHAN_IDLE         (unsigned char) 0
#define PHY_CCA_CHAN_BUSY         (unsigned char) 1
#define PHY_CCA_TRANCEIVER_OFF    (unsigned char) 2
#define PHY_CCA_OTH_ERROR         (unsigned char) 3


// ------- DATA REQUEST VOID   -----------
// ------- DATA CONFIRM TX CODES
#define PHY_DATA_SUCCES           (unsigned char) 0
#define PHY_DATA_TRANCEIVER_OFF   (unsigned char) 1
#define PHY_DATA_TRANCEIVER_BUSY  (unsigned char) 2
#define PHY_DATA_RECEIVER_ON      (unsigned char) 3
#define PHY_DATA_OTH_ERROR        (unsigned char) 4


// ------- LOCAL MANAGEMENT REQUEST SERVICES PARAMETERS
#define PHY_MNG_RESET                 (unsigned char) 0
#define PHY_MNG_READ_TX_PWR_LVL       (unsigned char) 1
#define PHY_MNG_WRITE_TX_PWR_LVL      (unsigned char) 2
#define PHY_MNG_WRITE_RX_OVERFLOW_LVL (unsigned char) 3
#define PHY_MNG_IDLE                  (unsigned char) 4
// ------- LOCAL MANAGEMENT CONFIRM SERVICES CODES
#define PHY_MNG_SUCCES                 (unsigned char) 0
#define PHY_MNG_ERROR                  (unsigned char) 1


// ------- ERROR CODES (RX INDICATE)
#define RX_BUFFER_OVERFLOW        (unsigned char) 1
#define TX_BUFFER_UNDERFLOW       (unsigned char) 2
#define PACKET_INCOMPLETE         (unsigned char) 3
#define TRANCEIVER_BUSY           (unsigned char) 4
#define OTH_ERROR                 (unsigned char) 5
#define RX_GENERAL_ERROR          (unsigned char) 6
#define RX_CRC_ERROR              (unsigned char) 7


// ------- STATE MACHINE CODE
#define   PHY_DISABLED             (unsigned char)  1
#define   PHY_IDLE                 (unsigned char)  2
#define   PHY_CCA_START_DO         (unsigned char)  3 // start action
#define   PHY_TX_START_DO          (unsigned char)  4 // start action
#define   PHY_TX_WITH_CCA_START_DO (unsigned char)  5 // start action
#define   PHY_RX_START_DO          (unsigned char)  6 // start action
#define   PHY_TX_IN_PROGRESS       (unsigned char)  7
#define   PHY_RX_IN_PROGRESS       (unsigned char)  8
#define   PHY_CCA_IN_PROGRESS      (unsigned char)  9

// ------- AES Mode
#define   PHY_AES_DISABLED        (unsigned char)  CC2420_SECCTRL0_NO_SECURITY
#define   PHY_AES_CTR             (unsigned char)  CC2420_SECCTRL0_CTR
#define   PHY_AES_CBC_MAC         (unsigned char)  CC2420_SECCTRL0_CBC_MAC
#define   PHY_AES_CCM             (unsigned char)  CC2420_SECCTRL0_CCM


#ifdef LOW_POWER_DEVICE
  void PHY_Disable_RequestExt( unsigned char p_ucDisableLevel );
  #define PHY_Disable_Request(x) PHY_Disable_RequestExt(x)
#else
  void PHY_Disable_RequestExt( void );
  #define PHY_Disable_Request(x) PHY_Disable_RequestExt()
#endif

void PHY_Reset(void);
void PHY_RX_Request ( unsigned char p_ucChannel, unsigned char p_ucRelativeFlag,  unsigned int p_unDelay );
void PHY_TX_Request2( unsigned char p_ucChannel, unsigned char p_ucRelativeFlag,  unsigned int p_unDelay, unsigned char p_ucCCAFlag);

#define PHY_TX_Request(p_ucChannel,p_ucRelativeFlag,p_unDelay,p_ucCCAFlag,p_pucData,p_ucLen) PHY_TX_Request2(p_ucChannel,p_ucRelativeFlag,p_unDelay,p_ucCCAFlag)

void PHY_MNG_Request ( unsigned char p_ucService, unsigned char * p_pucData);

extern unsigned int  g_unTimestampSFDReceived;
extern uint16 g_unRxAckSFDOffsetFract;

#define PHY_GetLastRXSFDTmrOffset() (g_unTimestampSFDReceived-g_unTMRStartSlotOffset)
extern void PHY_OnTimeTriggeredAction(void);



extern void PHY_EnableConfirm  ( unsigned char p_ucStatus,  unsigned char p_ucChannel);
extern void PHY_EnableIndicate ( void );
extern void PHY_CCAConfirm     ( unsigned char p_ucStatus);
extern void PHY_DataConfirm    ( unsigned char p_ucStatus);
extern void PHY_ErrorIndicate  ( unsigned char p_ucStatus,  unsigned char * p_pucData);

extern void PHY_MNG_Confirm    ( unsigned char p_ucService, unsigned char p_ucStatus,  unsigned char * p_pucData);
extern void PHY_MNG_Indicate   ( unsigned char p_ucService, unsigned char p_ucStatus,  unsigned char * p_pucData);





//-------------------------------------------------------------------------------
// Security Module
//-------------------------------------------------------------------------------

// Stand-Alone buffer Encryption/Decryption (AES CTR/CBC-MAC/CCM on a given buffer)
void PHY_AES_SetDecryptNonce( const unsigned char* p_ucNonce );
void PHY_AES_SetDecryptPlainLen( unsigned char p_ucPlainRx );
  
void PHY_AES_SetEncryptNonce( const unsigned char* p_ucNonce, unsigned char p_ucPlainTx );

unsigned char  PHY_AES_DecryptUser( const unsigned char* p_ucKey,
                                    const unsigned char* p_pucNonce,
                                    const unsigned char* p_pucToAuthBuf,
                                    unsigned int   p_unToAuthOnlyLen,
                                    unsigned char* p_pucToDecryptBuff,
                                    unsigned int   p_unToDecryptLen,
                                    unsigned char  micSize 
                                  );

unsigned char PHY_AES_EncryptUser( const unsigned char* p_ucKey,
                                   const unsigned char* p_pucNonce,
                                   const unsigned char* p_pucToAuthBuf,
                                   unsigned int   p_unToAuthOnlyLen,
                                   unsigned char* p_pucToEncryptBuff,
                                   unsigned int   p_unToEncryptLen,
                                   unsigned char  micSize 
                                 );

void PHY_AES_SetKeyInterrupt(const unsigned char* p_ucKey );


#define HASH_DATA_BLOCK_LEN     (uint8)16           //lenght in bytes of the hash data block
#define MAX_HMAC_INPUT_SIZE     (2*16)+(2*8)+2+(2*3)+(2*16) //according with the Security Join Response Specs

uint8 Keyed_Hash_MAC(const uint8* p_pucKey,
                  uint8* const p_pucInputBuff,
                  uint16 p_unInputBuffLen
                  );

extern const uint8 * g_pPrevKey;
#define PHY_AES_ResetKey() g_pPrevKey = NULL


#ifdef LOW_POWER_DEVICE
  __monitor void PHY_SetToIdle(void);
    void PHY_WakeUpRequest( void );
#else
  #define PHY_SetToIdle()
#endif

void RF_Interrupt( void );

#define PHY_IsBusyChannel() CC2420_IsBusyChannel()

uint8 PHY_IsRXOnChannel(uint8 p_ucChannel );

#endif /* _NIVIS_PHY_H_ */

