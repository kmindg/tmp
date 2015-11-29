#ifndef SATA_FLASH_DRIVE_PRIVATE_H
#define SATA_FLASH_DRIVE_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file sata_flash_drive_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  sata flash drive.
 * 
 * HISTORY
 *   2/22/2010:  Created. chenl6
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_transport.h"

#include "fbe_ssp_transport.h"
#include "sata_physical_drive_private.h"
#include "swap_exports.h"

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sata_flash_drive);

/* These are the lifecycle condition ids for a sata SC drive object. */
typedef enum fbe_sata_flash_drive_lifecycle_cond_id_e {
    FBE_SATA_FLASH_DRIVE_LIFECYCLE_COND_INVALID = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_SATA_FLASH_DRIVE),

    FBE_SATA_FLASH_DRIVE_LIFECYCLE_COND_LAST /* must be last */
} fbe_sata_flash_drive_lifecycle_cond_id_t;

typedef struct fbe_sata_flash_drive_s{
    fbe_sata_physical_drive_t   sata_physical_drive;

    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_SATA_FLASH_DRIVE_LIFECYCLE_COND_LAST));

}fbe_sata_flash_drive_t;


/* Methods */
fbe_status_t fbe_sata_flash_drive_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_sata_flash_drive_destroy_object( fbe_object_handle_t object_handle);

fbe_status_t fbe_sata_flash_drive_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sata_flash_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sata_flash_drive_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_sata_flash_drive_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_sata_flash_drive_monitor_load_verify(void);

fbe_status_t fbe_sata_flash_drive_init(fbe_sata_flash_drive_t * sata_flash_drive);

/* Usurper functions */

/* Executer functions */

/* Block transport entry */

#endif /* SATA_FLASH_DRIVE_PRIVATE_H */
