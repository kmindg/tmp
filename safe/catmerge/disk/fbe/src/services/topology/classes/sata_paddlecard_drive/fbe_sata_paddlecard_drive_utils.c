/**********************************************************************
 *  Copyright (C)  EMC Corporation 2010
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_sata_paddlecard_drive_utils.c
 ***************************************************************************
 *
 *  The routines in this file contains all the utility functions used by the 
 *  SATA paddlecard drive object.
 *
 * NOTE: 
 *
 * HISTORY
 *   12/30/2010  Created.  Wayne Garrett
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_transport.h"

#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"

#include "sata_paddlecard_drive_private.h"
#include "sata_paddlecard_drive_config.h"

/**************************************************************************
* fbe_sata_paddlecard_drive_get_vendor_table()                  
***************************************************************************
*
* DESCRIPTION
*       This is an override function which will return the vendor table
*       for the default mode page settings.
*       
* PARAMETERS
*       sas_physical_drive - Pointer to SAS physical drive object.
*       drive_vendor_id - Vendor ID
*       table_ptr - Ptr to vendor table
*       num_table_entries - Number of entries in vendor table
*
* RETURN VALUES
*       status. 
*
* NOTES
*       This is based on a Template Method design pattern.
*
* HISTORY
*   12/30/2010 Wayne Garrett - Created.
***************************************************************************/
fbe_sas_drive_status_t    
fbe_sata_paddlecard_drive_get_vendor_table(fbe_sas_physical_drive_t * sas_physical_drive, fbe_drive_vendor_id_t drive_vendor_id, fbe_vendor_page_t ** table_ptr, fbe_u16_t * num_table_entries)
{
    if (sas_physical_drive->sas_drive_info.use_additional_override_tbl)
    {
        *table_ptr = fbe_cli_mode_page_override_tbl;
        *num_table_entries = fbe_cli_mode_page_override_entries;
        return FBE_SAS_DRIVE_STATUS_OK;
    }
    *table_ptr = fbe_def_sata_paddlecard_vendor_tbl;
    *num_table_entries = fbe_def_sata_paddlecard_vendor_tbl_entries;

    return FBE_SAS_DRIVE_STATUS_OK;
}


