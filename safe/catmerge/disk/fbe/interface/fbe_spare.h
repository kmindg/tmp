#ifndef FBE_SPARE_H
#define FBE_SPARE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_spare.h
 ***************************************************************************
 *
 * @brief
 *  This file contains exported defination for the drive swap service library.
 *
 * @version
  *  8/26/2009:  Created. Dhaval Patel
 *
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_enclosure.h"

/******************************
 * LITERAL DEFINITIONS.
 ******************************/

#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
#define FBE_SPARE_PS_THRESHOLD_RESET_SECS 2
#define FBE_SPARE_PS_THRESHOLD_RESET_TENTHS RG_HS_THRESHOLD_RESET_SECS * 10
#else
#define FBE_SPARE_PS_THRESHOLD_RESET_SECS 30
#define FBE_SPARE_PS_THRESHOLD_RESET_TENTHS RG_HS_THRESHOLD_RESET_SECS * 60
#endif

/* Size of our window in the thresholding algorithm.
 */
#define FBE_SPARE_PS_THRESHOLD_WINDOW_SIZE      1024

/* Units to increment by on success according to the thresholding algorithm.
 * This means that the window tracks a total of 1024 / 8  = 128 successes.
 */
#define FBE_SPARE_PS_THRESHOLD_SUCCESS_UNITS    8

/* Units to decrement by on failure according to the thresholding algorithm.
 * Currently we fail on the very first failure!!
 */
#define FBE_SPARE_PS_THRESHOLD_FAILURE_UNITS    (FBE_SPARE_PS_THRESHOLD_WINDOW_SIZE / 1)

/* The maximum health for the drive is the window size - 1.
 */
#define FBE_SPARE_PS_MAX_HEALTH                 (FBE_SPARE_PS_THRESHOLD_WINDOW_SIZE - 1)

/* This is the health reset update increment value.  When we have
 * exceeded this reset seconds we improve the health by this amount.
 */
#define FBE_SPARE_PS_RESET_INCREMENT            1

/* This is the permanent spare swap-in default trigger time. 
 */
#define FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME  300 /*5 minutes*/

/*************************
 *   FUNCTION PROTOTYPES
 *************************/

/***************************
 *   STRUCTURE DEFINITIONS
 ***************************/
 /*!****************************************************************************
 *    
 * @struct  fbe_spare_proactive_health_t
 *  
 * @brief 
 *   This structure holds the health information for a proactive spare.
 ******************************************************************************/
typedef fbe_u32_t   fbe_spare_proactive_health_t;

/*!*******************************************************************
 * @enum fbe_spare_swap_command_t
 *********************************************************************
 * @brief
 *  It defines swap command type (swap-in, swap-out).
 *
 *********************************************************************/
typedef enum fbe_spare_swap_command_e
{
    FBE_SPARE_SWAP_INVALID_COMMAND              = 0,    /*!< Invalid spare command */
    FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND   = 1,    /*!< Locate a suitable spare and swap it into the failed position of the virtual drive. */
    FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND   = 2,    /*!< Initiate a proactive copy command by selecting a suitable spare, swapping in the drive and changing to mirror mode. */
    FBE_SPARE_INITIATE_USER_COPY_COMMAND        = 3,    /*!< Initiate a user copy to spare command by selecting a suitable spare, swapping in the drive and changing to mirror mode. */
    FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND     = 4,    /*!< Initiate a user copy to selected drive command by swapping in the selected drive and changing to mirror mode. */
    FBE_SPARE_COMPLETE_COPY_COMMAND             = 5,    /*!< Complete the copy request by changing to pass-thru mode and then swapping out the source drive. */
    FBE_SPARE_ABORT_COPY_COMMAND                = 6,    /*!< Abort the copy request by changing to pass-thru mode and then swapping out the failed drive. */

    FBE_SPARE_SWAP_COMMAND_LAST                         /* Count of how many command there are */
} fbe_spare_swap_command_t;

/*!*******************************************************************
 * @enum fbe_spare_internal_error_t
 *********************************************************************
 * @brief
 *  There are cases where an error needs to be reported but there is
 *  no job associated with the failure.  This enum defines the possible
 *  `internal' error codes.
 *
 *********************************************************************/
typedef enum fbe_spare_internal_error_s
{
    /*! @note Do not change these values.  If needed add new entries at the end. 
     */
    FBE_SPARE_INTERNAL_ERROR_INVALID                            = 0,    /*!< Swap status was never populated. */
    FBE_SPARE_INTERNAL_ERROR_NONE                               = 1,    /*!< The swap request completed successfully. */
    FBE_SPARE_INTERNAL_ERROR_NO_SPARE_AVAILABLE                 = 2,    /*!< There are no spare drives in the system. */
    FBE_SPARE_INTERNAL_ERROR_NO_SUITABLE_SPARE_AVAILABLE        = 3,    /*!< There are no suitable spares. */
    FBE_SPARE_INTERNAL_ERROR_UNEXPECTED_JOB_STATUS              = 4,    /*!< The job status supplied was not expected. */
	FBE_SPARE_INTERNAL_ERROR_SWAP_IN_VALIDATION_FAIL            = 5,    /*!< Error during swap-in validation. */
    FBE_SPARE_INTERNAL_ERROR_INVALID_SWAP_COMMAND_TYPE          = 6,    /*!< An invalid/unsupported swap command was used. */
    FBE_SPARE_INTERNAL_ERROR_INVALID_VIRTUAL_DRIVE_OBJECT_ID    = 7,    /*!< The virtual drive object id is invalid. */
    FBE_SPARE_INTERNAL_ERROR_INVALID_EDGE_INDEX                 = 8,    /*!< The edge index supplied is not valid. */
    FBE_SPARE_INTERNAL_ERROR_INVALID_VIRTUAL_DRIVE_CONFIG_MODE  = 9,    /*!< The virtual drive configuration mode is invalid. */
    FBE_SPARE_INTERNAL_ERROR_INVALID_PROVISION_DRIVE_CONFIG_MODE = 10,  /*!< The provision drive configuration mode is invalid. */
    FBE_SPARE_INTERNAL_ERROR_INVALID_SPARE_OBJECT_ID            = 11,   /*!< Invalid object id of spare. */
    FBE_SPARE_INTERNAL_ERROR_CREATE_EDGE_FAILED                 = 12,   /*!< Create edge failed. */
    FBE_SPARE_INTERNAL_ERROR_DESTROY_EDGE_FAILED                = 13,   /*!< Destroy edge failed. */
    FBE_SPARE_INTERNAL_ERROR_UNSUPPORTED_EVENT_CODE             = 14,   /*!< The event code supplied is not supported. */
    FBE_SPARE_INTERNAL_ERROR_RAID_GROUP_DENIED                  = 15,   /*!< The parent raid group denied the spare request.*/
} fbe_spare_internal_error_t;

/*!*******************************************************************
 * @enum fbe_spare_copy_originator_e
 *********************************************************************
 * @brief
 *  It defines originator of the proactive copy request.
 *
 *********************************************************************/
typedef enum fbe_spare_copy_originator_e
{
   FBE_SPARE_UNKNOWN_TRIGGER_COPY_REQUEST  = 0,     // Unknown initiator for the copy request.
   FBE_SPARE_NAVI_TRIGGER_COPY_REQUEST     = 1,     // NAVI requested a drive copy operation.
   FBE_SPARE_FBECLI_TRIGGER_COPY_REQUEST   = 2,     // FCLI requested a drive copy operation.
   FBE_SPARE_EOL_TRIGGER_COPY_REQUEST      = 3,     // EOL bit set on edge trigger copy request.
}
fbe_spare_copy_originator_t;

/*!****************************************************************************
 * @enum fbe_performance_tier_e
 ******************************************************************************
 * @brief
 *  Enumeration the performance tier table.
 ******************************************************************************/
typedef enum fbe_performance_tier_number_e {
    FBE_PERFORMANCE_TIER_INVALID = -1,
    FBE_PERFORMANCE_TIER_ZERO,
    FBE_PERFORMANCE_TIER_ONE,
    FBE_PERFORMANCE_TIER_TWO,
    FBE_PERFORMANCE_TIER_THREE,
    FBE_PERFORMANCE_TIER_FOUR,
    FBE_PERFORMANCE_TIER_FIVE,
    FBE_PERFORMANCE_TIER_SIX,
    FBE_PERFORMANCE_TIER_SEVEN,
    FBE_PERFORMANCE_TIER_EIGHT,

    FBE_PERFORMANCE_TIER_LAST
}fbe_performance_tier_number_t;

/*!****************************************************************************
 * @struct fbe_performance_tier_drive_information_t
 ******************************************************************************
 * @brief
 *  It defines a performance tier table, currently it has only tier number and
 *  the drive types that are supported.
 ******************************************************************************/
typedef struct fbe_performance_tier_drive_information_s
{
    fbe_performance_tier_number_t   tier_number;
    fbe_drive_type_t                supported_drive_type_array[FBE_DRIVE_TYPE_LAST];
}
fbe_performance_tier_drive_information_t;

/*!*******************************************************************
 * @struct fbe_spare_drive_info_t
 *********************************************************************
 * @brief
 *  It defines spare drive information and it is used to select the 
 *  best suitable spare for the drive.
 *
 *********************************************************************/
typedef struct fbe_spare_drive_info_s 
{
    fbe_u32_t           port_number; 
    fbe_u32_t           enclosure_number; 
    fbe_u32_t           slot_number;
    fbe_drive_type_t    drive_type;
    fbe_lba_t           capacity_required;      /*!< The minimum configured capacity required. */
    fbe_lba_t           configured_capacity;    /*!< The configured capacity of te orignal provisioned drive. */
    fbe_lba_t           exported_offset;        /*!< Default offset for the start of exported extent. */
    fbe_provision_drive_configured_physical_block_size_t    configured_block_size;
    fbe_u32_t           pool_id;
    fbe_bool_t          b_is_system_drive;
    fbe_bool_t          b_is_end_of_life;
    fbe_bool_t          b_is_slf;
} fbe_spare_drive_info_t;


/*!****************************************************************************
 * fbe_spare_initialize_spare_drive_info()
 ******************************************************************************
 * @brief 
 *   It initializes the drive spare information with default values.
 *
 * @param spare_drive_info_p   - spare drive information pointer.
 ******************************************************************************/
static __forceinline void fbe_spare_initialize_spare_drive_info(fbe_spare_drive_info_t *spare_drive_info_p)
{
    spare_drive_info_p->drive_type = FBE_DRIVE_TYPE_INVALID;
    spare_drive_info_p->capacity_required = FBE_LBA_INVALID;
    spare_drive_info_p->configured_capacity = FBE_LBA_INVALID; /* This must be the max value */
    spare_drive_info_p->configured_block_size = FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520;
    spare_drive_info_p->exported_offset = FBE_LBA_INVALID;
    spare_drive_info_p->port_number = FBE_PORT_NUMBER_INVALID;
    spare_drive_info_p->enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID;
    spare_drive_info_p->slot_number = FBE_SLOT_NUMBER_INVALID;
    spare_drive_info_p->pool_id = FBE_POOL_ID_INVALID;
    spare_drive_info_p->b_is_system_drive = FBE_FALSE;
    spare_drive_info_p->b_is_end_of_life = FBE_FALSE;
    spare_drive_info_p->b_is_slf = FBE_FALSE;
    return;
}
/******************************************************************************
 * end fbe_spare_initialize_spare_drive_info()
 ******************************************************************************/

/*!*******************************************************************
 * @enum fbe_spare_status_t
 *********************************************************************
 * @brief
 *  It defines a list of object-id which is returned from topology
 *  service.
 *  Note: This structure defination is exactly same as topology spare
 *  drive pool (fbe_topology_control_get_spare_drive_pool_t).
 * 
 *  Any changes to topology spare drive pool data structure needs to
 *  be reflected here.
 *********************************************************************/
typedef struct fbe_spare_drive_pool_s
{
   fbe_object_id_t spare_object_list[FBE_MAX_SPARE_OBJECTS]; /* OUT */ 
}
fbe_spare_drive_pool_t;


/*!****************************************************************************
 * fbe_spare_initialize_spare_drive_pool()
 ******************************************************************************
 * @brief 
 *   It initializes the drive spare pool.
 *
 * @param spare_drive_pool_p    - spare drive pool pointer.
 ******************************************************************************/
static __forceinline void
fbe_spare_initialize_spare_drive_pool(fbe_spare_drive_pool_t *spare_drive_pool_p)
{
    fbe_u32_t index = 0;

    /* Initialize the spare drive pool as invalid object ids.
     */
    for (index = 0; index < FBE_MAX_SPARE_OBJECTS; index++)
    {
        spare_drive_pool_p->spare_object_list[index] = FBE_OBJECT_ID_INVALID;
    }
}

/*!*******************************************************************
 * @struct fbe_spare_get_upsteam_object_info_t
 *********************************************************************
 * @brief
 *  This usurper is used to get information about the raid group that
 *  the virtual drive belongs to.  This information is required to
 *  determine if the
 * 
 * @note    FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_UPSTREAM_OBJECT_INFO
 *********************************************************************/
typedef struct fbe_spare_get_upsteam_object_info_s
{
    fbe_edge_index_t    upstream_edge_to_select;        /*!< IN Defines which virtual drive upstream object to select. */
    fbe_object_id_t     upstream_object_id;             /*!< OUT the object id of the upstream objected selected. */
    fbe_bool_t          b_is_upstream_object_in_use;    /*!< OUT Determines if the upstream object is in use or not. */
    fbe_bool_t          b_is_upstream_object_degraded;  /*!< OUT Determines if the upstream object is degraded or not. */
    fbe_bool_t          b_is_upstream_object_redundant; /*!< OUT Determines if the upstream object is redundant or not. */
    fbe_bool_t          b_is_upstream_rg_ready;         /*!< OUT Determines if the parent raid group is Ready or not. */
    fbe_u32_t           num_copy_in_progress;           /*!< OUT How copy operations are already in progress.*/
    fbe_bool_t          b_is_encryption_in_progress;    /*!< OUT Determines if the upstream object is in the middle of encryption or not. */   
} fbe_spare_get_upsteam_object_info_t;


/* Externally accessible APIs
 */

/*************************** 
 * fbe_spare_utils.c
 ***************************/
fbe_status_t fbe_spare_lib_utils_is_drive_type_valid_for_performance_tier(fbe_performance_tier_number_t performance_tier_number,
                                                                          fbe_drive_type_t curr_spare_drive_type,
                                                                          fbe_bool_t *b_is_drive_type_valid_for_performance_tier);

/*************************** 
 * fbe_spare_selection.c
 ***************************/
fbe_status_t fbe_spare_lib_selection_get_drive_performance_tier_number(fbe_drive_type_t desired_spare_drive_type, 
                                                                       fbe_performance_tier_number_t *performance_tier_number);
fbe_status_t fbe_spare_lib_selection_get_drive_performance_tier_group(fbe_drive_type_t  desired_spare_drive_type, 
                                                                      fbe_performance_tier_number_t *performance_tier_group);


/*!***************************************************************************
 * @typedef fbe_spare_selection_priority_bitmask_t
 *****************************************************************************
 * @brief   Defines a bitmask for all possible spare selection priorities.
 * 
 *****************************************************************************/
typedef fbe_u32_t   fbe_spare_selection_priority_bitmask_t;

/* We need to set the bit for each of the rules. Since Rule 0 is not really valid we dont set that bit
 * e.g. If there are 6 rules, then it will be 0x7E(6 Bits are set) */
#define FBE_SPARE_SELECTION_PRIORITY_VALID_MASK     ((1 << (FBE_SPARE_SELECTION_PRIORITY_NUM_OF_RULES+ 1)) - 2)   /*! Bitmask of legal values.*/

/*!****************************************************************************
 * @struct fbe_spare_selection_info_s
 ******************************************************************************
 * @brief
 *  It defines a spare selection information, it combines object-id of the spare
 *  selection and spare drive info to make association between hot spare object-id and
 *  associated spare drive information.
 ******************************************************************************/
typedef struct fbe_spare_selection_info_s
{
    /*! PVD object-id of the proposed spare drive. 
     */
    fbe_object_id_t                         spare_object_id;

    /*! Spare selection priority bitmask.
     */
    fbe_spare_selection_priority_bitmask_t  spare_selection_bitmask;
              
    /*! Spare drive information of the hot spare drive. 
     */
    fbe_spare_drive_info_t                  spare_drive_info;
} fbe_spare_selection_info_t;

void fbe_spare_initialize_spare_selection_drive_info(fbe_spare_selection_info_t *spare_selection_drive_info_p);

fbe_status_t fbe_spare_lib_selection_apply_selection_algorithm(fbe_spare_selection_info_t *desired_spare_drive_info_p,
                                                                      fbe_spare_selection_info_t *curr_hot_spare_info_p,
                                                                      fbe_spare_selection_info_t *selected_hot_spare_info_p);

#endif /* FBE_SPARE_H */
