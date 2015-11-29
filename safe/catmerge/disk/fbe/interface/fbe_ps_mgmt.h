#ifndef FBE_PS_MGMT_H
#define FBE_PS_MGMT_H

#include "fbe/fbe_types.h"

fbe_status_t fbe_ps_mgmt_fup_build_image_file_name(fbe_u8_t * pImageFileNameBuffer,
                                                   char * pImageFileNameConstantPortion,
                                                   fbe_u8_t bufferLen,
                                                   fbe_u8_t * pSubenclProductId,
                                                   fbe_u32_t * pImageFileNameLen);

fbe_status_t 
fbe_ps_mgmt_updatePsFailStatus(fbe_u8_t *mgmtPtr,
                               fbe_device_physical_location_t *pLocation,
                               fbe_u64_t deviceType,
                               fbe_bool_t newValue);

#endif /* FBE_PS_MGMT_H */
