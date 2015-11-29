#ifndef FBE_JOB_SERVICE_OPERATIONS_H
#define FBE_JOB_SERVICE_OPERATIONS_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_job_service_operations.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for raid Group and Lun creation functions 
 *
 * @version
 *   01/05/2010:  Created. Jesus Medina
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_base_object.h"
#include "fbe_transport_memory.h"
#include "fbe_base_service.h"
#include "fbe_topology.h"
#include "fbe_job_service.h"

/* fbe lun creation library */
extern fbe_job_service_operation_t fbe_lun_create_job_service_operation;

/* fbe lun destroy library */
extern fbe_job_service_operation_t fbe_lun_destroy_job_service_operation;

/* fbe lun creation library */
extern fbe_job_service_operation_t fbe_raid_group_create_job_service_operation;

/* fbe lun destroy library */
extern fbe_job_service_operation_t fbe_raid_group_destroy_job_service_operation;

/*power saving change library*/
extern fbe_job_service_operation_t fbe_set_system_ps_info_job_service_operation;

/*updating data in the raid grop and persisting it*/
extern fbe_job_service_operation_t fbe_update_raid_group_job_service_operation;

/* upcating rg, vd, pvd encryption modes and persisting them.  */
extern fbe_job_service_operation_t fbe_update_encryption_mode_job_service_operation;

/* Spare Library registration with job service. */
extern fbe_job_service_operation_t fbe_spare_job_service_operation;

/* Spare library - spare config update registeration with job service. */
extern fbe_job_service_operation_t fbe_update_spare_config_job_service_operation;

/*update configuration mode of the virtual drive object and persisting it */
extern fbe_job_service_operation_t fbe_update_virtual_drive_job_service_operation;

/*updating config type of virtual drive object and persisting it */
extern fbe_job_service_operation_t fbe_update_provision_drive_job_service_operation;

/*updating config type of virtual drive object and persisting it */
extern fbe_job_service_operation_t fbe_update_provision_drive_block_size_job_service_operation;

/* fbe provision drive creation library */
extern fbe_job_service_operation_t fbe_create_provision_drive_job_service_operation;

/* fbe lun update library */
extern fbe_job_service_operation_t fbe_update_lun_job_service_operation;

/* fbe_create library in fbe_update_db_on_peer.c*/
extern fbe_job_service_operation_t fbe_update_db_on_peer_job_service_operation;

/* fbe_create library in fbe_destroy_provision_drive.c*/
extern fbe_job_service_operation_t fbe_destroy_provision_drive_job_service_operation;

/* reinitialize the pvd for system drive if a new drive get inserted */
extern fbe_job_service_operation_t fbe_reinitialize_pvd_job_service_operation;

/* fbe_control_system_bg_service library */
extern fbe_job_service_operation_t fbe_control_system_bg_service_job_service_operation;

/* fbe_create library in fbe_ndu_commit.c*/
extern fbe_job_service_operation_t fbe_ndu_commit_job_service_operation;
/*system time threshold library*/
extern fbe_job_service_operation_t fbe_set_system_time_threshold_job_service_operation;

/* fbe drive connect library */
extern fbe_job_service_operation_t fbe_drive_connect_job_service_operation;

/* update multi pvds pool id library */
extern fbe_job_service_operation_t fbe_update_multi_pvds_pool_id_job_service_operation;

/* change the configuration (time to wait for operations etc) configuration */
extern fbe_job_service_operation_t fbe_update_spare_library_config_operation;

/*encryption change library*/
extern fbe_job_service_operation_t fbe_set_system_encryption_info_job_service_operation;

/* process encryption keys */
extern fbe_job_service_operation_t fbe_process_encryption_keys_job_service_operation;

/* scrubbing previous user data */
extern fbe_job_service_operation_t fbe_scrub_old_user_data_job_service_operation;

extern fbe_job_service_operation_t fbe_set_encryption_paused_job_service_operation;

/* validate in-memory database against on-disk and take requested action */
extern fbe_job_service_operation_t fbe_validate_database_job_service_operation;

extern fbe_job_service_operation_t fbe_create_extent_pool_job_service_operation;

extern fbe_job_service_operation_t fbe_create_extent_pool_lun_job_service_operation;

extern fbe_job_service_operation_t fbe_destroy_extent_pool_job_service_operation;

extern fbe_job_service_operation_t fbe_destroy_extent_pool_lun_job_service_operation;

#endif /* end FBE_JOB_SERVICE_OPERATIONS_H */
