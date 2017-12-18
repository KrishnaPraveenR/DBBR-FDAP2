#ifndef _NIVIS_ERRORS_H_
#define _NIVIS_ERRORS_H_

//severity of the error
#define LOG_ERROR     0
#define LOG_WARNING   1
#define LOG_INFO      2
#define LOG_DEBUG     3

//module 
#define LOG_M_SYS     0
#define LOG_M_DLL     1
#define LOG_M_NL      2
#define LOG_M_TL      3
#define LOG_M_ASL     4
#define LOG_M_APP     5
#define LOG_M_MAP     6
#define NUMBER_OF_MODULES       16 //power of 2

//SYS log operation
typedef enum
{
  SYSOP_GENERAL,
  SYSOP_Clock,
  PLOG_DataReq,
  PLOG_HdrIndicate,
  PLOG_Indicate,
  PLOG_Confirm,
  ETH_UdpSnd,
  ETH_UdpRcv
} SYS_LOG_OPERATIONS;

//DLL log operation
typedef enum
{
  DLOG_DataReq,
  DLOG_Indicate,
  DLOG_Confirm,
  DLOG_SMIB_Add,
  DLOG_SMIB_Del,
  DLOG_SMIB_Mod,
  DLOG_Retry,
  DLOG_Hash,
  DLOG_LocalLoop,
  DLOG_AckInterpreter,
  DLOG_genAdvDauxHdr
} DL_LOG_OPERATIONS;

//NL_ log operation
typedef enum
{
  NLOG_DataReq,
  NLOG_Indicate,
  NLOG_Confirm,
  NLOG_SMIB_Add,
  NLOG_SMIB_Del,
  NLOG_SMIB_Mod,
  NLOG_6LoPANToUdp,
  NLOG_FindRoute
} NL_LOG_OPERATIONS;


//TL_ log operation
typedef enum
{
  TLOG_DataReq,
  TLOG_Indicate,
  TLOG_Confirm,
  TLOG_SMIB_Add,
  TLOG_SMIB_Del,
  TLOG_SMIB_Mod
} TL_LOG_OPERATIONS;


//ASL log operation
typedef enum
{
  ASLOG_Get,
  ASLOG_Set
} ASL_LOG_OPERATIONS;

//APP log operation
//typedef enum
//{
//} APP_LOG_OPERATIONS;

//MAP log operation
typedef enum
{
  MAPLOG_Status,
  MAPLOG_Reset,
  MAPLOG_ReadReq,
  MAPLOG_WriteReq,
  MAPLOG_ExecReq,
  MAPLOG_ReadRsp,
  MAPLOG_WriteRsp,
  MAPLOG_ExecRsp,
  MAPLOG_JontRsp,
  MAPLOG_ChSts,
  MAPLOG_NeSts,
  MAPLOG_LkSts
  
} MAP_LOG_OPERATIONS;

#endif //_NIVIS_ERRORS_H_
