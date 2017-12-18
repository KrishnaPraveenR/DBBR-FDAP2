////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Eduard Erdei
/// Date:         March 2009
/// Description:  This file implements UAP functionalities
/// Changes:      Created
/// Revisions:    1.0
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "uap.h"
#include "uap_mo.h"
#include "uap_data.h"
#include "aslde.h"
#include "aslsrvc.h"
#include "dmap.h"
#include "dmap_co.h"
#include "../digitals.h"
#include "../CommonAPI/RadioApi.h"
#include "../COmmonAPI/Common_API.h"



// Rares: Some constants defining nr of 250ms loops. Note: due to unsigned char [0..255], max 255*250ms = 63.75 sec is possible
#define EVERY_250MSEC 1
#define EVERY_500MSEC 2*EVERY_250MSEC
#define EVERY_1SEC    2*EVERY_500MSEC
#define EVERY_5SEC    5*EVERY_1SEC
#define EVERY_10SEC   2*EVERY_5SEC
#define EVERY_15SEC   3*EVERY_5SEC
#define EVERY_30SEC   2*EVERY_15SEC
#define EVERY_60SEC   2*EVERY_30SEC



#if ( _UAP_TYPE != 0 )

#if ( _UAP_TYPE == UAP_TYPE_SIMPLE_API )

// SHHT 1x Temperature & Humidity Sensor
#include "../Sensors/Sensirion_SHT1x/SHT1x.h"
#include "../adc.h"

uint16 g_unAttributesMap;
uint32 g_ulDAQReadInterval = 0;
uint8  g_ucShtReadRequest;

    // How often to read ADC, SHT
    #define DATA_READ_INTERVAL_FAST  EVERY_5SEC
    #define DATA_READ_INTERVAL_SLOW  EVERY_15SEC
    #define DATA_READ_INTERVAL       DATA_READ_INTERVAL_SLOW
    
    
    // The period of time the LEDs indication is active (User presses the Wakeup&Status push-button to start the LEDs activity)
    #define LEDS_SIGNALING_PERIOD        10*EVERY_1SEC // This is the time period on which the LEDs signaling is active
    #define LEDS_SIGNALING_ALWAYS_ON     255          // Same as above but only for debug -> LEDs signaling is always active
    #define LEDS_SIGNALING_OFF           0            // All LEDs should be turned off.
    
    // The states signaled by the JOIN_STATUS led through solid on (0) or blinking at different rates (for example slow every second or fast every 1/2 second)
    //#define LED_JOIN_STATUS_JOINED_PERIOD   1                 // Solid ON
    #define LED_JOIN_STATUS_DISCOVERY_PERIOD  2*EVERY_1SEC      // Blink
    #define LED_JOIN_STATUS_JOINING_PERIOD    EVERY_500MSEC     // Blink
    
    // The states signaled by the EXTERNAL_COMM led
    //#define LED_EXTERNAL_COMM_NOT_CONNECTED   0
    //#define LED_EXTERNAL_COMM_CONNECTED       1               // Solid ON
    #define LED_EXTERNAL_COMM_COMMUNICATING   EVERY_500MSEC     // Blink
    #define EXTERNAL_COMM_NOTOK               0
    #define EXTERNAL_COMM_OK                  1
    #define EXTERNAL_COMM_DISABLED            2
    
    // The states signaled by the BATTERY_STATUS led
    //#define LED_BATTERY_STATUS_OK           1                // Solid ON
    #define LED_BATTERY_STATUS_NOTOK          EVERY_1SEC       // Blink
    #define LED_BATTERY_STATUS_OK_THRESHOLD   3.0f             // If the battery voltage is bellow this threshold, blink the LED
  
    uint8 g_ucRequestForLedsON;   // Activate/Deactivate LEDs Signaling
    uint8 g_ucExternalCommStatus; // Holds the status of communication with external device
    
#endif






////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Processes a read request in the application queue and passes it to the target object 
/// @param  p_pstContract - pointer to the contract structure 
/// @param  p_pReadReq - read request structure
/// @param  p_pIdtf - apdu structure
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void UAP_processReadRequest( DMO_CONTRACT_ATTRIBUTE * p_pstContract,
                             READ_REQ_SRVC * p_pReadReq,
                             APDU_IDTF *     p_pIdtf)

{
  READ_RSP_SRVC stReadRsp;
  uint8         aucRspBuff[MAX_GENERIC_VAL_SIZE]; // todo: check the size of the buffer

  stReadRsp.m_unDstOID = p_pReadReq->m_unSrcOID;
  stReadRsp.m_unSrcOID = p_pReadReq->m_unDstOID;
  stReadRsp.m_ucReqID  = p_pReadReq->m_ucReqID;
  
  stReadRsp.m_pRspData = aucRspBuff;
  stReadRsp.m_unLen = sizeof(aucRspBuff); // inform following functions about maximum available buffer size;
  
  stReadRsp.m_ucSFC = GetGenericAttribute( UAP_APP1_ID,
                                           p_pReadReq->m_unDstOID,    
                                           &p_pReadReq->m_stAttrIdtf, 
                                           stReadRsp.m_pRspData, 
                                           &stReadRsp.m_unLen );
  
  ASLSRVC_AddGenericObject(  &stReadRsp,
                             SRVC_READ_RSP,
                             0,                         // priority
                             p_pIdtf->m_ucDstTSAPID,    // SrcTSAPID
                             p_pIdtf->m_ucSrcTSAPID,    // DstTSAPID
                             0,
                             NULL,                      // EUI64 addr
                             p_pstContract->m_unContractID,
                             0 ); // p_unBinSize
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Processes a write request in the application queue and passes it to the target object 
/// @param  p_pstContract - pointer to the contract structure
/// @param  p_pWriteReq - write request structure
/// @param  p_pIdtf - apdu structure
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void UAP_processWriteRequest( DMO_CONTRACT_ATTRIBUTE * p_pstContract,
                              WRITE_REQ_SRVC * p_pWriteReq,
                              APDU_IDTF *      p_pIdtf)
{
  WRITE_RSP_SRVC stWriteRsp;  
  
  stWriteRsp.m_unDstOID = p_pWriteReq->m_unSrcOID;
  stWriteRsp.m_unSrcOID = p_pWriteReq->m_unDstOID;
  stWriteRsp.m_ucReqID  = p_pWriteReq->m_ucReqID;
  
  stWriteRsp.m_ucSFC = SetGenericAttribute( UAP_APP1_ID,
                                            p_pWriteReq->m_unDstOID,
                                            &p_pWriteReq->m_stAttrIdtf,
                                            p_pWriteReq->p_pReqData,
                                            p_pWriteReq->m_unLen );                 
                                             
  // todo: check if Add succsesfull, otherwise mark the processed apdu as NOT_PROCESSED
  ASLSRVC_AddGenericObject(  &stWriteRsp,
                             SRVC_WRITE_RSP,
                             0,
                             p_pIdtf->m_ucDstTSAPID,    // SrcTSAPID; faster than extracting it from contract
                             p_pIdtf->m_ucSrcTSAPID,    // DstTSAPID; faster than extracting it from contract  
                             0,
                             NULL,                      // EUI64 addr
                             p_pstContract->m_unContractID,
                             0 ); //p_unBinSize
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC,
/// @brief  notifies an internal or external UAP about a new or modified contract
/// @param  p_pContractRsp - contract response buffer
/// @param  p_ucContractRspLen - contract response len
/// @return -
/// @remarks
///      Access level: user level
///      Obs:
////////////////////////////////////////////////////////////////////////////////////////////////////
void UAP_NotifyContractResponse( const uint8 * p_pContractRsp, uint8 p_ucContractRspLen )
{
    // called every time a new contract is added to DMAP DMO contract table

    // Send Contract Data onto the application processor.
    SendMessage(STACK_SPECIFIC, ISA100_NOTIFY_ADD_CONTRACT, API_REQ, 0, p_ucContractRspLen, p_pContractRsp );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC,
/// @brief  notifies an internal or external UAP about a contract deletion from DMAP DMO contract table
/// @param  p_unContractID - UID of the deleted contract
/// @return -
/// @remarks
///      Access level: user level
///      Obs:
////////////////////////////////////////////////////////////////////////////////////////////////////
void UAP_NotifyContractDeletion(uint16 p_unContractID)
{
    // called every time a contract is deleted in DMAP DMO contract table
    // check here if this UAP is the owner of this contract and perform required operations

    // Send notification of contract deletion to the application processor
    SendMessage(STACK_SPECIFIC, ISA100_NOTIFY_DEL_CONTRACT, API_REQ, 0, 2, (uint8 *)&p_unContractID);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC,
/// @brief  Propagates a TL-ASL layer confirmation to UAPs
/// @param  p_pucData  - handle of message
/// @param  p_ucStatus - confirmation status
/// @return -
/// @remarks
///      Access level: user level
///      Context:
////////////////////////////////////////////////////////////////////////////////////////////////////
void UAP_DataConfirm( uint16 p_unAppHandle, uint8 p_ucStatus )
{
  // todo: handles are generated by ASL on 1 byte; separate somehow UAP handles from DMAP handles
//  if (SFC_NO_CONTRACT == p_ucStatus){
//      SendMessage(API_NACK, NACK_STACK_ERROR, FALSE, 0, 0, NULL);
//  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC,
/// @brief
/// @param  -
/// @return -
/// @remarks
///      Access level: user level
///      Obs:
////////////////////////////////////////////////////////////////////////////////////////////////////
void UAP_Init()
{
    // this function is not called yet from anywhere

    // TBD
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC,
/// @brief
/// @param  -
/// @return -
/// @remarks
///      Access level: user level
///      Obs:
////////////////////////////////////////////////////////////////////////////////////////////////////
void UAP_OnJoinReset(void)
{
    static const uint8 aConst[1] = {0};
    //called every time the ISA100 stack is reset
    SendMessage(STACK_SPECIFIC, ISA100_NOTIFY_JOIN, API_REQ, 0, 1, aConst);
    
#if ( _UAP_TYPE == UAP_TYPE_SIMPLE_API )
    
    g_unAttributesMap = 0;
    if( 1 == g_stDPO.m_stDAQPubCommEndpoint.m_ucCfgStatus )
    {
        //need to configure the UAP's Concentrator object
        CO_STRUCT* pstCrtCO = AddConcentratorObject(UAP_CO_OBJ_ID, UAP_APP1_ID);
        if( pstCrtCO )
        {
            //the appropriate CO element already exist or was just added - update/set the object attributes  
            
            memcpy(&pstCrtCO->m_stEndpoint, &g_stDPO.m_stDAQPubCommEndpoint, sizeof(g_stDPO.m_stDAQPubCommEndpoint));
            memcpy(pstCrtCO->m_aAttrDescriptor, g_stDPO.m_aAttrDescriptor, sizeof(g_stDPO.m_aAttrDescriptor));
            
            //find the number of attributes
            OBJ_ATTR_IDX_AND_SIZE* pstAttrDescriptor    = g_stDPO.m_aAttrDescriptor;
            OBJ_ATTR_IDX_AND_SIZE* pstEndAttrDescriptor = g_stDPO.m_aAttrDescriptor + MAX_PUBLISH_ITEMS;
            
            for(; (pstAttrDescriptor < pstEndAttrDescriptor) && pstAttrDescriptor->m_unSize; pstAttrDescriptor++)
            {
                if( pstAttrDescriptor->m_unAttrID < DIGITAL_DATA_ATTR_ID_OFFSET ) 
                {
                    g_unAttributesMap |= 1 << (pstAttrDescriptor->m_unAttrID);
                }
                else
                {
                    g_unAttributesMap |= 1 << (pstAttrDescriptor->m_unAttrID - DIGITAL_DATA_ATTR_ID_OFFSET + UAP_DATA_DIGITAL1);
                }                
            }
            pstCrtCO->m_ucPubItemsNo = pstAttrDescriptor - g_stDPO.m_aAttrDescriptor;            
            pstCrtCO->m_ucRevision = g_stDPO.m_ucUAPCORevision;
        }
    }
#endif    
}

void UAP_OnJoin(void)
{
    static const uint8 aConst[1] = {1};
    //called every time the ISA100 stack is reset
    SendMessage(STACK_SPECIFIC, ISA100_NOTIFY_JOIN, API_REQ, 0, 1, aConst );
}







#if ( _UAP_TYPE == UAP_TYPE_SIMPLE_API )


  void UAP_readDAQValues( void )
  {
      if( 1 == g_stDPO.m_stDAQPubCommEndpoint.m_ucCfgStatus )
      {
          if( g_ulDAQReadInterval )
          {
              g_ulDAQReadInterval--;
          }
          else
          {
              //read the analog/digitals channels from the DAQ
              uint8 aucMsgBuff[ANALOG_CH_NO + DIGITAL_CH_NO];
              uint8 ucIdx = 0;
              
              for( ; ucIdx < ANALOG_CH_NO; ucIdx++)
              {
                  aucMsgBuff[ucIdx] = ucIdx + ANALOG_DATA_ATTR_ID_OFFSET;
              }
              
              for( ; ucIdx < ANALOG_CH_NO + DIGITAL_CH_NO; ucIdx++)
              {
                  aucMsgBuff[ucIdx] = (ucIdx - ANALOG_CH_NO) + DIGITAL_DATA_ATTR_ID_OFFSET;
              }
              
              //send the API Read Data command
              if( NO_ERR == SendMessage(DATA_PASSTHROUGH, 
                                        READ_DATA_REQ, 
                                        API_REQ,   //response waiting
                                        0,      //Todo: Message ID is for later use
                                        sizeof(aucMsgBuff),      // Attribute ID(1 byte) * AttrNo    
                                        aucMsgBuff) )
              {
                  //update the sensor data read interval
                  if( g_stDPO.m_stDAQPubCommEndpoint.m_nPubPeriod & 0x8000 ) // negative period
                  {
                      if( g_stDPO.m_stDAQPubCommEndpoint.m_nPubPeriod >= -4 )
                          g_ulDAQReadInterval = -g_stDPO.m_stDAQPubCommEndpoint.m_nPubPeriod;
                  }
                  else
                  {
                      g_ulDAQReadInterval = g_stDPO.m_stDAQPubCommEndpoint.m_nPubPeriod * 2;  //in 1/4 sec units - 2 DAQ readings per publishing period
                  }
              
                  // LED_EXTERNAL_COMM status signaling
                  g_ucExternalCommStatus = EXTERNAL_COMM_OK;
              }
              else
              {
                  // LED_EXTERNAL_COMM status signaling
                  g_ucExternalCommStatus = EXTERNAL_COMM_NOTOK;
              }
          }
      }
      else
      {
          // LED_EXTERNAL_COMM status signaling
          g_ucExternalCommStatus = EXTERNAL_COMM_DISABLED;
      }
  }

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC,
/// @brief  
/// @param  -
/// @return -
/// @remarks
///      Access level: user level
///      Obs:
////////////////////////////////////////////////////////////////////////////////////////////////////
  void UAP_readLocalValues( void )
  {
     static uint8 s_ucLocalValuePeriod;
      
      if ( (s_ucLocalValuePeriod++) >= (DATA_READ_INTERVAL) ) // rollover every DATA_READ_INTERVAL seconds
     {      
          s_ucLocalValuePeriod = 0;
          
          // Raise flag to read the SHT. SHT is read every DATA_READ_INTERVAL second(s).
          g_ucShtReadRequest = 1;
          
          // Read battery voltage only once every DATA_READ_INTERVAL second(s)
          if( g_unAttributesMap & (1 << UAP_DATA_INPUT_BATTERY) )
          {
              UAPDATA_SetAnalogVal( UAP_DATA_INPUT_BATTERY, Read_Battery_Voltage() );
          }
      }
              
      // SHT Reading
      if( g_unAttributesMap & ((1 << UAP_DATA_INPUT_TEMP) | (1 << UAP_DATA_INPUT_HUMIDITY) | (1 << UAP_DATA_INPUT_DEWPOINT) ) )
      {
          if( g_ucShtReadRequest )  // Do I need to begin a new SHT Read cycle ?
              {
                  SHT1x_SENSOR_READINGS stSHT1xValues;
                  
                  if ( SHT1x_ReadData( &stSHT1xValues) == SHT_STATE_DONE )
                  {
                      UAPDATA_SetAnalogVal( UAP_DATA_INPUT_TEMP, stSHT1xValues.fTemperature );
                      UAPDATA_SetAnalogVal( UAP_DATA_INPUT_HUMIDITY, stSHT1xValues.fHumidity );
                      UAPDATA_SetAnalogVal( UAP_DATA_INPUT_DEWPOINT, stSHT1xValues.fDewPoint );
        
                  // SHT reading complete. Set flag to 0 to prevent another SHT reading until next cycle begins
                      g_ucShtReadRequest = 0;                      
                  }
              }
          }
  }

  void UAP_ckPushButtonAndLED( void )
  {
     static uint8 s_ucPushButtonTimeout;
      
     if( WAKEUP_STATUS_IS_PRESSED() )
     {
        if( s_ucPushButtonTimeout < (10*EVERY_1SEC) )
        {
            s_ucPushButtonTimeout++;
        }
        else // button pressed for more that 10sec
        {
            uint16 unTmp;
            uint8  ucTmp;
            g_stDPO.m_ucAllowProvisioning = 1;
            DPO_ResetToDefault( &unTmp, &ucTmp );
            
            s_ucPushButtonTimeout = 0;
        }
        
        // LEDs status here as specified in ERD document -> enable leds signaling for the specified period of time
        g_ucRequestForLedsON = LEDS_SIGNALING_PERIOD;
     }
     else // button released
     {
        s_ucPushButtonTimeout = 0;
        
        // LEDs status here as specified in ERD document
        if (g_ucRequestForLedsON > 0 && g_ucRequestForLedsON != LEDS_SIGNALING_ALWAYS_ON) // The LEDs signaling is active all the time if LEDS_SIGNALING_ALWAYS_ON is used. Debug only.
          g_ucRequestForLedsON--;
     }
     }
    



  void UAP_ledsSignaling( void )
  {
     if (g_ucRequestForLedsON)
     {
         // 
         // LED_JOIN_STATUS
         //
         if ( g_ucJoinStatus == DEVICE_DISCOVERY )  /* Field Device in discovery state */
         {
            if ( g_ucRequestForLedsON % LED_JOIN_STATUS_DISCOVERY_PERIOD == 0 ) // Blink slower -> Field device in discovery mode
            {
               LED_JOIN_STATUS_TOGGLE();
            }
         }
         else if ( g_ucJoinStatus == DEVICE_JOINED ) /* Field Device Joined */
         {
           LED_JOIN_STATUS_ON(); 
         }
         else //if () /* Field Device in Joining Mode -> all the other states that are not discovery or joined*/
         {
            if ( g_ucRequestForLedsON % LED_JOIN_STATUS_JOINING_PERIOD == 0 )   // Blink faster -> Field device in joining mode
            {
               LED_JOIN_STATUS_TOGGLE();
            }
         }          
           
         // 
         // LED_EXTERNAL_COMM
         //
         if ( g_ucExternalCommStatus == EXTERNAL_COMM_OK ) /* Field Device established communication with external device */
         {
            LED_EXTERNAL_COMM_ON();
         }
         else if ( g_ucExternalCommStatus == EXTERNAL_COMM_NOTOK ) /* Field Device in progress of communicating with external device ?! */
         {
            if ( g_ucRequestForLedsON % LED_EXTERNAL_COMM_COMMUNICATING == 0 )
            {
               LED_EXTERNAL_COMM_TOGGLE();
            }
         } 
         else // if ( g_ucExternalCommStatus == EXTERNAL_COMM_DISABLED )  // Communicate with external device is disabled ?!
         {
           LED_EXTERNAL_COMM_OFF();
         }
    
         // 
         // LED_BATTERY_STATUS
         //
         // Field Device's battery voltage is equal or above the minimum level
         //
         if (g_afAnalogData[ UAP_DATA_INPUT_BATTERY-UAP_DATA_ANALOG1 ] >= LED_BATTERY_STATUS_OK_THRESHOLD ) 
         {
            LED_BATTERY_STATUS_ON();
         }
         else /* Field Device's battery voltage is lower than the minimum level */
         {
            if ( g_ucRequestForLedsON % LED_BATTERY_STATUS_NOTOK == 0 )   // Blink faster -> device in joining mode
            {
               LED_BATTERY_STATUS_TOGGLE();
            }
         }
     }
     else
     {
        // Turn OFF all the LEDs
        LEDS_OFF();
     }
  }

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC,
/// @brief  UAP periodic task
/// @param  -
/// @return -
/// @remarks
///      Access level: user level
///      Obs:
////////////////////////////////////////////////////////////////////////////////////////////////////
void UAP_MainTask()
{
    // implement here any required periodic functionalities for this UAP
    // called every main loop
  
#if ( _UAP_TYPE == UAP_TYPE_SIMPLE_API )
    UAP_readDAQValues();      
    UAP_readLocalValues();
    UAP_ckPushButtonAndLED();
    UAP_ledsSignaling();
  
    // check if there is an incoming APDU that has to be processed by UAP
    APDU_IDTF stAPDUIdtf;
    const uint8 * pAPDUStart = ASLDE_GetMyAPDU( UAP_APP1_ID, &stAPDUIdtf );
    
    
    if( pAPDUStart )
    {
        GENERIC_ASL_SRVC  stGenSrvc;
        const uint8 *     pNext = pAPDUStart;
        
        //check if the appropriate contract exist
        DMO_CONTRACT_ATTRIBUTE * pstContract = DMO_GetContract( stAPDUIdtf.m_aucAddr,
                                                                ISA100_START_PORTS + stAPDUIdtf.m_ucDstTSAPID,
                                                                ISA100_START_PORTS + stAPDUIdtf.m_ucSrcTSAPID,                        
                                                                SRVC_APERIODIC_COMM );
      
        if( !pstContract )
        {
            //request a new contract for the UAP
            DMO_CONTRACT_BANDWIDTH stBandwidth;
 
            stBandwidth.m_stAperiodic.m_nComittedBurst  = 1;
            stBandwidth.m_stAperiodic.m_nExcessBurst    = 2;
            stBandwidth.m_stAperiodic.m_ucMaxSendWindow = 1; //not relevant for the moment 

            DMO_RequestNewContract( stAPDUIdtf.m_aucAddr,
                                    ISA100_START_PORTS + stAPDUIdtf.m_ucDstTSAPID, // m_unDstTLPort,
                                    ISA100_START_PORTS + stAPDUIdtf.m_ucSrcTSAPID, // m_unSrcTLPort,
                                    0xFFFFFFFF,                                    // contract life
                                    SRVC_APERIODIC_COMM,                           // p_ucSrcvType,
                                    DMO_PRIORITY_BEST_EFFORT,                      // p_ucPriority,
                                    MAX_APDU_SIZE,                                 // p_unMaxAPDUSize,
                                    0,                                             //  p_ucReliability,
                                    &stBandwidth );
            return;
        }
            
        while ( pNext = ASLSRVC_GetGenericObject( pNext,
                                                 stAPDUIdtf.m_unDataLen - (pNext - pAPDUStart),
                                                 &stGenSrvc,
                                                 stAPDUIdtf.m_aucAddr)
               )
        {
            switch ( stGenSrvc.m_ucType )
            {
                case SRVC_READ_REQ  : UAP_processReadRequest( pstContract, &stGenSrvc.m_stSRVC.m_stReadReq, &stAPDUIdtf ); break;
                case SRVC_WRITE_REQ : UAP_processWriteRequest( pstContract, &stGenSrvc.m_stSRVC.m_stWriteReq, &stAPDUIdtf ); break;
                default: return;
            }        
        }

        ASLDE_DeleteAPDU( (uint8*)pAPDUStart ); // todo: check if always applicable       
    }
    
#endif    
}


#endif // #if ( _UAP_TYPE != 0 )
