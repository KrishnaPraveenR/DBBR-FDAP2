#ifndef _AT91SAM7X512NIVIS_H
#define _AT91SAM7X512NIVIS_H

#include "at91sam7x512.h"

#define BIT00                (0x00000001)
#define BIT01                (0x00000002)
#define BIT02                (0x00000004)
#define BIT03                (0x00000008)
#define BIT04                (0x00000010)
#define BIT05                (0x00000020)
#define BIT06                (0x00000040)
#define BIT07                (0x00000080)

#define BIT08                (0x00000100)
#define BIT09                (0x00000200)
#define BIT10                (0x00000400)
#define BIT11                (0x00000800)
#define BIT12                (0x00001000)
#define BIT13                (0x00002000)
#define BIT14                (0x00004000)
#define BIT15                (0x00008000)

#define BIT16                (0x00010000)
#define BIT17                (0x00020000)
#define BIT18                (0x00040000)
#define BIT19                (0x00080000)
#define BIT20                (0x00100000)
#define BIT21                (0x00200000)
#define BIT22                (0x00400000)
#define BIT23                (0x00800000)

#define BIT24                (0x01000000)
#define BIT25                (0x02000000)
#define BIT26                (0x04000000)
#define BIT27                (0x08000000)
#define BIT28                (0x10000000)
#define BIT29                (0x20000000)
#define BIT30                (0x40000000)
#define BIT31                (0x80000000)

#define BIT0                BIT00
#define BIT1                BIT01
#define BIT2                BIT02
#define BIT3                BIT03
#define BIT4                BIT04
#define BIT5                BIT05
#define BIT6                BIT06
#define BIT7                BIT07
#define BIT8                BIT08
#define BIT9                BIT09
#define BITA                BIT10
#define BITB                BIT11
#define BITC                BIT12
#define BITD                BIT13
#define BITE                BIT14
#define BITF                BIT15

typedef volatile unsigned int AT91_REG; // Hardware register definition

// Internal SRAM
#define ISRAM	              (0x00200000) // Internal SRAM base address
#define ISRAM_SIZE	      (0x00020000) // Internal SRAM size in byte (128 Kbytes)
// Internal FLASH
#define IFLASH	                  (0x00100000)  // Internal FLASH base address
#define IFLASH_SIZE	          (0x00080000)  // Internal FLASH size in byte (512 Kbytes)
#define IFLASH_PAGE_SIZE	  (256)         // Internal FLASH Page Size: 256 bytes
#define IFLASH_LOCK_REGION_SIZE	  (16384)       // Internal FLASH Lock Region Size: 16 Kbytes
#define IFLASH_NB_OF_PAGES	  (2048)        // Internal FLASH Number of Pages: 2048 bytes
#define IFLASH_NB_OF_LOCK_BITS	  (32)          // Internal FLASH Number of Lock Bits: 32 bytes

//AIC Advanced Interrupt Controller
typedef struct _AT91S_AIC {
	AT91_REG	 SMR[32]; 	// Source Mode Register
	AT91_REG	 SVR[32]; 	// Source Vector Register
	AT91_REG	 IVR; 	        // IRQ Vector Register
	AT91_REG	 FVR; 	        // FIQ Vector Register
	AT91_REG	 ISR; 	        // Interrupt Status Register
	AT91_REG	 IPR; 	        // Interrupt Pending Register
	AT91_REG	 IMR; 	        // Interrupt Mask Register
	AT91_REG	 CISR; 	        // Core Interrupt Status Register
	AT91_REG	 Reserved0[2]; 	// 
	AT91_REG	 IECR; 	        // Interrupt Enable Command Register
	AT91_REG	 IDCR; 	        // Interrupt Disable Command Register
	AT91_REG	 ICCR; 	        // Interrupt Clear Command Register
	AT91_REG	 ISCR; 	        // Interrupt Set Command Register
	AT91_REG	 EOICR; 	// End of Interrupt Command Register
	AT91_REG	 SPU; 	        // Spurious Vector Register
	AT91_REG	 DCR; 	        // Debug Control Register (Protect)
	AT91_REG	 Reserved1[1]; 	// 
	AT91_REG	 FFER; 	        // Fast Forcing Enable Register
	AT91_REG	 FFDR; 	        // Fast Forcing Disable Register
	AT91_REG	 FFSR; 	        // Fast Forcing Status Register
} AT91S_AIC, *AT91PS_AIC;
#define AIC_BASE_ADDR   (0xFFFFF000)
#define MODULE_AIC      (*((AT91PS_AIC) AIC_BASE_ADDR))

//PIOA Parallel I/O Controller A
typedef struct _AT91S_PIO {
	AT91_REG	 PER; 	        // PIO Enable Register
	AT91_REG	 PDR; 	        // PIO Disable Register
	AT91_REG	 PSR; 	        // PIO Status Register
	AT91_REG	 Reserved0[1]; 	// 
	AT91_REG	 OER; 	        // Output Enable Register
	AT91_REG	 ODR; 	        // Output Disable Registerr
	AT91_REG	 OSR; 	        // Output Status Register
	AT91_REG	 Reserved1[1]; 	// 
	AT91_REG	 IFER; 	        // Input Filter Enable Register
	AT91_REG	 IFDR; 	        // Input Filter Disable Register
	AT91_REG	 IFSR; 	        // Input Filter Status Register
	AT91_REG	 Reserved2[1]; 	// 
	AT91_REG	 SODR; 	        // Set Output Data Register
	AT91_REG	 CODR; 	        // Clear Output Data Register
	AT91_REG	 ODSR; 	        // Output Data Status Register
	AT91_REG	 PDSR; 	        // Pin Data Status Register
	AT91_REG	 IER; 	        // Interrupt Enable Register
	AT91_REG	 IDR; 	        // Interrupt Disable Register
	AT91_REG	 IMR; 	        // Interrupt Mask Register
	AT91_REG	 ISR; 	        // Interrupt Status Register
	AT91_REG	 MDER; 	        // Multi-driver Enable Register
	AT91_REG	 MDDR; 	        // Multi-driver Disable Register
	AT91_REG	 MDSR; 	        // Multi-driver Status Register
	AT91_REG	 Reserved3[1]; 	// 
	AT91_REG	 PPUDR; 	// Pull-up Disable Register
	AT91_REG	 PPUER; 	// Pull-up Enable Register
	AT91_REG	 PPUSR; 	// Pull-up Status Register
	AT91_REG	 Reserved4[1]; 	// 
	AT91_REG	 ASR; 	        // Select A Register
	AT91_REG	 BSR; 	        // Select B Register
	AT91_REG	 ABSR; 	        // AB Select Status Register
	AT91_REG	 Reserved5[9]; 	// 
	AT91_REG	 OWER; 	        // Output Write Enable Register
	AT91_REG	 OWDR; 	        // Output Write Disable Register
	AT91_REG	 OWSR; 	        // Output Write Status Register
} AT91S_PIO, *AT91PS_PIO;
#define PIOA_BASE_ADDR   (0xFFFFF400)
#define MODULE_PIOA      (*((AT91PS_PIO) PIOA_BASE_ADDR))

//PIOB Parallel I/O Controller B
#define PIOB_BASE_ADDR   (0xFFFFF600)
#define MODULE_PIOB      (*((AT91PS_PIO) PIOB_BASE_ADDR))

//SPI0 Serial Peripheral Interface 0
typedef struct _AT91S_SPI {
	AT91_REG	  CR; 	        // Control Register
	AT91_REG	  MR; 	        // Mode Register
	AT91_REG	  RDR; 	        // Receive Data Register
	AT91_REG	  TDR; 	        // Transmit Data Register
	AT91_REG	  SR; 	        // Status Register
	AT91_REG	  IER; 	        // Interrupt Enable Register
	AT91_REG	  IDR; 	        // Interrupt Disable Register
	AT91_REG	  IMR; 	        // Interrupt Mask Register
	AT91_REG	  Reserved0[4]; // 
	AT91_REG	  CSR[4]; 	// Chip Select Register
	AT91_REG	  Reserved1[48];// 
	AT91_REG	  RPR; 	        // Receive Pointer Register
	AT91_REG	  RCR; 	        // Receive Counter Register
	AT91_REG	  TPR; 	        // Transmit Pointer Register
	AT91_REG	  TCR; 	        // Transmit Counter Register
	AT91_REG	  RNPR; 	// Receive Next Pointer Register
	AT91_REG	  RNCR; 	// Receive Next Counter Register
	AT91_REG	  TNPR; 	// Transmit Next Pointer Register
	AT91_REG	  TNCR; 	// Transmit Next Counter Register
	AT91_REG	  PTCR; 	// PDC Transfer Control Register
	AT91_REG	  PTSR; 	// PDC Transfer Status Register
} AT91S_SPI, *AT91PS_SPI;
#define SPI0_BASE_ADDR    (0xFFFE0000)
#define MODULE_SPI0       (*((AT91PS_SPI) SPI0_BASE_ADDR))

//SPI1 Serial Peripheral Interface 1
#define SPI1_BASE_ADDR    (0xFFFE4000)
#define MODULE_SPI1       (*((AT91PS_SPI) SPI1_BASE_ADDR))

//US0 USART 0
typedef struct _AT91S_USART {
	AT91_REG	 CR; 	        // Control Register
	AT91_REG	 MR; 	        // Mode Register
	AT91_REG	 IER; 	        // Interrupt Enable Register
	AT91_REG	 IDR; 	        // Interrupt Disable Register
	AT91_REG	 IMR; 	        // Interrupt Mask Register
	AT91_REG	 CSR; 	        // Channel Status Register
	AT91_REG	 RHR; 	        // Receiver Holding Register
	AT91_REG	 THR; 	        // Transmitter Holding Register
	AT91_REG	 BRGR; 	        // Baud Rate Generator Register
	AT91_REG	 RTOR; 	        // Receiver Time-out Register
	AT91_REG	 TTGR; 	        // Transmitter Time-guard Register
	AT91_REG	 Reserved0[5]; 	// 
	AT91_REG	 FIDI; 	        // FI_DI_Ratio Register
	AT91_REG	 NER; 	        // Nb Errors Register
	AT91_REG	 Reserved1[1]; 	// 
	AT91_REG	 IF; 	        // IRDA_FILTER Register
	AT91_REG	 Reserved2[44]; // 
	AT91_REG	 RPR; 	        // Receive Pointer Register
	AT91_REG	 RCR; 	        // Receive Counter Register
	AT91_REG	 TPR; 	        // Transmit Pointer Register
	AT91_REG	 TCR; 	        // Transmit Counter Register
	AT91_REG	 RNPR; 	        // Receive Next Pointer Register
	AT91_REG	 RNCR; 	        // Receive Next Counter Register
	AT91_REG	 TNPR; 	        // Transmit Next Pointer Register
	AT91_REG	 TNCR; 	        // Transmit Next Counter Register
	AT91_REG	 PTCR; 	        // PDC Transfer Control Register
	AT91_REG	 PTSR; 	        // PDC Transfer Status Register
} AT91S_USART, *AT91PS_USART;
#define US0_BASE_ADDR    (0xFFFC0000)
#define MODULE_USART0    (*((AT91PS_USART) US0_BASE_ADDR))

//US1 USART 1
#define US1_BASE_ADDR    (0xFFFC4000)
#define MODULE_USART1    (*((AT91PS_USART) US1_BASE_ADDR))

//SSC Synchronous Serial Controller
typedef struct _AT91S_SSC {
	AT91_REG	 CR; 	            // Control Register
	AT91_REG	 CMR; 	            // Clock Mode Register
	AT91_REG	 Reserved0[2]; 	    // 
	AT91_REG	 RCMR; 	            // Receive Clock ModeRegister
	AT91_REG	 RFMR; 	            // Receive Frame Mode Register
	AT91_REG	 TCMR; 	            // Transmit Clock Mode Register
	AT91_REG	 TFMR; 	            // Transmit Frame Mode Register
	AT91_REG	 RHR; 	            // Receive Holding Register
	AT91_REG	 THR; 	            // Transmit Holding Register
	AT91_REG	 Reserved1[2]; 	    // 
	AT91_REG	 RSHR; 	            // Receive Sync Holding Register
	AT91_REG	 TSHR; 	            // Transmit Sync Holding Register
	AT91_REG	 Reserved2[2]; 	    // 
	AT91_REG	 SR; 	            // Status Register
	AT91_REG	 IER; 	            // Interrupt Enable Register
	AT91_REG	 IDR; 	            // Interrupt Disable Register
	AT91_REG	 IMR; 	            // Interrupt Mask Register
	AT91_REG	 Reserved3[44];     // 
	AT91_REG	 RPR; 	            // Receive Pointer Register
	AT91_REG	 RCR; 	            // Receive Counter Register
	AT91_REG	 TPR; 	            // Transmit Pointer Register
	AT91_REG	 TCR; 	            // Transmit Counter Register
	AT91_REG	 RNPR; 	            // Receive Next Pointer Register
	AT91_REG	 RNCR; 	            // Receive Next Counter Register
	AT91_REG	 TNPR; 	            // Transmit Next Pointer Register
	AT91_REG	 TNCR; 	            // Transmit Next Counter Register
	AT91_REG	 PTCR; 	            // PDC Transfer Control Register
	AT91_REG	 PTSR; 	            // PDC Transfer Status Register
} AT91S_SSC, *AT91PS_SSC;
#define SSC_BASE_ADDR     (0xFFFD4000)
#define MODULE_SSC        (*((AT91PS_SSC) SSC_BASE_ADDR))

//TWI Two-wire Interface
typedef struct _AT91S_TWI {
	AT91_REG	 CR; 	            // Control Register
	AT91_REG	 MMR; 	            // Master Mode Register
	AT91_REG	 Reserved0[1]; 	    // 
	AT91_REG	 IADR; 	            // Internal Address Register
	AT91_REG	 CWGR; 	            // Clock Waveform Generator Register
	AT91_REG	 Reserved1[3]; 	    // 
	AT91_REG	 SR; 	            // Status Register
	AT91_REG	 IER; 	            // Interrupt Enable Register
	AT91_REG	 IDR; 	            // Interrupt Disable Register
	AT91_REG	 IMR; 	            // Interrupt Mask Register
	AT91_REG	 RHR; 	            // Receive Holding Register
	AT91_REG	 THR; 	            // Transmit Holding Register
	AT91_REG	 Reserved2[50];     // 
	AT91_REG	 RPR; 	            // Receive Pointer Register
	AT91_REG	 RCR; 	            // Receive Counter Register
	AT91_REG	 TPR; 	            // Transmit Pointer Register
	AT91_REG	 TCR; 	            // Transmit Counter Register
	AT91_REG	 RNPR; 	            // Receive Next Pointer Register
	AT91_REG	 RNCR; 	            // Receive Next Counter Register
	AT91_REG	 TNPR; 	            // Transmit Next Pointer Register
	AT91_REG	 TNCR; 	            // Transmit Next Counter Register
	AT91_REG	 PTCR; 	            // PDC Transfer Control Register
	AT91_REG	 PTSR; 	            // PDC Transfer Status Register
} AT91S_TWI, *AT91PS_TWI;
#define TWI_BASE_ADDR     (0xFFFB8000)
#define MODULE_TWI        (*((AT91PS_TWI) TWI_BASE_ADDR))

//PWMC Pulse Width Modulation Controller
typedef struct _AT91S_PWMC_CH {
	AT91_REG	    CMR;          // Channel Mode Register
	AT91_REG	    CDTYR; 	  // Channel Duty Cycle Register
	AT91_REG	    CPRDR; 	  // Channel Period Register
	AT91_REG	    CCNTR; 	  // Channel Counter Register
	AT91_REG	    CUPDR; 	  // Channel Update Register
	AT91_REG	    Reserved[3];  // Reserved
} AT91S_PWMC_CH, *AT91PS_PWMC_CH;
typedef struct _AT91S_PWMC {
	AT91_REG	    MR; 	      // PWMC Mode Register
	AT91_REG	    ENA; 	      // PWMC Enable Register
	AT91_REG	    DIS; 	      // PWMC Disable Register
	AT91_REG	    SR; 	      // PWMC Status Register
	AT91_REG	    IER; 	      // PWMC Interrupt Enable Register
	AT91_REG	    IDR; 	      // PWMC Interrupt Disable Register
	AT91_REG	    IMR; 	      // PWMC Interrupt Mask Register
	AT91_REG	    ISR; 	      // PWMC Interrupt Status Register
	AT91_REG	    Reserved0[55];    // 
	AT91_REG	    VR; 	      // PWMC Version Register
	AT91_REG	    Reserved1[64];    // 
	AT91S_PWMC_CH	    CH[4]; 	      // PWMC Channel
} AT91S_PWMC, *AT91PS_PWMC;
#define PWMC_BASE_ADDR      (0xFFFCC000)
#define MODULE_PWMC         (*((AT91PS_PWMC) PWMC_BASE_ADDR)) 
#define PWMC3_BASE_ADDR     (0xFFFCC260) 
#define MODULE_PWMC_CH3     (*((AT91PS_PWMC_CH) PWMC3_BASE_ADDR))
#define PWMC2_BASE_ADDR     (0xFFFCC240) 
#define MODULE_PWMC_CH2     (*((AT91PS_PWMC_CH) PWMC2_BASE_ADDR))
#define PWMC1_BASE_ADDR     (0xFFFCC220) 
#define MODULE_PWMC_CH1     (*((AT91PS_PWMC_CH) PWMC1_BASE_ADDR))
#define PWMC0_BASE_ADDR     (0xFFFCC200) 
#define MODULE_PWMC_CH0     (*((AT91PS_PWMC_CH) PWMC0_BASE_ADDR))

//UDP USB Device Port
typedef struct _AT91S_UDP {
	AT91_REG	 NUM; 	// Frame Number Register
	AT91_REG	 GLBSTATE; 	// Global State Register
	AT91_REG	 FADDR; 	// Function Address Register
	AT91_REG	 Reserved0[1]; 	// 
	AT91_REG	 IER; 	// Interrupt Enable Register
	AT91_REG	 IDR; 	// Interrupt Disable Register
	AT91_REG	 IMR; 	// Interrupt Mask Register
	AT91_REG	 ISR; 	// Interrupt Status Register
	AT91_REG	 ICR; 	// Interrupt Clear Register
	AT91_REG	 Reserved1[1]; 	// 
	AT91_REG	 RSTEP; 	// Reset Endpoint Register
	AT91_REG	 Reserved2[1]; 	// 
	AT91_REG	 CSR[6]; 	// Endpoint Control and Status Register
	AT91_REG	 Reserved3[2]; 	// 
	AT91_REG	 FDR[6]; 	// Endpoint FIFO Data Register
	AT91_REG	 Reserved4[3]; 	// 
	AT91_REG	 TXVC; 	// Transceiver Control Register
} AT91S_UDP, *AT91PS_UDP;
#define UDP_BASE_ADDR    (0xFFFB0000)
#define MODULEE_UDP      (*((AT91PS_UDP) UDP_BASE_ADDR))

//TC0 Timer/Counter 0
typedef struct _AT91S_TC {
	AT91_REG	 CCR; 	        // Channel Control Register
	AT91_REG	 CMR; 	        // Channel Mode Register (Capture Mode / Waveform Mode)
	AT91_REG	 Reserved0[2]; 	// 
	AT91_REG	 CV; 	        // Counter Value
	AT91_REG	 RA; 	        // Register A
	AT91_REG	 RB; 	        // Register B
	AT91_REG	 RC; 	        // Register C
	AT91_REG	 SR; 	        // Status Register
	AT91_REG	 IER; 	        // Interrupt Enable Register
	AT91_REG	 IDR; 	        // Interrupt Disable Register
	AT91_REG	 IMR; 	        // Interrupt Mask Register
} AT91S_TC, *AT91PS_TC;
#define TC0_BASE_ADDR     (0xFFFA0000)
#define MODULE_TC0        (*((AT91PS_TC) TC0_BASE_ADDR))

//TC1 Timer/Counter 1
#define TC1_BASE_ADDR     (0xFFFA0040)
#define MODULE_TC1        (*((AT91PS_TC) TC1_BASE_ADDR))

//TC2 Timer/Counter 2
#define TC2_BASE_ADDR     (0xFFFA0080)
#define MODULE_TC2        (*((AT91PS_TC) TC2_BASE_ADDR))

//TCB
typedef struct _AT91S_TCB {
	AT91S_TC	  TC0; 	            // TC Channel 0
	AT91_REG	  Reserved0[4];     // 
	AT91S_TC	  TC1; 	            // TC Channel 1
	AT91_REG	  Reserved1[4];     // 
	AT91S_TC	  TC2; 	            // TC Channel 2
	AT91_REG	  Reserved2[4];     // 
	AT91_REG	  BCR; 	            // TC Block Control Register
	AT91_REG	  BMR; 	            // TC Block Mode Register
} AT91S_TCB, *AT91PS_TCB;
#define TCB_BASE_ADDR     (0xFFFA0000)
#define MODULE_TCB    (*((AT91PS_TCB) TCB_BASE_ADDR))

//CAN CAN Controller
typedef struct _AT91S_CAN_MB {
	AT91_REG	  MMR; 	  // MailBox Mode Register
	AT91_REG	  MAM; 	  // MailBox Acceptance Mask Register
	AT91_REG	  MID; 	  // MailBox ID Register
	AT91_REG	  MFID;   // MailBox Family ID Register
	AT91_REG	  MSR; 	  // MailBox Status Register
	AT91_REG	  MDL; 	  // MailBox Data Low Register
	AT91_REG	  MDH; 	  // MailBox Data High Register
	AT91_REG	  MCR; 	  // MailBox Control Register
} AT91S_CAN_MB, *AT91PS_CAN_MB;
typedef struct _AT91S_CAN {
	AT91_REG	  MR; 	    // Mode Register
	AT91_REG	  IER; 	    // Interrupt Enable Register
	AT91_REG	  IDR; 	    // Interrupt Disable Register
	AT91_REG	  IMR; 	    // Interrupt Mask Register
	AT91_REG	  SR; 	    // Status Register
	AT91_REG	  BR; 	    // Baudrate Register
	AT91_REG	  TIM; 	    // Timer Register
	AT91_REG	  TIMESTP;  // Time Stamp Register
	AT91_REG	  ECR; 	    // Error Counter Register
	AT91_REG	  TCR; 	    // Transfer Command Register
	AT91_REG	  ACR; 	    // Abort Command Register
	AT91_REG	  Reserved0[52]; 	// 
	AT91_REG	  VR; 	    // Version Register
	AT91_REG	  Reserved1[64]; 	// 
	AT91S_CAN_MB	  MB[16];   // CAN Mailbox 0..15
} AT91S_CAN, *AT91PS_CAN;
#define CAN_BASE_ADDR     (0xFFFD0000)
#define MODULE_CAN        (*((AT91PS_CAN) CAN_BASE_ADDR))
#define CAN_MB0_BASE_ADDR (0xFFFD0200)
#define MODULE_CAN_MB0    (*((AT91PS_CAN_MB) CAN_MB0_BASE_ADDR))
#define CAN_MB1_BASE_ADDR (0xFFFD0220)
#define MODULE_CAN_MB1    (*((AT91PS_CAN_MB) CAN_MB1_BASE_ADDR))
#define CAN_MB2_BASE_ADDR (0xFFFD0240)
#define MODULE_CAN_MB2    (*((AT91PS_CAN_MB) CAN_MB2_BASE_ADDR))
#define CAN_MB3_BASE_ADDR (0xFFFD0260)
#define MODULE_CAN_MB3    (*((AT91PS_CAN_MB) CAN_MB3_BASE_ADDR))
#define CAN_MB4_BASE_ADDR (0xFFFD0280)
#define MODULE_CAN_MB4    (*((AT91PS_CAN_MB) CAN_MB4_BASE_ADDR))
#define CAN_MB5_BASE_ADDR (0xFFFD02A0)
#define MODULE_CAN_MB5    (*((AT91PS_CAN_MB) CAN_MB5_BASE_ADDR))
#define CAN_MB6_BASE_ADDR (0xFFFD02C0)
#define MODULE_CAN_MB6    (*((AT91PS_CAN_MB) CAN_MB6_BASE_ADDR))
#define CAN_MB7_BASE_ADDR (0xFFFD02E0)
#define MODULE_CAN_MB7    (*((AT91PS_CAN_MB) CAN_MB7_BASE_ADDR))

//EMAC Ethernet MAC
typedef struct _AT91S_EMAC {
	AT91_REG	  NCR; 	            // Network Control Register
	AT91_REG	  NCFGR; 	    // Network Configuration Register
	AT91_REG	  NSR; 	            // Network Status Register
	AT91_REG	  Reserved0[2];     // 
	AT91_REG	  TSR; 	            // Transmit Status Register
	AT91_REG	  RBQP; 	    // Receive Buffer Queue Pointer
	AT91_REG	  TBQP; 	    // Transmit Buffer Queue Pointer
	AT91_REG	  RSR; 	            // Receive Status Register
	AT91_REG	  ISR; 	            // Interrupt Status Register
	AT91_REG	  IER; 	            // Interrupt Enable Register
	AT91_REG	  IDR; 	            // Interrupt Disable Register
	AT91_REG	  IMR; 	            // Interrupt Mask Register
	AT91_REG	  MAN; 	            // PHY Maintenance Register
	AT91_REG	  PTR; 	            // Pause Time Register
	AT91_REG	  PFR; 	            // Pause Frames received Register
	AT91_REG	  FTO; 	            // Frames Transmitted OK Register
	AT91_REG	  SCF; 	            // Single Collision Frame Register
	AT91_REG	  MCF; 	            // Multiple Collision Frame Register
	AT91_REG	  FRO; 	            // Frames Received OK Register
	AT91_REG	  FCSE; 	    // Frame Check Sequence Error Register
	AT91_REG	  ALE; 	            // Alignment Error Register
	AT91_REG	  DTF; 	            // Deferred Transmission Frame Register
	AT91_REG	  LCOL; 	    // Late Collision Register
	AT91_REG	  ECOL; 	    // Excessive Collision Register
	AT91_REG	  TUND; 	    // Transmit Underrun Error Register
	AT91_REG	  CSE; 	            // Carrier Sense Error Register
	AT91_REG	  RRE; 	            // Receive Ressource Error Register
	AT91_REG	  ROV; 	            // Receive Overrun Errors Register
	AT91_REG	  RSE; 	            // Receive Symbol Errors Register
	AT91_REG	  ELE; 	            // Excessive Length Errors Register
	AT91_REG	  RJA; 	            // Receive Jabbers Register
	AT91_REG	  USF; 	            // Undersize Frames Register
	AT91_REG	  STE; 	            // SQE Test Error Register
	AT91_REG	  RLE; 	            // Receive Length Field Mismatch Register
	AT91_REG	  TPF; 	            // Transmitted Pause Frames Register
	AT91_REG	  HRB; 	            // Hash Address Bottom[31:0]
	AT91_REG	  HRT; 	            // Hash Address Top[63:32]
	AT91_REG	  SA1L; 	    // Specific Address 1 Bottom, First 4 bytes
	AT91_REG	  SA1H; 	    // Specific Address 1 Top, Last 2 bytes
	AT91_REG	  SA2L; 	    // Specific Address 2 Bottom, First 4 bytes
	AT91_REG	  SA2H; 	    // Specific Address 2 Top, Last 2 bytes
	AT91_REG	  SA3L; 	    // Specific Address 3 Bottom, First 4 bytes
	AT91_REG	  SA3H; 	    // Specific Address 3 Top, Last 2 bytes
	AT91_REG	  SA4L; 	    // Specific Address 4 Bottom, First 4 bytes
	AT91_REG	  SA4H; 	    // Specific Address 4 Top, Last 2 bytes
	AT91_REG	  TID; 	            // Type ID Checking Register
	AT91_REG	  TPQ; 	            // Transmit Pause Quantum Register
	AT91_REG	  USRIO; 	    // USER Input/Output Register
	AT91_REG	  WOL; 	            // Wake On LAN Register
	AT91_REG	  Reserved1[13];    // 
	AT91_REG	  REV; 	            // Revision Register
} AT91S_EMAC, *AT91PS_EMAC;
#define EMAC_BASE_ADDR    (0xFFFDC000)
#define MODULE_EMAC      (*((AT91PS_EMAC) EMAC_BASE_ADDR)) // (EMAC) Base Address

//ADC Analog-to Digital Converter
typedef struct _AT91S_ADC {
	AT91_REG	 CR; 	        // ADC Control Register
	AT91_REG	 MR; 	        // ADC Mode Register
	AT91_REG	 Reserved0[2];  // 
	AT91_REG	 CHER; 	        // ADC Channel Enable Register
	AT91_REG	 CHDR; 	        // ADC Channel Disable Register
	AT91_REG	 CHSR; 	        // ADC Channel Status Register
	AT91_REG	 SR; 	        // ADC Status Register
	AT91_REG	 LCDR; 	        // ADC Last Converted Data Register
	AT91_REG	 IER; 	        // ADC Interrupt Enable Register
	AT91_REG	 IDR; 	        // ADC Interrupt Disable Register
	AT91_REG	 IMR; 	        // ADC Interrupt Mask Register
	AT91_REG	 CDR0; 	        // ADC Channel Data Register 0
	AT91_REG	 CDR1; 	        // ADC Channel Data Register 1
	AT91_REG	 CDR2; 	        // ADC Channel Data Register 2
	AT91_REG	 CDR3; 	        // ADC Channel Data Register 3
	AT91_REG	 CDR4; 	        // ADC Channel Data Register 4
	AT91_REG	 CDR5; 	        // ADC Channel Data Register 5
	AT91_REG	 CDR6; 	        // ADC Channel Data Register 6
	AT91_REG	 CDR7; 	        // ADC Channel Data Register 7
	AT91_REG	 Reserved1[44]; // 
	AT91_REG	 RPR; 	        // Receive Pointer Register
	AT91_REG	 RCR; 	        // Receive Counter Register
	AT91_REG	 TPR; 	        // Transmit Pointer Register
	AT91_REG	 TCR; 	        // Transmit Counter Register
	AT91_REG	 RNPR; 	        // Receive Next Pointer Register
	AT91_REG	 RNCR; 	        // Receive Next Counter Register
	AT91_REG	 TNPR; 	        // Transmit Next Pointer Register
	AT91_REG	 TNCR; 	        // Transmit Next Counter Register
	AT91_REG	 PTCR; 	        // PDC Transfer Control Register
	AT91_REG	 PTSR; 	        // PDC Transfer Status Register
} AT91S_ADC, *AT91PS_ADC;
#define ADC_BASE_ADDR   (0xFFFD8000)
#define MODULE_ADC      (*((AT91PS_ADC) ADC_BASE_ADDR))

//PDC Peripheral DMA Controller
typedef struct _AT91S_PDC {
	AT91_REG	    RPR; 	// Receive Pointer Register
	AT91_REG	    RCR; 	// Receive Counter Register
	AT91_REG	    TPR; 	// Transmit Pointer Register
	AT91_REG	    TCR; 	// Transmit Counter Register
	AT91_REG	    RNPR; 	// Receive Next Pointer Register
	AT91_REG	    RNCR; 	// Receive Next Counter Register
	AT91_REG	    TNPR; 	// Transmit Next Pointer Register
	AT91_REG	    TNCR; 	// Transmit Next Counter Register
	AT91_REG	    PTCR; 	// PDC Transfer Control Register
	AT91_REG	    PTSR; 	// PDC Transfer Status Register
} AT91S_PDC, *AT91PS_PDC;
#define PDC_SPI0_BASE_ADDR  (0xFFFE0100)
#define MODULE_PDC_SPI0     (*((AT91PS_PDC) PDC_SPI0_BASE_ADDR))
#define PDC_SPI1_BASE_ADDR  (0xFFFE4100)
#define MODULE_PDC_SPI1     (*((AT91PS_PDC) PDC_SPI1_BASE_ADDR))
#define PDC_US0_BASE_ADDR   (0xFFFC0100)
#define MODULE_PDC_USART0   (*((AT91PS_PDC) PDC_US0_BASE_ADDR))
#define PDC_US1_BASE_ADDR   (0xFFFC4100)
#define MODULE_PDC_USART1   (*((AT91PS_PDC) PDC_US1_BASE_ADDR))
#define PDC_ADC_BASE_ADDR   (0xFFFD8100)
#define MODULE_PDC_ADC      (*((AT91PS_PDC) PDC_ADC_BASE_ADDR))
#define PDC_SSC_BASE_ADDR   (0xFFFD4100)
#define MODULE_PDC_SSC      (*((AT91PS_PDC) PDC_SSC_BASE_ADDR))
#define PDC_DBGU_BASE_ADDR  (0xFFFFF300)
#define MODULE_PDC_DBGU     (*((AT91PS_PDC) PDC_DBGU_BASE_ADDR))

//RSTC Reset Controller
typedef struct _AT91S_RSTC {
	AT91_REG	  RCR; 	// Reset Control Register
	AT91_REG	  RSR; 	// Reset Status Register
	AT91_REG	  RMR; 	// Reset Mode Register
} AT91S_RSTC, *AT91PS_RSTC;
#define RSTC_BASE_ADDR    (0xFFFFFD00)
#define MODULE_RSTC       (*((AT91PS_RSTC) RSTC_BASE_ADDR))

//CKGR  Clock Generator
typedef struct _AT91S_CKGR {
	AT91_REG	  MOR; 	          // Main Oscillator Register
	AT91_REG	  MCFR; 	  // Main Clock  Frequency Register
	AT91_REG	  Reserved0[1];   // 
	AT91_REG	  PLLR; 	  // PLL Register
} AT91S_CKGR, *AT91PS_CKGR;
#define CKGR_BASE_ADDR    (0xFFFFFC20)
#define MODULE_CKGR       (*((AT91PS_CKGR) CKGR_BASE_ADDR))

//PMC Power Management Controler
typedef struct _AT91S_PMC {
	AT91_REG	  SCER; 	  // System Clock Enable Register
	AT91_REG	  SCDR; 	  // System Clock Disable Register
	AT91_REG	  SCSR; 	  // System Clock Status Register
	AT91_REG	  Reserved0[1];   // 
	AT91_REG	  PCER; 	  // Peripheral Clock Enable Register
	AT91_REG	  PCDR; 	  // Peripheral Clock Disable Register
	AT91_REG	  PCSR; 	  // Peripheral Clock Status Register
	AT91_REG	  Reserved1[1];   // 
	AT91_REG	  MOR; 	          // Main Oscillator Register
	AT91_REG	  MCFR; 	  // Main Clock  Frequency Register
	AT91_REG	  Reserved2[1];   // 
	AT91_REG	  PLLR; 	  // PLL Register
	AT91_REG	  MCKR; 	  // Master Clock Register
	AT91_REG	  Reserved3[3];   // 
	AT91_REG	  PCKR[4]; 	  // Programmable Clock Register
	AT91_REG	  Reserved4[4];   // 
	AT91_REG	  IER; 	          // Interrupt Enable Register
	AT91_REG	  IDR; 	          // Interrupt Disable Register
	AT91_REG	  SR; 	          // Status Register
	AT91_REG	  IMR; 	          // Interrupt Mask Register
} AT91S_PMC, *AT91PS_PMC;
#define PMC_BASE_ADDR     (0xFFFFFC00)
#define MODULE_PMC        (*((AT91PS_PMC) PMC_BASE_ADDR))

//RTTC Real Time Timer Controller
typedef struct _AT91S_RTTC {
	AT91_REG	  RTMR; 	// Real-time Mode Register
	AT91_REG	  RTAR; 	// Real-time Alarm Register
	AT91_REG	  RTVR; 	// Real-time Value Register
	AT91_REG	  RTSR; 	// Real-time Status Register
} AT91S_RTTC, *AT91PS_RTTC;
#define RTTC_BASE_ADDR    (0xFFFFFD20)
#define MODULE_RTTC       (*((AT91PS_RTTC) RTTC_BASE_ADDR))

//PITC Periodic Interval Timer Controller
typedef struct _AT91S_PITC {
	AT91_REG	  MR;           // Period Interval Mode Register
    #define PITEN     BIT24
    #define PITIEN    BIT25        
	AT91_REG	  SR;           // Period Interval Status Register
	AT91_REG	  PIVR; 	// Period Interval Value Register
	AT91_REG	  PIIR; 	// Period Interval Image Register
} AT91S_PITC, *AT91PS_PITC;
#define PITC_BASE_ADDR    (0xFFFFFD30)
#define MODULE_PITC      (*((AT91PS_PITC) PITC_BASE_ADDR))

//WDTC Watchdog Timer Controller 
typedef struct _AT91S_WDTC {
	AT91_REG	  WDCR; 	// Watchdog Control Register
	AT91_REG	  WDMR; 	// Watchdog Mode Register
	AT91_REG	  WDSR; 	// Watchdog Status Register
} AT91S_WDTC, *AT91PS_WDTC;
#define WDTC_BASE_ADDR    (0xFFFFFD40)
#define MODULE_WDTC      (*((AT91PS_WDTC) WDTC_BASE_ADDR))

//VREG Voltage Regulator Mode Controller
typedef struct _AT91S_VREG {
	AT91_REG	  MR; 	// Voltage Regulator Mode Register
} AT91S_VREG, *AT91PS_VREG;
#define VREG_BASE_ADDR    (0xFFFFFD60)
#define MODULE_VREG       (*((AT91PS_VREG) VREG_BASE_ADDR))

//EFC Embedded Flash Controller
typedef struct _AT91S_EFC {
	AT91_REG	  FMR; 	// MC Flash Mode Register
	AT91_REG	  FCR; 	// MC Flash Command Register
	AT91_REG	  FSR; 	// MC Flash Status Register
	AT91_REG	  VR; 	// MC Flash Version Register
} AT91S_EFC, *AT91PS_EFC;
#define EFC0_BASE_ADDR    (0xFFFFFF60)
#define MODULE_EFC0       (*((AT91PS_EFC) EFC0_BASE_ADDR))
#define EFC1_BASE_ADDR    (0xFFFFFF70)
#define MODULE_EFC1       (*((AT91PS_EFC) EFC1_BASE_ADDR))

//MC Memory Controller
typedef struct _AT91S_MCF {
	AT91_REG	  FMR; 	// MC Flash Mode Register
	AT91_REG	  FCR; 	// MC Flash Command Register
	AT91_REG	  FSR; 	// MC Flash Status Register
	AT91_REG	  VR; 	// MC Flash Version Register
} AT91S_MCF, *AT91PS_MCF;         
typedef struct _AT91S_MC {
	AT91_REG	  RCR; 	          // MC Remap Control Register
	AT91_REG	  ASR; 	          // MC Abort Status Register
	AT91_REG	  AASR; 	  // MC Abort Address Status Register
	AT91_REG	  Reserved0[1];   // 
	AT91_REG	  PUIA[16]; 	  // MC Protection Unit Area
	AT91_REG	  PUP; 	          // MC Protection Unit Peripherals
	AT91_REG	  PUER; 	  // MC Protection Unit Enable Register
	AT91_REG	  Reserved1[2];   // 
        AT91S_MCF         FLASH[2];       //MC Flash Registers
} AT91S_MC, *AT91PS_MC;
#define MC_BASE_ADDR      (0xFFFFFF00)
#define MODULE_MC         (*((AT91PS_MC) MC_BASE_ADDR))

//DBGU  Debug Unit  
typedef struct _AT91S_DBGU {
	AT91_REG	  CR; 	          // Control Register
	AT91_REG	  MR; 	          // Mode Register
	AT91_REG	  IER; 	          // Interrupt Enable Register
	AT91_REG	  IDR; 	          // Interrupt Disable Register
	AT91_REG	  IMR; 	          // Interrupt Mask Register
	AT91_REG	  CSR; 	          // Channel Status Register
	AT91_REG	  RHR; 	          // Receiver Holding Register
	AT91_REG	  THR; 	          // Transmitter Holding Register
	AT91_REG	  BRGR; 	  // Baud Rate Generator Register
	AT91_REG	  Reserved0[7];   // 
	AT91_REG	  CIDR; 	  // Chip ID Register
	AT91_REG	  EXID; 	  // Chip ID Extension Register
	AT91_REG	  FNTR; 	  // Force NTRST Register
	AT91_REG	  Reserved1[45];  // 
	AT91_REG	  RPR; 	          // Receive Pointer Register
	AT91_REG	  RCR; 	          // Receive Counter Register
	AT91_REG	  TPR; 	          // Transmit Pointer Register
	AT91_REG	  TCR; 	          // Transmit Counter Register
	AT91_REG	  RNPR; 	  // Receive Next Pointer Register
	AT91_REG	  RNCR; 	  // Receive Next Counter Register
	AT91_REG	  TNPR; 	  // Transmit Next Pointer Register
	AT91_REG	  TNCR; 	  // Transmit Next Counter Register
	AT91_REG	  PTCR; 	  // PDC Transfer Control Register
	AT91_REG	  PTSR; 	  // PDC Transfer Status Register
} AT91S_DBGU, *AT91PS_DBGU;
#define DBGU_BASE_ADDR    (0xFFFFF200)
#define MODULE_DBGU       (*((AT91PS_DBGU) DBGU_BASE_ADDR))

#endif //_AT91SAM7X512NIVIS_H
