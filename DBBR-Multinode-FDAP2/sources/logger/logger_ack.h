#ifndef _NIVIS_LOG_ACK_H_
#define _NIVIS_LOG_ACK_H_

#include "err.h"

  #include "../typedef.h"
    
  void WCI_AddMessage(uint16 p_unLength, const uint8 * p_pucSourceData );
  const uint8 *  WCI_GetHeadMessage( void );
  uint32 WCI_GetMessageId(void);
  void WCI_DeleteHeadMessage(void);
  
  void WCI_InitQueue(void);        
    
  #define WCI_Log(...)  
  #define LOGAckQ_Init(...)
  #define SetLogAckThreshold(...)
  #define LogAck(...)
  #define LogAckShort(...)
  #define LOGAckQ_GetOutMsg(...) 0
  #define LOGAckQ_AckOutMsg(...)
  #define LogAckKick()
  #define LogAckMsgToSend() 0    

#endif //_NIVIS_LOG_ACK_H_
