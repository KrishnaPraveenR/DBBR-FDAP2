/***************************************************************************************************
* Name:         eth.h
* Author:       Nivis LLC,
* Date:         Aug, 2008
* Description:  This header file provide API of implmentation of ETH controller
* Changes:
* Revisions:
****************************************************************************************************/
#ifndef _NIVIS_ETH_H_
#define _NIVIS_ETH_H_

    extern uint8 c_aETH_MAC[];

    void ETH_Init(void);
    void ETH_Interrupt(void);
    void ETH_Task(void);
        
    extern uint16 g_unArpRequestCounter;
    extern uint16 g_unNTPReqCounter;
    
    void ETH_Inc10msCounter();
#endif // _NIVIS_ETH_H_

