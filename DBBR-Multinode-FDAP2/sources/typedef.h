/***************************************************************************************************
* Name:         typedef.h
* Author:       Nivis LLC, Ion Ticus
* Date:         Aug 2008
* Description:  This header file
* Changes:
* Revisions:
****************************************************************************************************/
#ifndef _NIVIS_TYPEDEF_H_
#define _NIVIS_TYPEDEF_H_

/* Types definition */
typedef unsigned char   byte;
typedef unsigned short  word;
typedef unsigned long   dword;

typedef signed   char   int8;
typedef unsigned char   uint8;
typedef signed   short  int16;
typedef unsigned short  uint16;
typedef signed   long   int32;
typedef unsigned long   uint32;

typedef unsigned char  u_char;
typedef unsigned int   u_int;
typedef unsigned short u_short;
typedef unsigned long  u_long;

typedef void (* handler_func_type)(void);

typedef uint16  HANDLE;
typedef uint16  SHORT_ADDR;
typedef uint8   LONG_ADDR[8];
typedef uint8   IPV6_ADDR[16];
typedef uint8   EUI64_ADDR[8];
typedef uint32  TIME;

//#define TRUE    1
//#define FALSE   0

typedef enum { FALSE, TRUE } Boolean;

#define BIT0    0x00000001UL
#define BIT1    0x00000002UL
#define BIT2    0x00000004UL
#define BIT3    0x00000008UL
#define BIT4    0x00000010UL
#define BIT5    0x00000020UL
#define BIT6    0x00000040UL
#define BIT7    0x00000080UL
#define BIT8    0x00000100UL
#define BIT9    0x00000200UL
#define BIT10   0x00000400UL
#define BIT11   0x00000800UL
#define BIT12   0x00001000UL
#define BIT13   0x00002000UL
#define BIT14   0x00004000UL
#define BIT15   0x00008000UL
#define BIT16   0x00010000UL
#define BIT17   0x00020000UL
#define BIT18   0x00040000UL
#define BIT19   0x00080000UL
#define BIT20   0x00100000UL
#define BIT21   0x00200000UL
#define BIT22   0x00400000UL
#define BIT23   0x00800000UL
#define BIT24   0x01000000UL
#define BIT25   0x02000000UL
#define BIT26   0x04000000UL
#define BIT27   0x08000000UL
#define BIT28   0x10000000UL
#define BIT29   0x20000000UL
#define BIT30   0x40000000UL
#define BIT31   0x80000000UL

#ifndef NULL
#define NULL (void *)(0)
#endif

#define offsetof(s,m)	(uint8)&(((s *)0)->m)

#define DEV_TYPE_MC13225      1
//#define DEV_TYPE_MSP430F1611  2
#define DEV_TYPE_MSP430F2618  3
#define DEV_TYPE_AP91SAM7X512 4

#define UAP_TYPE_SIMPLE_API            1 // Used for modem running UAP.
#define UAP_TYPE_ISA100_API            2 // Used for Application Processor running UAP.

#define PLATFORM_UNKNOWN          1
#define PLATFORM_DEVELOPMENT      2   /* Nivis ISA100 Raptor COMM Interface 52000127 Rev. D + Crow Development Board (Raptor form factor) 52000144 Rev. A + Crow Modem 51000143 Rev. D */
#define PLATFORM_INTEGRATION_KIT  3   /* Nivis ISA100/WirelessHart Integration Kit platform 52000158 Rev. A + Crow Modem 51000143 Rev. E */
#define PLATFORM_GE_DEVICE        4   /* GE Device */
#define PLATFORM_YOKOGAWA         5   /* YOKOGAWA ISA100 simple API 51000160 + Crow Modem 51000143 rev.E */
#define PLATFORM_HN_RAPTOR        6   /* Nivis ISA100 Raptor COMM Interface 52000127 Rev. D + Crow Development Board (Raptor form factor) 52000144 Rev. A + Crow Modem 51000143 Rev. D */
#define PLATFORM_HN_BBR           7   /* Nivis ISA100 Raptor COMM Interface 52000127 Rev. D + Crow Development Board (Raptor form factor) 52000144 Rev. A + Crow Modem 51000143 Rev. D */



#endif /* _TYPEDEF_H_ */




