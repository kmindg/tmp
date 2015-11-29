#ifndef SEP_REBUILD_UTILS_H
#define SEP_REBUILD_UTILS_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file sep_rebuild_utils.h
 ***************************************************************************
 *
 * @brief
 *   This file contains the defines and function prototypes for the rebuild
 *   service tests.
 * 
 * @version
 *   06/24/2011:  Created. Jason White.
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:


#include "fbe/fbe_types.h"                          //  general types
#include "sep_utils.h"                              //  for fbe_test_raid_group_disk_set_t
#include "fbe/fbe_terminator_api.h"                 //  for fbe_terminator_sas_drive_info_t

//-----------------------------------------------------------------------------
//  NAMED CONSTANTS (#defines):


//  Retry parameters when waiting for an action or state 
#define SEP_REBUILD_UTILS_MAX_RETRIES               1200      // retry count - number of times to loop
#define SEP_REBUILD_UTILS_RETRY_TIME                1000     // wait time in ms, in between retries

//  Common configuration parameters 
#define SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP       3       // number of LUNs in the RG
#define SEP_REBUILD_UTILS_ELEMENT_SIZE              128     // element size
#define SEP_REBUILD_UTILS_CHUNKS_PER_LUN            3       // number of chunks per LUN
#define SEP_REBUILD_UTILS_RG_CAPACITY               0x32000 // raid group's capacity
#define SEP_REBUILD_UTILS_DB_DRIVES_RG_CAPACITY     0xE000  // raid group's capacity
#define SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS   1       // position in the rg/edge of the drive that is removed first
#define SEP_REBUILD_UTILS_LAST_REMOVED_DRIVE_POS    0       // position in the rg/edge of the drive that is removed last
#define SEP_REBUILD_UTILS_SECOND_REMOVED_DRIVE_POS  2       // position in the rg/edge of the drive that is removed second

//  Bus and enclosure for the disks in the RG.
//  Enclosure 0-1 will be reserved for Hot Spares. 
#define SEP_REBUILD_UTILS_PORT                      0       // port (bus) 0
#define SEP_REBUILD_UTILS_ENCLOSURE_FIRST           0       // enclosure 1
#define SEP_REBUILD_UTILS_ENCLOSURE_SECOND          1       // enclosure 2
#define SEP_REBUILD_UTILS_ENCLOSURE_THIRD           2       // enclosure 3
#define SEP_REBUILD_UTILS_ENCLOSURE_FOURTH          3       // enclosure 4
#define SEP_REBUILD_UTILS_HOT_SPARE_ENCLOSURE       1       // enclosure 1 - all Hot Spares are located here

//  RAID group-specific configuration parameters 
#define SEP_REBUILD_UTILS_NUM_RAID_GROUPS           4       // number of RGs

#define SEP_REBUILD_UTILS_R5_RAID_GROUP_WIDTH       3       // number of drives in the RAID-5 RG
#define SEP_REBUILD_UTILS_R1_RAID_GROUP_WIDTH       2       // number of drives in the RAID-1 RG
#define SEP_REBUILD_UTILS_TRIPLE_MIRROR_RAID_GROUP_WIDTH    3
                                                    // number of drives in the triple mirror RAID-1 RG 
#define SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH       6       // number of drives in a RAID-6 RG
#define SEP_REBUILD_UTILS_R6_RAID_GROUP_2_WIDTH     4       // number of drives in another RAID-6 RG
#define SEP_REBUILD_UTILS_DB_DRIVES_RAID_GROUP_WIDTH 3      // number of drives in the user RG on the DB drives
#define SEP_REBUILD_UTILS_DUMMY_RAID_GROUP_WIDTH    2       // number of drives in dummy RG
#define SEP_REBUILD_UTILS_R10_RAID_GROUP_WIDTH      4       // number of drives in the RAID-10 RG

//  Disk slots for the RAID groups and Hot Spares  
#define SEP_REBUILD_UTILS_R5_2_SLOT_1               0       // slot for 1st disk in RAID-5: VD1 - 0-2-0 - rg 9 position 0
#define SEP_REBUILD_UTILS_R5_2_SLOT_2               1       // slot for 2nd disk in RAID-5: VD1 - 0-2-1 - rg 9 position 1
#define SEP_REBUILD_UTILS_R5_2_SLOT_3               2       // slot for 3rd disk in RAID-5: VD1 - 0-2-2 - rg 9 position 2
#define SEP_REBUILD_UTILS_HS_SLOT_0_2_3             3       // slot for hot spare disk          - 0-2-3

#define SEP_REBUILD_UTILS_DUMMY_SLOT_1              8       // slot for 1st disk in dummy RG: 0-2-8
#define SEP_REBUILD_UTILS_DUMMY_SLOT_2              9       // slot for 2nd disk in dummy RG: 0-2-9

#define SEP_REBUILD_UTILS_DB_DRIVES_SLOT_0          0       // slot for 1st disk in user RG on DB drives: 0-0-0
#define SEP_REBUILD_UTILS_DB_DRIVES_SLOT_1          1       // slot for 2nd disk in user RG on DB drives: 0-0-1
#define SEP_REBUILD_UTILS_DB_DRIVES_SLOT_2          2       // slot for 3rd disk in user RG on DB drives: 0-0-2

//  Disk positions in the RG  
#define SEP_REBUILD_UTILS_POSITION_0                0       // disk/edge position 0
#define SEP_REBUILD_UTILS_POSITION_1                1       // disk/edge position 1
#define SEP_REBUILD_UTILS_POSITION_2                2       // disk/edge position 2

 // Max time in seconds to wait for all error injections
#define SEP_REBUILD_UTILS_MAX_ERROR_INJECT_WAIT_TIME_SECS   100

 // Max sleep for quiesce test
#define SEP_REBUILD_UTILS_QUIESCE_TEST_MAX_SLEEP_TIME       1000

//  Max time wait for state changes
#define SEP_REBUILD_UTILS_WAIT_SEC                          60

//  Max time wait in MS for state changes
#define SEP_REBUILD_UTILS_WAIT_MSEC                  1000 * SEP_REBUILD_UTILS_WAIT_SEC

//  Threads per LUN for rdgen tests
#define SEP_REBUILD_UTILS_THREADS_PER_LUN                   8


//-----------------------------------------------------------------------------
//  ENUMERATIONS:


//-----------------------------------------------------------------------------
//  TYPEDEFS: 
//

//-----------------------------------------------------------------------------
//  MACROS:


//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES: 

//  Create a RG and its LUNs 
void sep_rebuild_utils_setup_raid_group(
                        fbe_test_rg_configuration_t*        in_rg_config_p, 
                        fbe_test_raid_group_disk_set_t*     in_disk_set_p,
                        fbe_lun_number_t*                   io_next_lun_num_p);

//  Test removal of a single drive in a RAID-5, RAID-3, or RAID-1 and that not rb logging
void sep_rebuild_utils_start_rb_logging_r5_r3_r1(
                        fbe_test_rg_configuration_t*        in_rg_config_p,
                        fbe_u32_t                           in_position,
                        fbe_api_terminator_device_handle_t* out_drive_info_p); 
 
//  Perform the removal of a drive and verify that the drive-related objects change state accordingly 
void sep_rebuild_utils_remove_drive_and_verify(
                                fbe_test_rg_configuration_t*        in_rg_config_p,
                                fbe_u32_t                           in_position,
                                fbe_u32_t                           in_num_objects,
                                fbe_api_terminator_device_handle_t* out_drive_info_p);

void sep_rebuild_utils_remove_drive_by_location(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot,
                                                fbe_api_terminator_device_handle_t * out_drive_info_p);


//  Perform the re-insertion of a drive and verify that the drive-related objects change state accordingly 
void sep_rebuild_utils_reinsert_drive_and_verify(
                                        fbe_test_rg_configuration_t*        in_rg_config_p,
                                        fbe_u32_t                           in_position,
                                        fbe_u32_t                           in_num_objects, 
                                        fbe_api_terminator_device_handle_t* in_drive_info_p);


// Gets bus, enclosure and slot information for the specific rg position.
void sep_rebuild_utils_get_drive_loc_for_specific_rg_pos(fbe_test_rg_configuration_t*    in_rg_config_p,
                                                      fbe_u32_t                       drive_rg_position, 
                                                      fbe_u32_t*                      out_bus_num_p,
                                                      fbe_u32_t*                      out_encl_num_p,
                                                      fbe_u32_t*                      out_slot_num_p);
//  Check that rb logging has started
void sep_rebuild_utils_verify_rb_logging(
                        fbe_test_rg_configuration_t*        in_rg_config_p,
                        fbe_u32_t                           in_position);

// Check that rebuild logging for all raid groups
fbe_status_t sep_rebuild_utils_verify_raid_groups_rb_logging(fbe_test_rg_configuration_t *in_rg_config_p,
                                                     fbe_u32_t  raid_group_count,
                                                     fbe_u32_t  in_position);

//  Check that rb logging is started, using the object ID 
void sep_rebuild_utils_verify_rb_logging_by_object_id(
                        fbe_object_id_t                     in_raid_group_object_id, 
                        fbe_u32_t                           in_position);

//  Check that rb logging is not started
void sep_rebuild_utils_verify_not_rb_logging(
                        fbe_test_rg_configuration_t*        in_rg_config_p,
                        fbe_u32_t                           in_position);

// verify none of the raid groups are rebuild logging
fbe_status_t sep_rebuild_utils_verify_no_raid_groups_are_rb_logging(fbe_test_rg_configuration_t *rg_config_p,
                                                                    fbe_u32_t raid_group_count,
                                                                    fbe_u32_t position);

//  Get the object ID for the R10 mirror object and adjust the position accordingly
void sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(
                                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                                    fbe_u32_t                       in_position, 
                                                    fbe_object_id_t*                out_mirror_raid_group_object_id_p, 
                                                    fbe_u32_t*                      out_position_p);

//  Check that the raid group and all of its LUNs are in the given state
void sep_rebuild_utils_check_raid_and_lun_states(
                        fbe_test_rg_configuration_t*        in_rg_config_p,
                        fbe_lifecycle_state_t               in_state);

//  Check that any SEP object is in the given state
void sep_rebuild_utils_verify_sep_object_state(
                        fbe_object_id_t                     in_object_id,
                        fbe_lifecycle_state_t               in_state);

//  Test reinsertion of a second drive in a triple mirror RAID-1 and RAID-6 and that rb logging stopped
void sep_rebuild_utils_triple_mirror_r6_second_drive_restored(
                        fbe_test_rg_configuration_t*        in_rg_config_p,
                        fbe_api_terminator_device_handle_t* in_drive_info_p);


// Setup the hot spare.
void sep_rebuild_utils_setup_hot_spare(
                          fbe_u32_t           in_port_num,
                          fbe_u32_t           in_encl_num,
                          fbe_u32_t           in_slot_num);

//  Test removal of a second drive in a triple mirror RAID-1 or a RAID-6 and that rb logging started
void sep_rebuild_utils_triple_mirror_r6_second_drive_removed(
                        fbe_test_rg_configuration_t*        in_rg_config_p,
                        fbe_api_terminator_device_handle_t* out_drive_info_p);

//  Test reinsertion of the first drive in a RAID-5, RAID-3, or RAID-1 and that rb logging stopped
void sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(
                        fbe_test_rg_configuration_t*        in_rg_config_p,
                        fbe_u32_t                           in_position,
                        fbe_api_terminator_device_handle_t* in_drive_info_p);

//  Wait for a rebuild to complete on the given disk
void sep_rebuild_utils_wait_for_rb_comp(
                            fbe_test_rg_configuration_t*        in_rg_config_p,
                            fbe_u32_t                           in_position);

//  Wait for a rebuild to complete, using the object id
void sep_rebuild_utils_wait_for_rb_comp_by_obj_id(
                            fbe_object_id_t                     in_raid_group_object_id,
                            fbe_u32_t                           in_position);

//  Write a background pattern to the LUNs/RGs indicated by the given context
void sep_rebuild_utils_write_bg_pattern(
                            fbe_api_rdgen_context_t*            in_test_context_p,
                            fbe_u32_t                           in_element_size);

//  Read the same background pattern from the LUNs/RGs indicated by the given context
void sep_rebuild_utils_read_bg_pattern(
                            fbe_api_rdgen_context_t*            in_test_context_p,
                            fbe_u32_t                           in_element_size);

// Wait for a rebuild to start
void sep_rebuild_utils_wait_for_rb_to_start(
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_position);

// Return the first position that starts rebuilding.
fbe_status_t sep_rebuild_utils_wait_for_any_rb_to_start(fbe_test_rg_configuration_t* rg_config_p,
                                                        fbe_raid_position_t * position_p);
// Wait until this rg is not rebuild logging.
fbe_status_t sep_rebuild_utils_wait_for_rb_logging_clear(fbe_test_rg_configuration_t* rg_config_p);

// Wait for rebuild logging to be clear for the position specified
fbe_status_t sep_rebuild_utils_wait_for_rb_logging_clear_for_position(fbe_object_id_t rg_object_id,
                                                                      fbe_u32_t position);

// Wait for a rebuild to start for an object id
void sep_rebuild_utils_wait_for_rb_to_start_by_obj_id(
                                    fbe_object_id_t                 in_raid_group_object_id,
                                    fbe_u32_t                       in_position);

// randomly pick a specified number of slots from the given config
void sep_rebuild_utils_get_random_drives(fbe_test_rg_configuration_t*    rg_config_p,
							       fbe_u32_t                       drives_to_remove,
							       fbe_u32_t*                      slots);

void sep_rebuild_utils_setup(void);

void sep_rebuild_utils_dualsp_setup(void);

void sep_rebuild_utils_remove_drive_no_check(
                                        fbe_u32_t                   		in_port_num,
                                        fbe_u32_t                   		in_encl_num,
                                        fbe_u32_t                   		in_slot_num,
                                        fbe_u32_t                   		in_num_objects,
                                        fbe_api_terminator_device_handle_t  *unused);

void sep_rebuild_utils_reinsert_removed_drive(fbe_u32_t                   in_port_num,
										      fbe_u32_t                   in_encl_num,
										      fbe_u32_t                   in_slot_num,
										      fbe_api_terminator_device_handle_t  *drive_handle);

void sep_rebuild_utils_reinsert_removed_drive_no_check(fbe_u32_t                           in_port_num,
                                  	  	  	           fbe_u32_t                           in_encl_num,
                                  	  	  	           fbe_u32_t                           in_slot_num,
                                  	  	  	           fbe_api_terminator_device_handle_t  *drive_handle);

void sep_rebuild_utils_verify_reinserted_drive(
                                        fbe_test_rg_configuration_t*        in_rg_config_p,
                                        fbe_u32_t                           in_position,
                                        fbe_u32_t                           in_num_objects,
                                        fbe_api_terminator_device_handle_t* in_drive_info_p);

fbe_status_t sep_rebuild_utils_find_free_drive(fbe_test_raid_group_disk_set_t *free_drive_pos_p,
                                               fbe_test_raid_group_disk_set_t *disk_set_p,
                                               fbe_u32_t hs_count);

void sep_rebuild_utils_check_for_reb_restart(
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_position);

void sep_rebuild_utils_check_for_reb_in_progress(
        fbe_test_rg_configuration_t*    in_rg_config_p,
        fbe_u32_t                       in_position);

void sep_rebuild_utils_get_reb_checkpoint(
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_position,
                                    fbe_lba_t*                      out_checkpoint);

void sep_rebuild_utils_remove_hs_drive(
								fbe_u32_t 							bus,
								fbe_u32_t 							enclosure,
								fbe_u32_t 							slot,
								fbe_object_id_t                     raid_group_object_id,
                                fbe_u32_t                           in_position,
                                fbe_u32_t                           in_num_objects,
                                fbe_api_terminator_device_handle_t* out_drive_info_p);

fbe_status_t sep_rebuild_utils_validate_event_log_errors(void);

//  Write new data to the RG when rb logging
void sep_rebuild_utils_write_new_data(
                                        fbe_test_rg_configuration_t*        in_rg_config_p,
                                        fbe_api_rdgen_context_t*            in_test_context_p,
                                        fbe_lba_t                           in_start_lba);

//  Verify the new data is correct after rebuild
void sep_rebuild_utils_verify_new_data(
                                        fbe_test_rg_configuration_t*        in_rg_config_p,
                                        fbe_api_rdgen_context_t*            in_test_context_p,
                                        fbe_lba_t                           in_start_lba);

//  Perform I/Os to read or write the new data
void sep_rebuild_utils_do_io(
                                        fbe_test_rg_configuration_t*        in_rg_config_p,
                                        fbe_api_rdgen_context_t*            in_test_context_p,
                                        fbe_rdgen_operation_t               in_operation_type,
                                        fbe_lba_t                           in_start_lba);

// Check the bits in an RG for rebuild complete
void sep_rebuild_utils_check_bits(
                                    fbe_object_id_t                 in_raid_group_object_id);

/* Validate that all raid groups are degraded. */
void fbe_test_sep_rebuild_validate_rg_is_degraded(fbe_test_rg_configuration_t *rg_config_p, 
                                                  fbe_u32_t raid_group_count,
                                                  fbe_u32_t expected_removed_position);

// Validate that the position specified is not marked NR
void fbe_test_sep_rebuild_utils_check_bits_clear_for_position(fbe_object_id_t  in_raid_group_object_id,
                                                              fbe_u32_t position_to_validate);

// Validate that the position specified is marked NR
void fbe_test_sep_rebuild_utils_check_bits_set_for_position(fbe_object_id_t  in_raid_group_object_id,
                                                            fbe_u32_t position_to_validate);

// Wait for rebuild logging to be clear for the position specified
void fbe_test_sep_rebuild_utils_wait_for_rebuild_logging_clear_for_position(fbe_object_id_t rg_object_id,
                                                            fbe_u32_t position_to_validate);

// Corrupt the paged metadata associated with teh user space extent supplied
fbe_status_t fbe_test_sep_rebuild_utils_corrupt_paged_metadata(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_lba_t user_space_lba,
                                                               fbe_block_count_t user_space_blocks);

// Determine the rebuild hook lba to stop after the consumed percentage has been rebuilt
fbe_status_t fbe_test_sep_rebuild_get_consumed_checkpoint_to_pause_at(fbe_object_id_t rg_object_id,
                                                                     fbe_u32_t percent_user_space_rebuilt_before_pause,
                                                                     fbe_lba_t *checkpoint_to_pause_at_p);

// Determine the rebuild hook lba to stop after the raid group percentage has been rebuilt
fbe_status_t fbe_test_sep_rebuild_get_raid_group_checkpoint_to_pause_at(fbe_object_id_t rg_object_id,
                                                                     fbe_u32_t percent_user_space_rebuilt_before_pause,
                                                                     fbe_lba_t *checkpoint_to_pause_at_p);

// Generate a set of distributed I/Os
fbe_status_t fbe_test_sep_rebuild_generate_distributed_io(fbe_test_rg_configuration_t *rg_config_p,
                                                          fbe_u32_t raid_group_count,
                                                          fbe_api_rdgen_context_t *rdgen_context_p,
                                                          fbe_rdgen_operation_t rdgen_op,
                                                          fbe_rdgen_pattern_t rdgen_pattern,
                                                          fbe_u64_t requested_blocks);

// Validate that no I/Os are stuck in quiesce
fbe_status_t fbe_test_sep_rebuild_validate_no_quiesced_io(fbe_test_rg_configuration_t *rg_config_p,
                                                          fbe_u32_t raid_group_count);

// Set eval mark NR done for set of raid groups
void fbe_test_sep_rebuild_set_raid_group_mark_nr_done_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                           fbe_u32_t raid_group_count);

// Wait for eval mark NR to complete on set of raid groups
void fbe_test_sep_rebuild_wait_for_raid_group_mark_nr_done_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                                fbe_u32_t raid_group_count);

// Validate that no chunks were marked NR on a set of raid groups
void fbe_test_sep_rebuild_validate_raid_group_not_marked_nr(fbe_test_rg_configuration_t *rg_config_p,
                                                            fbe_u32_t raid_group_count);

// Validate that there are no chunks marked NR for teh position specified
void fbe_test_sep_rebuild_validate_raid_group_position_not_marked_nr(fbe_test_rg_configuration_t *rg_config_p,
                                                                     fbe_u32_t raid_group_count,
                                                                     fbe_u32_t position_to_check);

// Remove the eval mark NR done hook
void fbe_test_sep_rebuild_del_raid_group_mark_nr_done_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                           fbe_u32_t raid_group_count);

#endif // SEP_REBUILD_UTILS_H


