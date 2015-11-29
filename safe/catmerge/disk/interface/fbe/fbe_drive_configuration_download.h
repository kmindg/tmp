#ifndef FBE_DRIVE_CONFIGURATION_DOWNLOAD_H
#define FBE_DRIVE_CONFIGURATION_DOWNLOAD_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_drive_configuration_download.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_drive_configuration_service_download.c.
 * 
 * @version
 *   11/16/2011    chenl6 - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_payload_cdb_operation.h"
#include "fbe/fbe_payload_fis_operation.h"
#include "fbe/fbe_payload_cdb_fis.h"

/*************************
 *   DEFINITIONS
 *************************/

#define FBE_MAX_DOWNLOAD_DRIVES          2000

#define FBE_FDF_TLA_SIZE                 12
#define FBE_FDF_REVISION_SIZE            24 
#define FBE_FDF_PAD_SIZE (128-FBE_FDF_TLA_SIZE-FBE_FDF_REVISION_SIZE-sizeof(long)-sizeof(long)-sizeof(long))
#define FBE_FDF_FILENAME_LEN             30

#define FBE_FDF_INSTALL_HEADER_SIZE   8192
#define FBE_FDF_HEADER_MARKER            0x76543210
#define FBE_FDF_TRAILER_MARKER           0x4E617669

#define FBE_FDF_CFLAGS_RESERVED            0x01
#define FBE_FDF_CFLAGS_SELECT_DRIVE        0x02  /* check drive bitmap for targeted drives              */
#define FBE_FDF_CFLAGS_CHECK_REVISION      0x04  /* Compare revision on Drive vs File revision          */
#define FBE_FDF_CFLAGS_NO_REBOOT           0x08  /* Don't reboot SP after download (NorthStar)          */
#define FBE_FDF_CFLAGS_TRAILER_PRESENT     0x10  /* Indicates if TLA trailer is present in header       */
#define FBE_FDF_CFLAGS_DELAY_DRIVE_RESET   0x20  /* Manual Reset of drive required for firmware load    */
#define FBE_FDF_CFLAGS_ONLINE              0x40  /* Online firmware download                            */
#define FBE_FDF_CFLAGS_FAIL_NONQUALIFIED   0x80  /* Fail if there are non-qualified drives              */

#define FBE_FDF_CFLAGS2_CHECK_TLA          0x01  /* TLA # compared between the FDF and the drive when set */
#define FBE_FDF_CFLAGS2_FAST_DOWNLOAD      0x02  /* Issue a fast online disk firmware upgrade (ODFU)    */
#define FBE_FDF_CFLAGS2_TRIAL_RUN          0x10  /* Issue an ODFU trial run                             */
/* engineering flags */
#define FBE_FDF_CFLAGS2_FORCE              0x80  /* Forces a download even if RG is in a degrade or failed state */


/*!********************************************************************* 
 * @struct fbe_fdf_serviceability_block_s 
 *  
 * @brief 
 *   FDF File Serviceability Meta-data block.
 *   This structure defines the information we get
 *   in the LAST bytes of a drive installation header.
 *
 **********************************************************************/
typedef struct fbe_fdf_serviceability_block_s
{
    fbe_u32_t trailer_marker;
    fbe_u32_t revisionNumber;
    fbe_u32_t tflags;                                       /* Trailer Flags              */
    fbe_u8_t  tla_number[FBE_FDF_TLA_SIZE];                 /* left justified             */
    fbe_u8_t  fw_revision[FBE_FDF_REVISION_SIZE];           /*    "    "                  */
    fbe_u8_t  pad[FBE_FDF_PAD_SIZE];                        /* reserved expansion space   */
}fbe_fdf_serviceability_block_t;

/*!********************************************************************* 
 * @struct fbe_drive_selected_element_t 
 *  
 * @brief 
 *   Contains drive location.    
 *
 **********************************************************************/
typedef struct fbe_drive_selected_element_s
{
    fbe_u8_t bus;
    fbe_u8_t enclosure;
    fbe_u8_t slot;
    fbe_u8_t reserved;

}fbe_drive_selected_element_t;

/*!********************************************************************* 
 * @struct fbe_fdf_header_block_t 
 *  
 * @brief 
 *   Customized FDF File Header
 *
 *   This structure defines some of the data which we get
 *   in the first block of a drive installation.  
 *
 * @note
 *   The caller is expected to allocate enough space at end of this
 *   struct to hold the array of selected drives. 
 *
 **********************************************************************/
#pragma pack(4)
typedef struct fbe_fdf_header_block_s
{
    fbe_u32_t header_marker;
    fbe_u32_t image_size;
    fbe_u8_t vendor_id[FBE_SCSI_INQUIRY_VENDOR_ID_SIZE];
    fbe_u8_t product_id[FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE];
    fbe_u8_t fw_revision[FBE_SCSI_INQUIRY_REVISION_SIZE];
    fbe_u32_t install_header_size;
    fbe_u16_t trailer_offset;
    fbe_u8_t cflags;
    fbe_u8_t cflags2;
    fbe_u8_t cflagsReserved[2];
    fbe_u16_t max_dl_drives;
    fbe_u8_t ofd_filename[FBE_FDF_FILENAME_LEN];
    fbe_u16_t num_drives_selected;
    fbe_drive_selected_element_t first_drive;
}fbe_fdf_header_block_t;
#pragma pack()

/*!********************************************************************* 
 * @struct fbe_drive_configuration_control_fw_info_t 
 *  
 * @brief 
 *   Contains the drive firmware info.
 *
 **********************************************************************/
typedef struct fbe_drive_configuration_control_fw_info_s
{
    CSX_ALIGN_N(8) fbe_fdf_header_block_t  *header_image_p; //!< pointer to the image(we allign to 64 bits to make sure user/kernel passing works)
    CSX_ALIGN_N(8) fbe_u8_t                *data_image_p;   //!< pointer to the image(we allign to 64 bits to make sure user/kernel passing works)
    fbe_u32_t                                 header_size;
    fbe_u32_t                                 data_size;
} fbe_drive_configuration_control_fw_info_t;


/*!********************************************************************* 
 * @enum fbe_drive_configuration_download_process_state_t 
 *  
 * @brief 
 * This enumeration lists the firmware download process status.
 *         
 **********************************************************************/
typedef enum fbe_drive_configuration_download_process_state_e
{
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_IDLE=0,
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_RUNNING,
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAILED,
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_ABORTED,  /* only used if initiated by user*/
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL,

} fbe_drive_configuration_download_process_state_t;   

/*!********************************************************************* 
 * @enum fbe_drive_configuration_download_process_fail_e 
 *  
 * @brief 
 * This enumeration lists the fail status for the firmware download 
 * process. 
 *  
 * @note 
 * Some of the status' are deprecated, which were kept from legacy s/w. 
 *  
 **********************************************************************/
typedef enum fbe_drive_configuration_download_process_fail_e
{
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE=0,
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_NONQUALIFIED,          // One or more nonqualified drives exist in the array 
                                                            //    and the FDF_CFLAGS_FAIL_NONQUALIFIED bit is set.
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_PROBATION,             // A drive failed to transition probational mode.                              
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_NO_DRIVES_TO_UPGRADE,  // There are no drives that can be upgraded.    
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_FW_REV_MISMATCH,       // After finishing upgrade, a drive did not have the expected FW rev.
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_WRITE_BUFFER,          // DH returned bad status from a write buffer command sent to a drive.
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_CREATE_LOG,            // (deprecated) An error ocurred that inhibits creating a rebuild log.        
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_BRICK_DRIVE,           // A drive failed to respond after sending the download.
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_DRIVE,                 // A drive failed sometime during the FW download.
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_ABORTED,               // The process was aborted by Navi.
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_MISSING_TLA,           // The FDF file is missing the FDF SERVICEABILITY BLOCK, and TLA #. 
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_GENERIC_FAILURE,       // The FDF file is missing the FDF SERVICEABILITY BLOCK, and TLA #. 
}
fbe_drive_configuration_download_process_fail_t;

/*!********************************************************************* 
 * @enum fbe_drive_configuration_download_process_operation_t 
 *  
 * @brief 
 * This enumeration lists the firmware download process operation.
 *         
 **********************************************************************/
typedef enum fbe_drive_configuration_download_process_operation_e
{
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_OP_NOP=0,
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_OP_TRIAL_RUN,
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_OP_UPGRADE,
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_OP_FAST_UPGRADE,

} fbe_drive_configuration_download_process_operation_t;

/*!********************************************************************* 
 * @enum fbe_drive_configuration_download_drive_state_t 
 *  
 * @brief 
 * This enumeration lists drive state during firmware download 
 * process. 
 *
 **********************************************************************/
typedef enum fbe_drive_configuration_download_drive_state_s
{
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_IDLE,               /* FRU is not marked for download */
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_INIT,               /* FRU is in download init state */
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_ACTIVE,             /* FRU is in download active state */
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_COMPLETE,           /* FRU is in download complete state */
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_COMPLETE_TRIAL_RUN, /* FRU is in download trial run complete state */
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL,               /* FRU fails in download */
} fbe_drive_configuration_download_drive_state_t;


/*!********************************************************************* 
 * @enum fbe_drive_configuration_download_drive_fail_t 
 *  
 * @brief 
 * This enumeration lists drive fail reason during firmware download 
 * process. 
 *
 **********************************************************************/
typedef enum fbe_drive_configuration_download_drive_fail_e 
{
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_NO_FAILURE,             /* FRU has been successful so far  */
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_NONQUALIFIED,         /* FRU  is in a non-redundant RG */
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_WRITE_BUFFER,         /* FRU fails in WRITE BUFFER command */
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_POWER_UP,             /* FRU fails to power up */
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_WRONG_REV,            /* Firmware Revision is not correct */
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_ABORTED,              /* FRU fails to create rebuild log */
    FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_GENERIC_FAILURE,              /* FRU fails to create rebuild log */
} fbe_drive_configuration_download_drive_fail_t;


/*!********************************************************************* 
 * @struct fbe_drive_configuration_download_get_process_info_t 
 *  
 * @brief 
 *   Contains the request for the download process information.
 *
 **********************************************************************/
typedef struct fbe_drive_configuration_download_get_process_info_s
{
    fbe_bool_t                                            in_progress;
    fbe_drive_configuration_download_process_state_t      state;
    fbe_drive_configuration_download_process_fail_t       fail_code;
    fbe_drive_configuration_download_process_operation_t  operation_type;
    fbe_system_time_t                                     start_time;
    fbe_system_time_t                                     stop_time;
    fbe_u32_t                                             num_qualified; /* total number of drives issued a download req */
    fbe_u32_t                                             num_complete; /* total of successful and failed */
    fbe_u8_t                                              fdf_filename[FBE_FDF_FILENAME_LEN];

} fbe_drive_configuration_download_get_process_info_t;

/*!********************************************************************* 
 * @struct fbe_drive_configuration_download_get_drive_info_t 
 *  
 * @brief 
 *   Contains the request for the download drive information.
 *
 **********************************************************************/
typedef struct fbe_drive_configuration_download_get_drive_info_s
{

    /************ IN DATA **************/
    /* Location
     */
    fbe_u8_t   bus;
    fbe_u8_t   enclosure;
    fbe_u8_t   slot;

    /************ OUT DATA **************/
    fbe_drive_configuration_download_drive_state_t     state;
    fbe_drive_configuration_download_drive_fail_t      fail_code;
    CSX_ALIGN_N(8) fbe_u8_t                         prior_product_rev[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE+1];    

} fbe_drive_configuration_download_get_drive_info_t;


/*!********************************************************************* 
 * @struct fbe_drive_configuration_get_download_max_drive_count_t 
 *  
 * @brief 
 *   Contains the request for the download drive information.
 *
 **********************************************************************/
typedef struct fbe_drive_configuration_get_download_max_drive_count_s{
    /***** OUT DATA ****/
    fbe_u32_t       max_dl_count;
}fbe_drive_configuration_get_download_max_drive_count_t;
#endif /* FBE_DRIVE_CONFIGURATION_DOWNLOAD_H */

