#ifndef _NIVIS_BOOTL_H_
#define _NIVIS_BOOTL_H_

#if   (DEVICE_TYPE == DEV_TYPE_MC13225) 
    uint32 CodePageAdress(void);
    void CallBootl(uint32* p_pulCodeDest);
#else
    void CallBootl(void);    
#endif
    
#endif //_NIVIS_BOOTL_H_

