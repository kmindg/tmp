/***************************************************************************
 * Copyright (C) EMC Corporation 2010 - 2012
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

#ifndef FBE_MODULE_MGMT_MAPPING_H
#define FBE_MODULE_MGMT_MAPPING_H

#include "fbe_module_info.h"
#include "fbe_module_mgmt_reg_util.h"

typedef struct
{
    FAMILY_ID               ffid;       // Key
    fbe_module_slic_type_t  slic_type;
    FBE_IO_MODULE_PROTOCOL  protocol;
    fbe_char_t              label_name[255];
} fbe_ffid_property_t;

extern fbe_ffid_property_t fbe_ffid_property_map[];
extern fbe_u32_t fbe_ffid_property_map_size;

typedef struct
{
    fbe_module_slic_type_t  slic_type;  // Key
    const fbe_char_t        *name;
    fbe_iom_group_t         group;
    fbe_ioport_role_t       role;
} fbe_slic_type_property_t;

extern fbe_slic_type_property_t fbe_slic_type_property_map[];
extern fbe_u32_t fbe_slic_type_property_map_size;

typedef struct
{
    fbe_iom_group_t         group;      // Key
    const fbe_char_t        *driver;
} fbe_iom_group_property_t;

extern fbe_iom_group_property_t fbe_iom_group_property_map[];
extern fbe_u32_t fbe_iom_group_property_map_size;

typedef struct
{
    const fbe_char_t        *driver;    // Key
    fbe_iom_group_t         default_group;
} fbe_driver_property_t;

extern fbe_driver_property_t fbe_driver_property_map[];
extern fbe_u32_t fbe_driver_property_map_size;

#endif /* FBE_MODULE_MGMT_MAPPING_H */

/*******************************
 * end fbe_module_mgmt_mapping.h
 *******************************/
