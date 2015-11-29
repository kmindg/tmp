#ifndef FBE_DRIVE_CONFIGURATION_SERVICE_DOWNLOAD_PRIVATE_H
#define FBE_DRIVE_CONFIGURATION_SERVICE_DOWNLOAD_PRIVATE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_drive_configuration_service_download_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains structures and functions for drive firmware 
 *  download service.
 *
 * @version
 *   11/16/2011:  Created. chenl6
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_drive_configuration_download.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_transport_memory.h"


/* Maximum concurrent downloads allowed.  This is used by online 
 * drive firmware download.  This is an artifical limit.
 * It is supposed to be set to the max rebuild logs allowed, but the
 * design has changed such that there is no longer a limit.  Keeping
 * this as carry over from the legacy s/w. 
 * This has been deprecated, but still here since Admin still makes
 * calls to get it.
 */
#define FBE_DCS_MAX_CONFIG_DL_DRIVES 20  

#define DCS_MAX_DOWNLOAD_SG_ELEMENTS 100


/*************************
 *   STRUCTURES
 *************************/

/*!****************************************************************************
 *    
 * @struct fbe_drive_configuration_download_drive_info_s
 *  
 * @brief 
 *   This is the definition of the structure for the fw 
 *   download drive information.
 ******************************************************************************/
typedef struct fbe_drive_configuration_download_drive_s
{
    fbe_drive_selected_element_t location;
    fbe_object_id_t object_id;
    fbe_drive_configuration_download_drive_state_t dl_state;
    fbe_drive_configuration_download_drive_fail_t fail_code;
    fbe_u8_t prior_product_rev[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE+1];
} fbe_drive_configuration_download_drive_t;


/*!****************************************************************************
 *    
 * @struct fbe_drive_configuration_download_s
 *  
 * @brief 
 *   This is the definition of the structure for the fw 
 *   download process information.
 ******************************************************************************/
typedef struct fbe_drive_configuration_download_process_s
{
    fbe_spinlock_t              download_lock;

    /* Pointers to firmware image
     */
    fbe_fdf_header_block_t *    header_ptr;
    fbe_u32_t                   num_image_sg_elements;
    fbe_sg_element_t            image_sgl[DCS_MAX_DOWNLOAD_SG_ELEMENTS];  /* sg_list to send to physical package */ 

    /* Indicates if firmware download is in progess
     */
    fbe_bool_t in_progress; 

    /* Indicates whether cleanup after firmware download is in progess
     */
    fbe_bool_t cleanup_in_progress; 

    /* Maximum number of drive downloads allowed.
     * This has been deprecated, but still here since Admin still makes
     * calls to get it.
     */
    fbe_u32_t max_dl_count;

    /* Overall process state
     */
    fbe_drive_configuration_download_process_state_t  process_state;

    /* Overall fail reason if process state indicates failure
     */
    fbe_drive_configuration_download_process_fail_t  process_fail_reason;

    /* Type of firmware download operation
     */
    fbe_drive_configuration_download_process_operation_t  operation_type;

    /* When true, this SP is running ODFU. 
     */
    fbe_bool_t master_sp;

    /* The process should exit for failures, or if it's a user cancel. 
     */
    fbe_bool_t fail_abort; 

    /* The process is trial run.
     */
    fbe_bool_t trial_run;

    /* Disruptive download.  RAID checking is skipped by PDO.
    */
    fbe_bool_t fast_download;

    /* Fail the download if one or more drives are nonqualified.
    */
    fbe_bool_t fail_nonqualified;

    /* Force download for cases where PVD is failed or RG is degraded
    */
    fbe_bool_t force_download;

    /* The number of all qualified drives.
     */
    fbe_u32_t num_qualified_drives;

    /* The number of drives that are done download. 
     */
    fbe_atomic_t num_done_drives;

    /* TLA number.
     */
    fbe_u8_t  tla_number[FBE_FDF_TLA_SIZE + 1];

    /* The firmware revision to be downloaded. 
     */
    fbe_u8_t fw_rev[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE+1];

    /* Name of FDF file
     */
    fbe_u8_t  fdf_filename[FBE_FDF_FILENAME_LEN+1];

    /* Time process starts and stops
     */
    fbe_system_time_t start_time;
    fbe_system_time_t stop_time;

    fbe_drive_configuration_download_drive_t drives[FBE_MAX_DOWNLOAD_DRIVES];

} fbe_drive_configuration_download_process_t;


/*************************
 *   FUNCTIONS
 *************************/
fbe_status_t fbe_drive_configuration_control_download_firmware(fbe_packet_t *packet);
fbe_status_t fbe_drive_configuration_control_abort_download(fbe_packet_t *packet);
fbe_status_t fbe_drive_configuration_control_get_download_process_info(fbe_packet_t *packet);
fbe_status_t fbe_drive_configuration_control_get_download_drive_info(fbe_packet_t *packet);
fbe_status_t fbe_drive_configuration_service_initialize_download(void);
fbe_status_t fbe_drive_configuration_service_destroy_download(void);
fbe_status_t fbe_drive_configuration_control_get_max_dl_count(fbe_packet_t *packet);
fbe_status_t fbe_drive_configuration_control_get_all_download_drives_info(fbe_packet_t *packet);

#endif /* FBE_DRIVE_CONFIGURATION_SERVICE_DOWNLOAD_PRIVATE_H */
