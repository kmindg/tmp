#ifndef SAS_FLASH_DRIVE_PRIVATE_H
#define SAS_FLASH_DRIVE_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file sas_flash_drive_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  sas flash drive.
 * 
 * HISTORY
 *   2/22/2010:  Created. chenl6
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_transport.h"

#include "fbe_ssp_transport.h"
#include "sas_physical_drive_private.h"
#include "swap_exports.h"

#define FBE_SCSI_INQUIRY_VPD_MAX_PAGES 8

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_flash_drive);

/* These are the lifecycle condition ids for a sas SC drive object. */
typedef enum fbe_sas_flash_drive_lifecycle_cond_id_e {
    FBE_SAS_FLASH_DRIVE_LIFECYCLE_COND_VPD_INQUIRY = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_SAS_FLASH_DRIVE),

    FBE_SAS_FLASH_DRIVE_LIFECYCLE_COND_LAST /* must be last */
} fbe_sas_flash_drive_lifecycle_cond_id_t;

enum fbe_sas_flash_drive_constants_e {
    FBE_SAS_FLASH_DRIVE_OUTSTANDING_IO_MAX = 32,    /* Qdepth for all IOs */
};

typedef struct fbe_vpd_pages_info_s{
    //fbe_u8_t vpd_page_code;
    fbe_u8_t vpd_page_counter;
    fbe_u8_t vpd_max_supported_pages;
    fbe_u8_t supported_vpd_pages[FBE_SCSI_INQUIRY_VPD_MAX_PAGES + 1];
}fbe_vpd_pages_info_t;

typedef struct fbe_sas_flash_drive_s{
    fbe_sas_physical_drive_t   sas_physical_drive;
    fbe_vpd_pages_info_t vpd_pages_info;

    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_SAS_FLASH_DRIVE_LIFECYCLE_COND_LAST));

}fbe_sas_flash_drive_t;



/* Methods */
fbe_status_t fbe_sas_flash_drive_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_sas_flash_drive_destroy_object( fbe_object_handle_t object_handle);

fbe_status_t fbe_sas_flash_drive_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_flash_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_flash_drive_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_sas_flash_drive_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_sas_flash_drive_monitor_load_verify(void);

fbe_status_t fbe_sas_flash_drive_init(fbe_sas_flash_drive_t * sas_flash_drive);
fbe_sas_drive_status_t fbe_flash_drive_get_vendor_table(fbe_sas_physical_drive_t * sas_physical_drive, fbe_drive_vendor_id_t drive_vendor_id, fbe_vendor_page_t ** table_ptr, fbe_u16_t * num_table_entries);

/* Usurper functions */

/* Executer functions */
fbe_status_t fbe_sas_flash_drive_send_vpd_inquiry(fbe_sas_flash_drive_t * sas_flash_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_flash_drive_send_log_sense(fbe_sas_flash_drive_t * sas_flash_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_flash_drive_send_sanitize(fbe_sas_flash_drive_t* sas_flash_drive, fbe_packet_t* packet, fbe_scsi_sanitize_pattern_t pattern);
fbe_status_t fbe_sas_flash_drive_send_sanitize_tur(fbe_sas_flash_drive_t* sas_flash_drive, fbe_packet_t* packet);
fbe_status_t fbe_sas_flash_drive_send_vpd(fbe_sas_flash_drive_t * sas_flash_drive, fbe_packet_t * packet);
/* Block transport entry */

#endif /* SAS_FLASH_DRIVE_PRIVATE_H */
 
