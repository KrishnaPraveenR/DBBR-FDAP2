////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Mihaela Goloman
/// Date:         November 2008
/// Description:  This file holds definitions of the dmap_utils module
/// Changes:      Created
/// Revisions:    1.0
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _NIVIS_DMAP_UTILS_H_
#define _NIVIS_DMAP_UTILS_H_

#include "../typedef.h"
#include "config.h"

#define RO_ATTR_ACCESS   0x01  // read only attribute access
#define WO_ATTR_ACCESS   0x02  // write only attribute access
#define RW_ATTR_ACCESS   0x03  // read/write attribute access

#define ATTR_CONST(x) (void*)&x, sizeof(x)

typedef void (*DMAP_READ_FCT)(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize);
typedef void (*DMAP_WRITE_FCT)(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);

typedef struct
{
  void *          m_pValue;
  uint8           m_ucSize;
  DMAP_READ_FCT   m_pReadFct;
  DMAP_WRITE_FCT  m_pWriteFct;
  
} DMAP_FCT_STRUCT; 

void DMAP_ReadUint8(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);
void DMAP_ReadUint16(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);
void DMAP_ReadUint32(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);
void DMAP_ReadVisibleString(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);
void DMAP_ReadAlertDescriptor(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);
void DMAP_ReadCompressAlertDescriptor(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);
void DMAP_ReadContractMeta(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);
void DMAP_EmptyReadFunc(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);

void DMAP_WriteUint8(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize);
void DMAP_WriteUint16(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize);
void DMAP_WriteUint32(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize);
void DMAP_WriteVisibleString(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize);
void DMAP_WriteAlertDescriptor(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize);
void DMAP_WriteCompressAlertDescriptor(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize);

uint8 * DMAP_InsertUint32( uint8 * p_pData, uint32  p_ulValue);
uint8 * DMAP_InsertUint16( uint8 * p_pData, uint16  p_unValue);
const uint8 * DMAP_ExtractUint32( const uint8 * p_pData, uint32 * p_pulValue);
const uint8 * DMAP_ExtractUint16( const uint8 * p_pData, uint16 * p_punValue);

uint8 DMAP_ReadAttr(uint16 p_unAttrID, uint16* p_punBufferSize, uint8* p_pucRspBuffer, const DMAP_FCT_STRUCT * p_pDmapFct, uint8 p_ucAttrNo );
uint8 DMAP_WriteAttr(uint16 p_unAttrID, uint8 p_ucBufferSize, const uint8* p_pucBuffer, const DMAP_FCT_STRUCT * p_pDmapFct, uint8 p_ucAttrNo );
uint16 DMAP_GetAlignedLength(uint16 p_unLen);

#endif //_NIVIS_DMAP_UTILS_H_
