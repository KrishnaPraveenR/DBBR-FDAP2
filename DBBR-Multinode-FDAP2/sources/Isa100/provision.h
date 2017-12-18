////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Eduard Erdei
/// Date:         February 2008
/// Description:  This file holds the provisioning data
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NIVIS_PROVISION_H_
#define _NIVIS_PROVISION_H_

#include "config.h"
#include "dlme.h"
#include "../typedef.h"

#define JOIN_BACKOFF_BASE     5   // 500 msec

#define VENDOR_ID_SIZE  5
#define MODEL_ID_SIZE   9
#define TAG_NAME_SIZE   4
#define PWR_SUPPLY_SIZE 3
#define MEM_INFO_SIZE   6

#define CHIPCON_DEFAULT_PA_LEVEL    0x07    //(~-15 dBm according to the datasheet) + External PA gain

#define DEFAULT_TS_TEMPLATE_NO  3
#define DEFAULT_CH_NO           5


#if( DEVICE_TYPE == DEV_TYPE_MC13225 )
  #include "../spif_interface.h"    

  typedef uint32 PROV_ADDR_TYPE;

  #define SECTOR_FLAG  1
  #define SECTOR_SIZE (4*1024)

  // persistent data sectors (sector 31 is reserved for factory, 0 for bootloader)
  #define MANUFACTURING_SECTOR_IDX  30 
  #define PROVISION_SECTOR_IDX      29
  #define APPLICATION_SECTOR_IDX    28 

  #define MANUFACTURING_SECTOR_NO   (1UL << MANUFACTURING_SECTOR_IDX)  
  #define PROVISION_SECTOR_NO   (1UL << PROVISION_SECTOR_IDX)  
  #define APPLICATION_SECTOR_NO   (1UL << APPLICATION_SECTOR_IDX)  

  #define MANUFACTURING_START_ADDR   (SECTOR_SIZE * MANUFACTURING_SECTOR_IDX)
  #define PROVISION_START_ADDR       (SECTOR_SIZE * PROVISION_SECTOR_IDX)
  #define APPLICATION_START_ADDR     (SECTOR_SIZE * APPLICATION_SECTOR_IDX)

#elif       (DEVICE_TYPE == DEV_TYPE_MSP430F2618) // raptor modem
  #include "../SPI1_eeprom.h"  

  typedef uint16 PROV_ADDR_TYPE;
        
  #define SECTOR_FLAG  0

  #define MANUFACTURING_START_ADDR     (0*128)  // -> page 0  in eeprom 
  #define PROVISION_START_ADDR         (32*128) // -> page 32 in eeprom 
  #define APPLICATION_START_ADDR       (64*128) // -> page 64 in eeprom 

#elif       (DEVICE_TYPE == DEV_TYPE_AP91SAM7X512) 
  #include "../SPI_eeprom.h"
  #include "../i2c.h"
  #include "../logger/err.h"

  typedef uint16 PROV_ADDR_TYPE;
  
  #define SECTOR_FLAG  0
  
  #define MANUFACTURING_START_ADDR     (0*128)  // -> page 0  in eeprom 
  #define PROVISION_START_ADDR         (32*128) // -> page 32 in eeprom 
  #define APPLICATION_START_ADDR       (64*128) // -> page 64 in eeprom 
  
#endif

#include "dmap_dpo.h"

typedef struct 
{
    //contract structure
    uint8 m_ucContractCriticality;
    uint8 m_aPayloadSize[2];
    
    //capabilities structure
    uint8 m_aucEUI64[8];
    uint8 m_aDLLSubnetID[2]; 
    uint8 m_ucVendorIdLen;
    uint8 m_aVendorID[VENDOR_ID_SIZE];
    uint8 m_ucDeviceType;
    uint8 m_ucImplementationType;
    uint8 m_ucModelIdLen;
    uint8 m_aModelID[MODEL_ID_SIZE];
    uint8 m_ucTagNameLen;
    uint8 m_aTagName[TAG_NAME_SIZE];
    uint8 m_aSerialNumber[4];
    uint8 m_ucPwrSupplyStatusLen;
    uint8 m_aPwrSupplyStatus[PWR_SUPPLY_SIZE];
    uint8 m_ucMemInfoLen;
    uint8 m_aMemInfo[MEM_INFO_SIZE];
    uint8 m_ucDeviceClockPrecision;
    uint8 m_ucDeviceClockAccuracy;
    uint8 m_ucCommSwMajorVersion;
    uint8 m_ucCommSwMinorVersion;
    uint8 m_ucSoftwareRevisionNmber;
} DEVICE_INFO;


extern EUI64_ADDR   c_oEUI64BE; // nwk order EUI64ADDR
extern EUI64_ADDR   c_oEUI64LE; // little endian order 

#define   c_oSecManagerEUI64BE g_stDPO.m_aTargetSecurityMngrEUI
  
extern const DLL_SMIB_ENTRY_TIMESLOT    c_aDefTemplates[];
extern const DLL_SMIB_ENTRY_CHANNEL     c_aDefChannels[];
extern const DLL_MIB_DEV_CAPABILITIES   c_stCapability;
extern const DLL_MIB_ENERGY_DESIGN      c_stEnergyDesign; 
  
 
#ifdef BACKBONE_SUPPORT
  #define DLL_DL_ROLE   0x04
#elif ROUTING_SUPPORT
  #define DLL_DL_ROLE   0x02
#else
  #define DLL_DL_ROLE   0x01
#endif  

#if defined(at91sam7x512)

  void PROVISION_Init(void); 

  #define c_ulIP      (*(uint32*)(g_stDMO.m_auc128BitAddr+12))
  extern uint16 c_unPort; 
//  extern uint32 c_ulMASK;   //
//  extern uint32 c_ulGWY;    //
//  extern uint32 c_ulIP4LOG;       //
//  extern uint16 c_usPort4LOG;     //
//  extern uint8 c_ucVerboseLOG[];    //
//#ifdef WCI_SUPPORT
//  extern uint32 c_ulIP4LOGAck;       //
//  extern uint16 c_usPort4LOGAck;     //
//  extern uint8 c_ucVerboseLOGAck[];    //
//#endif  // WCI_SUPPORT

    #define c_ulMASK        g_stProvisioning.m_ulIPv4BBRMask   
    #define c_ulGWY         g_stProvisioning.m_ulIPv4GWY  
    #define c_ulIP4LOG      g_stProvisioning.m_ulIP4LOG       
    #define c_usPort4LOG    g_stProvisioning.m_unPort4LOG     
    #define c_ucVerboseLOG  g_stProvisioning.m_aucVerboseLOG    
#ifdef WCI_SUPPORT
    #define c_ulIP4LOGAck       g_stProvisioning.m_ulIP4LOGAck       
    #define c_usPort4LOGAck     g_stProvisioning.m_unPort4LOGAck        
    #define c_ucVerboseLOGAck   g_stProvisioning.m_aucVerboseLOGAck    
#endif  // WCI_SUPPORT
     
  
  #define g_aIPv4Address ((uint8*)&c_ulIP)
#endif
  
  void PROVISION_Init(void);
  
#ifdef FAKE_ADVERTISING_ROUTER
  
  void  PROVISION_InitFakeAdvertiseRouter(void);
  
#endif // FAKE_ADVERTISING_ROUTER
 
uint8 PROVISION_AddCmdToProvQueue( uint16  p_unObjId,
                                   uint8   p_ucAttrId,
                                   uint8   p_ucSize,
                                   uint8*  p_pucPayload );
void PROVISION_ValidateProvQueue( void );
void PROVISION_ExecuteProvCmdQueue( void );   

#if( DEVICE_TYPE == DEV_TYPE_MC13225 ) && defined (BACKBONE_SUPPORT)
    extern uint8 g_ucProvisioned;
#else
    #define g_ucProvisioned 1
#endif
        
void ReadPersistentData( uint8 *p_pucDst, PROV_ADDR_TYPE p_uAddr, uint16 p_unSize );
void WritePersistentData( const uint8 *p_pucSrc, PROV_ADDR_TYPE p_uAddr, uint16 p_unSize );

#if( DEVICE_TYPE == DEV_TYPE_MC13225 )
  void EraseSector( uint32 p_ulSectorNmb );
#else // EEPROM -> not need to erase the sector first
  #define EraseSector( ...)
#endif 
  
#if defined(at91sam7x512)

enum
{
  PROV_FORMAT_VER                   = 1,
  PROV_SUBNET_ID                    = 2,
  PROV_FILTER_BITMASK               = 3,
  PROV_FILTER_TARGET_ID             = 4,
  PROV_APP_JOIN_KEY                 = 5,
  PROV_DLL_KEY                      = 6,
  PROV_PROVISION_KEY                = 7,
  PROV_SEC_MNGR_EUI64               = 8,
  PROV_SM_IPV6                      = 9,
  PROV_BBR_IPV6                     = 10,
  PROV_CRT_UTC_DRIFT                = 11,
  PROV_NEXT_TIME_UTC                = 12,
  PROV_NEXT_UTC_DRIFT               = 13,
  PROV_BBR_IPV4_MASK                = 14,
  PROV_GW_IPV4                      = 15,
  PROV_LOG_IPV4                     = 16,
  PROV_LOG_PORT                     = 17,
  PROV_VERBOSE_LOG                  = 18,

#ifdef WCI_SUPPORT 
  PROV_ACK_LOG_IPV4                 = 19,
  PROV_ACK_LOG_PORT                 = 20,
  PROV_VERBOSE_ACK_LOG              = 21,
#endif

  PROV_DLL_QUEUE_ORDER              = 22,
  PROV_ACK_CONFIG                   = 23,
  PROV_LOGGING_LEVEL                = 24,
  PROV_CLOCK_DRIFT                  = 25,
  PROV_WRONG_DAUX_COUNT             = 26,
  PROV_WRONG_UDP_CS_COUNT           = 27,
  PROV_WRONG_ACK_MIC_COUNT          = 28,
  PROV_START_TAI_SEC                = 29,
  
  MANUF_EUI64_ADDR                  = 30,
  PROV_PA_LEVEL                     = 31, 
  ENABLE_2009_ADVT                  = 32,
//  MANUF_FORMAT_VER                  = 30,
//  MANUF_EUI64_ADDR                  = 31,              
//  MANUF_VREF                        = 32,
//  MANUF_MAX_PA                      = 33,
//  MANUF_CRISTAL                     = 34,

  CUST_PROV_ATTR_NO                  
}; // CUST_PROV_ATTRIBUTES
  

    typedef struct 
    {
      uint16 m_unFormatVersion;                   // 0x0080 
//      uint16 m_unSubnetID;                        // 0x0082
//      uint16 m_unFilterBitMask;                   // 0x0084
//      uint16 m_unFilterTargetID;                  // 0x0086             
//      uint8  m_aAppJoinKey[16];                   // 0x0088 
      uint8  m_aDllJoinKey[16];                   // 0x0098
      uint8  m_aProvisionKey[16];                 // 0x00A8    
//      uint8  m_aSecMngrEUI64[8];                  // 0x00B8
//      uint8  m_aSysMngrIPv6[16];                  // 0x00C0
//      uint8  m_aIpv6BBR[16];                      // 0x00D0
//      uint8  m_aSysMngrIPv4[4];                   // 0x00E0       redundant data (last 4 bytes from corresponding IPv6); the space allocated remains
      
//      uint8  m_aReserved0[12];                    // 0x00E4       reserved0 data : 12 bytes
      
//      int16  m_nCrtUTCDrift;                      // 0x00F0
//      uint8  m_aNextTimeUTCDrift[4];              // 0x00F2
//      int16  m_nNextUTCDrift;                     // 0x00F6
      
//      uint8  m_aReserved1[8];                     // 0x00F8       reserved1 data : 8 bytes
      
//      uint8  m_aIPv4BBR[4];                       // 0x0100       redundant data (last 4 bytes from corresponding IPv6); the space allocated remains
      uint32  m_ulIPv4BBRMask;                    // 0x0104      
      uint32  m_ulIPv4GWY;                        // 0x0108
      
      uint32 m_ulIP4LOG;                          // 0x010C
      uint16 m_unPort4LOG;                        // 0x0110
      uint8  m_aucVerboseLOG[NUMBER_OF_MODULES];   // 0x0112
      
#ifdef WCI_SUPPORT 
      uint32 m_ulIP4LOGAck;                          // 0x0122
      uint16 m_unPort4LOGAck;                        // 0x0126
      uint8  m_aucVerboseLOGAck[NUMBER_OF_MODULES];   // 0x0128
#endif
    } BBR_CUST_PROVISIONING;

extern BBR_CUST_PROVISIONING g_stProvisioning; 
extern const DMAP_FCT_STRUCT c_aCustProvFct[CUST_PROV_ATTR_NO];
extern uint8 g_ucDllQueueOrdering;

#define CUST_ACK_TYPE_OFFSET        0x05
#define CUST_ACK_COUNT_MASK         0x1F

#define CUST_ACK_TYPE_NO_SENT       0x04

extern uint8 g_ucACKConfig;
extern uint8 g_ucNoSendACK;
extern int16 g_nClockDriftMicroSec;
extern uint16 g_unWrongDAUXCount;
extern uint8 g_ucWrongUDPCSCount;
extern uint8 g_ucWrongAckMICCount;
extern uint32 g_ulStartTAISec;
extern uint8 g_ucPALevel;

#define BASIC_LOGGING_LEVEL         0x00 //just Application Request/Responses
#define DLL_LOGGING_LEVEL           0x01 //Basic Level + DLL messages without D-BBR TxAdvertisement
#define EXTENDED_LOGGING_LEVEL      0x02 //Basic + DLL + TxAdvertisements

extern uint8 g_ucLoggingLevel;

void SaveManufacturingData();
void SaveProvisioningData();

#define PROV_Read(p_unAttrID,p_punBufferSize,p_pucRspBuffer) \
            DMAP_ReadAttr(p_unAttrID,p_punBufferSize,p_pucRspBuffer,c_aCustProvFct,CUST_PROV_ATTR_NO)

#define PROV_Write(p_unAttrID,p_ucBufferSize,p_pucBuffer)   \
            DMAP_WriteAttr(p_unAttrID,p_ucBufferSize,p_pucBuffer,c_aCustProvFct,CUST_PROV_ATTR_NO)

#endif //defined(at91sam7x512)

#endif // _NIVIS_PROVISION_H_
