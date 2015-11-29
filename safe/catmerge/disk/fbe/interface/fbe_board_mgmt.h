#ifndef FBE_BOARD_MGMT_H
#define FBE_BOARD_MGMT_H

#include "fbe/fbe_types.h"


fbe_status_t 
fbe_board_mgmt_updateSpFailStatus(fbe_u8_t *mgmtPtr,
                                  fbe_device_physical_location_t *pLocation,
                                  fbe_base_environment_failure_type_t failureType,
                                  fbe_bool_t newValue);

#endif /* FBE_BOARD_MGMT_H */
