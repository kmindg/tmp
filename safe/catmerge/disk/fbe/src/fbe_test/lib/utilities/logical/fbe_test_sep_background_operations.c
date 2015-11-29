/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_sep_background_operations.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the methods to start, validate and stop background
 *  operations.
 * 
 * @author
 *  03/22/2012  Ron Proulx  - Created.
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "sep_tests.h"
#include "fbe_test_common_utils.h"
#include "pp_utils.h"
#include "sep_utils.h"
#include "sep_rebuild_utils.h"
#include "neit_utils.h"
#include "sep_test_io.h"
#include "sep_test_io_private.h"
#include "sep_test_background_ops.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_sep.h"

/*****************************************
 * DEFINITIONS
 *****************************************/
#define FBE_TEST_SEP_BACKGROUND_OPS_MAX_LUNS_PER_RAID_GROUP               3
#define FBE_TEST_SEP_BACKGROUND_OPS_MAX_RETRIES                         160     // retry count - number of times to loop
#define FBE_TEST_SEP_BACKGROUND_OPS_RETRY_TIME                         1000     // wait time in ms, in between retries 
#define FBE_TEST_SEP_BACKGROUND_OPS_SHORT_RETRY_TIME                    100     // wait time in ms, in between retries 
#define FBE_TEST_SEP_BACKGROUND_OPS_MAX_SHORT_RETRIES                  1200
#define FBE_TEST_SEP_BACKGROUND_OPS_EVENT_LOG_WAIT_TIME                 100 

/*!*******************************************************************
 * @def     FBE_TEST_SEP_BACKGROUND_OPS_INVALID_DISK_POSITION
 *********************************************************************
 *  @brief  Invalid disk position.  Used when searching for a disk 
 *          position and no disk is found that meets the criteria.
 *
 *********************************************************************/
#define FBE_TEST_SEP_BACKGROUND_OPS_INVALID_DISK_POSITION           ((fbe_u32_t) -1)

/*!*******************************************************************
 * @def     FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUPS_WITHOUT_DELAY
 *********************************************************************
 * @brief   Maximum number of raid groups that can be tested without
 *          adding a debug hook to slow copy process. 
 *
 *********************************************************************/
#define FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUPS_WITHOUT_DELAY   (2)

/*************************
 *   GLOBALS
 *************************/
static fbe_test_sep_background_ops_event_info_t fbe_test_sep_background_ops_event_info[FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_LAST] =
{
    {FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_INVALID,                 "INVALID EVENT !"        },
    {FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SOURCE_EDGE_EOL,         "SOURCE EDGE EOL"        },
    {FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_IN,                 "PVD SWAP IN"            },
    {FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_MIRROR_MODE,             "MIRROR MODE"            },
    {FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_METADATA_REBUILD_START,  "METADATA REBUILD START" },
    {FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_REBUILD_HOOK,            "REBUILD HOOK"           },
    {FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_START,              "COPY START"             },
    {FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_COMPLETE,           "COPY COMPLETE"          },
    {FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_COMPLETE_INITIATED, "COPY_COMPLETE_INITIATED"},
    {FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_OUT,                "PVD SWAP OUT"           },
    {FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_OUT_COMPLETE,       "SWAP OUT JOB COMPLETE"  },
    {FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SOURCE_MARKED_NR,        "SOURCE MARKED NR"       },
    {FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_REINSERT_FAILED_DRIVES,  "REINSERT FAILED DRIVES" },
};

/*!***************************************************************************
 * @var fbe_test_sep_background_ops_b_is_DE542_fixed
 *****************************************************************************
 * @brief DE542 - Due to the fact that protocol error are injected on the
 *                way to the disk, the data is never sent to or read from
 *                the disk.  Therefore until this defect is fixed b_start_io
 *                MUST be True.
 *
 * @todo  Fix defect DE542.
 *
 *****************************************************************************/
static fbe_bool_t fbe_test_sep_background_ops_b_is_DE542_fixed = FBE_FALSE;

/*!*******************************************************************
 * @var fbe_test_sep_background_ops_user_space_lba_to_inject_to
 *********************************************************************
 * @brief This is the default user space lba to start the error injection
 *        to.
 *
 *********************************************************************/
static fbe_lba_t fbe_test_sep_background_ops_user_space_lba_to_inject_to          =   0x0ULL; /* lba 0 on first LUN.  Must be aligned to 4K block size.*/

/*!*******************************************************************
 * @var fbe_test_sep_background_ops_user_space_lba_to_inject_to
 *********************************************************************
 * @brief This is the default user space lba to start the error injection
 *        to.
 *
 *********************************************************************/
static fbe_block_count_t fbe_test_sep_background_ops_user_space_blocks_to_inject  = 0x20ULL; /* Must be aligned to 4K block size.*/

/*!*******************************************************************
 * @var fbe_test_sep_background_ops_default_offset
 *********************************************************************
 * @brief This is the default user space offset.
 *
 *********************************************************************/
static fbe_lba_t fbe_test_sep_background_ops_default_offset                       = 0x10000ULL; /* Offset used maybe different */

/*!*******************************************************************
 * @var fbe_test_sep_background_ops_copy_position
 *********************************************************************
 * @brief This is the current copy position.
 *
 *********************************************************************/
static fbe_u32_t fbe_test_sep_background_ops_copy_position = FBE_TEST_SEP_BACKGROUND_OPS_INVALID_DISK_POSITION;

/*!*******************************************************************
 * @var     fbe_test_sep_background_ops_dest_array
 *********************************************************************
 * @brief   Arrays of destination positions to spare (for user copy).
 *
 *********************************************************************/
static fbe_test_raid_group_disk_set_t fbe_test_sep_background_ops_dest_array[FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT] = { 0 };

/*!*******************************************************************
 * @var fbe_test_sep_background_ops_test_context
 *********************************************************************
 * @brief rdgen context that is only used when I/O is required to start
 *        the copy operation.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t  fbe_test_sep_background_ops_test_context[FBE_TEST_SEP_BACKGROUND_OPS_MAX_LUNS_PER_RAID_GROUP * 2];

/*!*******************************************************************
 * @var     fbe_test_sep_background_ops_rebuild_delay
 *********************************************************************
 * @brief   This is used to `delay' the rebuild so that we don't jump
 *          ahead in the copy operation.
 *
 *********************************************************************/
static fbe_sep_package_load_params_t fbe_test_sep_background_ops_rebuild_delay = { 0 }; 


/*************************
 *   FUNCTIONS
 *************************/


/*!**************************************************************
 *          fbe_test_sep_background_ops_get_event_info()
 ****************************************************************
 * @brief
 *  Returns a pointer to the event information for the requested event.
 *
 * @param event_type - Event type to get information for
 * @param opaque_param_p - Opaque pointer to parameters for event
 *
 * @return Pointer to fbe_test_sep_background_ops_event_info_t
 ****************************************************************/
fbe_test_sep_background_ops_event_info_t *fbe_test_sep_background_ops_get_event_info(fbe_test_sep_background_ops_event_type_t event_type)
{
    MUT_ASSERT_TRUE((event_type > FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_INVALID) &&
                    (event_type < FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_LAST)       );
    return (&fbe_test_sep_background_ops_event_info[event_type]);
}
/************************************************
 * end fbe_test_sep_background_ops_get_event_info()
 ************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_refresh_raid_group_disk_info()
 *****************************************************************************
 *
 * @brief   `Refresh' the raid group test configuraiton information.
 *
 * @param   rg_config_p - Pointer to raid group array information
 * @param   raid_group_count - Number of riad groups in the array
 *
 * @return  status
 *
 * @note    Assumes that `extra_disks' is the number required for degraded I/O.
 *
 * @author
 *  04/11/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_ops_refresh_raid_group_disk_info(fbe_test_rg_configuration_t *rg_config_p,
                                                                      fbe_u32_t raid_group_count)
{
    /* First refresh the bound drive information
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);

    /*! @note Since drives marked EOL are still `online' there should be no need
     *        to refresh the physical disk information.
     */
    //fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

    /* Currently this always succeeds.
     */
    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_test_sep_background_ops_refresh_raid_group_disk_info()
 ****************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_populate_io_error_injection_record()
 *****************************************************************************
 *
 * @brief   Populates the passed protocol error injection record.
 *
 * @param   rg_config_p - Pointer to raid group information
 * @param   drive_pos - The position in the array passed to inject to
 * @param   requested_scsi_command_to_trigger - The SCSI command to inject for
 * @param   record_p - Pointer to record to populate
 * @param   record_handle_p - Pointer to record handle
 *
 * @return Pointer to fbe_test_sep_background_ops_event_info_t
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_ops_populate_io_error_injection_record(fbe_test_rg_configuration_t *rg_config_p,
                                                                         fbe_u32_t drive_pos,
                                                                         fbe_u8_t requested_scsi_command_to_trigger,
                                                                         fbe_protocol_error_injection_error_record_t *record_p,
                                                                         fbe_protocol_error_injection_record_handle_t *record_handle_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u8_t                    scsi_command_to_trigger = requested_scsi_command_to_trigger;
    fbe_object_id_t             drive_object_id = 0;
    fbe_object_id_t             pvd_object_id = 0;
    fbe_api_provision_drive_info_t provision_drive_info;

    /* Get the drive object id for the position specified.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                              rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                              rg_config_p->rg_disk_set[drive_pos].slot, 
                                                              &drive_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Calculate the physical lba to inject to.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_background_ops_default_offset = provision_drive_info.default_offset;

    /*! @todo DE542 - Due to the fact that protocol error are injected on the
     *                way to the disk, the data is never sent to or read from
     *                the disk.  Therefore until this defect is fixed force the
     *                SCSI opcode to inject on to VERIFY.
     */
    if (fbe_test_sep_background_ops_b_is_DE542_fixed == FBE_FALSE)
    {
        if (requested_scsi_command_to_trigger != FBE_SCSI_VERIFY_16)
        {
            //mut_printf(MUT_LOG_LOW, "%s: Due to DE542 change opcode from: 0x%02x to: 0x%02x", 
            //           __FUNCTION__, requested_scsi_command_to_trigger, FBE_SCSI_VERIFY_16);
            scsi_command_to_trigger = FBE_SCSI_VERIFY_16;
        }
        MUT_ASSERT_TRUE(scsi_command_to_trigger == FBE_SCSI_VERIFY_16);
    }

    /* Invoke the method to initialize and populate the (1) error record
     */
    status = fbe_test_neit_utils_populate_protocol_error_injection_record(rg_config_p->rg_disk_set[drive_pos].bus,
                                                                          rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                                          rg_config_p->rg_disk_set[drive_pos].slot,
                                                                          record_p,
                                                                          record_handle_p,
                                                                          fbe_test_sep_background_ops_user_space_lba_to_inject_to,
                                                                          fbe_test_sep_background_ops_user_space_blocks_to_inject,
                                                                          FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI,
                                                                          scsi_command_to_trigger,
                                                                          FBE_SCSI_SENSE_KEY_RECOVERED_ERROR,
                                                                          FBE_SCSI_ASC_PFA_THRESHOLD_REACHED,
                                                                          FBE_SCSI_ASCQ_GENERAL_QUALIFIER,
                                                                          FBE_PORT_REQUEST_STATUS_SUCCESS, 
                                                                          1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return status;
}
/**********************************************************************
 * end fbe_test_sep_background_ops_populate_io_error_injection_record()
 **********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_run_io_to_start_copy()
 *****************************************************************************
 *
 * @brief   If the test isn't running I/O but they want to trigger a proactive
 *          copy, this method is invoked to run the specified I/O type which
 *          will trigger the copy.
 *
 * @param   rg_config_p - Pointer to raid group array to locate pvds
 * @param   raid_group_count - Number of raid groups
 * @param   sparing_position - Position that is being copied
 * @param   requested_scsi_opcode_to_inject_on - SCSI operation that is being injected to
 * @param   io_size - Fixed number of blocks per I/O
 *
 * @return  status - FBE_STATUS_OK
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_ops_run_io_to_start_copy(fbe_test_rg_configuration_t *rg_config_p,
                                                              fbe_u32_t raid_group_count,
                                                              fbe_u32_t sparing_position,
                                                              fbe_u8_t requested_scsi_opcode_to_inject_on,
                                                              fbe_block_count_t io_size)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t        *rdgen_context_p = &fbe_test_sep_background_ops_test_context[0];
    fbe_u8_t                        scsi_opcode_to_inject_on = requested_scsi_opcode_to_inject_on;
    fbe_rdgen_operation_t           rdgen_operation = FBE_RDGEN_OPERATION_INVALID;
    fbe_u32_t                       rg_index;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_raid_group_disk_set_t *drive_to_spare_p = NULL; 
    fbe_api_get_block_edge_info_t   edge_info;
    fbe_class_id_t                  class_id;
    fbe_lba_t                       user_space_start_lba = FBE_LBA_INVALID;
    fbe_block_count_t               blocks = 0;
    fbe_lba_t                       start_lba = FBE_LBA_INVALID;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_lba_t                       default_offset = FBE_LBA_INVALID;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;

    /* We must inject to the same pvd block that the error was injected to.
     */
    user_space_start_lba = fbe_test_sep_background_ops_user_space_lba_to_inject_to;
    blocks = fbe_test_sep_background_ops_user_space_blocks_to_inject;

    /*! @todo DE542 - Due to the fact that protocol error are injected on the
     *                way to the disk, the data is never sent to or read from
     *                the disk.  Therefore until this defect is fixed force the
     *                SCSI opcode to inject on to VERIFY.
     */
    if (fbe_test_sep_background_ops_b_is_DE542_fixed == FBE_FALSE)
    {
        if (requested_scsi_opcode_to_inject_on != FBE_SCSI_VERIFY_16)
        {
            //mut_printf(MUT_LOG_LOW, "%s: Due to DE542 change opcode from: 0x%02x to: 0x%02x", 
            //           __FUNCTION__, requested_scsi_opcode_to_inject_on, FBE_SCSI_VERIFY_16);
            scsi_opcode_to_inject_on = FBE_SCSI_VERIFY_16;
        }
        MUT_ASSERT_TRUE(scsi_opcode_to_inject_on == FBE_SCSI_VERIFY_16);
    }

    /* Determine the rdgen operation based on the SCSI opcode
     */
    switch (scsi_opcode_to_inject_on)
    {
        case FBE_SCSI_READ_6:
        case FBE_SCSI_READ_10:
        case FBE_SCSI_READ_12:
        case FBE_SCSI_READ_16:
            /* Generate read only requests. */
            rdgen_operation = FBE_RDGEN_OPERATION_READ_ONLY;
            break;
        case FBE_SCSI_VERIFY_10:
        case FBE_SCSI_VERIFY_16:
            /* Generate verify only */
            rdgen_operation = FBE_RDGEN_OPERATION_VERIFY;
            break;
        case FBE_SCSI_WRITE_6:
        case FBE_SCSI_WRITE_10:
        case FBE_SCSI_WRITE_12:
        case FBE_SCSI_WRITE_16:
            /* Generate write only */
            rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
            break;
        case FBE_SCSI_WRITE_SAME_10:
        case FBE_SCSI_WRITE_SAME_16:
            /* Generate zero requests. */
            rdgen_operation = FBE_RDGEN_OPERATION_ZERO_ONLY;
            break;
        case FBE_SCSI_WRITE_VERIFY_10:
        case FBE_SCSI_WRITE_VERIFY_16:
            /* Generate write verify requests. */
            rdgen_operation = FBE_RDGEN_OPERATION_WRITE_VERIFY;
            break;
        default:
            /* Unsupported opcode */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Unsupported SCSI opcode: 0x%x",
                       __FUNCTION__, scsi_opcode_to_inject_on);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
    }

    /* For all raid groups locate the pvd to inject the error on then
     * inject the error on the command requested.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Skip raid groups that are not enabled.
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        drive_to_spare_p = &current_rg_config_p->rg_disk_set[sparing_position];
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, sparing_position, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Determine the source and destination edge index.
         */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Get the pvd to issue the I/O to
         */
        status = fbe_api_get_block_edge_info(vd_object_id, source_edge_index, &edge_info, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_get_object_class_id(vd_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_TRUE((status == FBE_STATUS_OK) && (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE));

        /* Verify the class-id of the object to be provision drive
         */
        pvd_object_id = edge_info.server_id;
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_object_id);
        status = fbe_api_get_object_class_id(pvd_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);

        /*! @note PVD Sniff could automatically trigger copy.
         */
        if ((status != FBE_STATUS_OK)                  || 
            (class_id != FBE_CLASS_ID_PROVISION_DRIVE)    )  
        {
            if (rdgen_operation == FBE_RDGEN_OPERATION_VERIFY)
            {
                /* Sniff will trigger EOL.
                */
                start_lba = user_space_start_lba + fbe_test_sep_background_ops_default_offset;
                mut_printf(MUT_LOG_TEST_STATUS, "%s: rdgen op: %d pvd Sniff to lba: 0x%llx blks: 0x%llx triggers copy.", 
                           __FUNCTION__, rdgen_operation, (unsigned long long)start_lba, (unsigned long long)blocks);
                current_rg_config_p++;
                continue;
            }
            MUT_ASSERT_TRUE((status == FBE_STATUS_OK) && (class_id == FBE_CLASS_ID_PROVISION_DRIVE));
        }

        /* Get default offset to locate lba to issue request to.
         */
        status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
        /*! @note PVD Sniff could automatically trigger copy.
         */
        if (status != FBE_STATUS_OK)
        {
            if (rdgen_operation == FBE_RDGEN_OPERATION_VERIFY)
            {
                /* Sniff will trigger EOL.
                */
                start_lba = user_space_start_lba + fbe_test_sep_background_ops_default_offset;
                mut_printf(MUT_LOG_TEST_STATUS, "%s: rdgen op: %d pvd Sniff to lba: 0x%llx blks: 0x%llx triggers copy.", 
                           __FUNCTION__, rdgen_operation, (unsigned long long)start_lba, (unsigned long long)blocks);
                current_rg_config_p++;
                continue;
            }
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        default_offset = provision_drive_info.default_offset;
        start_lba = user_space_start_lba + default_offset;

        /* Send a single I/O to each PVD.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Issue rdgen op: %d to pvd: 0x%x lba: 0x%llx blks: 0x%llx ", 
                   __FUNCTION__, rdgen_operation, pvd_object_id, (unsigned long long)start_lba, (unsigned long long)blocks);
        status = fbe_api_rdgen_send_one_io(rdgen_context_p, 
                                           pvd_object_id,       /* Issue to a single pvd */
                                           FBE_CLASS_ID_INVALID,
                                           FBE_PACKAGE_ID_SEP_0,
                                           rdgen_operation,
                                           FBE_RDGEN_PATTERN_LBA_PASS,
                                           start_lba, /* lba */
                                           blocks, /* blocks */
                                           FBE_RDGEN_OPTIONS_STRIPE_ALIGN,
                                           0, 0, /* no expiration or abort time */
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);

        /*! @note PVD Sniff could automatically trigger copy.
         */
        if (status != FBE_STATUS_OK)
        {
            if (rdgen_operation == FBE_RDGEN_OPERATION_VERIFY)
            {
                /* Sniff will trigger EOL.
                */
                start_lba = user_space_start_lba + fbe_test_sep_background_ops_default_offset;
                mut_printf(MUT_LOG_TEST_STATUS, "%s: rdgen op: %d pvd Sniff to lba: 0x%llx blks: 0x%llx triggers copy.", 
                           __FUNCTION__, rdgen_operation, (unsigned long long)start_lba, (unsigned long long)blocks);
                current_rg_config_p++;
                continue;
            }
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Goto the next raid group 
         */
        current_rg_config_p++;
    }

    /* Return the execution status.
     */
    return status;
}
/********************************************************
 * end fbe_test_sep_background_ops_run_io_to_start_copy()
 ********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_get_source_destination_edge_index()
 *****************************************************************************
 *
 * @brief   Based on the virtual drive configuration mode determine the 
 *          `source' (where data is being read from) and the destination (where 
 *          data is being written to) edge index of the virtual drive.
 *
 * @param   vd_object_id - object id of virtual drive performing copy
 * @param   source_edge_index_p - Address of source edge index to populate
 * @param   dest_edge_index_p - Address of destination edge index to populate
 *
 * @return  status
 *
 * @author  
 *  03/09/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_ops_get_source_destination_edge_index(fbe_object_id_t vd_object_id,
                                                                           fbe_edge_index_t *source_edge_index_p,
                                                                           fbe_edge_index_t *dest_edge_index_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t      configuration_mode;

    /* Initialize our local variables.
     */
    *source_edge_index_p = FBE_EDGE_INDEX_INVALID;
    *dest_edge_index_p = FBE_EDGE_INDEX_INVALID;

    /* verify that Proactive spare gets swapped-in and configuration mode gets changed to mirror. */
    fbe_test_sep_util_get_virtual_drive_configuration_mode(vd_object_id, &configuration_mode);
    if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) ||
        (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)       )
    {
        *source_edge_index_p = 0;
        *dest_edge_index_p = 1;
    }
    else if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) ||
             (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)       )
    { 
        *source_edge_index_p = 1;
        *dest_edge_index_p = 0;
    }
    else
    {
        /* Else unsupported configuration mode.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: get source and dest for vd obj: 0x%x mode: %d invalid.", 
                   vd_object_id, configuration_mode);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return status;
}
/*********************************************************************
 * end fbe_test_sep_background_ops_get_source_destination_edge_index()
 *********************************************************************/

/*!***************************************************************************
 * fbe_test_sep_background_ops_get_source_destination_edge_index_based_on_event()
 *****************************************************************************
 *
 * @brief   Based on the virtual drive configuration mode and the `event' 
 *          (a.k.a. state of the copy operation) determine the `source' (where
 *          data is being read from) and the destination (where data is being
 *          written to) edge index of the virtual drive.
 *
 * @param   vd_object_id - object id of virtual drive performing copy
 * @param   event_info_p - Optional pointer to event which lets use know the 
 *              `state' of the copy operation.
 * @param   configuration_mode_p - Pointer to configuraiton mode to update
 * @param   source_edge_index_p - Address of source edge index to populate
 * @param   dest_edge_index_p - Address of destination edge index to populate

 *
 * @return  status
 *
 * @note    The purpose of `event_type' is to give us additional information
 *          to help determine the source and destination edges.  For instance
 *          if the copy is complete the virtual drive will change back to
 *          pass-thru mode which means the original source edge is actually
 *          the `destination' when determining if the copy is complete.
 *
 * @author  
 *  04/12/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_ops_get_source_destination_edge_index_based_on_event(fbe_object_id_t vd_object_id,
                                                                                  fbe_test_sep_background_ops_event_info_t *event_info_p,
                                                                                  fbe_virtual_drive_configuration_mode_t *configuration_mode_p,
                                                                                  fbe_edge_index_t *source_edge_index_p,
                                                                                  fbe_edge_index_t *dest_edge_index_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t      configuration_mode;
    fbe_test_sep_background_ops_event_info_t    local_event;
    fbe_test_sep_background_ops_event_info_t   *local_event_p = &local_event;

    /* Initialize our local variables.
     */
    *source_edge_index_p = FBE_EDGE_INDEX_INVALID;
    *dest_edge_index_p = FBE_EDGE_INDEX_INVALID;
    local_event.event_type = FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_INVALID;
    local_event.event_name = "Event type n/a";

    /* verify that Proactive spare gets swapped-in and configuration mode gets changed to mirror. */
    fbe_test_sep_util_get_virtual_drive_configuration_mode(vd_object_id, &configuration_mode);
    *configuration_mode_p = configuration_mode;
    if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) ||
        (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)       )
    {
        *source_edge_index_p = 0;
        *dest_edge_index_p = 1;
    }
    else if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) ||
             (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)       )
    { 
        *source_edge_index_p = 1;
        *dest_edge_index_p = 0;
    }
    else
    {
        /* Else unsupported configuration mode.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: get source and dest for vd obj: 0x%x mode: %d invalid.", 
                   vd_object_id, configuration_mode);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /*! @note We may need to `flip' the source and destination based on the 
     *        state of the copy operation.
     */
    if (event_info_p != NULL)
    {
        local_event_p = event_info_p;
    }
    switch(local_event_p->event_type)
    {
        case FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_COMPLETE:
        case FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_OUT:
        case FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_OUT_COMPLETE:
            /*! @note If the copy has completed and the source edge has been
             *        swapped-out and the configuration mode has been changed
             *        to pass-thru, we need to `flip' the source and destination.
             */
            if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)  ||
                (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)    )
            {
                /* Print a message for debug purposes.
                 */
                *source_edge_index_p = (*source_edge_index_p == 0) ? 1 : 0;
                *dest_edge_index_p = (*dest_edge_index_p == 0) ? 1 : 0;
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "fbe_test_sep_background_ops: vd obj: 0x%x mode: %d event: %s `flipping' source to: %d dest to: %d", 
                           vd_object_id, configuration_mode, local_event_p->event_name,
                           *source_edge_index_p, *dest_edge_index_p);
            }
            break;

        default:
            /*! @note For all other cases don't modify the edges.
             */
            break;
    }

    return status;
}
/************************************************************************************
 * end fbe_test_sep_background_ops_get_source_destination_edge_index_based_on_event()
 ************************************************************************************/

/*!***************************************************************************
 * fbe_test_sep_background_ops_check_event_log_msg()
 *****************************************************************************
 *
 * @brief   start the user copy operation.
 *
 * @param event_type - Event type to get information for
 *
 * @return Pointer to fbe_test_sep_background_ops_event_info_t
 *****************************************************************************/
void fbe_test_sep_background_ops_check_event_log_msg(fbe_test_rg_configuration_t * rg_config_p,
                                           fbe_u32_t disk_position,fbe_u32_t message_code,
                                           fbe_test_raid_group_disk_set_t *spare_drive_location)
{
    fbe_status_t                    status;
    fbe_bool_t                      is_msg_present = FBE_FALSE;
    fbe_u32_t                       retry_count;
    fbe_u32_t                       max_retries;         


    /* set the max retries in a local variable. */
    max_retries = FBE_TEST_SEP_BACKGROUND_OPS_MAX_RETRIES;

    /*  loop until the number of chunks marked NR is 0 for the drive that we are rebuilding. */
    for (retry_count = 0; retry_count < max_retries; retry_count++)
    {
        /* check that given event message is logged correctly */
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                    &is_msg_present, 
                    message_code,
                    spare_drive_location->bus,spare_drive_location->enclosure,spare_drive_location->slot,
                    rg_config_p->rg_disk_set[disk_position].bus,
                    rg_config_p->rg_disk_set[disk_position].enclosure,
                    rg_config_p->rg_disk_set[disk_position].slot);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (FBE_TRUE == is_msg_present)
        {
            return;
        }

        /*  wait before trying again. */
        fbe_api_sleep(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_LOG_WAIT_TIME);
    }

    
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, is_msg_present, "Event log msg is not present!");
 
    return;
}
/*********************************************************************
 * end fbe_test_sep_background_ops_check_event_log_msg()
 *********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_mark_swapped_in_as_consumed()
 *****************************************************************************
 *
 * @brief   The drive that swapped in is no longer available for sparing.  
 *          Make sure it is removed for the `extra' or sparing pool.                                       
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Total count of raid groups under test this pass.
 * @parma   rg_to_remove_from_p - Pointer to raid group config to remove from.
 * @param   sparing_position - Raid group position that is being spared
 * @param   swapped_in_pvd_object_id - Object id of pvd tht we just consumed
 * @param   source_edge_index - The virtual drive source index
 * @param   dest_edge_index - The virtual drive destination index
 *
 * @return  fbe_status_t 
 *
 * @author
 *  04/06/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_ops_mark_swapped_in_as_consumed(fbe_test_rg_configuration_t *rg_config_p,
                                                                            fbe_u32_t raid_group_count,
                                                                            fbe_test_rg_configuration_t *rg_to_remove_from_p,
                                                                            fbe_u32_t sparing_position,
                                                                            fbe_object_id_t swapped_in_pvd_object_id,
                                                                            fbe_edge_index_t source_edge_index,
                                                                            fbe_edge_index_t dest_edge_index)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_class_id_t                  class_id;
    fbe_api_get_block_edge_info_t   edge_info;
    fbe_object_id_t                 source_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 dest_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_raid_group_disk_set_t  swapped_in_location;
    fbe_api_virtual_drive_get_configuration_t get_configuration;
    fbe_u32_t                       extra_index;
    fbe_u32_t                       extra_drive_located_index = FBE_TEST_SEP_BACKGROUND_OPS_INVALID_DISK_POSITION;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_test_rg_configuration_t    *rg_config_drive_removed_from_p = NULL;
    fbe_u32_t                       rg_index;
    fbe_bool_t                      b_is_automatic_sparing_enabled = FBE_TRUE;

    /* Get the information and validate the parameters
     */
    fbe_api_database_lookup_raid_group_by_number(rg_to_remove_from_p->raid_group_id, &rg_object_id);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, sparing_position, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);
    status = fbe_api_get_object_class_id(vd_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE((status == FBE_STATUS_OK) && (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE));
    status = fbe_api_virtual_drive_get_unused_drive_as_spare_flag(vd_object_id, &b_is_automatic_sparing_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the information about the virtual drive so that we can
     * determine the source and destination pvd information.
     */
    status = fbe_api_virtual_drive_get_configuration(vd_object_id, &get_configuration);        
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    switch(get_configuration.configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            MUT_ASSERT_INT_EQUAL(0, source_edge_index);
            status = fbe_api_get_block_edge_info(vd_object_id, source_edge_index, &edge_info, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            source_pvd_object_id = edge_info.server_id;
            status = fbe_api_get_block_edge_info(vd_object_id, dest_edge_index, &edge_info, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            dest_pvd_object_id = edge_info.server_id;
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            MUT_ASSERT_INT_EQUAL(1, source_edge_index);
            status = fbe_api_get_block_edge_info(vd_object_id, source_edge_index, &edge_info, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            source_pvd_object_id = edge_info.server_id;
            status = fbe_api_get_block_edge_info(vd_object_id, dest_edge_index, &edge_info, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            dest_pvd_object_id = edge_info.server_id;
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
        default:
            /* Unsupported configuration.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid configuration mode: %d",
                       __FUNCTION__, get_configuration.configuration_mode);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }
    
    /* Get and validate the destination (a.k.a swapped-in) pvd
     */
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, dest_pvd_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(0, dest_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(swapped_in_pvd_object_id, dest_pvd_object_id);
    status = fbe_api_provision_drive_get_location(dest_pvd_object_id,
                                                  &swapped_in_location.bus,
                                                  &swapped_in_location.enclosure,
                                                  &swapped_in_location.slot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*! @note The drive could have come from any raid group configuration under
     *        (not just the raid group the received the swapped-in drive).
     *        Therefore we need to walk all the raid groups under test and locate
     *        the `extra' drive that was consumed for the swap-in.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Skip raid groups that are not enabled.
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Walk the `extra' drive array and check if this drive should be removed.
         */
        for (extra_index = 0; extra_index < current_rg_config_p->num_of_extra_drives; extra_index++)
        {
            /* Check if the swapped-in drive came from the extra array.
            */
            if ((current_rg_config_p->extra_disk_set[extra_index].bus == swapped_in_location.bus)             &&
                (current_rg_config_p->extra_disk_set[extra_index].enclosure == swapped_in_location.enclosure) &&
                (current_rg_config_p->extra_disk_set[extra_index].slot == swapped_in_location.slot)              )
            {
                extra_drive_located_index = extra_index;
                rg_config_drive_removed_from_p = current_rg_config_p;
                break;
            }
        }

        /* If we locate the raid groups it was removed from break.
         */
        if (extra_drive_located_index != FBE_TEST_SEP_BACKGROUND_OPS_INVALID_DISK_POSITION)
        {
            break;
        }

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* If we didn't locate the swapped in drive in any of the `extra' array(s)
     * it maybe that `automatic' sparing is enabled and thus we used an automatic
     * spare (`unused') drive.  Validate that automatic sparing is enabled.
     */
    if (extra_drive_located_index == FBE_TEST_SEP_BACKGROUND_OPS_INVALID_DISK_POSITION)
    {
        /* If automatic sparing is enabled simply display the drive
         * information.
         */
        if (b_is_automatic_sparing_enabled == FBE_TRUE)
        {
            /* Display the `automatic' spare that we consumed.
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "fbe_test_sep_background_ops: spare pos: %d consumed `automatic' disk: (%d_%d_%d)",
                       sparing_position,
                       swapped_in_location.bus, swapped_in_location.enclosure, swapped_in_location.slot);
            return FBE_STATUS_OK;
        }

        /* We expect all `consumed' drives to come from the `extra' array.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s rg obj: 0x%x spare pos: %d (%d_%d_%d) didn't come from `extra' !",
                   __FUNCTION__, rg_object_id, sparing_position,
                   swapped_in_location.bus, swapped_in_location.enclosure, swapped_in_location.slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* If the raid group config that the drive came from isn't the one that
     * consumed it, swap the drive locations so that it is in the raid group
     * `extra' array that consumed it.
     */
    if (rg_config_drive_removed_from_p != rg_to_remove_from_p)
    {
        /* Simply swap the first extra drive.
         */
        MUT_ASSERT_TRUE(rg_to_remove_from_p->num_of_extra_drives > 0);
        if ((rg_to_remove_from_p->extra_disk_set[0].bus == 0)       &&
            (rg_to_remove_from_p->extra_disk_set[0].enclosure == 0) &&
            (rg_to_remove_from_p->extra_disk_set[0].slot == 0)         )
        {
            /* Something went wrong
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s rg obj: 0x%x spare pos: %d (%d_%d_%d) no `extra' left !",
                       __FUNCTION__, rg_object_id, sparing_position,
                       swapped_in_location.bus, swapped_in_location.enclosure, swapped_in_location.slot);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
        
        /* Swap the drive locations. We know the location consumed.  First
         * update the raid groups that it actually came from with the one it
         * should have come from.  Then update the one it should have come from.
         */
        rg_config_drive_removed_from_p->extra_disk_set[extra_drive_located_index] = rg_to_remove_from_p->extra_disk_set[0];
        rg_to_remove_from_p->extra_disk_set[0] = swapped_in_location;
    }

    /* Walk the `extra' drive array and check if this drive should be removed.
     */
    extra_drive_located_index = FBE_TEST_SEP_BACKGROUND_OPS_INVALID_DISK_POSITION;
    for (extra_index = 0; extra_index < rg_to_remove_from_p->num_of_extra_drives; extra_index++)
    {
        /* Check if the swapped-in drive came from the extra array.
         */
        if ((rg_to_remove_from_p->extra_disk_set[extra_index].bus == swapped_in_location.bus)             &&
            (rg_to_remove_from_p->extra_disk_set[extra_index].enclosure == swapped_in_location.enclosure) &&
            (rg_to_remove_from_p->extra_disk_set[extra_index].slot == swapped_in_location.slot)              )
        {
            extra_drive_located_index = extra_index;
            break;
        }
    }

    /* If we found one we need to shift the `extra' array accordingly.
     */
    if (extra_drive_located_index != FBE_TEST_SEP_BACKGROUND_OPS_INVALID_DISK_POSITION)
    {
        /* Walk all the extra drives.
         */
        for (extra_index = 0; extra_index < rg_to_remove_from_p->num_of_extra_drives; extra_index++)
        {
            /* If the index is greater than or eqaul we need to do something.
             */
            if (extra_index >= extra_drive_located_index)
            {
                /* If it's not the last copy the next.
                 */
                if (extra_index < (rg_to_remove_from_p->num_of_extra_drives - 1))
                {
                    /* Copy the next to here.
                     */
                    rg_to_remove_from_p->extra_disk_set[extra_index] = rg_to_remove_from_p->extra_disk_set[extra_index + 1];
                }
                else
                {
                    /* Else set the last to `unused' (0_0_0).
                     */
                    rg_to_remove_from_p->extra_disk_set[extra_index].bus = 0;
                    rg_to_remove_from_p->extra_disk_set[extra_index].enclosure = 0;
                    rg_to_remove_from_p->extra_disk_set[extra_index].slot = 0;
                    /*! @note Don't modify the number of extra drives required.
                     *        This field is the number of extra disk required not
                     *        the number of extra disk available.
                     */
                    //rg_to_remove_from_p->num_of_extra_drives--;
                    break;
                }
            }

        } /* for all extra drives*/

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: spare pos: %d consumed `extra' disk index: %d (%d_%d_%d)",
                   sparing_position, extra_drive_located_index, 
                   swapped_in_location.bus, swapped_in_location.enclosure, swapped_in_location.slot);

    } /* if we consumed an extra drive */
    else
    {
        /* We expect the swapped-in drive to come form one of the `extra'
         * arrays.
         */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s rg obj: 0x%x spare pos: %d (%d_%d_%d) did not come from `extra' !",
                       __FUNCTION__, rg_object_id, sparing_position,
                       swapped_in_location.bus, swapped_in_location.enclosure, swapped_in_location.slot);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
    }

    /* Return the status
     */
    return status;
}
/***************************************************************
 * end fbe_test_sep_background_ops_mark_swapped_in_as_consumed()
 ***************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_save_source_info()
 *****************************************************************************
 *
 * @brief   Save the `original' source drive information so that we can
 *          determine the original drive information if we need to
 *          `resurrect' the drive from End Of Life.                                       
 *
 * @param   current_rg_config_p - Current raid group config
 * @param   sparing_position - Raid group position that is being spared
 * @param   source_edge_index - The virtual drive source index
 * @param   dest_edge_index - The virtual drive destination index
 *
 * @return  fbe_status_t 
 *
 * @author
 *  05/09/2012  - Ron Proulx    - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_ops_save_source_info(fbe_test_rg_configuration_t *current_rg_config_p,
                                                          fbe_u32_t sparing_position,
                                                          fbe_edge_index_t source_edge_index,
                                                          fbe_edge_index_t dest_edge_index)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_class_id_t                  class_id;
    fbe_api_get_block_edge_info_t   edge_info;
    fbe_object_id_t                 source_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 dest_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_raid_group_disk_set_t  source_pvd_location;

    /* Get the information and validate the parameters
     */
    fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, sparing_position, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);
    status = fbe_api_get_object_class_id(vd_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE((status == FBE_STATUS_OK) && (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE));

    /* Get the source and destination pvd information.
     */
    status = fbe_api_get_block_edge_info(vd_object_id, source_edge_index, &edge_info, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    source_pvd_object_id = edge_info.server_id;
    status = fbe_api_provision_drive_get_location(source_pvd_object_id,
                                                  &source_pvd_location.bus,
                                                  &source_pvd_location.enclosure,
                                                  &source_pvd_location.slot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_get_block_edge_info(vd_object_id, dest_edge_index, &edge_info, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    dest_pvd_object_id = edge_info.server_id;

    /* Validate the `source' isn't already in use.
     */
    if ((current_rg_config_p->source_disk.bus != 0)       ||
        (current_rg_config_p->source_disk.enclosure != 0) ||
        (current_rg_config_p->source_disk.slot != 0)         )
    {
        /* Source disk is already in use.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: spare pos: %d config source disk:(%d_%d_%d) in already in-use!",
                   sparing_position,
                   current_rg_config_p->source_disk.bus, current_rg_config_p->source_disk.enclosure, current_rg_config_p->source_disk.slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }
    current_rg_config_p->source_disk.bus = source_pvd_location.bus;
    current_rg_config_p->source_disk.enclosure = source_pvd_location.enclosure;
    current_rg_config_p->source_disk.slot = source_pvd_location.slot;

    /* Display some information
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "fbe_test_sep_background_ops: spare pos: %d saved source: 0x%x(%d_%d_%d) in `source_disk'.",
               sparing_position, source_pvd_object_id,
               source_pvd_location.bus, source_pvd_location.enclosure, source_pvd_location.slot);

    /* Return the status
     */
    return status;
}
/***************************************************************
 * end fbe_test_sep_background_ops_save_source_info()
 ***************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_set_rebuild_hook()
 *****************************************************************************
 *
 * @brief   To guarantee that we run I/O during the copy operation and that the
 *          copy operation actually occurs.  We set a debug hook so that when
 *          the copy operation starts to rebuild.                                   
 *
 * @param   current_rg_config_p - Current raid group config
 * @param   rg_index - Raid group array index
 * @param   sparing_position - Raid group position that is being spared
 *
 * @return  fbe_status_t 
 *
 * @author
 *  06/26/2012  - Ron Proulx    - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_ops_set_rebuild_hook(fbe_test_rg_configuration_t *current_rg_config_p,
                                                                 fbe_u32_t rg_index,
                                                                 fbe_u32_t sparing_position)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_class_id_t                  class_id;

    /* Get the information and validate the parameters
     */
    fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, sparing_position, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);
    status = fbe_api_get_object_class_id(vd_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE((status == FBE_STATUS_OK) && (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE));

    /* Set the rebuild hook.
     */
    status = fbe_test_sep_background_pause_set_debug_hook(&fbe_test_sep_background_ops_rebuild_delay.scheduler_hooks[rg_index], rg_index,
                                                    vd_object_id,
                                                    SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                    FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                    0, 0, 
                                                    SCHEDULER_CHECK_STATES,
                                                    SCHEDULER_DEBUG_ACTION_PAUSE,
                                                    FBE_FALSE /* This is not a reboot test */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Return the status
     */
    return status;
}
/***************************************************************
 * end fbe_test_sep_background_ops_set_rebuild_hook()
 ***************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_copy_cleanup_disks()
 *****************************************************************************
 *
 * @brief   Execute and disk cleanup required when a copy operation completes.                                      
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Total count of raid groups under test this pass.
 * @param   sparing_position - Raid group position that is being spared
 * @param   b_clear_eol - FBE_TRUE - Clear End Life on source disk
 *                        FBE_FALSE - No need to clear EOL on source
 *
 * @return  fbe_status_t 
 *
 * @note    If `b_clear_eol' is FBE_TRUE EOL must be set otherwise the request
 *          will fail.
 *
 * @author
 *  05/10/2012  - Ron Proulx    - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_ops_copy_cleanup_disks(fbe_test_rg_configuration_t *rg_config_p,
                                                            fbe_u32_t raid_group_count,
                                                            fbe_u32_t sparing_position,
                                                            fbe_bool_t b_clear_eol)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t                   rg_index;

    /* Step 1: Re-insert the drives that swapped out (due to EOL).  First loop 
     *         thru and clear the End Of Life (EOL) for any drives that were 
     *         forced into proactive copy source.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        
        /* Validate that the source disk is a valid disk.
         */
        if ((current_rg_config_p->source_disk.bus == 0)       &&
            (current_rg_config_p->source_disk.enclosure == 0) &&
            (current_rg_config_p->source_disk.slot == 0)         )
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s rg_index: %d rg id: %d source_disk: %d_%d_%d not valid.",
                       __FUNCTION__, rg_index, current_rg_config_p->raid_group_id,
                       current_rg_config_p->source_disk.bus, current_rg_config_p->source_disk.enclosure, current_rg_config_p->source_disk.slot);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }

        /* If required (i.e. proactive copy), call method to clear
         * EOL in the drive.  Eventually all upstream edges should have the
         * EOL flag cleared.
         */
        if (b_clear_eol == FBE_TRUE)
        {
            status = fbe_test_sep_util_clear_disk_eol(current_rg_config_p->source_disk.bus,
                                                      current_rg_config_p->source_disk.enclosure,
                                                      current_rg_config_p->source_disk.slot);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            if (status != FBE_STATUS_OK)
            {
                return status;
            }
        }

        /* Now clear the `source disk' information in the test configuration.
         */
        current_rg_config_p->source_disk.bus = 0;
        current_rg_config_p->source_disk.enclosure = 0;
        current_rg_config_p->source_disk.slot = 0;

        /* Goto the next raid group
         */
        current_rg_config_p++;
    }

    /* Return the status.
     */
    return status;
}
/***************************************************************
 * end fbe_test_sep_background_ops_copy_cleanup_disks()
 ***************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_wait_for_event()
 *****************************************************************************
 *
 * @brief   Wait (or timeout) for the event specified.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Total number of raid groups in the array
 * @param   sparing_position - Raid group position that is being spared
 * @param   event_info_p - Pointer to event information to wait for
 * @param   original_source_edge_index - Source edge of virtual drive to copy from
 * @param   original_dest_edge_index - Destination edge of virtual drive to copy to
 *
 * @return  fbe_status_t 
 *
 * @author
 *  03/15/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_ops_wait_for_event(fbe_test_rg_configuration_t *rg_config_p, 
                                                        fbe_u32_t raid_group_count,
                                                        fbe_u32_t sparing_position,
                                                        fbe_test_sep_background_ops_event_info_t *event_info_p,
                                                        fbe_edge_index_t original_source_edge_index,
                                                        fbe_edge_index_t original_dest_edge_index)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_lifecycle_state_t           vd_lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_object_id_t                 source_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 dest_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 original_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 active_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_raid_group_disk_set_t  active_location;
    fbe_u32_t                       retry_count;
    fbe_u32_t                       max_retries = FBE_TEST_SEP_BACKGROUND_OPS_MAX_SHORT_RETRIES;
    fbe_bool_t                      raid_groups_complete[FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT];
    fbe_u32_t                       num_raid_groups_complete = 0;
    fbe_test_raid_group_disk_set_t *drive_to_spare_p = NULL; 
    fbe_api_get_block_edge_info_t   source_edge_info;
    fbe_api_get_block_edge_info_t   dest_edge_info;
    fbe_virtual_drive_configuration_mode_t configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;
    fbe_virtual_drive_configuration_mode_t expected_configuration_mode;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_class_id_t                  class_id;
    fbe_api_raid_group_get_info_t   raid_group_info;
    fbe_api_virtual_drive_get_info_t virtual_drive_info;
    fbe_lifecycle_state_t           reinsert_pvd_lifecycle_state = FBE_LIFECYCLE_STATE_NOT_EXIST;
    fbe_edge_index_t                source_edge_index = original_source_edge_index;
    fbe_edge_index_t                dest_edge_index = original_dest_edge_index;
    fbe_bool_t                      b_is_debug_hook_reached = FBE_FALSE;

    /* Initialize complete array.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        raid_groups_complete[rg_index] = FBE_FALSE;
    }

    /*  Loop until we either see all the proactive copies start or timeout
     */
    for (retry_count = 0; (retry_count < max_retries) && (num_raid_groups_complete < raid_group_count); retry_count++)
    {
        /* For all raid groups simply check */
        current_rg_config_p = rg_config_p;
        for (rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            /* Skip raid groups that have completed the event.
             */
            if (raid_groups_complete[rg_index] == FBE_TRUE)
            {
                current_rg_config_p++;
                continue;
            }

            /* Skip raid groups that are not enabled.
             */
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                raid_groups_complete[rg_index] = FBE_TRUE;
                num_raid_groups_complete++;
                current_rg_config_p++;
                continue;
            }

            /* Validate sparing position.
             */
            if ((sparing_position == FBE_TEST_SEP_BACKGROUND_OPS_INVALID_DISK_POSITION) ||
                (sparing_position >= current_rg_config_p->width)                       )
            {
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, "%s Event: %d(%s) invalid sparing pos: %d",
                           __FUNCTION__, event_info_p->event_type, event_info_p->event_name, sparing_position);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status; 
            }

            /* Get the vd object id of the position to force into proactive copy
             */
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[sparing_position];
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, sparing_position, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);
            status = fbe_api_get_object_class_id(vd_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_TRUE((status == FBE_STATUS_OK) && (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE));
            status = fbe_api_get_object_lifecycle_state(vd_object_id, &vd_lifecycle_state, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Need to update the source and destination based on configuration 
             * and the event we are waiting for.
             */
            status = fbe_test_sep_background_ops_get_source_destination_edge_index_based_on_event(vd_object_id, 
                                                                                         event_info_p, /* Need event */
                                                                                         &configuration_mode,
                                                                                         &source_edge_index, 
                                                                                         &dest_edge_index);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
            MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

            /* Get the information about the virtual drive so that we can
             * determine the source and destination pvd information.
             */
            switch(configuration_mode)
            {
                case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
                    /* Determine the `active' information based on the source
                     * index.
                     */
                    if (source_edge_index == 0)
                    {
                        /* If the source edge is 0 it means we haven't started
                         * the copy operation and therefore the `active' edge
                         * is the source.
                         */
                        status = fbe_api_get_block_edge_info(vd_object_id, source_edge_index, &source_edge_info, FBE_PACKAGE_ID_SEP_0);
                        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                        source_pvd_object_id = source_edge_info.server_id;
                        active_pvd_object_id = source_pvd_object_id;
                    }
                    else
                    {
                        /* Else if the destination is 0 it means we have 
                         * completed the copy operation and we will re-insert 
                         * the `old' drive.
                         */
                        status = fbe_api_get_block_edge_info(vd_object_id, dest_edge_index, &dest_edge_info, FBE_PACKAGE_ID_SEP_0);
                        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                        dest_pvd_object_id = dest_edge_info.server_id;
                        active_pvd_object_id = dest_pvd_object_id;
                    }
                    break;

                case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
                    /* Determine the `active' information based on the source
                     * index.
                     */
                    if (source_edge_index == 1)
                    {
                        /* If the source edge is 1 it means we haven't started
                         * the copy operation and therefore the `active' edge
                         * is the source.
                         */
                        status = fbe_api_get_block_edge_info(vd_object_id, source_edge_index, &source_edge_info, FBE_PACKAGE_ID_SEP_0);
                        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                        source_pvd_object_id = source_edge_info.server_id;
                        active_pvd_object_id = source_pvd_object_id;
                    }
                    else
                    {
                        /* Else if the destination is 1 it means we have 
                         * completed the copy operation and we will re-insert 
                         * the `old' drive.
                         */
                        status = fbe_api_get_block_edge_info(vd_object_id, dest_edge_index, &dest_edge_info, FBE_PACKAGE_ID_SEP_0);
                        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                        dest_pvd_object_id = dest_edge_info.server_id;
                        active_pvd_object_id = dest_pvd_object_id;
                    }
                    break;

                case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
                    MUT_ASSERT_INT_EQUAL(0, source_edge_index);
                    status = fbe_api_get_block_edge_info(vd_object_id, source_edge_index, &source_edge_info, FBE_PACKAGE_ID_SEP_0);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    source_pvd_object_id = source_edge_info.server_id;
                    status = fbe_api_get_block_edge_info(vd_object_id, dest_edge_index, &dest_edge_info, FBE_PACKAGE_ID_SEP_0);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    dest_pvd_object_id = dest_edge_info.server_id;
                    active_pvd_object_id = dest_pvd_object_id;
                    break;

                case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
                    MUT_ASSERT_INT_EQUAL(1, source_edge_index);
                    status = fbe_api_get_block_edge_info(vd_object_id, source_edge_index, &source_edge_info, FBE_PACKAGE_ID_SEP_0);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    source_pvd_object_id = source_edge_info.server_id;
                    status = fbe_api_get_block_edge_info(vd_object_id, dest_edge_index, &dest_edge_info, FBE_PACKAGE_ID_SEP_0);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    dest_pvd_object_id = dest_edge_info.server_id;
                    active_pvd_object_id = dest_pvd_object_id;
                    break;

                default:
                    /* Unsupported configuration.
                     */
                    status = FBE_STATUS_GENERIC_FAILURE;
                    mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid configuration mode: %d",
                               __FUNCTION__, configuration_mode);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    break;
            }
            
            /* Get and validate the `active' edge (if the virtual drive is in
             * Ready state)
             */
            if (vd_lifecycle_state == FBE_LIFECYCLE_STATE_READY)
            {
                MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, active_pvd_object_id);
                MUT_ASSERT_INT_NOT_EQUAL(0, active_pvd_object_id);
                status = fbe_api_provision_drive_get_location(active_pvd_object_id,
                                                              &active_location.bus,
                                                              &active_location.enclosure,
                                                              &active_location.slot);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }

            /* Based on the event to wait for get the information and check it.
             */
            switch(event_info_p->event_type)
            {
                case FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SOURCE_EDGE_EOL:
                    /* Wait a short period for the eol bit gets set for the pdo 
                     * and it gets propogated to virtual drive object. 
                     */
                    status = fbe_api_get_block_edge_info(vd_object_id, source_edge_index, &source_edge_info, FBE_PACKAGE_ID_SEP_0);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    if (source_edge_info.path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE)
                    {
                        mut_printf(MUT_LOG_TEST_STATUS, "fbe_test_sep_background_ops: EOL bit is set on the edge between vd (0x%x) and pvd (0x%x).",
                                   vd_object_id, source_edge_info.server_id);
                        raid_groups_complete[rg_index] = FBE_TRUE;
                        num_raid_groups_complete++;
                    }
                    break;

                case FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_IN:
                    /* Wait for the virtual drive destination edge to be enabled.
                     */
                    status = fbe_api_get_block_edge_info(vd_object_id, dest_edge_index, &dest_edge_info, FBE_PACKAGE_ID_SEP_0);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            
                    /* If the destination edge is enabled the swap-in is complete.
                     */
                    if (dest_edge_info.path_state == FBE_PATH_STATE_ENABLED)
                    {
                        /* Display some information
                         */
                        mut_printf(MUT_LOG_TEST_STATUS, "fbe_test_sep_background_ops: Swap in edge index: %d for vd: 0x%x complete.",
                                   dest_edge_index, vd_object_id);
                        raid_groups_complete[rg_index] = FBE_TRUE;
                        num_raid_groups_complete++;
                    }
                    break;

                case FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_MIRROR_MODE:
                    /* Wait for the configuration to be mirror mode.
                     */
                    if (source_edge_index == 0)
                    {
                        expected_configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE;
                    }
                    else
                    {
                        expected_configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE;
                    }
            
                    /* If we are in mirror mode then we are complete.
                     */
                    if (configuration_mode == expected_configuration_mode)
                    {
                        /* The drive that swapped in is no longer available for 
                         * sparing.  Make sure it is removed for the `extra'
                         * or sparing pool.
                         */
                        status = fbe_test_sep_background_ops_mark_swapped_in_as_consumed(rg_config_p,
                                                                                         raid_group_count,
                                                                                         current_rg_config_p,
                                                                                         sparing_position,
                                                                                         dest_pvd_object_id,
                                                                                         source_edge_index,
                                                                                         dest_edge_index);
                        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                        mut_printf(MUT_LOG_TEST_STATUS, 
                                   "fbe_test_sep_background_ops: Mirror mode on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                                   rg_object_id, sparing_position, vd_object_id,
                                   drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);
                        raid_groups_complete[rg_index] = FBE_TRUE;
                        num_raid_groups_complete++;
                    }
                    break;

                case FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_METADATA_REBUILD_START:
                    /* Wait for rebuild logging to be cleared for the destination
                     * position.
                     */
                    status = fbe_api_raid_group_get_info(vd_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                    /* Get the expected configuration mode
                     */
                    if (source_edge_index == 0)
                    {
                        expected_configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE;
                    }
                    else
                    {
                        expected_configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE;
                    }
            
                    /* If we are in mirror mode and rebuild logging for the 
                     * destination is clear then the swap-in occurred.
                     */
                    if ((configuration_mode == expected_configuration_mode)          &&
                        (raid_group_info.b_rb_logging[dest_edge_index] == FBE_FALSE)    )
                    {
                        /* Validate that the destination edge is marked NR.
                         */
                        fbe_test_sep_rebuild_utils_check_bits_set_for_position(vd_object_id, dest_edge_index);

                        mut_printf(MUT_LOG_TEST_STATUS, 
                                   "fbe_test_sep_background_ops: Metadata rebuild start on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                                   rg_object_id, sparing_position, vd_object_id,
                                   drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);
                        raid_groups_complete[rg_index] = FBE_TRUE;
                        num_raid_groups_complete++;
                    }
                    break;

                case FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_REBUILD_HOOK:
                    /* Wait for the rebuild hook (for a short period).
                     */
                    status = fbe_test_sep_background_pause_check_debug_hook(&fbe_test_sep_background_ops_rebuild_delay.scheduler_hooks[rg_index],
                                                                            &b_is_debug_hook_reached);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                    /* If we have reached the debug hook log a message.
                     */
                    if (b_is_debug_hook_reached == FBE_TRUE)
                    {
                        /* Flag the debug hook reached.
                         */
                        mut_printf(MUT_LOG_TEST_STATUS, 
                           "fbe_test_sep_background_ops: Hit debug hook raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                           rg_object_id, sparing_position, vd_object_id,
                           drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);
                        raid_groups_complete[rg_index] = FBE_TRUE;
                        num_raid_groups_complete++;

                        /* Immediately remove the debug hook.
                         */
                        status = fbe_test_sep_background_pause_remove_debug_hook(&fbe_test_sep_background_ops_rebuild_delay.scheduler_hooks[rg_index], rg_index,
                                                                                 FBE_FALSE /* This is not a reboot test*/);
                        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    }
                    break;

                case FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_START:
                    /* Wait for the copy checkpoint to not be 0.
                     */
                    status = fbe_api_raid_group_get_info(vd_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                    /* If the rebuild checkpoint is set to the logical marker 
                     * for the given disk, then the rebuild has finished.
                     * Display message and remove from the `spared' arrays.
                     */
                    if (raid_group_info.rebuild_checkpoint[dest_edge_index] != 0)
                    {
                        /* Display some information
                         */
                        mut_printf(MUT_LOG_TEST_STATUS, 
                                   "fbe_test_sep_background_ops: Copy start on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                                   rg_object_id, sparing_position, vd_object_id,
                                   drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);
                        raid_groups_complete[rg_index] = FBE_TRUE;
                        num_raid_groups_complete++;
                    }
                    break;

                case FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_COMPLETE:
                    /* Wait for all chunks on the virtual drive to be rebuilt.
                     */
                    status = fbe_api_virtual_drive_get_info(vd_object_id, &virtual_drive_info);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                    /* If the rebuild checkpoint is set to the logical marker 
                     * for the given disk, then the rebuild has finished.
                     * Display message and remove from the `spared' arrays.
                     */
                    if ((virtual_drive_info.b_is_copy_complete == FBE_TRUE)   ||
                        (virtual_drive_info.vd_checkpoint == FBE_LBA_INVALID)    )
                    {
                        /* Display some information
                         */
                        mut_printf(MUT_LOG_TEST_STATUS, 
                                   "fbe_test_sep_background_ops: Copy complete on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                                   rg_object_id, sparing_position, vd_object_id,
                                   drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);
                        raid_groups_complete[rg_index] = FBE_TRUE;
                        num_raid_groups_complete++;
                    }
                    break;

                case FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_COMPLETE_INITIATED:
                    /* Wait for the initiate copy complete to start (i.e. job-in-progress)
                     */
                    status = fbe_api_virtual_drive_get_info(vd_object_id, &virtual_drive_info);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                    /* If the rebuild checkpoint is set to the logical marker 
                     * for the given disk, then the rebuild has finished.
                     * Display message and remove from the `spared' arrays.
                     */
                    if ((virtual_drive_info.b_is_copy_complete == FBE_TRUE)    &&
                        (virtual_drive_info.b_request_in_progress == FBE_TRUE)    )
                    {
                        /* Display some information
                         */
                        mut_printf(MUT_LOG_TEST_STATUS, 
                                   "fbe_test_sep_background_ops: Initiate copy complete on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                                   rg_object_id, sparing_position, vd_object_id,
                                   drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);
                        raid_groups_complete[rg_index] = FBE_TRUE;
                        num_raid_groups_complete++;
                    }
                    break;

                case FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_OUT:
                    /* Wait for the source pvd to swap out of the virtual drive.
                     */
                    if (source_edge_index == 0)
                    {
                        expected_configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE;
                    }
                    else
                    {
                        expected_configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE;
                    }
            
                    /* If we find the configuration mode gets updated as asked 
                     * by user then increment the number of passed raid groups.
                     */
                    if ((configuration_mode == expected_configuration_mode) ||
                        (vd_lifecycle_state != FBE_LIFECYCLE_STATE_READY)      )
                    {
                        /* Remove the drive from the `needing spare' array.
                         */
                        fbe_test_sep_util_delete_needing_spare_position(current_rg_config_p, sparing_position);

                        mut_printf(MUT_LOG_TEST_STATUS, "fbe_test_sep_background_ops: Swap out edge index: %d for vd: 0x%x complete.",
                                   source_edge_index, vd_object_id);
                        raid_groups_complete[rg_index] = FBE_TRUE;
                        num_raid_groups_complete++;
                    }
                    break;

                case FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_OUT_COMPLETE:
                    /* Wait for the swap out job to complete.
                     */
                    status = fbe_api_virtual_drive_get_info(vd_object_id, &virtual_drive_info);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    if (source_edge_index == 0)
                    {
                        expected_configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE;
                    }
                    else
                    {
                        expected_configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE;
                    }
            
                    /* Wait for the configuration mode to be that expected and
                     * wait for the job to be complete.
                     */
                    if ((configuration_mode == expected_configuration_mode)     &&
                        (virtual_drive_info.b_request_in_progress == FBE_FALSE)    )
                    {
                        /* Remove the drive from the `needing spare' array.
                         */
                        fbe_test_sep_util_delete_needing_spare_position(current_rg_config_p, sparing_position);

                        mut_printf(MUT_LOG_TEST_STATUS, "fbe_test_sep_background_ops: Swap out job edge index: %d for vd: 0x%x complete.",
                                   source_edge_index, vd_object_id);
                        raid_groups_complete[rg_index] = FBE_TRUE;
                        num_raid_groups_complete++;
                    }
                    break;

                case FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SOURCE_MARKED_NR:
                    /* Wait for the source position of the virtual drive to have
                     * the rebuild checkpoint set to the end marker.
                     */
                    status = fbe_api_raid_group_get_info(vd_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            
                    /* If the rebuild checkpoint is set to the logical marker 
                     * for the given disk, then the parent `mark NR' is complete.
                     */
                    if (raid_group_info.rebuild_checkpoint[source_edge_index] == FBE_LBA_INVALID)
                    {
                        /* Display some information
                         */
                        mut_printf(MUT_LOG_TEST_STATUS, 
                                   "fbe_test_sep_background_ops: Mark NR for degraded source complete on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                                   rg_object_id, sparing_position, vd_object_id,
                                   drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);
                        raid_groups_complete[rg_index] = FBE_TRUE;
                        num_raid_groups_complete++;
                    }
                    break;

                case FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_REINSERT_FAILED_DRIVES:
                    /* The expected configuration mode is pass-thru first or
                     * second edge.
                     */
                    if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)  ||
                        (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)    )
                    {
                        /* If the EOL has been cleared in the associated pvd 
                         * the event has occurred.  Since the pvd has been removed
                         * from the virtual drive we must use the location to get
                         * it object id.
                         */
                        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[sparing_position].bus,
                                                                                rg_config_p->rg_disk_set[sparing_position].enclosure,
                                                                                rg_config_p->rg_disk_set[sparing_position].slot,
                                                                                &original_pvd_object_id);
                        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                        status = fbe_api_provision_drive_get_info(original_pvd_object_id, &provision_drive_info);
                        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                        status = fbe_api_get_object_lifecycle_state(original_pvd_object_id, &reinsert_pvd_lifecycle_state, FBE_PACKAGE_ID_SEP_0);
                        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                        /* If EOL is cleared and the lifecycle state is ready
                         * we can remove the drive from the removed array.
                         */
                        if ((provision_drive_info.end_of_life_state == FBE_FALSE)       &&
                            (reinsert_pvd_lifecycle_state == FBE_LIFECYCLE_STATE_READY)    )
                        {
                            /* Mark this raid group as complete.
                             */
                            raid_groups_complete[rg_index] = FBE_TRUE;
                            num_raid_groups_complete++;

                            /* Display some information
                             */
                            mut_printf(MUT_LOG_TEST_STATUS, 
                                   "fbe_test_sep_background_ops: Re-insert swapped on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                                   rg_object_id, sparing_position, vd_object_id,
                                   drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);

                            /* Invoke the method to update the test configuration for this position.
                             * Use the `active' pvd id.
                             */
                            MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, active_pvd_object_id);
                            status = fbe_test_sep_util_update_position_info(current_rg_config_p, sparing_position,
                                                                            drive_to_spare_p, &active_location);
                            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                        } /* end if EOL cleared*/

                    } /* end if proper configuration mode*/
                    break;

                default:
                    /* Unexpected event type.
                     */
                    status = FBE_STATUS_GENERIC_FAILURE;
                    mut_printf(MUT_LOG_TEST_STATUS, "fbe_test_sep_background_ops: Unexpected event type: %d",
                               event_info_p->event_type);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    return status;

            } /* end switch on event type */


            /* Goto next raid group
             */                
            current_rg_config_p++;

        } /* end for all raid groups */

        /* If not done sleep for a short period
         */
        if (num_raid_groups_complete < raid_group_count)
        {
            fbe_api_sleep(FBE_TEST_SEP_BACKGROUND_OPS_SHORT_RETRY_TIME);
        }

    } /* end while not complete or retries exhausted */

    /* Check if we are not complete.
     */
    if ((retry_count >= max_retries)                  || 
        (num_raid_groups_complete < raid_group_count)    )
    {
        /* Go thru all the raid groups and report the ones that timed out.
         */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            /* Skip raid groups that are not enabled.
             */
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                current_rg_config_p++;
                continue;
            }

            /* Skip raid groups that have completed the event.
             */
            if (raid_groups_complete[rg_index] == FBE_TRUE)
            {
                current_rg_config_p++;
                continue;
            }

            /* Using the position supplied, get the virtual drive information.
             */
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[sparing_position];
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, sparing_position, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

            /* Display some information
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "fbe_test_sep_background_ops: Timed out %s raid: 0x%x pos: %d vd: 0x%x (%d_%d_%d)",
                       event_info_p->event_name, rg_object_id, sparing_position, vd_object_id,
                       drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);

            /* Goto the next raid group
             */
            current_rg_config_p++;

        } /* For all raid groups */

        /* Print a message then fail.
         */
        status = FBE_STATUS_TIMEOUT;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: %d raid groups out of: %d took longer than: %d seconds to complete event: %d",
                   (raid_group_count - num_raid_groups_complete), raid_group_count, 
                   ((FBE_TEST_SEP_BACKGROUND_OPS_MAX_SHORT_RETRIES * FBE_TEST_SEP_BACKGROUND_OPS_SHORT_RETRY_TIME) / 1000),
                   event_info_p->event_type);

        /* Generate an error trace on the SP and fail the test.
         */
        fbe_test_sep_util_raid_group_generate_error_trace(rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Return the execution status.
     */
    return status;
}
/************************************************
 * end fbe_test_sep_background_ops_wait_for_event()
 ************************************************/

/*!**************************************************************
 * fbe_test_sep_background_ops_proactive_spare_drives()
 ****************************************************************
 * @brief
 *  Forces drives into proactive sparing, one per raid group.
 *
 * @param rg_config_p - Array of raid group information  
 * @param raid_group_count - Number of valid raid groups in array 
 * @param position_to_spare - raid group position to start proactive
 * @param scsi_opcode_to_inject_on - The SCSI opcode to inject for
 * @param b_start_io - FBE_TRUE - Start I/O to force the error
 *
 * @return fbe_status_t 
 *
 * @author
 *  03/14/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_test_sep_background_ops_proactive_spare_drives(fbe_test_rg_configuration_t *rg_config_p,
                                                      fbe_u32_t raid_group_count,
                                                      fbe_u32_t position_to_spare,
                                                      fbe_u8_t scsi_opcode_to_inject_on,
                                                      fbe_bool_t b_start_io)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index = 0;
    fbe_test_raid_group_disk_set_t *drive_to_spare_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_protocol_error_injection_error_record_t error_record[FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT];
    fbe_protocol_error_injection_record_handle_t record_handle_p[FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT];
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Forcing proactive sparing not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Validate that there aren't too many raid groups
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* For every raid group remove one drive.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* We need to add the drive to the `needing spare' position primarily for
         * debug purposes.
         */
        fbe_test_sep_util_add_needing_spare_position(current_rg_config_p, position_to_spare);
        drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_spare];

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_spare, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /*! @note Get the edge index of the source and destination drive based 
         *        on configuration mode.
         *
         *  @note This assumes that all raid groups have the same source and 
         *        destination index. 
         */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Save the source information.
         */
        status = fbe_test_sep_background_ops_save_source_info(current_rg_config_p, position_to_spare,
                                                              source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* If we have too many raid groups under test set debug hook.
         */
        if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUPS_WITHOUT_DELAY)
        {
            status = fbe_test_sep_background_ops_set_rebuild_hook(current_rg_config_p, rg_index, position_to_spare);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: Start proactive on raid: 0x%x pos: %d vd obj: 0x%x(%d_%d_%d) source: %d dest: %d",
                   rg_object_id, position_to_spare, vd_object_id,
                   drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot,
                   source_edge_index, dest_edge_index);

        /* Populate the protocol error injection record.
         */
        status = fbe_test_sep_background_ops_populate_io_error_injection_record(current_rg_config_p,
                                                                      position_to_spare,
                                                                      scsi_opcode_to_inject_on,
                                                                      &error_record[rg_index],
                                                                      &record_handle_p[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Stop the job service recovery queue to hold the job service swap 
         * commands. 
         */
        fbe_test_sep_util_disable_recovery_queue(vd_object_id);

        /* Now allow automatic spare selection for this virtual drive object.
         */
        status = fbe_api_virtual_drive_set_unused_drive_as_spare_flag(vd_object_id, FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* inject the scsi errors. */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: start error injection.", __FUNCTION__);
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*! @todo DE542 - Due to the fact that protocol error are injected on the
     *                way to the disk, the data is never sent to or read from
     *                the disk.  Therefore until this defect is fixed b_start_io
     *                MUST be True.
     */
    if (fbe_test_sep_background_ops_b_is_DE542_fixed == FBE_FALSE)
    {
        if (b_start_io == FBE_FALSE)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Due to DE542 b_start_io: %d MUST be True !", 
                       __FUNCTION__, b_start_io);
        }
        MUT_ASSERT_TRUE(b_start_io == FBE_TRUE);
    }

    /* If requested start I/O to trigger the smart error.
     */
    if (b_start_io == FBE_TRUE)
    {
        /* Start the background pattern to all the PVDs to trigger the 
         * errors. 
         */
        fbe_test_sep_background_ops_run_io_to_start_copy(rg_config_p,
                                                         raid_group_count,
                                                         position_to_spare,
                                                         scsi_opcode_to_inject_on, 
                                                         FBE_TEST_SEP_BACKGROUND_OPS_ELEMENT_SIZE);
    }

    /* Step 1: Validate that PVD for the edge selected goes EOL
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for source edge EOL", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SOURCE_EDGE_EOL);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_spare,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Source edge EOL Complete.", __FUNCTION__);

    /* stop injecting scsi errors. */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: stop error injection.", __FUNCTION__);
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	status  = fbe_api_protocol_error_injection_remove_all_records();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: stop error injection complete.", __FUNCTION__);
	
    /* Step 2: Restart the job service queue to proceed with job service 
     * swap command. 
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Fetch the drive to copy information from the position passed.
         */
        drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_spare];

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_spare, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Display some infomation.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: Enable recovery raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_spare, vd_object_id,
                   drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);

        /* Restart the job service queue to proceed with job service 
         * swap command. 
         */
        fbe_test_sep_util_enable_recovery_queue(vd_object_id);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Step 3: Wait for the proactive spare to swap-in. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for Swap In", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_IN);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_spare,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Swap In Complete.", __FUNCTION__);

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_test_sep_background_ops_proactive_spare_drives()
 **********************************************************/

/*!**************************************************************
 * fbe_test_sep_background_ops_wait_for_proactive_copy_complete()
 ****************************************************************
 * @brief
 *  Wait for proactive copy to complete for all raid groups.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   copy_position - Position in raid group the is being copied
 * @param   b_wait_for_swap_out - FBE_TRUE - Wait for the source pvd to be swapped 
 *              out of the virtual drive.  This value cannot be set to True
 *              unless b_skip_copy_operation_cleanup is also True.
 * @param   b_skip_copy_operation_cleanup - FBE_TRUE - Skip the automatic cleanup
 *              (re-insert drives marked EOL etc).  The only reason this would be
 *              set is to verify that drives are marked EOL.
 *
 * @return fbe_status_t 
 *
 * @author
 *  03/14/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_test_sep_background_ops_wait_for_proactive_copy_complete(fbe_test_rg_configuration_t *rg_config_p, 
                                                                          fbe_u32_t raid_group_count,
                                                                          fbe_u32_t position_to_complete,
                                                                          fbe_bool_t b_wait_for_swap_out,
                                                                          fbe_bool_t b_skip_copy_operation_cleanup)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index = 0;
    fbe_test_raid_group_disk_set_t *drive_to_complete_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Proactive sparing not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that there aren't too many raid groups
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Validate the use of `wait for swap out' and `skip copy cleanup'.
     */
    if ((b_wait_for_swap_out == FBE_FALSE)           &&
        (b_skip_copy_operation_cleanup == FBE_FALSE)    )
    {
        /* Invalid combination.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "%s Wait for swap: %d and skip cleanup: %d cannot both be False.", 
                   __FUNCTION__, b_wait_for_swap_out, b_skip_copy_operation_cleanup);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* First loop is simply to print a message
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* The position to complete is supplied.
         */
        drive_to_complete_p = &current_rg_config_p->rg_disk_set[position_to_complete];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_complete, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Get the edge index of the source and destination drive based on configuration mode. */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: Wait for proactive sparing complete on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_complete, vd_object_id,
                   drive_to_complete_p->bus, drive_to_complete_p->enclosure, drive_to_complete_p->slot);

        /* Goto the next raid group
         */
        current_rg_config_p++;
    }

    /* Step 1: If required wait for rebuild debug hook.
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUPS_WITHOUT_DELAY)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for rebuild hook", __FUNCTION__);
        event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_REBUILD_HOOK);
        status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_complete,
                                          event_info_p, source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for rebuild hook - complete.", __FUNCTION__);
    }

    /* Step 2: Wait for all chunks on the virtual drive to be rebuilt.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for virtual drive copy complete", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_COMPLETE);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_complete,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Virtual drive copy complete.", __FUNCTION__);

    /* Step 3: If requested, wait for source to be swapped out of virtual drive
     */
    if (b_wait_for_swap_out == FBE_TRUE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for source to be swapped out", __FUNCTION__);
        event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_OUT);
        status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_complete,
                                                            event_info_p, source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Source swap out complete.", __FUNCTION__);
    }

    /* Step 4: Unless `skip cleanup' is True, re-insert the drives that
     *          swapped out (due to EOL).
     */
    if (b_skip_copy_operation_cleanup == FBE_FALSE)
    {
        /* First loop thru and clear the End Of Life (EOL) for any drives
         * that were forced into proactive copy source.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Copy Cleanup disks", __FUNCTION__);
        status = fbe_test_sep_background_ops_copy_cleanup_disks(rg_config_p, raid_group_count, position_to_complete,
                                                            FBE_TRUE /* Clear EOL*/);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Copy Cleanup disks complete.", __FUNCTION__);

        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for swapped-out drive to be re-inserted", __FUNCTION__);
        event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_REINSERT_FAILED_DRIVES);
        status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_complete,
                                                  event_info_p, source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Swapped-out drives re-inserted", __FUNCTION__);

        /* In all cases we need to `refresh' the actual drives being used in each
         * of the raid group test configurations.
         */
        status = fbe_test_sep_background_ops_refresh_raid_group_disk_info(rg_config_p, raid_group_count);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Return the execution status.
     */
    return status;
}
/********************************************************************
 * end fbe_test_sep_background_ops_wait_for_proactive_copy_complete()
 ********************************************************************/

/*!***************************************************************************
 * fbe_test_sep_background_ops_wait_for_proactive_copy_cleanup()
 *****************************************************************************
 * @brief
 *  Wait for proactive copy to `cleanup'.  Wait for proactive source drives
 *  (currently marked End Of Life) to come back.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_cleanup - Position that needs cleanup
 * @param   b_is_cleanup_required - FBE_FALSE - Cleanup was performed already
 *              simply return.
 *                                  FBE_TRUE - Cleanup needs to be performed.
 *
 * @return fbe_status_t 
 *
 * @author
 *  03/28/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_ops_wait_for_proactive_copy_cleanup(fbe_test_rg_configuration_t *rg_config_p, 
                                                                                fbe_u32_t raid_group_count,
                                                                                fbe_u32_t position_to_cleanup,
                                                                                fbe_bool_t b_is_cleanup_required)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index = 0;
    fbe_test_raid_group_disk_set_t *drive_to_complete_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;
    fbe_virtual_drive_configuration_mode_t configuration_mode;

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Proactive sparing not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that there aren't too many raid groups
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Check if cleanup has already been performed.
     */
    if (b_is_cleanup_required == FBE_FALSE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Cleanup already performed.", 
                   __FUNCTION__);
        return FBE_STATUS_OK;
    }

    /* First loop is simply to print a message
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* The position to cleanup is supplied.
         */
        drive_to_complete_p = &current_rg_config_p->rg_disk_set[position_to_cleanup];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_cleanup, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /*! @note Get the edge index of the source and destination drive based
         *        on configuration mode.
         * 
         *  @note Since this is the `cleanup' method, we assume the copy
         *        operation has been started and thus the `swap-in' has occurred.
         *        Therefore at a minimum we assume the `source' edge was the 
         *        original `destination' edge and that we are waiting for the 
         *        original `source' edge to be swapped-out.
         */
        fbe_test_sep_util_get_virtual_drive_configuration_mode(vd_object_id, &configuration_mode);
        switch(configuration_mode)
        {
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
                /* The `wait for event' method expects the edge index as it
                 * was when the copy operation started.  Therefore the source
                 * index is 0 when the current configuration mode is second edge.
                 */
                source_edge_index = 0;
                dest_edge_index = 1;
                break;
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
                /* The `wait for event' method expects the edge index as it
                 * was when the copy operation started.  Therefore the source
                 * index is 1 when the current configuration mode is first edge.
                 */
                source_edge_index = 1;
                dest_edge_index = 0;
                break;
            default:
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "%s Unexpected vd obj: 0x%x config mode: %d ",
                           __FUNCTION__, vd_object_id, configuration_mode);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
                break;
        }

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: Wait for proactive cleanup on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_cleanup, vd_object_id,
                   drive_to_complete_p->bus, drive_to_complete_p->enclosure, drive_to_complete_p->slot);

        /* Goto the next raid group
         */
        current_rg_config_p++;
    }

    /* Step 1:  Cleanup wasn't performed, wait for swap-out.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for source to be swapped out", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_OUT);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_cleanup,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Source swap out complete.", __FUNCTION__);

    /* Step 2: Re-insert the drives that swapped out (due to EOL).  First loop 
     *         thru and clear the End Of Life (EOL) for any drives that were 
     *         forced into proactive copy source.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Copy Cleanup disks", __FUNCTION__);
    status = fbe_test_sep_background_ops_copy_cleanup_disks(rg_config_p, raid_group_count, position_to_cleanup,
                                                            FBE_TRUE /* Clear EOL*/);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Copy Cleanup disks complete.", __FUNCTION__);

    /* Step 3: Wait for failed drives to come back.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for swapped-out drive to be re-inserted", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_REINSERT_FAILED_DRIVES);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_cleanup,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Swapped-out drives re-inserted", __FUNCTION__);
    
    /* Return the execution status.
     */
    return status;
}
/********************************************************************
 * end fbe_test_sep_background_ops_wait_for_proactive_copy_cleanup()
 ********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_start_user_copy_to()
 *****************************************************************************
 *
 * @brief   Initiates a user copy operation.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - The position in the raid group to copy from
 * @param   dest_array_p - Destination pvd object id for user copy operations
 *
 * @return  fbe_status_t 
 *
 * @author
 *  04/04/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_ops_start_user_copy_to(fbe_test_rg_configuration_t *rg_config_p,
                                                                fbe_u32_t raid_group_count,
                                                                fbe_u32_t position_to_copy,
                                                                fbe_test_raid_group_disk_set_t *dest_array_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index = 0;
    fbe_test_raid_group_disk_set_t *drive_to_copy_p = NULL;
    fbe_test_raid_group_disk_set_t *dest_drive_p = dest_array_p;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 source_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 dest_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;
    fbe_provision_drive_copy_to_status_t copy_status;

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Forcing proactive sparing not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Validate that there aren't too many raid groups
     */
    if (dest_array_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: dest_array_p is NULL. Must supply destination for copy.", 
                   __FUNCTION__);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Validate the destination array is not NULL
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Step 1: Issue the user copy request (recovery queue is enabled).
     */
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* We need to add the drive to the `needing spare' position primarily for
         * debug purposes.
         */
        fbe_test_sep_util_add_needing_spare_position(current_rg_config_p, position_to_copy);
        drive_to_copy_p = &current_rg_config_p->rg_disk_set[position_to_copy];

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /*! @note Get the edge index of the source and destination drive based 
         *        on configuration mode.
         *
         *  @note This assumes that all raid groups have the same source and 
         *        destination index. 
         */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Save the source information.
         */
        status = fbe_test_sep_background_ops_save_source_info(current_rg_config_p, position_to_copy,
                                                              source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* If we have too many raid groups under test set debug hook.
         */
        if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUPS_WITHOUT_DELAY)
        {
            status = fbe_test_sep_background_ops_set_rebuild_hook(current_rg_config_p, rg_index, position_to_copy);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: Start user copy on raid: 0x%x pos: %d vd obj: 0x%x(%d_%d_%d) source: %d dest: %d(%d_%d_%d)",
                   rg_object_id, position_to_copy, vd_object_id,
                   drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot,
                   source_edge_index, dest_edge_index,
                   dest_drive_p->bus, dest_drive_p->enclosure, dest_drive_p->slot);

        /*  Get and validate the source and destination pvd object ids.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot,
                                                                &source_pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(0, source_pvd_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, source_pvd_object_id);
        status = fbe_api_provision_drive_get_obj_id_by_location(dest_drive_p->bus, dest_drive_p->enclosure, dest_drive_p->slot,
                                                                &dest_pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(0, dest_pvd_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, dest_pvd_object_id);

        /* Now allow automatic spare selection for this virtual drive object.
         */
        status = fbe_api_virtual_drive_set_unused_drive_as_spare_flag(vd_object_id, FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now issue the user copy request.
         */
        status = fbe_api_copy_to_replacement_disk(source_pvd_object_id, dest_pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR, copy_status);

        /* Goto next raid group and destination drive.
         */
        current_rg_config_p++;
        dest_drive_p++;
    }

    /* Step 2: Wait for the spare to swap-in. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for Swap In", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_IN);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_copy,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Swap In Complete.", __FUNCTION__);

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_test_sep_background_ops_start_user_copy_to()
 **********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_wait_for_user_copy_to_complete()
 *****************************************************************************
 *
 * @brief   Wait for user copy to complete for all raid groups.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_complete - Position in raid group the is being copied
 * @param   b_wait_for_swap_out - FBE_TRUE - Wait for the source pvd to be swapped 
 *              out of the virtual drive.  This value cannot be set to True
 *              unless b_skip_copy_operation_cleanup is also True.
 * @param   b_skip_copy_operation_cleanup - FBE_TRUE - Skip the automatic cleanup
 *              (re-insert drives marked EOL etc).  The only reason this would be
 *              set is to verify that drives are marked EOL.
 *
 * @return fbe_status_t 
 *
 * @author
 *  04/04/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_test_sep_background_ops_wait_for_user_copy_to_complete(fbe_test_rg_configuration_t *rg_config_p, 
                                                                            fbe_u32_t raid_group_count,
                                                                            fbe_u32_t position_to_complete,
                                                                            fbe_bool_t b_wait_for_swap_out,
                                                                            fbe_bool_t b_skip_copy_operation_cleanup)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index = 0;
    fbe_test_raid_group_disk_set_t *drive_to_complete_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Proactive sparing not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that there aren't too many raid groups
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Validate the use of `wait for swap out' and `skip copy cleanup'.
     */
    if ((b_wait_for_swap_out == FBE_FALSE)           &&
        (b_skip_copy_operation_cleanup == FBE_FALSE)    )
    {
        /* Invalid combination.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "%s Wait for swap: %d and skip cleanup: %d cannot both be False.", 
                   __FUNCTION__, b_wait_for_swap_out, b_skip_copy_operation_cleanup);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* First loop is simply to print a message
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* The position to complete is supplied.
         */
        drive_to_complete_p = &current_rg_config_p->rg_disk_set[position_to_complete];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_complete, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Get the edge index of the source and destination drive based on configuration mode. */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: Wait for user copy complete on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_complete, vd_object_id,
                   drive_to_complete_p->bus, drive_to_complete_p->enclosure, drive_to_complete_p->slot);

        /* Goto the next raid group
         */
        current_rg_config_p++;
    }

    /* Step 1: If required wait for rebuild debug hook.
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUPS_WITHOUT_DELAY)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for rebuild hook", __FUNCTION__);
        event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_REBUILD_HOOK);
        status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_complete,
                                          event_info_p, source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for rebuild hook - complete.", __FUNCTION__);
    }

    /* Step 2: Wait for all chunks on the virtual drive to be rebuilt.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for virtual drive copy complete", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_COMPLETE);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_complete,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Virtual drive copy complete.", __FUNCTION__);

    /* Step 3: If requested, wait for source to be swapped out of virtual drive
     */
    if (b_wait_for_swap_out == FBE_TRUE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for source to be swapped out", __FUNCTION__);
        event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_OUT_COMPLETE);
        status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_complete,
                                                  event_info_p, source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Source swap out complete.", __FUNCTION__);
    }

    /* Step 4: If cleanup is requested refresh the raid groups.
     */
    if (b_skip_copy_operation_cleanup == FBE_FALSE)
    {
        /* Cleanup any disk changes.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Copy Cleanup disks", __FUNCTION__);
        status = fbe_test_sep_background_ops_copy_cleanup_disks(rg_config_p, raid_group_count, position_to_complete,
                                                                FBE_FALSE /* No need to clear EOL. */);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Copy Cleanup disks complete.", __FUNCTION__);

        /* In all cases we need to `refresh' the actual drives being used in each
         * of the raid group test configurations.
         */
        status = fbe_test_sep_background_ops_refresh_raid_group_disk_info(rg_config_p, raid_group_count);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Return the execution status.
     */
    return status;
}
/********************************************************************
 * end fbe_test_sep_background_ops_wait_for_user_copy_to_complete()
 ********************************************************************/

/*!***************************************************************************
 * fbe_test_sep_background_ops_wait_for_user_copy_to_cleanup()
 *****************************************************************************
 * @brief
 *  Wait for user copy to `cleanup'.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_cleanup - Position to cleanup if required
 * @param   b_is_cleanup_required - FBE_FALSE - Cleanup was performed already
 *              simply return.
 *                                  FBE_TRUE - Cleanup needs to be performed.
 *
 * @return fbe_status_t 
 *
 * @author
 *  03/28/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_ops_wait_for_user_copy_to_cleanup(fbe_test_rg_configuration_t *rg_config_p, 
                                                                           fbe_u32_t raid_group_count,
                                                                           fbe_u32_t position_to_cleanup,
                                                                           fbe_bool_t b_is_cleanup_required)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index = 0;
    fbe_test_raid_group_disk_set_t *drive_to_complete_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;
    fbe_virtual_drive_configuration_mode_t configuration_mode;

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Proactive sparing not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that there aren't too many raid groups
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Check if cleanup has already been performed.
     */
    if (b_is_cleanup_required == FBE_FALSE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Cleanup already performed.", 
                   __FUNCTION__);
        return FBE_STATUS_OK;
    }

    /* First loop is simply to print a message
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* The position to cleanup is now explicitly passed.
         */
        drive_to_complete_p = &current_rg_config_p->rg_disk_set[position_to_cleanup];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_cleanup, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /*! @note Get the edge index of the source and destination drive based
         *        on configuration mode.
         * 
         *  @note Since this is the `cleanup' method, we assume the copy
         *        operation has been started and thus the `swap-in' has occurred.
         *        Therefore at a minimum we assume the `source' edge was the 
         *        original `destination' edge and that we are waiting for the 
         *        original `source' edge to be swapped-out.
         */
        fbe_test_sep_util_get_virtual_drive_configuration_mode(vd_object_id, &configuration_mode);
        switch(configuration_mode)
        {
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
                /* The `wait for event' method expects the edge index as it
                 * was when the copy operation started.  Therefore the source
                 * index is 0 when the current configuration mode is second edge.
                 */
                source_edge_index = 0;
                dest_edge_index = 1;
                break;
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
                /* The `wait for event' method expects the edge index as it
                 * was when the copy operation started.  Therefore the source
                 * index is 1 when the current configuration mode is first edge.
                 */
                source_edge_index = 1;
                dest_edge_index = 0;
                break;
            default:
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "%s Unexpected vd obj: 0x%x config mode: %d ",
                           __FUNCTION__, vd_object_id, configuration_mode);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
                break;
        }

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: Wait for user copy cleanup on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_cleanup, vd_object_id,
                   drive_to_complete_p->bus, drive_to_complete_p->enclosure, drive_to_complete_p->slot);

        /* Goto the next raid group
         */
        current_rg_config_p++;
    }

    /* Step 1:  Cleanup wasn't performed, wait for swap-out to complete.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for source to be swapped out", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_OUT_COMPLETE);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_cleanup,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Source swap out complete.", __FUNCTION__);
    
    /* Step 2: Perform any disk cleanup (such as clearing the `source disk'
     *         information).
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Copy Cleanup disks", __FUNCTION__);
    status = fbe_test_sep_background_ops_copy_cleanup_disks(rg_config_p, raid_group_count, position_to_cleanup,
                                                            FBE_FALSE /* No need to clear EOL. */);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Copy Cleanup disks complete.", __FUNCTION__);

    /* Return the execution status.
     */
    return status;
}
/********************************************************************
 * end fbe_test_sep_background_ops_wait_for_user_copy_to_cleanup()
 ********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_start_user_copy()
 *****************************************************************************
 *
 * @brief   Initiates a user copy operation.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - The position in the raid group to copy from
 *
 * @return  fbe_status_t 
 *
 * @author
 *  04/09/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_ops_start_user_copy(fbe_test_rg_configuration_t *rg_config_p,
                                                                          fbe_u32_t raid_group_count,
                                                                          fbe_u32_t position_to_copy)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index = 0;
    fbe_test_raid_group_disk_set_t *drive_to_copy_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 source_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;
    fbe_provision_drive_copy_to_status_t copy_status;

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Forcing proactive sparing not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Validate the destination array is not NULL
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Step 1: Issue the user copy request (recovery queue is enabled).
     */
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* We need to add the drive to the `needing spare' position primarily for
         * debug purposes.
         */
        fbe_test_sep_util_add_needing_spare_position(current_rg_config_p, position_to_copy);
        drive_to_copy_p = &current_rg_config_p->rg_disk_set[position_to_copy];

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /*! @note Get the edge index of the source and destination drive based 
         *        on configuration mode.
         *
         *  @note This assumes that all raid groups have the same source and 
         *        destination index. 
         */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Save the source information.
         */
        status = fbe_test_sep_background_ops_save_source_info(current_rg_config_p, position_to_copy,
                                                              source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* If we have too many raid groups under test set debug hook.
         */
        if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUPS_WITHOUT_DELAY)
        {
            status = fbe_test_sep_background_ops_set_rebuild_hook(current_rg_config_p, rg_index, position_to_copy);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: Start user copy on raid: 0x%x pos: %d vd obj: 0x%x(%d_%d_%d) source: %d dest: %d(n/a)",
                   rg_object_id, position_to_copy, vd_object_id,
                   drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot,
                   source_edge_index, dest_edge_index);

        /*  Get and validate the source and destination pvd object ids.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot,
                                                                &source_pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(0, source_pvd_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, source_pvd_object_id);

        /* Now allow automatic spare selection for this virtual drive object.
         */
        status = fbe_api_virtual_drive_set_unused_drive_as_spare_flag(vd_object_id, FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*! @note Now issue the proactive user copy request.  This API has
         *        no timeout so there must be spares available.
         */
        status = fbe_api_provision_drive_user_copy(source_pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR, copy_status);

        /* Goto next raid group and destination drive.
         */
        current_rg_config_p++;
    }

    /* Step 2: Wait for the spare to swap-in. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for Swap In", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_IN);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_copy,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Swap In Complete.", __FUNCTION__);

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/*************************************************************
 * end fbe_test_sep_background_ops_start_user_copy()
 *************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_wait_for_user_copy_complete()
 *****************************************************************************
 *
 * @brief   Wait for user copy to complete for all raid groups.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_complete - Position in raid group the is being copied
 * @param   b_wait_for_swap_out - FBE_TRUE - Wait for the source pvd to be swapped 
 *              out of the virtual drive.  This value cannot be set to True
 *              unless b_skip_copy_operation_cleanup is also True.
 * @param   b_skip_copy_operation_cleanup - FBE_TRUE - Skip the automatic cleanup
 *              (re-insert drives marked EOL etc).  The only reason this would be
 *              set is to verify that drives are marked EOL.
 *
 * @return fbe_status_t 
 *
 * @author
 *  04/04/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_test_sep_background_ops_wait_for_user_copy_complete(fbe_test_rg_configuration_t *rg_config_p, 
                                                                            fbe_u32_t raid_group_count,
                                                                            fbe_u32_t position_to_complete,
                                                                            fbe_bool_t b_wait_for_swap_out,
                                                                            fbe_bool_t b_skip_copy_operation_cleanup)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index = 0;
    fbe_test_raid_group_disk_set_t *drive_to_complete_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Proactive sparing not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that there aren't too many raid groups
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Validate the use of `wait for swap out' and `skip copy cleanup'.
     */
    if ((b_wait_for_swap_out == FBE_FALSE)           &&
        (b_skip_copy_operation_cleanup == FBE_FALSE)    )
    {
        /* Invalid combination.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "%s Wait for swap: %d and skip cleanup: %d cannot both be False.", 
                   __FUNCTION__, b_wait_for_swap_out, b_skip_copy_operation_cleanup);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* First loop is simply to print a message
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* The position to complete is supplied.
         */
        drive_to_complete_p = &current_rg_config_p->rg_disk_set[position_to_complete];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_complete, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Get the edge index of the source and destination drive based on configuration mode. */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: Wait for user copy complete on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_complete, vd_object_id,
                   drive_to_complete_p->bus, drive_to_complete_p->enclosure, drive_to_complete_p->slot);

        /* Goto the next raid group
         */
        current_rg_config_p++;
    }

    /* Step 1: If required wait for rebuild debug hook.
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUPS_WITHOUT_DELAY)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for rebuild hook", __FUNCTION__);
        event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_REBUILD_HOOK);
        status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_complete,
                                          event_info_p, source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for rebuild hook - complete.", __FUNCTION__);
    }

    /* Step 2: Wait for all chunks on the virtual drive to be rebuilt.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for virtual drive copy complete", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_COMPLETE);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_complete,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Virtual drive copy complete.", __FUNCTION__);

    /* Step 3: If requested, wait for source to be swapped out of virtual drive
     */
    if (b_wait_for_swap_out == FBE_TRUE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for source to be swapped out", __FUNCTION__);
        event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_OUT);
        status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_complete,
                                                  event_info_p, source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Source swap out complete.", __FUNCTION__);
    }

    /* Step 4: If cleanup is requested refresh the raid groups.
     */
    if (b_skip_copy_operation_cleanup == FBE_FALSE)
    {
        /* Cleanup any disk changes.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Copy Cleanup disks", __FUNCTION__);
        status = fbe_test_sep_background_ops_copy_cleanup_disks(rg_config_p, raid_group_count, position_to_complete,
                                                                FBE_FALSE /* No need to clear EOL. */);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Copy Cleanup disks complete.", __FUNCTION__);

        /* In all cases we need to `refresh' the actual drives being used in each
         * of the raid group test configurations.
         */
        status = fbe_test_sep_background_ops_refresh_raid_group_disk_info(rg_config_p, raid_group_count);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Return the execution status.
     */
    return status;
}
/********************************************************************
 * end fbe_test_sep_background_ops_wait_for_user_copy_complete()
 ********************************************************************/

/*!***************************************************************************
 * fbe_test_sep_background_ops_wait_for_user_copy_cleanup()
 *****************************************************************************
 * @brief
 *  Wait for user copy to `cleanup'.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_cleanup - Position to cleanup if required
 * @param   b_is_cleanup_required - FBE_FALSE - Cleanup was performed already
 *              simply return.
 *                                  FBE_TRUE - Cleanup needs to be performed.
 *
 * @return fbe_status_t 
 *
 * @author
 *  03/28/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_ops_wait_for_user_copy_cleanup(fbe_test_rg_configuration_t *rg_config_p, 
                                                                           fbe_u32_t raid_group_count,
                                                                           fbe_u32_t position_to_cleanup,
                                                                           fbe_bool_t b_is_cleanup_required)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index = 0;
    fbe_test_raid_group_disk_set_t *drive_to_complete_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;
    fbe_virtual_drive_configuration_mode_t configuration_mode;

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Proactive sparing not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that there aren't too many raid groups
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Check if cleanup has already been performed.
     */
    if (b_is_cleanup_required == FBE_FALSE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Cleanup already performed.", 
                   __FUNCTION__);
        return FBE_STATUS_OK;
    }

    /* First loop is simply to print a message
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* The position to cleanup is now explicitly passed.
         */
        drive_to_complete_p = &current_rg_config_p->rg_disk_set[position_to_cleanup];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_cleanup, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /*! @note Get the edge index of the source and destination drive based
         *        on configuration mode.
         * 
         *  @note Since this is the `cleanup' method, we assume the copy
         *        operation has been started and thus the `swap-in' has occurred.
         *        Therefore at a minimum we assume the `source' edge was the 
         *        original `destination' edge and that we are waiting for the 
         *        original `source' edge to be swapped-out.
         */
        fbe_test_sep_util_get_virtual_drive_configuration_mode(vd_object_id, &configuration_mode);
        switch(configuration_mode)
        {
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
                /* The `wait for event' method expects the edge index as it
                 * was when the copy operation started.  Therefore the source
                 * index is 0 when the current configuration mode is second edge.
                 */
                source_edge_index = 0;
                dest_edge_index = 1;
                break;
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
                /* The `wait for event' method expects the edge index as it
                 * was when the copy operation started.  Therefore the source
                 * index is 1 when the current configuration mode is first edge.
                 */
                source_edge_index = 1;
                dest_edge_index = 0;
                break;
            default:
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "%s Unexpected vd obj: 0x%x config mode: %d ",
                           __FUNCTION__, vd_object_id, configuration_mode);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
                break;
        }

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: User proactive cleanup on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_cleanup, vd_object_id,
                   drive_to_complete_p->bus, drive_to_complete_p->enclosure, drive_to_complete_p->slot);

        /* Goto the next raid group
         */
        current_rg_config_p++;
    }

    /* Step 1:  Cleanup wasn't performed, wait for swap-out.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for source to be swapped out", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_OUT);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_cleanup,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Source swap out complete.", __FUNCTION__);
    
    /* Step 2: Perform any disk cleanup (such as clearing the `source disk'
     *         information).
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Copy Cleanup disks", __FUNCTION__);
    status = fbe_test_sep_background_ops_copy_cleanup_disks(rg_config_p, raid_group_count, position_to_cleanup,
                                                            FBE_FALSE /* No need to clear EOL. */);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Copy Cleanup disks complete.", __FUNCTION__);

    /* Return the execution status.
     */
    return status;
}
/********************************************************************
 * end fbe_test_sep_background_ops_wait_for_user_copy_cleanup()
 ********************************************************************
 
/*!***************************************************************************
 *          fbe_test_sep_should_background_ops_run()
 *****************************************************************************
 *
 * @brief   Determine based on the following if background operations should be
 *          run or not for this run od rdgen.
 *              o If background operations have already run for this sequence 
 *                don't run them again.
 *              o If background operations are not enabled for this sequence type
 *                (for instance block opertions) do not run them.
 *              o If it doesn't make sense to run them for the rdgen operations
 *                requested (for instance write-only for a prefill) don't run them. 
 *
 * @param   sequence_config_p - Pointer to sequence configuration 
 * @param   context_p - Pointer to the rdgen context so that we can determine
 *              if it makes sense to run the background operation or not.
 *
 * @return  bool - FBE_TRUE - Ok to inject on this sequence type / rdgen operation.
 *                 FBE_FALSE - Doesn't make any sense to run the background operation.
 *
 * @author
 *  03/22/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_bool_t fbe_test_sep_should_background_ops_run(fbe_test_sep_io_sequence_config_t  *sequence_config_p,
                                                         fbe_api_rdgen_context_t *context_p)
{
    fbe_test_sep_io_sequence_type_t sequence_type = sequence_config_p->sequence_type;
    fbe_rdgen_operation_t           rdgen_operation = context_p->start_io.specification.operation;
  
    /*! If we have already determine that background operations should not run
     *  return False
     */
    if (sequence_config_p->b_should_background_ops_run == FBE_FALSE)
    {
        return FBE_FALSE;
    }

    /*! @note Currently we only run background operations (1) time per sequence!!!
     */
    if (sequence_config_p->b_have_background_ops_run == FBE_TRUE)
    {
        return FBE_FALSE;
    }
  
    /* Based on the sequence type (and possibly the rdgen operation code)
     * determine if it makes sense to run the background operation or not.
     */
    switch(sequence_type)
    {
        case FBE_TEST_SEP_IO_SEQUENCE_TYPE_TEST_BLOCK_OPERATIONS:
            /* Does not make sense.  Don't bother printing a message. */
            sequence_config_p->b_should_background_ops_run = FBE_FALSE;
            break;

        case FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD:
        case FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_PREFILL:
        case FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_ABORTS:
        case FBE_TEST_SEP_IO_SEQUENCE_TYPE_CATERPILLAR:
            /* Don't inject on the prefill */
            switch(rdgen_operation)
            {
                case FBE_RDGEN_OPERATION_WRITE_ONLY:
                    /* We want to allow `glitch' drive during writes.
                     */
                    if (sequence_config_p->background_ops & FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_DRIVE_GLITCH)
                    {
                        mut_printf(MUT_LOG_TEST_STATUS, "fbe_test_sep_background_ops: Allow ops: 0x%x for sequence type: %d rdgen op: %d",
                               sequence_config_p->background_ops, sequence_type, context_p->start_io.specification.operation);
                        break;
                    }
                    // Fall thru
                case FBE_RDGEN_OPERATION_ZERO_ONLY:
                case FBE_RDGEN_OPERATION_WRITE_SAME:
                case FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK:
                case FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY:
                case FBE_RDGEN_OPERATION_CORRUPT_DATA_READ_CHECK:
                    /* Don't run on these rdgen operations. */
                    mut_printf(MUT_LOG_TEST_STATUS, "fbe_test_sep_background_ops: Skipping ops: 0x%x for sequence type: %d rdgen op: %d",
                               sequence_config_p->background_ops, sequence_type, context_p->start_io.specification.operation);
                    sequence_config_p->b_should_background_ops_run = FBE_FALSE;
                    break;
                default:
                    /* We run on all these.*/
                    break;
            }
            break;

        default:
            /* Unsupported return false*/
            mut_printf(MUT_LOG_TEST_STATUS, "fbe_test_sep_background_ops: Skipping ops: 0x%x for unsupported sequence type: %d ",
                       sequence_config_p->background_ops, sequence_type);
            sequence_config_p->b_should_background_ops_run = FBE_FALSE;
            break;
    }

    /* Return if we should run the background operation or not.
     */
    return sequence_config_p->b_should_background_ops_run;
}
/**********************************************
 * end fbe_test_sep_should_background_ops_run()
 **********************************************/

/*!***************************************************************************
 *          fbe_test_sep_determine_scsi_op_to_inject_on()
 *****************************************************************************
 *
 * @brief   Determine the SCSI opcode that we should inject the error for.
 *
 * @param   sequence_config_p - Pointer to sequence configuration 
 * @param   background_ops - The background operations to start 
 * @param   context_p - Used to determine what the SCSI operation code is that
 *              we should inject for (when the background operation requires
 *              that we inject error for it to initiate).
 *
 * @return  FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  03/22/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_determine_scsi_op_to_inject_on(fbe_api_rdgen_context_t *context_p,
                                                                fbe_u8_t *scsi_op_to_inject_on_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_rdgen_operation_t   rdgen_operation = FBE_RDGEN_OPERATION_INVALID;

    /* Get the rdgen operation from the context.
     */
    rdgen_operation = context_p->start_io.specification.operation;

    /* Based on the rdgen operation determine the SCSI opcode that
     * we inject on.
     */
    switch (rdgen_operation)
    {
        case FBE_RDGEN_OPERATION_READ_ONLY:
            /* If read inject on reads.*/
            *scsi_op_to_inject_on_p = FBE_SCSI_READ_16;
            break;
        case FBE_RDGEN_OPERATION_VERIFY:
        case FBE_RDGEN_OPERATION_READ_ONLY_VERIFY:
            /* An rdgen verify is done at the raid group.  Therefore inject
             * on reads. */
            *scsi_op_to_inject_on_p = FBE_SCSI_READ_16;
            break;
        case FBE_RDGEN_OPERATION_WRITE_ONLY:
            /* If write only inject on writes. */
            *scsi_op_to_inject_on_p = FBE_SCSI_WRITE_16;
            break;
        case FBE_RDGEN_OPERATION_ZERO_ONLY:
            /* if zero inject on write same */
            *scsi_op_to_inject_on_p = FBE_SCSI_WRITE_SAME_16;
            break;
        case FBE_RDGEN_OPERATION_WRITE_VERIFY:
            /* Raid does allow a write-verify.  Since the drive executes the
             * verify inject on the write-verify request. */
            *scsi_op_to_inject_on_p = FBE_SCSI_WRITE_VERIFY_16;
            break;
        case FBE_RDGEN_OPERATION_READ_CHECK:
        case FBE_RDGEN_OPERATION_WRITE_READ_CHECK:
        case FBE_RDGEN_OPERATION_WRITE_SAME_READ_CHECK:
        case FBE_RDGEN_OPERATION_WRITE_VERIFY_READ_CHECK:
        case FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK:
        case FBE_RDGEN_OPERATION_ZERO_READ_CHECK:
        case FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK:
        case FBE_RDGEN_OPERATION_CORRUPT_DATA_READ_CHECK:
            /* For all read check operations inject on the read.*/
            *scsi_op_to_inject_on_p = FBE_SCSI_READ_16;
            break;
        default:
            /* If we are here it means there is no way to inject and shouldn't
             * have attempted to run the requested background operation.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Unsupported rdgen operation: 0x%x",
                       __FUNCTION__, rdgen_operation);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
    }

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/***************************************************
 * end fbe_test_sep_determine_scsi_op_to_inject_on()
 ***************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_cleanup()
 *****************************************************************************
 *
 * @brief   First enable `automatic' sparing.  Then move any unused `extra'
 *          drives from the `reserved as spare' pool back to the `unconsumed.'
 *
 * @param   None
 *
 * @return  FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  04/24/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_ops_cleanup(void)              
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Step 1: Disable `automatic' sparing selection.
     */
    fbe_api_control_automatic_hot_spare(FBE_FALSE);

    /* Step 2: Change all unused `spares' to `unconsumed'.
     */
    status = fbe_test_sep_util_unconfigure_all_spares_in_the_system();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3: Enable `automatic' sparing selection.
     */
    fbe_api_control_automatic_hot_spare(FBE_TRUE);

    return status;
}
/***************************************************************
 * end fbe_test_sep_background_ops_cleanup()
 ***************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_setup_and_validate_raid_groups()
 *****************************************************************************
 *
 * @brief   For the any copy background operation we must have at least (1)
 *          drive in the `extra' disk array for each raid groups.  This method
 *          populates the extra information and then validates that there is
 *          at least (1) `extra' drive. 
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 *
 * @return  FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  04/24/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_ops_setup_and_validate_raid_groups(fbe_test_rg_configuration_t *rg_config_p,   
                                                                               fbe_u32_t raid_group_count)              
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t       rg_index = 0;

    /* Validate the destination array is not NULL
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Step 1: Detemine which disk to use as the `extra' disk set.
     */
    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

    /* Step 2: Walk thru all the raid groups and validate that there is at
     *         least (1) extra drive.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Validate that there is at least (1) extra drive.
         */
        if ((current_rg_config_p->num_of_extra_drives < 1)                ||
            ((current_rg_config_p->extra_disk_set[0].bus == 0)       &&
             (current_rg_config_p->extra_disk_set[0].enclosure == 0) &&
             (current_rg_config_p->extra_disk_set[0].slot == 0)         )     )
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s: rg_index: %d id: %d type: %d width: %d num of extra: %d (1) extra drive required!", 
                       __FUNCTION__, rg_index, current_rg_config_p->raid_group_id, current_rg_config_p->raid_type,
                       current_rg_config_p->width, current_rg_config_p->num_of_extra_drives);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Return status
     */
    return status;
}
/******************************************************************
 * end fbe_test_sep_background_ops_setup_and_validate_raid_groups()
 ******************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_populate_destination_drives()
 *****************************************************************************
 *
 * @brief   First disable `automatic' sparing.  Then determine which free drives 
 *          will be used for each of the raid groups under test and then add 
 *          them to the `extra' information for each of the raid groups under 
 *          test.  
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 *
 * @return  FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  03/22/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_ops_populate_destination_drives(fbe_test_rg_configuration_t *rg_config_p,   
                                                                            fbe_u32_t raid_group_count)              
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t       rg_index = 0;
    fbe_u32_t       dest_index = 0;

    /* Validate the destination array is not NULL
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Step 1: Disable `automatic' sparing selection.  We need to do this since
     *         the user copy API must specify a specific destination PVD.
     */
    fbe_api_control_automatic_hot_spare(FBE_FALSE);

    /* Step 2: Walk thru all the raid groups and selected a spare to add to
     *         the `extra' array groups (this is the destination for the copy
     *         request).
     */
    status = fbe_test_sep_util_add_hotspares_to_hotspare_pool(rg_config_p, raid_group_count);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3: Walk thru all the raid groups and add the `extra' drive to
     *         the array of destination drives.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Add (1) `extra' drive per raid group to the `destination' array.
         */
        fbe_test_sep_background_ops_dest_array[dest_index] = current_rg_config_p->extra_disk_set[0];

        /* Goto next raid group.
         */
        current_rg_config_p++;
        dest_index++;
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/***************************************************************
 * end fbe_test_sep_background_ops_populate_destination_drives()
 ***************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_cleanup_destination_drives()
 *****************************************************************************
 *
 * @brief   First enable `automatic' sparing.  Then move any unused `extra'
 *          drives from the `reserved as spare' pool back to the `unconsumed
 *          spare' pool.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 *
 * @return  FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  04/18/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_ops_cleanup_destination_drives(fbe_test_rg_configuration_t *rg_config_p,   
                                                                           fbe_u32_t raid_group_count)              
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       dest_index = 0;

    /* Validate the destination array is not NULL
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Step 1: Set all the destination drive to `unused' (0_0_0).
     */
    for (dest_index = 0; dest_index < FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT; dest_index++)
    {
        /* Set the destination index to `unused'
         */
        fbe_test_sep_background_ops_dest_array[dest_index].bus = 0;
        fbe_test_sep_background_ops_dest_array[dest_index].enclosure = 0;
        fbe_test_sep_background_ops_dest_array[dest_index].slot = 0;
    }

    /* Step 2: Refresh the location of the all drives in each raid group. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);

    /* Step 3: Refresh the locations of the all extra drives in each raid group. 
     */
    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

    /* Step 4: Walk thru all the raid groups and move any `unused' spare drive
     *         back to the `automatic spare' pool.
     */
    fbe_test_sep_util_remove_hotspares_from_hotspare_pool(rg_config_p, raid_group_count);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 5: Enable `automatic' sparing selection.
     */
    fbe_api_control_automatic_hot_spare(FBE_TRUE);

    /* Return success
     */
    return FBE_STATUS_OK;
}
/***************************************************************
 * end fbe_test_sep_background_ops_cleanup_destination_drives()
 ***************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_start_background_ops()
 *****************************************************************************
 *
 * @brief   Start the requested background operations.
 *
 * @param   sequence_config_p - Pointer to sequence configuration 
 * @param   context_p - Used to determine 
 *
 * @return  FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  03/22/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_start_background_ops(fbe_test_sep_io_sequence_config_t  *sequence_config_p,
                                               fbe_api_rdgen_context_t *context_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t                   raid_group_count = 0;
    fbe_test_sep_io_background_op_enable_t background_ops;
    fbe_u32_t                   position_to_spare;
    fbe_u8_t                    scsi_op_to_inject_on = 0;

    /* Determine if it makes sense to run this background operation or not.
     */
    if (fbe_test_sep_should_background_ops_run(sequence_config_p, context_p) == FBE_FALSE)
    {
        /* Simply return success.
         */
        return FBE_STATUS_OK;
    }

    /* Get the array of raid group configuration
     */
    background_ops = sequence_config_p->background_ops;
    rg_config_p = sequence_config_p->rg_config_p;
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    position_to_spare = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "%s operation: 0x%x on position: %d",
               __FUNCTION__, background_ops, position_to_spare);
    fbe_test_sep_background_ops_copy_position = position_to_spare;

    /*! @note Currently we only support (1) active background operation
     *        active at a time.
     */
    switch(background_ops)
    {
        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_VERIFY:
            /*! @note Currently verify is not supported.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s Verify background op: 0x%x is not supported.",
                       __FUNCTION__, background_ops);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_PROACTIVE_COPY:
            /* Run a proactive copy during the I/O.
             */
            status = fbe_test_sep_background_ops_setup_and_validate_raid_groups(rg_config_p,
                                                                                raid_group_count);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_sep_determine_scsi_op_to_inject_on(context_p,
                                                                 &scsi_op_to_inject_on);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_sep_background_ops_proactive_spare_drives(rg_config_p,
                                                                        raid_group_count,
                                                                        position_to_spare,
                                                                        scsi_op_to_inject_on,
                                                                        FBE_TRUE /* Start I/O to force copy */);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_USER_COPY:
            /* Run a user copy during I/O.
             */
            status = fbe_test_sep_background_ops_setup_and_validate_raid_groups(rg_config_p,
                                                                                raid_group_count);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_sep_background_ops_populate_destination_drives(rg_config_p,   
                                                                             raid_group_count);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_sep_background_ops_start_user_copy_to(rg_config_p,
                                                                 raid_group_count,
                                                                 position_to_spare,
                                                                 &fbe_test_sep_background_ops_dest_array[0]);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_USER_PROACTIVE_COPY:
            /*! @note Currently user copy is not supported.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s user copy background op: 0x%x is not supported.",
                       __FUNCTION__, background_ops);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_DRIVE_GLITCH:
            /* `Glitch' a drive during I/O.
             */
            status = fbe_test_sep_background_pause_glitch_drives(rg_config_p,
                                                                 raid_group_count,
                                                                 0,         /* Position 0 is as good as any */
                                                                 FBE_TRUE,  /* Glitch the source drive */
                                                                 FBE_FALSE  /* There is no destination drive */);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        default:
            /* Invalid/unsupported combination of background operations.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s Unsupported setting for background ops: 0x%x",
                       __FUNCTION__, background_ops);
            status = FBE_STATUS_GENERIC_FAILURE;
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }

    /* Return the execution status.
     */
    return status;
}
/*********************************************
 * end fbe_test_sep_start_background_ops()
 *********************************************/

/*!***************************************************************************
 *          fbe_test_sep_wait_for_background_ops_complete()
 *****************************************************************************
 *
 * @brief   Wait for the background operations to complete.
 *
 * @param   sequence_config_p - Pointer to sequence configuration 
 * @param   wait_timeout_ms - Maximum time to wait for operations to complete
 * @param   context_p - Used to determine if background operatiosn were
 *              executed or not.
 *
 * @return  FBE_STATUS_OK if no errors were encountered.
 * 
 * @note    Currently wait_timeout_ms is ignored.
 * 
 * @author
 *  03/22/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_wait_for_background_ops_complete(fbe_test_sep_io_sequence_config_t  *sequence_config_p,
                                                           fbe_u32_t wait_timeout_ms,
                                                           fbe_api_rdgen_context_t *context_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_sep_io_background_op_enable_t background_ops;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t                   raid_group_count = 0;
    fbe_u32_t                   position_to_complete = fbe_test_sep_background_ops_copy_position;

    /* Determine if background operations have been started or not.
     */
    if (fbe_test_sep_should_background_ops_run(sequence_config_p, context_p) == FBE_FALSE)
    {
        /* Simply return success.
         */
        return FBE_STATUS_OK;
    }

    /* Get the array of raid group configuration
     */
    background_ops = sequence_config_p->background_ops;
    rg_config_p = sequence_config_p->rg_config_p;
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_TEST_SEP_BACKGROUND_OPS_INVALID_DISK_POSITION, position_to_complete);

    /*! @note Currently we only support (1) active background operation
     *        active at a time.
     */
    switch(background_ops)
    {
        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_VERIFY:
            /*! @note Currently verify is not supported.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s Verify background op: 0x%x is not supported.",
                       __FUNCTION__, background_ops);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_PROACTIVE_COPY:
            /* Wait for proactive copy to complete.
             */
            status = fbe_test_sep_background_ops_wait_for_proactive_copy_complete(rg_config_p, 
                                                                                  raid_group_count,
                                                                                  position_to_complete,
                                                                                  FBE_TRUE, /* Wait for swap out */
                                                                                  FBE_FALSE /* Do not skip cleanup */);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_USER_COPY:
            /* Wait for user copy to complete.
             */
            status = fbe_test_sep_background_ops_wait_for_user_copy_to_complete(rg_config_p, 
                                                                             raid_group_count,
                                                                             position_to_complete,
                                                                             FBE_TRUE, /* Wait for swap out */
                                                                             FBE_FALSE /* Do not skip cleanup */);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_USER_PROACTIVE_COPY:
            /*! @todo Currently user copy is not supported.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s User copy background op: 0x%x is not supported.",
                       __FUNCTION__, background_ops);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_DRIVE_GLITCH:
            /* Currently nothing to do (we could confirm EOL is not set)
             */
            break;

        default:
            /* Invalid/unsupported combination of background operations.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s Unsupported setting for background ops: 0x%x",
                       __FUNCTION__, background_ops);
            status = FBE_STATUS_GENERIC_FAILURE;
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }

    /* Return the execution status.
     */
    return status;
}
/*****************************************************
 * end fbe_test_sep_wait_for_background_ops_complete()
 *****************************************************/

/*!***************************************************************************
 *          fbe_test_sep_wait_for_background_ops_cleanup()
 *****************************************************************************
 *
 * @brief   The background operations have completed but there are other items
 *          that should occur:
 *              o Copy source shound be swapped out
 *              o Virtuaal drive should be in pas-thru mode
 *              o etc.
 *
 * @param   sequence_config_p - Pointer to sequence configuration 
 * @param   wait_timeout_ms - Maximum time to wait for operations to complete
 * @param   context_p - Used to determine if background operatiosn were
 *              executed or not.
 *
 * @return  FBE_STATUS_OK if no errors were encountered.
 * 
 * @note    Currently wait_timeout_ms is ignored.
 * 
 * @todo    Not implemented yet.
 * 
 * @author
 *  03/22/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_wait_for_background_ops_cleanup(fbe_test_sep_io_sequence_config_t  *sequence_config_p,
                                                          fbe_u32_t wait_timeout_ms,
                                                          fbe_api_rdgen_context_t *context_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t                   raid_group_count = 0;
    fbe_test_sep_io_background_op_enable_t background_ops;
    fbe_u32_t                   position_to_cleanup = fbe_test_sep_background_ops_copy_position;

    /* Determine if background operations have been run or not.
     */
    if (fbe_test_sep_should_background_ops_run(sequence_config_p, context_p) == FBE_FALSE)
    {
        /* Simply return success.
         */
        return FBE_STATUS_OK;
    }

    /*! @note Flag the fact that we have run background operations for this 
     *        sequence already.
     */
    sequence_config_p->b_have_background_ops_run = FBE_TRUE;

    /* Get the array of raid group configuration
     */
    background_ops = sequence_config_p->background_ops;
    rg_config_p = sequence_config_p->rg_config_p;
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_TEST_SEP_BACKGROUND_OPS_INVALID_DISK_POSITION, position_to_cleanup);

    /*! @note Currently we only support (1) active background operation
     *        active at a time.
     */
    switch(background_ops)
    {
        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_VERIFY:
            /*! @note Currently verify is not supported.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s Verify background op: 0x%x is not supported.",
                       __FUNCTION__, background_ops);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_PROACTIVE_COPY:
            /*! @note We have already cleaned up.
             */
            status = fbe_test_sep_background_ops_wait_for_proactive_copy_cleanup(rg_config_p,       
                                                                                 raid_group_count,
                                                                                 position_to_cleanup,
                                                                                 FBE_FALSE /* We have already cleaned up*/);  
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;


        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_USER_COPY:
            /* Cleanup after a user copy operation.  First move all the `reserved'
             * destination drives from the `reserved for spare' pool back to the
             * `unconsumed spare' pool.
             */
            status = fbe_test_sep_background_ops_cleanup_destination_drives(rg_config_p,   
                                                                            raid_group_count); 
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_sep_background_ops_wait_for_user_copy_to_cleanup(rg_config_p,       
                                                                            raid_group_count,
                                                                            position_to_cleanup,
                                                                            FBE_FALSE /* We have already cleaned up*/);  
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_USER_PROACTIVE_COPY:
            /*! @todo Currently user copy is not supported.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s User copy background op: 0x%x is not supported.",
                       __FUNCTION__, background_ops);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_DRIVE_GLITCH:
            /* Currently nothing to do (we could clear EOL)
             */
            break;

        default:
            /* Invalid/unsupported combination of background operations.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s Unsupported setting for background ops: 0x%x",
                       __FUNCTION__, background_ops);
            status = FBE_STATUS_GENERIC_FAILURE;
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }

    /* Set the position to copy to invalid.
     */
    fbe_test_sep_background_ops_copy_position = FBE_TEST_SEP_BACKGROUND_OPS_INVALID_DISK_POSITION;

    /* Return the execution status.
     */
    return status;
}
/*****************************************************
 * end fbe_test_sep_wait_for_background_ops_cleanup()
 *****************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_start_copy_operation()
 ***************************************************************************** 
 * 
 * @brief   Start a copy operation (of the type requested) on the raid group
 *          array specified.
 * 
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array
 * @param   position_to_spare - The position in the raid group to copy from
 * @param   copy_type - Type of copy command to initiate
 * @param   dest_array_p - Array of destination drives to copy to
 *
 * @return fbe_status_t 
 *
 * @author
 *  03/25/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_ops_start_copy_operation(fbe_test_rg_configuration_t *rg_config_p,
                                                              fbe_u32_t raid_group_count,
                                                              fbe_u32_t position_to_spare,
                                                              fbe_spare_swap_command_t copy_type,
                                                              fbe_test_raid_group_disk_set_t *dest_array_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Based on the requested copy type, initiate the copy request.
     */
    switch(copy_type)
    {
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
            /* Force a proactive copy operation by injecting an error.
             * Validate that not destination drives were supplied.
             */
            if (dest_array_p != NULL)
            {
                /* Cannot specify either a source or destination for proactive copy.
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, "%s Cannot specify dest: (%d_%d_%d) pvd for copy_type: %d", 
                           __FUNCTION__, dest_array_p->bus, dest_array_p->enclosure, dest_array_p->slot, copy_type);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
            }
            status = fbe_test_sep_background_ops_proactive_spare_drives(rg_config_p, 
                                                                raid_group_count,
                                                                position_to_spare,
                                                                FBE_SCSI_READ_16,
                                                                FBE_TRUE /* Start I/O to force copy */);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
            /* The position supplied should be the virtual drive contains the 
             * source pvd.  The destination arrays should be an unbound pvds 
             * to copy to.
             */
            if ((position_to_spare >= rg_config_p->width) ||
                (dest_array_p == NULL)                       )
            {
                /* Cannot specify either a source or destination for proactive copy.
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, "%s pos: %d or dest array: 0x%p not valid for copy_type: %d", 
                           __FUNCTION__, position_to_spare, dest_array_p, copy_type);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
            }
            status = fbe_test_sep_background_ops_start_user_copy_to(rg_config_p, 
                                                                 raid_group_count,
                                                                 position_to_spare,
                                                                 dest_array_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
            /* Start a user copy operation but kill the drive when the copy
             * is complete.  Validate that not destination drives were supplied.
             */
            if (dest_array_p != NULL)
            {
                /* Cannot specify either a source or destination for user copy.
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, "%s Cannot specify dest: (%d_%d_%d) pvd for copy_type: %d", 
                           __FUNCTION__, dest_array_p->bus, dest_array_p->enclosure, dest_array_p->slot, copy_type);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
            }
            status = fbe_test_sep_background_ops_start_user_copy(rg_config_p, 
                                                                           raid_group_count,
                                                                           position_to_spare);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND:
            /*! @todo Currently these copy operations are not supported. 
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s TO-DO: copy type: %d not supported !", 
                       __FUNCTION__, copy_type);
            break;

        default:
            /* Invalid copy_type specified.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid copy type: %d specified.", 
                       __FUNCTION__, copy_type);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }

    /* Return the execution status.
     */
    return status;
}
/*******************************************************
 * end fbe_test_sep_background_ops_start_copy_operation()
 *******************************************************/

/*!***************************************************************************
 * fbe_test_sep_background_ops_wait_for_copy_operation_complete()
 *****************************************************************************
 *
 * @brief   Wait for the copy operation to complete for all raid groups.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array
 * @param   copy_position - Position in raid group the is being copied
 * @param   copy_type - The copy operation type to wait for 
 * @param   b_wait_for_swap_out - FBE_TRUE - Wait for the source pvd to be swapped 
 *              out of the virtual drive.  This value cannot be set to True
 *              unless b_skip_copy_operation_cleanup is also True.
 * @param   b_skip_copy_operation_cleanup - FBE_TRUE - Skip the automatic cleanup
 *              (re-insert drives marked EOL etc).  The only reason this would be
 *              set is to verify that drives are marked EOL.
 *
 * @return fbe_status_t 
 *
 * @note    Unless b_skip_copy_operation_cleanup is set the FBE_TRUE, this
 *          method will also `cleanup' which means any drives marked EOL etc.
 *          will be put back into their original state.
 *
 * @author
 *  03/25/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_ops_wait_for_copy_operation_complete(fbe_test_rg_configuration_t *rg_config_p, 
                                                                          fbe_u32_t raid_group_count,
                                                                          fbe_u32_t copy_position,
                                                                          fbe_spare_swap_command_t copy_type,
                                                                          fbe_bool_t b_wait_for_swap_out,
                                                                          fbe_bool_t b_skip_copy_operation_cleanup)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Validate the use of `wait for swap out' and `skip copy cleanup'.
     */
    if ((b_wait_for_swap_out == FBE_FALSE)           &&
        (b_skip_copy_operation_cleanup == FBE_FALSE)    )
    {
        /* Invalid combination.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "%s Wait for swap: %d and skip cleanup: %d cannot both be False.", 
                   __FUNCTION__, b_wait_for_swap_out, b_skip_copy_operation_cleanup);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Generate a warning if `skip copy cleanup' is set.
     */
    if (b_skip_copy_operation_cleanup == FBE_TRUE)
    {
        /*! @todo Is this option required?
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s skip cleanup: %d. Must call `copy operation cleanup' if additional tests!!", 
                   __FUNCTION__, b_skip_copy_operation_cleanup);
    }

    /*! @note Currently all copy operation types use the same completion. 
     */
    switch(copy_type)
    {
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
            /*! @note Currently most copy operations use this completion.
             */
            status = fbe_test_sep_background_ops_wait_for_proactive_copy_complete(rg_config_p, 
                                                                                  raid_group_count,
                                                                                  copy_position,
                                                                                  b_wait_for_swap_out,
                                                                                  b_skip_copy_operation_cleanup);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
            /* Wait for a user requested copy command to complete.
             */
            status = fbe_test_sep_background_ops_wait_for_user_copy_to_complete(rg_config_p, 
                                                                             raid_group_count,
                                                                             copy_position,
                                                                             b_wait_for_swap_out,
                                                                             b_skip_copy_operation_cleanup);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
            /* Wait for a user requested copy command to complete.
             */
            status = fbe_test_sep_background_ops_wait_for_user_copy_complete(rg_config_p, 
                                                                                       raid_group_count,
                                                                                       copy_position,
                                                                                       b_wait_for_swap_out,
                                                                                       b_skip_copy_operation_cleanup);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND:
            /*! @todo Currently these copy operations are not supported. 
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s TO-DO: copy type: %d not supported !", 
                       __FUNCTION__, copy_type);
            break;

        default:
            /* Invalid copy_type specified.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid copy type: %d specified.", 
                       __FUNCTION__, copy_type);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }

    /* Return the execution status.
     */
    return status;
}
/*********************************************************************
 * end fbe_test_sep_background_ops_wait_for_copy_operation_complete()
 *********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_ops_copy_operation_cleanup()
 ***************************************************************************** 
 * 
 * @brief   Re-inserts any pvds that have been marked EOL etc due to a copy
 *          operation.  This method must be invoked to re-use drives for the
 *          next set of tests (otherwise there will be insufficient drives).
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array
 * @param   position_to_cleanup - Position that requires cleanup
 * @param   copy_type - The copy operation type to wait for
 * @param   b_has_cleanup_been_performed - FBE_TRUE - Cleanup has alreay been done.
 *
 * @return fbe_status_t 
 *
 * @author
 *  03/25/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_ops_copy_operation_cleanup(fbe_test_rg_configuration_t *rg_config_p, 
                                                                fbe_u32_t raid_group_count,
                                                                fbe_u32_t position_to_cleanup,
                                                                fbe_spare_swap_command_t copy_type,
                                                                fbe_bool_t b_has_cleanup_been_performed)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_bool_t      b_is_cleanup_required = FBE_TRUE;

    /* Check if cleanup is required.
     */
    b_is_cleanup_required = (b_has_cleanup_been_performed) ? FBE_FALSE : FBE_TRUE;

    /*! @note Currently all copy operation types use the same completion. 
     */
    switch(copy_type)
    {
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
            /*! @todo Currently most operations use this cleanup.  But only 
             *        proactive copy operations should use it.
             */
            status = fbe_test_sep_background_ops_wait_for_proactive_copy_cleanup(rg_config_p,       
                                                                                 raid_group_count,
                                                                                 position_to_cleanup,
                                                                                 b_is_cleanup_required);  
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;


        case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
            /* User requested copy cleanup
             */
            status = fbe_test_sep_background_ops_wait_for_user_copy_to_cleanup(rg_config_p,       
                                                                            raid_group_count,
                                                                            position_to_cleanup,
                                                                            b_is_cleanup_required);  
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
            /* User requested copy cleanup
             */
            status = fbe_test_sep_background_ops_wait_for_user_copy_cleanup(rg_config_p,       
                                                                                      raid_group_count,
                                                                                      position_to_cleanup,
                                                                                      b_is_cleanup_required);  
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND:
            /*! @todo Currently these copy operations are not supported. 
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s TO-DO: copy type: %d not supported !", 
                       __FUNCTION__, copy_type);
            break;

        default:
            /* Invalid copy_type specified.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid copy type: %d specified.", 
                       __FUNCTION__, copy_type);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }

    /* In all cases we need to `refresh' the actual drives being used in each
     * of the raid group test configurations.
     */
    status = fbe_test_sep_background_ops_refresh_raid_group_disk_info(rg_config_p, raid_group_count);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Return the execution status.
     */
    return status;
}
/*********************************************************************
 * end fbe_test_sep_background_ops_copy_operation_cleanup()
 *********************************************************************/

/******************************************************
 * end of file fbe_test_sep_background_operations.c
 ******************************************************/

