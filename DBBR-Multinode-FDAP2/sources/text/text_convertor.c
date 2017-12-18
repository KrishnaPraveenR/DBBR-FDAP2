#include "text_convertor.h"

#ifdef WCI_TEXT_SUPPORT
    #include <stdio.h>
    #include <string.h>
    #include "../Isa100/mlsm.h"
 
    #define ASCII_CASE_OFFSET 32
    #define TERMINATOR_CHAR   '\0'
    #define SPACE_CHAR        ' '
    #define SEPARATOR_CHAR    ','
    #define SEPARATOR_STRING  ","

    const char * c_aszCommands[WCITXT_MSG_TYPES] = {"TX_CFG", "TX_RF", "TX_RSP", "RX_RF", "RX_CFG", "RX_ERR", "RX_DLL_CFM", "RX_SENT_DLL_ACK", "RX_RECV_DLL_ACK"};    
    #define SEVERITY_TYPES    4
    const char * c_aszSeverity[SEVERITY_TYPES] = {"Error", "Warning", "Info", "Debug"};    
    #define STACK_LEVELS      6
    const char * c_aszStackLvl[STACK_LEVELS] = {"PHY", "DLL", "NL", "TL", "ASL", "APP"};
    
#ifdef DEBUG_MSG_DELAY
    uint32 g_ulMsgTaiSec;
    uint32 g_ulMsgTaiMicroSec;
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @author NIVIS LLC, Mircea Vlasin
    /// @brief  return the received message delay in milliseconds
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    uint16 GetMSecMessageDelay(uint32 p_ulMsgTaiSec, uint32 p_ulMsgTaiMicroSec)
    {
        uint32 ulTaiSec;
        uint16 unTaiFract;
        
        MLSM_GetCrtTaiTime(&ulTaiSec, &unTaiFract);
        
        ulTaiSec -= p_ulMsgTaiSec;
        
        if( ulTaiSec & 0x80000000 )
        {
            return 0;
        }
        else
        {
            p_ulMsgTaiMicroSec >>= 5;  //convert to 2^-15 sec 
            
            if( unTaiFract >= p_ulMsgTaiMicroSec )
            {
                return (ulTaiSec * 1000 + ((unTaiFract - (uint16)p_ulMsgTaiMicroSec + ROUND_FACTOR) >> 5)); 
            }
            else
            {
                if( ulTaiSec-- )
                {
                    return (ulTaiSec * 1000 + (((uint16)p_ulMsgTaiMicroSec - unTaiFract + ROUND_FACTOR) >> 5));
                }
                else
                {
                    return 0;
                }
            }    
        }
    }
#endif

    // reversing bytes functions
    uint16 ntohs(uint16 p_ushNo)
    {
            uint16 ushOne = 1;
            if (*(uint8 *)&ushOne)
                    return (p_ushNo<<8) | (p_ushNo>>8);
            return p_ushNo;
    }
    
    uint32 ntohl(uint32 p_ulNo)
    {
            uint16 ushOne = 1;
            if (*(uint8 *)&ushOne)
                    return (ntohs((uint16)p_ulNo)<<16) | ntohs((uint16)(p_ulNo>>16));
            return p_ulNo;
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// sprintf_string
    /// @author NIVIS LLC, Eugen GHICA
    /// @brief  similar to sprintf(dst_string, "%s", str)
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    char * add_string(char * p_sDst, const char * p_sSrc, char p_ucDelimChar )
    {
        if( p_ucDelimChar )
        {
          *(p_sDst++) = p_ucDelimChar;
        }
        
        while (*p_sSrc)
        {
            *p_sDst++ = *p_sSrc++;
        }
        
        *p_sDst = '\0';
        
        return p_sDst;
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// sprintf_long
    /// @author NIVIS LLC, Eugen GHICA
    /// @brief  similar to sprintf(dst_string, "%u", no)
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    char * add_long(char * p_sDst, unsigned long p_ulNo, char p_ucDelimChar )
    {
        if( p_ucDelimChar )
        {
          *(p_sDst++) = p_ucDelimChar;
        }
        
        p_sDst += sprintf( p_sDst, "%u", p_ulNo );
        *p_sDst = '\0';
        
        return p_sDst;
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// getNextParam
    /// @author NIVIS LLC, Eugen GHICA
    /// @brief  get next parameter  separated by comma and extract unwanted chars from a string
    ///         assume first call of strtok was already made     
    /// @params none
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    char * getNextParam(void)
    {
        char * pRet = strtok( NULL, SEPARATOR_STRING );
        if ( pRet )
        {
            char * pIdx = pRet;
            char * pCrt = pRet;
            for( ; *pIdx; pIdx++ )
            {
                if( *pIdx != SPACE_CHAR )
                    *(pCrt++) = *pIdx;
            }
            *pCrt = TERMINATOR_CHAR;            
        }        
        return pRet;
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Hex2Bin
    /// @author NIVIS LLC, Eugen GHICA
    /// @brief  hex text to binary
    /// @params ASCII hex
    ///         buffer to put binary results
    /// @returns: len of binary data
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    uint16 Hex2Bin(char * p_svMsg, uint8 * p_puchRes)
    {
        uint16 nRet = 0;
        int nTmp = 0;
                
        do
        {
            if ( sscanf(p_svMsg, "%02X", &nTmp) != 1 )
		 break;
            nRet++;
            *p_puchRes++ = nTmp;
            p_svMsg += 2;
        }
        while (TRUE);
        
        return nRet;
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Bin2Hex
    /// @author NIVIS LLC, Eugen GHICA
    /// @brief  binary to hex text
    /// @params output hex
    ///         input binary buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    char * add_Bin2Hex(char * p_pDst, uint8 * p_puchBin, uint16 p_nLen,  char p_ucDelimChar )
    { 
        if( p_ucDelimChar )
        {
          *(p_pDst++) = p_ucDelimChar;
        }
      
        while ( p_nLen-- )
        {
            uint8 uchChar = ((*p_puchBin) >> 4);
            
            if (uchChar < 0xA)  uchChar += '0';
            else                uchChar += 'A' - 0xA;
            *(p_pDst++) = uchChar;
            
            uchChar = ((*(p_puchBin++)) & 0x0F);
            
            if (uchChar < 0xA)  uchChar += '0';
            else                uchChar += 'A' - 0xA;
            *(p_pDst++) = uchChar;
        }
        
        *p_pDst = 0;
        return p_pDst;
    }    
      
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WCITXT_ConvertTxt2Msg
    /// @author NIVIS LLC, Eugen GHICA
    /// @brief  parse a comma separated list of parameters into binary 
    /// @params UDP as string: comma separated params
    ///         structure pointer to put binary results
    /// @returns: operation result as boolean
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    uint8   WCITXT_ConvertTxt2Msg( char * p_sTxt, WCITXT_RCV_MSG * p_pRcvMsg )
    {
        char * p;                           // crt string param
        int i = 0;                          // counter
        unsigned int nTmp = 0;              // scan val
        uint8 * pChrFld = NULL;             // member addr
        WCITXT_BIN_FIELD * pBinFld = NULL;  // hex member addr
        
        // message type
        p = strtok( p_sTxt, SEPARATOR_STRING );
        if ( !p ) return 0;
        for ( i = 0; i < WCITXT_MSG_TYPES; i++ )
        {
            if ( !strcmp(c_aszCommands[i], p) )
                break;
        }        
        if ( i == WCITXT_MSG_TYPES )  return 0; // invalid msg type
        p_pRcvMsg->m_ucMsgType = i;
        
        // message number
        if ( !(p = getNextParam()) )  return 0;
        p_pRcvMsg->m_ulMsgNo = atol(p);
        
        // app payload - hex digits
        if ( !(p = getNextParam()) )  return 0;
        p_pRcvMsg->m_stApp.m_ucLen = Hex2Bin(p, p_pRcvMsg->m_stApp.m_aBuff);
        
        switch (p_pRcvMsg->m_ucMsgType) {          
        case WCITXT_MSG_TO_RF:
        case WCITXT_MSG_TO_RF_AS_RSP:
            
            // TL encryption, priority, discard eligible, ECN - dec, dec, bool, bool
            for ( i = 0, pChrFld = (uint8 *)&p_pRcvMsg->m_u; i < 4; i++ )
            {
                if ( !(p = getNextParam()) )  return 0;
                nTmp = atol(p);
                if ( i > 1) *pChrFld = (nTmp == 0) ? FALSE : TRUE; // boolean
                else        *pChrFld = nTmp;                       // decimal
                pChrFld++;                
            }
           
            // IPv6 src address, IPv6 dst address - 32, 32 hex digits
            for ( i = 0, pChrFld = p_pRcvMsg->m_u.m_stRf.m_aIPv6SrcAddr; i < 2; i++ )
            {
                if ( !(p = getNextParam()) )  return 0;
                memset(pChrFld, 0, sizeof(IPV6_ADDR));
                Hex2Bin(p, pChrFld);
                pChrFld += sizeof(IPV6_ADDR);
            }
            
            // IPv6 src port, IPv6 dst port, Contract ID - dec, dec, dec
            for ( i = 0, pChrFld = p_pRcvMsg->m_u.m_stRf.m_aIPv6SrcPort; i < 3; i++ )
            {
                if ( !(p = getNextParam()) )  return 0;
                *(uint16 *)pChrFld = ntohs(atol(p));
                pChrFld += sizeof(uint16);
            }
            
            // UDP compression
            if ( !(p = getNextParam()) )  return 0;
            p_pRcvMsg->m_u.m_stRf.m_ucUDPCompression = atol(p);
            
            // Network layer header - hex
            if ( !(p = getNextParam()) )  return 0;           
            p_pRcvMsg->m_u.m_stRf.m_stNL.m_ucLen = Hex2Bin(p, p_pRcvMsg->m_u.m_stRf.m_stNL.m_aBuff);
            
            // Link related message - dec
            if ( !(p = getNextParam()) )  return 0;
            p_pRcvMsg->m_u.m_stRf.m_unLinkRelated = atol(p);
            
            // DLL layer header - hex
            if ( !(p = getNextParam()) )  return 0;            
            p_pRcvMsg->m_u.m_stRf.m_stDL.m_ucLen = Hex2Bin(p, p_pRcvMsg->m_u.m_stRf.m_stDL.m_aBuff);
            
            if ( p_pRcvMsg->m_ucMsgType == WCITXT_MSG_TO_RF_AS_RSP )
            {
                // Orig APDU, Orig TL, Orig NL, Orig DL - hex, hex, hex, hex
                for ( i = 0, pBinFld = &p_pRcvMsg->m_u.m_stRsp.m_stOrigAPDU; i < 4; i++ )
                { 
                    if ( !(p = getNextParam()) )  return 0;
                    pBinFld->m_ucLen = Hex2Bin(p, pBinFld->m_aBuff);
                    pBinFld++;
                }
            }
            
        #ifdef DEBUG_MSG_DELAY
           if ( !(p = getNextParam()) )  return 0; 
           g_ulMsgTaiSec = atol(p); 

           if ( !(p = getNextParam()) )  return 0; 
           g_ulMsgTaiMicroSec = atol(p); 
        #endif
            break;
            
        case WCITXT_MSG_TO_APP:
        #ifdef DEBUG_MSG_DELAY
           if ( !(p = getNextParam()) )  return 0; 
           g_ulMsgTaiSec = atol(p); 

           if ( !(p = getNextParam()) )  return 0; 
           g_ulMsgTaiMicroSec = atol(p); 
        #endif
            break;
            
        default:
            return 0;
            
        } // switch end - msg type
        
        return 1;
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WCITXT_ConvertMsg2Txt
    /// @author NIVIS LLC, Eugen GHICA
    /// @brief  converts binary messages into comma separated parameters
    /// @params binary message
    ///         output string
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    uint16  WCITXT_ConvertMsg2Txt( unsigned long p_ulMsgId, const WCITXT_TX_MSG * p_pTxMsg, uint8 * p_pTxt )
    {
        int i = 0;                          // counter
        char * sMsg = (char *)p_pTxt;       // output string
        WCITXT_BIN_FIELD * pBinFld = NULL;  // hex member addr

        // message type
        if ( p_pTxMsg->m_ucMsgType >= WCITXT_MSG_TYPES )  return 0;
        sMsg = add_string(sMsg, c_aszCommands[p_pTxMsg->m_ucMsgType], 0 );        

        // message number
        sMsg = add_long(sMsg, p_ulMsgId, SEPARATOR_CHAR );
        
        switch ( p_pTxMsg->m_ucMsgType ) {
        case WCITXT_MSG_TO_BBR_AS_RF:
        case WCITXT_MSG_TO_BBR_AS_APP:
            for ( i = 0, pBinFld = (WCITXT_BIN_FIELD *)&p_pTxMsg->m_u; i < 4; i++ )
            {
                sMsg = add_Bin2Hex(sMsg, pBinFld->m_aBuff, pBinFld->m_ucLen, SEPARATOR_CHAR);
                
                if ( p_pTxMsg->m_ucMsgType != WCITXT_MSG_TO_BBR_AS_RF )
                    break;
               
                pBinFld++;
            }
            
            if( p_pTxMsg->m_ucMsgType == WCITXT_MSG_TO_BBR_AS_RF )
            {
                sMsg = add_Bin2Hex(sMsg, (uint8 *)p_pTxMsg->m_MMIC, MAX_DMIC_SIZE, SEPARATOR_CHAR);
                sMsg = add_Bin2Hex(sMsg, (uint8 *)p_pTxMsg->m_TMIC, MAX_TMIC_SIZE, SEPARATOR_CHAR);
                sMsg = add_long(sMsg, p_pTxMsg->m_ucChannel, SEPARATOR_CHAR );

                // time of nominal RX slot start in 2^-16 sec(accuracy 2^-10 sec[~ms]) 
                sMsg = add_long(sMsg, p_pTxMsg->m_ulRxSlotTAISec, SEPARATOR_CHAR );
                sMsg = add_long(sMsg, p_pTxMsg->m_unRxSlotTAIFract, SEPARATOR_CHAR );
                sMsg = add_long(sMsg, p_pTxMsg->m_unRxSFDFract, SEPARATOR_CHAR );
            }
            
        #ifdef DEBUG_MSG_DELAY
            else
            {
                sMsg = add_long(sMsg, g_ulMsgTaiMicroSec, SEPARATOR_CHAR );
            }
        #endif
            break;
        
        case WCITXT_MSG_TO_RF:    
/* TX_RF,
          Message number,
          APDU TL encryption, 
          Priority, 
          Discard eligible,
          ECN, 
          IPv6 src address, 
          IPv6 dst address ,
          IPv6 src port,
          IPv6 dst port,
          Contract ID,
          UDP compression,
          Network layer  header, 
          Link related message,
          DLL layer header,
          DMIC
          TMIC
          TAI_Sec, 
          TAI_Fraction 
          */
                if(pBinFld->m_ucLen >= 128)  // test code only.. to be removed.
                  pBinFld->m_ucLen = 128 ;
                pBinFld = (WCITXT_BIN_FIELD *)&p_pTxMsg->m_u.m_stDllTx.m_stTxDL;
                sMsg = add_Bin2Hex(sMsg, pBinFld->m_aBuff, pBinFld->m_ucLen, SEPARATOR_CHAR);
                sMsg = add_Bin2Hex(sMsg, pBinFld->m_aBuff, pBinFld->m_ucLen, SEPARATOR_CHAR);
                sMsg = add_Bin2Hex(sMsg, (uint8 *)p_pTxMsg->m_MMIC, MAX_DMIC_SIZE, SEPARATOR_CHAR);
                sMsg = add_Bin2Hex(sMsg, (uint8 *)p_pTxMsg->m_TMIC, MAX_TMIC_SIZE, SEPARATOR_CHAR);
                sMsg = add_long(sMsg, p_pTxMsg->m_ulRxSlotTAISec, SEPARATOR_CHAR );
                sMsg = add_long(sMsg, p_pTxMsg->m_unRxSlotTAIFract, SEPARATOR_CHAR );
        break;
        case WCITXT_MSG_TO_BBR_AS_CFM:
            sMsg = add_long(sMsg, p_pTxMsg->m_u.m_stDllCfm.ulTxMsgID, SEPARATOR_CHAR);
            sMsg = add_long(sMsg, p_pTxMsg->m_u.m_stDllCfm.ucTxStatus, SEPARATOR_CHAR);
            sMsg = add_long(sMsg, p_pTxMsg->m_u.m_stDllCfm.ucTxRetryCount, SEPARATOR_CHAR);
            pBinFld = (WCITXT_BIN_FIELD *)&p_pTxMsg->m_u.m_stDllCfm.m_stDL;
            sMsg = add_Bin2Hex(sMsg, pBinFld->m_aBuff, pBinFld->m_ucLen, SEPARATOR_CHAR);

            sMsg = add_Bin2Hex(sMsg, (uint8 *)p_pTxMsg->m_MMIC, MAX_DMIC_SIZE, SEPARATOR_CHAR);
            sMsg = add_long(sMsg, p_pTxMsg->m_ucChannel, SEPARATOR_CHAR );

            // time of nominal RX slot start in 2^-16 sec(accuracy 2^-10 sec[~ms]) 
            sMsg = add_long(sMsg, p_pTxMsg->m_ulRxSlotTAISec, SEPARATOR_CHAR );
            sMsg = add_long(sMsg, p_pTxMsg->m_unRxSlotTAIFract, SEPARATOR_CHAR );
        #ifdef DEBUG_MSG_DELAY
            sMsg = add_long(sMsg, p_pTxMsg->m_unDelayMilliSec, SEPARATOR_CHAR );
        #endif
        
            break;
            
        case WCITXT_MSG_TO_BBR_AS_TX_ACK:
        case WCITXT_MSG_TO_BBR_AS_RX_ACK:
            pBinFld = (WCITXT_BIN_FIELD *)&p_pTxMsg->m_u.m_stDllAck.m_stDL;
            sMsg = add_Bin2Hex(sMsg, pBinFld->m_aBuff, pBinFld->m_ucLen, SEPARATOR_CHAR);

            sMsg = add_Bin2Hex(sMsg, (uint8 *)p_pTxMsg->m_MMIC, MAX_DMIC_SIZE, SEPARATOR_CHAR);
            sMsg = add_long(sMsg, p_pTxMsg->m_ucChannel, SEPARATOR_CHAR );

            // time of nominal RX/TX slot start in 2^-16 sec(accuracy 2^-10 sec[~ms]) 
            sMsg = add_long(sMsg, p_pTxMsg->m_ulRxSlotTAISec, SEPARATOR_CHAR );
            sMsg = add_long(sMsg, p_pTxMsg->m_unRxSlotTAIFract, SEPARATOR_CHAR );
            
            if( WCITXT_MSG_TO_BBR_AS_RX_ACK == p_pTxMsg->m_ucMsgType )
            {
                sMsg = add_long(sMsg, p_pTxMsg->m_unRxSFDFract, SEPARATOR_CHAR );
            }

            break;
            
        case WCITXT_MSG_TO_BBR_AS_ERR:          
            // severity
            if ( p_pTxMsg->m_u.m_stErr.m_ucSeverity >= SEVERITY_TYPES ) return 0;
            sMsg = add_string(sMsg, c_aszSeverity[p_pTxMsg->m_u.m_stErr.m_ucSeverity], SEPARATOR_CHAR );
            
            // stack level
            if ( p_pTxMsg->m_u.m_stErr.m_ucLevel >= STACK_LEVELS )  return 0;
            sMsg = add_string(sMsg, c_aszStackLvl[p_pTxMsg->m_u.m_stErr.m_ucLevel], SEPARATOR_CHAR );
            
            // error code
            sMsg = add_long(sMsg, p_pTxMsg->m_u.m_stErr.m_ucErrorCode, SEPARATOR_CHAR );
            
            // error description
            sMsg = add_string(sMsg,  (const char*) p_pTxMsg->m_u.m_stErr.m_stDesc.m_aBuff, SEPARATOR_CHAR );            
            break;
        
        default:
            return 0;
        } // switch end - msg type

        *sMsg = '\n';
        return sMsg + 1 - (char *)p_pTxt;;
    }
    

#endif  // WCI_TEXT_SUPPORT
