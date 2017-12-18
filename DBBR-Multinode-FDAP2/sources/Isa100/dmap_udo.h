////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, 
/// Date:         June 2008
/// Description:  his file holds definitions of the Upload/Download Object in DMAP
/// Changes:      Created
/// Revisions:    1.0
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _NIVIS_DMAP_UDO_H_
#define _NIVIS_DMAP_UDO_H_

#include "../typedef.h"
#include "aslsrvc.h" 

#if   (DEVICE_TYPE == DEV_TYPE_MC13225) 
  #define FLASH_PROG_START_ADDR       0
  #define FLASH_PROG_END_ADDR         0x0001FFFF
  #define UDP_FW_CODE_START_ADDR      0
    
#elif (DEVICE_TYPE == DEV_TYPE_MSP430F2618) 
  #include "../spi1_eeprom.h"
  #include "../spi1.h"

  #define FLASH_PROG_START_ADDR       0x3600
  #define FLASH_PROG_END_ADDR         0x1FFFF
  #define SWAP_ENTRY_POINT_ADDR       0x3100
  #define UDP_FW_CODE_START_ADDR      EEP_FW_CODE_ADDR
    
#elif (DEVICE_TYPE == DEV_TYPE_AP91SAM7X512) 
  #include "../spi_eeprom.h"
  #include "../spi1.h"

  #define FLASH_PROG_START_ADDR       0x0000
  #define FLASH_PROG_END_ADDR         0x1FFFF //maximum 128 KB flash reserved for code   
  #define UDP_FW_CODE_START_ADDR      EEP_FW_CODE_ADDR
    
#else
  #warning  "UDO_downloadData not supported"   
#endif    

enum{
  UDO_ZERO,
  UDO_START_DWLD,
  UDO_DWLD_DATA,
  UDO_END_DWLD,
  UDO_START_UPLD,
  UDO_UPLD_DATA,
  UDO_END_UPLD,
  UDO_NO
};//UDO_METHODS;

//enum{
//  UDO_ZERO,
//  UDO_START_DOWNLOAD,
//  UDO_DOWNLOAD_DATA,
//  UDO_END_DOWNLOAD,
//  UDO_START_UPLOAD,
//  UDO_UPLOAD_DATA,
//  UDO_END_UPLOAD,
//  UDO_NO
//};//UDO_METHODS;

enum{
  UDO_RES = 0,        // reserved for future use
  UDO_OPS,            // operations supported: 
                      //    0=defined size unicast upld only
                      //    1=defined size unicast dwld only
                      //    2=defined size unicast upd & unicast dwld
                      //    3-15 res. for future use
  UDO_DESCR,          // human readable identification of associated content
  UDO_STATE,          // 0=idle, 1=downloading, 2=uploading, 3=applying
                      // 4=dwld complete, 5=upld complete, 6=dwld err, 7=upld err
  UDO_COMM,           // 0=reset, 1=apply(dwld only), 2-15 res. for future use
  UDO_MAX_BLCK_SIZE,   
  UDO_MAX_DWLD_SIZE,
  UDO_MAX_UPLD_SIZE,
  UDO_DWLD_PREP_TIME, // time required, in seconds, to prepare for a dwld
  UDO_DWLD_ACT_TIME,  // time in seconds for the obj to activate newly dwlded content
  UDO_UPLD_PREP_TIME, // time required in seconds to prepare for an upld
  UDO_UPLD_PR_TIME,   // typical time in seconds for this obj to process a req to 
                      // upld a block
  UDO_DWLD_PR_TIME,   // typical time in seconds for this object to process a
                      // downloaded block
  UDO_CUTOVER_TIME,   // time specified to activate the download content
  UDO_LAST_BLK_DWLD,   // time specified to activate the dwld content
  UDO_LAST_BLK_UPLD,
  UDO_ATTR_NO
};//UDO_ATTRIBUTES;


enum{
  UPLOAD_IDLE,
  UPLOAD_RUNNING,
  UPLOAD_DATA_WAITING,
  UPLOAD_COMPLETE,
  UPLOAD_ERROR
};

enum {
  UDO_STATE_IDLE = 0,
  UDO_STATE_DOWNLOADING,
  UDO_STATE_UPLOADING,
  UDO_STATE_APPLYING,
  UDO_STATE_DL_COMPLETE,
  UDO_STATE_UL_COMPLETE,
  UDO_STATE_DL_ERROR,
  UDO_STATE_UL_ERROR
}; // UDO_STATE;


extern uint8 g_ucUDOState; //RO
extern uint8 g_ucCommand; //WO - non cacheable ??
extern uint16 g_unMaxBlockSize; //RO
extern uint16 g_unMaxUploadSize; //RO
extern uint16 g_unDownloadPrepTime; //RO
extern uint16 g_unDownloadActivationTime; //RO
extern uint16 g_unUploadPrepTime; //RO
extern uint16 g_unUploadProcessingTime; //RO
extern uint16 g_unDownloadProcessingTime; //RO
extern TIME   g_ulUDOCutoverTime; //RW - in seconds
extern uint16 g_unLastBlockDwld; //RO
extern uint16 g_unLastBlockUpld; //RO


extern uint16 g_unCrtClientUpldBlockNo;

extern void UDO_Init();

extern void UDO_ProcessExecuteRequest( EXEC_REQ_SRVC * p_pExecReq, 
                                       EXEC_RSP_SRVC * p_pExecRsp );

extern uint8 UDO_Read(  uint16   p_unAttrId, 
                        uint16 * p_pSize, 
                        uint8*  p_pBuf);

extern uint8 UDO_Write( uint16   p_unAttrId, 
                        uint16   p_unSize, 
                        const uint8* p_pBuf);


#endif //_NIVIS_DMAP_UDO_H_
