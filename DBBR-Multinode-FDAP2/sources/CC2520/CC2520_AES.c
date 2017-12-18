//=========================================================================================
// Name:        CC2520_AES.c
// Author:      Gourav Sharma
// Date:        December 2009
// Description: CC2520 Driver - AES Module
// Changes:
// Revisions:
//=========================================================================================

#include "../global.h"
#include "../digitals.h"
#include "../spi1.h"
#include "../system.h"
#include "../asm.h"

#include "cc2520.h"
#include "cc2520_aes.h" // Common file between CC2420 and CC2520 radio firmware
#include "cc2520_macros.h"

#include <string.h>

#ifdef BBR2_HW

unsigned char CC2520_waitEnc(void);

//////////////////////////////////////////////////////////////////////
// Function: CC2520_StartDecryptRXBuffer
// Author: Rares Ivan, Ion Ticus
// Description: Start decrypt buffer using CC2520 read/write Memory (as RX buffer is read only)
// Parameters: None
// Return: None
// Note: CC2520_Set_DecryptSettings() must be called before this function
//       RXFIFO is used to decrypt.
//////////////////////////////////////////////////////////////////////
void CC2520_StartDecryptRXBuffer(uint8 encLen, uint8 MicSize  )
{
    uint8 arg = 0;
    if(MicSize == 16)     { arg = 3; }
    else if(MicSize == 8) { arg = 2; }
    else                  { arg = 1; }

  if (CC2520_RADIO_1 == eActiveTransmitter )
  {
    CC2520_1_MACRO_SELECT();
    CC2520_1_MACRO_SET_AES_MODE(CC2520_SECCTRL0_CCM);      // Select AES Security mode
    CC2520_1_MACRO_DECRYPT_RXFIFO(encLen, arg); // Start decryption of data in RXFIFO
    CC2520_1_MACRO_RELEASE();
  }
}

//////////////////////////////////////////////////////////////////////
// Function: CC2520_StartDecryptAckBuffer
// Author: Prasad Samudrala, HTS Bangalore
// Description: Start decrypt buffer using RXFIFO
// Parameters: None
// Return: None
// Note: CC2520_Set_DecryptSettings() must be called before this function
//       RXFIFO is used to decrypt.
//////////////////////////////////////////////////////////////////////
void CC2520_StartDecryptAckBuffer(void )
{
  if (CC2520_RADIO_1 == eActiveTransmitter )
  {
    CC2520_1_MACRO_SELECT();
    CC2520_1_MACRO_SET_AES_MODE(CC2520_SECCTRL0_CCM);      // Select AES Security mode
    CC2520_1_MACRO_DECRYPT_ACKFIFO(); // Start decryption of data in RXFIFO
    CC2520_1_MACRO_RELEASE();
  }
}

//////////////////////////////////////////////////////////////////////
// Function: CC2520_CkDecryptRXBuffer
// Author: Rares Ivan, Ion Ticus
// Description: Check the decrypted result
// Parameters: p_ucBufferLen - buffer length (without MIC32)
// Return: AES_ERROR if fail, AES_SUCCESS if success
// Note: CC2520_StartDecryptRXBuffer() must be called before this function
//       RXFIFO is used to decrypt.
//////////////////////////////////////////////////////////////////////
unsigned char CC2520_CkDecryptRXBuffer( unsigned char p_ucBufferLen )  // Decrypt buffer using RXFIFO
{
  unsigned char rv;

  if (CC2520_RADIO_1 == eActiveTransmitter )
  {
    CC2520_1_MACRO_SELECT();
    rv = CC2520_waitEnc();                   // Wait for encryption to finish

    if( rv == AES_SUCCESS )
    {
        // Read the status of DPUSTAT register to check if MIC passed or not.
        CC2520_1_MACRO_GETREG(CC2520_DPUSTAT, rv);
        rv = ((rv & CC2520_AUTHSTAT_H) ? AES_SUCCESS : AES_ERROR);
    }
    else
    {
        Log(LOG_ERROR,LOG_M_SYS,SYSOP_GENERAL, 1, "4", 1, &p_ucBufferLen );
    }

    CC2520_1_MACRO_SET_AES_MODE(CC2520_SECCTRL0_NO_SECURITY);      // Select AES Security mode
    CC2520_1_MACRO_RELEASE();
  }

  return rv;
}

void CC2520_ReadDecryptedBuffer( unsigned char* p_pucData, unsigned char p_ucLen )
{
  unsigned int rv;
  if (CC2520_RADIO_1 == eActiveTransmitter )
  {
    CC2520_1_MACRO_SELECT();

    CC2520_1_MACRO_GET_AES_RX_PLAINTEXT(rv);
    //rv = rv + CC2520_RX_DECRYP_BUFF ;
     rv = rv + CC2520_RAM_RXBUF + 1 ;

    CC2520_1_MACRO_READ_RAM_LITTLE_E( p_pucData, rv, p_ucLen);

    CC2520_1_MACRO_RELEASE();
  }
}

//////////////////////////////////////////////////////////////////////
// Function: CC2520_StartEncryptTXBuffer
// Author: Rares Ivan
// Description: Encrypt or Encrypt&Authenticate up to 125 bytes of data
// Parameters:
//       TXFIFO is used to encrypt.
//////////////////////////////////////////////////////////////////////
void CC2520_StartEncryptTXBuffer(uint8 encLen, uint8 MicSize)
{
  uint8 arg = 0;
    
  if(MicSize == 16)     { arg = 3; }
  else if(MicSize == 8) { arg = 2; }
  else                  { arg = 1; }
  
  
  if (CC2520_RADIO_1 == eActiveTransmitter )
  {
    CC2520_1_MACRO_SELECT();

    CC2520_1_MACRO_SET_AES_MODE( CC2520_SECCTRL0_CCM);      // Select AES Security mode

    CC2520_1_MACRO_ENCRYPT_TXFIFO(encLen,arg);      // Start encryption of data in TXFIFO
    CC2520_1_MACRO_RELEASE();
  }
}
//////////////////////////////////////////////////////////////////////
// Function: CC2520_CkEncryptTXBuffer
// Author: Rares Ivan
// Description: Check the Encrypt or Encrypt&Authentication
// Parameters:
//       TXFIFO is used to encrypt.
//////////////////////////////////////////////////////////////////////
void CC2520_CkEncryptTXBuffer(void)
{
  if (CC2520_RADIO_1 == eActiveTransmitter )
  {
    CC2520_1_MACRO_SELECT();
    CC2520_waitEnc();                   // Wait for encryption to finish
    CC2520_1_MACRO_SET_AES_MODE( CC2520_SECCTRL0_NO_SECURITY);      // Select AES Security mode
    CC2520_1_MACRO_RELEASE();
  }
}

void CC2520_LoadTXBuffer( const unsigned char* p_pucData, unsigned char p_ucLen)
{
  if (CC2520_RADIO_1 == eActiveTransmitter )
  {
    CC2520_1_MACRO_SELECT();
    CC2520_1_MACRO_FLUSH_TXFIFO(); // Flush the TXFIFO first
    CC2520_1_LOAD_TX_BUFFER(p_pucData, p_ucLen);  // Load the data in to the Tx Buffer
    CC2520_1_MACRO_RELEASE();
  }
}

void CC2520_LoadRXBuffer( const unsigned char* p_pucData, unsigned char p_ucLen )
{
   if (CC2520_RADIO_1 == eActiveTransmitter )
  {
    CC2520_1_MACRO_SELECT();
    CC2520_1_LOAD_RX_BUFFER(p_pucData, p_ucLen);  // Load the data in the Rx Buffer
    CC2520_1_MACRO_RELEASE();
  }
}

//////////////////////////////////////////////////////////////////////
// Function: CC2520_SetKeySA
// Author: Ion Ticus
// Description: Set stand alone Key
// Parameters: p_pKey - encryption key
// Note:
//    Stand Alone uses KEY1 for Security Operations
//    RXFIFO & TXFIFO use KEY0 for Security Operations
//    used for user only space
//  !! interrupts must be disabled during of function
//////////////////////////////////////////////////////////////////////
__monitor void CC2520_SetKeySA( const unsigned char* p_pKey )
{
  MONITOR_ENTER();
  if (CC2520_RADIO_1 == eActiveTransmitter )
  {
    CC2520_1_MACRO_SELECT();
    CC2520_1_MACRO_SET_ENCKEY1(p_pKey);  // Load the key in RAM by overwriting KEY1 location
    CC2520_1_MACRO_RELEASE();
  }
  MONITOR_EXIT();
}

//////////////////////////////////////////////////////////////////////
// Function: CC2520_ComputeAES
// Author: Ion Ticus
// Description: Encrypt 16 bytes of data in Stand Alone buffer
// Parameters:  p_pucData - data buffer
// Note:
//    used for user only space
//  !! interrupts must be disabled during of function
//////////////////////////////////////////////////////////////////////
__monitor void CC2520_ComputeAES( const unsigned char* p_pSrc, unsigned char* p_pDst )
{
  MONITOR_ENTER();
  if (CC2520_RADIO_1 == eActiveTransmitter )
  {
    CC2520_1_MACRO_SELECT();
    CC2520_1_MACRO_SET_SABUF(p_pSrc); // Load the buffer into the SA buffer (16 bytes)
    CC2520_waitEnc();                   // Wait for encryption to finish

    CC2520_1_MACRO_RESELECT();
    CC2520_1_MACRO_ENCRYPT_SA_BUFFER();    // Start encryption of data in SA Buffer
    CC2520_waitEnc();                   // Wait for encryption to finish

    CC2520_1_MACRO_RESELECT();
    CC2520_1_MACRO_GET_SABUF(p_pDst);   // Read the buffer from the SA buffer (16 bytes)
    CC2520_1_MACRO_RELEASE();
  }
  MONITOR_EXIT();
}

unsigned char CC2520_waitEnc(void)
{
  uint8  ucStatus;
  volatile uint16 unTmpCnt = 500; // 1ms timeout
  if (CC2520_RADIO_1 == eActiveTransmitter )
  {
    do
    {
      // Check if any high priority instruction is active
      CC2520_1_MACRO_GETREG(CC2520_DPUSTAT, ucStatus); // 2us at 10Mhz
      if( !(--unTmpCnt) )
      {
          CC2520_1_MACRO_RELEASE();
          Log(LOG_ERROR,LOG_M_SYS,SYSOP_GENERAL, 1, "3", 1, &ucStatus, sizeof(g_unCC2520SecCtrl_0), &g_unCC2520SecCtrl_0, sizeof(g_unCC2520SecCtrl_1), &g_unCC2520SecCtrl_1 );
          return AES_ERROR;
      }
    }
    while( ucStatus &  CC2520_DPUH_ACTIVE);

    CC2520_1_MACRO_RELEASE();
  }
  return AES_SUCCESS;
}

#endif

