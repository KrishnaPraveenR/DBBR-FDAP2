//=========================================================================================
// Name:        CC2520_AES.h
// Author:      Gourav Sharma
// Date:        February 2010
// Description: CC2520 Driver - AES Module
// Changes:     
// Revisions:   
//=========================================================================================


#ifndef CC2520_AES_H
#define CC2520_AES_H

void CC2520_StartDecryptRXBuffer( uint8 encLen, uint8 MicSize );  // Start decrypt buffer using RXFIFO
void CC2520_StartEncryptTXBuffer(uint8 encLen, uint8 MicSize);
void CC2520_StartDecryptAckBuffer(void );
unsigned char CC2520_CkDecryptRXBuffer( unsigned char p_ucBufferLen );  // Decrypt buffer using RXFIFO
void CC2520_CkEncryptTXBuffer( void );  // Encrypt buffer using TXFIFO
void CC2520_LoadTXBuffer( const unsigned char* p_pucData, unsigned char p_ucLen ); 
void CC2520_ReadDecryptedBuffer( unsigned char* p_pucData, unsigned char p_ucLen ); 
// Encrypt buffer using TXFIFO
void CC2520_LoadRXBuffer( const unsigned char* p_pucData, unsigned char p_ucLen );
__monitor void CC2520_SetKeySA( const unsigned char* p_pKey );
__monitor void CC2520_ComputeAES( const unsigned char* p_pSrc, unsigned char* p_pDst );

#endif
