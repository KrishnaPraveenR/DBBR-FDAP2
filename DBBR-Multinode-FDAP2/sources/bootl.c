#include "system.h"

#if   (DEVICE_TYPE == DEV_TYPE_MC13225) 
#include "D:\My Working Folder\SVN\MC13225\ISA100\trunk\ISA100 2.xx\source_files\spif_interface.h"
//#include "spif_interface.h"
    #define  ISA_ACTIVE_CODE_ADDR_LOCATION   0x015000
    #define HART_ACTIVE_CODE_ADDR_LOCATION   0x016000
    #define  ISA_PAGE1_CODE_ADDR    0x002000
    #define  ISA_PAGE2_CODE_ADDR    0x022000
  #warning  "TODO: Check code addr location to match the bootloader"   
    #define CODE_ADDR_LOCATION_SECTOR   (1UL<<(ISA_ACTIVE_CODE_ADDR_LOCATION/4096))
    uint32 CodePageAdress(void)
    {
        uint32 ulLocal;
        NVM_FlashRead(&ulLocal, ISA_ACTIVE_CODE_ADDR_LOCATION, 4);
        if ((ulLocal==ISA_PAGE1_CODE_ADDR) || (ulLocal==ISA_PAGE2_CODE_ADDR))
            return ulLocal;
        else
            return ISA_PAGE1_CODE_ADDR;
    }

    void CallBootl(uint32* p_pulCodeDest)
    {
        //scrie in flash-ul intern noua adresa la adresa ISA_ACTIVE_CODE_ADDR_LOCATION
        NVM_FlashErase(CODE_ADDR_LOCATION_SECTOR);
        NVM_FlashWrite((void*)p_pulCodeDest, ISA_ACTIVE_CODE_ADDR_LOCATION, 4);
    }

#elif (DEVICE_TYPE == DEV_TYPE_MSP430F1611) 
   #warning  "OBSOLATE"   
        
#elif (DEVICE_TYPE == DEV_TYPE_AP91SAM7X512)        
    #define BOOTL_START_ADDR    0x00100100
    void CallBootl(void)
    {
      ((void(*)(void))BOOTL_START_ADDR)();
    }

#elif (DEVICE_TYPE == DEV_TYPE_MSP430F2618)
   #warning  "TODO: Call Bootloader"   

#else
  #warning  "NoBootloaderExpected" 
    void CallBootl(void) {return;}
#endif  
