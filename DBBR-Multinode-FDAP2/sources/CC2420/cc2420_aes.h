//=========================================================================================
// Name:        CC2420_AES.h
// Author:      Rares Ivan
// Date:        February 2008
// Description: CC2420 Driver - AES Module
// Changes:     
// Revisions:   
//=========================================================================================

#ifdef BBR1_HW
#ifndef CC2420_AES_H
#define CC2420_AES_H

void CC2420_StartDecryptRXBuffer( uint16 );  // Start decrypt buffer using RXFIFO
unsigned char CC2420_CkDecryptRXBuffer( unsigned char p_ucBufferLen );  // Decrypt buffer using RXFIFO
void CC2420_StartEncryptTXBuffer( uint16 );  // Encrypt buffer using TXFIFO
void CC2420_CkEncryptTXBuffer( void );  // Encrypt buffer using TXFIFO
void CC2420_LoadTXBuffer( const unsigned char* p_pucData, unsigned char p_ucLen ); 
void CC2420_ReadDecryptedBuffer( unsigned char* p_pucData, unsigned char p_ucLen ); 
// Encrypt buffer using TXFIFO
void CC2420_LoadRXBuffer( const unsigned char* p_pucData, unsigned char p_ucLen );
__monitor void CC2420_SetKeySA( const unsigned char* p_pKey );
__monitor void CC2420_ComputeAES( const unsigned char* p_pSrc, unsigned char* p_pDst );

#endif
#endif