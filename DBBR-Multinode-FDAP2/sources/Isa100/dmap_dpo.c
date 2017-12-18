////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Mihaela Goloman
/// Date:         February 2009
/// Description:  This file implements the device provisioning object in DMAP
/// Changes:      Created
/// Revisions:    1.0
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include "dmap_dpo.h"
#include "uap.h"
#include "uap_data.h"
#include "dmap.h"
#include "dlmo_utils.h"
#include "dmap_dmo.h"
#include "dmap_dlmo.h"
#include "dmap_co.h"
#include "mlsm.h"
#include "dlme.h"
#include "dmap_armo.h"

#ifdef PROVISIONING_DEVICE
  #undef BACKBONE_SUPPORT
#endif

    // provision info   
#ifndef FILTER_BITMASK        
    #define FILTER_BITMASK 0xFFFF //specific network only      
#endif

#ifndef FILTER_TARGETID        
    #define FILTER_TARGETID 0x0001 // default provisioning network      
#endif

#define DPO_DEFAULT_PROV_SIGNATURE 0xFF

#define ISA100_GW_UAP       1

#ifdef BACKBONE_SUPPORT  

  const DPO_STRUCT c_stDefaultDPO = 
  {
      DPO_DEFAULT_PROV_SIGNATURE, //   m_ucStructSignature; 
      DEFAULT_JOIN_ONLY, //   m_ucJoinMethodCapability; 
      TRUE, //   m_ucAllowProvisioning;
  #ifndef BACKBONE_SUPPORT
      TRUE, //   m_ucAllowOverTheAirProvisioning;
      TRUE, //   m_ucAllowOOBProvisioning;
  #else
      FALSE, //   m_ucAllowOverTheAirProvisioning;
      TRUE, //   m_ucAllowOOBProvisioning;
  #endif
      TRUE, //   m_ucAllowResetToFactoryDefaults;
      TRUE, //   m_ucAllowDefaultJoin;
      0, //  m_ucTargetJoinMethod // 0 = symmetric key, 1 = public key
      FILTER_BITMASK, //  m_unTargetNwkBitMask; // aligned to 4
      FILTER_TARGETID, //  m_unTargetNwkID; 
      { 0x00,0x49,0x00,0x53,0x00,0x41,0x00,0x20,0x00,0x31,0x00,0x30,0x00,0x30,0x00,0x00}, //   m_aJoinKey[16];        // aligned to 4
      {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}, //   m_aTargetSecurityMngrEUI[8];
      0xFFFF, // uint16  m_unTargetFreqList; //frequencies for advertisments which are to be used
      34, //   m_nCurrentUTCAdjustment // from 2009 ...
#if ( _UAP_TYPE == UAP_TYPE_SIMPLE_API )
      {   //   m_stDAQPubCommEndpoint 
          {0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4E, 0x7C, 0x0A, 0x10, 0x00, 0xDE},
          ISA100_START_PORTS + ISA100_GW_UAP,   //m_unRemoteTLPort;
          5,    //m_unRemoteObjID;  
          20,   //m_nPubPeriod; 
          50,   //m_ucIdealPhase;
          0,    //m_ucStaleDataLimit;
          0,    //m_ucPubAutoRetransmit;
          0,    //m_ucCfgStatus; //0=not configured; 1=configured;
      },
      {   //m_aAttrDescriptor
          {UAP_DATA_OBJ_TYPE, UAP_DATA_ANALOG1, 0, sizeof(int32)},
          {UAP_DATA_OBJ_TYPE, UAP_DATA_ANALOG2, 0, sizeof(int32)},
          {UAP_DATA_OBJ_TYPE, UAP_DATA_ANALOG3, 0, sizeof(int32)},
          {UAP_DATA_OBJ_TYPE, UAP_DATA_ANALOG4, 0, sizeof(int32)},
          {0, 0, 0, 0},
      },
      UAP_CO_VERSION,       
#endif      
      DEVICE_ROLE,
      0x0000
  };
#endif


#if ( PLATFORM == PLATFORM_GE_DEVICE )   
    uint8  g_ucFWUploadCompleted;
    uint8  g_aucFWUpgradeCutover[6];
    uint8  g_aucSWRevisionInfo[16];    
#endif // #if ( PLATFORM == PLATFORM_GE_DEVICE ) 

DPO_STRUCT  g_stDPO;

#ifndef BACKBONE_SUPPORT
  uint16 g_stFilterTargetID;
  uint16 g_stFilterBitMask;
#endif  

void DPO_ReadDLConfigInfo(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize);
void DPO_WriteDLConfigInfo(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);

void DPO_readAssocEndp(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize);
void DPO_writeAssocEndp(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);
void DPO_readAttrDesc(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize);
void DPO_writeAttrDesc(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);
void DPO_WriteDeviceRole(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);
void DPO_WriteUAPCORevision(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);

const uint16  c_unDefaultNetworkID      = 1; //id for the default network
#define   c_aDefaultSYMJoinKey c_aulWellKnownISAKey
#define   c_aOpenSYMJoinKey    c_aulWellKnownISAKey

const uint16  c_unDefaultFrequencyList  = 0xFFFF; //list of frequencies used by the adv. routers of the default network

// has to start with write dll config, has to end with write ntw id

#ifdef BACKBONE_SUPPORT
  #define DPO_writeUint8          NULL
  #define DPO_writeUint16         NULL
  #define DPO_writeVisibleString  NULL
#else
  void DPO_flashDPOInfo(void);
  void DPO_writeUint8(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize);
  void DPO_writeUint16(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize);
  void DPO_writeVisibleString(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize);
#endif

const DMAP_FCT_STRUCT c_aDPOFct[DPO_ATTR_NO] = {
   0,   0,                                              DMAP_EmptyReadFunc,     NULL,  
   ATTR_CONST(c_unDefaultNetworkID),                    DMAP_ReadUint16,        NULL,
   ATTR_CONST(c_aDefaultSYMJoinKey),                    DMAP_ReadVisibleString, NULL,
   ATTR_CONST(c_aOpenSYMJoinKey),                       DMAP_ReadVisibleString, NULL,
   ATTR_CONST(c_unDefaultFrequencyList),                DMAP_ReadUint16,        NULL,
   ATTR_CONST(g_stDPO.m_ucJoinMethodCapability),        DMAP_ReadUint8,         DPO_writeUint8,
   ATTR_CONST(g_stDPO.m_ucAllowProvisioning),           DMAP_ReadUint8,         DPO_writeUint8,
   ATTR_CONST(g_stDPO.m_ucAllowOverTheAirProvisioning), DMAP_ReadUint8,         DPO_writeUint8,
   ATTR_CONST(g_stDPO.m_ucAllowOOBProvisioning),        DMAP_ReadUint8,         DPO_writeUint8,
   ATTR_CONST(g_stDPO.m_ucAllowResetToFactoryDefaults), DMAP_ReadUint8,         NULL,
   ATTR_CONST(g_stDPO.m_ucAllowDefaultJoin),            DMAP_ReadUint8,         DPO_writeUint8,
   ATTR_CONST(g_stDPO.m_unTargetNwkID),                 DMAP_ReadUint16,        DPO_writeUint16,
   ATTR_CONST(g_stDPO.m_unTargetNwkBitMask),            DMAP_ReadUint16,        DPO_writeUint16,
   ATTR_CONST(g_stDPO.m_ucTargetJoinMethod),            DMAP_ReadUint8,         DPO_writeUint8,
   ATTR_CONST(g_stDPO.m_aTargetSecurityMngrEUI),        DMAP_ReadVisibleString, DPO_writeVisibleString,
   ATTR_CONST(g_stDMO.m_aucSysMng128BitAddr),           DMAP_ReadVisibleString, DPO_writeVisibleString,   
   ATTR_CONST(g_stDPO.m_unTargetFreqList),              DMAP_ReadUint16,        DPO_writeUint16,
   NULL, MAX_GENERIC_VAL_SIZE,                          DPO_ReadDLConfigInfo,   DPO_WriteDLConfigInfo,
   NULL, 0,                                             DMAP_EmptyReadFunc,     NULL,
   NULL, 0,                                             DMAP_EmptyReadFunc,     NULL,
   NULL, 0,                                             DMAP_EmptyReadFunc,     NULL,
   
#ifdef BACKBONE_SUPPORT  
   NULL, 0,                                             DMAP_EmptyReadFunc,     NULL,
   NULL, 0,                                             DMAP_EmptyReadFunc,     NULL,
   NULL, 0,                                             DMAP_EmptyReadFunc,     NULL,
   NULL, 0,                                             DMAP_EmptyReadFunc,     NULL,
   NULL, 0,                                             DMAP_EmptyReadFunc,     NULL,
#elif ( _UAP_TYPE == UAP_TYPE_SIMPLE_API )
   ATTR_CONST(g_stDPO.m_nCurrentUTCAdjustment),         DMAP_ReadUint16,        DMAP_WriteUint16,

   ATTR_CONST(g_stDPO.m_stDAQPubCommEndpoint),          CO_readAssocEndp,       DPO_writeAssocEndp,
   ATTR_CONST(g_stDPO.m_aAttrDescriptor),               DPO_readAttrDesc,       DPO_writeAttrDesc,
   ATTR_CONST(g_stDPO.m_unDeviceRole),                  DMAP_ReadUint16,        DPO_WriteDeviceRole,
   ATTR_CONST(g_stDPO.m_ucUAPCORevision),               DMAP_ReadUint8,         DPO_WriteUAPCORevision
#else    // UAP_TYPE_ISA100_API
   ATTR_CONST(g_stDPO.m_nCurrentUTCAdjustment),         DMAP_ReadUint16,        DMAP_WriteUint16,

   NULL, 0,                                             DMAP_EmptyReadFunc,     NULL,
   NULL, 0,                                             DMAP_EmptyReadFunc,     NULL,
   ATTR_CONST(g_stDPO.m_unDeviceRole),                  DMAP_ReadUint16,        DPO_WriteDeviceRole,
   NULL, 0,                                             DMAP_EmptyReadFunc,     NULL
#endif     
     
#if !defined(BACKBONE_SUPPORT) && ( PLATFORM == PLATFORM_GE_DEVICE )   
   ,ATTR_CONST(g_ucFWUploadCompleted),                   DMAP_ReadUint8,         DPO_writeUint8
   ,ATTR_CONST(g_aucFWUpgradeCutover),                   DMAP_ReadVisibleString, DPO_writeVisibleString
   ,ATTR_CONST(g_aucSWRevisionInfo),                     DMAP_ReadVisibleString, DPO_writeVisibleString
#endif // #if ( PLATFORM == PLATFORM_GE_DEVICE )      
};

//from the joining device point of view
#define ROUTER_JREQ_ADV_RX_BASE   1
#define ROUTER_JREQ_TX_OFF        3
#define ROUTER_JRESP_RX_OFF       5

#if defined( FAKE_ADVERTISING_ROUTER ) || defined( PROVISIONING_DEVICE )

  #define ADV_SUPERFRAME_UID        0x10
  
  const DLL_SMIB_ENTRY_SUPERFRAME  c_aDefSuperframe[] =
  {
      {ADV_SUPERFRAME_UID,   // uid
      10464,  
      4,      // m_unChIndex (channels 14, 9, 4) 
      25,     // m_unSfPeriod
      0x00,   // m_unSfBirth        
      0x7fff,   // m_unChMap        
      0,      // m_ucChBirth        
      0x1C,   // m_ucInfo
      0,      // m_lIdleTimer
      0,      // m_ucChRate
      0,      // m_ucRndSlots  
      0,      // m_ucChLen  
      0, 0, 0 // m_unCrtOffset, m_ucCrtChOffset, m_ucCrtSlowHopOffset
      }
  };
  
  const DLL_SMIB_ENTRY_LINK c_aDefLink[] =
  {
      {0x0020,
       ADV_SUPERFRAME_UID, // m_ucSuperframeUID
       (DLL_MASK_LNKTX | (0x01 << DLL_ROT_LNKDISC)), // m_mfLinkType // Tx + Adv. link
       0, // m_ucPriority
       DEFAULT_TX_TEMPL_UID, // m_ucTemplate1
       0x00, // m_ucTemplate2
       0x01 << DLL_ROT_LNKNEI, // m_mfTypeOptions // options: neighbor by address
       0, // m_ucChOffset
       BROADCAST_DEST_ADDRESS, // m_unNeighbor
       0, // m_mfGraphId
       {
         ROUTER_JREQ_ADV_RX_BASE, 0 // m_unOffset, m_unNA
       } // m_aSchedule
      },

      {0x0021,
       ADV_SUPERFRAME_UID, // m_ucSuperframeUID
       DLL_MASK_LNKRX, // m_mfLinkType //Rx link
       0, // m_ucPriority
       DEFAULT_RX_TEMPL_UID, // m_ucTemplate1
       0x00, // m_ucTemplate2
       0x02 << DLL_ROT_LNKNEI, // m_mfTypeOptions // options: group of neighbors, ChType=1
       0, // m_ucChOffset
       0, // m_unNeighbor
       0, // m_mfGraphId
       {
         ROUTER_JREQ_TX_OFF, 0 // m_unOffset, m_unNA
       } // m_aSchedule
      },

      {0x0022,
       ADV_SUPERFRAME_UID, // m_ucSuperframeUID
//       (DLL_MASK_LNKTX | DLL_MASK_LNKRX), // m_mfLinkType //Tx/Rx link
       (DLL_MASK_LNKTX | DLL_MASK_LNKRX | DLL_MASK_LNKJOIN), // m_mfLinkType //Tx/Rx link     
       0, // m_ucPriority
       DEFAULT_TX_TEMPL_UID, // m_ucTemplate1
       DEFAULT_RX_TEMPL_UID, // m_ucTemplate2
       0x02 << DLL_ROT_LNKNEI, // m_mfTypeOptions // options: group of neighbors, ChType=1
       0, // m_ucChOffset
       0, // m_unNeighbor
       0, // m_mfGraphId
       {
         ROUTER_JRESP_RX_OFF, 0 // m_unOffset, m_unNA
       } // m_aSchedule
      }
  };
  
#elif defined( PROVISIONED_SMIBS )
  
  #define DISCOVERY_HOP_SEQ_UID 0x8001
  
  const DLL_SMIB_ENTRY_SUPERFRAME  c_aJoinSuperframe = 
  {
    {
      JOIN_SF,   // uid
      10464,  // 10480*0,95367us=9.9945ms //m_unTimeslotsLength -must be a little shorter than 10ms!
      DISCOVERY_HOP_SEQ_UID,      // m_unChIndex  
      3000,     // m_unSfPeriod
      0x00,   // m_unSfBirth        
      0x7fff,   // m_unChMap        
      0,      // m_ucChBirth  
      0x00,   // m_ucInfo
      0,      // m_lIdleTimer                     
      250,    // m_ucChRate
      0,      // m_ucRndSlots  
      0,      // m_ucChLen
      0, 0, 0 // m_unCrtOffset, m_ucCrtChOffset, m_ucCrtSlowHopOffset
    }
  };  
  
  const DLL_SMIB_ENTRY_LINK c_stNwkDiscoveryLink =
  {
      // rx link along all superframe (range_
      {JOIN_ADV_RX_LINK_UID,
       JOIN_SF, // m_ucSuperframeUID
       DLL_MASK_LNKRX,                // m_mfLinkType Rx link
       0, // m_ucPriority
       DEFAULT_DISCOVERY_TEMPL_UID,   // m_ucTemplate1
       0,                             // m_ucTemplate2
       (uint8)( DLL_LNK_GROUP_MASK | (2 << DLL_ROT_LNKSC)),    // m_mfTypeOptions // options: group of neighbors, schedule typr = range 
       0, // m_ucChOffset
       0, // m_unNeighbor
       0, // m_mfGraphId
       {
         0, 2999 // range: first, last
       } // m_aSchedule
      }
  };
  
  const DLL_SMIB_ENTRY_CHANNEL c_aDiscoveryHopSeq =
  {
    { DISCOVERY_HOP_SEQ_UID,
      4, //range 1 to 16 (not 0 to 15!!!)
      0x58, 0xB3, 0, 0, 0, 0, 0, 0 //seq0 ... seq8
    }
  };
#else

  const DLL_SMIB_ENTRY_SUPERFRAME  c_aJoinSuperframe = 
  {
    {
      JOIN_SF,   // uid
      12000,  // 10480*0,95367us=9.9945ms //m_unTimeslotsLength -must be a little shorter than 10ms!
      3,      // m_unChIndex (channels 4, 9, 14)  
      50,     // m_unSfPeriod
      0x00,   // m_unSfBirth        
      0x7fff,   // m_unChMap        
      0,      // m_ucChBirth  
      0x00,   // m_ucInfo
      0,      // m_lIdleTimer                     
      0,      // m_ucChRate
      0,      // m_ucRndSlots  
      0,      // m_ucChLen
      0, 0, 0 // m_unCrtOffset, m_ucCrtChOffset, m_ucCrtSlowHopOffset
    }
  };  
  
  const DLL_SMIB_ENTRY_LINK c_stNwkDiscoveryLink =
  {
      // rx link along all superframe (range_
      {JOIN_ADV_RX_LINK_UID,
       JOIN_SF, // m_ucSuperframeUID
       DLL_MASK_LNKRX,                // m_mfLinkType Rx link
       0,                             // m_ucPriority
       DEFAULT_DISCOVERY_TEMPL_UID,   // m_ucTemplate1
       0,                             // m_ucTemplate2
       (uint8)( DLL_LNK_GROUP_MASK | (2 << DLL_ROT_LNKSC)),    // m_mfTypeOptions // options: group of neighbors, schedule typr = range 
       0, // m_ucChOffset
       0, // m_unNeighbor
       0, // m_mfGraphId
       {
         0, 49 // range: first, last
       } // m_aSchedule
      }
  };
  
#endif

#ifdef BACKBONE_SUPPORT

  const DLL_MIB_ADV_JOIN_INFO c_stDefJoinInfo =
  {
    0x24,     // Join timeout is 2^4 = 16 sec, Join backof is 2^2 = 4 times
    DLL_MASK_ADV_RX_FLAG,   //all join links type Offset only 
    ROUTER_JREQ_TX_OFF,
    0,
    ROUTER_JRESP_RX_OFF,
    0,
    ROUTER_JREQ_ADV_RX_BASE,
    0
  };

#endif

  
  typedef struct {
    uint16                m_unVersion;
    
    uint8                 m_aTagName[16];
    uint8                 m_aucVendorID[16];
    uint8                 m_aucModelID[16];
    uint8                 m_aucSerialNo[16];
    
    struct
    {
      struct
      {
            uint8   m_aNetworkAddr[16]; //IPv6 address of remote enpoint
            uint16  m_unTLPort; //transport layer port at remote endpoint
            uint16  m_unObjID; //object identifier at remote endpoint
      } m_stEndPoint;
      uint8    m_ucConfTimeout;    //timeout waiting for alert confirmation
      uint8    m_ucAlertsDisable;  //disable all alerts for this category
      uint8    m_ucRecoveryAlertsDisable; 
      uint8    m_ucRecoveryAlertsPriority; 
    }   m_aAlerts[6];
    
#if ( PLATFORM == PLATFORM_GE_DEVICE )      
    uint8  m_aucDPOFWUpgradeCutover[6];
    uint8  m_aucDPOWRevisionInfo[16];  
    uint8  m_ucDPOFWUploadCompleted;
#endif // #if ( PLATFORM == PLATFORM_GE_DEVICE )  
    
  } DPO_PERSISTENT;
  
  void DPO_SavePersistentData(void);
  void DPO_ExtractPersistentData(void);
  
 
#ifndef BACKBONE_SUPPORT  
    void DPO_setToDefault(void);
#endif    


#ifdef BACKBONE_SUPPORT  
  void DPO_Init(void)
  {
      // Necesary for initialization of JoinBackoff and JoinTimeout
      DLME_SetMIBRequest(DL_ADV_JOIN_INFO, &c_stDefJoinInfo);
      
      DPO_ExtractPersistentData();
            
      g_stDPO.m_unDeviceRole = DEVICE_ROLE;
      g_unDllSubnetId = g_stFilterTargetID;      
  }
  
  void DPO_ReadDLConfigInfo(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize)
  {
  }

  void DPO_WriteDLConfigInfo(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize)
  {    
  }    
  
  void DPO_ResetToDefault( void )
  {
  }
  
  uint8 DPO_WriteSYMJoinKey(uint16  p_unReqSize, 
                            uint8*  p_pReqBuf,
                            uint16* p_pRspSize,
                            uint8*  p_pRspBuf)
  {
    uint8 ucSFC = SFC_OBJECT_STATE_CONFLICT;
    *p_pRspSize = 0;
    return ucSFC;
  }
  

  __monitor uint8 DPO_Execute(uint8   p_ucMethID,
                     uint16  p_unReqSize, 
                     uint8*  p_pReqBuf,
                     uint16* p_pRspSize,
                     uint8*  p_pRspBuf)
  {
    uint8 ucSFC = SFC_OBJECT_STATE_CONFLICT;
    *p_pRspSize = 0;
    return ucSFC;
  }
      
#else // ! BACKBONE_SUPPORT

void DPO_writeUint8(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize)
{
  *(uint8*)(p_pValue) = *p_pBuf;
  DPO_flashDPOInfo();
}

void DPO_writeUint16(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize)
{
  *(uint16*)(p_pValue) = (uint16)(*(p_pBuf++)) << 8;
  *(uint16*)(p_pValue) |= (uint16)(*(p_pBuf++));
  DPO_flashDPOInfo();
}

void DPO_writeVisibleString(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize)
{
  memcpy(p_pValue, p_pBuf, p_ucSize);
  DPO_flashDPOInfo();
}

  
void DPO_flashDPOInfo(void)
{
    DPO_SavePersistentData();

    if( g_stDPO.m_ucStructSignature == DPO_SIGNATURE )
    {
  #if (SECTOR_FLAG)
      uint8 aDlConfigInfo[MAX_GENERIC_VAL_SIZE+1];
      
      ReadPersistentData( aDlConfigInfo, PROVISION_START_ADDR + 256, sizeof(aDlConfigInfo) );
      
      EraseSector( PROVISION_SECTOR_NO );
      
      WritePersistentData( aDlConfigInfo, PROVISION_START_ADDR + 256, sizeof(aDlConfigInfo) );
#endif
      
      WritePersistentData( (uint8*)&g_stDPO, PROVISION_START_ADDR, sizeof(g_stDPO) ); 
    }
}

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /// @author NIVIS LLC
  /// @brief  Initializes the device provisioning object
  /// @param  none
  /// @return none
  /// @remarks
  ///      Access level: user level
  ////////////////////////////////////////////////////////////////////////////////////////////////////      
  void DPO_Init(void)
  {
    uint8 aDlConfigInfo[MAX_GENERIC_VAL_SIZE+1];
    uint16 unDeviceRole = g_stDPO.m_unDeviceRole;
    
    ReadPersistentData( (uint8*)&g_stDPO, PROVISION_START_ADDR, sizeof(g_stDPO) );
    ReadPersistentData( aDlConfigInfo, PROVISION_START_ADDR + 256, sizeof(aDlConfigInfo) );
    
#ifdef RESET_TO_FACTORY
    //need to be called after the g_stDPO structure is populated - the allow provisioning flag will be checked 
    DPO_ResetToDefault(); 
    ReadPersistentData( (uint8*)&g_stDPO, PROVISION_START_ADDR, sizeof(g_stDPO) );
#endif
    
    uint8 ucDlConfigInfoSize = aDlConfigInfo[0];
    uint8 ucAttrNo = aDlConfigInfo[1];
    
    
    if( (DPO_SIGNATURE != g_stDPO.m_ucStructSignature) || (ucDlConfigInfoSize > MAX_GENERIC_VAL_SIZE) ) // unknown signature
    {  
        DPO_setToDefault();
    }
    else
    {
        ATTR_IDTF stAttrIdtf;
        
        uint8 * pDLLAttr = aDlConfigInfo+2;
        uint8 * pEndDLLAttr = aDlConfigInfo+1+ucDlConfigInfoSize;
        while( (ucAttrNo--) && (pDLLAttr < pEndDLLAttr) )
        {
          
          stAttrIdtf.m_unAttrID = *(pDLLAttr++);
          stAttrIdtf.m_unIndex1 = 0xFFFF;
          
          uint16 unAttrSize = pEndDLLAttr-pDLLAttr;
          if( DLMO_Write( &stAttrIdtf, &unAttrSize, pDLLAttr ) != SFC_SUCCESS )
          {            
              DPO_setToDefault();   // corrupted flash
              break;
          }
          DLME_ParseMsgQueue(); // avoid getting full of DLME action queue 
          
          pDLLAttr += unAttrSize;            
        }
    }
    
    if( !unDeviceRole ) // after restart
    {        
        unDeviceRole = g_stDPO.m_unDeviceRole;
        if( DEVICE_ROLE == 0x03 ) // routing and IO supported by FW
        {
            if( unDeviceRole != 1 && unDeviceRole != 2 )              
            {
                unDeviceRole = DEVICE_ROLE;
            }
        }
        else
        {
            unDeviceRole = DEVICE_ROLE;
        }
    }
    g_stDPO.m_unDeviceRole = unDeviceRole;

    MLSM_SetSlotLength(g_aDllSuperframesTable[0].m_stSuperframe.m_unTsDur);  
    
    g_stFilterBitMask  = g_stDPO.m_unTargetNwkBitMask;
    g_stFilterTargetID = g_stDPO.m_unTargetNwkID;
    
    DPO_ExtractPersistentData();
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /// @author NIVIS LLC
  /// @brief  Reads data link layer related configuration information
  /// @param  p_pValue - input buffer
  /// @param  p_pBuf - buffer to receive data
  /// @param  p_ucSize - data size
  /// @return none
  /// @remarks
  ///      Access level: user level
  ////////////////////////////////////////////////////////////////////////////////////////////////////  
  void DPO_ReadDLConfigInfo(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize)
  {
      ReadPersistentData( p_pBuf, PROVISION_START_ADDR + 256, 1 );
      if( p_pBuf[0] < MAX_GENERIC_VAL_SIZE )
      {
          *p_ucSize = p_pBuf[0];
          ReadPersistentData( p_pBuf, PROVISION_START_ADDR + 256, p_pBuf[0] );
      }
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /// @author NIVIS LLC
  /// @brief  Writes data link layer related configuration information
  /// @param  p_pValue - data buffer 
  /// @param  p_pBuf - input data
  /// @param  p_ucSize - data size
  /// @return none
  /// @remarks
  ///      Access level: user level
  ////////////////////////////////////////////////////////////////////////////////////////////////////    
  void DPO_WriteDLConfigInfo(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize)
  {    
      uint8 aDlConfigInfo[MAX_GENERIC_VAL_SIZE+1];
            
      aDlConfigInfo[0] = p_ucSize;
      memcpy( aDlConfigInfo+1, p_pBuf, p_ucSize );
      
  #if (SECTOR_FLAG)
      EraseSector( PROVISION_SECTOR_NO );
      WritePersistentData( (uint8*)&g_stDPO, PROVISION_START_ADDR, sizeof(g_stDPO) );
  #endif
      
      WritePersistentData( aDlConfigInfo, PROVISION_START_ADDR + 256, 1+p_ucSize );      
  }
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /// @author NIVIS LLC
  /// @brief  Resets provisioning 
  /// @param  none
  /// @return none
  /// @remarks
  ///      Access level: user level
  ////////////////////////////////////////////////////////////////////////////////////////////////////  
  void DPO_ResetToDefault(void)
  {   
    //reset the default settings for the provisioning
    EraseSector(PROVISION_SECTOR_NO);
#if (SECTOR_FLAG == 0) // eeprom
    static const uint8 c_aFF = 0xFF;  
    WritePersistentData( &c_aFF, PROVISION_START_ADDR, 1 );  
#endif    
  }
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /// @author NIVIS LLC
  /// @brief  Write a symmetric AES join key to the device
  /// @param  p_unReqSize - key data size
  /// @param  p_pReqBuf - buffer containing key data
  /// @param  p_unRspSize - response size
  /// @param  p_pRspBuf - buffer containing response 
  /// @return service feedback code
  /// @remarks
  ///      Access level: user level
  ////////////////////////////////////////////////////////////////////////////////////////////////////    
  uint8 DPO_WriteSYMJoinKey(uint16  p_unReqSize, 
                            uint8*  p_pReqBuf,
                            uint16* p_pRspSize,
                            uint8*  p_pRspBuf)
  {
   //Table 414 – Write symmetric join key method
  //This method is used to write a symmetric AES join key to a device. This method is
  //evoked by the DPSO to provision a DBP with the Target AES join key. Depending
  //on the provisioning method used the join key may be encrypted by the session key
  //or the device’s public key. This method shall be executed only when
  //Allow_Provisioning is enabled  
    
    //arguments uint128 key value, uint8 encripted by)
    if(17 != p_unReqSize) 
      return SFC_INVALID_SIZE;
    
    uint8 aKeyValue[16]; 
    memcpy(aKeyValue, p_pReqBuf, 16);
      
  #warning "TODO: Decrypt join key" 
      
    memcpy( g_stDPO.m_aJoinKey, aKeyValue, 16 );
      
    *p_pRspSize = 1;
    *p_pRspBuf = 0; //success
    
    return SFC_SUCCESS;
  }
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /// @author NIVIS LLC, Mihaela Goloman
  /// @brief  Executes a DPO method
  /// @param  p_unMethID - method identifier
  /// @param  p_unReqSize - input parameters size
  /// @param  p_pReqBuf - input parameters buffer
  /// @param  p_pRspSize - output parameters size
  /// @param  p_pRspBuf - output parameters buffer
  /// @return service feedback code
  /// @remarks
  ///      Access level: user level
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  __monitor uint8 DPO_Execute(uint8   p_ucMethID,
                     uint16  p_unReqSize, 
                     uint8*  p_pReqBuf,
                     uint16* p_pRspSize,
                     uint8*  p_pRspBuf )
  {
    uint8 ucSFC = SFC_INVALID_METHOD;
    *p_pRspSize = 0;
    
    if( !DPO_CkAccess() )
    {
        return SFC_OBJECT_ACCESS_DENIED;
    }
    
    MONITOR_ENTER();
    switch(p_ucMethID)
    {
        case DPO_RESET_TO_DEFAULT: 
            *p_pRspSize = 1;                             
            *p_pRspBuf  = 0; //success
            ucSFC = SFC_SUCCESS;
            g_stDMO.m_ucJoinCommand = DMO_JOIN_CMD_RESET_FACTORY_DEFAULT;
            break;

        case DPO_WRITE_SYMMETRIC_JOIN_KEY:   
            ucSFC = DPO_WriteSYMJoinKey(p_unReqSize, p_pReqBuf, p_pRspSize, p_pRspBuf); 
            break;
    }  
    MONITOR_EXIT();
    return ucSFC;
  }
  
  /////////////////////////////////////////////////////////////////////////////////////////
#if ( _UAP_TYPE == UAP_TYPE_SIMPLE_API )        
  void DPO_writeAssocEndp(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize)
  {
      if( 26 != p_ucSize )
      {
          memset(&g_stDPO.m_stDAQPubCommEndpoint, 0, sizeof(g_stDPO.m_stDAQPubCommEndpoint));
      }
      else
      {

        ATTR_IDTF stAttrIdtf;
        stAttrIdtf.m_ucAttrFormat = ATTR_NO_INDEX;
        stAttrIdtf.m_unAttrID = CO_COMMENDP;
        
        //TODO - call just if the device not in provisioning process
        //if already the UAP concentrator configured the old contract will be terminate
        SetGenericAttribute(UAP_APP1_ID,
                            UAP_CO_OBJ_ID,
                            &stAttrIdtf,
                            p_pBuf, 
                            p_ucSize);
        
        g_ulDAQReadInterval = 0;
        
        //parse the AssocEndpoint
        memcpy(g_stDPO.m_stDAQPubCommEndpoint.m_aRemoteAddr128, p_pBuf, 16);
        p_pBuf += 16;

        p_pBuf = DMAP_ExtractUint16(p_pBuf, &g_stDPO.m_stDAQPubCommEndpoint.m_unRemoteTLPort);
        p_pBuf = DMAP_ExtractUint16(p_pBuf, &g_stDPO.m_stDAQPubCommEndpoint.m_unRemoteObjID);

        g_stDPO.m_stDAQPubCommEndpoint.m_ucStaleDataLimit = *(p_pBuf++);

        p_pBuf = DMAP_ExtractUint16(p_pBuf, (uint16*)&g_stDPO.m_stDAQPubCommEndpoint.m_nPubPeriod);

        g_stDPO.m_stDAQPubCommEndpoint.m_ucIdealPhase         = *(p_pBuf++);
        g_stDPO.m_stDAQPubCommEndpoint.m_ucPubAutoRetransmit  = *(p_pBuf++);
        g_stDPO.m_stDAQPubCommEndpoint.m_ucCfgStatus          = *(p_pBuf++);
      }
      
      DPO_flashDPOInfo();
  }
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /// @author NIVIS LLC, Mircea Vlasin
  /// @brief Reads publish items information (object, attribute and value size)
  /// @param p_pValue - address of publish items information
  /// @param p_pBuf - buffer to gather publish items
  /// @param p_ucSize - publish items information size
  /// @return none
  /// @remarks
  ///      Access level: user level
  ////////////////////////////////////////////////////////////////////////////////////////////////////  
  void DPO_readAttrDesc(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize)
  {
      OBJ_ATTR_IDX_AND_SIZE * pstNext = g_stDPO.m_aAttrDescriptor;
      OBJ_ATTR_IDX_AND_SIZE * pstEnd  = g_stDPO.m_aAttrDescriptor + MAX_PUBLISH_ITEMS;
      
      uint8* pStart = p_pBuf;
      
      while (pstNext < pstEnd && pstNext->m_unSize )
      {
          p_pBuf = DMAP_InsertUint16( p_pBuf, pstNext->m_unObjID);
          p_pBuf = DMAP_InsertUint16( p_pBuf, pstNext->m_unAttrID);
          p_pBuf = DMAP_InsertUint16( p_pBuf, pstNext->m_unAttrIdx);
          p_pBuf = DMAP_InsertUint16( p_pBuf, pstNext->m_unSize);
          
          pstNext++;
      }
      *p_ucSize = p_pBuf - pStart;      
  }
  
  
  void DPO_writeAttrDesc(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize)
  {      
      ATTR_IDTF stAttrIdtf;
      stAttrIdtf.m_ucAttrFormat = ATTR_NO_INDEX;
      stAttrIdtf.m_unAttrID = CO_PUBDATA;
        
      //TODO - call just if the device not in provisioning process
      //if already the UAP concentrator configured the old contract will be terminate
      uint8 ucStatus = SetGenericAttribute(UAP_APP1_ID,
                                           UAP_CO_OBJ_ID,
                                           &stAttrIdtf,
                                           p_pBuf, 
                                           p_ucSize);

      //parse the Attribute descriptors      
      memset( g_stDPO.m_aAttrDescriptor, 0, sizeof( g_stDPO.m_aAttrDescriptor ) );
            
      unsigned int unIdx = 0;
      for( ; (unIdx < MAX_PUBLISH_ITEMS) && (p_ucSize >= 8) ; unIdx++ )
      {
        p_pBuf = DMAP_ExtractUint16( p_pBuf, &g_stDPO.m_aAttrDescriptor[unIdx].m_unObjID );
        p_pBuf = DMAP_ExtractUint16( p_pBuf, &g_stDPO.m_aAttrDescriptor[unIdx].m_unAttrID );
        p_pBuf = DMAP_ExtractUint16( p_pBuf, &g_stDPO.m_aAttrDescriptor[unIdx].m_unAttrIdx );
        p_pBuf = DMAP_ExtractUint16( p_pBuf, &g_stDPO.m_aAttrDescriptor[unIdx].m_unSize );

        p_ucSize -= 8;
      }

      DPO_flashDPOInfo();    
  }
#endif

  
  ///////////////////////////////////////////////////////////////////////////////////////
  void DPO_WriteDeviceRole(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize)
  {
      if( 2 != p_ucSize )
          return;
      
      DMAP_ExtractUint16(p_pBuf, (uint16*)&g_stDPO.m_unDeviceRole); 
      DPO_flashDPOInfo();
  }
  
  
  ///////////////////////////////////////////////////////////////////////////////////////
#if ( _UAP_TYPE == UAP_TYPE_SIMPLE_API )
  void DPO_WriteUAPCORevision(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize)
  {
      if( 1 != p_ucSize )
          return;
      
      g_stDPO.m_ucUAPCORevision = p_pBuf[0];
  
      DPO_flashDPOInfo();
      
      //check if the associated concentrator already running and need to be updated
      g_pstCrtCO = FindConcentratorByObjId(UAP_CO_OBJ_ID, UAP_APP1_ID);
      if( g_pstCrtCO )
      {
         g_pstCrtCO->m_ucRevision = p_pBuf[0];                
      }
  }
#endif
  
   
#pragma diag_suppress=Pa039                     
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /// @author NIVIS LLC, Mihaela Goloman
  /// @brief  Sets DPO to default
  /// @param  none
  /// @return none
  /// @remarks
  ///      Access level: user level
  ////////////////////////////////////////////////////////////////////////////////////////////////////    
  void DPO_setToDefault(void)
  {
      uint16 unVersionNo = 0;
      memcpy(&g_stDPO, &c_stDefaultDPO, sizeof(g_stDPO));
      ReadPersistentData( (uint8*)&unVersionNo, MANUFACTURING_START_ADDR, 2 );
      if( unVersionNo )
      {
          ReadPersistentData( g_stDPO.m_aJoinKey,  
                             (unVersionNo & 0xFEFE ? MANUFACTURING_START_ADDR+12+16 : MANUFACTURING_START_ADDR+12+2) , 
                             sizeof(g_stDPO.m_aJoinKey) + sizeof(g_stDPO.m_aTargetSecurityMngrEUI) );          
      }
      
  #if defined( FAKE_ADVERTISING_ROUTER ) || defined( PROVISIONING_DEVICE )
  
      g_unDllAdvSuperframe = ADV_SUPERFRAME_UID;
      
      DLMO_DOUBLE_IDX_SMIB_ENTRY stTwoIdxSMIB;
      
      stTwoIdxSMIB.m_unUID = *(uint16*)c_aDefSuperframe;
      memcpy(&stTwoIdxSMIB.m_stEntry, c_aDefSuperframe, sizeof(DLL_SMIB_ENTRY_SUPERFRAME));      
      DLME_SetSMIBRequest(DL_SUPERFRAME, 0, &stTwoIdxSMIB);   
      DLME_ParseMsgQueue();   // avoid getting full of DLME action queue 
  
      memcpy(&g_stDAUXSuperframe, &c_aDefSuperframe[0].m_stSuperframe, sizeof(g_stDAUXSuperframe));
      
      stTwoIdxSMIB.m_unUID = *(uint16*)c_aDefLink;
      memcpy(&stTwoIdxSMIB.m_stEntry, c_aDefLink, sizeof(DLL_SMIB_ENTRY_LINK));      
      DLME_SetSMIBRequest(DL_LINK, 0, &stTwoIdxSMIB);
      
      stTwoIdxSMIB.m_unUID = *(uint16*)(c_aDefLink+1);
      memcpy(&stTwoIdxSMIB.m_stEntry, c_aDefLink+1, sizeof(DLL_SMIB_ENTRY_LINK));  
      DLME_SetSMIBRequest(DL_LINK, 0, &stTwoIdxSMIB);
      
      stTwoIdxSMIB.m_unUID = *(uint16*)(c_aDefLink+2);
      memcpy(&stTwoIdxSMIB.m_stEntry, c_aDefLink+2, sizeof(DLL_SMIB_ENTRY_LINK));  
      DLME_SetSMIBRequest(DL_LINK, 0, &stTwoIdxSMIB);
      
      DLME_ParseMsgQueue(); // avoid getting full of DLME action queue 
          
      g_unDllSubnetId = FILTER_TARGETID;
      g_stDMO.m_unShortAddr = (uint16)c_oEUI64LE[0] << 8 | c_oEUI64LE[1];
      
      g_stDllAdvJoinInfo.m_mfDllJoinBackTimeout = 0x24;   // Join timeout is 2^4 = 16 sec, Join backof is 2^2 = 4 times
      g_stDllAdvJoinInfo.m_mfDllJoinLinksType = DLL_MASK_ADV_RX_FLAG,   //all join links type Offset only
      g_stDllAdvJoinInfo.m_stDllJoinTx.m_aOffset.m_unOffset = ROUTER_JREQ_TX_OFF;
      g_stDllAdvJoinInfo.m_stDllJoinRx.m_aOffset.m_unOffset = ROUTER_JRESP_RX_OFF;
      g_stDllAdvJoinInfo.m_stDllJoinAdvRx.m_aOffset.m_unOffset = ROUTER_JREQ_ADV_RX_BASE;
        
      g_ucJoinStatus = DEVICE_JOINED;
      MLDE_SetDiscoveryState(DEVICE_JOINED);    
    
  #else // defined( PROVISIONED_SMIBS ) 
      DLMO_DOUBLE_IDX_SMIB_ENTRY stTwoIdxSMIB;
      
      //set also the SecManagerEUI64 and AppJoinKey
      #if defined( PROVISIONED_SMIBS ) 
          
          stTwoIdxSMIB.m_unUID = c_aDiscoveryHopSeq.m_stChannel.m_unUID;
          memcpy(&stTwoIdxSMIB.m_stEntry, &c_aDiscoveryHopSeq.m_stChannel, sizeof(c_aDiscoveryHopSeq.m_stChannel)); 
          DLME_SetSMIBRequest( DL_CH, 0, &stTwoIdxSMIB );
          
          static const uint8 c_aucJoinKey[16] = { 0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF};
          static const uint8 c_aucSecEUI64[8] = { 0x00,0x00,0x00,0x00,0x0a,0x10,0x00,0xA0};
          
          memcpy(g_stDPO.m_aJoinKey, c_aucJoinKey, sizeof(g_stDPO.m_aJoinKey) ); 
          memcpy(g_stDPO.m_aTargetSecurityMngrEUI, c_aucSecEUI64, sizeof(g_stDPO.m_aTargetSecurityMngrEUI) );
          
          g_stDPO.m_unTargetNwkID = 34;          
      #endif

      stTwoIdxSMIB.m_unUID = c_aJoinSuperframe.m_stSuperframe.m_unUID;
      memcpy(&stTwoIdxSMIB.m_stEntry, &c_aJoinSuperframe.m_stSuperframe, sizeof(c_aJoinSuperframe.m_stSuperframe)); 
      DLME_SetSMIBRequest( DL_SUPERFRAME, 0, &stTwoIdxSMIB );
  
      stTwoIdxSMIB.m_unUID = c_stNwkDiscoveryLink.m_stLink.m_unUID;
      memcpy(&stTwoIdxSMIB.m_stEntry, &c_stNwkDiscoveryLink.m_stLink, sizeof(c_stNwkDiscoveryLink.m_stLink)); 
      DLME_SetSMIBRequest( DL_LINK, 0, &stTwoIdxSMIB );          
          
      DLME_ParseMsgQueue(); // avoid getting full of DLME action queue 
        
  #endif // FAKE_ADVERTISING_ROUTER 
  }   
#pragma diag_default=Pa039
#endif  // ! BACKBONE_SUPPORT
  
void DPO_SavePersistentData(void)
{
    DPO_PERSISTENT stCrt;
    DPO_PERSISTENT stOld;
    
    memset( &stCrt, 0, sizeof(stCrt) );
    
    stCrt.m_unVersion = 1;
    memcpy( stCrt.m_aTagName, g_stDMO.m_aucTagName, sizeof(stCrt.m_aTagName) );
    memcpy( stCrt.m_aucVendorID, g_stDMO.m_aucVendorID, sizeof(stCrt.m_aucVendorID) );
    memcpy( stCrt.m_aucModelID, g_stDMO.m_aucModelID, sizeof(stCrt.m_aucModelID) );
    memcpy( stCrt.m_aucSerialNo, g_stDMO.m_aucSerialNo, sizeof(stCrt.m_aucSerialNo) );
    
    for(unsigned  int i=0; i<4; i++ )
    {
        memcpy( &stCrt.m_aAlerts[i].m_stEndPoint, &g_aARMO[i].m_stAlertMaster, sizeof(stCrt.m_aAlerts[i].m_stEndPoint) );
        stCrt.m_aAlerts[i].m_ucConfTimeout = g_aARMO[i].m_ucConfTimeout;
        stCrt.m_aAlerts[i].m_ucAlertsDisable = g_aARMO[i].m_ucAlertsDisable;
        stCrt.m_aAlerts[i].m_ucRecoveryAlertsDisable = g_aARMO[i].m_stRecoveryDescr.m_bAlertReportDisabled;
        stCrt.m_aAlerts[i].m_ucRecoveryAlertsPriority = g_aARMO[i].m_stRecoveryDescr.m_ucPriority;
    }    

    ReadPersistentData( (uint8*)&stOld, APPLICATION_START_ADDR, sizeof(stOld) );
    
    if( memcmp( &stCrt, &stOld, sizeof(stOld) ) ) // changed, saved it
    {
    #if (SECTOR_FLAG)
        EraseSector( APPLICATION_SECTOR_NO );
    #endif
          
        WritePersistentData( (uint8*)&stCrt, APPLICATION_START_ADDR, sizeof(stCrt) ); 
    }
}

void asciiChar( uint8 p_ucHex, uint8 * p_pStr )
{
    uint8 ucHex = p_ucHex >> 4;
    
    p_pStr[0] = (ucHex < 10 ?  '0' + ucHex : 'A' - 10 + ucHex);
    
    ucHex = p_ucHex & 0x0F;
    p_pStr[1] = (ucHex < 10 ?  '0' + ucHex : 'A' - 10 + ucHex);
}

void DPO_ExtractPersistentData(void)
{
    DPO_PERSISTENT stCrt;
    
    memset(&g_aARMO, 0, sizeof(g_aARMO));

    ReadPersistentData( (uint8*)&stCrt, APPLICATION_START_ADDR, sizeof(stCrt) );
    if( stCrt.m_unVersion == 1 )
    {
        memcpy( g_stDMO.m_aucTagName, stCrt.m_aTagName, sizeof(g_stDMO.m_aucTagName) );
        memcpy( g_stDMO.m_aucVendorID, stCrt.m_aucVendorID, sizeof(g_stDMO.m_aucVendorID) );
        memcpy( g_stDMO.m_aucModelID, stCrt.m_aucModelID, sizeof(g_stDMO.m_aucModelID) );
        memcpy( g_stDMO.m_aucSerialNo, stCrt.m_aucSerialNo, sizeof(g_stDMO.m_aucSerialNo) );
        
        for(unsigned  int i=0; i<4; i++ )
        {
            memcpy( &g_aARMO[i].m_stAlertMaster, &stCrt.m_aAlerts[i].m_stEndPoint, sizeof(g_aARMO[i].m_stAlertMaster) );
            g_aARMO[i].m_ucConfTimeout = stCrt.m_aAlerts[i].m_ucConfTimeout;
            g_aARMO[i].m_ucAlertsDisable = stCrt.m_aAlerts[i].m_ucAlertsDisable;
            g_aARMO[i].m_stRecoveryDescr.m_bAlertReportDisabled = stCrt.m_aAlerts[i].m_ucRecoveryAlertsDisable;
            g_aARMO[i].m_stRecoveryDescr.m_ucPriority = stCrt.m_aAlerts[i].m_ucRecoveryAlertsPriority;
        }
    }
    else
    {
      //an update event will save the hardcoded values as persistent
      for(unsigned  int i=0; i<8; i++ )
      {
          asciiChar(c_oEUI64BE[i], g_stDMO.m_aucTagName+i*2);          
      }

      g_stDMO.m_aucTagName[0] = 'T'; // not standard but want to diferentiate between TAG ans EUI64     
      
      memcpy( g_stDMO.m_aucVendorID, "NIVIS           ", sizeof(g_stDMO.m_aucVendorID) );
      memcpy( g_stDMO.m_aucModelID, "FREESCALE_VN210 ", sizeof(g_stDMO.m_aucModelID) );
      memcpy( g_stDMO.m_aucSerialNo, g_stDMO.m_aucTagName, sizeof(g_stDMO.m_aucSerialNo) ); 
      g_stDMO.m_aucSerialNo[0] = 'S';      
    }
    
#if ( PLATFORM == PLATFORM_GE_DEVICE )  
    
    g_ucFWUploadCompleted = stCrt.m_ucDPOFWUploadCompleted;
    memcpy( g_aucFWUpgradeCutover, stCrt.m_aucDPOFWUpgradeCutover, sizeof(g_aucFWUpgradeCutover) );
    memcpy( g_aucSWRevisionInfo, stCrt.m_aucDPOWRevisionInfo, sizeof(g_aucSWRevisionInfo) );        
    
#endif // ( PLATFORM == PLATFORM_GE_DEVICE )      
}
  

uint8 DPO_CkAccess(void)
{
    if( !g_stDPO.m_ucAllowProvisioning )
        return 0;
                    
//    if( !g_ucIsOOBAccess ) // is over the air
//        return g_stDPO.m_ucAllowOverTheAirProvisioning;

    return g_stDPO.m_ucAllowOOBProvisioning;
 }
