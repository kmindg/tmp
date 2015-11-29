#ifndef FBE_VIRTUAL_DRIVE_H
#define FBE_VIRTUAL_DRIVE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_virtual_drive.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the
 *  virtual_drive object.
 * 
 * @ingroup virtual_drive_class_files
 * 
 * @revision
 *   5/20/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_object.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe_spare.h"
#include "fbe/fbe_port.h"
#include "fbe_limits.h"
#include "fbe/fbe_raid_group.h"

/*************************
 *   FUNCTION DEFINITIONS 
 *************************/

 /*! @defgroup virtual_drive_class Virtual Drive Class
 *  @brief This is the set of definitions for the base config.
 *  @ingroup fbe_base_object
 */ 
/*! @defgroup virtual_drive_usurper_interface Virtual Drive Usurper Interface
 *  @brief This is the set of definitions that comprise the base config class
 *  usurper interface.
 *  @ingroup fbe_classes_usurper_interface
 * @{
 */ 

/*! @def FBE_VIRTUAL_DRIVE_WIDTH
 *  @brief this is the (1) and only width supported for a virtual drive object
 */
#define FBE_VIRTUAL_DRIVE_WIDTH 2

/*!*******************************************************************
 * @def FBE_VIRTUAL_DRIVE_MAX_TRACE_CHARS
 *********************************************************************
 * @brief max number of chars we will put in a formatted trace string.
 *
 *********************************************************************/
#define FBE_VIRTUAL_DRIVE_MAX_TRACE_CHARS 128

/*! @enum fbe_virtual_drive_configuration_mode_t 
 *  
 *  @brief Mutually exclusive configuration mode of the virtual drive object,.
 *  this information will be persisted to the configuration information for
 *  the virtual drive object.
 *  
 */
typedef enum fbe_virtual_drive_configuration_mode_e
{
    /*! Virtual drive object hasn't been initialized.
     */
    FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN                = 0,

    /*! Indicates that virtual drive is currently configured as pass through
     *  mode and it expects only edge0 to be online before it goes to activate
     *  state and become READY.
     */
    FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE   = 1,

    /*! Indicates that virtual drive is currently configured as pass through
     *  mode and it expects only edge1 to be online before it goes to activate
     *  state and become READY.
     */
    FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE  = 2,

    /*! Indicates that a proactive spare is swapped-in on second edge and virtual drive 
     * currently acts as a mirror object.
     */
    FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE     = 3,

    /*! Indicates that a proactive spare is swapped-in on first edge and virtual drive 
     * currently acts as a mirror object.
     */
    FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE     = 4

} fbe_virtual_drive_configuration_mode_t;

/*!********************************************************************* 
 * @enum fbe_virtual_drive_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the base config specific control codes. 
 * These are control codes specific to the base config, which only 
 * the base config object will accept. 
 *         
 **********************************************************************/
typedef enum
{
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_VIRTUAL_DRIVE),

    FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_SPARE_INFO,
    
    /*! Configure the virtual drive attributes such as the type of
     *  drive that is allowed to swap into the parent raid group etc.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_SET_CONFIGURATION,

    /*! Configuration service sends update config mode to update the 
     *  virtual drive configuration mode.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_UPDATE_CONFIGURATION,

    /*! Copy completed successfully - set checkpoint to end marker.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_COPY_COMPLETE_SET_CHECKPOINT_TO_END_MARKER,

    /*! Copy failed - set checkpoint to end marker.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_COPY_FAILED_SET_CHECKPOINT_TO_END_MARKER,

    /*! Usurper command to send the completion status for the job service 
     *  swap request.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_SWAP_REQUEST_COMPLETE,

    /*! Configure the downstream edge for instance remove the metadata 
     *  capacity etc.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_SET_EDGE_CONFIGURATION,

    /*! Usurper command to get the checkpoint 
    */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CHECKPOINT,

    /*! Usurper command to get the upstream object information.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_UPSTREAM_OBJECT_INFO,

    /*! Usurper command to virtual drive class to calculate the capacity 
     *  for the metadata.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_CALCULATE_CAPACITY,

    /*! Usurpur command to update the virtua
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_UPDATE_SPARE_CONFIG,

    /*! Get spare configuration. */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_GET_SPARE_CONFIG,

    /*! It is used to get the configuration of the virtual drive object.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CONFIGURATION,

    /*! Initiate a user copy request. */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_INITIATE_USER_COPY,

    /*! validate the swap-in request to validate whether we need to swap-in anymore. */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_SWAP_IN_VALIDATION,

    /*! validate the swap-in request to validate whether we need to swap-out anymore. */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_SWAP_OUT_VALIDATION,

    /*! It is used to get the virtual drive unused drive as spare flag. */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_UNUSED_DRIVE_AS_SPARE_FLAG,

    /*! It is used to set the virtual drive unused drive as spare flag. */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_SET_UNUSED_DRIVE_AS_SPARE_FLAG,

    /*! It is used to set the vd non paged to indicate we have swapped in a perm spare. */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_SET_NONPAGED_PERM_SPARE_BIT,

    /*! It is used to tell the virtual drive that the parent raid group has completed 
     *  any `needs rebuild' bits for the associated position.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_MARK_NEEDS_REBUILD_DONE,  

    /*! This is sent to the object to get the virtual drive debug flags.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_DEBUG_FLAGS,

    /*! This is sent to the object to change the virtual drive debug flags.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_SET_DEBUG_FLAGS,
     
    /*! This is sent to the class to get the default virtual drive debug flags
     *  for all newly created virtual drives.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_GET_DEBUG_FLAGS,

    /*! This is sent to the class to set the default virtual drive debug flags
     *  for all newly created raid groups.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_SET_DEBUG_FLAGS,

    /*! Usurpur command to Virtual Drive CLASS to set the init unused drive as hot spare flag to false. */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_SET_UNUSED_AS_SPARE_FLAG,

    /*! Get the virtual drive information including: 
     *      o Configuration mode
     *      o Original / source pvd object id
     *      o Spare / destination pvd object id
     *      o etc
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,

    /*! Usurper command that tells virtual drive that the job is complete.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_SEND_SWAP_COMMAND_COMPLETE,

    /*! Usurpur command to Virtual Drive to start encryption. */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_ENCRYPTION_START,

    /*! The swap request is going to rollback.  This usurper tells the virtual 
     *  drive to perform any cleanup so that the rollback can start.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_SWAP_REQUEST_ROLLBACK,

    /*! This is sent to the class to get the performance tier information.
     */
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_GET_PERFORMANCE_TIER,

    FBE_VIRTUAL_DRIVE_CONTROL_CODE_LAST
}
fbe_virtual_drive_control_code_t;


/* Edge index constants. */
enum edge_index_constants_e 
{
        FIRST_EDGE_INDEX = 0,
        SECOND_EDGE_INDEX = 1,
};

/*!*******************************************************************
 * @struct fbe_virtual_drive_initiate_user_copy_s
 *********************************************************************
 * @brief
 *  Code code: FBE_VIRTUAL_DRIVE_CONTROL_CODE_INITIATE_USER_COPY
 *  Informs virtual drive that a user copy request is initiated.
 *
 *********************************************************************/
typedef struct fbe_virtual_drive_initiate_user_copy_s
{
    fbe_spare_swap_command_t    command_type;       // swap in or swap out commands.
    fbe_edge_index_t            swap_edge_index;    // swap in edge index
    fbe_bool_t                  b_operation_confirmation; // FBE_TRUE (default) start he request thru the monitor context

}
fbe_virtual_drive_initiate_user_copy_t;

/*!*******************************************************************
 * @struct fbe_virtual_drive_unused_drive_as_spare_flag_s
 *********************************************************************
 * @brief
 *  It is used to get the value of unused drive as spare flagfrom
 *  virtual drive object.
 *
 *********************************************************************/
typedef struct fbe_virtual_drive_unused_drive_as_spare_flag_s
{
    /*Virtual drive unused drive as spare flag*/
    fbe_bool_t unused_drive_as_spare_flag;
}
fbe_virtual_drive_unused_drive_as_spare_flag_t;

/*!*******************************************************************
 * @enum fbe_virtual_drive_swap_status_t
 *********************************************************************
 * @brief
 *      Enumeration of swap-in and swap-out status.
 *
 *********************************************************************/
typedef enum fbe_virtual_drive_swap_status_e
{
    FBE_VIRTUAL_DRIVE_SWAP_STATUS_INVALID                  = 0, /*!< Status of operation is invalid */
    FBE_VIRTUAL_DRIVE_SWAP_STATUS_OK,                           /*!< Ok to proceed with swap request */
    FBE_VIRTUAL_DRIVE_SWAP_STATUS_INTERNAL_ERROR,               /*!< Unexpected error occurred. */
    FBE_VIRTUAL_DRIVE_SWAP_STATUS_PERMANENT_SPARE_NOT_REQUIRED, /*!< Drive came back.  Spare not required. */
    FBE_VIRTUAL_DRIVE_SWAP_STATUS_PROACTIVE_SPARE_NOT_REQUIRED, /*!< EOL is not set on selected edge */
    FBE_VIRTUAL_DRIVE_SWAP_STATUS_VIRTUAL_DRIVE_BROKEN,         /*!< Virtual drive must be in Ready/Hibernation state */
    FBE_VIRTUAL_DRIVE_SWAP_STATUS_VIRTUAL_DRIVE_DEGRADED,       /*!< Virtual drive cannot be degraded (source drive must be fully rebuilt).*/
    FBE_VIRTUAL_DRIVE_SWAP_STATUS_CONFIG_MODE_DOESNT_SUPPORT,   /*!< The current configuration mode doesn't support this request (i.e. already in mirror mode etc.)*/
    FBE_VIRTUAL_DRIVE_SWAP_STATUS_UPSTREAM_DENIED_PERMANENT_SPARE_REQUEST, /*!< The upstream raid group denied the request.*/
    FBE_VIRTUAL_DRIVE_SWAP_STATUS_UPSTREAM_DENIED_PROACTIVE_SPARE_REQUEST, /*!< The upstream raid group denied the request.*/
    FBE_VIRTUAL_DRIVE_SWAP_STATUS_UPSTREAM_DENIED_USER_COPY_REQUEST, /*!< The upstream raid group is degraded . */
    FBE_VIRTUAL_DRIVE_SWAP_STATUS_UNSUPPORTED_COMMAND,          /*!< Swap command not supported */

    FBE_VIRTUAL_DRIVE_SWAP_STATUS_LAST                          /*!< Number of defined status*/
} fbe_virtual_drive_swap_status_t;

/*!*******************************************************************
 * @struct fbe_virtual_drive_swap_in_validation_s
 *********************************************************************
 * @brief
 *  It is used by job service to determine whether vd is still in same
 * 	state where it needs to swap-in spare.
 *********************************************************************/
typedef struct fbe_virtual_drive_swap_in_validation_s
{
    fbe_spare_swap_command_t        command_type;    // swap in command type
    fbe_edge_index_t                swap_edge_index; // Edge index where we need to swap-in.
    fbe_virtual_drive_swap_status_t swap_status;     // status of swap request
    fbe_bool_t                      b_confirmation_enabled; // By default we wait for the virtual drive                
}
fbe_virtual_drive_swap_in_validation_t;

/*!*******************************************************************
 * @struct fbe_virtual_drive_swap_out_validation_s
 *********************************************************************
 * @brief
 *  It is used by job service to determine whether vd is still in same
 * 	state where it needs to swap-in spare.
 *********************************************************************/
typedef struct fbe_virtual_drive_swap_out_validation_s
{
    fbe_spare_swap_command_t        command_type;    // swap out
    fbe_edge_index_t                swap_edge_index; // Edge index where we need to swap-out.
    fbe_virtual_drive_swap_status_t swap_status;     // status of swap request                
    fbe_bool_t                      b_confirmation_enabled; // By default we wait for the virtual drive                
}
fbe_virtual_drive_swap_out_validation_t;

/* FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_CALCULATE_CAPACITY */
typedef struct fbe_virtual_drive_control_class_calculate_capacity_s {
	fbe_lba_t imported_capacity; /* PVD capacity */
	fbe_block_edge_geometry_t block_edge_geometry; /* Downstream edge geometry */
	fbe_lba_t exported_capacity; /* VD calculated capacity */
}fbe_virtual_drive_control_class_calculate_capacity_t;

/*!*******************************************************************
 * @struct fbe_system_spare_config_info_t
 *********************************************************************
 * @brief
 *  This structure is used to set spare related information in the system
 *
 * @ingroup fbe_api_power_save_interface
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_system_spare_config_info_s
{
    fbe_u64_t permanent_spare_trigger_time;
}fbe_system_spare_config_info_t;
#pragma pack()

/* FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_UPDATE_SPARE_CONFIG */
typedef struct fbe_virtual_drive_control_class_update_spare_config_s {
    fbe_system_spare_config_info_t     system_spare_config_info;
}fbe_virtual_drive_control_class_update_spare_config_t;

/* FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_GET_SPARE_CONFIG */
typedef struct fbe_virtual_drive_control_class_get_spare_config_s {
    fbe_system_spare_config_info_t     system_spare_config_info;
}fbe_virtual_drive_control_class_get_spare_config_t;

/*!*******************************************************************
 * @struct fbe_virtual_drive_get_info_t
 *********************************************************************
 * @brief
 *  Used to get information about the about the virtual drive subclass
 *  (use raid group usurper to get information about the virtual drive
 *   raid group superclass)
 * @note    FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO
 * 
 *********************************************************************/
typedef struct fbe_virtual_drive_get_info_s
{
    fbe_lba_t                               vd_checkpoint;          /* The rebuild checkpoint for the virtual drive */
    fbe_virtual_drive_configuration_mode_t  configuration_mode;     /* Current mode virtual drive is configured as */
    fbe_edge_index_t                        swap_edge_index;        /* Edge index that is / will be needed to swap out */
    fbe_object_id_t                         original_pvd_object_id; /* Object id of original / source pvd */
    fbe_object_id_t                         spare_pvd_object_id;    /* Object id of spare / destination pvd (if none then FBE_OBJECT_ID_INVALID) */
    fbe_bool_t                              b_user_copy_started;    /* FBE_TRUE - The virtual drive has started the user copy */
    fbe_bool_t                              b_request_in_progress;  /* Indicates if there is a job request in progress */
    fbe_bool_t                              b_is_swap_in_complete;  /* FBE_TRUE - the swap-in is complete */
    fbe_bool_t                              b_is_change_mode_complete; /* FBE_TRUE - is the configration mode change complete */
    fbe_bool_t                              b_is_copy_complete;     /* Indicates if the rebuild is complete or not. */
    fbe_bool_t                              b_is_swap_out_complete; /* FBE_TRUE - Swap-out is complete */
    fbe_bool_t                              b_copy_complete_in_progress; /* FBE_TRUE The virtual drive is processing a `copy complete' request */
    fbe_bool_t                              b_copy_failed_in_progress; /* FBE_TRUE The virtual drive is processing a `copy failed' request */

} fbe_virtual_drive_get_info_t;

/*! @enum fbe_virtual_drive_debug_flags_t 
 *  
 *  @brief Debug flags that allow run-time debug functions.
 *
 *  @note Typically these flags should only be set for unit or
 *        intergration testing since they could have SEVERE 
 *        side effects.
 */
typedef enum fbe_virtual_drive_debug_flags_e
{
    FBE_VIRTUAL_DRIVE_DEBUG_FLAG_NONE               = 0x00000000,

    /*! Enable raid group and raid library debug flags. 
     *  (Used to enable raid group and raid library flags if no other virtual
     *   drive debug flags are set).
     */
    FBE_VIRTUAL_DRIVE_DEBUG_FLAG_RAID_DEBUG_ENABLE  = 0x00000001,

    /*! Enable rebuild (copy) tracing.
     */
    FBE_VIRTUAL_DRIVE_DEBUG_FLAG_REBUILD_TRACING    = 0x00000002,

    /*! Enable proactive copy tracing
     */
    FBE_VIRTUAL_DRIVE_DEBUG_FLAG_PROACTIVE_TRACING  = 0x00000004,


    /*! Enable configuration change tracing
     */
    FBE_VIRTUAL_DRIVE_DEBUG_FLAG_CONFIG_TRACING     = 0x00000008,

    /*! Enable drive swap (in and out) tracing
     */
    FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING       = 0x00000010,

    /*! Enable tracing of quiesce and unquiesce conditions
     */
    FBE_VIRTUAL_DRIVE_DEBUG_FLAG_QUIESCE_TRACING    = 0x00000020,

    /*! Enable tracing of `rebuild logging'
     */
    FBE_VIRTUAL_DRIVE_DEBUG_FLAG_RL_TRACING         = 0x00000040,

    /*! Enable tracing of `mark needs rebuild'
     */
    FBE_VIRTUAL_DRIVE_DEBUG_FLAG_MARK_NR_TRACING    = 0x00000080,

    /*! Enable tracing of downstream health evaluation. 
     */
    FBE_VIRTUAL_DRIVE_DEBUG_FLAG_HEALTH_TRACING     = 0x00000100,

    /*! Enable copy specific tracing
     */
    FBE_VIRTUAL_DRIVE_DEBUG_FLAG_COPY_TRACING       = 0x00000200,

    /*! Enable extended media error handling tracing
     */
    FBE_VIRTUAL_DRIVE_DEBUG_FLAG_EMEH_TRACING       = 0x00000400,

    FBE_VIRTUAL_DRIVE_DEBUG_FLAG_LAST               = 0x00000800,

    FBE_VIRTUAL_DRIVE_DEBUG_FLAG_DEFAULT            = (FBE_VIRTUAL_DRIVE_DEBUG_FLAG_NONE),                 

} fbe_virtual_drive_debug_flags_t; 

/*!*******************************************************************
 * @def FBE_VIRTUAL_DRIVE_DEBUG_FLAG_STRINGS
 *********************************************************************
 * @brief Set of strings that correspond to the above flags.
 *
 *********************************************************************/
#define FBE_VIRTUAL_DRIVE_DEBUG_FLAG_STRINGS\
    "FBE_VIRTUAL_DRIVE_DEBUG_FLAG_RAID_DEBUG_ENABLE",\
    "FBE_VIRTUAL_DRIVE_DEBUG_FLAG_REBUILD_TRACING",\
    "FBE_VIRTUAL_DRIVE_DEBUG_FLAG_PROACTIVE_TRACING",\
    "FBE_VIRTUAL_DRIVE_DEBUG_FLAG_CONFIG_TRACING",\
    "FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING",\
    "FBE_VIRTUAL_DRIVE_DEBUG_FLAG_QUIESCE_TRACING",\
    "FBE_VIRTUAL_DRIVE_DEBUG_FLAG_RL_TRACING",\
    "FBE_VIRTUAL_DRIVE_DEBUG_FLAG_MARK_NR_TRACING",\
    "FBE_VIRTUAL_DRIVE_DEBUG_FLAG_HEALTH_TRACING",\
    "FBE_VIRTUAL_DRIVE_DEBUG_FLAG_COPY_TRACING",\
    "FBE_VIRTUAL_DRIVE_DEBUG_FLAG_EMEH_TRACING",\
    "FBE_VIRTUAL_DRIVE_DEBUG_FLAG_LAST"

/*!*******************************************************************
 * @struct fbe_virtual_drive_virtual_drive_debug_payload_t
 *********************************************************************
 *
 * @brief   This structure is used with the following control codes:
 *          o   FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_DEBUG_FLAGS
 *          o   FBE_VIRTUAL_DRIVE_CONTROL_CODE_SET_DEBUG_FLAGS
 *          o   FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_GET_DEBUG_FLAGS
 *          o   FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_SET_DEBUG_FLAGS
 *          Currently there is only (1) field in this payload but it could
 *          be expanded in the future.  
 *
 *********************************************************************/
typedef struct fbe_virtual_drive_debug_payload_s
{
    fbe_virtual_drive_debug_flags_t    virtual_drive_debug_flags; /*! virtual drive debug flags */
}
fbe_virtual_drive_debug_payload_t;


/*!**********************************************************************
 * @enum    fbe_virtual_drive_background_op_e
 *  
 * @brief Background operations supported by virtual drive object.
 * 
 * @note  Since the virtual drive is a leaf class of the raid group object
 *        we should use the same bit definitions for similar operations:
 *          o Raid group rebuild ==> Virtual drive copy
 *
*************************************************************************/
typedef enum fbe_virtual_drive_background_op_e
{
    FBE_VIRTUAL_DRIVE_BACKGROUND_OP_NONE                = 0x0000,
    FBE_VIRTUAL_DRIVE_BACKGROUND_OP_METADATA_REBUILD    = FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD,
    FBE_VIRTUAL_DRIVE_BACKGROUND_OP_COPY                = FBE_RAID_GROUP_BACKGROUND_OP_REBUILD,
    FBE_VIRTUAL_DRIVE_BACKGROUND_OP_ALL                 = (FBE_VIRTUAL_DRIVE_BACKGROUND_OP_METADATA_REBUILD | FBE_VIRTUAL_DRIVE_BACKGROUND_OP_COPY), /* bitmask of all operations */
}fbe_virtual_drive_background_op_t;

/*FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_SET_UNUSED_AS_SPARE_FLAG*/
typedef struct fbe_virtual_drive_control_class_set_unused_drive_as_spare_s{
	fbe_bool_t		enable; /*! by default is FBE_TRUE in the class level which meand HS swap in automatically from unsused drives */
}
fbe_virtual_drive_control_class_set_unused_drive_as_spare_t;

/*FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_ID_OF_RECONSTRCUTED_PVD*/
typedef struct fbe_virtual_drive_control_get_reconstructed_pvd_s{
	fbe_object_id_t		reconstructed_pvd_id; /*OUT*/
}fbe_virtual_drive_control_get_reconstructed_pvd_t;

/*FBE_VIRTUAL_DRIVE_CONTROL_CODE_SWAP_REQUEST_ROLLBACK*/
typedef struct fbe_virtual_drive_control_swap_request_rollback_s{
	fbe_object_id_t     orig_pvd_object_id; /*OUT*/
}fbe_virtual_drive_control_swap_request_rollback_t;

/*! @note The use nad definition may chnage. 
 *
 * FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_GET_PERFORMANCE_TIER
 */
typedef struct fbe_virtual_drive_control_get_performance_tier_s {
    fbe_u32_t                     drive_type_last;                       /*! Output with the maximum valid drive type +1 */                   
    fbe_performance_tier_number_t performance_tier[FBE_DRIVE_TYPE_LAST]; /*! Output with peformance tier for each drive type */
}fbe_virtual_drive_control_get_performance_tier_t;

#endif /* FBE_VIRTUAL_DRIVE_H */

/*******************************
 * end file fbe_virtual_drive.h
 ********************************/
