
#include "logger.h"
#include "../isa100/provision.h"

#ifdef _NIVIS_LOGGING_

  #include "../itc.h"
  
  #define LOGQUEUE_BUFFER_SIZE (1024)
  #define SELECT_SEVERITY      (0xFF)
  #define SELECT_MODULE        (NUMBER_OF_MODULES-1)
  
  typedef struct
  {
      uint16  m_unLen;
      uint8 * m_pStart;
      uint16  m_unLogMsgNumber;
      uint8   m_pBuf[LOGQUEUE_BUFFER_SIZE];
  } LOG_QUEUE;
  

#pragma pack(1)
  typedef struct
  {
      union {
          struct {
              uint8   m_ucDataLen;
              uint8   m_unSeverity;
              uint8   m_unModule;
              uint8   m_unOperation;
          };
          uint32 m_DataLen_Severity_Module_Operation;
      };
      uint16  m_unLogMsgNumber;
  } LOG_HEADER;
#pragma pack()  
  
  LOG_QUEUE g_stLOGOut;
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /// LOGQ_Init
  /// @author NIVIS LLC, Avram Ionut
  /// @brief  Init the logger queue
  /// @remarks
  ///      Access level:  
  ///      Context: 
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  void LOGQ_Init(void)
  {
      g_stLOGOut.m_unLen = 0;
      g_stLOGOut.m_unLogMsgNumber = 0;    
      g_stLOGOut.m_pStart = g_stLOGOut.m_pBuf;  
  }
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /// SetLogThreshold
  /// @author NIVIS LLC, Avram Ionut
  /// @brief  set a new level for verbose
  /// @remarks lower = restrictive
  ///      Access level:  
  ///      Context: 
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  void SetLogThreshold (unsigned char p_ucNewLevel, unsigned char p_ucOfModule)
  {
      c_ucVerboseLOG[p_ucOfModule&SELECT_MODULE] = p_ucNewLevel;
      
  }
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /// Log
  /// @author NIVIS LLC, Avram Ionut
  /// @brief  add some bytes to queue if there is enough space 
  /// @remarks // ... = unsigned char, unsigned char*, ...
  ///      Access level:  
  ///      Context: 
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  //  void Log(unsigned char p_ucSeverity, unsigned char p_ucModule, unsigned char p_ucOperation, ...) 
  //replaced by
  void MyLog(unsigned long p_ucUnused_ucSeverity_ucModule_ucOperation, unsigned char p_ucLenght, ... )   
  // ... = unsigned char*, unsigned char, unsigned char*, ...
  {
      uint32 usDataLen;
      uint32 * pucData;
      uint16 usMsgNumber;
      uint8 * pEnd;
      unsigned long ulInterruptState;
  
      if( (p_ucUnused_ucSeverity_ucModule_ucOperation & SELECT_SEVERITY) > 
         c_ucVerboseLOG[(p_ucUnused_ucSeverity_ucModule_ucOperation>>8) & SELECT_MODULE]) {
          //message is not important and it will be discarded
          return;
      }
      //calculate log data length as char
      usDataLen = 0;
      pucData = (uint32*)&p_ucLenght;
      while(* (pucData)) {
          usDataLen += (* pucData) + 1; //count the length byte too
          pucData+=2;
      }
      //test 255 limit and test if enough space in buffer
      ulInterruptState = GlobalDisablePushIRQ();  //here is a space reserve in buffer if enough
          usMsgNumber = ++g_stLOGOut.m_unLogMsgNumber;     //increment message number for accepted message
          if( (usDataLen>255) || (g_stLOGOut.m_unLen + usDataLen > LOGQUEUE_BUFFER_SIZE - sizeof(LOG_HEADER))) {
              //there is no room in buffer for this message => message discarded
              GlobalRestorePopIRQ(ulInterruptState);        
              return;
          }
          pEnd = g_stLOGOut.m_pStart + g_stLOGOut.m_unLen;
          g_stLOGOut.m_unLen += (uint8)usDataLen + sizeof(LOG_HEADER);
      GlobalRestorePopIRQ(ulInterruptState);        
      //now get data in queue
      {
          uint8 * pucDataBlock;
          const uint8 * pRollover = g_stLOGOut.m_pBuf + LOGQUEUE_BUFFER_SIZE;
          LOG_HEADER LogHeader;
          unsigned char i;
          
          //LogHeader.m_ucDataLen = usDataLen;
          //LogHeader.m_unSeverity = p_ucSeverity;
          //LogHeader.m_unModule = p_ucModule;
          //LogHeader.m_unOperation = p_ucOperation;
          LogHeader.m_DataLen_Severity_Module_Operation = (p_ucUnused_ucSeverity_ucModule_ucOperation<<8) + usDataLen;
          LogHeader.m_unLogMsgNumber = usMsgNumber;
          for(i=0;i<sizeof(LOG_HEADER);i++) {
              if( pEnd >= pRollover ) pEnd -= LOGQUEUE_BUFFER_SIZE;
              *(pEnd++) = ((uint8*)&LogHeader)[i];               
          } 
          pucData = (uint32*)&p_ucLenght;
          while(i = *(pucData++)) {
              if( pEnd >= pRollover ) pEnd -= LOGQUEUE_BUFFER_SIZE;
              *(pEnd++) = i;               
              pucDataBlock = (uint8*)(*(pucData++));
              while(i--) {
                  if( pEnd >= pRollover ) pEnd -= LOGQUEUE_BUFFER_SIZE;
                  *(pEnd++) = *(pucDataBlock++);               
              }
          }
      }
  }
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /// Log
  /// @author NIVIS LLC, Avram Ionut
  /// @brief  add some bytes to queue if there is enough space 
  /// @remarks // ... = unsigned char, unsigned char*, ...
  ///      Access level:  
  ///      Context: 
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  void MyShortLog(unsigned long p_ucUnused_ucSeverity_ucModule_ucOperation, unsigned char p_ucNumberOfParams, ... )
  {
      uint16 usDataLen;
      uint32 * pucData;
      uint16 usMsgNumber;
      uint8 * pEnd;
      unsigned long ulInterruptState;
  
      if( (p_ucUnused_ucSeverity_ucModule_ucOperation & SELECT_SEVERITY) > 
         c_ucVerboseLOG[(p_ucUnused_ucSeverity_ucModule_ucOperation>>8) & SELECT_MODULE]) {
          //message is not important and it will be discarded
          return;
      }
      //calculate log data length as char
      usDataLen = p_ucNumberOfParams*3;
      //test 255 limit and test if enough space in buffer
      ulInterruptState = GlobalDisablePushIRQ();  //here is a space reserve in buffer if enough
          usMsgNumber = ++g_stLOGOut.m_unLogMsgNumber;     //increment message number for accepted message
          if( (usDataLen>255) || (g_stLOGOut.m_unLen + usDataLen > LOGQUEUE_BUFFER_SIZE - sizeof(LOG_HEADER))) {
              //there is no room in buffer for this message => message discarded
              GlobalRestorePopIRQ(ulInterruptState);        
              return;
          }
          pEnd = g_stLOGOut.m_pStart + g_stLOGOut.m_unLen;
          g_stLOGOut.m_unLen += (uint8)usDataLen + sizeof(LOG_HEADER);
      GlobalRestorePopIRQ(ulInterruptState);        
      //now get data in queue
      {
          const uint8 * pRollover = g_stLOGOut.m_pBuf + LOGQUEUE_BUFFER_SIZE;
          LOG_HEADER LogHeader;
          unsigned char i;
          
          //LogHeader.m_ucDataLen = usDataLen;
          //LogHeader.m_unSeverity = p_ucSeverity;
          //LogHeader.m_unModule = p_ucModule;
          //LogHeader.m_unOperation = p_ucOperation;
          LogHeader.m_DataLen_Severity_Module_Operation = (p_ucUnused_ucSeverity_ucModule_ucOperation<<8) + usDataLen;
          LogHeader.m_unLogMsgNumber = usMsgNumber;
          for(i=0;i<sizeof(LOG_HEADER);i++) {
              if( pEnd >= pRollover ) pEnd -= LOGQUEUE_BUFFER_SIZE;
              *(pEnd++) = ((uint8*)&LogHeader)[i];               
          } 
          pucData = (uint32*)&p_ucNumberOfParams;
          while(p_ucNumberOfParams--) {
              if( pEnd >= pRollover ) pEnd -= LOGQUEUE_BUFFER_SIZE;
              *(pEnd++) = 2;               
              if( pEnd >= pRollover ) pEnd -= LOGQUEUE_BUFFER_SIZE;
              *(pEnd++) = (*(++pucData)) & 0xFF;
              if( pEnd >= pRollover ) pEnd -= LOGQUEUE_BUFFER_SIZE;
              *(pEnd++) = ((*pucData)>>8) & 0xFF;;               
          }
      }
}

  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /// LOGQ_GetOutMsg
  /// @author NIVIS LLC, Avram Ionut
  /// @brief  retrieve some bytes from queue and returns the number of bytes retreived
  /// @remarks 
  ///      Access level:  
  ///      Context: 
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  uint16 LOGQ_GetOutMsg( uint8 * p_pMsg )
  {
      uint16 unMsgLen = 0;
      if( g_stLOGOut.m_unLen >= sizeof(LOG_HEADER) )
      {
          //there is a message in buffer => get it
          uint8 * pStart = g_stLOGOut.m_pStart;
          uint16 unMsgLenX = (unMsgLen = *(pStart) + sizeof(LOG_HEADER)); 
          while( unMsgLenX-- )
          {
                if( (++pStart) >= (g_stLOGOut.m_pBuf+LOGQUEUE_BUFFER_SIZE) ) pStart -= LOGQUEUE_BUFFER_SIZE;              
                *(p_pMsg++) = *(pStart);          
          }
          
          GlobalDisableIRQ();
              g_stLOGOut.m_unLen -= unMsgLen--;
              g_stLOGOut.m_pStart = pStart;
          GlobalEnableIRQ();    
      }
      return unMsgLen;
  }

#endif //_NIVIS_LOGGING_
