#ifndef MCR_SUPPORT_H
#define MCR_SUPPORT_H
#include "fbe/fbe_types.h"


fbe_status_t mcr_support_init(bool isBaseSuite);
fbe_status_t mcr_support_store_ProcessIDs(void);
void mcr_support_destroy_transport(void);
void mcr_support_destroy_sim(void);

#endif /*MCR_SUPPORT_H*/

