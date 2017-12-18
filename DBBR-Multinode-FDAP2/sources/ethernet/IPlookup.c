
#include <string.h>
#include "IPlookup.h"
#include "../isa100/provision.h"
#include "../isa100/dmap_dmo.h"

typedef union
  {
      uint16  asShort[3];
      uint8   asByte[6];
  }T_IPV4_MC;
    
typedef  struct{
      uint32	ulIP;		//4bytes
      T_IPV4_MC	mMAC;		//8bytes but only 6 utilized
  }T_IP4_MAC_MAP;

  
#define NB_OF_ENTRIES   8 
T_IP4_MAC_MAP gIP4MacMap[NB_OF_ENTRIES];
uint8 g_ucAddIdx = 0;

__arm const uint8 * GetMacOfIP ( uint32 p_ulIP )
{
   T_IP4_MAC_MAP * pIpv4Map = gIP4MacMap;
   
    if( (p_ulIP & c_ulMASK) != (c_ulIP & c_ulMASK)) {
        p_ulIP = c_ulGWY;  
    }
   
   do
   {
      if (p_ulIP == pIpv4Map->ulIP ) 
      {
          return pIpv4Map->mMAC.asByte;
      }
  }
  while( (++pIpv4Map) < gIP4MacMap + NB_OF_ENTRIES);
  
  return NULL;
}

void AddMacOfIP(const uint16 * p_pIP, const uint16 * p_pMAC)  //se executa numai dupa RenewMacOfIP
{
    uint32 ulIP = p_pIP[0] + ((uint32)(p_pIP[1]) << 16);
    
    if( !RenewMacOfIP(ulIP,p_pMAC ) )
    {
        T_IP4_MAC_MAP * pIpv4Map = gIP4MacMap + g_ucAddIdx;
        
        pIpv4Map->ulIP = ulIP;
        pIpv4Map->mMAC.asShort[0] = p_pMAC[0];
        pIpv4Map->mMAC.asShort[1] = p_pMAC[1];
        pIpv4Map->mMAC.asShort[2] = p_pMAC[2];
    
        if( (++g_ucAddIdx) >= NB_OF_ENTRIES )
            g_ucAddIdx = 1; //  =0; protect first entry (SM)
    }
}

unsigned char RenewMacOfIP(uint32 p_ulIP, const uint16* p_pMAC)
{
   T_IP4_MAC_MAP * pIpv4Map = gIP4MacMap;
   
   do
   {
      if (p_ulIP == pIpv4Map->ulIP) 
      {
         pIpv4Map->mMAC.asShort[0] =  p_pMAC[0];
         pIpv4Map->mMAC.asShort[1] =  p_pMAC[1];
         pIpv4Map->mMAC.asShort[2] =  p_pMAC[2];
         return 1;
      }
  }
  while( (++pIpv4Map) < gIP4MacMap + NB_OF_ENTRIES);
  
  return 0;
}

void ClearListMacOfIP( void )
{
  memset( gIP4MacMap, 0, sizeof(gIP4MacMap) );
  g_ucAddIdx = 0;
}

