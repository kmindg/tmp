#ifndef FBE_PROVISION_DRIVE_H
#define FBE_PROVISION_DRIVE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the
 *  provision_drive object.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @revision
 *   7/24/2009:  Created. Peter Puhov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_object.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_power_save_interface.h"
#include "fbe/fbe_base_config.h"

/*************************
 *   FUNCTION DEFINITIONS 
 *************************/

 /*! @defgroup provision_drive_class Provision Virtual Drive Class
 *  @brief This is the set of definitions for the base config.
 *  @ingroup fbe_base_object
 */ 
/*! @defgroup provision_drive_usurper_interface Provision Drive Usurper Interface
 *  @brief This is the set of definitions that comprise the base config class
 *  usurper interface.
 *  @ingroup fbe_classes_usurper_interface
 * @{
 */

/*!*******************************************************************
 * @def FBE_PROVISION_DRIVE_DEFAULT_CHUNK_SIZE
 *********************************************************************
 * @brief This is the standard chunk size in blocks.
 *
 *********************************************************************/
#define FBE_PROVISION_DRIVE_DEFAULT_CHUNK_SIZE 2048

/*!********************************************************************* 
 * @enum fbe_provision_drive_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the provision drive specific control codes.
 * These are control codes specific to the provision drive object, which only
 * the provision drive object will accept.
 *         
 **********************************************************************/
typedef enum
{
    FBE_PROVISION_DRIVE_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_PROVISION_DRIVE),

    /* This usurper command is used to set the configuration of provision
     * drive object.
     */
    FBE_PROVISION_DRIVE_CONTROL_CODE_SET_CONFIGURATION,

    /* This usurper command is used to update PVD config*/
    FBE_PROVISION_DRIVE_CONTROL_CODE_UPDATE_CONFIGURATION,

    /* This usurpur command is used to get the drive information associated
     * with provision drive object.
     */
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO,

    FBE_PROVISION_DRIVE_CONTROL_CODE_CLASS_CALCULATE_CAPACITY,

    /* This usurpur command is used to get the spare inforamtion associated
     * with hot spare PVD.
     */
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SPARE_INFO,

    FBE_PROVISION_DRIVE_CONTROL_CODE_SET_ZERO_CHECKPOINT,
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_ZERO_CHECKPOINT,

    FBE_PROVISION_DRIVE_CONTROL_CODE_SET_VERIFY_INVALIDATE_CHECKPOINT,
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_VERIFY_INVALIDATE_CHECKPOINT,

    /* Initiate disk zeroing through usurper command */
    FBE_PROVISION_DRIVE_CONTROL_CODE_INITIATE_DISK_ZEROING,

    /*! clear sniff verify report
     */
     FBE_PROVISION_DRIVE_CONTROL_CODE_CLEAR_VERIFY_REPORT,

    /*! get sniff verify report
     */
     FBE_PROVISION_DRIVE_CONTROL_CODE_GET_VERIFY_REPORT,

    /*! get sniff verify status
     */
     FBE_PROVISION_DRIVE_CONTROL_CODE_GET_VERIFY_STATUS,

    /*! set sniff verify checkpoint
     */
     FBE_PROVISION_DRIVE_CONTROL_CODE_SET_VERIFY_CHECKPOINT,

    FBE_PROVISION_DRIVE_CONTROL_CODE_METADATA_PAGED_WRITE,

    FBE_PROVISION_DRIVE_CONTROL_CODE_SET_BACKGROUND_PRIORITIES,
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_BACKGROUND_PRIORITIES,

    /* it is used to get drive location from nonpaged metadata. */
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_DRIVE_LOCATION,

    /* it is used to get the drive location of pvd object. */
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PHYSICAL_DRIVE_LOCATION,

    /* Get metadata memory of the object and his peer */
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_METADATA_MEMORY,

    /* Get metadata memory with version information of the object and his peer*/
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_VERSIONED_METADATA_MEMORY,

    /* Set EOL state to provision drive object */
    FBE_PROVISION_DRIVE_CONTROL_CODE_SET_EOL_STATE,

    /* Clear the EOL state in the provision drive object */
    FBE_PROVISION_DRIVE_CONTROL_CODE_CLEAR_EOL_STATE,

    /* Set debug trace flag at object level */
    FBE_PROVISION_DRIVE_CONTROL_CODE_SET_DEBUG_FLAGS,

    /* Set debug trace flag at class level*/
    FBE_PROVISION_DRIVE_CONTROL_CODE_SET_CLASS_DEBUG_FLAGS,

    /* Send download request to a PVD */
    FBE_PROVISION_DRIVE_CONTROL_CODE_SEND_DOWNLOAD,

    /* Send ABORT download request */
    FBE_PROVISION_DRIVE_CONTROL_CODE_ABORT_DOWNLOAD,

    /* Get the download information of this pvd*/
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_DOWNLOAD_INFO,

    /* Raid group acknowledge PVD's download request */
    FBE_PROVISION_DRIVE_CONTROL_CODE_DOWNLOAD_ACK,    /*todo: move to base_config, so VD and PVD can handle*/

    /*get the opaque data of this pvd*/
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_OPAQUE_DATA,

    /* Get the Pool-id of this pvd*/
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_POOL_ID,

    /* Set the Pool-id of this pvd*/
    FBE_PROVISION_DRIVE_CONTROL_CODE_SET_POOL_ID,
    /* set/get removed time stamp of this PVD*/
    FBE_PROVISION_DRIVE_CONTROL_CODE_SET_REMOVED_TIMESTAMP,
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_REMOVED_TIMESTAMP,

    /* Stub for VD get checkpoint - used for first 4 sys drives */
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_CHECKPOINT,

    /* Enable/disable SLF */
    FBE_PROVISION_DRIVE_CONTROL_CODE_SET_SLF_ENABLED,

    /* Get SLF enabled */
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SLF_ENABLED,

    /* Get SLF state */
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SLF_STATE,

    /* Test SLF */
    FBE_PROVISION_DRIVE_CONTROL_CODE_TEST_SLF,

    /* Clear the NP during the reinit of system pvds */
    FBE_PROVISION_DRIVE_CONTROL_CODE_CLEAR_NONPAGED,

    /*! Map a client lba to a chunk index and metadata lba. */ 
    FBE_PROVISION_DRIVE_CONTROL_CODE_MAP_LBA_TO_CHUNK,    

    /* Clear DRIVE FAULT */
    FBE_PROVISION_DRIVE_CONTROL_CODE_CLEAR_DRIVE_FAULT,

    /* set background operation speed */
    FBE_PROVISION_DRIVE_CONTROL_CODE_SET_BG_OP_SPEED,

    /* set background operation speed */
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_BG_OP_SPEED,

    /* update the percent rebuilt */
    FBE_PROVISION_DRIVE_CONTROL_CODE_SET_PERCENT_REBUILT,

    /* Control code to register the keys */
    FBE_PROVISION_DRIVE_CONTROL_CODE_REGISTER_KEYS,

    /* Control code to unregister keys */
    FBE_PROVISION_DRIVE_CONTROL_CODE_UNREGISTER_KEYS,

    /* Control code to get the miniport key handle for
     * testing purposes
     */
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_MINIPORT_KEY_HANDLE,

    /* Control code start encryption by encrypting paged. */
    FBE_PROVISION_DRIVE_CONTROL_CODE_RECONSTRUCT_PAGED,

    /* Control code that validates if a particular block is encrypted or not */
    FBE_PROVISION_DRIVE_CONTROL_CODE_VALIDATE_BLOCK_ENCRYPTION,
    
    /* Get health check info from this PVD */
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_HEALTH_CHECK_INFO,

    /* zero PVD area already comsumed */
    FBE_PROVISION_DRIVE_CONTROL_CODE_INITIATE_CONSUMED_AREA_ZEROING,

    /* Control code that gets DEK info */
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_DEK_INFO,

    /* Control code that quiesces PVD */
    FBE_PROVISION_DRIVE_CONTROL_CODE_QUIESCE,

    /* Control code that unquiesces PVD */
    FBE_PROVISION_DRIVE_CONTROL_CODE_UNQUIESCE,

    /* Control code that unquiesces PVD */
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_CLUSTERED_FLAG,

    /* Control code that disables PVD paged cache */
    FBE_PROVISION_DRIVE_CONTROL_CODE_DISABLE_PAGED_CACHE,

    /* Control code that gets PVD paged cache information */
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PAGED_CACHE_INFO,

    /* This control code is used to set the `swap pending' NP flag.  This is
     * set to prevent the source drive from being zeroed prior to the completion 
     * of the swap operation 
     */
    FBE_PROVISION_DRIVE_CONTROL_SET_SWAP_PENDING,

    /* This control code is used to clear the `swap pending' NP flag whcih will
     * all the encryption zero pvd progress to proceed.
     */
    FBE_PROVISION_DRIVE_CONTROL_CLEAR_SWAP_PENDING,

    /* This control code is used to retrieve the status of the `swap pending'
     * flags.
     */
    FBE_PROVISION_DRIVE_CONTROL_GET_SWAP_PENDING,

    /* This control code is used to set EAS start.
     */
    FBE_PROVISION_DRIVE_CONTROL_SET_EAS_START,

    /* This control code is used to set EAS complete.
     */
    FBE_PROVISION_DRIVE_CONTROL_SET_EAS_COMPLETE,

    /* This control code is used to get EAS state.
     */
    FBE_PROVISION_DRIVE_CONTROL_GET_EAS_STATE,

    FBE_PROVISION_DRIVE_CONTROL_CODE_UPDATE_BLOCK_SIZE,  /* ENGINEERING ONLY option to change block size*/

    /* Control code that gets DEK info */
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SSD_STATISTICS,

    /* Control code to get vpd page b0 block limits */
    FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SSD_BLOCK_LIMITS,

    /* Control code to clear the unmap bit flag */
    FBE_PROVISION_DRIVE_CONTROL_CODE_SET_UNMAP_ENABLED_DISABLED,

    /* Control code to set the wear leveling timer */
    FBE_PROVISION_DRIVE_CONTROL_CODE_SET_CLASS_WEAR_LEVELING_TIMER,

    /* Insert new control codes here. */
    FBE_PROVISION_DRIVE_CONTROL_CODE_LAST
}
fbe_provision_drive_control_code_t;

/*!**********************************************************************
 * @enum fbe_provision_drive_config_type_e
 *  
 *  @brief 
 * These are the configuration type associated with provision drive object
 * and it is used to configure 
 *
*************************************************************************/

typedef enum fbe_provision_drive_config_type_e
{
    /*! Invalid config type, provision drive object is not created yet. 
     */
    FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID,

    /*! This default spare config type.  The provision drive can be used when as 
     *  new raid group is created or it can be used as a spare (replacement) drive.
     */
    FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED,

    /*! It indicates that this drive is either currently configured as 
     * part of the raid group or it was configured before.
     */
    FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID,

    /*! The drive has been marked `failed' (i.e. unusable).
     */
    FBE_PROVISION_DRIVE_CONFIG_TYPE_FAILED,

    /*! This attribute indicates provision drive is configured as hotspare 
     * and it is part of the spare pool.
     */
    FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE,

    /*! The drive configured as part of the extent_pool.
     */
    FBE_PROVISION_DRIVE_CONFIG_TYPE_EXT_POOL,
} fbe_provision_drive_config_type_t;

/*! @note Only add to these values.  Never remove any of them. */
typedef enum fbe_provision_drive_configured_physical_block_size_e{
    FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_INVALID,
    FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_512,
    FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520,
    FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_4160,
    FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_LAST
}fbe_provision_drive_configured_physical_block_size_t;

  /*!**********************************************************************
 * @enum fbe_provision_drive_debug_flags_t
 *  
 *  @brief Debug flags that allow run-time debug functions for PVD object.
 *
 *  @note Typically these flags should only be set for unit or
 *        intergration testing since they could have SEVERE 
 *        side effects.
 *
*************************************************************************/
typedef enum fbe_provision_drive_debug_flags_e
{
    FBE_PROVISION_DRIVE_DEBUG_FLAG_NONE                       = 0x00000000,

    /*! Log information which is generic.
     */
    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING      =       0x00000001,

    /*! Log information for background zeroing operation.
     */
    FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING             =    0x00000002,

    /*! Log information for user zeroing request.
     */
    FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING  =         0x00000004,

    /*! Log information for zero on demand read/write.
     */
    FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING             =    0x00000008,

    /*! Log information for sniff verify operation .
     */
    FBE_PROVISION_DRIVE_DEBUG_FLAG_VERIFY_TRACING        =      0x00000010,

    /*! Log information for PVD metadata update operation.
     */
    FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING   =         0x00000020,

    /*! Log the stripe lock tracing. 
    */
    FBE_PROVISION_DRIVE_DEBUG_FLAG_LOCK_TRACING =               0x00000040,

    FBE_PROVISION_DRIVE_DEBUG_FLAG_SNIFF_TRACING =              0x00000080,

    FBE_PROVISION_DRIVE_DEBUG_FLAG_SLF_TRACING =                0x00000100,

    FBE_PROVISION_DRIVE_DEBUG_FLAG_HEALTH_CHECK_TRACING =       0x00000200,

    FBE_PROVISION_DRIVE_DEBUG_FLAG_NO_WRITE_SAME              = 0x00000400,
    FBE_PROVISION_DRIVE_DEBUG_FLAG_ENCRYPTION                 = 0x00000800,

    FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE                = 0x00001000,

    FBE_PROVISION_DRIVE_DEBUG_FLAG_HEALTH_TRACING =             0x00002000,

    FBE_PROVISION_DRIVE_DEBUG_FLAG_LAST                       = 0x00004000,

} fbe_provision_drive_debug_flags_t;


/*!*******************************************************************
 * @enum fbe_provision_drive_error_precedence_t
 *********************************************************************
 * @brief
 *   Enumeration of the different error precedence.  This determines
 *   which error is reported for a multi-subrequests I/O. They are
 *   enumerated from lowest to highest weight.
 *
 *********************************************************************/
typedef enum fbe_provision_drive_error_precedence_e
{
    FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_NO_ERROR           = 0,
    FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_INVALID_REQUEST    = 10,
    FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_MEDIA_ERROR        = 20,
    FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_NOT_READY          = 30,
    FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_ABORTED            = 40,
    FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_TIMEOUT            = 50,
    FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_DEVICE_DEAD        = 60,
    FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_UNKNOWN            = 100,
} fbe_provision_drive_error_precedence_t;


/*!*******************************************************************
 * @struct fbe_provision_drive_update_config_type_s
 *********************************************************************
 * @brief
 *  This structure is used with FBE_DATABASE_CONTROL_CODE_UPDATE_PROVISION_DRIVE
 * 
 *  It is used to update the provision drive configuration.
 *
 *********************************************************************/
typedef struct fbe_provision_drive_update_config_type_s {
    /*! Provision drive object gets created with configuration mode as unknown but 
     *  it will change its configuration type when it is configured either as SPARE
     *  or as RAID.
     */
    fbe_provision_drive_config_type_t   config_type;

    /*! Provision drive object gets created with sniff verify state as enabled.
     */
    fbe_bool_t                  sniff_verify_state;

    /*! Put any future user configured parameters here.
     */
} fbe_provision_drive_update_config_type_t;


/*!****************************************************************************
 * @struct fbe_provision_drive_get_verify_status_t
 *
 * @brief
 *    This provision drive structure holds sniff verify status information.
 *
 *    Sniff verify status for a given provision drive can be retrieved using
 *    the FBE_PROVISION_DRIVE_CONTROL_CODE_GET_VERIFY_STATUS control code.
 *
 ******************************************************************************/

typedef struct fbe_provision_drive_get_verify_status_s
{
    fbe_bool_t                      verify_enable;      // sniff verify enable flag
    fbe_lba_t                       verify_checkpoint;  // sniff verify checkpoint
    fbe_lba_t                       exported_capacity;  // exported capacity
    fbe_u8_t                        precentage_completed; //precentage of disk that was already 
                                                          //verified in current pass 

} fbe_provision_drive_get_verify_status_t;


/*!****************************************************************************
 * @struct fbe_provision_drive_set_verify_checkpoint_t
 *
 * @brief
 *    This provision drive structure holds the new sniff verify checkpoint.
 *
 *    Sniff verify checkpoint for a given provision drive can be set using the
 *    FBE_PROVISION_DRIVE_CONTROL_CODE_SET_VERIFY_CHECKPOINT control code.
 *
 ******************************************************************************/

typedef struct fbe_provision_drive_set_verify_checkpoint_s
{
    fbe_lba_t                       verify_checkpoint;  // new sniff verify checkpoint

} fbe_provision_drive_set_verify_checkpoint_t;


/*!****************************************************************************
 * @struct fbe_provision_drive_verify_error_counts_t
 *
 * @brief
 *    This provision drive structure holds verify results data collected during
 *    sniff verify operations and used in verify reports.
 *
 ******************************************************************************/

typedef struct fbe_provision_drive_verify_error_counts_s
{
    fbe_u32_t                       recov_read_count;   // recoverable read errors
    fbe_u32_t                       unrecov_read_count; // unrecoverable read errors

} fbe_provision_drive_verify_error_counts_t;


/*!****************************************************************************
 * @struct fbe_provision_drive_verify_report_t
 *
 * @brief
 *    This provision drive structure holds verify report information collected
 *    during sniff verify operations and presented as a single report.
 *
 *    A sniff verify report for a given provision drive can be retrieved using
 *    the FBE_PROVISION_DRIVE_CONTROL_CODE_GET_VERIFY_REPORT control code.
 *
 *    A sniff verify report for a given provision drive can  be  cleared using
 *    the FBE_PROVISION_DRIVE_CONTROL_CODE_CLEAR_VERIFY_REPORT control code.
 *
 ******************************************************************************/

typedef struct fbe_provision_drive_verify_report_s
{
    fbe_u32_t                       revision;           // verify report revision number
    fbe_u32_t                       pass_count;         // number of passes completed

    fbe_provision_drive_verify_error_counts_t current;  // counts from the current pass
    fbe_provision_drive_verify_error_counts_t previous; // counts from the previous pass
    fbe_provision_drive_verify_error_counts_t totals;   // accumulated total counts

} fbe_provision_drive_verify_report_t;

#define FBE_PROVISION_DRIVE_VERIFY_REPORT_REV 1         // current revision of verify report


/*!**********************************************************************
 * @enum fbe_provision_drive_slf_type_e
 *  
 *  @brief 
 * These are the types for single loop failure. 
 *
*************************************************************************/
typedef enum fbe_provision_drive_slf_state_e
{
    /*! No single loop failure. 
     */
    FBE_PROVISION_DRIVE_SLF_NONE,
    /*! SLF on this SP 
     */
    FBE_PROVISION_DRIVE_SLF_THIS_SP,
    /*! SLF on peer SP 
     */
    FBE_PROVISION_DRIVE_SLF_PEER_SP,
    /*! SLF on both SP. PVD failed. 
     */
    FBE_PROVISION_DRIVE_SLF_BOTH_SP,
} 
fbe_provision_drive_slf_state_t;

/*!********************************************************************* 
 * @enum fbe_provision_drive_flags_t
 *  
 * @brief 
 * This enumeration lists all the provision drive specific flags. 
 *         
 **********************************************************************/
typedef enum fbe_provision_drive_flags_e
{
    FBE_PROVISION_DRIVE_FLAG_INVALID                    = 0,
    FBE_PROVISION_DRIVE_FLAG_SNIFF_VERIFY_ENABLED       = 0x00000001,
    FBE_PROVISION_DRIVE_FLAG_DOWNLOAD_ORIGINATOR        = 0x00000002,
    FBE_PROVISION_DRIVE_FLAG_HC_ORIGINATOR              = 0x00000004,
    FBE_PROVISION_DRIVE_FLAG_EVENT_OUTSTANDING          = 0x00000008,
    FBE_PROVISION_DRIVE_FLAG_SPIN_DOWN_QUALIFIED        = 0x00000010,
    FBE_PROVISION_DRIVE_FLAG_CHECK_LOGICAL_STATE        = 0x00000020,
    FBE_PROVISION_DRIVE_FLAG_CHECK_PASSIVE_REQUEST      = 0x00000040,
    FBE_PROVISION_DRIVE_FLAG_CREATE_AFTER_ENCRYPTION    = 0x00000080,
    FBE_PROVISION_DRIVE_FLAG_SCRUB_NEEDED               = 0x00000100,
    FBE_PROVISION_DRIVE_FLAG_UNMAP_SUPPORTED            = 0x00000200,
    FBE_PROVISION_DRIVE_FLAG_UNMAP_ENABLED              = 0x00000400,
    FBE_PROVISION_DRIVE_FLAG_REPORT_WPD_WARNING         = 0x00000800,
    FBE_PROVISION_DRIVE_FLAG_LAST                       = 0xffffffff,
}
fbe_provision_drive_flags_t;


/*!****************************************************************************
 *    
 * @struct fbe_provision_drive_info_t
 *  
 * @brief 
 *   This is the definition of the drive information which we return to GET drive info usurper
 *   command..
 ******************************************************************************/
typedef struct fbe_provision_drive_info_s
{
    fbe_lba_t                                               capacity;
    fbe_provision_drive_config_type_t                       config_type;
    fbe_provision_drive_configured_physical_block_size_t    configured_physical_block_size;
    fbe_lba_t                                               paged_metadata_lba;
    fbe_lba_t                                               paged_metadata_capacity;
    fbe_lba_t                                               paged_mirror_offset;
    fbe_chunk_size_t chunk_size;    /*!< Number of blocks represented by one chunk. */
    fbe_chunk_count_t total_chunks; /*!< Total number of paged chunks. */
    fbe_u8_t                                                serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
    fbe_u32_t                                               port_number;
    fbe_u32_t                                               enclosure_number;
    fbe_u32_t                                               slot_number;
    fbe_drive_type_t                                        drive_type;
    fbe_block_count_t                                       max_drive_xfer_limit;
    fbe_provision_drive_debug_flags_t                       debug_flags;
    fbe_bool_t                                              end_of_life_state;
    fbe_bool_t                                              is_system_drive;
    fbe_lba_t                                               media_error_lba;
    fbe_bool_t                                              zero_on_demand;
    fbe_bool_t                                              sniff_verify_state;
    fbe_lba_t                                               default_offset;
    fbe_block_count_t                                       default_chunk_size;
    fbe_lba_t                                               zero_checkpoint;
    fbe_lba_t                                               imported_capacity;
    fbe_provision_drive_slf_state_t                         slf_state;
    fbe_bool_t                                              spin_down_qualified; /* PVD is spin down qualified or not. */
    fbe_bool_t                                              power_save_capable; /* power save capable not only requires the PVD is spin down 
                                                                                   qualified, but also requires not system drive. */
    fbe_bool_t                                              drive_fault_state;
    fbe_bool_t                                              created_after_encryption_enabled;
    fbe_bool_t                                              scrubbing_in_progress;
    fbe_u32_t                                               percent_rebuilt;    /* A parent raid group will `push' this information down */
    fbe_bool_t                                              swap_pending;
    fbe_provision_drive_flags_t                             flags;
}fbe_provision_drive_info_t;


/* FBE_PROVISION_DRIVE_CONTROL_CODE_CLASS_CALCULATE_CAPACITY */
typedef struct fbe_provision_drive_control_class_calculate_capacity_s {
    fbe_lba_t imported_capacity;                    /* LDO capacity */
    fbe_block_edge_geometry_t block_edge_geometry;  /* Downstream edge geometry */
    fbe_lba_t exported_capacity;                    /* PVD calculated capacity */
}fbe_provision_drive_control_class_calculate_capacity_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_SET_ZERO_CHECKPOINT */
typedef struct fbe_provision_drive_control_set_zero_checkpoint_s {
    fbe_lba_t background_zero_checkpoint; 
}fbe_provision_drive_control_set_zero_checkpoint_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_GET_ZERO_CHECKPOINT */
typedef struct fbe_provision_drive_control_get_zero_checkpoint_s {
    fbe_lba_t           zero_checkpoint; 
}fbe_provision_drive_get_zero_checkpoint_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_SET_ZERO_CHECKPOINT */
typedef struct fbe_provision_drive_control_set_unmap_enabled_disabled_s {
    fbe_bool_t is_enabled; 
}fbe_provision_drive_control_set_unmap_enabled_disabled_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_REGISTER_KEYS */
typedef struct fbe_provision_drive_control_register_keys_s {
    fbe_encryption_key_info_t *key_handle; 
    fbe_encryption_key_info_t *key_handle_paco; 
}fbe_provision_drive_control_register_keys_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_GET_MINIPORT_KEY_HANDLE */
typedef struct fbe_provision_drive_control_get_mp_key_handle_s {
    fbe_edge_index_t index; 
    fbe_key_handle_t mp_handle_1;
    fbe_key_handle_t mp_handle_2;
}fbe_provision_drive_control_get_mp_key_handle_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_SET_VERIFY_INVALIDATE_CHECKPOINT */
typedef struct fbe_provision_drive_control_set_verify_invalidate_checkpoint_s {
    fbe_lba_t verify_invalidate_checkpoint; 
}fbe_provision_drive_set_verify_invalidate_checkpoint_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_GET_VERIFY_INVALIDATE_CHECKPOINT */
typedef struct fbe_provision_drive_control_get_verify_invalidate_checkpoint_s {
    fbe_lba_t           verify_invalidate_checkpoint; 
}fbe_provision_drive_get_verify_invalidate_checkpoint_t;


/* FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PHYSICAL_DRIVE_LOCATION */
typedef struct fbe_base_config_physical_drive_location_s fbe_provision_drive_get_physical_drive_location_t;

/*
 *  FBE_PROVISION_DRIVE_CONTROL_CODE_SET_METADATA_PAGED_BITS
 *  FBE_PROVISION_DRIVE_CONTROL_CODE_GET_METADATA_PAGED_BITS 
 *  FBE_PROVISION_DRIVE_CONTROL_CODE_CLEAR_METADATA_PAGED_BITS
 *  FBE_PROVISION_DRIVE_CONTROL_CODE_METADATA_PAGED_WRITE
 */ 
#pragma pack(1)
typedef struct fbe_provision_drive_control_paged_bits_s{
    fbe_u16_t valid_bit:1;
    fbe_u16_t need_zero_bit:1;
    fbe_u16_t user_zero_bit:1;
    fbe_u16_t consumed_user_data_bit:1;
    fbe_u16_t unused_bit:12;    
}fbe_provision_drive_control_paged_bits_t;
#pragma pack()

typedef struct fbe_provision_drive_control_paged_metadata_s{
    fbe_u64_t       metadata_offset;
    fbe_u32_t       metadata_record_data_size;
    fbe_u64_t       metadata_repeat_count;
    fbe_u64_t       metadata_repeat_offset;
    fbe_provision_drive_control_paged_bits_t  metadata_bits;
}fbe_provision_drive_control_paged_metadata_t;

/*FBE_PROVISION_DRIVE_CONTROL_CODE_SET_BACKGROUND_PRIORITIES*/
/*FBE_PROVISION_DRIVE_CONTROL_CODE_GET_BACKGROUND_PRIORITIES*/
typedef struct fbe_provision_drive_set_priorities_s{
    fbe_traffic_priority_t zero_priority;
    fbe_traffic_priority_t verify_priority;
    fbe_traffic_priority_t verify_invalidate_priority;
}fbe_provision_drive_set_priorities_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PORT_INFO */ 
typedef struct fbe_provision_drive_get_port_info_s {
    /*see:  FBE_BASE_PORT_CONTROL_CODE_GET_PORT_INFO */
    fbe_port_type_t port_type;
    fbe_u32_t io_port_number;
    fbe_u32_t io_portal_number;
    fbe_port_speed_t port_link_speed;
} fbe_provision_drive_get_port_info_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_GET_METADATA_MEMORY */
typedef struct fbe_provision_drive_control_get_metadata_memory_s{
    fbe_lifecycle_state_t   lifecycle_state;
    fbe_power_save_state_t  power_save_state;

    /* Peer */
    fbe_lifecycle_state_t   lifecycle_state_peer;
    fbe_power_save_state_t  power_save_state_peer;
}fbe_provision_drive_control_get_metadata_memory_t;


/*the data structure is used to get the PVD metadata memory and version header*/
/*FBE_PROVISION_DRIVE_CONTROL_CODE_GET_VERSIONED_METADATA_MEMORY*/
typedef struct fbe_provision_drive_control_get_versioned_metadata_memory_s{
    fbe_sep_version_header_t ver_header;
    fbe_sep_version_header_t peer_ver_header;

    fbe_lifecycle_state_t   lifecycle_state;
    fbe_power_save_state_t  power_save_state;
    /* Peer */
    fbe_lifecycle_state_t   lifecycle_state_peer;
    fbe_power_save_state_t  power_save_state_peer;
}fbe_provision_drive_control_get_versioned_metadata_memory_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_SET_DEBUG_FLAGS */
/* FBE_PROVISION_DRIVE_CONTROL_CODE_SET_CLASS_DEBUG_FLAGS */
typedef struct fbe_provision_drive_set_debug_trace_flag_s {
    fbe_provision_drive_debug_flags_t   trace_flag;
}fbe_provision_drive_set_debug_trace_flag_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_SEND_DOWNLOAD */
typedef struct fbe_provision_drive_send_download_s{
    fbe_object_id_t                             object_id;
    fbe_bool_t                                  fast_download;
    fbe_scsi_error_code_t                       download_error_code;
    fbe_payload_block_operation_status_t        status;
    fbe_payload_block_operation_qualifier_t     qualifier;
    fbe_u32_t                                   transfer_count;/*Image size in bytes*/
}fbe_provision_drive_send_download_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_GET_DOWNLOAD_INFO */
typedef struct fbe_provision_drive_get_download_info_s{
    fbe_bool_t  download_originator;
    fbe_u64_t   local_state;
    fbe_u64_t   clustered_flag;
    fbe_u64_t   peer_clustered_flag;
}fbe_provision_drive_get_download_info_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_GET_HEALTH_CHECK_INFO */
typedef struct fbe_provision_drive_get_health_check_info_s{
    fbe_bool_t  originator;
    fbe_u64_t   local_state;
    fbe_u64_t   clustered_flag;
    fbe_u64_t   peer_clustered_flag;
}fbe_provision_drive_get_health_check_info_t;


/*FBE_PROVISION_DRIVE_CONTROL_CODE_GET_OPAQUE_DATA*/
#define FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX 128
typedef struct fbe_provision_drive_get_opaque_data_s{
    fbe_u8_t opaque_data[FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX];
}fbe_provision_drive_get_opaque_data_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_MAP_LBA_TO_CHUNK */

typedef struct fbe_provision_drive_map_info_s 
{
    fbe_lba_t lba; /*!< This is the only input. */

    /* Below this point are only outputs.
     */
    fbe_chunk_index_t chunk_index; /*!< Paged metadata chunk for the above lba. */
    fbe_lba_t metadata_pba; /*!< physical lba drive where the metadata resides. */
}
fbe_provision_drive_map_info_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_GET_POOL_ID */
/* FBE_PROVISION_DRIVE_CONTROL_CODE_SET_POOL_ID */
typedef struct fbe_provision_drive_control_pool_id_s {
    fbe_u32_t           pool_id; 
}fbe_provision_drive_control_pool_id_t;


/* FBE_PROVISION_DRIVE_CONTROL_CODE_SET_BG_OP_SPEED */
typedef struct fbe_provision_drive_control_set_bg_op_speed_s {
    fbe_u32_t           background_op_speed; 
    fbe_base_config_background_operation_t background_op;    
}fbe_provision_drive_control_set_bg_op_speed_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_GET_BG_OP_SPEED */
typedef struct fbe_provision_drive_control_get_bg_op_speed_s {
    fbe_u32_t           disk_zero_speed; 
    fbe_u32_t           sniff_speed; 
}fbe_provision_drive_control_get_bg_op_speed_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_SET_PERCENT_REBUILT */
typedef struct fbe_provision_drive_control_set_percent_rebuilt_s {
    fbe_u32_t           percent_rebuilt; 
}fbe_provision_drive_control_set_percent_rebuilt_t;

/*!**********************************************************************
 * @enum fbe_provision_drive_background_op_e
 *  
 *  @brief Background operations supported by provision drive object
 *         Each operation represent with specific bit value.
 *
 *
*************************************************************************/
typedef enum fbe_provision_drive_background_op_e
{
    FBE_PROVISION_DRIVE_BACKGROUND_OP_NONE                 = 0x0000,
    FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING         = 0x0001,
    FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF                = 0x0002,
    FBE_PROVISION_DRIVE_BACKGROUND_OP_ALL                  = 0x0003, /* bitmask of all operations */
}fbe_provision_drive_background_op_t;


/*!**********************************************************************
 * @enum fbe_provision_drive_zero_notification_e
 *  
 *  @brief Used to notify upper layers if the drive started/ended or progressing with Zero
 *
 *
*************************************************************************/
typedef enum fbe_object_zero_notification_s
{
    FBE_OBJECT_ZERO_STARTED,
    FBE_OBJECT_ZERO_ENDED,
    FBE_OBJECT_ZERO_IN_PROGRESS

}fbe_object_zero_notification_t;

/*!**********************************************************************
 * @enum fbe_provision_drive_zero_notification_data_s
 *  
 *  @brief data associated with the zero operation
 *
 *
*************************************************************************/
typedef struct fbe_object_zero_notification_data_s
{
    fbe_object_zero_notification_t 	zero_operation_status;
    fbe_u32_t						zero_operation_progress_percent;/*valid only when zero_operation_status = FBE_OBJECT_ZERO_IN_PROGRESS*/
    fbe_bool_t                      scrub_complete_to_be_set;       /* valid only when FBE_OBJECT_ZERO_ENDED */
}fbe_object_zero_notification_data_t;


/*!**********************************************************************
 * @enum fbe_provision_drive_copy_to_status_s
 *  
 *  @brief this is the copy to disk status which can be used as a meaningful
 *         failure reason for spare validation and can be mapped to
 *         corresponding spare job error type.
 *
 *
*************************************************************************/
typedef enum fbe_provision_drive_copy_to_status_e 
{
    /* there is no error. */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR = 0,
    /* internal operation failure.*/
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_INTER_OP_FAILURE,
    /* the copy request exceeded the time allowed. */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_EXCEEDED_ALLOTTED_TIME,
    /* no device. */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_UPSTREAM_RAID_GROUP,
    /* only expected (1) upstream vd*/
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_UNEXPECTED_DRIVE_CONFIGURATION,
    /* invalid spare object id. */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_INVALID_ID,
    /* spare drive is not in the READY state. */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_NOT_READY,
    /* spare drive is removed or end of life. */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_REMOVED,
    /* spare drive is busy and being used which means either the copy on
     that spare drive is in progress or spare drive is in RG.*/
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_BUSY,
    /* source drive is busy which means copyTo on this source drive is in progress.*/
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_SOURCE_BUSY,
    /* dest drive and source drive are not on the same performance tier. */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_PERF_TIER_MISMATCH,
    /* dest drive block size is not equal to source block size. */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_BLOCK_SIZE_MISMATCH,
    /* dest drive capacity is less than source drive cap. */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_CAPACITY_MISMATCH,
    /* target drive is system drive, src & dest drive edge offest do not match. */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_SYS_DRIVE_MISMATCH,
    /* Interanl logic error */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_UNEXPECTED_ERROR,
    /* EOL is no longer set on source edge */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_PROACTIVE_SPARE_NOT_REQUIRED,
    /* The virtual drive is not in Ready/Hibernate state.*/
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_VIRTUAL_DRIVE_BROKEN,
    /* The virtual drive is not in a mode (for instance it is in mirror mode) to support the request.*/
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT,
    /* EOL is set on the destination drive and so no copy-to is allowed.. */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_DESTINATION_DRIVE_NOT_HEALTHY,
    /* The parent raid group must be redundant (i.e. RAID-0). */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_COPY_NOT_ALLOWED_TO_NON_REDUNDANT_RAID_GROUP,
    /* The parent raid group must be in the Ready state. */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_COPY_NOT_ALLOWED_TO_BROKEN_RAID_GROUP,
    /* The parent raid group must be consumed (i.e. have LUN(s) bound). */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_COPY_NOT_ALLOWED_TO_UNCONSUMED_RAID_GROUP,
    /* The parent raid group cannot be degraded. */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_RAID_GROUP_IS_DEGRADED,
    /* unsupported command */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_UNSUPPORTED_COMMAND,
    /* no spares available */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_SPARES_AVAILABLE,
    /* no suitable spare */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_SUITABLE_SPARES,
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_RG_COPY_ALREADY_IN_PROGRESS,
    /* multiple RAID groups */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_MORE_THAN_ONE_RAID_GROUP,
    /* degraded proactive RG */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_PROACTIVE_RAID_GROUP_DEGRADED_OR_UNCONSUMED,
    /* degraded user copy RG */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_USER_COPY_RAID_GROUP_DEGRADED_OR_UNCONSUMED,
    /* Upstream raid group denied the copy (encryption in progress, odfu etc)*/
    FBE_PROVISION_DRIVE_COPY_TO_UPSTREAM_RAID_GROUP_DENIED,
    /* unknown status. */
    FBE_PROVISION_DRIVE_COPY_TO_STATUS_UNKNOWN,

    /*! Add COPY TO error status here. */

} fbe_provision_drive_copy_to_status_t;
/* FBE_PROVISION_DRIVE_CONTROL_CODE_SET_ZERO_CHECKPOINT */
typedef struct fbe_provision_drive_control_set_removed_timestamp_s {
    fbe_system_time_t removed_tiem_stamp;
    fbe_bool_t  is_persist_b;
}fbe_provision_drive_control_set_removed_timestamp_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_GET_ZERO_CHECKPOINT */
typedef struct fbe_provision_drive_control_get_removed_timestamp_s {
    fbe_system_time_t removed_tiem_stamp; 
}fbe_provision_drive_get_removed_timestamp_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SLF_STATE */
typedef struct fbe_provision_drive_get_slf_state_s {
    fbe_bool_t is_drive_slf; 
}fbe_provision_drive_get_slf_state_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_GET_DEK_INFO */
typedef struct fbe_provision_drive_get_key_info_s {
    fbe_edge_index_t server_index;
    fbe_encryption_key_mask_t key_mask;
    fbe_bool_t key1_valid;
    fbe_bool_t key2_valid;
    fbe_object_id_t port_object_id;    
}fbe_provision_drive_get_key_info_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PAGED_CACHE_INFO */
typedef struct fbe_provision_drive_get_paged_cache_info_s {
    fbe_u32_t           miss_count;
    fbe_u32_t           hit_count;
}fbe_provision_drive_get_paged_cache_info_t;

/*!**********************************************************************
 * @enum fbe_provision_drive_swap_pending_reason_e
 *  
 *  @brief 
 * These are the reasons that a provision could have been marked offline.
 *
*************************************************************************/
typedef enum fbe_provision_drive_swap_pending_reason_e
{
    /*! Provision drive is not marked offline.
     */
    FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_NONE,

    /*! Drive marked offline due to proactive copy swap-out.
     */
    FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_PROACTIVE_COPY,
    
    /*! Drive marked offline due to user copy swap-out.
     */
    FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_USER_COPY,


    /*! Last/number of offline reasons.
     */
    FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_LAST,
} 
fbe_provision_drive_swap_pending_reason_t;

/* FBE_PROVISION_DRIVE_CONTROL_SET_SWAP_PENDING */
typedef struct fbe_provision_drive_set_swap_pending_s {
    fbe_provision_drive_swap_pending_reason_t set_swap_pending_reason;
}fbe_provision_drive_set_swap_pending_t;

/* FBE_PROVISION_DRIVE_CONTROL_GET_SWAP_PENDING */
typedef struct fbe_provision_drive_get_swap_pending_s {
    fbe_provision_drive_swap_pending_reason_t get_swap_pending_reason;
}fbe_provision_drive_get_swap_pending_t;

/* FBE_PROVISION_DRIVE_CONTROL_GET_EAS_STATE */
typedef struct fbe_provision_drive_get_eas_state_s {
    fbe_bool_t is_started;
    fbe_bool_t is_complete;
}fbe_provision_drive_get_eas_state_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SSD_STATISTICS */
typedef struct fbe_provision_drive_get_ssd_statistics_s {
    fbe_u64_t max_erase_count;
    fbe_u64_t average_erase_count;
    fbe_u64_t eol_PE_cycles;
    fbe_u64_t power_on_hours;
}fbe_provision_drive_get_ssd_statistics_t;

/* FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SSD_BLOCK_LIMITS */
typedef struct fbe_provision_drive_get_ssd_block_limits_s {
    fbe_u64_t max_transfer_length;
    fbe_u64_t max_unmap_lba_count;
    fbe_u64_t max_unmap_block_descriptor_count;
    fbe_u64_t optimal_unmap_granularity;
    fbe_u64_t unmap_granularity_alignment;
    fbe_u64_t max_write_same_length;
}fbe_provision_drive_get_ssd_block_limits_t;

typedef fbe_u64_t fbe_provision_drive_clustered_flags_t;
typedef struct fbe_provision_drive_metadata_memory_s{
    fbe_base_config_metadata_memory_t base_config_metadata_memory; /* MUST be first */
    fbe_provision_drive_clustered_flags_t flags;
    fbe_u32_t           port_number;
    fbe_u32_t           enclosure_number; 
    fbe_u32_t           slot_number;
}fbe_provision_drive_metadata_memory_t;

/*!*******************************************************************
 * @struct fbe_provision_drive_validate_block_encryption_t
 *********************************************************************
 * @brief Structure to pass information on what PBAs to check for encryption.
 * and returns if they are encrypted or not
 *********************************************************************/
typedef struct fbe_provision_drive_validate_block_encryption_s
{
    fbe_lba_t encrypted_pba; /*!< PBA to check if it is encrypted or not  */
    fbe_block_count_t num_blocks_to_check; /*!< Number of blocks to check  */
    fbe_block_count_t blocks_encrypted; /*!< OUTPUT - Actual number of blocks encrypted  */
}
fbe_provision_drive_validate_block_encryption_t;

/* Provision drive - paged metadata structure (2 Byte size) */ 
#pragma pack(1)

typedef struct fbe_provision_drive_nonpaged_metadata_drive_info_s
{
    fbe_u32_t           port_number;
    fbe_u32_t           enclosure_number; 
    fbe_u32_t           slot_number;
    fbe_drive_type_t    drive_type;
} fbe_provision_drive_nonpaged_metadata_drive_info_t;

typedef fbe_u32_t fbe_provision_drives_np_flags_t;
/*!*******************************************************************
 * @enum fbe_provision_drive_np_flags_t
 *********************************************************************
 * @brief Flags to indicate state pvd.
 *
 *********************************************************************/
typedef enum fbe_provision_drive_np_flags_e
{
    /*! No flags set.
     */
    FBE_PROVISION_DRIVE_NP_FLAG_INVALID                     = 0x00000000,

    /*! The paged needs to be re-written. 
     */
    FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_CONSUMED        = 0x00000001, 

    FBE_PROVISION_DRIVE_NP_FLAG_PAGED_VALID                 = 0x00000002,

    FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_ZERO            = 0x00000004,

    FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED          = 0x00000008,
    FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_COMPLETE        = 0x00000010,
    FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_INTENT          = 0x00000020,

    FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_PROACTIVE_COPY = 0x00000040,
    FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_USER_COPY      = 0x00000080,

    FBE_PROVISION_DRIVE_NP_FLAG_EAS_STARTED                 = 0x00000100,
    FBE_PROVISION_DRIVE_NP_FLAG_EAS_COMPLETE                = 0x00000200,

    FBE_PROVISION_DRIVE_NP_FLAG_LAST                        = 0x00000400
}
fbe_provision_drive_np_flags_t;

#define FBE_PROVISION_DRIVE_NP_FLAG_SCRUB_BITS (FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED|\
                                                FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_COMPLETE |\
                                                FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_INTENT)

#define FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_BITS (FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_PROACTIVE_COPY|\
                                                       FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_USER_COPY)


/*!****************************************************************************
 * @struct fbe_provision_drive_nonpaged_metadata_t
 *
 * @brief
 *   This structure contains the persistent, non-paged metadata for the
 *   provision drive object.
 * 
 ******************************************************************************/

typedef struct fbe_provision_drive_nonpaged_metadata_s
{
    /*! nonpaged metadata which is common to all the objects derived from
     *  the base config.
     */
    fbe_base_config_nonpaged_metadata_t base_config_nonpaged_metadata;  /* MUST be first */

    /*! sniff verify checkpoint, one for each provision drive; identifies
     *  lda of next chunk of a given disk to be checked for media errors
     */
    fbe_lba_t   sniff_verify_checkpoint;

    fbe_lba_t   zero_checkpoint;

    /*! Zero on demand transition flag, it is to identify whether we are in 
     *  zero on demand mode or not.
     */
    fbe_bool_t  zero_on_demand;

    /*! It indiciates whether this drive has end of life state set, if it has 
     *  end of life state set and if it is configured as unknown then it will
     *  remain in fail state.
     */ 
    fbe_bool_t  end_of_life_state;

    /*! port, enclosure and slot information tracking for the pvd object.
     */
    fbe_provision_drive_nonpaged_metadata_drive_info_t nonpaged_drive_info;

    /*! Will be used by sniff verify to save media error lba
     */ 
    fbe_lba_t   media_error_lba;

    /*! record the time piont of an pvd when drive removed
    */ 
    fbe_system_time_t   remove_timestamp;

    /*! Metadata verify checkpoint - Identifies which 
     *  lba to start reading and validating the metadata
     *  bits 
     */
    fbe_lba_t verify_invalidate_checkpoint;

    /*! It indiciates whether this drive has drive fault state set, if it has 
     *  the state set and it will remain in fail state.
     */ 
    fbe_bool_t drive_fault_state;

    fbe_provision_drive_np_flags_t flags;
    /* One bit per client to indicate we initialized the 
     * encryption validation area. 
     */
    fbe_u32_t validate_area_bitmap;

    /*! It indicates the number of sniff passes completed
      */
    fbe_u32_t pass_count;
    /*NOTES!!!!!!*/
    /*non-paged metadata versioning requires that any modification for this data structure should 
      pay attention to version difference between disk and memory. Please note that:
      1) the data structure should be appended only (after MCR released first version)
      2) the metadata verify conditon in its monitor will detect version difference, The
         new version developer is requried to set its defualt value when the new version
         software detects old version data in the metadata verify condition of specialize state
      3) database service maintain the committed metadata size, so any modification please
         invalidate the disk
    */

} fbe_provision_drive_nonpaged_metadata_t;

/*

    NZ = need_zero_bit
    UZ = user_zero_bit
    CUD = consumed_user_data_bit

    truth table:
     NZ UZ	CUD
     0 	0 	0	=	Illegal 
     0 	0 	1 	=
     0 	1 	0   = 
     0 	1 	1	=
     1 	0 	0	= 
     1 	1 	0	=
     1 	0	1 	=
     1 	1 	1	=
*/
typedef struct fbe_provision_drive_paged_metadata_s
{
    fbe_u16_t valid_bit:1; /* If TRUE, this paged metadata is valid. 
                              If FALSE, this paged metadata encountered uncorrectable read error and was zeroed by MDS */
    fbe_u16_t need_zero_bit:1; /* If TRUE, this area has to be zeroed out, if FALSE, this area was already zeroed by PVD */
    fbe_u16_t user_zero_bit:1; /* if TRUE, if the user requested to zero this area, needs to be cleaned after zeroing*/
    fbe_u16_t consumed_user_data_bit:1; /* associate chunk has valid user data if set TRUE */
    fbe_u16_t unused_bit:12;   /* unused bits */
}fbe_provision_drive_paged_metadata_t;

#define FBE_PROVISION_DRIVE_PAGED_DATA_SIZE_FOR_EXT_POOL 64
typedef struct fbe_provision_drive_paged_metadata_for_pool_s
{
    union{                                          
        fbe_u8_t pad[FBE_PROVISION_DRIVE_PAGED_DATA_SIZE_FOR_EXT_POOL];
    }u;
}fbe_provision_drive_paged_metadata_for_pool_t;

#pragma pack()

#endif /* FBE_PROVISION_DRIVE_H */

/*************************
 * end file fbe_provision_drive.h
 *************************/

