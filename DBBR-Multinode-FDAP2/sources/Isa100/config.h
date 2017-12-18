///////////////////////////////////////////////////////////////////////////////////////////////////
// Name:         config.h
// Author:       Nivis, LLC
// Date:         March 2008
// Description:  Define main ISA config parameters for different layers
// Changes:
// Revisions:
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NIVIS_ISA100_CONFIG_H_
#define _NIVIS_ISA100_CONFIG_H_

#include "../system.h"

  // DMAP DMO initial default defines
  // defines for inital default dmo attributes values
  #define DMO_DEF_DMAP_STATE          1
  #define DMO_DEF_CONTRACT_TOUT       30
  #define DMO_DEF_MAX_CLNT_SRV_RETRY  3
  #define DMO_DEF_MAX_RETRY_TOUT_INT  62 //30  //64 sec - maximum acceptable by TL layer validation(message TTL)
  #define DMO_DEF_WARM_RESTART_TOUT   60

  //#define NL_FRAG_SUPPORT               // fragmentation defined
  #define CYCLIC_GRAPH_RETRY

  // contract establishment timeouts
  #define CONTRACT_WAIT_TIMEOUT   (DMO_DEF_MAX_CLNT_SRV_RETRY * DMO_DEF_MAX_RETRY_TOUT_INT)   // units in seconds     
  #define CONTRACT_RETRY_TIMEOUT  (DMO_DEF_MAX_CLNT_SRV_RETRY * DMO_DEF_MAX_RETRY_TOUT_INT)   // units in seconds

// Application configuration
  #if (DEVICE_TYPE == DEV_TYPE_MC13225) // 96k of RAM
      #define COMMON_APDU_BUF_SIZE      2000
      #define MAX_SIMULTANEOUS_JOIN_NO  1//3
      #define ARMO_QUEUE_SIZE           400
  #elif defined( BACKBONE_SUPPORT )
      #define COMMON_APDU_BUF_SIZE      2000
      #define MAX_SIMULTANEOUS_JOIN_NO  1//2
      #define ARMO_QUEUE_SIZE           600
  #elif defined( ROUTING_SUPPORT )
      #define COMMON_APDU_BUF_SIZE      800
      #define MAX_SIMULTANEOUS_JOIN_NO  1
      #define ARMO_QUEUE_SIZE           100
  #else
      #define COMMON_APDU_BUF_SIZE  700
      #define ARMO_QUEUE_SIZE           200
  #endif 
  
  #define MIN_JOIN_FREE_SPACE       600 // minimum rx/tx APDU buffer to accept a join request

  /* time to live (seconds) of an APDU in the queue             */
  #define APP_QUE_TTL_SEC            DMO_DEF_MAX_RETRY_TOUT_INT // g_unDmapRetryTout * DMO_DEF_MAX_CLNT_SRV_RETRY

  #define DEFAULT_UAP_RETRY_TIMEOUT  DMO_DEF_MAX_RETRY_TOUT_INT // not specified by standard    
  #define DEFAULT_UAP_RETRY_NO    3

// Application sublayer configuration
    #define MAX_PARAM_SIZE        130
    #define MAX_RSP_SIZE          220
    #define MAX_GENERIC_VAL_SIZE  100
  
    #ifdef NL_FRAG_SUPPORT
        #define MAX_APDU_SIZE     304
    #else       
        #define MAX_APDU_SIZE      84//78
    #endif


// Network layer configuration
    #define MAX_DATAGRAM_SIZE     (MAX_APDU_SIZE + 32)  // 400 //1280  // datagram size
    #define MAX_RCV_TIME_MULTIPLE_PK  10 // seconds // timeout for a multipacket RX message


// DMAP layer configuration
    #define SM_LINK_RETRY_TIMEOUT     30 // 60 //seconds
    #define SM_LINK_CK_TIMEOUT        10 //seconds
    #define SM_LINK_TIMEOUT           (SM_LINK_CK_TIMEOUT+SM_LINK_RETRY_TIMEOUT)


// Data Link Layer configuration

#if defined( BACKBONE_SUPPORT )
    #if (DEVICE_TYPE == DEV_TYPE_MC13225) // 96k of code + RAM

        #define DLL_MAX_SUPERFRAMES_NO  10  // minimum 10 dlmo superframes required by PICS
        #define DLL_MAX_CHANNELS_NO     10  // minimum 10 dlmo hopping sequences required by PICS
        #define DLL_MAX_TIMESLOTS_NO    10  // minimum 10 dlmo timeslot templates required by PICS
        #define DLL_MAX_LINKS_NO        80  // minimum 30 dlmo links required by PICS
        #define DLL_MAX_GRAPHS_NO       100  // minimum 14 dlmo graphs required by PICS
        #define DLL_MAX_ROUTES_NO       200  // minimum 14 dlmo routes required by PICS
        #define DLL_MAX_NEIGHBORS_NO    64  // minimum  8 dlmo neighbors required by PICS

        /* Warning: Always consider both values when modifying one of the following two values!!! */
        #define DLL_MSG_QUEUE_SIZE_MAX        32 // minimum 3 entries in DLL message queue in PICS   
        #define DLL_MSG_QUEUE_ECN_THRESHOLD   26 // not specified in ISA100; 
    
        #define DLME_MSG_QUEUE_LENGTH  1024        
    
        #define MAX_ROUTES_NO        8
        #define MAX_CONTRACT_NO      8
        #define MAX_ADDR_TRANSLATIONS_NO (DLL_MAX_NEIGHBORS_NO+8)

        #define DUPLICATE_ARRAY_SIZE     32
    
    #elif (DEVICE_TYPE == DEV_TYPE_AP91SAM7X512) // 512k of code + 128k RAM
    
        #define DLL_MAX_SUPERFRAMES_NO  10  // minimum 10 dlmo superframes required by PICS
        #define DLL_MAX_CHANNELS_NO     10  // minimum 10 dlmo hopping sequences required by PICS
        #define DLL_MAX_TIMESLOTS_NO    10  // minimum 10 dlmo timeslot templates required by PICS
        #define DLL_MAX_LINKS_NO        80  // minimum 30 dlmo links required by PICS 
        #define DLL_MAX_GRAPHS_NO       100  // minimum 14 dlmo graphs required by PICS
        #define DLL_MAX_ROUTES_NO       200  // minimum 14 dlmo routes required by PICS
        #define DLL_MAX_NEIGHBORS_NO    64  // minimum  8 dlmo routes required by PICS

        /* Warning: Always consider both values when modifying one of the following two values!!! */
        #define DLL_MSG_QUEUE_SIZE_MAX        32  // minimum 3 entries in DLL message queue in PICS      
        #define DLL_MSG_QUEUE_ECN_THRESHOLD   26 // not specified in ISA100; 
    
        #define DLME_MSG_QUEUE_LENGTH  2048        
    
        #define MAX_ROUTES_NO           15   // minimum 4 required by PICS
        #define MAX_CONTRACT_NO          8   // 
        #define MAX_ADDR_TRANSLATIONS_NO  (DLL_MAX_NEIGHBORS_NO+8) // minimum 15 required by PICS
        #define DUPLICATE_ARRAY_SIZE     32
    #else
        #error "DEVICE_TYPE incompatible with BACKBONE_SUPPORT"
    #endif

    #define DLL_MAX_NEIGH_DIAG_NO    1
    #define DLL_ADVERTISE_RATE    1000   // signed int 16 - capacity to transmit advertismenst/minute
    #define DLL_IDLE_LISTEN       1000   // signed int 16 - capacity for idle listening (seconds pe hour)
    #define MAX_SLME_KEYS_NO         2+4+4+12    //2 MasterKey(ProxyDUT + JoiningDUT) + 4 DLLKey + 4 SmSessionKeys + 12(for testing)  
    #define MAX_ASLMO_COUNTER_N0     8   // counts malformed APDUs for maximum 8 communication endpoints
    #define MAX_NB_OF_PORTS          4
    
    #define ARMO_ALERT_ACK_TIMEOUT        180    // units in seconds; reset device if no ACK is received for an alert within ARMO_ALERT_ACK_TIMEOUT seconds   

#elif defined( ROUTING_SUPPORT )
    #if (DEVICE_TYPE == DEV_TYPE_MC13225) // 96k of code + RAM
        #define DLL_MAX_SUPERFRAMES_NO        5   // minimum 5 dlmo superframes required by PICS
        #define DLL_MAX_CHANNELS_NO           10  // minimum 10 dlmo hopping sequences required by PICS
        #define DLL_MAX_TIMESLOTS_NO          10  // minimum 10 dlmo timeslot templates required by PICS
        #define DLL_MAX_LINKS_NO              64  // minimum 15 dlmo links required by PICS
        #define DLL_MAX_GRAPHS_NO             64  // minimum 4 dlmo graphs required by PICS
        #define DLL_MAX_ROUTES_NO             16  // minimum 4 dlmo routes required by PICS
        #define DLL_MAX_NEIGHBORS_NO          32  // minimum  8 dlmo neighbors required by PICS

        /* Warning: Always consider both values when modifying one of the following two values!!! */
        #define DLL_MSG_QUEUE_SIZE_MAX        32  // minimum 3 entries in DLL message queue in PICS     
        #define DLL_MSG_QUEUE_ECN_THRESHOLD   26  // not specified in ISA100; 
    
        #define DLME_MSG_QUEUE_LENGTH  2048   

        #define MAX_SLME_KEYS_NO         1+1+1+1+1+2+2    //1 MasterKey + 1 DLLKey + 1 SmSessionKeys + 1 NewDllKey + 1 NewSessionKeys
                                                            //2 GwSessionKeys(Old+New) + 2 LocalLoopSessionKeys(Old+New)
        #define MAX_CONTRACT_NO          8  // 
        #define MAX_ADDR_TRANSLATIONS_NO 8  // minimum 3 required by PICS
        #define MAX_NB_OF_PORTS          4

        //routers banning support
        #define MAX_BANNED_ROUTER_NO      8
        #define MAX_ADV_DISC_NO          16
        #define DUPLICATE_ARRAY_SIZE     32

    #elif (DEVICE_TYPE == DEV_TYPE_MSP430F2618) // 116k of code + 88k RAM
        #define DLL_MAX_SUPERFRAMES_NO        5  // minimum  5 dlmo superframes required by PICS
        #define DLL_MAX_CHANNELS_NO           8  // minimum 10 dlmo hopping sequences required by PICS
        #define DLL_MAX_TIMESLOTS_NO          3  // minimum 10 dlmo timeslot templates required by PICS
        #define DLL_MAX_LINKS_NO              25 // minimum 15 dlmo links required by PICS
        #define DLL_MAX_GRAPHS_NO             15 // minimum  4 dlmo graphs required by PICS
        #define DLL_MAX_ROUTES_NO             8  // minimum  8 dlmo routes required by PICS
        #define DLL_MAX_NEIGHBORS_NO          8  // minimum  8 dlmo neighbors required by PICS

        /* Warning: Always consider both values when modifying one of the following two values!!! */
        #define DLL_MSG_QUEUE_SIZE_MAX        5  // minimum 3 entries in DLL message queue in PICS  
        #define DLL_MSG_QUEUE_ECN_THRESHOLD   4  // not specified in ISA100; 
    
        #define DLME_MSG_QUEUE_LENGTH  460    

        #define MAX_SLME_KEYS_NO         1+1+1+1+1+2//+2    //1 MasterKey + 1 DLLKey + 1 SmSessionKeys + 1 NewDllKey + 1 NewSessionKeys
                                                            //2 GwSessionKeys(Old+New) + 2 LocalLoopSessionKeys(Old+New)
        #define MAX_CONTRACT_NO          5  // DMAP, SMAP, GW publish, GW server, local loop
        #define MAX_ADDR_TRANSLATIONS_NO 4  // minimum 3 required by PICS (GW, SM, itself, + local loop)
        #define MAX_NB_OF_PORTS          3

        //routers banning support
        #define MAX_BANNED_ROUTER_NO     3
        #define DUPLICATE_ARRAY_SIZE     4

    #else
        #error "DEVICE_TYPE incompatible with ROUTING_SUPPORT"
    #endif

    #define DLL_MAX_NEIGH_DIAG_NO    3  // minimum  3 dlmo neighbor diagnostics required by PICS
    #define DLL_ADVERTISE_RATE     100  // signed int 16 - capacity to transmit advertismenst/minute
    #define DLL_IDLE_LISTEN          0  // signed int 16 - capacity for idle listening (seconds pe hour)

    #define MAX_ROUTES_NO            0  // minimum 3 required by PICS

    #define MAX_ASLMO_COUNTER_N0     4   // counts malformed APDUs for maximum four communication endpoints


#else // non router support (end node)
    #if (DEVICE_TYPE == DEV_TYPE_MC13225) // 96k of code + RAM
        #define DLL_MAX_SUPERFRAMES_NO        5   // minimum 5 dlmo superframes required by PICS
        #define DLL_MAX_CHANNELS_NO           10  // minimum 10 dlmo hopping sequences required by PICS
        #define DLL_MAX_TIMESLOTS_NO          10  // minimum 10 dlmo timeslot templates required by PICS
        #define DLL_MAX_LINKS_NO              15  // minimum 15 dlmo links required by PICS
        #define DLL_MAX_GRAPHS_NO             4   // minimum 4 dlmo graphs required by PICS
        #define DLL_MAX_ROUTES_NO             4   // minimum 4 dlmo routes required by PICS
        #define DLL_MAX_NEIGHBORS_NO          4   // minimum 2 dlmo neighbors required by PICS        

        /* Warning: Always consider both values when modifying one of the following two values!!! */
        #define DLL_MSG_QUEUE_SIZE_MAX        4   // minimum 3 entries in DLL message queue in PICS     
        #define DLL_MSG_QUEUE_ECN_THRESHOLD   3   // not specified in ISA100; 

        #define DLME_MSG_QUEUE_LENGTH  2048

        //routers banning support
        #define MAX_BANNED_ROUTER_NO     8
        #define MAX_CONTRACT_NO          8 
        #define MAX_NB_OF_PORTS          4

        #define MAX_ADV_DISC_NO          16
        #define DUPLICATE_ARRAY_SIZE     16

    #elif (DEVICE_TYPE == DEV_TYPE_MSP430F2618) // 116k of code + 88k RAM

        #define DLL_MAX_SUPERFRAMES_NO  3   // minimum  3 dlmo superframes required by PICS
        #define DLL_MAX_CHANNELS_NO     10  // minimum 10 dlmo hopping sequences required by PICS
        #define DLL_MAX_TIMESLOTS_NO    8   // minimum  8 dlmo timeslot templates required by PICS
        #define DLL_MAX_LINKS_NO        9   // minimum  9 dlmo links required by PICS
        #define DLL_MAX_GRAPHS_NO       3   // minimum  2 dlmo graphs required by PICS
        #define DLL_MAX_ROUTES_NO       4   // minimum  2 dlmo graphs required by PICS
        #define DLL_MAX_NEIGHBORS_NO    2   // minimum  2 dlmo neighbors required by PICS        

        /* Warning: Always consider both values when modifying one of the following two values!!! */
        #define DLL_MSG_QUEUE_SIZE_MAX        3  //  no minimum DLL message queue size in PICS    
        #define DLL_MSG_QUEUE_ECN_THRESHOLD   2  // not specified in ISA100; 

        #define DLME_MSG_QUEUE_LENGTH  460  // length in octets of the dlme command queue

        //routers banning support
        #define MAX_BANNED_ROUTER_NO     3
        #define MAX_CONTRACT_NO          4 
        #define MAX_NB_OF_PORTS          3
        #define DUPLICATE_ARRAY_SIZE     4
    #else
        #error "DEVICE_TYPE incompatible with non routing support"
    #endif

    #define DLL_MAX_NEIGH_DIAG_NO     2   // minimum  2 dlmo neighbor diagnostics required by PICS
    #define DLL_ADVERTISE_RATE        0   // signed int 16 - capacity to transmit advertismenst/minute
    #define DLL_IDLE_LISTEN           0   // signed int 16 - capacity for idle listening (seconds pe hour)
    #define MAX_SLME_KEYS_NO          1+1+1+1+1+2+2    //1 MasterKey + 1 DLLKey + 1 SmSessionKeys + 1 NewDllKey + 1 NewSessionKeys
                                                       //2 GwSessionKeys(Old+New) + 2 LocalLoopSessionKeys(Old+New)
    #define MAX_ROUTES_NO             0  // minimum 3 required by PICS
    #define MAX_ADDR_TRANSLATIONS_NO  4   // minimum 4 required by PICS

    #define MAX_ASLMO_COUNTER_N0      4   // counts malformed APDUs for maximum four communication endpoints

#endif //defined( BACKBONE_SUPPORT )

   #define MAX_APDU_CONF_ENTRY_NO    DLL_MSG_QUEUE_SIZE_MAX   // maximum TL confirmation list size

    #define MAX_TX_NO_TO_ONE_NEIHBOR      4
    #define DLL_MAX_CANDIDATE_NO          5       // minimum 4 required by ISA100 specification
    #define DLL_CHANNEL_MAP               0xFFFF  // all channels available
    #define DLL_FORWARD_RATE              100     // device's energy capacity to forward DPDUs on behalf of its neighbors, in number of DPDUs per minute
    #define DLL_ACK_TURNAROUND            100     // time needed by the device to process incomming DPDU and respond to it with ACK/NACK, in units of 2^-15
    #define DLL_CLOCK_ACCURACY            8       // nominal clock accuracy in ppm ( DS32khz datasheet specifies 7.5 ppm for -40 to 85 degrees celsius )

    #define DLMO_NEIGH_DIAG_ALERT_TIMEOUT 180     // units in seconds; no new alarm will be generated within this period

    #define MAX_DROUT_ROUTES_NO           15

    #define ECN_IGNORE_TIME                 12      // units of 1/4 seconds    

    #define ARMO_ALERT_ACK_TIMEOUT        180    // units in seconds; reset device if no ACK is received for an alert within ARMO_ALERT_ACK_TIMEOUT seconds   

    #define DMAP_PORT 0
    #define UAP_PORT  1

#endif // _NIVIS_ISA100_CONFIG_H_
