////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Mircea Vlasin
/// Date:         June 2009
/// Description:  This file holds common definitions of a data aquisition object
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NIVIS_UAP_DATA_H_
#define _NIVIS_UAP_DATA_H_

#include "../typedef.h"
#include "dmap_utils.h"

#define ANALOG_CH_NO           4
#define LOCAL_ANALOG_CH_NO     4
#define DIGITAL_CH_NO          4

enum
{
    UAP_DATA_RESERVED,
    UAP_DATA_ANALOG1,
    UAP_DATA_ANALOG2,
    UAP_DATA_ANALOG3,
    UAP_DATA_ANALOG4,
    UAP_DATA_INPUT_TEMP,
    UAP_DATA_INPUT_HUMIDITY,
    UAP_DATA_INPUT_DEWPOINT,
    UAP_DATA_INPUT_BATTERY,
    UAP_DATA_DIGITAL1,
    UAP_DATA_DIGITAL2,
    UAP_DATA_DIGITAL3,
    UAP_DATA_DIGITAL4,

    UAP_DATA_ATTR_NO                  
}; // UAPDATA_ATTRIBUTES

#if ( _UAP_TYPE != 0 )

    extern const DMAP_FCT_STRUCT c_aUapDataAttributes[UAP_DATA_ATTR_NO];
    #define UAPDATA_Read(p_unAttrID,p_punBufferSize,p_pucRspBuffer) \
                DMAP_ReadAttr(p_unAttrID,p_punBufferSize,p_pucRspBuffer,c_aUapDataAttributes,UAP_DATA_ATTR_NO)
    
    #define UAPDATA_Write(p_unAttrID,p_ucBufferSize,p_pucBuffer)   \
                DMAP_WriteAttr(p_unAttrID,p_ucBufferSize,p_pucBuffer,c_aUapDataAttributes,UAP_DATA_ATTR_NO)
    
    extern fp32    g_afAnalogData[ANALOG_CH_NO + LOCAL_ANALOG_CH_NO];
    extern uint8   g_aucDigitalData[DIGITAL_CH_NO];
    
    #pragma inline 
    void UAPDATA_SetAnalogVal( uint8 p_ucAttrNo, fp32 p_fValue )
    {
      if( (p_ucAttrNo-UAP_DATA_ANALOG1)  <= sizeof(g_afAnalogData) / sizeof(g_afAnalogData[0]) ) 
      {
         g_afAnalogData[p_ucAttrNo-UAP_DATA_ANALOG1] = p_fValue;
      }
    }

    #pragma inline 
    void UAPDATA_SetDigitalVal( uint8 p_ucAttrNo, uint8 p_ucValue )
    {
      if( (p_ucAttrNo-UAP_DATA_DIGITAL1) <= sizeof(g_aucDigitalData) / sizeof(g_aucDigitalData[0]) ) 
      {
         g_aucDigitalData[p_ucAttrNo-UAP_DATA_DIGITAL1] = p_ucValue;
      }
    }
    
#else
    #define UAPDATA_Read(...) SFC_INVALID_OBJ_ID
    #define UAPDATA_Write(...) SFC_INVALID_OBJ_ID
#endif  // #if ( _UAP_TYPE != 0 )

#endif // _NIVIS_UAP_DATA_H_
