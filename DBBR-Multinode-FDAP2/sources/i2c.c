/****************************************************
* Name:        i2c.c
* Author:      Eugen GHICA
* Date:        September 2008  
* Description: I2C Module Interface
* Changes:     
* Revisions:   
****************************************************/

// Include files
#include "i2c.h"
#include "wdt.h"
#include "timers.h"
#include "digitals.h"
#include "system.h"
//#include "ISA100/tmr_util.h"

//-------------------------------------------
// Define I2C constants
#define OP_WRITE                0
#define OP_READ                 1
#define DEVICE_SELECT_CODE      (0xA0 >> 1)    // M24C04 - binary: 1 0 1 0 E2 E1 A8 - 
#define I2C_TIMEOUT             1000    // communication timeout

#define I2C_ENABLE_PIO()        AT91C_BASE_PIOA->PIO_PDR = AT91C_PA10_TWD | AT91C_PA11_TWCK; AT91C_BASE_PIOA->PIO_ASR = AT91C_PA10_TWD | AT91C_PA11_TWCK
#define I2C_ENABLE_DRV()        AT91C_BASE_PIOA->PIO_MDER = AT91C_PA10_TWD | AT91C_PA11_TWCK; AT91C_BASE_PIOA->PIO_PPUER = AT91C_PA10_TWD | AT91C_PA11_TWCK; 
#define I2C_ENABLE_CLK()        AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_TWI
#define I2C_ENABLE()            I2C_ENABLE_PIO(); I2C_ENABLE_DRV(); I2C_ENABLE_CLK()

#define I2C_DATA_HI()           AT91C_BASE_PIOA->PIO_SODR = AT91C_PA10_TWD;
#define I2C_DATA_LO()           AT91C_BASE_PIOA->PIO_CODR = AT91C_PA10_TWD;
#define I2C_CLK_HI()            AT91C_BASE_PIOA->PIO_SODR = AT91C_PA11_TWCK;
#define I2C_CLK_LO()            AT91C_BASE_PIOA->PIO_CODR = AT91C_PA11_TWCK;

// MCK = (3*18432000L) = 55.296 MHz

// Tlow = ((CLDIV x 2 ^^ CKDIV )+ 3) × TMCK -> 
// at 400 KHz tLow = 1/400 000/2 = 1.25 us -> 
// ((CLDIV x 2 ^^ CKDIV )+ 3) = MCK  / 800000
// CLDIV = MCK  / 800000 - 3

//#define TWI_CKDIV_400K      0
//#define TWI_CHDIV_400K      ((MCK/800000 - 3) + 1)
//#define TWI_CLDIV_400K      TWI_CHDIV_400K

#define TWI_CKDIV_400K 0
#define TWI_CKDIV_100K 2
#define TWI_CHDIV      (unsigned int)(MCK/800000)
#define TWI_CLDIV      (unsigned int)(TWI_CHDIV)

// TLow = ( 69*4 + 3 ) / 55.295 = 5 us -> 100 kHz

// i2c 400KBits/sec
#define EEPROM_CLDIV        TWI_CLDIV
#define EEPROM_CHDIV        (TWI_CHDIV << 8)
#define EEPROM_CKDIV        (TWI_CKDIV_400K << 16)

//-------------------------------------------  
// I2C Initialize clock
#define I2C_CLOCK()         AT91C_BASE_TWI->TWI_CWGR = EEPROM_CLDIV | EEPROM_CHDIV | EEPROM_CKDIV

//-------------------------------------------  
// I2C Master/Slave Mode
#define I2C_MASTERMODE()    AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSEN
#define I2C_SLAVEMODE()     AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSDIS

//-------------------------------------------  
// I2C Transmit/Receive Mode
#define I2C_TXMODE()        AT91C_BASE_TWI->TWI_MMR &= ~AT91C_TWI_MREAD
#define I2C_RXMODE()        AT91C_BASE_TWI->TWI_MMR |= AT91C_TWI_MREAD  

//-------------------------------------------  
// I2C Generate a START/STOP condition
#define I2C_START()         AT91C_BASE_TWI->TWI_CR = AT91C_TWI_START
#define I2C_STOP()          AT91C_BASE_TWI->TWI_CR = AT91C_TWI_STOP
#define I2C_START_STOP()    AT91C_BASE_TWI->TWI_CR = AT91C_TWI_START | AT91C_TWI_STOP

//-------------------------------------------  
// I2C Interrupts ON/OFF
#define I2C_IRQ_ON()        AT91C_BASE_TWI->TWI_IER = AT91C_TWI_TXCOMP | AT91C_TWI_RXRDY | AT91C_TWI_TXRDY | AT91C_TWI_NACK
#define I2C_IRQ_OFF()       AT91C_BASE_TWI->TWI_IDR = AT91C_TWI_TXCOMP | AT91C_TWI_RXRDY | AT91C_TWI_TXRDY | AT91C_TWI_NACK

//-------------------------------------------
// I2C Error Codes
#define I2C_OK		    0
#define I2C_NACK            1
#define I2C_RX_TIMEOUT      2
#define I2C_TX_TIMEOUT	    3
#define I2C_COMP_TIMEOUT    4
  

// I2C status flag
uint8  g_chI2CStatusFlag = I2C_OK;
#define SET_I2C_FLAG(FLAG)  { g_chI2CStatusFlag = (FLAG); }

uint8 i2c_FlushEEPROM(void);

//////////////////////////////////////////////////////////////////////////////////
// Name: i2c_selectAddress4Read
// Author: Eugen GHICA
// Description: prepare adddress for read
// Returns: none
////////////////////////////////////////////////////////////////////////////////// 
void i2c_selectDeviceAddress(uint16 p_nAddress)
{    
    // Master mode
    I2C_MASTERMODE();
        
    MONITOR_ENTER();

    
    // Device slave address
    AT91C_BASE_TWI->TWI_MMR = ((DEVICE_SELECT_CODE | (p_nAddress >> 8) ) << 16 )
//                              | AT91C_TWI_MREAD
                                ;    

    while( ! (AT91C_BASE_TWI->TWI_SR & AT91C_TWI_TXRDY ) )
        ;            
    
    // internal buffer address 
    AT91C_BASE_TWI->TWI_THR = (uint8)p_nAddress;
    
    MONITOR_EXIT();
}


//////////////////////////////////////////////////////////////////////////////////
// Name: i2c_waitForResponse
// Author: Eugen GHICA
// Description: Wait for a response until flag is set or timeout
// Parameters: none
// Returns: 1 - response came on time
//          0 - no response (timeout) or NACK
////////////////////////////////////////////////////////////////////////////////// 
uint8 i2c_waitForResponse(void)
{
    uint16 unTWIStatus;
    do
    {
        unTWIStatus = AT91C_BASE_TWI->TWI_SR;       
        if( unTWIStatus & AT91C_TWI_NACK )
            return 0;
        
        if( unTWIStatus & AT91C_TWI_RXRDY )
            return 1;

        if( unTWIStatus & AT91C_TWI_TXCOMP ) // end of transfer but not RX, abort
            return 0;
        
    }
    while ( 1 );    
}


//////////////////////////////////////////////////////////////////////////////////
// Name: i2c_waitForCompletion
// Author: Eugen GHICA
// Description: Wait for data completion until flag is set or timeout
// Parameters: none
// Returns: 1 - completion came on time
//          0 - no completion (timeout)
////////////////////////////////////////////////////////////////////////////////// 
uint8 i2c_waitForCompletion(void)
{
    while ( !(AT91C_BASE_TWI->TWI_SR & AT91C_TWI_TXCOMP) )
      ;
        
//    I2C_SLAVEMODE();
    return 1;
}

//////////////////////////////////////////////////////////////////////////////////
// Name: I2C_Init
// Author: Eugen GHICA
// Description: Initialize the I2C module
// Parameters:
// Returns: 
////////////////////////////////////////////////////////////////////////////////// 
void I2C_Init(void)
{
    I2C_ENABLE();     // peripheral control    
    I2C_SLAVEMODE();
    I2C_IRQ_OFF();    // No interrupts - timeouts used
    I2C_CLOCK();
    
//    I2C_DATA_HI();
//    I2C_CLK_HI();
    
}

///////////////////////////////////////////////////////////////////////////////////
// Name: i2c_ReadEEPROM
// Author: Eugen GHICA
// Description: Reads a sequence of bytes
// Parameters: p_nAddress  - unsigned short - the address from which to read; contain device address
//             p_pchDst    - unsigned char* - the buffer in which to write
//             p_nSize     - unsigned short - the number of characters to be read from memory  
// Returns:  0 if fail, 1 if success
/////////////////////////////////////////////////////////////////////////////////// 
uint16 i2c_ReadEEPROM (uint16 p_nAddress, uint8 * p_pchDst, uint16 p_nSize)
{    
//    if (p_nAddress >= EEPROM_MEM_SIZE)
//        return 0;
    
//    if ( p_nAddress + p_nSize > EEPROM_MEM_SIZE )
//        p_nSize = EEPROM_MEM_SIZE - p_nAddress;
    
    i2c_selectDeviceAddress( p_nAddress );
    i2c_waitForCompletion();    
    
    I2C_MASTERMODE();
    I2C_RXMODE();             
    
    if( p_nSize == 1 )
    {
        I2C_START_STOP();
        if ( !i2c_waitForResponse() )                 // timeout ?
            return 0;
        
        *p_pchDst = AT91C_BASE_TWI->TWI_RHR;  // copy into buffer                        
    }
    else
    {
        I2C_START();
        while( p_nSize-- )
        {
            if ( !p_nSize )                     // last byte follows?
            {
                I2C_STOP();
            }
            
            if ( !i2c_waitForResponse() )                 // timeout ?
                return 0;
            
            *(p_pchDst++) = AT91C_BASE_TWI->TWI_RHR;  // copy into buffer                        
        }
    }
    
    i2c_waitForCompletion();    
    return 1;
}

//////////////////////////////////////////////////////////////////////////////////
// Name: i2c_WriteEEPROM
// Author: Eugen GHICA
// Description: Writes a sequence of bytes
// Parameters: p_nAddress  - unsigned short - the address to which will write; contain device address
//             p_pchSrc    - unsigned char* - the buffer from which to read
//             p_nSize     - unsigned short - the number of characters to be written to memory  
// Returns:  0 if fail, 1 if success
//////////////////////////////////////////////////////////////////////////////////
uint16 i2c_WriteEEPROM(uint16 p_nAddress, const uint8 * p_pchSrc, uint16 p_nSize)
{
//    if (p_nAddress >= EEPROM_MEM_SIZE)
//        return 0;
    
//    if ( p_nAddress + p_nSize > EEPROM_MEM_SIZE )
//        p_nSize = EEPROM_MEM_SIZE - p_nAddress;
  
 
    volatile unsigned int unDelay;
  
    while (p_nSize)
    {
        // write pages
      
        // calculate length to end of current write (end of a page or end of the entire writing)
        uint8 nCrtWriteLen = EEPROM_PAGE_SIZE-(p_nAddress & (EEPROM_PAGE_SIZE-1));
        if ( nCrtWriteLen > p_nSize )
            nCrtWriteLen = p_nSize;
                
          
        I2C_EEPROM_WRITE_ENABLE();          
        
        I2C_MASTERMODE();
        
        MONITOR_ENTER();
            
        // Device slave address
        AT91C_BASE_TWI->TWI_MMR = ((DEVICE_SELECT_CODE | (p_nAddress >> 8) ) << 16 )
    //                              | AT91C_TWI_MREAD
                                    ;    

        while( ! (AT91C_BASE_TWI->TWI_SR & AT91C_TWI_TXRDY ) )
            ;            
        
        // internal buffer address 
        AT91C_BASE_TWI->TWI_THR = (uint8)p_nAddress;
                
        // write into current page
        while (nCrtWriteLen)
        {
            if( AT91C_BASE_TWI->TWI_SR & AT91C_TWI_NACK )
            {
                I2C_EEPROM_WRITE_DISABLE();  
                MONITOR_EXIT();
                return 0;
            }
                        
            while( ! (AT91C_BASE_TWI->TWI_SR & AT91C_TWI_TXRDY ) )
                ;            
                        
            AT91C_BASE_TWI->TWI_THR = *(p_pchSrc++);  // copy from buffer
            
            nCrtWriteLen--;
            
            // recalculate parameters
            p_nAddress++;
            p_nSize--;            
        }
               
        
        MONITOR_EXIT();
        
        // wait completion
        i2c_waitForCompletion();
        
        I2C_EEPROM_WRITE_DISABLE();
        
        if( !i2c_FlushEEPROM() )
        {
            return 0;
        }
                
    }
        
    return 1;
}

#define TMR_CLK_10MS           (MCK/1024/100)

uint8 i2c_FlushEEPROM(void)
{
//    //maximum 5 ms requested by EEPROM for page write processing
//    //to be sure we wait 10 ms
//    
//    uint32 ulCrtTime = TIMER_Get250msOffset();
//    
//    if( ulCrtTime + TMR_CLK_10MS > TMR_CLK_250MS )
//    {
//        ulCrtTime = TMR_CLK_10MS - (TMR_CLK_250MS - ulCrtTime); 
//    
//        while( ulCrtTime < TIMER_Get250msOffset() );
//    }
//    else
//    {
//        ulCrtTime += TMR_CLK_10MS;
//    }
//    
//    while( ulCrtTime > TIMER_Get250msOffset() );
//    
//    return 1;
    
    uint8 ucTmp;
    uint16 unTimeout = I2C_TIMEOUT; 
    while( unTimeout-- )
    {
        FEED_WDT();
        if( i2c_ReadEEPROM(0,&ucTmp,1) )
            return i2c_ReadEEPROM(0,&ucTmp,1);  //for clear state
    }
    
    return 0;   
}
