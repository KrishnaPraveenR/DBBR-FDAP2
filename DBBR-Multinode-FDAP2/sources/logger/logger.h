#ifndef _NIVIS_LOG_H_
#define _NIVIS_LOG_H_

#include "err.h"

#ifdef _NIVIS_LOGGING_
    #include "../typedef.h"

    void LOGQ_Init(void);
    void SetLogThreshold (unsigned char p_ucNewLevel, unsigned char p_ucOfModule);
#define Log(X,Y,Z,A,...)      MyLog      (((X)+((Y)<<8)+((Z)<<16)),A,__VA_ARGS__,0)
#define LogShort(X,Y,Z,A,...) MyShortLog (((X)+((Y)<<8)+((Z)<<16)),A,__VA_ARGS__)
    void MyLog      (unsigned long p_ucUnused_ucSeverity_ucModule_ucOperation, unsigned char p_ucLenght, ... )           ;
    void MyShortLog (unsigned long p_ucUnused_ucSeverity_ucModule_ucOperation, unsigned char p_ucNumberOfParams, ... )   ;
    uint16 LOGQ_GetOutMsg( uint8 * p_pMsg );
#else
    #define LOGQ_Init(...)
    #define SetLogThreshold(...)
    #define Log(...)
    #define LogShort(...)
    #define LOGQ_GetOutMsg(...) 0
#endif //_NIVIS_LOGGING_

#endif //_NIVIS_LOG_H_
