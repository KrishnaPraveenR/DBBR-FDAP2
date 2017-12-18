  #include <string.h>
  #include "logger_ack.h"
  #include "../system.h"
  
//////////////////////////////////////////////////////////////////////////////////////
#define LOG_WCI_BUFFER_SIZE    (10*1024)

typedef struct
{
  uint32  m_ulMsgID;
  uint8 * m_pucEndBuf;
  uint8   m_aucBuf[LOG_WCI_BUFFER_SIZE];
} LOG_WCI_QUEUE; 

LOG_WCI_QUEUE g_stWCIQueue;

///////////////////////////////////////////////////////////////////////////////////
// Name: WCI_InitQueue 
// Author : Nivis LLC, Dorin Muica
// Description: Init WCI queue 
// Parameters: none
// Return: none       
// Remarks :
// Access level :
///////////////////////////////////////////////////////////////////////////////////
void WCI_InitQueue(void)
{
  g_stWCIQueue.m_pucEndBuf = g_stWCIQueue.m_aucBuf;
}

///////////////////////////////////////////////////////////////////////////////////
// Name: WCI_AddMessage
// Author : Nivis LLC, Dorin Muica
// Description: Add a WCI message at end of  the queue 
// Parameters: p_unLength = data length, p_pucSourceData = pointer to data block
// Return: none           
// Remarks :  
// Access level :
///////////////////////////////////////////////////////////////////////////////////

// WCI Queue format:
//  ___________________________________________________________________________
// | Message #1 Length | Message #1 Data | Message #2 Length | Message #2 Data |...
// | __________________________________________________________________________|
// |     4 bytes       |  "Length" bytes |     4 bytes       |  "Length" bytes |... 
// | __________________________________________________________________________|
void WCI_AddMessage(uint16 p_unLength, const uint8 *p_pucSourceData )
{
  MONITOR_ENTER();
  
  uint8 * pNewEnd = g_stWCIQueue.m_pucEndBuf + p_unLength + 4;
 
  if( pNewEnd < g_stWCIQueue.m_aucBuf + sizeof(g_stWCIQueue.m_aucBuf) ) 
  {
  
      g_stWCIQueue.m_pucEndBuf[0] = (uint8) p_unLength;
      g_stWCIQueue.m_pucEndBuf[1] = (uint8)(p_unLength >> 8);
      g_stWCIQueue.m_pucEndBuf[2] = 0;
      g_stWCIQueue.m_pucEndBuf[3] = 0;
      memcpy( g_stWCIQueue.m_pucEndBuf+4, p_pucSourceData, p_unLength );
      
      g_stWCIQueue.m_pucEndBuf = pNewEnd;
  }
  MONITOR_EXIT();
}

///////////////////////////////////////////////////////////////////////////////////
// Name: GetWCIHeadMessage
// Author : Dorin Muica
// Description: Gets message from the WCI queue
// Parameters: void
// Return: structure containing pointer to message and message length           
// Remarks :  
// Access level :
///////////////////////////////////////////////////////////////////////////////////
const uint8 *  WCI_GetHeadMessage( void )
{    
  if( g_stWCIQueue.m_aucBuf == g_stWCIQueue.m_pucEndBuf )
      return NULL;
  
  return g_stWCIQueue.m_aucBuf;
}

void WCI_DeleteHeadMessage(void)
{
    if( g_stWCIQueue.m_aucBuf != g_stWCIQueue.m_pucEndBuf )
    {        
        uint16 unMsgLen = *(uint16*)g_stWCIQueue.m_aucBuf + 4;
        
        g_stWCIQueue.m_ulMsgID ++;
        
        MONITOR_ENTER();
        g_stWCIQueue.m_pucEndBuf -= unMsgLen;
        if(  g_stWCIQueue.m_aucBuf != g_stWCIQueue.m_pucEndBuf )
        {
            memcpy( g_stWCIQueue.m_aucBuf, 
                   g_stWCIQueue.m_aucBuf + unMsgLen, 
                   g_stWCIQueue.m_pucEndBuf - g_stWCIQueue.m_aucBuf ); 
        }                
        MONITOR_EXIT();
    }
}

uint32 WCI_GetMessageId(void)
{
    return g_stWCIQueue.m_ulMsgID;
}

