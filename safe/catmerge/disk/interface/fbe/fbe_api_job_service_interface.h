#ifndef FBE_API_JOB_SERVICE_INTERFACE_H
#define FBE_API_JOB_SERVICE_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_job_service_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_api_job_service_interface.
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 * @version
 *   11/24/2009:  Created. Bo Gao
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_job_service.h"
#include "fbe/fbe_api_common.h"
#include "fbe_database.h"

FBE_API_CPP_EXPORT_START

//----------------------------------------------------------------
// Define the top level group for the FBE Storage Extent Package APIs
//----------------------------------------------------------------
/*! @defgroup fbe_api_storage_extent_package_class FBE Storage Extent Package APIs
 *  @brief 
 *    This is the set of definitions for FBE Storage Extent Package APIs.
 *
 *  @ingroup fbe_api_storage_extent_package_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API Job Service Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_job_service_interface_usurper_interface FBE API Job Service Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Job Service class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

/*!*******************************************************************
 * @struct fbe_api_job_service_lun_create_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_LUN_CREATE.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_lun_create
 *********************************************************************/
typedef struct fbe_api_job_service_lun_create_s {
    /* for validation */
    fbe_raid_group_type_t   raid_type;      /*!< RAID Type */
    fbe_raid_group_number_t raid_group_id;  /*!< RG ID */

    /* this following are the user specified parameters for LUN*/
    fbe_assigned_wwid_t      world_wide_name;      /*! World-Wide Name associated with the LUN */
    fbe_user_defined_lun_name_t user_defined_name;    /*! User-Defined Name for the LUN */
    fbe_lun_number_t        lun_number;        /*!< LU Number */
    fbe_lba_t               capacity;          /*!< LU Capacity */
    fbe_block_transport_placement_t placement; /*!< Tranport Placement */
    fbe_lba_t                       addroffset; /*!< Address offset; used for NDB */
    fbe_bool_t                      ndb_b;      /*!< NDB bool */
    fbe_bool_t                      noinitialverify_b; /*!< NOINITIALVERIFY bool */ 
    //void  *context;     /*!< indicates specific caller context, pass thru for job service */
    fbe_u64_t   job_number; /*!< job id */

    /* add additional parameters here */
    fbe_time_t  bind_time;
    fbe_bool_t  user_private;
    fbe_bool_t               wait_ready;                /*!< indicate if the user want to wait for ready. */
    fbe_u32_t                ready_timeout_msec;        /*!< indicate how long the user want to wait for ready, otherwise timeout. */

    /*Used for creating a system LUN*/
    fbe_bool_t is_system_lun;
    fbe_object_id_t lun_id;

    /* Whether this lun needs to be exported by blkshim */
    fbe_bool_t export_lun_b;
}fbe_api_job_service_lun_create_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_lun_destroy_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_LUN_DESTROY.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_lun_destroy
 *********************************************************************/
typedef struct fbe_api_job_service_lun_destroy_s {
    fbe_u32_t        lun_number;   /*!< only need lun number to identify the lun to be destroyed*/  
    fbe_u64_t        job_number;   /*!< job id */
    fbe_bool_t       allow_destroy_broken_lun;
    //void             *context;     /*!< indicates specific caller context, pass thru for job service */   
    fbe_bool_t       wait_destroy;                /*!< indicate if the user want to wait for destroy. */
    fbe_u32_t        destroy_timeout_msec;        /*!< indicate how long the user want to wait for destroy, otherwise timeout. */ 
}fbe_api_job_service_lun_destroy_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_lun_update_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_LUN_UPDATE.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_lun_update
 *********************************************************************/
typedef struct fbe_api_job_service_lun_update_s {
    fbe_object_id_t     object_id;/*!< which object is being changed */
    fbe_u64_t           job_number;   /*!< job id */
    fbe_assigned_wwid_t world_wide_name;
    fbe_u32_t                   attributes;
    fbe_user_defined_lun_name_t user_defined_name;
    fbe_lun_update_type_t       update_type;
    //void                *context;     /*!< indicates specific caller context, pass thru for job service */    
}fbe_api_job_service_lun_update_t;


/*!*******************************************************************
 * @struct fbe_api_job_service_raid_group_create_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_RAID_GROUP_CREATE.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_raid_group_create
 *********************************************************************/
typedef struct fbe_api_job_service_raid_group_create_s {

    fbe_raid_group_number_t  raid_group_id;         /*!< User provided id */
    fbe_u32_t                drive_count;           /*!< number of drives that make up the Raid Group */
    fbe_u64_t                power_saving_idle_time_in_seconds; /*!< How long RG have to wait without IO to start saving power */
    fbe_u64_t                max_raid_latency_time_is_sec;  /*!< What's the longest time it should take the RG/drives to be ready after saving power */
    fbe_raid_group_type_t    raid_type;          /*!< indicates Raid Group type, i.e Raid 5, Raid 0, etc */
    //fbe_bool_t               explicit_removal;      /*!< Indicates to remove the Raid Group after removing the last lun in the Raid Group */
    fbe_job_service_tri_state_t is_private;            /*!< defines raid group as private */
    fbe_bool_t               b_bandwidth;           /*!< Determines if the user selected bandwidth or iops */
    fbe_lba_t                capacity;              /*!< indicates specific capacity settings for Raid group */
    fbe_u64_t                job_number;            /*!< job id */
    //void                     *context;              /*!< indicates specific caller context, pass thru for job service */    
    fbe_job_service_bes_t    disk_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH]; /*!< set of disks for raid group creation */
    fbe_bool_t               user_private;				/*!< indicate it is a user private RG*/
    fbe_bool_t               wait_ready;                /*!< indicate if the user want to wait for ready. */
    fbe_u32_t                ready_timeout_msec;        /*!< indicate how long the user want to wait for ready, otherwise timeout. */
    
    fbe_bool_t               is_system_rg;          /* Define raid group is a system RG*/
    fbe_object_id_t           rg_id;                /* If we create a system Raid group, we need to define the rg object id*/              
}
fbe_api_job_service_raid_group_create_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_raid_group_destroy_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_RAID_GROUP_DESTROY.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_raid_group_destroy
 *********************************************************************/
typedef struct fbe_api_job_service_raid_group_destroy_s 
{
    fbe_raid_group_number_t raid_group_id; /*!< User provided id */
    fbe_u64_t           job_number;    /*!< job id */
    fbe_bool_t          allow_destroy_broken_rg; /*if allow to destory when system 3-way mirror is broken */
    //void                *context;      /*!< indicates specific caller context, pass thru for job service */
    fbe_bool_t          wait_destroy;                /*!< indicate if the user want to wait for destroy. */
    fbe_u32_t           destroy_timeout_msec;        /*!< indicate how long the user want to wait for destroy, otherwise timeout. */
} fbe_api_job_service_raid_group_destroy_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_change_system_power_save_info_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_POWER_SAVING_INFO.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_change_system_power_save_info
 *********************************************************************/
typedef struct fbe_api_job_service_change_system_power_save_info_s 
{
    fbe_system_power_saving_info_t  system_power_save_info;/*!< user provided information to change */
    fbe_u64_t           job_number;    /*!< job id */
} fbe_api_job_service_change_system_power_save_info_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_change_rg_info_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_UPDAGE_RAID_GROUP.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_change_rg_info
 *********************************************************************/
typedef struct fbe_api_job_service_change_rg_info_s 
{
    fbe_update_raid_type_t update_type;/*!< what is it we are changing */
    fbe_u64_t   power_save_idle_time_in_sec;/*!< how long to wait before power save */
    fbe_bool_t  power_save_enabled;/*!< is power save enabled or disabled */
    fbe_object_id_t object_id;/*!< which object is being changed */
    fbe_lba_t	new_size;/*used for expansion*/
    fbe_lba_t	new_edge_size;/*used for expansion*/
} fbe_api_job_service_change_rg_info_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_drive_swap_in_request_s
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_DRIVE_SWAP_REQUEST.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_drive_swap_request
 *********************************************************************/
typedef struct fbe_api_job_service_drive_swap_in_request_s
{
    fbe_object_id_t     vd_object_id;       /*!< VD object id for the drive which fails. */
    fbe_object_id_t     original_object_id; /*!< PVD object id for `source' drive. */
    fbe_object_id_t     spare_object_id;    /*!< PVD object-id of the user specified spare if any.*/
    fbe_u64_t           job_number;         /*!< indicates the job number. */
    fbe_spare_swap_command_t command_type;  /*!< swap in or swap out commands. */
} fbe_api_job_service_drive_swap_in_request_t;


/*!*******************************************************************
 * @struct fbe_api_job_service_update_provision_drive_config_type_s
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE_CONFIG_TYPE.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_update_provision_drive_config_type
 *********************************************************************/
typedef struct fbe_api_job_service_update_provision_drive_config_type_s
{
    fbe_object_id_t                         pvd_object_id;      /*<!<PVD object id for which update the config type. */
    fbe_provision_drive_config_type_t       config_type;        /*<!<Config type of the provision drive object. */
    fbe_u64_t                               job_number;         /*<!<indicates the job number. */
    fbe_u8_t                                opaque_data[FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX]; /* opaque_data for the spare stuff*/
} fbe_api_job_service_update_provision_drive_config_type_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_update_pvd_sniff_verify_s
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_UPDATE_PVD_SNIFF_VERIFY_STATUS.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_update_pvd_sniff_verify_status
 *********************************************************************/
typedef struct fbe_api_job_service_update_pvd_sniff_verify_status_s
{
    fbe_object_id_t    pvd_object_id;        /*<!<PVD object id for which update the config type. */
    fbe_bool_t         sniff_verify_state;  /*<!<sniff verify enable flag */ 
    fbe_u64_t          job_number;           /*<!<indicates the job number. */
} fbe_api_job_service_update_pvd_sniff_verify_status_t;


/*!*******************************************************************
 * @struct fbe_api_job_service_update_pvd_pool_id_s
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_UPDATE_PVD_POOL_ID.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_update_pvd_sniff_verify_status
 *********************************************************************/
typedef struct fbe_api_job_service_update_pvd_pool_id_s
{
    fbe_u32_t          pvd_object_id;        /*<!<PVD object id for which update the config type. */
    fbe_u32_t          pvd_pool_id;          /*<!<PVD Pool id */ 
    fbe_u64_t          job_number;           /*<!<indicates the job number. */
} fbe_api_job_service_update_pvd_pool_id_t;


/*!*******************************************************************
 * @struct fbe_api_job_service_update_permanent_spare_timer_s
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_UPDATE_PERMANENT_SPARE_TIMER.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_update_permanent_spare_timer
 *********************************************************************/
typedef struct fbe_api_job_service_update_permanent_spare_timer_s 
{
    fbe_u64_t                       permanent_spare_trigger_timer;/*!< user provided information to change */
    fbe_u64_t                       job_number;    /*!< job id */
} fbe_api_job_service_update_permanent_spare_timer_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_control_system_bg_service_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_control_system_bg_service
 *********************************************************************/
typedef struct fbe_api_job_service_control_system_bg_service_s 
{
    fbe_base_config_control_system_bg_service_t system_bg_service;/*!< user provided information to change*/
    fbe_u64_t           job_number;    /*!< job id */
} fbe_api_job_service_control_system_bg_service_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_set_system_time_threshold_info_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_SET_SYSTEM_TIME_THRESHOLD_INFO.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_set_system_time_threshold_info
 *********************************************************************/
typedef struct fbe_api_job_service_set_system_time_threshold_info_s 
{
    fbe_system_time_threshold_info_t  system_time_threshold_info;/*!< user provided information to set */
    fbe_u64_t           job_number;    /*!< job id */
} fbe_api_job_service_set_system_time_threshold_info_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_update_multi_pvds_pool_id_s
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_UPDATE_MULTI_PVDS_POOL_ID.
 *  
 * @ingroup 
 * @ingroup 
 *********************************************************************/
typedef struct fbe_api_job_service_update_multi_pvds_pool_id_s
{
    update_pvd_pool_id_list_t pvd_pool_data_list;  /*<!pvd pool id list. */
    fbe_u64_t          job_number;           /*<!<indicates the job number. */
} fbe_api_job_service_update_multi_pvds_pool_id_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_change_system_encryption_info_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_ENCRYPTION_INFO.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_change_system_encryption_info
 *********************************************************************/
typedef struct fbe_api_job_service_change_system_encryption_info_s 
{
    fbe_system_encryption_info_t  system_encryption_info;/*!< user provided information to change */
    fbe_u64_t           job_number;    /*!< job id */
} fbe_api_job_service_change_system_encryption_info_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_update_encryption_mode_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_UPDATE_ENCRYPTION_MODE.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_update_encryption_mode
 *********************************************************************/
typedef struct fbe_api_job_service_update_encryption_mode_s 
{
    fbe_object_id_t     object_id;
    fbe_base_config_encryption_mode_t  encryption_mode;/*!< user provided information to change */
    fbe_u64_t           job_number;    /*!< job id */
} fbe_api_job_service_update_encryption_mode_t;

#define FBE_API_SG_LIST_SIZE        2

typedef struct fbe_api_job_service_process_encryption_keys_s
{
    fbe_u64_t                   job_number;     /*!< job id */
    fbe_job_service_encryption_key_operation_t operation;
    fbe_sg_element_t           *keys_sg_list;   /*!< sg list for key table */
}fbe_api_job_service_process_encryption_keys_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_validate_database_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_VALIDATE_DATABASE.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_validate_database
 *********************************************************************/
typedef struct fbe_api_job_service_validate_database_s
{
    fbe_database_validate_request_type_t    validate_caller;
    fbe_database_validate_failure_action_t  failure_action;
    fbe_u64_t                               job_number;    /*!< job id */
} fbe_api_job_service_validate_database_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_create_extent_pool_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 *********************************************************************/
typedef struct fbe_api_job_service_create_extent_pool_s 
{
    fbe_u32_t           pool_id;
    fbe_u32_t           drive_count;
    fbe_drive_type_t    drive_type;
    fbe_u64_t           job_number;
} fbe_api_job_service_create_extent_pool_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_create_ext_pool_lun_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL_LUN.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 *********************************************************************/
typedef struct fbe_api_job_service_create_ext_pool_lun_s 
{
    fbe_u32_t           lun_id;
    fbe_u32_t           pool_id;
    fbe_lba_t           capacity;
    fbe_assigned_wwid_t world_wide_name;
    fbe_user_defined_lun_name_t user_defined_name;    /*! User-Defined Name for the LUN */
    fbe_u64_t           job_number;
    fbe_object_id_t     lun_object_id;
} fbe_api_job_service_create_ext_pool_lun_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_destroy_extent_pool_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 *********************************************************************/
typedef struct fbe_api_job_service_destroy_extent_pool_s 
{
    fbe_u32_t           pool_id;
    fbe_u64_t           job_number;
} fbe_api_job_service_destroy_extent_pool_t;

/*!*******************************************************************
 * @struct fbe_api_job_service_destroy_ext_pool_lun_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL_LUN.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 *********************************************************************/
typedef struct fbe_api_job_service_destroy_ext_pool_lun_s 
{
    fbe_u32_t           lun_id;
    fbe_u32_t           pool_id;
    fbe_u64_t           job_number;
} fbe_api_job_service_destroy_ext_pool_lun_t;


/*!*******************************************************************
 * @struct fbe_api_job_service_update_pvd_block_size_s
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_job_service_interface
 * @ingroup fbe_api_job_service_update_pvd_block_size
 *********************************************************************/
typedef struct fbe_api_job_service_update_pvd_block_size_s 
{
    fbe_object_id_t     pvd_id;
    fbe_provision_drive_configured_physical_block_size_t    block_size;    /*!< user provided information to change */
    fbe_u64_t           job_number;    /*!< job id */
} fbe_api_job_service_update_pvd_block_size_t;

/*! @} */ /* end of group fbe_api_job_service_interface_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Job Service Interface.  
// This is where all the function prototypes for the FBE API Job Service.
//----------------------------------------------------------------
/*! @defgroup fbe_api_job_service_interface FBE API Job Service Interface
 *  @brief 
 *    This is the set of FBE API Job Service. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_job_service_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL 
fbe_api_job_service_lun_create(fbe_api_job_service_lun_create_t *in_fbe_lun_create_req_p);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_lun_destroy(fbe_api_job_service_lun_destroy_t *in_fbe_lun_destroy_req_p);

fbe_status_t FBE_API_CALL fbe_api_job_service_lun_update(
             fbe_api_job_service_lun_update_t *in_fbe_lun_update_req_p);


fbe_status_t FBE_API_CALL 
fbe_api_job_service_raid_group_create(fbe_api_job_service_raid_group_create_t *in_fbe_raid_group_create_req_p);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_raid_group_destroy(fbe_api_job_service_raid_group_destroy_t *in_fbe_raid_group_destroy_req_p);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_process_command(fbe_job_queue_element_t    *in_fbe_job_queue_element_p,
                                    fbe_job_service_control_t  in_control_code,
                                    fbe_packet_attr_t          in_attribute);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_add_element_to_queue_test(fbe_job_queue_element_t *fbe_job_queue_element_p,
                                             fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL 
fbe_api_job_service_set_system_power_save_info(fbe_api_job_service_change_system_power_save_info_t *in_change_power_save_info);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_change_rg_info(fbe_api_job_service_change_rg_info_t *in_change_rg_info, fbe_u64_t *job_number);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_drive_swap_in_request(fbe_api_job_service_drive_swap_in_request_t * in_drive_swap_request_p);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_update_provision_drive_config_type(fbe_api_job_service_update_provision_drive_config_type_t * in_update_pvd_config_p);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_update_pvd_sniff_verify(fbe_api_job_service_update_pvd_sniff_verify_status_t * in_update_pvd_sniff_verify_stat_p);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_update_pvd_pool_id(fbe_api_job_service_update_pvd_pool_id_t *in_update_pvd_pool_id_p);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_update_provision_drive_block_size(fbe_object_id_t in_pvd_object_id,  fbe_provision_drive_configured_physical_block_size_t in_block_size);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_set_permanent_spare_trigger_timer(fbe_api_job_service_update_permanent_spare_timer_t *in_update_spare_timer_p);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_set_pvd_sniff_verify( fbe_object_id_t in_pvd_object_id, fbe_status_t* job_status,
                                          fbe_job_service_error_type_t *js_error_type,
                                          fbe_bool_t sniff_verify_state );

fbe_status_t FBE_API_CALL 
fbe_api_job_service_control_system_bg_service(fbe_api_job_service_control_system_bg_service_t *in_control_system_bg_service);


fbe_status_t FBE_API_CALL 
fbe_api_job_service_destroy_system_pvd( fbe_object_id_t in_pvd_object_id,fbe_status_t* job_status);
fbe_status_t FBE_API_CALL 
fbe_api_job_service_create_system_pvd( fbe_object_id_t in_pvd_object_id, fbe_status_t* job_status );
fbe_status_t FBE_API_CALL 
fbe_api_job_service_set_system_time_threshold_info(fbe_api_job_service_set_system_time_threshold_info_t *in_set_time_threshold_info);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_update_multi_pvds_pool_id(fbe_api_job_service_update_multi_pvds_pool_id_t *in_update_multi_pvds_pool_id_p);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_set_spare_library_config(fbe_bool_t b_disable_operation_confirmation,
                                             fbe_bool_t b_set_default_values,
                                             fbe_u32_t spare_operation_timeout_in_seconds);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_set_system_encryption_info(fbe_api_job_service_change_system_encryption_info_t *in_change_encryption_info_p);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_set_encryption_paused(fbe_bool_t encryption_paused);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_update_encryption_mode(fbe_api_job_service_update_encryption_mode_t *in_update_mode_p);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_setup_encryption_keys(fbe_encryption_key_table_t *keys_p,
                                          fbe_status_t* job_status,
                                          fbe_job_service_error_type_t *js_error_type);
fbe_status_t FBE_API_CALL 
fbe_api_job_service_scrub_old_user_data(void);
fbe_status_t FBE_API_CALL 
fbe_api_job_service_add_debug_hook(fbe_object_id_t object_id,
                                   fbe_job_type_t hook_type,
                                   fbe_job_action_state_t hook_state,
                                   fbe_job_debug_hook_state_phase_t hook_phase,
                                   fbe_job_debug_hook_action_t hook_action);
fbe_status_t FBE_API_CALL 
fbe_api_job_service_get_debug_hook(fbe_object_id_t object_id,
                                   fbe_job_type_t hook_type,
                                   fbe_job_action_state_t hook_state,
                                   fbe_job_debug_hook_state_phase_t hook_phase,
                                   fbe_job_debug_hook_action_t hook_action,
                                   fbe_job_service_debug_hook_t *debug_hook_p);
fbe_status_t FBE_API_CALL 
fbe_api_job_service_delete_debug_hook(fbe_object_id_t object_id,
                                      fbe_job_type_t hook_type,
                                      fbe_job_action_state_t hook_state,
                                      fbe_job_debug_hook_state_phase_t hook_phase,
                                      fbe_job_debug_hook_action_t hook_action);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_validate_database(fbe_api_job_service_validate_database_t *validate_database_p);

fbe_status_t FBE_API_CALL 
fbe_api_job_service_create_extent_pool(fbe_api_job_service_create_extent_pool_t *in_create_pool_p);
fbe_status_t FBE_API_CALL 
fbe_api_job_service_create_ext_pool_lun(fbe_api_job_service_create_ext_pool_lun_t *in_create_lun_p);
fbe_status_t FBE_API_CALL 
fbe_api_job_service_destroy_extent_pool(fbe_api_job_service_destroy_extent_pool_t *in_destroy_pool_p);
fbe_status_t FBE_API_CALL 
fbe_api_job_service_destroy_ext_pool_lun(fbe_api_job_service_destroy_ext_pool_lun_t *in_destroy_lun_p);

/*! @} */ /* end of group fbe_api_job_service_interface */

//----------------------------------------------------------------
// Define the group for all FBE Storage Extent Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API Storage Extent
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_storage_extent_package_interface_class_files FBE Storage Extent Package APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE Storage Extent Package APIs Interface.
 * 
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------
FBE_API_CPP_EXPORT_END

#endif /*FBE_API_JOB_SERVICE_INTERFACE_H*/






