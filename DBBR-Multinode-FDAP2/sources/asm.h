/***************************************************************************************************
* Name:         asm.h
* Author:       Marius Vilvoi
* Date:         October 2007
* Description:  This header file is provided ...
*               This file holds definitions of the ...
* Changes:
* Revisions:
****************************************************************************************************/
#ifndef _NIVIS_ASM_H_
#define _NIVIS_ASM_H_

#include "typedef.h"



#ifdef BBR2_HW
#include "CC2520/cc2520_aes.h"
#include "CC2520/CC2520.h"
#include "CC2520/CC2520_Macros.h"
#endif
#ifdef BBR1_HW
#include "CC2420/cc2420_aes.h"
#include "CC2420/CC2420.h"
#include "CC2420/CC2420_Macros.h"
#endif



#define AES_SUCCESS             0
#define AES_ERROR               1



#define MAX_NONCE_LEN           13

//void ASM_Init(void);

#define AES_Crypt_User   PHY_AES_EncryptUser
#define AES_Decrypt_User PHY_AES_DecryptUser
              
#endif /* _NIVIS_ASM_H_ */

