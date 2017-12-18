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

#include <string.h>

#include "provision.h"
#include "../isa100/dmap_dmo.h"
#include "../isa100/dlmo_utils.h"
#include "../isa100/dmap.h"
#include "../isa100/mlde.h"
#include "../isa100/mlsm.h"
#include "../usart.h"
#include "../eth.h"


  EUI64_ADDR c_oEUI64BE; // nwk order EUI64ADDR
  EUI64_ADDR c_oEUI64LE; // little endian order
  
#if defined(at91sam7x512)
  
  #ifdef RESET_TO_FACTORY
  uint32 g_ulPersistent_SS_IPv4;
  uint16 g_unPersistent_SS_PORT;
  IPV6_ADDR g_aucPersistent_DBBR_IPv6;
  uint32 g_ulPersistent_MASK_IPv4;
  uint32 g_ulPersistent_StartTAI;
  #endif

  BBR_CUST_PROVISIONING g_stProvisioning;
  
  uint16 c_unPort; 
  
//  uint32 c_ulMASK;   //
//  uint32 c_ulGWY;    //
//  uint32 c_ulIP4LOG;        //
//  uint16 c_usPort4LOG;      //
//  uint8 c_ucVerboseLOG[NUMBER_OF_MODULES];     //
//#ifdef WCI_SUPPORT  
//  uint32 c_ulIP4LOGAck;     //  
//  uint16 c_usPort4LOGAck;   //
//  uint8 c_ucVerboseLOGAck[NUMBER_OF_MODULES];     //
//#endif  // WCI_SUPPORT  
#endif  // at91sam7x512
  uint8  g_ucDllQueueOrdering;
  uint8  g_ucACKConfig;    //b7b6b5 - ACK type - 0 - normal functioning
                           //                  - 1 - ACK ECN 
                           //                  - 2 - NACK0
                           //                  - 3 - NACK1
                           //                  - 4 - no ACK/NACK sent
                           //b4..b0 - count of the ACK/NACK transmissions/escapes 
  uint8 g_ucLoggingLevel;
  uint8 g_ucNoSendACK;
  int16 g_nClockDriftMicroSec;
  uint16 g_unWrongDAUXCount;
  uint8 g_ucWrongUDPCSCount;
  uint8 g_ucWrongAckMICCount;
  uint32 g_ulStartTAISec;
  uint8 g_ucPALevel;
  
#if( DEVICE_TYPE == DEV_TYPE_MC13225 ) && defined (BACKBONE_SUPPORT)
    uint8 g_ucProvisioned;
#endif
  
const DMAP_FCT_STRUCT c_aCustProvFct[CUST_PROV_ATTR_NO] =
{
  { 0, 0                                              , DMAP_EmptyReadFunc     , NULL },      // just for protection; attributeID will match index in this table     
  { ATTR_CONST(g_stProvisioning.m_unFormatVersion)    , DMAP_ReadUint16        , DMAP_WriteUint16 }, 
  { ATTR_CONST(g_unDllSubnetId)                       , DMAP_ReadUint16        , DMAP_WriteUint16 },        
  { ATTR_CONST(g_stFilterBitMask)                     , DMAP_ReadUint16        , DMAP_WriteUint16 },    
  { ATTR_CONST(g_stFilterTargetID)                    , DMAP_ReadUint16        , DMAP_WriteUint16 },        
  { ATTR_CONST(g_aJoinAppKey)                         , DMAP_ReadVisibleString , DMAP_WriteVisibleString },          
  
  { ATTR_CONST(g_stProvisioning.m_aDllJoinKey)        , DMAP_ReadVisibleString , DMAP_WriteVisibleString }, 
  { ATTR_CONST(g_stProvisioning.m_aProvisionKey)      , DMAP_ReadVisibleString , DMAP_WriteVisibleString },  
  { ATTR_CONST(c_oSecManagerEUI64BE)                  , DMAP_ReadVisibleString , DMAP_WriteVisibleString },  
  { ATTR_CONST(g_stDMO.m_aucSysMng128BitAddr)         , DMAP_ReadVisibleString , DMAP_WriteVisibleString },  
#ifdef RESET_TO_FACTORY
  { ATTR_CONST(g_aucPersistent_DBBR_IPv6)             , DMAP_ReadVisibleString , DMAP_WriteVisibleString },      
#else
  { ATTR_CONST(g_stDMO.m_auc128BitAddr)               , DMAP_ReadVisibleString , DMAP_WriteVisibleString },      
#endif
  { ATTR_CONST(g_stDMO.m_unCrtUTCDrift)               , DMAP_ReadUint16         , DMAP_WriteUint16 },  
  { ATTR_CONST(g_stDMO.m_ulNextDriftTAI)              , DMAP_ReadUint32         , DMAP_WriteUint32 },  
  { ATTR_CONST(g_stDMO.m_unNextUTCDrift)              , DMAP_ReadUint16         , DMAP_WriteUint16 },   

#ifdef RESET_TO_FACTORY
  { ATTR_CONST(g_ulPersistent_MASK_IPv4)              , DMAP_ReadVisibleString , DMAP_WriteVisibleString },//DMAP_ReadUint32         , DMAP_WriteUint32 },
#else
  { ATTR_CONST(g_stProvisioning.m_ulIPv4BBRMask)      , DMAP_ReadVisibleString , DMAP_WriteVisibleString },//DMAP_ReadUint32         , DMAP_WriteUint32 },
#endif
  { ATTR_CONST(g_stProvisioning.m_ulIPv4GWY)          , DMAP_ReadVisibleString , DMAP_WriteVisibleString },//DMAP_ReadUint32         , DMAP_WriteUint32 },
  { ATTR_CONST(g_stProvisioning.m_ulIP4LOG)           , DMAP_ReadVisibleString , DMAP_WriteVisibleString },//DMAP_ReadUint32         , DMAP_WriteUint32 },
  { ATTR_CONST(g_stProvisioning.m_unPort4LOG)         , DMAP_ReadVisibleString , DMAP_WriteVisibleString },//DMAP_ReadUint16         , DMAP_WriteUint16 },
  { ATTR_CONST(g_stProvisioning.m_aucVerboseLOG)      , DMAP_ReadVisibleString , DMAP_WriteVisibleString }, 

#ifdef WCI_SUPPORT 
   #ifdef RESET_TO_FACTORY
   { ATTR_CONST(g_ulPersistent_SS_IPv4)                , DMAP_ReadVisibleString , DMAP_WriteVisibleString },//DMAP_ReadUint32         , DMAP_WriteUint32 
   { ATTR_CONST(g_unPersistent_SS_PORT)                , DMAP_ReadVisibleString , DMAP_WriteVisibleString },//DMAP_ReadUint16         , DMAP_WriteUint16 },         
   #else   
   { ATTR_CONST(g_stProvisioning.m_ulIP4LOGAck)        , DMAP_ReadVisibleString , DMAP_WriteVisibleString },//DMAP_ReadUint32         , DMAP_WriteUint32 },
   { ATTR_CONST(g_stProvisioning.m_unPort4LOGAck)      , DMAP_ReadVisibleString , DMAP_WriteVisibleString },//DMAP_ReadUint16         , DMAP_WriteUint16 },         
   #endif
   { ATTR_CONST(g_stProvisioning.m_aucVerboseLOGAck)   , DMAP_ReadVisibleString , DMAP_WriteVisibleString },      
#endif
   //below are not persistent parameters
   { ATTR_CONST(g_ucDllQueueOrdering)                  , DMAP_ReadUint8         , DMAP_WriteUint8 },
   { ATTR_CONST(g_ucACKConfig)                         , DMAP_ReadUint8         , DMAP_WriteUint8 },
   { ATTR_CONST(g_ucLoggingLevel)                      , DMAP_ReadUint8         , DMAP_WriteUint8 },
   { ATTR_CONST(g_nClockDriftMicroSec)                 , DMAP_ReadUint16        , DMAP_WriteUint16 },
   { ATTR_CONST(g_unWrongDAUXCount)                    , DMAP_ReadUint16        , DMAP_WriteUint16 },
   { ATTR_CONST(g_ucWrongUDPCSCount)                   , DMAP_ReadUint8         , DMAP_WriteUint8 },
   { ATTR_CONST(g_ucWrongAckMICCount)                  , DMAP_ReadUint8         , DMAP_WriteUint8 },
   
   //below attribute must be persistent
#ifdef RESET_TO_FACTORY
    { ATTR_CONST(g_ulPersistent_StartTAI)              , DMAP_ReadUint32        , DMAP_WriteUint32 },
#else
    { ATTR_CONST(g_ulStartTAISec)                      , DMAP_ReadUint32        , DMAP_WriteUint32 },
#endif
       
    { ATTR_CONST(c_oEUI64BE)                           , DMAP_ReadVisibleString , DMAP_WriteVisibleString },
   
    //below attribute must be persistent
    { ATTR_CONST(g_ucPALevel)                          , DMAP_ReadUint8         , DMAP_WriteUint8 },
};    
    
const DLL_MIB_DEV_CAPABILITIES c_stCapability = 
{
  DLL_MSG_QUEUE_SIZE_MAX, // m_unQueueCapacity
  DLL_CHANNEL_MAP,        // m_unChannelMap
  DLL_ACK_TURNAROUND,     // m_unAckTurnaround
  200,                    // m_unNeighDiagCapacity
  DLL_CLOCK_ACCURACY,     // m_ucClockAccuracy;
  DLL_DL_ROLE,            // m_ucDLRoles;
  SUPPORTED_CCA_MODE_3,   //m_ucSupportedCCAModes (ISA2011 change)  // In CC2420 the setting is done in MDMCTRL0, whereas in CC2520 it is done in CCACTRL1
  (BIT0 | BIT1)           // m_ucOptions;   //group codes and graph extensions are supported 
};

const DLL_MIB_ENERGY_DESIGN c_stEnergyDesign = 
{
#ifdef BACKBONE_SUPPORT
  
  0x7FFF, //  days capacity, 0x7FFF stands for permanent power supply
    3600, //  3600 seconds per hour Rx listen capacity
     600, //  600 seconds per hour Rx listen capacity
     120  //  120 Advertises/minute rate 
  
#else 
    #ifdef ROUTING_SUPPORT
      180,    // 180 days battery capacity
      360,    // 360 seconds per hour Rx listen capacity
      100,    // 100 DPDUs/minute transmit rate
       60     //  60 Advertises/minute rate          
    #else
      180,    // 180 days battery capacity
      360,    // 360 seconds per hour Rx listen capacity
      100,    // 100 DPDUs/minute transmit rate
       60     //  60 Advertises/minute rate     
  #endif // ROUTING_SUPPORT 
#endif // BACKBONE_SUPPORT
};

const DLL_SMIB_ENTRY_TIMESLOT c_aDefTemplates[DEFAULT_TS_TEMPLATE_NO] =
{
  {
    { DEFAULT_RX_TEMPL_UID, // m_unUID
      DLL_MASK_TSBOUND, // m_ucType - Rx + ACK relative to the end of incoming DPDU + respect slot boundaries
      {  
        //1us = 1.048576 * 2^-20
        1271, 3578, 977, 1187, 0, 0, 0, 0  //timings inside Rx timeslot(2^-20 sec)
      } // m_utTemplate
    }  
  },
  {
    { DEFAULT_TX_TEMPL_UID, // m_unUID
      DLL_TX_TIMESLOT | (1 << DLL_ROT_TSCCA ),  // m_ucType - Tx + ACK relative to the end of incoming DPDU +  
                                                //            CCA when energy above threshold + not keep listening
      {  
        //1us = 1.048576 * 2^-20
        2319, 2529, 977, 1187, 0, 0, 0, 0       //timings inside Tx timeslot(2^-20 sec)
      } // m_utTemplate
    }  
  },
  { //default receive template for scanning
    { DEFAULT_TX_TEMPL_UID + 1, // m_unUID
      0, // m_ucType - Rx + ACK relative to the end of incoming DPDU + not respect slot boundaries
      {  
        //1us = 1.048576 * 2^-20
        0, 0xffff, 977, 1187, 0, 0, 0, 0       //timings inside Rx timeslot(2^-20 sec)
      } // m_utTemplate
    }  
  },
};


const DLL_SMIB_ENTRY_CHANNEL c_aDefChannels[DEFAULT_CH_NO] =
{
  {
    { 0x01,
      16, //range 1 to 16 (not 0 to 15!!!)
      0x18, 0xD9, 0xC5, 0xE7, 0xA3, 0x40, 0x6B, 0xF2 //seq0 ... seq8
    }
  },
  {
    { 0x02,
      16, //range 1 to 16 (not 0 to 15!!!)
      0x2F, 0xB6, 0x04, 0x3A, 0x7E, 0x5C, 0x9D, 0x81 //seq0 ... seq8
    }
  },
  {
    { 0x03,
      3, //range 1 to 16 (not 0 to 15!!!)
      0x94, 0x0E, 0, 0, 0, 0, 0, 0 //seq0 ... seq8
    }
  },
  {
    { 0x04,
      3, //range 1 to 16 (not 0 to 15!!!)
      0x9E, 0x04, 0, 0, 0, 0, 0, 0 //seq0 ... seq8
    }
  },
  {
    { 0x05,
      16, //range 1 to 16 (not 0 to 15!!!)
      0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE //seq0 ... seq8
    }
  }

};

  
  void PROVISION_Overwrite(void)
  {
//#define OVERWRITE_MANUFACTURING_INFO  // if this line is uncommented will overwrite the provisioned info     
#ifdef OVERWRITE_MANUFACTURING_INFO
    
    // manufacturing info
    const struct 
    {
        uint16 m_unFormatVersion; 
        uint8  m_aMAC[8];              
        uint16 m_unVRef;    
        uint8  m_ucMaxPA;        
        uint8  m_ucCristal;        
    } c_stManufacturing = {0, 
      #if defined( PROVISIONING_DEVICE )
                        { 0xFD, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, _ISA100_DEVICE_ID }, // dev addr
      #elif defined( BACKBONE_SUPPORT )
                        { 0xFB, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, _ISA100_DEVICE_ID }, // dev addr
                        
      #elif defined( ROUTING_SUPPORT )
                        { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0B, _ISA100_DEVICE_ID }, // dev addr

      #else
                        { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, _ISA100_DEVICE_ID }, // dev addr
                        
      #endif                            
                        0xC409,
                        0xE7,
                        0x00
                        };
    
    EraseSector( MANUFACTURING_SECTOR_NO );
    WritePersistentData( (uint8*)&c_stManufacturing, MANUFACTURING_START_ADDR, sizeof(c_stManufacturing) );

  #endif // OVERWRITE_MANUFACTURING_INFO
  }

  
  void PROVISION_Init(void)
  {        
//   if( !g_stDPO.m_ucStructSignature ) 
     {
         PROVISION_Overwrite();

#if  (DEVICE_TYPE == DEV_TYPE_MC13225)         
          uint8 aCalibratedValues[2];
          ReadPersistentData( aCalibratedValues, MANUFACTURING_START_ADDR + 2 + 8 + 2, sizeof(aCalibratedValues) ); // read calibrated m_ucMaxPA and m_ucCristal
          
          CRM_XTAL_CNTL.CTune = (0x18 + aCalibratedValues[1]) & 0x001F; // m_ucCristal
#endif         
         
#if !defined( BACKBONE_SUPPORT ) || defined( PROVISIONING_DEVICE ) // BBR will receive manufacturing on different way

         // manufacturing data
          ReadPersistentData( c_oEUI64BE, MANUFACTURING_START_ADDR + 2, sizeof(c_oEUI64BE) );
          DLME_CopyReversedEUI64Addr( c_oEUI64LE, c_oEUI64BE );          
          
#else // BBR                  
  #if  (DEVICE_TYPE == DEV_TYPE_AP91SAM7X512)         
        // manufacturing data   
        i2c_ReadEEPROM( 2, c_oEUI64BE, 8 );
        // initialize vendro to Honeywell always.  For BB22 hardware to work we need valid vendor id.
        c_aETH_MAC[0]= 0x00;
        c_aETH_MAC[1]= 0x40;
        c_aETH_MAC[2]= 0x84;
        memcpy( c_aETH_MAC+3, c_oEUI64BE+5, 3);
        DLME_CopyReversedEUI64Addr( c_oEUI64LE, c_oEUI64BE );

            
          // provisioning data (sector 7)
//          BBR_UART_PROVISIONING stCrtProvisioning;             
//          i2c_ReadEEPROM(0x80, (uint8*)&stCrtProvisioning, sizeof(stCrtProvisioning));
          
          i2c_ReadEEPROM( 0x80, (uint8*)&g_stProvisioning.m_unFormatVersion, 2 );
          //i2c_ReadEEPROM( 0x82, (uint8*)&g_unDllSubnetId, 2 );
          i2c_ReadEEPROM( 0x84, (uint8*)&g_stFilterBitMask, 2 );
          i2c_ReadEEPROM( 0x86, (uint8*)&g_stFilterTargetID, 2 );
          
          i2c_ReadEEPROM( 0x88, g_aJoinAppKey, 16 );
          i2c_ReadEEPROM( 0x98, g_stProvisioning.m_aDllJoinKey, 16 );
          i2c_ReadEEPROM( 0xA8, g_stProvisioning.m_aProvisionKey, 16 );
          
    
          i2c_ReadEEPROM( 0xB8, c_oSecManagerEUI64BE, 8 );
          i2c_ReadEEPROM( 0xC0, g_stDMO.m_aucSysMng128BitAddr, 16 );
          i2c_ReadEEPROM( 0xD0, g_stDMO.m_auc128BitAddr, 16 );
          memcpy( &c_unPort, g_stDMO.m_auc128BitAddr+10, 2);
          
          i2c_ReadEEPROM( 0xF0, (uint8*)&g_stDMO.m_unCrtUTCDrift, 2 );
          g_stDMO.m_ulNextDriftTAI = 0xFFFFFFFF;
          g_stDMO.m_unNextUTCDrift = g_stDMO.m_unCrtUTCDrift;
          
          //read start TAI second 
          i2c_ReadEEPROM( 0xF8, (uint8*)&g_ulStartTAISec, 4 );
          
          //read Chipcon PA level
          i2c_ReadEEPROM( 0xFC, &g_ucPALevel, 1 );
          
        #define IPv4_ADDR       0x100
    //        i2c_ReadEEPROM( 0x100, (uint8*)&c_ulIP, 4 );
          i2c_ReadEEPROM( IPv4_ADDR+4, (uint8*)&c_ulMASK, 4 );
          i2c_ReadEEPROM( IPv4_ADDR+8, (uint8*)&c_ulGWY, 4 );    
          
          i2c_ReadEEPROM( IPv4_ADDR+12, (uint8*)&c_ulIP4LOG, 4 );
          i2c_ReadEEPROM( IPv4_ADDR+16, (uint8*)&c_usPort4LOG, 2 );            
          i2c_ReadEEPROM( IPv4_ADDR+18, (uint8*)c_ucVerboseLOG, NUMBER_OF_MODULES );      
    
      #ifdef WCI_SUPPORT
        #define IPv4_WCI_ADDR   (IPv4_ADDR+NUMBER_OF_MODULES+18)      
          i2c_ReadEEPROM( IPv4_WCI_ADDR, (uint8*)&c_ulIP4LOGAck, 4 );
          i2c_ReadEEPROM( IPv4_WCI_ADDR+4, (uint8*)&c_usPort4LOGAck, 2 );    
          i2c_ReadEEPROM( IPv4_WCI_ADDR+6, (uint8*)c_ucVerboseLOGAck, NUMBER_OF_MODULES );      
      #endif  // WCI_SUPPORT        
    

    #ifdef RESET_TO_FACTORY
          const uint8 aucFactory_IPv6[16] = {0xFE, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0x4E, 0x7D, 192, 168, 254, 254};
          const uint8 aucFactory_MASK_IPv4[4] = {255, 255, 255, 0 };

          memcpy( g_stDMO.m_auc128BitAddr, aucFactory_IPv6, sizeof(g_stDMO.m_auc128BitAddr) );
          memcpy( (uint8*)&c_unPort, g_stDMO.m_auc128BitAddr+10, 2);
          
          memcpy((uint8*)&c_ulMASK, aucFactory_MASK_IPv4, sizeof(c_ulMASK));
          c_usPort4LOGAck = 0x7E4E; //LE format  
          
          //          memcpy((uint8*)&c_ulGWY, aucFactory_GW_IPv4, sizeof(c_ulGWY));
          
          g_ulStartTAISec = 0x0FFF;
          g_ucPALevel = CHIPCON_DEFAULT_PA_LEVEL;
          //update also the SubnetID which normally is updated earlier
          g_unDllSubnetId = g_stFilterTargetID;

    
          //for test
//          BBR_UART_PROVISIONING stCrtProvisioning;
//          memset(&stCrtProvisioning, 0x00, sizeof(stCrtProvisioning));
//          i2c_WriteEEPROM(0x80, (const uint8*)&stCrtProvisioning, sizeof(stCrtProvisioning) );
    
    #endif

    #endif  // DEVICE_TYPE == DEV_TYPE_AP91SAM7X512      
#endif  // BBR   
          
     }        
  }
  
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Dorin Pavel   
/// @brief  Generic function for read non-volatile data 
/// @params p_pucDst - destination buffer
///         p_uAddr  - (sector) address 
///         p_unSize - number of bytes to read
/// @return none
/// @remarks
///      Access level: user level
//////////////////////////////////////////////////////////////////////////////////////////////////// 
void ReadPersistentData( uint8 *p_pucDst, PROV_ADDR_TYPE p_uAddr, uint16 p_unSize )
{ 
#if( DEVICE_TYPE == DEV_TYPE_MC13225 )
   // read persistent data from flash
    NVM_FlashRead(p_pucDst, p_uAddr, p_unSize);  
#elif( DEVICE_TYPE == DEV_TYPE_MSP430F2618 )
   //  read persistent data from eeprom 
    SPI_ReadEEPROM(p_pucDst, p_uAddr, p_unSize);  
#endif  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Dorin Pavel   
/// @brief  Generic function for write non-volatile data  
/// @params p_pucSrc            - source buffer
///         p_uAddr - (sector) address
///         p_unSize            - number of bytes to write
/// @return none
/// @remarks
///      Access level: user level
//////////////////////////////////////////////////////////////////////////////////////////////////// 
void WritePersistentData( const uint8 *p_pucSrc, PROV_ADDR_TYPE p_uAddr, uint16 p_unSize )
{ 
#if( DEVICE_TYPE == DEV_TYPE_MC13225 )
   // write data into flash
      NVM_FlashWrite( (void*)p_pucSrc, p_uAddr, p_unSize);
#elif( DEVICE_TYPE == DEV_TYPE_MSP430F2618 )
   //  write data into eeprom 
      SPI_eeprom_write( p_uAddr, p_pucSrc, p_unSize);  
#endif  
}


void SaveProvisioningData( )
{ 
    BBR_UART_PROVISIONING stCrtProvisioning, stInitProvisioning;
        
    i2c_ReadEEPROM(0x80, (uint8*)&stInitProvisioning, sizeof(stInitProvisioning));
    
    //to keep the Reserved fields
    memcpy(&stCrtProvisioning, &stInitProvisioning, sizeof(stCrtProvisioning));
    
    //update parameters
    stCrtProvisioning.m_unFormatVersion = g_stProvisioning.m_unFormatVersion;
    stCrtProvisioning.m_unSubnetID = g_unDllSubnetId;
    stCrtProvisioning.m_unFilterBitMask = g_stFilterBitMask;
    stCrtProvisioning.m_unFilterTargetID = g_stFilterTargetID;

    memcpy(stCrtProvisioning.m_aAppJoinKey, g_aJoinAppKey, sizeof(stCrtProvisioning.m_aAppJoinKey));
    memcpy(stCrtProvisioning.m_aDllJoinKey, g_stProvisioning.m_aDllJoinKey, sizeof(stCrtProvisioning.m_aDllJoinKey));
    memcpy(stCrtProvisioning.m_aProvisionKey, g_stProvisioning.m_aProvisionKey, sizeof(stCrtProvisioning.m_aProvisionKey));
    memcpy(stCrtProvisioning.m_aSecMngrEUI64, c_oSecManagerEUI64BE, sizeof(stCrtProvisioning.m_aSecMngrEUI64));
    memcpy(stCrtProvisioning.m_aSysMngrIPv6, g_stDMO.m_aucSysMng128BitAddr, sizeof(stCrtProvisioning.m_aSysMngrIPv6));
    
#ifdef RESET_TO_FACTORY    
    memcpy(stCrtProvisioning.m_aIpv6BBR, g_aucPersistent_DBBR_IPv6, sizeof(stCrtProvisioning.m_aIpv6BBR));    
    memcpy(stCrtProvisioning.m_aIPv4BBR, g_aucPersistent_DBBR_IPv6 + 12, sizeof(stCrtProvisioning.m_aIPv4BBR));
    memcpy(stCrtProvisioning.m_aIPv4BBRMask, &g_ulPersistent_MASK_IPv4, sizeof(stCrtProvisioning.m_aIPv4BBRMask));
    #ifdef WCI_SUPPORT
    memcpy(stCrtProvisioning.m_aIP4LOGAck, &g_ulPersistent_SS_IPv4, sizeof(stCrtProvisioning.m_aIP4LOGAck));
    memcpy(stCrtProvisioning.m_aPort4LOGAck, &g_unPersistent_SS_PORT, sizeof(stCrtProvisioning.m_aPort4LOGAck));
    #endif
    stCrtProvisioning.m_ulInitialTAISec = g_ulPersistent_StartTAI;
#else
    memcpy(stCrtProvisioning.m_aIpv6BBR, g_stDMO.m_auc128BitAddr, sizeof(stCrtProvisioning.m_aIpv6BBR));
    memcpy(stCrtProvisioning.m_aIPv4BBR, g_stDMO.m_auc128BitAddr + 12, sizeof(stCrtProvisioning.m_aIPv4BBR));
    memcpy(stCrtProvisioning.m_aIPv4BBRMask, &g_stProvisioning.m_ulIPv4BBRMask, sizeof(stCrtProvisioning.m_aIPv4BBRMask));
    #ifdef WCI_SUPPORT
    memcpy(stCrtProvisioning.m_aIP4LOGAck, &g_stProvisioning.m_ulIP4LOGAck, sizeof(stCrtProvisioning.m_aIP4LOGAck));
    memcpy(stCrtProvisioning.m_aPort4LOGAck, &g_stProvisioning.m_unPort4LOGAck, sizeof(stCrtProvisioning.m_aPort4LOGAck));
    #endif
    stCrtProvisioning.m_ulInitialTAISec = g_ulStartTAISec;
#endif
    
    stCrtProvisioning.m_ucPALevel = g_ucPALevel;
    
    memcpy(stCrtProvisioning.m_aSysMngrIPv4, g_stDMO.m_aucSysMng128BitAddr + 12, sizeof(stCrtProvisioning.m_aSysMngrIPv4));
    stCrtProvisioning.m_nCrtUTCDrift = g_stDMO.m_unCrtUTCDrift;
    memcpy(stCrtProvisioning.m_aNextTimeUTCDrift, &g_stDMO.m_ulNextDriftTAI, sizeof(stCrtProvisioning.m_aNextTimeUTCDrift));
    stCrtProvisioning.m_nNextUTCDrift = g_stDMO.m_unNextUTCDrift;
    
    memcpy(stCrtProvisioning.m_aIPv4GWY, &g_stProvisioning.m_ulIPv4GWY, sizeof(stCrtProvisioning.m_aIPv4GWY));
    memcpy(stCrtProvisioning.m_aIP4LOG, &g_stProvisioning.m_ulIP4LOG, sizeof(stCrtProvisioning.m_aIP4LOG));
    memcpy(stCrtProvisioning.m_aPort4LOG, &g_stProvisioning.m_unPort4LOG, sizeof(stCrtProvisioning.m_aPort4LOG));
    memcpy(stCrtProvisioning.m_ucVerboseLOG, g_stProvisioning.m_aucVerboseLOG, sizeof(stCrtProvisioning.m_ucVerboseLOG));

#ifdef WCI_SUPPORT
    memcpy(stCrtProvisioning.m_ucVerboseLOGAck, g_stProvisioning.m_aucVerboseLOGAck, sizeof(stCrtProvisioning.m_ucVerboseLOGAck));
#endif 
      
    if( memcmp(&stInitProvisioning, &stCrtProvisioning, sizeof(stCrtProvisioning)) )
    {
        i2c_WriteEEPROM(0x80, (const uint8*)&stCrtProvisioning, sizeof(stCrtProvisioning) );
    }
}

void SaveManufacturingData( )
{ 
//    BBR_UART_MANUFACTURING stInitManufacturing;
//    i2c_ReadEEPROM(0x00, (uint8*)&stInitManufacturing, sizeof(stInitManufacturing));
    EUI64_ADDR aucCrtEUI64;
    i2c_ReadEEPROM( 0x02, aucCrtEUI64, sizeof(aucCrtEUI64) );
    
    //currently just the EUI64 address from the manufacturting is used by DBBR     
    if( memcmp(aucCrtEUI64, c_oEUI64BE, sizeof(c_oEUI64BE)) )
    {
        //memcpy(stInitManufacturing.m_aMAC, c_oEUI64BE, sizeof(c_oEUI64BE));
        i2c_WriteEEPROM(0x02, (const uint8*)c_oEUI64BE, sizeof(c_oEUI64BE) );
    }
}

#if( DEVICE_TYPE == DEV_TYPE_MC13225 )
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /// @author NIVIS LLC, Dorin Pavel   
  /// @brief  Generic function for clear non-volatile data  
  /// @params p_ulSectorNmb - sector number 
  /// @return none
  /// @remarks
  ///      Access level: user level
  //////////////////////////////////////////////////////////////////////////////////////////////////// 
  void EraseSector( uint32 p_ulSectorNmb )
  {  
     NVM_FlashErase(p_ulSectorNmb);
  }
#endif  
