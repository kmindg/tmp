/*****************************************************************************
 * Copyright (C) EMC Corp. 2014
 * All rights reserved.
 * Licensed material - Property of EMC Corporation.
 *****************************************************************************/

/*****************************************************************************
 *  fbe_device_types.h
 *****************************************************************************/
#ifndef FBE_DEVICE_TYPES_H
#define FBE_DEVICE_TYPES_H

#define FBE_DEVICE_TYPE_INVALID       0ULL
#define FBE_DEVICE_TYPE_ENCLOSURE     0x1ULL
#define FBE_DEVICE_TYPE_LCC             0x2ULL
#define FBE_DEVICE_TYPE_PS              0x4ULL
#define FBE_DEVICE_TYPE_FAN             0x8ULL
#define FBE_DEVICE_TYPE_SPS             0x10ULL
#define FBE_DEVICE_TYPE_IOMODULE        0x20ULL
#define FBE_DEVICE_TYPE_SSC             0x40ULL
#define FBE_DEVICE_TYPE_SSD             0x80ULL
#define FBE_DEVICE_TYPE_DRIVE          0x100ULL
#define FBE_DEVICE_TYPE_MEZZANINE      0x200ULL
#define FBE_DEVICE_TYPE_MGMT_MODULE    0x400ULL
#define FBE_DEVICE_TYPE_FLT_EXP        0x800ULL
#define FBE_DEVICE_TYPE_SLAVE_PORT     0x1000ULL
#define FBE_DEVICE_TYPE_PLATFORM       0x2000ULL
#define FBE_DEVICE_TYPE_SUITCASE       0x4000ULL
#define FBE_DEVICE_TYPE_MISC           0x8000ULL
#define FBE_DEVICE_TYPE_SFP            0x20000ULL
#define FBE_DEVICE_TYPE_SP             0x40000ULL
#define FBE_DEVICE_TYPE_TEMPERATURE    0x80000ULL
#define FBE_DEVICE_TYPE_PORT_LINK      0x100000ULL
#define FBE_DEVICE_TYPE_SP_ENVIRON_STATE      0x200000ULL           // State of Caching HW
#define FBE_DEVICE_TYPE_CONNECTOR             0x400000ULL
#define FBE_DEVICE_TYPE_DISPLAY               0x800000ULL
#define FBE_DEVICE_TYPE_BACK_END_MODULE       0x1000000ULL
#define FBE_DEVICE_TYPE_BATTERY               0x2000000ULL
#define FBE_DEVICE_TYPE_FLT_REG               0x4000000ULL
#define FBE_DEVICE_TYPE_SPS_BATTERY           0x8000000ULL
#define FBE_DEVICE_TYPE_BMC                   0x10000000ULL
#define FBE_DEVICE_TYPE_DRIVE_MIDPLANE        0x20000000ULL
#define FBE_DEVICE_TYPE_CACHE_CARD            0x40000000ULL
#define FBE_DEVICE_TYPE_DIMM                  0x80000000ULL
#define FBE_DEVICE_TYPE_IOANNEX                  0x100000000ULL
#define FBE_DEVICE_TYPE_ALL                  0xFFFFFFFFFULL


typedef enum fbe_device_state_e {
  FBE_DEVICE_STATE_INVALID           = 0x00000000,
  FBE_DEVICE_STATE_GOOD,
  FBE_DEVICE_STATE_DEGRADED,
  FBE_DEVICE_STAT_FAULTED,
} fbe_device_state_t;


typedef struct fbe_device_location_s {
    ULONG          bus;
    ULONG          device;
    ULONG          function;
} fbe_device_location_t;

typedef struct fbe_device_desc_s {

    UINT_64                 type;
    fbe_device_state_t      state;
    fbe_device_location_t   location;
} fbe_device_desc_t;


#endif
