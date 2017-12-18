////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Mircea Vlasin
/// Date:         June 2009
/// Description:  This file implements UAP sensor/actuator data aquisition object functionalities
/// Changes:      Created
/// Revisions:    1.0
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "uap_data.h"
#include "uap.h"
#include "sfc.h"
#include "../CommonAPI/RadioApi.h"
#include "../CommonAPI/Common_API.h"

#if ( _UAP_TYPE != 0 )

fp32    g_afAnalogData[ANALOG_CH_NO + LOCAL_ANALOG_CH_NO]   = {0, 0, 0, 0, 0, 0, 0, 0};
uint8   g_aucDigitalData[DIGITAL_CH_NO] = {FALSE, FALSE, FALSE, FALSE};

static void UAP_DATA_WriteFp32(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize);
static void UAP_DATA_WriteUint8(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize);

const DMAP_FCT_STRUCT c_aUapDataAttributes[UAP_DATA_ATTR_NO] = {
   {  0, 0,                            DMAP_EmptyReadFunc,  NULL },
   {  ATTR_CONST(g_afAnalogData[0]),   DMAP_ReadUint32,  UAP_DATA_WriteFp32 },
   {  ATTR_CONST(g_afAnalogData[1]),   DMAP_ReadUint32,  UAP_DATA_WriteFp32 },
   {  ATTR_CONST(g_afAnalogData[2]),   DMAP_ReadUint32,  UAP_DATA_WriteFp32 },
   {  ATTR_CONST(g_afAnalogData[3]),   DMAP_ReadUint32,  UAP_DATA_WriteFp32 },
   {  ATTR_CONST(g_afAnalogData[4]),   DMAP_ReadUint32,  NULL },
   {  ATTR_CONST(g_afAnalogData[5]),   DMAP_ReadUint32,  NULL },
   {  ATTR_CONST(g_afAnalogData[6]),   DMAP_ReadUint32,  NULL },
   {  ATTR_CONST(g_afAnalogData[7]),   DMAP_ReadUint32,  NULL },
   {  ATTR_CONST(g_aucDigitalData[0]), DMAP_ReadUint8,   UAP_DATA_WriteUint8 },
   {  ATTR_CONST(g_aucDigitalData[1]), DMAP_ReadUint8,   UAP_DATA_WriteUint8 },
   {  ATTR_CONST(g_aucDigitalData[2]), DMAP_ReadUint8,   UAP_DATA_WriteUint8 },
   {  ATTR_CONST(g_aucDigitalData[3]), DMAP_ReadUint8,   UAP_DATA_WriteUint8 }
};


void UAP_writeDataToDAQ( const uint8 * p_pBuf )
{
    //send the API Write command
    SendMessage(DATA_PASSTHROUGH, 
                WRITE_DATA_REQ, 
                API_REQ,  //no response waiting
                0,      // Todo: Message ID is for later use
                5,      // Attribute ID(1 byte) + Attr Value(4 bytes)    
                p_pBuf);
}

static void UAP_DATA_WriteFp32(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize)
{
    uint32 ulTemp;
    
    uint8 aucMsgBuff[1+4];
    
    aucMsgBuff[0] = (fp32*)p_pValue - (fp32*)g_afAnalogData;
    aucMsgBuff[0] += ANALOG_DATA_ATTR_ID_OFFSET;    //attribute ID
        
    memcpy(aucMsgBuff+ 1, p_pBuf, 4);     //big endian order
    
    ((uint8*)p_pValue)[0] = aucMsgBuff[4]; // little endian order
    ((uint8*)p_pValue)[1] = aucMsgBuff[3];
    ((uint8*)p_pValue)[2] = aucMsgBuff[2];
    ((uint8*)p_pValue)[3] = aucMsgBuff[1];
    
    UAP_writeDataToDAQ( aucMsgBuff );
}


static void UAP_DATA_WriteUint8(void * p_pValue, const uint8 * p_pBuf, uint8 p_ucSize)
{
    uint8 aucMsgBuff[1+4];
    
    memset( aucMsgBuff, 0, sizeof( aucMsgBuff ) );   
    aucMsgBuff[0] = (uint8*)p_pValue - g_aucDigitalData;
    aucMsgBuff[0] += DIGITAL_DATA_ATTR_ID_OFFSET;    //attribute ID
    
    aucMsgBuff[4] = *p_pBuf; 
        
    *(uint8*)p_pValue = *p_pBuf;
 
    //send the API Write command
    SendMessage(DATA_PASSTHROUGH, 
                WRITE_DATA_REQ, 
                API_REQ,  //no response waiting
                0,      // Todo: Message ID is for later use
                sizeof(aucMsgBuff),      // Attribute ID(1 byte) + Attr Value(4 bytes)    
                aucMsgBuff);

    UAP_writeDataToDAQ( aucMsgBuff );
}


#endif // #if ( _UAP_TYPE != 0 )
