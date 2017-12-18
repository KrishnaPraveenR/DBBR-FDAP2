////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Mihaela Goloman
/// Date:         December 2008
/// Description:  This file implements the concentrator object in DMAP
/// Changes:      Created
/// Revisions:    1.0
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include "provision.h"
#include "dmap_co.h"
#include "dmap_utils.h"
#include "dmap_dmo.h"
#include "dmap_dlmo.h"
#include "dmap.h"
#include "uap_mo.h"
#include "aslsrvc.h"
#include "sfc.h"
#include "mlsm.h"
#include "tlde.h"

#define WAIT_FOR_CO_OBJECTS 30 // seconds

#if( _UAP_TYPE == UAP_TYPE_SIMPLE_API )
  #define MAX_CO_OBJECTS_NO    2
#else
  #define MAX_CO_OBJECTS_NO    1
#endif

CO_STRUCT g_astCOTable[MAX_CO_OBJECTS_NO];
CO_STRUCT* g_pstCrtCO;
uint8 g_ucCOObjectNo;


const uint8 c_ucMaxPblItemsNo = (uint8)MAX_PUBLISH_ITEMS;

extern const uint16 c_unMaxNsduSize; //from nlme


static void CO_readContract(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize);

static uint8 CO_writeAssocEndp(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);

static void CO_readObjAttr(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize);
static uint8 CO_writeObjAttr(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);

static uint8 CO_protectReadOnlyAttribute(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize);
static uint8 CO_WriteRevision(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize);

const CO_FCT_STRUCT c_aCOFct[] =
{
   ATTR_CONST(g_astCOTable[0].m_ucRevision),           RW_ATTR_ACCESS, DMAP_ReadUint8,     CO_WriteRevision,      
   ATTR_CONST(g_astCOTable[0].m_stEndpoint),           RW_ATTR_ACCESS, CO_readAssocEndp,   CO_writeAssocEndp,
   ATTR_CONST(g_astCOTable[0].m_stContract),           RO_ATTR_ACCESS, CO_readContract,    CO_protectReadOnlyAttribute,
   (void*)&c_ucMaxPblItemsNo, sizeof(c_ucMaxPblItemsNo), RO_ATTR_ACCESS, DMAP_ReadUint8,     CO_protectReadOnlyAttribute,
   ATTR_CONST(g_astCOTable[0].m_ucPubItemsNo),         RO_ATTR_ACCESS, DMAP_ReadUint8,     CO_protectReadOnlyAttribute,
   ATTR_CONST(g_astCOTable[0].m_aAttrDescriptor),      RW_ATTR_ACCESS, CO_readObjAttr,     CO_writeObjAttr
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman
/// @brief Initializes the health reports concentrator object
/// @param none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void CO_Init()
{
  // read eeprom status
//  ReadPersistentData( (uint8*)&g_stCO, APPLICATION_START_ADDR, sizeof(g_stCO) );

//  if( 0xA5 != g_stCO.m_ucIsData ) // unknown version
//  {
    g_ucCOObjectNo = 0;  
    
    CO_STRUCT * pstCO = g_astCOTable;    
      
    for(; pstCO < g_astCOTable + MAX_CO_OBJECTS_NO; pstCO++)
    {  
        memset(pstCO, 0, (uint32)&((CO_STRUCT*)0)->m_ucFreshSeqNo); // preserve sequence no value
    }
    
//  }
    
    
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief Writes publish endpoint data
/// @param p_pValue - endpoint address
/// @param p_pBuf - endpoint data to be written
/// @param p_ucSize - endpoint data size
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
static uint8 CO_writeAssocEndp(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize)
{
  if( 26 != p_ucSize )
    return SFC_INVALID_SIZE;

  if ( g_pstCrtCO->m_stContract.m_ucStatus == AWAITING_CONTRACT_ESTABLISHMENT
        || g_pstCrtCO->m_stContract.m_ucStatus == CONTRACT_REQUEST_TERMINATION
        || g_pstCrtCO->m_stContract.m_ucStatus == AWAITING_CONTRACT_TERMINATION )
  {
      // protect the CO object state machine, to avoid duplicates on contract table
      return SFC_INNAPROPRIATE_PROCESS_MODE;
  }
  
  g_pstCrtCO->m_ucRevision++;  //Increment the CO revision whenever write to CO's attributes happens

  COMM_ASSOC_ENDP* p_stCommAssocEndp = (COMM_ASSOC_ENDP*)p_pValue;

  memcpy(p_stCommAssocEndp->m_aRemoteAddr128, p_pBuf, 16);
  p_pBuf += 16;

  p_pBuf = DMAP_ExtractUint16(p_pBuf, &p_stCommAssocEndp->m_unRemoteTLPort);
  p_pBuf = DMAP_ExtractUint16(p_pBuf, &p_stCommAssocEndp->m_unRemoteObjID);

  p_stCommAssocEndp->m_ucStaleDataLimit = *(p_pBuf++);

  p_pBuf = DMAP_ExtractUint16(p_pBuf, (uint16*)&p_stCommAssocEndp->m_nPubPeriod);

  p_stCommAssocEndp->m_ucIdealPhase         = *(p_pBuf++);
  p_stCommAssocEndp->m_ucPubAutoRetransmit  = *(p_pBuf++);
  p_stCommAssocEndp->m_ucCfgStatus          = *(p_pBuf++);

  if (g_pstCrtCO->m_stContract.m_ucStatus == CONTRACT_ACTIVE_AS_REQUESTED || g_pstCrtCO->m_stContract.m_ucStatus == CONTRACT_INACTIVE)
  {
      // before request a new contract, terminate the current one
      g_pstCrtCO->m_stContract.m_ucStatus = CONTRACT_REQUEST_TERMINATION;
  }

  // save persistent data
  //g_pstCrtCO->m_ucIsData = ENDPOINT_WRITTEN | ATTR_WRITTEN;

  //EraseSector( APPLICATION_SECTOR_NO );
  //WritePersistentData( (uint8*)&g_stCO, APPLICATION_START_ADDR, sizeof(g_stCO) ); // m_ucIsData + m_stEndpoint

  return SFC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman
/// @brief Reads publish endpoint data
/// @param p_pValue - endpoint address
/// @param p_pBuf - buffer to gather endpoint data
/// @param p_ucSize - endpoint data size
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void CO_readAssocEndp(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize)
{
  COMM_ASSOC_ENDP* p_stCommAssocEndp = (COMM_ASSOC_ENDP*)p_pValue;
  uint8* pStart = p_pBuf;

  memcpy(p_pBuf, &p_stCommAssocEndp->m_aRemoteAddr128, 16);
  p_pBuf += 16;

  p_pBuf = DMAP_InsertUint16( p_pBuf, p_stCommAssocEndp->m_unRemoteTLPort);
  p_pBuf = DMAP_InsertUint16( p_pBuf, p_stCommAssocEndp->m_unRemoteObjID);

  *(p_pBuf++) = p_stCommAssocEndp->m_ucStaleDataLimit;

  p_pBuf = DMAP_InsertUint16( p_pBuf, *(uint16*)&p_stCommAssocEndp->m_nPubPeriod);

  *(p_pBuf++) = p_stCommAssocEndp->m_ucIdealPhase;
  *(p_pBuf++) = p_stCommAssocEndp->m_ucPubAutoRetransmit;
  *(p_pBuf++) = p_stCommAssocEndp->m_ucCfgStatus;

  *p_ucSize = p_pBuf - pStart;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman
/// @brief Reads publish contract data
/// @param p_pValue - publish contract data adress
/// @param p_pBuf - buffer to gather publish contract data
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
static void CO_readContract(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize)
{
  COMM_CONTRACT_DATA* p_stContractData = (COMM_CONTRACT_DATA*)p_pValue;
  uint8* pStart = p_pBuf;

  p_pBuf = DMAP_InsertUint16( p_pBuf, p_stContractData->m_unContractID);
  *(p_pBuf++) = p_stContractData->m_ucStatus;
  *(p_pBuf++) = p_stContractData->m_ucActualPhase;

  *p_ucSize = p_pBuf - pStart;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief Reads publish items information (object, attribute and value size)
/// @param p_pValue - address of publish items information
/// @param p_pBuf - buffer to gather publish items
/// @param p_ucSize - publish items information size
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
static void CO_readObjAttr(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize)
{
  OBJ_ATTR_IDX_AND_SIZE * pstNext = (OBJ_ATTR_IDX_AND_SIZE*)p_pValue; //g_pstCrtCO->m_aAttrDescriptor
  OBJ_ATTR_IDX_AND_SIZE * pstEnd  = (OBJ_ATTR_IDX_AND_SIZE*)p_pValue + g_pstCrtCO->m_ucPubItemsNo; //g_pstCrtCO->m_aAttrDescriptor+g_pstCrtCO->m_ucPubItemsNo

  uint8* pStart = p_pBuf;

  while (pstNext < pstEnd)
  {
      p_pBuf = DMAP_InsertUint16( p_pBuf, pstNext->m_unObjID);
      p_pBuf = DMAP_InsertUint16( p_pBuf, pstNext->m_unAttrID);
      p_pBuf = DMAP_InsertUint16( p_pBuf, pstNext->m_unAttrIdx);
      p_pBuf = DMAP_InsertUint16( p_pBuf, pstNext->m_unSize);

      pstNext++;
  }
  *p_ucSize = p_pBuf - pStart;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief Writes publish items information (object, attribute and value size)
/// @param p_pValue - address of publish items information
/// @param p_pBuf - buffer containing publish items
/// @param p_ucSize - publish items information size
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
static uint8 CO_writeObjAttr(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize)
{
  if(0 != p_ucSize%8) //(nof items * 8 bytes per obj&attr_structure)
    return SFC_INVALID_SIZE;

  uint8 ucItemsNo = p_ucSize/8;

  if (ucItemsNo > MAX_PUBLISH_ITEMS )
      return SFC_INVALID_VALUE;
  
  g_pstCrtCO->m_ucRevision++;  //Increment the CO revision whenever write to CO's attributes happens    

  g_pstCrtCO->m_ucPubItemsNo = ucItemsNo;

  OBJ_ATTR_IDX_AND_SIZE * pstNext = (OBJ_ATTR_IDX_AND_SIZE*)p_pValue; //g_pstCrtCO->m_aAttrDescriptor

  while( ucItemsNo-- )
  {
      p_pBuf = DMAP_ExtractUint16( p_pBuf, &pstNext->m_unObjID);
      p_pBuf = DMAP_ExtractUint16( p_pBuf, &pstNext->m_unAttrID);
      p_pBuf = DMAP_ExtractUint16( p_pBuf, &pstNext->m_unAttrIdx);
      p_pBuf = DMAP_ExtractUint16( p_pBuf, &pstNext->m_unSize);

      pstNext++;
  }

  // store data in eeprom
//  g_pstCrtCO->m_ucIsData = ENDPOINT_WRITTEN | ATTR_WRITTEN;

//  EraseSector( APPLICATION_SECTOR_NO );
//  WritePersistentData( (uint8*)&g_stCO, APPLICATION_START_ADDR, sizeof(g_stCO) ); // m_ucIsData + m_stEndpoint

  return SFC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman
/// @brief Reads a concentrator object attribute
/// @param p_pstCO - pointer to a Co element from the CO table
/// @param p_unAttrID - attribute identifier
/// @param p_unSize - attribute data size
/// @param p_pBuf - attribute data
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 CO_Read(uint16 p_unAttrID, uint16* p_punSize, uint8* p_pBuf)
{
  // check if valid attribute id
  if ( (--p_unAttrID) >= CO_ATTR_NO - 1 ) // transform p_unAttrID from 1 base to 0 base
      return SFC_INVALID_ATTRIBUTE;

  // check if size of buffer is large enough
  if ( c_aCOFct[p_unAttrID].m_ucSize > *p_punSize )
  {
      *p_punSize = 0;
      return SFC_INVALID_SIZE;
  }

  uint8 ucSize = c_aCOFct[p_unAttrID].m_ucSize;

  // do not check access rights; all CO attributes are readable
  // call specific read function
  c_aCOFct[p_unAttrID].m_pReadFct((uint8*)g_pstCrtCO + ((uint8*)c_aCOFct[p_unAttrID].m_pValue - (uint8*)g_astCOTable),
                                 p_pBuf,
                                 &ucSize);
  *p_punSize = ucSize;
  return SFC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman
/// @brief Writes a concentrator object attribute
/// @param p_unAttrID - attribute identifier
/// @param p_unSize - attribute data size
/// @param p_pBuf - buffer to gather attribute data
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 CO_Write(uint16 p_unAttrID, uint16 p_unSize, const uint8* p_pBuf)
{
  //check if valid attribute id
  if ( (--p_unAttrID) >= CO_ATTR_NO - 1 ) // transform p_unAttrID from 1 base to 0 base
    return SFC_INVALID_ATTRIBUTE;

  return c_aCOFct[p_unAttrID].m_pWriteFct((uint8*)g_pstCrtCO + ((uint8*)c_aCOFct[p_unAttrID].m_pValue - (uint8*)g_astCOTable), 
                                          p_pBuf, 
                                          p_unSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief Contains Concentrator Object's state machine
/// @param none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void CO_ConcentratorTask()
{    
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Updates the CO contract descriptor. Called by DMAP each time a new contract entry
///         is written into DMO contract table.
/// @param  p_pContract - pointer to the new contract
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void CO_NotifyAddContract(DMO_CONTRACT_ATTRIBUTE * p_pContract)
{
//  if (g_stCO.m_stContract.m_ucStatus != AWAITING_CONTRACT_ESTABLISHMENT)
//      return;

}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Checks if a deleted contract in DMAP is in the scope of the concentrator object
/// @param  p_unContractID  - contract ID
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void CO_NotifyContractDeletion(uint16 p_unContractID)
{
    CO_STRUCT* pstCrtCO = FindConcentratorByContract(p_unContractID);
    
    if( NULL == pstCrtCO )
        return;
    
    if( pstCrtCO->m_stContract.m_ucStatus == CONTRACT_ACTIVE_AS_REQUESTED || 
        pstCrtCO->m_stContract.m_ucStatus == AWAITING_CONTRACT_TERMINATION )
    {
        pstCrtCO->m_stContract.m_ucStatus = ENDPOINT_NOT_CONFIGURED;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Empty write function; used to protect read-only attributes
/// @return - service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 CO_protectReadOnlyAttribute(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize)
{
  // empty function
  return SFC_READ_ONLY_ATTRIBUTE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Finds a concentrator object inside the concentrator table 
/// @param  p_unSrcObjectID  - object identifier of the CO
/// @param  p_unSrcTSAP  - source transport SAP ID
/// @return - pointer to a concentrator structure
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
CO_STRUCT* FindConcentratorByObjId(uint16 p_unSrcObjectID, uint16 p_unSrcTSAP)
{
    CO_STRUCT* pstCrtCO = g_astCOTable;
  
    for(; pstCrtCO < g_astCOTable + g_ucCOObjectNo; pstCrtCO++)
    {
        if( pstCrtCO->m_unSrcObjectId == p_unSrcObjectID &&
            pstCrtCO->m_unSrcTSAP == p_unSrcTSAP )
            return pstCrtCO;
    }
    
    return NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Finds a concentrator object inside the concentrator table 
/// @param  p_unContractID  - contract ID of the concentrator
/// @return - pointer to a concentrator structure
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
CO_STRUCT* FindConcentratorByContract(uint16 p_unContractID)
{
    CO_STRUCT* pstCrtCO = g_astCOTable;
  
    for(; pstCrtCO < g_astCOTable + g_ucCOObjectNo; pstCrtCO++)
    {
        if( pstCrtCO->m_stContract.m_unContractID == p_unContractID )
            return pstCrtCO;
    }
    
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Add a new concentrator object inside the concentrator table if not already exist 
/// @param  p_unSrcObjectID  - object identifier of the CO
/// @param  p_unSrcTSAP  - source transport SAP ID
/// @return - pointer to a concentrator structure(already existing or the new added) or NULL
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
CO_STRUCT* AddConcentratorObject(uint16 p_unSrcObjectID, uint16 p_unSrcTSAP)
{
    CO_STRUCT* pstCrtCO = FindConcentratorByObjId(p_unSrcObjectID, p_unSrcTSAP);   

    if( pstCrtCO )
        return pstCrtCO;
    else
    {
        if( g_ucCOObjectNo < MAX_CO_OBJECTS_NO )
        {
            //add a new CO object inside table
            pstCrtCO = g_astCOTable + g_ucCOObjectNo;
            pstCrtCO->m_unSrcObjectId = p_unSrcObjectID;
            pstCrtCO->m_unSrcTSAP = p_unSrcTSAP;
            g_ucCOObjectNo++;
            return pstCrtCO;
        }
    }
    
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author Uday, HTS Bangalore
/// @brief  Writes CO revision and returns success
/// @param  p_unSrcObjectID  - CO revision, value to be written and size
/// @return - succcess / invalid size
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
static uint8 CO_WriteRevision(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize)
{
  if (p_ucSize > 1)
    return SFC_INVALID_SIZE;
  
  *(uint8*)(p_pValue) = *p_pBuf;
  return SFC_SUCCESS;
}
