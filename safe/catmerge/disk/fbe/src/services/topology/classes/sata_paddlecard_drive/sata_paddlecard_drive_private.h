#ifndef SATA_PADDLECARD_DRIVE_PRIVATE_H
#define SATA_PADDLECARD_DRIVE_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file sata_paddlecard_drive_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  sata paddlecard drive.
 * 
 * HISTORY
 *   12/28/2010:  Created. Wayne Garrett
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

#define FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_MAX_PAGES 8

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sata_paddlecard_drive);

/* These are the lifecycle condition ids for a sas SC drive object. */
typedef enum fbe_sata_paddlecard_drive_lifecycle_cond_id_e {
    FBE_SATA_PADDLECARD_DRIVE_LIFECYCLE_COND_VPD_INQUIRY = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_SATA_PADDLECARD_DRIVE),
    /*add new conditions here*/

    FBE_SATA_PADDLECARD_DRIVE_LIFECYCLE_COND_LAST /* must be last */
} fbe_sata_paddlecard_drive_lifecycle_cond_id_t;

/*NOTE: The vpd_pages structs and condition functions are duplicated from sas_flash_drive.  In the future
 consider factoring out common logic. The best place is SAS_PHYSICAL_DRIVE since SAS drives support vpd inquiry*/
typedef struct fbe_sata_paddlecard_vpd_pages_info_s{
    fbe_u8_t vpd_page_counter;
    fbe_u8_t vpd_max_supported_pages;
    fbe_u8_t supported_vpd_pages[FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_MAX_PAGES + 1];
}fbe_sata_paddlecard_vpd_pages_info_t;

typedef enum fbe_sata_paddlecard_scsi_inquiry_vpd_offset_e {
    FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_PAGE_CODE_OFFSET = 1, /* Page Code is found at same offset in all VPD pages */
    FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_00_MAX_SUPPORTED_PAGES_OFFSET = 3,
    FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_00_SUPPORTED_PAGES_START_OFFSET = 4, 
    FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_C0_PROD_REV_OFFSET = 6,     /* The Product Revision Major/minor (MMmm)is in VPD 0xC0, bytes 6-9 */
    FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_D2_BRIDGE_HW_REV_OFFSET =9, /* Bridge H/W revision is at byte 9 in VPD Page 0xD2 */
}fbe_sata_paddlecard_scsi_inquiry_vpd_offset_t;

/* List of VPD pages of interest. Add more as needed. */
typedef enum fbe_sata_paddlecard_scsi_inquiry_vpd_page_e {
    FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_PAGE_00 = 0x00,
    FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_PAGE_C0 = 0xC0,
    FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_PAGE_D2 = 0xD2,
}fbe_sata_paddlecard_scsi_inquiry_vpd_page_t;

typedef struct fbe_sata_paddlecard_drive_s{
    fbe_sas_physical_drive_t   sas_physical_drive;
    fbe_sata_paddlecard_vpd_pages_info_t vpd_pages_info;

    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_SATA_PADDLECARD_DRIVE_LIFECYCLE_COND_LAST));

}fbe_sata_paddlecard_drive_t;



/* All FBE timeouts in miliseconds */
enum fbe_sata_paddlecard_drive_timeouts_e {
    FBE_SATA_PADDLECARD_DRIVE_INQUIRY_TIMEOUT       = FBE_SAS_PHYSICAL_DRIVE_INQUIRY_TIMEOUT, 
    FBE_SATA_PADDLECARD_DRIVE_READ_CAPACITY_TIMEOUT = FBE_SAS_PHYSICAL_DRIVE_READ_CAPACITY_TIMEOUT,
    FBE_SATA_PADDLECARD_DRIVE_START_UNIT_TIMEOUT    = FBE_SAS_PHYSICAL_DRIVE_START_UNIT_TIMEOUT,
    FBE_SATA_PADDLECARD_DRIVE_WRITE_BUFFER_TIMEOUT  = FBE_SAS_PHYSICAL_DRIVE_WRITE_BUFFER_TIMEOUT,
    FBE_SATA_PADDLECARD_DRIVE_DEFAULT_TIMEOUT       = 25000 /* 25 sec */
};


/* Methods */
fbe_status_t fbe_sata_paddlecard_drive_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_sata_paddlecard_drive_destroy_object( fbe_object_handle_t object_handle);

fbe_status_t fbe_sata_paddlecard_drive_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sata_paddlecard_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sata_paddlecard_drive_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_sata_paddlecard_drive_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_sata_paddlecard_drive_monitor_load_verify(void);

fbe_status_t fbe_sata_paddlecard_drive_init(fbe_sata_paddlecard_drive_t * sata_paddlecard_drive);

/* Utility functions */
fbe_sas_drive_status_t  fbe_sata_paddlecard_drive_get_vendor_table(fbe_sas_physical_drive_t * sas_physical_drive, fbe_drive_vendor_id_t drive_vendor_id, fbe_vendor_page_t ** table_ptr, fbe_u16_t * num_table_entries);

/* Error Handling functions */
fbe_status_t fbe_sata_paddlecard_drive_get_port_error(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet, fbe_payload_cdb_operation_t * payload_cdb_operation, fbe_scsi_error_code_t * scsi_error);

/* Usurper functions */

/* Executer functions */
fbe_status_t fbe_sata_paddlecard_drive_send_vpd_inquiry(fbe_sata_paddlecard_drive_t * sata_paddlecard_drive, fbe_packet_t * packet);

/* Block transport entry */

#endif /* SATA_PADDLECARD_DRIVE_PRIVATE_H */

