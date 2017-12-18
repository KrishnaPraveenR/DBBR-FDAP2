#ifndef _NIVIS_TEXT_H_
#define _NIVIS_TEXT_H_

#ifdef WCI_TEXT_SUPPORT
  #include "../typedef.h"
  #include "../ISA100/phy.h" 

    typedef enum
    {
        WCITXT_MSG_TO_APP,
        WCITXT_MSG_TO_RF,
        WCITXT_MSG_TO_RF_AS_RSP,
        WCITXT_MSG_TO_BBR_AS_RF,
        WCITXT_MSG_TO_BBR_AS_APP,
        WCITXT_MSG_TO_BBR_AS_ERR,
        WCITXT_MSG_TO_BBR_AS_CFM,    //TX RF confirmation
        WCITXT_MSG_TO_BBR_AS_TX_ACK, //Sent ACK
        WCITXT_MSG_TO_BBR_AS_RX_ACK, //Received ACK
        WCITXT_MSG_TYPES
    } WCITXT_MSG_TYPE;

    typedef struct
    {
        uint8 m_ucLen;
        uint8 m_aBuff[128];
    } WCITXT_BIN_FIELD;
	
// RCV messages types 
          
    // TX_RF, Message number, app payload, TL encryption, priority, discard eligible, ECN, 
          // IPv6 src address, IPv6 dst address, IPv6 src port, IPv6 dst port, 
          // Contract ID, UDP compression, Network layer header, Link related message, DLL layer header
    typedef struct          
    {
        uint8  m_ucTLEncryption;
        uint8  m_ucPriority;
        uint8  m_ucDiscardEligible;
        uint8  m_ucECN;
        uint8  m_aIPv6SrcAddr[16];
        uint8  m_aIPv6DstAddr[16];
        uint8  m_aIPv6SrcPort[2];
        uint8  m_aIPv6DstPort[2];
        uint16 m_unContractID;
        uint16 m_unLinkRelated;
        uint8  m_ucUDPCompression;        
        WCITXT_BIN_FIELD m_stNL;
        WCITXT_BIN_FIELD m_stDL;
    } WCITXT_TX_RF;

    // TX_RSP, Message number, app payload, TL encryption, priority, discard eligible, ECN, 
          // IPv6 src address, IPv6 dst address, IPv6 src port, IPv6 dst port, 
          // Contract ID, UDP compression, Network layer header, Link related message, DLL layer header
          // Orig APDU, Orig TL, Orig NL, Orig DL
    typedef struct          
    {
        WCITXT_TX_RF      m_stRf;
        WCITXT_BIN_FIELD  m_stOrigAPDU;
        WCITXT_BIN_FIELD  m_stOrigTL;
        WCITXT_BIN_FIELD  m_stOrigNL;
        WCITXT_BIN_FIELD  m_stOrigDL;
    } WCITXT_TX_RSP;

    typedef struct
    {
        uint32 m_ulMsgNo;
        uint8  m_ucMsgType;
        WCITXT_BIN_FIELD m_stApp;
        union
        {
            WCITXT_TX_RF  m_stRf;
            WCITXT_TX_RSP m_stRsp;            
        } m_u;
    } WCITXT_RCV_MSG;

// TX messages types 

    // RX_RF, Message number, App message, Transport layer header, Network layer header, DLL layer header, Nominal RX Slot Second, Nominal RX Slot Fractions
    typedef struct          
    {
        WCITXT_BIN_FIELD m_stApp;
        WCITXT_BIN_FIELD m_stTL;
        WCITXT_BIN_FIELD m_stNL;
        WCITXT_BIN_FIELD m_stDL;
    } WCITXT_TX_RX_RF;
    
    //TX_DLL_CFM, Message number, TX Message ID, TxRetryCount, TxStatus, DLL layer header, Nominal RX Slot Second, Nominal RX Slot Fractions 
    typedef struct          
    {
        uint32 ulTxMsgID;
        uint8 ucTxStatus;
        uint8 ucTxRetryCount;
        WCITXT_BIN_FIELD m_stDL;
    } WCITXT_TX_DLL_CFM;
    
    //RX_SENT/RECV_DLL_ACK, Message number, AckVirtualMMIC, DLL layer header, AckComputedMMIC, Nominal RX Slot Second, Nominal RX Slot Fractions
    typedef struct          
    {
        //uint32 ulVirtualMMIC;
        WCITXT_BIN_FIELD m_stDL;
        //uint32 ulComputedMMIC;
    } WCITXT_DLL_ACK;
    
    // RX_CFG, Message number, App message
    // TX_CFG, Message number, App message
    typedef struct          
    {
        WCITXT_BIN_FIELD m_stTxDL;
    }WCITXT_DLL_TX;
    typedef struct          
    {
        WCITXT_BIN_FIELD m_stApp;
    } WCITXT_RX_CFG;
    
    // RX_ERR, Message number, Severity, Stack level, Error code, Error description
    typedef struct          
    {
        uint8 m_ucSeverity;
        uint8 m_ucLevel;
        uint8 m_ucErrorCode;
        WCITXT_BIN_FIELD m_stDesc;
    } WCITXT_RX_ERR;
    
    typedef struct
    {
        uint8  m_ucMsgType;
        union
        {
            WCITXT_TX_RX_RF  m_stRf;
            WCITXT_RX_CFG m_stCfg;
            WCITXT_RX_ERR m_stErr;
            WCITXT_TX_DLL_CFM m_stDllCfm;
            WCITXT_DLL_ACK m_stDllAck;
            WCITXT_DLL_TX  m_stDllTx;
        } m_u;
        uint32 m_ulRxSlotTAISec;
        uint16 m_unRxSlotTAIFract;
    #ifdef DEBUG_MSG_DELAY
        uint16 m_unDelayMilliSec;
    #endif
        uint16 m_unRxSFDFract;
        uint8  m_MMIC[MAX_DMIC_SIZE];
        uint8  m_TMIC[MAX_TMIC_SIZE];
        uint8  m_ucChannel;
        
    } WCITXT_TX_MSG;
    
#ifdef DEBUG_MSG_DELAY
    #define ROUND_FACTOR  (1 << 4)    //0,5 * 2^-15 sec
    
    extern uint32 g_ulMsgTaiSec;
    extern uint32 g_ulMsgTaiMicroSec;
    uint16 GetMSecMessageDelay(uint32 p_ulMsgTaiSec, uint32 p_ulMsgTaiMicroSec);
#endif
    
    // parsing methods 
    
    int32 atol(const char * p_sNo);
 
    void    Bin2Hex(char * p_svMsg, uint8 * p_puchBin, uint16 p_nLen);
    uint16  Hex2Bin(char * p_svMsg, uint8 * p_puchRes);
  
    uint8   WCITXT_ConvertTxt2Msg( char * p_sTxt, WCITXT_RCV_MSG * p_pRcvMsg );
    uint16  WCITXT_ConvertMsg2Txt( unsigned long p_ulMsgId, const WCITXT_TX_MSG * p_pTxMsg, uint8 * p_pTxt );
    
#endif  // WCI_TEXT_SUPPORT

#endif  // _NIVIS_TEXT_H_
