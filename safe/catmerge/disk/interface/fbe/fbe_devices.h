#ifndef FBE_DEVICES_H
#define FBE_DEVICES_H

#include "fbe/fbe_types.h"
#include "fbe_device_types.h"

#define FBE_XPE_PSEUDO_BUS_NUM    0xFE
#define FBE_XPE_PSEUDO_ENCL_NUM   0xFE
#define FBE_DEVICE_STRING_LENGTH    200

typedef struct fbe_device_physical_location_s {    
    fbe_bool_t  processorEnclosure;
    fbe_u8_t    bus;
    fbe_u8_t    enclosure;
    fbe_u8_t    componentId;
    fbe_s8_t    bank;
    fbe_u8_t    bank_slot;
    fbe_u8_t    slot;
    fbe_u8_t sp;
    fbe_u8_t port;
} fbe_device_physical_location_t;


typedef enum fbe_device_data_type_e {    
    FBE_DEVICE_DATA_TYPE_INVALID   =   0,
    FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
    FBE_DEVICE_DATA_TYPE_PORT_INFO,
    FBE_DEVICE_DATA_TYPE_FUP_INFO,
    FBE_DEVICE_DATA_TYPE_RESUME_PROM_INFO
} fbe_device_data_type_t;

/*!********************************************************************* 
 * @enum fbe_sps_cid_enum_t 
 *  
 * @brief 
 *   This enum includes the SPS comonent id used by the SPS firmware upgrade.
 *
 **********************************************************************/
typedef enum fbe_sps_cid_e { 
    FBE_SPS_COMPONENT_ID_PRIMARY = 0,
    FBE_SPS_COMPONENT_ID_SECONDARY = 1,
    FBE_SPS_COMPONENT_ID_BATTERY = 2,
    FBE_SPS_COMPONENT_ID_MAX = 3
} fbe_sps_cid_enum_t;

#endif 


