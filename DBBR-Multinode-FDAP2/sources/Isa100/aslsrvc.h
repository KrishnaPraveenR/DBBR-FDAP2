////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Eduard Erdei
/// Date:         January 2008
/// Description:  This file holds definitions of the application sub-layer data entity
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _NIVIS_ASLSRVC_H_
#define _NIVIS_ASLSRVC_H_

#include "aslde.h"
#include "../typedef.h"

#define GET_NEW_REQ_ID g_ucReqID++


extern const uint8 * ASLSRVC_GetGenericObject(  const uint8 *        p_pInBuffer, 
                                                uint16               p_nInBufLen, 
                                                GENERIC_ASL_SRVC *   p_pGenericObj,
                                                const uint8 *       p_pSrcAddr128 );

uint8 ASLSRVC_AddGenericObject( void *        p_pObj, 
                                uint8         p_ucServiceType,
                                uint8         p_ucPriority,
                                uint8         p_ucSrcTSAPID,
                                uint8         p_ucDstTSAPID,
                                uint16        p_unAppHandler,
                                const uint8 * p_pDstAddr, 
                                uint16        p_unContractID,
                                uint16        p_unBinSize);

uint8 ASLSRVC_AddGenericObjectToSM( void * p_pObj, uint8 p_ucServiceType );
extern uint8 ASLSRVC_GetLastReqID( void );
                                      
const uint8 * ASLSRVC_ExtractAPDUHeader( const uint8 * p_pData,
                                         uint16 *      p_unSrcOID,
                                         uint16 *      p_unDstOID);
#endif // _NIVIS_ASLSRVC_H_
