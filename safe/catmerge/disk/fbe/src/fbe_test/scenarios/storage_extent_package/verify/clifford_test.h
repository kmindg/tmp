/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file clifford_test.h
 ***************************************************************************
 *
 * @brief
 *  This file contains a common utility function definitions for clifford tests.
 *
 * @version
 *   1/10/2011 - Created. Zhihao Tang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_database.h"
#include "sep_dll.h"
#include "physical_package_dll.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "sep_utils.h"
#include "fbe_test_configurations.h"



/*!*******************************************************************
 * @def CLIFFORD_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define CLIFFORD_TEST_MAX_WIDTH 16

/*!*******************************************************************
 * @def CLIFFORD_RAID_GROUP_CHUNK_SIZE
 *********************************************************************
 * @brief Number of blocks in a raid group bitmap chunk.
 *
 *********************************************************************/
#define CLIFFORD_RAID_GROUP_CHUNK_SIZE  (2 * FBE_RAID_SECTORS_PER_ELEMENT)

/*!*******************************************************************
 * @def CLIFFORD_ELEMENT_SIZE
 *********************************************************************
 * @brief Element size of the LUNs.
 *********************************************************************/
#define CLIFFORD_ELEMENT_SIZE 128 

/*!*******************************************************************
 * @def CLIFFORD_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS
 *********************************************************************
 * @brief Capacity of the virtual drives.
 *********************************************************************/
    /* Treat the drive capacity as small to end up with a raid group
 * capacity that is relatively small also. 
 * For comparison, a typical 32 mb sim drive is about 0xF000 blocks. 
 */
#define CLIFFORD_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS (0xE000)

/*!*******************************************************************
 * @def CLIFFORD_MAX_VERIFY_WAIT_TIME_SECS
 *********************************************************************
 * @brief Maximum time in seconds to wait for a verify operation to complete.
 *********************************************************************/
#define CLIFFORD_MAX_VERIFY_WAIT_TIME_SECS  60


void clifford_set_debug_flags(fbe_test_rg_configuration_t* in_rg_config_p, fbe_bool_t b_set_flags);
void clifford_raid0_verify_test(fbe_u32_t index, fbe_test_rg_configuration_t * in_current_rg_config_p);
void clifford_read_only_verify_test(fbe_u32_t index);
void clifford_set_trace_level(fbe_trace_type_t trace_type, fbe_u32_t id, fbe_trace_level_t level);

fbe_status_t clifford_issue_rdgen_write(fbe_api_rdgen_context_t *context_p,
                                               fbe_rdgen_operation_t rdgen_operation);

void clifford_wait_for_verify_and_validate_results(fbe_test_rg_configuration_t* in_current_rg_config_p,                                                          
                                                   fbe_u32_t            pass_count,
                                                   fbe_u32_t            injected_on_first,
                                                   fbe_u32_t            injected_on_second);

void clifford_validate_verify_results(fbe_lun_verify_report_t in_verify_report,
                                             fbe_u32_t           pass_count,
                                             fbe_u32_t           expected_error_count,
                                             fbe_u32_t           uncorrectable_error_count,
                                             fbe_u32_t           total_errors);

fbe_status_t clifford_control_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

void clifford_wait_for_lun_verify_completion(fbe_object_id_t            in_object_id,
                                                    fbe_lun_verify_report_t*   in_verify_report_p,
                                                    fbe_u32_t                  in_pass_count);

void clifford_get_lun_verify_status(fbe_object_id_t in_object_id,
                                    fbe_lun_get_verify_status_t* out_verify_status);

void clifford_setup_fill_range_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                             fbe_object_id_t object_id,
                                             fbe_class_id_t class_id,
                                             fbe_rdgen_operation_t rdgen_operation,
                                             fbe_rdgen_pattern_t pattern,
                                             fbe_lba_t start_lba,
                                             fbe_lba_t end_lba,
                                             fbe_u32_t io_size_blocks);

