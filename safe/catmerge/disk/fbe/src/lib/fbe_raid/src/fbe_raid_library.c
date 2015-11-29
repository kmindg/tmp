/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_library.c
 ***************************************************************************
 *
 * @brief
 *  This file contains basic raid library functions.
 *
 * @version
 *   3/15/2010:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_memory_api.h"
#include "fbe_raid_library.h"
#include "fbe_raid_library_private.h"
#include "fbe_raid_library_proto.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
/*!*******************************************************************
 * @def FBE_RAID_MAX_IO_MSECS
 *********************************************************************
 * @brief Max time we will allow an I/O to take.
 *        After this time, the I/O will expire at the PDO.
 *
 *********************************************************************/
#define FBE_RAID_MAX_IO_MSECS 30000
/*!*******************************************************************
 * @var fbe_raid_library_max_io_msecs
 *********************************************************************
 * @brief Global to allow us to get/set this value.
 *        We need to set this value for testing to make it easier
 *        for an I/O to expire.
 *
 *********************************************************************/
static fbe_time_t fbe_raid_library_max_io_msecs = FBE_RAID_MAX_IO_MSECS;

fbe_time_t fbe_raid_library_get_max_io_msecs(void)
{
    return fbe_raid_library_max_io_msecs;
}
void fbe_raid_library_set_max_io_msecs(fbe_time_t msecs)
{
    fbe_raid_library_max_io_msecs = msecs;
    return;
}
/* Depending on if the system is encrypted we may or may not 
 * shoot drives that have multibit crc errors. 
 */
fbe_bool_t fbe_raid_library_is_encrypted = FBE_FALSE;

void fbe_raid_library_set_encrypted(void)
{
    if (!fbe_raid_library_is_encrypted) {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "encryption is enabled.\n");
    }
    fbe_raid_library_is_encrypted = FBE_TRUE;
}
fbe_bool_t fbe_raid_library_get_encrypted(void)
{
    return fbe_raid_library_is_encrypted;
}
/*!**************************************************************
 * fbe_raid_library_initialize()
 ****************************************************************
 * @brief
 *  Setup the raid library.
 *
 * @param None.               
 *
 * @return fbe_status_t
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_initialize(fbe_raid_library_debug_flags_t default_flags)
{
    fbe_raid_geometry_set_default_raid_library_debug_flags(default_flags);

    fbe_raid_cond_init_log();

    fbe_raid_geometry_load_default_registry_values();

    return fbe_raid_memory_initialize();
}
/******************************************
 * end fbe_raid_library_initialize()
 ******************************************/

/*!**************************************************************
 * fbe_raid_library_destroy()
 ****************************************************************
 * @brief
 *  Tear down the raid library.
 *
 * @param None.               
 *
 * @return fbe_status_t
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_library_destroy(void)
{
    fbe_raid_cond_destroy_log();

    return fbe_raid_memory_destroy();
}
/******************************************
 * end fbe_raid_library_destroy()
 ******************************************/
/*************************
 * end file fbe_raid_library.c
 *************************/
