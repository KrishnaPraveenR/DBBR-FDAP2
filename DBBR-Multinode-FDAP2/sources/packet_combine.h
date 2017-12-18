/*************************************************************************
* File: packet_combine.h
* Author: Honeywell 
* Packet Combining Algorithm implementation for Radio Dicersity
*************************************************************************/

#ifndef PACKET_COMBINE_H
#define PACKET_COMBINE_H

#ifdef BBR2_HW
#include "typedef.h"
//#include "at91sam7x512.h"
#include "isa100\dmap_utils.h"

#define CORRECTION_SUCCESSFULL           0
#define CORRECTION_FAILURE               1
#define CORRECTION_TIMEOUT               2

#define MAX_NO_OF_ERROR_CORRECTION            (6)
#define MAX_RETRY_RADIO_ERROR_CHECK           (21) // Based on complete one SF

typedef struct strDiversityMetrics
{
  uint32 RxDPDUCount;                     // Total Packets correctly received by either radio1 or radio2
  uint32 Radio1RxDPDUCount;               // Total Packets correctly received by radio 1 alone
  uint32 Radio2RxDPDUCount;               // Total Packets correctly received by radio 2 alone
  uint32 Radio1RxErrorCount;              // Total Packets received with CRC error on radio 1 
  uint32 Radio2RxErrorCount;              // Total Packets received with CRC error on radio 2 
  uint32 ErrorPacketCount;                // Total packets received with CRC error on both radios
  uint32 CorrectedPacketsCount;           // Total packets corrected by packet error correction algorithm
  uint32 ErrorDistributionCount[MAX_NO_OF_ERROR_CORRECTION-1]; // Each index corresponds to count of error distribution in bytes for 2, 3, 4, 5 and 6 bytes in error
  uint8  CorrectionGain;                  // Percentage Correction Gain of successfully corrected packets
  uint8  RedundancyGain;                  // Percentage Redundancy Gain 
  uint8  DiversityOperation;              // Currently Active Radio/Radios (0-Both/1-Radio1/2-Radio2)
  uint8  Dummy;                           // Just to maintain proper 4 byte Allignment 
}DIVERSITY_METRICS;

enum
{
  DO_TOTAL_RX_DPDU_COUNTER_ID       = 1,
  DO_RADIO1_RX_DPDU_COUNTER_ID      = 2,
  DO_RADIO2_RX_DPDU_COUNTER_ID      = 3,
  DO_ERROR_PACKET_COUNTER_ID        = 4,  
  DO_CORRECTED_PACKETS_COUNTER_ID   = 5,
  DO_ERROR_DISTRIBUTION_COUNTER_ID  = 6,    
  DO_CORRECTION_GAIN_ID             = 7,
  DO_REDUNDANCY_GAIN_ID             = 8,
  DO_DIVERSITY_OPERATION_ID         = 9,
  DO_RESET_STATISTICS_ID            = 10,
  DO_ATTR_NO                        = 11
}; // DO_ATTR_IDS

void Read_DO_EDC_ID(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);
void Read_DO_CG_ID(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);
void Read_DO_RG_ID(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);
void Write_DO_ID(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);
void Read_DO_DO_ID(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);


uint8 packet_combine(uint8 *ptrBuffer1, uint8 *ptrBuffer2, uint8 DataLength,
                     uint16 ReferenceCRC);

uint16 Compute_CRC_Kermit(uint16 InitialCRC,  uint8 *DataPtr, uint8 DataLength);

extern const DMAP_FCT_STRUCT c_aDOFct[DO_ATTR_NO];

#define DO_Read(p_unAttrID,p_punBufferSize,p_pucRspBuffer) \
            DMAP_ReadAttr(p_unAttrID,p_punBufferSize,p_pucRspBuffer,c_aDOFct,DO_ATTR_NO)

#define DO_Write(p_unAttrID,p_ucBufferSize,p_pucBuffer)   \
            DMAP_WriteAttr(p_unAttrID,p_ucBufferSize,p_pucBuffer,c_aDOFct,DO_ATTR_NO)

#endif  // end of BBR2_HW

#endif
