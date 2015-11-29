//***************************************************************************
// Copyright (C) EMC Corporation 2012
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/*!
 * @file EmcPAL_MapIoSpace.h
 * @addtogroup emcpal_misc
 * @{
 *
 * @brief
 *      This file contains structures, prototypes and defines related to I/O mapping.
 *
 * @version
 *      7/11/2011 M.Hamel - Created.
 *
 */

#ifndef _EMCPAL_MAPIOSPACE_H_
#define _EMCPAL_MAPIOSPACE_H_

#include "EmcPAL.h"

#ifdef __cplusplus
extern "C" {
#endif

  
/*!
 * Shorthand macro to locate the current module's context
 */
#define EMCPAL_MAP_IO_SPACE_ME EmcpalClientGetCsxModuleContext(EmcpalDriverGetCurrentClientObject())
/*!
 * A function to mimic MmMapIoSpace().
 */
EMCPAL_API
csx_cpvoid_t
EmcpalMapIoSpace(csx_module_context_t, csx_p_pci_device_handle_t, csx_phys_address_t,
                              csx_phys_length_t, csx_p_io_obj_cache_attr_e, csx_p_io_obj_bmap_attr_e,
                              csx_p_io_obj_location_attr_e, csx_p_io_map_t *, csx_resource_name_t);
/*!
 * A function to free the I/O Map Object allocated by EmcpalMapIoSpace()
 */
EMCPAL_API
csx_status_e
EmcpalUnmapIoSpace(csx_p_io_map_t *pio_map_object);

/*! @brief EmcPAL map-io-space object. 
 * Suitable for creating a list of mapped memory objects within a specific driver,
 * to make unmapping easier at unload time.
 * If all you need is to temporarily use a csx_p_io_map_t structure, it is better
 * to just allocate a csx_p_io_map_t structure directly.
 */
typedef struct
{
    EMCPAL_LIST_ENTRY ListEntry;							/*!< List entry */
    csx_p_io_map_t    Object;								/*!< IO map object */
} EMCPAL_MAPIOSPACE_OBJECT, *PEMCPAL_MAPIOSPACE_OBJECT;		/*!< Pointer to EmcPAL map-io-space object */

#ifdef __cplusplus
}
#endif
/*!
 * @} end group emcpal_misc
 * @} end file EmcPAL_MapIoSpace.h
 */

#endif /* _EMCPAL_MAPIOSPACE_H_ */
