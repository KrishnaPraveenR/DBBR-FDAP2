/***************************************************************************************************
 * Name:         Common_API.h
 * Author:       NIVIS LLC, John Chapman and Kedar Kapoor
 * Date:         July 2008
 * Description:  This file has the SendMessage routine for the RF Modem side of the common API.
 * 				 This also is the API for the single processor solution.
 * Changes:
 * Revisions:
 ****************************************************************************************************/

#ifndef _NIVIS_COMMON_API_H_
#define _NIVIS_COMMON_API_H_

/*===========================[ public includes ]=============================*/
#include "../typedef.h"
#include "RadioApi.h"
#include "../ISA100/dmap_utils.h"

#if ( _UAP_TYPE != 0 )
/*==========================[ public datatypes ]=============================*/
enum SEND_ERROR_CODE {
    NO_ERR = 0,
    DATA_FOR_PROCESSING,

    ER_QUEUE_FULL,
    ER_LINK_UNAVAILABLE, // Recommend querying STACK_SPECIFIC attributes for link outage cause
    ER_UNSUPPORTED_DATA_SIZE, // i.e. max string size is 150 and 160 byte string or an empty string is sent
    ER_UNKNOWN_DATA_TYPE, // Datagram type is not supported
    ER_PARAM, // There was an error with one of the parameters
    ER_API_FAILED,
    ER_API_NOT_INITIALIZED,

    NUM_ITEMS_SEND_ERROR_CODE
};

#define API_REQ FALSE
#define API_RSP TRUE

enum STACK_SPECIFIC_TYPES {
    ISA100_ITEMS = 10,
    // ISA100 100
    ISA100_LINK_STATUS,
    ISA100_LINK_QUALITY,
    ISA100_JOIN_STATUS,
    ISA100_SECURITY_LEVEL,
    ISA100_GET_TAI_TIME,            // Time is in seconds
    ISA100_POWER_TYPE,              
    ISA100_POWER_LEVEL,             
    ISA100_UAP_IDS,
    ISA100_ROUTING_DEVICE,
    ISA100_GET_ADDR,

    // ISA100 Contract
    ISA100_CONTRACT_REQUEST,
    ISA100_NOTIFY_ADD_CONTRACT,
    ISA100_NOTIFY_DEL_CONTRACT,
    ISA100_CONTRACT_TERMINATE,

    // ISA100 TL
    ISA100_TL_APDU_REQUEST,
    ISA100_TL_APDU_INDICATE,

    // ISA100 Notifications
    ISA100_NOTIFY_JOIN,
    ISA100_GET_INITIAL_INFO, 
    ISA100_SET_DPO, 

    NIVIS_ITEMS = 100,
    NIVIS_LINK_STATUS,
    NIVIS_LINK_QUALITY,

    HART_ITEMS = 150,
    HART_LINK_STATUS,
    HART_LINK_QUALITY,

    NUM_ITEMS_STACK_SPECIFIC_TYPES = 255
};

enum API_COMMAND_TYPES {
    API_HW_PLATFORM=1,
    API_FW_VERSION,
    API_MAX_BUFFER_SIZE, // This is platform specific and may vary depending on platform resources.
    API_MAX_SPI_SPEED,
    API_UPDATE_SPI_SPEED,
    API_MAX_UART_SPEED,
    API_UPDATE_UART_SPEED,
    API_HEARTBEAT_FREQ, // Frequency the RF processor should verify communications with application
    // processor.  This can be every 1, 2,4,8,12 or 24 hours.  Default = 1 Hour
    API_HEARTBEAT,

    NUM_ITEMS_API_COMMAND_TYPES
};

typedef enum {
    API_STATUS_INIT,
    API_STATUS_RUN
} API_STATUS_TYPE; 

/*==========================[ external attributes ]==============================*/

// Power attributes for use by system manager for routing algorithms.
extern const DMAP_FCT_STRUCT c_aApiReadWriteTable[NUM_ITEMS_API_COMMAND_TYPES];

/*==========================[ public functions ]==============================*/

////////////////////////////////////////////////////////////////////////////////
// Name: SendMessage
// Author: NIVIS LLC, John Chapman and Kedar Kapoor
// Description: method used to send message via common stack API.  This method will add the message
//              to a queue for transmission and assign a MSG_ID.  This will copy the data into
//              a stack buffer before sending it to the next method for processing.
// Parameters:
//        MSG_CLASS             - the type of message.  This directs SendMessage on how to handle the message buffer.
//        MSG_TYPE              - sub-type of the MSG_CLASS.  This may not be used for all MSG_CLASSes.
//        uint8                 - RequestResponse 0 = FALSE, 1 = TRUE
//        MSG_ID          *     - Empty if sending unsolicited message, MSG_ID if responding to a message
//                                  If sending message, the next indication a message is available to process will
//                                  contain the ID for this message.  This is for future use and not presently processed.
//        uint8 bufSize         - the size of the buffer
//        uint8 *buf            - the message to be sent
//
// Return: SEND_ERR_NO   - Error when handling the message.  Will return NO_ERR if message successfully handled.
////////////////////////////////////////////////////////////////////////////////
enum SEND_ERROR_CODE SendMessage(enum MSG_CLASS p_MsgClass,
                                 uint8 p_MsgType,
                                 uint8 p_ucAckRequest,
                                 uint8 p_ucMSG_ID,
                                 uint8 p_BuffSize,
                           const uint8 *p_pMsgBuff);

void API_OnRcvMsg( ApiMessageFrame * p_pMessageFrame );

#endif // _UAP_TYPE

#if         (DEVICE_TYPE == DEV_TYPE_MC13225)
    #if ( SPI1_MODE != NONE )
          #include "../spi.h"
          #define DAQ_BUFFER_SIZE   SPI_BUFFER_SIZE
          #define API_PHY_Write(p_pBuff,p_unBuffLen) SPI_WriteBuff(p_pBuff,p_unBuffLen)
          #define API_PHY_Read(p_pBuff,p_unBuffSize) SPI_ReadBuff(p_pBuff,p_unBuffSize)
          #define API_PHY_SetSpeed(p_Speed)          SPI_UpdateSpiSpeed(p_Speed)
    #elif ( UART2_MODE != NONE )
          #include "../uart2.h"
          #define DAQ_BUFFER_SIZE   UART2_BUFFER_SIZE
          #define API_PHY_Write(p_pBuff,p_unBuffLen) UART2_WriteBuff(p_pBuff,p_unBuffLen)
          #define API_PHY_Read(p_pBuff,p_unBuffSize) UART2_ReadBuff(p_pBuff,p_unBuffSize)
          #define API_PHY_SetSpeed(p_Speed)          UART2_UpdateSpeed(p_Speed)
    #else
        //let them rise errs
    #endif

#elif       (DEVICE_TYPE == DEV_TYPE_MSP430F2618) // raptor modem
      #include "../spi1.h"

      #define DAQ_BUFFER_SIZE   SPI_BUFFER_SIZE
      #define API_PHY_Write(p_pBuff,p_unBuffLen) SPI1_WriteBuff(p_pBuff,p_unBuffLen)
      #define API_PHY_Read(p_pBuff,p_unBuffSize) SPI1_ReadBuff(p_pBuff,p_unBuffSize)
      #define API_PHY_SetSpeed(p_Speed)          SPI1_UpdateSpiSpeed(p_Speed)

#elif       (DEVICE_TYPE == DEV_TYPE_AP91SAM7X512)

      #define API_PHY_Write(p_pBuff,p_unBuffLen)
      #define API_PHY_Read(p_pBuff,p_unBuffSize) 0
      #define API_PHY_SetSpeed(p_Speed)          SFC_FAILURE
#else
      #error  "Unknown device, please select apropiate API PHY interface"
#endif

#endif // _NIVIS_COMMON_API_H_
