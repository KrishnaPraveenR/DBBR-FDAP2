/*************************************************************************
* File: packet_combine.c
* Author: Honeywell
* Packet Combining Algorithm implementation for Radio Dicersity
*************************************************************************/
#ifdef BBR2_HW

#include "typedef.h"
#include "packet_combine.h"
#include ".\isa100\mlde.h"
#include "asm.h"
// Maximum number of errors that can be corrected as per current time limits
#define MAX_PKT_LEN                           (128)
#define MIN_PHY_PAYLOAD_LEN                   (8)

extern uint8 NoRxCounter[2];

static uint16 crc_additive_terms_msg1[MAX_NO_OF_ERROR_CORRECTION];
static uint16 crc_additive_terms_msg2[MAX_NO_OF_ERROR_CORRECTION];
static uint16 crc_terms_arr[MAX_NO_OF_ERROR_CORRECTION];
static uint8 err_positions_arr[MAX_NO_OF_ERROR_CORRECTION];

#pragma data_alignment=2
static uint8 base_message[MAX_PHY_PAYLOAD_LEN+3];
//static uint16 TimeOutInTimerTicks;

DIVERSITY_METRICS DiversityMetrics;

const uint8 *comb_iter_arr_ptr;
const uint8 *arr_ptr;

#pragma data_alignment=2
const uint8 comb_iterations_arr2[1] = {2};
const uint8 comb_iterations_arr3[2] = {3,3};
const uint8 comb_iterations_arr4[3] = {4, 6, 4};
const uint8 comb_iterations_arr5[4] = {5,10,10,5};
const uint8 comb_iterations_arr6[5] = {6,15,20,15,6};

// Combinations are calcualted as per the formula Summation from i = 1 to N-1 for (i* (N Combinations i))
const uint8 comb_arr2[4] = {0,1,
                /*unused*/  0, 0};

const uint8 comb_arr3[12]= {0, 1, 2,
                            0, 1, 0, 2, 1, 2,
                /*unused*/  0, 0, 0};

const uint8 comb_arr4[28]= {0, 1, 2, 3,
                      0, 1, 0, 2, 0, 3, 1, 2, 1, 3, 2, 3,
                      0, 1, 2, 0, 1, 3, 0, 2, 3, 1, 2, 3};

const uint8 comb_arr5[80] = {0, 1, 2, 3, 4,
                       0, 1, 0, 2, 0, 3, 0, 4, 1, 2, 1, 3, 1, 4, 2, 3, 2, 4, 3, 4,
                       0, 1, 2, 0, 1, 3, 0, 1, 4, 0, 2, 3, 0, 2, 4, 0, 3, 4, 1, 2, 3, 1, 2, 4, 1, 3, 4, 2, 3, 4,
                             0, 1, 2, 3, 0, 1, 2, 4, 0, 1, 3, 4, 0, 2, 3, 4, 1, 2, 3, 4,
                 /*unused*/  0, 0, 0, 0, 0} ;

uint8 comb_arr6[200] = {0, 1, 2, 3, 4, 5, 0, 1, 0, 2, 0,
                        3, 0, 4, 0, 5, 1, 2, 1, 3, 1, 4,
                        1, 5, 2, 3, 2, 4, 2, 5, 3, 4, 3,
                        5, 4, 5, 0, 1, 2, 0, 1, 3, 0, 1,
                        4, 0, 1, 5, 0, 2, 3, 0, 2, 4, 0,
                        2, 5, 0, 3, 4, 0, 3, 5, 0, 4, 5,
                        1, 2, 3, 1, 2, 4, 1, 2, 5, 1, 3,
                        4, 1, 3, 5, 1, 4, 5, 2, 3, 4, 2,
                        3, 5, 2, 4, 5, 3, 4, 5, 0, 1, 2,
                        3, 0, 1, 2, 4, 0, 1, 2, 5, 0, 1,
                        3, 4, 0, 1, 3, 5, 0, 1, 4, 5, 0,
                        2, 3, 4, 0, 2, 3, 5, 0, 2, 4, 5,
                        0, 3, 4, 5, 1, 2, 3, 4, 1, 2, 3,
                        5, 1, 2, 4, 5, 1, 3, 4, 5, 2, 3,
                        4, 5, 0, 1, 2, 3, 4, 0, 1, 2, 3,
                        5, 0, 1, 2, 4, 5, 0, 1, 3, 4, 5,
                        0, 2, 3, 4, 5, 1, 2, 3, 4, 5, 
          /*unused*/    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


uint16 fast_crc_table_ccitt[MAX_PKT_LEN] = {0x9295,0x36c4,0x407e,0xb5b7,0x52cf,0x2da7,0x4355,0xfa0d,
                                    0xa73f,0xd24c,0xc00b,0x6d7d,0xd0a6,0x60bc,0x3fa7,0x4347,
                                    0xad27,0xcbc5,0x6397,0x701d,0xb6b7,0x52cc,0x489b,0xf4e7,
                                    0x0784,0x0447,0xad60,0x66a1,0x8966,0xacfd,0x5877,0x9e3a,
                                    0x7d31,0x106f,0x87f1,0xdc8d,0x2f09,0x2bba,0xf594,0x15b7,
                                    0x526f,0x87b3,0xdead,0x0d0f,0xe1e0,0xeefd,0x5835,0x9c1a,
                                    0x5f37,0xda35,0x9c98,0x910f,0xe17c,0xf33e,0xf10c,0x8420,
                                    0x2280,0x8832,0x75a6,0x6019,0x3af7,0x1648,0x4c9f,0x78b3,
                                    0xde52,0x13fc,0x7bdc,0x59b0,0xbb4f,0xa55e,0x9756,0x9fe5,
                                    0x41c7,0x2535,0x9c67,0x8ffc,0x7b40,0x4473,0x1276,0xbd64,
                                    0xeae1,0xcde2,0xa8f9,0xd423,0x47ec,0x6a8a,0xc6d3,0xb8e0,
                                    0xeea4,0x26aa,0xe49b,0xf44b,0x2941,0x6735,0x9c25,0x8ddc,
                                    0x5946,0x8e29,0x091f,0xf0e6,0x2494,0x1566,0xac61,0x45b4,
                                    0x3703,0x650b,0x6dd8,0xd5f6,0x35b3,0xde1f,0xf031,0x10e2,
                                    0xa824,0xaefc,0x7b61,0x4563,0x0375,0xd849,0x6f45,0xeb23,
                                    0x47d3,0xb861,0x45a0,0xaa51,0x76b4,0x3730,0x3331,0x1021};

uint16 crc_table_kermit[256] = {0x0000,0x1189,0x2312,0x329b,0x4624,0x57ad,0x6536,0x74bf,
                                0x8c48,0x9dc1,0xaf5a,0xbed3,0xca6c,0xdbe5,0xe97e,0xf8f7,
                                0x1081,0x0108,0x3393,0x221a,0x56a5,0x472c,0x75b7,0x643e,
                                0x9cc9,0x8d40,0xbfdb,0xae52,0xdaed,0xcb64,0xf9ff,0xe876,
                                0x2102,0x308b,0x0210,0x1399,0x6726,0x76af,0x4434,0x55bd,
                                0xad4a,0xbcc3,0x8e58,0x9fd1,0xeb6e,0xfae7,0xc87c,0xd9f5,
                                0x3183,0x200a,0x1291,0x0318,0x77a7,0x662e,0x54b5,0x453c,
                                0xbdcb,0xac42,0x9ed9,0x8f50,0xfbef,0xea66,0xd8fd,0xc974,
                                0x4204,0x538d,0x6116,0x709f,0x0420,0x15a9,0x2732,0x36bb,
                                0xce4c,0xdfc5,0xed5e,0xfcd7,0x8868,0x99e1,0xab7a,0xbaf3,
                                0x5285,0x430c,0x7197,0x601e,0x14a1,0x0528,0x37b3,0x263a,
                                0xdecd,0xcf44,0xfddf,0xec56,0x98e9,0x8960,0xbbfb,0xaa72,
                                0x6306,0x728f,0x4014,0x519d,0x2522,0x34ab,0x0630,0x17b9,
                                0xef4e,0xfec7,0xcc5c,0xddd5,0xa96a,0xb8e3,0x8a78,0x9bf1,
                                0x7387,0x620e,0x5095,0x411c,0x35a3,0x242a,0x16b1,0x0738,
                                0xffcf,0xee46,0xdcdd,0xcd54,0xb9eb,0xa862,0x9af9,0x8b70,
                                0x8408,0x9581,0xa71a,0xb693,0xc22c,0xd3a5,0xe13e,0xf0b7,
                                0x0840,0x19c9,0x2b52,0x3adb,0x4e64,0x5fed,0x6d76,0x7cff,
                                0x9489,0x8500,0xb79b,0xa612,0xd2ad,0xc324,0xf1bf,0xe036,
                                0x18c1,0x0948,0x3bd3,0x2a5a,0x5ee5,0x4f6c,0x7df7,0x6c7e,
                                0xa50a,0xb483,0x8618,0x9791,0xe32e,0xf2a7,0xc03c,0xd1b5,
                                0x2942,0x38cb,0x0a50,0x1bd9,0x6f66,0x7eef,0x4c74,0x5dfd,
                                0xb58b,0xa402,0x9699,0x8710,0xf3af,0xe226,0xd0bd,0xc134,
                                0x39c3,0x284a,0x1ad1,0x0b58,0x7fe7,0x6e6e,0x5cf5,0x4d7c,
                                0xc60c,0xd785,0xe51e,0xf497,0x8028,0x91a1,0xa33a,0xb2b3,
                                0x4a44,0x5bcd,0x6956,0x78df,0x0c60,0x1de9,0x2f72,0x3efb,
                                0xd68d,0xc704,0xf59f,0xe416,0x90a9,0x8120,0xb3bb,0xa232,
                                0x5ac5,0x4b4c,0x79d7,0x685e,0x1ce1,0x0d68,0x3ff3,0x2e7a,
                                0xe70e,0xf687,0xc41c,0xd595,0xa12a,0xb0a3,0x8238,0x93b1,
                                0x6b46,0x7acf,0x4854,0x59dd,0x2d62,0x3ceb,0x0e70,0x1ff9,
                                0xf78f,0xe606,0xd49d,0xc514,0xb1ab,0xa022,0x92b9,0x8330,
                                0x7bc7,0x6a4e,0x58d5,0x495c,0x3de3,0x2c6a,0x1ef1,0x0f78};

const DMAP_FCT_STRUCT c_aDOFct[DO_ATTR_NO] = {
   0,   0,                                                  DMAP_EmptyReadFunc,    NULL,  
   ATTR_CONST(DiversityMetrics.RxDPDUCount),                DMAP_ReadUint32,       NULL,   
   ATTR_CONST(DiversityMetrics.Radio1RxDPDUCount),          DMAP_ReadUint32,       NULL,   
   ATTR_CONST(DiversityMetrics.Radio2RxDPDUCount),          DMAP_ReadUint32,       NULL,     
   ATTR_CONST(DiversityMetrics.ErrorPacketCount),           DMAP_ReadUint32,       NULL,   
   ATTR_CONST(DiversityMetrics.CorrectedPacketsCount),      DMAP_ReadUint32,       NULL,
   &DiversityMetrics.ErrorDistributionCount[0], 
     sizeof(DiversityMetrics.ErrorDistributionCount),       Read_DO_EDC_ID,        NULL,
   ATTR_CONST(DiversityMetrics.CorrectionGain),             Read_DO_CG_ID,         NULL,
   ATTR_CONST(DiversityMetrics.RedundancyGain),             Read_DO_RG_ID,         NULL,
   ATTR_CONST(DiversityMetrics.DiversityOperation),         Read_DO_DO_ID,         NULL,
   0,    0,                                                 DMAP_EmptyReadFunc,    Write_DO_ID
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function: Read_DO_EDC_ID
////////////////////////////////////////////////////////////////////////////////////////////////////
void Read_DO_EDC_ID(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize)
{ 
  for (uint8 Index = 0; Index < MAX_NO_OF_ERROR_CORRECTION-1; Index++)
  {
    // Copy 2,3,4,5,6 Error Bytes Data in to the Response Buffer
    *(p_pBuf++) = (*(uint32*)(p_pValue)) >> 24;
    *(p_pBuf++) = (*(uint32*)(p_pValue)) >> 16;
    *(p_pBuf++) = (*(uint32*)(p_pValue)) >> 8;
    *(p_pBuf++) = (*(uint32*)(p_pValue)); 
    p_pValue = ((uint32 *)p_pValue) + 1;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function: Read_DO_CG_ID
////////////////////////////////////////////////////////////////////////////////////////////////////
void Read_DO_CG_ID(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize)
{ 
  float temp;  
  temp = (float)DiversityMetrics.CorrectedPacketsCount/(float)DiversityMetrics.ErrorPacketCount;  
  // Update data and copy it to response buffer
  DiversityMetrics.CorrectionGain = 0xFF & (uint8)(temp * 100);
  DMAP_ReadUint8(p_pValue, p_pBuf, p_ucSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function: Read_DO_RG_ID
////////////////////////////////////////////////////////////////////////////////////////////////////
void Read_DO_RG_ID(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize)
{
  float temp;  
  
  // Update Redundancy gain and then copy it to response buffer
  if (DiversityMetrics.Radio1RxDPDUCount > DiversityMetrics.Radio2RxDPDUCount)
  {
    temp = (float)DiversityMetrics.Radio2RxDPDUCount/(float)DiversityMetrics.RxDPDUCount;
    DiversityMetrics.RedundancyGain = 0xFF & (uint8)(temp * 100);
  }
  else
  {
    temp = (float)DiversityMetrics.Radio1RxDPDUCount/(float)DiversityMetrics.RxDPDUCount;
    DiversityMetrics.RedundancyGain = 0xFF & (uint8)(temp * 100);
  }
  
  DiversityMetrics.RedundancyGain = 100 - DiversityMetrics.RedundancyGain;
  
  DMAP_ReadUint8(p_pValue, p_pBuf, p_ucSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function: Read_DO_DO_ID
////////////////////////////////////////////////////////////////////////////////////////////////////
void Read_DO_DO_ID(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize)
{
  if (NoRxCounter[1] >= MAX_RETRY_RADIO_ERROR_CHECK)
  {
    DiversityMetrics.DiversityOperation = CC2520_RADIO_1;
  }
  else if (NoRxCounter[0] >= MAX_RETRY_RADIO_ERROR_CHECK)
  {
    DiversityMetrics.DiversityOperation = CC2520_RADIO_2;
  }
  else
  {
    // Enum value is 0 for this
    DiversityMetrics.DiversityOperation = 0;
  }
  
  DMAP_ReadUint8(p_pValue, p_pBuf, p_ucSize);
}     
////////////////////////////////////////////////////////////////////////////////////////////////////
// Function: Write_DO_ID
////////////////////////////////////////////////////////////////////////////////////////////////////
void Write_DO_ID(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize)
{ 
  // If command is to reset then reset the diversiy metrics
  if (0 != (*p_pBuf))
  {
    // Reset all counters
    memset(&DiversityMetrics, 0, sizeof(DiversityMetrics));
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function: Compute_CRC_Kermit
////////////////////////////////////////////////////////////////////////////////////////////////////
uint16 Compute_CRC_Kermit(uint16 InitialCRC,  uint8 *DataPtr, uint8 DataLength)
{
  uint8 tbl_idx;

  while (DataLength--)
  {
    tbl_idx = InitialCRC ^ *DataPtr;
    InitialCRC = (crc_table_kermit[tbl_idx] ^ (InitialCRC >> 8));
    DataPtr++;
  }

  return InitialCRC;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function: Reflect2Bytes
////////////////////////////////////////////////////////////////////////////////////////////////////
uint16 Reflect2Bytes(uint16 data)
{
  uint8 b1,b2;
  uint16  reflection = 0;
  b1 = data&0xff;
  b2 = (data>>8)&0xff;
  b1 = ((b1 * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32;
  b2 = ((b2 * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32;
  reflection = (b2 << 8) | (b1);
  return (reflection);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Function: reflect_char
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 reflect_char(uint8 data)
{
  uint8 reflection;
  reflection = ((data * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32;
  return (reflection);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function: GaloisConvolution
////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long GaloisConvolution(unsigned short a, unsigned char b)
{
  unsigned char i;
  const unsigned char one = 0x01;
  unsigned long sum = 0,tsum;

  tsum = a;

  for(i = 0; i < 8; i++)
  {
    if(b&one)
    {
      sum = sum ^ tsum;
    }
    tsum = tsum << 1;
    b = b >> 1;
  }

  return sum;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function: rmod
////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned short rmod(unsigned long a)
{
  unsigned short result;
  int i;
  unsigned long r;
  unsigned long kermit = 0x11021;
  unsigned long mask   = 0x10000;

  if(a <= mask)
    return a;
  else
  {
    r = a;
    kermit = kermit << 7;
    mask = mask << 7;
    for(i = 0; i <= 7; i++)
    {
      if(r >= mask)
        r = r ^ kermit;

      kermit = kermit >> 1;
      mask = mask >> 1;
    }
  }

  result = r & 0xffff;
  return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function: printc
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8  printc(uint8 *ptrBuffer1, uint8 *ptrBuffer2, uint8 ErrorIteration, uint16 ReferenceCRC, 
              uint8 num_of_errors, uint16 crc_base_msg_mirror)
{
  uint8 crc_done   = CORRECTION_FAILURE;
  uint16 final_crc = 0;
  uint16 final_crc_mirror = 0;

  int i,z;
  const uint8 *arr_ptr_current;

  arr_ptr_current = arr_ptr;
//  arr_ptr_3 = arr_ptr_1;

  for (i = 0; i < ErrorIteration; ++i)
  {
    crc_terms_arr[arr_ptr[0]] = crc_additive_terms_msg1[arr_ptr[0]];
    arr_ptr++;
  }

  final_crc = crc_base_msg_mirror;

  for (z=0;z<num_of_errors; z++)
  {
    final_crc = final_crc^crc_terms_arr[z];
  }

  final_crc_mirror =  Reflect2Bytes(final_crc);

  if(final_crc_mirror == ReferenceCRC)
  {
    crc_done = CORRECTION_SUCCESSFULL;
    
    for (i = 0; i < ErrorIteration; ++i)
    {
      ptrBuffer2[err_positions_arr[arr_ptr_current[0]]] = ptrBuffer1[ err_positions_arr[arr_ptr_current[0]]];
      arr_ptr_current++;
    }
  }

  if(crc_done == CORRECTION_FAILURE)
  {
    for (i = 0; i < ErrorIteration; ++i)
    {
      crc_terms_arr[arr_ptr_current[0]] = crc_additive_terms_msg2[arr_ptr_current[0]];
      arr_ptr_current++;
    }
  }

  return crc_done;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function: TryCombinations
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 TryCombinations(uint16 ErrorIteration, uint8 *ptrBuffer1, uint8 *ptrBuffer2,
                      uint16 ReferenceCRC, uint8 num_of_errors, uint16 crc_base_msg_mirror)

{
  int p, q;

  p = *comb_iter_arr_ptr++;

  for(q = 0; q < p; q++)
  {
//    if (AT91C_BASE_TC1->TC_CV >= TimeOutInTimerTicks )
//      return CORRECTION_TIMEOUT;
    
    if(CORRECTION_SUCCESSFULL == printc(ptrBuffer1, ptrBuffer2, ErrorIteration, ReferenceCRC,
                                        num_of_errors, crc_base_msg_mirror))
    {
      return CORRECTION_SUCCESSFULL;
    }
  }

  return CORRECTION_FAILURE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function: packet_combine
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 packet_combine(uint8 *ptrBuffer1, uint8 *ptrBuffer2, uint8 DataLength,
                     uint16 ReferenceCRC)
{
  uint8 i, j;
  uint8 num_of_errors = 0;
  uint8 pkt_correction_flag = CORRECTION_FAILURE;
  uint16 FastOffset = MAX_PKT_LEN - DataLength;

  // Check if the datalength is in the valid packet size limits
  // Minimum packet length is 8 bytes so anythign which is less than this is not 
  // considered for correction
  if (DataLength > MAX_PHY_PAYLOAD_LEN ||
      DataLength < MIN_PHY_PAYLOAD_LEN)
  {
    return CORRECTION_FAILURE;
  }
  
  // Generate Base Message and compute the number of errors in the received message
  for(i=0,j=0; i < DataLength; i++)
  {
    base_message[i] = ptrBuffer2[i];

    if( ptrBuffer1[i] != ptrBuffer2[i])
    {
      num_of_errors++;

      // Maximum of 6 bytes are supported as of now
      if (num_of_errors > MAX_NO_OF_ERROR_CORRECTION)
      {
        return CORRECTION_FAILURE;
      }
      base_message[i] = 0;
      err_positions_arr[j++] = i;
    }
  }

  // Try error correction only if number of errors is more than 1 and less than 7
  if( (num_of_errors > 1) && (num_of_errors <= MAX_NO_OF_ERROR_CORRECTION))
  {
    uint8 err_index;
    unsigned long  conv_msg1   = 0;
    unsigned long  conv_msg2   = 0;
    uint16 crc_base_msg        = 0;
    uint16 crc_base_msg_mirror = 0;
    
    uint16 crc_hi, crc_lo;
    // Base Msg CRC
    crc_base_msg = Compute_CRC_Kermit(0, base_message, DataLength);
    crc_hi = crc_base_msg >> 8;
    crc_lo = (crc_base_msg & 0x00ff) << 8;
    crc_base_msg = crc_hi | crc_lo;

    crc_base_msg_mirror = Reflect2Bytes(crc_base_msg);

    // Compute Additive CRCs
    for(j = 0; j < num_of_errors; j++)
    {
      err_index = err_positions_arr[j];
      conv_msg1 = GaloisConvolution(fast_crc_table_ccitt[err_index+FastOffset], reflect_char(ptrBuffer1[err_index]));
      crc_additive_terms_msg1[j]  = rmod(conv_msg1);
      conv_msg2 = GaloisConvolution(fast_crc_table_ccitt[err_index+FastOffset], reflect_char(ptrBuffer2[err_index]));
      crc_additive_terms_msg2[j]  = rmod(conv_msg2);
      crc_terms_arr[j] = crc_additive_terms_msg2[j];
    }

    switch(num_of_errors)
    {
      case 2:
        arr_ptr =  &comb_arr2[0];
        comb_iter_arr_ptr = &comb_iterations_arr2[0];
        DiversityMetrics.ErrorDistributionCount[0]++;
        break;
      case 3:
        arr_ptr =  &comb_arr3[0];
        comb_iter_arr_ptr = &comb_iterations_arr3[0];
        DiversityMetrics.ErrorDistributionCount[1]++;
        break;
      case 4:
        arr_ptr =  &comb_arr4[0];
        comb_iter_arr_ptr = &comb_iterations_arr4[0];
        DiversityMetrics.ErrorDistributionCount[2]++;
        break;
      case 5:
        arr_ptr =  &comb_arr5[0];
        comb_iter_arr_ptr = &comb_iterations_arr5[0];
        DiversityMetrics.ErrorDistributionCount[3]++;
        break;
      case 6:
        arr_ptr =  &comb_arr6[0];
        comb_iter_arr_ptr = &comb_iterations_arr6[0];
        DiversityMetrics.ErrorDistributionCount[4]++;
        break;
      default:
       return CORRECTION_FAILURE;
    }

    for(i = 0; i < num_of_errors; i++)
    {
      crc_terms_arr[i] = crc_additive_terms_msg2[i];
    }

    // Start Trying Combinations for each number of errors
    for(i = 1; i < num_of_errors; i++)
    {
      pkt_correction_flag = TryCombinations(i, ptrBuffer1, ptrBuffer2, ReferenceCRC, 
                                            num_of_errors, crc_base_msg_mirror);

      // Check if the Correction is successfull
      if(pkt_correction_flag==CORRECTION_SUCCESSFULL )
      {
        break;
      }
    }
  }
  return pkt_correction_flag;
}
#endif 