#ifndef FBE_SPS_MGMT_H
#define FBE_SPS_MGMT_H

#include "fbe/fbe_types.h"

fbe_status_t 
fbe_sps_mgmt_updateBbuFailStatus(fbe_u8_t *mgmtPtr,
                                 fbe_device_physical_location_t *pLocation,
                                 fbe_u64_t deviceType,
                                 fbe_bool_t newValue);

#endif /* FBE_SPS_MGMT_H */
