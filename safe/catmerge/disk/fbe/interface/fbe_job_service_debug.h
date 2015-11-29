#ifndef FBE_JOB_SERVICE_DEBUG_H
#define FBE_JOB_SERVICE_DEBUG_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_job_service_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the job service debug library.
 *
 * @author
 *  31-Dec-2010 - Created. Hari Singh Chauhan
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"
#include "fbe_job_service.h"

fbe_status_t fbe_job_service_debug_trace(const fbe_u8_t * module_name,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context,
                                         fbe_u64_t last_queue_element,
                                         fbe_job_control_queue_type_t queue_type);

fbe_status_t fbe_job_service_queue_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr queue_element,
                                               fbe_u64_t last_queue_element,
                                               fbe_job_control_queue_type_t queue_type);

fbe_status_t fbe_job_service_queue_element_debug_trace(const fbe_u8_t * module_name,
                                                       fbe_trace_func_t trace_func,
                                                       fbe_trace_context_t trace_context,
                                                       fbe_u64_t queue_head,
                                                       fbe_u32_t queue_index);

fbe_status_t fbe_job_service_drive_swap_info_debug_trace(const fbe_u8_t * module_name,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_dbgext_ptr object_ptr);

fbe_status_t fbe_job_service_raid_group_create_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_dbgext_ptr object_ptr);

fbe_status_t fbe_job_service_raid_group_destroy_debug_trace(const fbe_u8_t * module_name,
                                                            fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_dbgext_ptr object_ptr);

fbe_status_t fbe_job_service_update_raid_group_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_dbgext_ptr object_ptr);

fbe_status_t fbe_job_service_lun_create_debug_trace(const fbe_u8_t * module_name,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_dbgext_ptr object_ptr);

fbe_status_t fbe_job_service_lun_destroy_debug_trace(const fbe_u8_t * module_name,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     fbe_dbgext_ptr object_ptr);

fbe_status_t fbe_job_service_update_virtual_drive_debug_trace(const fbe_u8_t * module_name,
                                                              fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_dbgext_ptr object_ptr);

fbe_status_t fbe_job_service_update_provision_drive_debug_trace(const fbe_u8_t * module_name,
                                                                fbe_trace_func_t trace_func,
                                                                fbe_trace_context_t trace_context,
                                                                fbe_dbgext_ptr object_ptr);

fbe_status_t fbe_job_service_update_provision_drive_block_size_debug_trace(const fbe_u8_t * module_name,
                                                                fbe_trace_func_t trace_func,
                                                                fbe_trace_context_t trace_context,
                                                                fbe_dbgext_ptr object_ptr);

fbe_status_t fbe_job_service_update_spare_config_debug_trace(const fbe_u8_t * module_name,
                                                             fbe_trace_func_t trace_func,
                                                             fbe_trace_context_t trace_context,
                                                             fbe_dbgext_ptr object_ptr);

fbe_status_t fbe_job_service_change_system_power_saving_info_debug_trace(const fbe_u8_t * module_name,
                                                                         fbe_trace_func_t trace_func,
                                                                         fbe_trace_context_t trace_context,
                                                                         fbe_dbgext_ptr object_ptr);

fbe_status_t fbe_job_service_update_db_on_peer_debug_trace(const fbe_u8_t * module_name,
                                                                         fbe_trace_func_t trace_func,
                                                                         fbe_trace_context_t trace_context,
                                                                         fbe_dbgext_ptr object_ptr);

fbe_status_t fbe_job_service_change_system_encryption_info_debug_trace(const fbe_u8_t * module_name,
                                                                         fbe_trace_func_t trace_func,
                                                                         fbe_trace_context_t trace_context,
                                                                         fbe_dbgext_ptr object_ptr);

fbe_status_t fbe_job_service_set_encryption_paused_debug_trace(const fbe_u8_t * module_name,
                                                                         fbe_trace_func_t trace_func,
                                                                         fbe_trace_context_t trace_context,
                                                                         fbe_dbgext_ptr object_ptr);

fbe_status_t fbe_job_service_validate_database_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_dbgext_ptr object_ptr);

#endif  /* FBE_JOB_SERVICE_DEBUG_H*/






