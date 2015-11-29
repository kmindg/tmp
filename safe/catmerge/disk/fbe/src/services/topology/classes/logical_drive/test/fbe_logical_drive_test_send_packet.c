/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_logical_drive_test_send_packet.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains test functions for sending packets.
 *
 * HISTORY
 *   6/16/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_trace.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_base_object.h"
#include "fbe_topology.h"
#include "fbe_logical_drive_test_private.h"
#include "fbe_logical_drive_test_prototypes.h"
#include "mut.h"
#include "fbe/fbe_api_logical_drive_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/****************************************************************
 * fbe_ldo_test_set_block_size()
 ****************************************************************
 * DESCRIPTION:
 *  Sets the block size for a given object.
 *
 * RETURNS:
 *  status of the operation..             
 *
 * HISTORY:
 *  5/19/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_ldo_test_set_block_size(fbe_lba_t capacity,
                                         fbe_block_size_t imported_block_size,
                                         fbe_object_id_t object_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_drive_attributes_t attributes;
    fbe_logical_drive_attributes_t * const attributes_p = &attributes;

    /* Next, set the logical drive's attributes.
     */
    attributes.imported_block_size = imported_block_size;
    attributes.imported_capacity = capacity;

    status = fbe_api_logical_drive_set_attributes(object_id, attributes_p);

    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "LDO: Set of attributes failed.");

    return status;
}
/******************************************
 * end fbe_ldo_test_set_block_size()
 ******************************************/
/*************************
 * end file fbe_logical_drive_test_send_packet.c
 *************************/
