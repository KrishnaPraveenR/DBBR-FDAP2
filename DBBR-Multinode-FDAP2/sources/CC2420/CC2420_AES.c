//=========================================================================================
// Name:        CC2420_AES.c
// Author:      Rares Ivan
// Date:        February 2008
// Description: CC2420 Driver - AES Module
// Changes:     
// Revisions:   
//=========================================================================================
#ifdef BBR1_HW
#include "../global.h"
#include "../digitals.h"
#include "../spi1.h"
#include "../system.h"
#include "../asm.h"
#include "cc2420.h"
#include "cc2420_aes.h"
#include "cc2420_macros.h"

#include <string.h>

unsigned char CC2420_waitEnc(void);

//////////////////////////////////////////////////////////////////////
// Function: CC2420_StartDecryptRXBuffer
// Author: Rares Ivan, Ion Ticus
// Description: Start decrypt buffer using RXFIFOunsigned char
// Parameters: None
// Return: None
// Note: CC2420_Set_DecryptSettings() must be called before this function
//       RXFIFO is used to decrypt.
//////////////////////////////////////////////////////////////////////
void CC2420_StartDecryptRXBuffer( uint16 MicSize )
{
  CC2420_MACRO_SELECT();
  if(MicSize == 8) { CC2420_MACRO_SET_MIC_LEN(3); }
  else             { CC2420_MACRO_SET_MIC_LEN(1); }
    CC2420_MACRO_SET_AES_MODE(CC2420_SECCTRL0_CCM);      // Select AES Security mode  
    CC2420_MACRO_STROBE(CC2420_SRXDEC); // Start decryption of data in RXFIFO
  CC2420_MACRO_RELEASE();
}    
//////////////////////////////////////////////////////////////////////
// Function: CC2420_CkDecryptRXBuffer
// Author: Rares Ivan, Ion Ticus
// Description: Check the decrypted result
// Parameters: p_ucBufferLen - buffer length (without MIC32)
// Return: AES_ERROR if fail, AES_SUCCESS if success
// Note: CC2420_StartDecryptRXBuffer() must be called before this function
//       RXFIFO is used to decrypt.
//////////////////////////////////////////////////////////////////////
unsigned char CC2420_CkDecryptRXBuffer( unsigned char p_ucBufferLen )  // Decrypt buffer using RXFIFO
{
    unsigned char rv;

    CC2420_MACRO_SELECT();
      rv = CC2420_waitEnc();                   // Wait for encryption to finish
  
      if( rv == AES_SUCCESS )
      {
        // Read the content of RXFIFO.   
        // RXFIFO begins at RAM address 0x080. Add 1 to skip frame len byte (first byte) which should be read unchanged (p_ucLen + 2)
        CC2420_MACRO_READ_RAM_LITTLE_E( (unsigned char*)&rv, 0x80+p_ucBufferLen, 1); // offset = 0x81+p_ucBufferLen-1
        CC2420_MACRO_RELEASE();
        
        rv = (rv ? AES_ERROR : AES_SUCCESS); 
        
        CC2420_MACRO_SELECT();        
      }
      else
      {
          Log(LOG_ERROR,LOG_M_SYS,SYSOP_GENERAL, 1, "4", 1, &p_ucBufferLen );
      }
       
      CC2420_MACRO_SET_AES_MODE(CC2420_SECCTRL0_NO_SECURITY);      // Select AES Security mode
  CC2420_MACRO_RELEASE();

  return rv;
}

void CC2420_ReadDecryptedBuffer( unsigned char* p_pucData, unsigned char p_ucLen )
{
  unsigned int rv;
  CC2420_MACRO_SELECT();
  
    // Read the content of RXFIFO.   
    // RXFIFO begins at RAM address 0x080. Add 1 to skip frame len byte (first byte) which should be read unchanged (p_ucLen + 2)
    //CC2420_MACRO_WAIT4OSC(rv);
    CC2420_MACRO_GET_AES_RX_PLAINTEXT(rv);
    rv += 0x81;
    
    CC2420_MACRO_READ_RAM_LITTLE_E( p_pucData, rv, p_ucLen);

  CC2420_MACRO_RELEASE();  
}

//////////////////////////////////////////////////////////////////////
// Function: CC2420_StartEncryptTXBuffer
// Author: Rares Ivan
// Description: Encrypt or Encrypt&Authenticate up to 125 bytes of data
// Parameters: 
//       TXFIFO is used to encrypt.
//////////////////////////////////////////////////////////////////////
void CC2420_StartEncryptTXBuffer(uint16 MicSize)
{
  CC2420_MACRO_SELECT();
  if(MicSize == 8) { CC2420_MACRO_SET_MIC_LEN(3); } 
  else             { CC2420_MACRO_SET_MIC_LEN(1); }
    CC2420_MACRO_SET_AES_MODE(CC2420_SECCTRL0_CCM);      // Select AES Security mode    
    CC2420_MACRO_STROBE(CC2420_STXENC); // Start encryption of data in TXFIFO      
  CC2420_MACRO_RELEASE();
}

//////////////////////////////////////////////////////////////////////
// Function: CC2420_CkEncryptTXBuffer
// Author: Rares Ivan
// Description: Encrypt or Encrypt&Authenticate up to 125 bytes of data
// Parameters: 
//       TXFIFO is used to encrypt.
//////////////////////////////////////////////////////////////////////
void CC2420_CkEncryptTXBuffer(void)
{
  CC2420_MACRO_SELECT();
            
      CC2420_waitEnc();                   // Wait for encryption to finish
                      
      CC2420_MACRO_SET_AES_MODE( CC2420_SECCTRL0_NO_SECURITY);      // Select AES Security mode
  CC2420_MACRO_RELEASE();
}

void CC2420_LoadTXBuffer( const unsigned char* p_pucData, unsigned char p_ucLen )
{
  CC2420_MACRO_SELECT();
    //CC2420_MACRO_WAIT4OSC(rv); // Make sure the CC2420's Oscilator is stabilized before accesing RAM
    CC2420_MACRO_FLUSH_TXFIFO(); // Flush the TXFIFO first
    
    // Load the buffer into the TXFIFO buffer [ packet len 1 byte (data len, max 125 bytes + 2 bytes FCS), data up to 125 bytes
    SPI1_WriteByte( CC2420_TXFIFO );                  // Write the TXFIFO address (Instruct CC2420 that a write to RAM's TXFIFO is desired)
    SPI1_WriteByte( ((unsigned char)(p_ucLen + 2)) ); // Write frame length byte into the TXFIFO. (Added 2 bytes FCS (max value 125 + 2 = 127 bytes) to frame length)
    SPI1_WriteBuff(p_pucData, p_ucLen);        // Write the data into the TXFIFO
  CC2420_MACRO_RELEASE();        
}

void CC2420_LoadRXBuffer( const unsigned char* p_pucData, unsigned char p_ucLen )
{
  CC2420_MACRO_SELECT();
    //CC2420_MACRO_WAIT4OSC(rv); // Make sure the CC2420's Oscilator is stabilized before accesing RAM
    CC2420_MACRO_FLUSH_RXFIFO(); // Flush the RXFIFO first
    
    // Load the buffer into the RXFIFO buffer [ packet len 1 byte (data len, max 125 bytes + 2 bytes FCS), data up to 125 bytes
    SPI1_WriteByte( CC2420_RXFIFO );                  // Write the RXFIFO address (Instruct CC2420 that a write to RAM's RXFIFO is desired)
    SPI1_WriteByte( ((unsigned char)(p_ucLen + 2)) ); // Write frame length byte into the RXFIFO. (Added 2 bytes FCS (max value 125 + 2 = 127 bytes) to frame length)
    SPI1_WriteBuff(p_pucData, p_ucLen);        // Write the data into the RXFIFO
  CC2420_MACRO_RELEASE();        
}




//////////////////////////////////////////////////////////////////////
// Function: CC2420_SetKeySA
// Author: Ion Ticus
// Description: Set stand alone Key
// Parameters: p_pKey - encryption key
// Note:
//    Stand Alone uses KEY1 for Security Operations
//    RXFIFO & TXFIFO use KEY0 for Security Operations
//    used for user only space
//  !! interrupts must be disabled during of function
//////////////////////////////////////////////////////////////////////
__monitor void CC2420_SetKeySA( const unsigned char* p_pKey )
{
  MONITOR_ENTER();
    CC2420_MACRO_SELECT();
    
    CC2420_MACRO_SET_ENCKEY1(p_pKey);  // Load the key in RAM by overwriting KEY1 location      
      
    CC2420_MACRO_RELEASE();
  MONITOR_EXIT();
}

//////////////////////////////////////////////////////////////////////
// Function: CC2420_ComputeAES
// Author: Ion Ticus
// Description: Encrypt 16 bytes of data in Stand Alone buffer
// Parameters:  p_pucData - data buffer
// Note:
//    used for user only space
//  !! interrupts must be disabled during of function
//////////////////////////////////////////////////////////////////////
__monitor void CC2420_ComputeAES( const unsigned char* p_pSrc, unsigned char* p_pDst )
{
  MONITOR_ENTER();
  
  CC2420_MACRO_SELECT();
    CC2420_MACRO_SET_SABUF(p_pSrc); // Load the buffer into the SA buffer (16 bytes)

  CC2420_MACRO_RESELECT();
    
    CC2420_waitEnc();                   // Wait for encryption to finish
    
    CC2420_MACRO_STROBE(CC2420_SAES);    // Start encryption of data in SA 
    CC2420_waitEnc();                   // Wait for encryption to finish
        
    CC2420_MACRO_GET_SABUF(p_pDst);   // Read the buffer from the SA buffer (16 bytes)  
  CC2420_MACRO_RELEASE();

  MONITOR_EXIT();
}


unsigned char CC2420_waitEnc(void)
{
    uint8  ucStatus;
    uint16 unTmpCnt = 500; // 1ms timeout
    do
    { 
      CC2420_MACRO_GET_STATUS(ucStatus); // 2us at 10Mhz
      if( !(--unTmpCnt) )
      {
          Log(LOG_ERROR,LOG_M_SYS,SYSOP_GENERAL, 1, "3", 1, &ucStatus, sizeof(g_unCC2420SecCtrl_0), &g_unCC2420SecCtrl_0 );
          return AES_ERROR;
      }      
    }
    while( ucStatus & ( 1 << CC2420_ENC_BUSY) );
    
    return AES_SUCCESS;
}

#endif 