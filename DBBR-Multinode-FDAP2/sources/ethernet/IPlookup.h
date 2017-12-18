#ifndef _NIVIS_IP_LOOKUP_H_
#define _NIVIS_IP_LOOKUP_H_

#include "../typedef.h"

__arm const uint8 * GetMacOfIP ( uint32 p_ulIP );

void AddMacOfIP(const uint16 * p_pIP, const uint16* p_pMAC);

unsigned char RenewMacOfIP(uint32 p_ulIP, const uint16* p_pMAC);

void ClearListMacOfIP( void );
#endif /* _NIVIS_IP_LOOKUP_H_ */
