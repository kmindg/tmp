/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_neit_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions for use with the `New NEIT' package.
 * 
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/

#include "mut.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_protocol_error_injection_service.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_sector.h"
#include "sep_utils.h"

/*************************
 *   FUNCTIONS
 *************************/

fbe_status_t
fbe_test_neit_utils_init_error_injection_record(fbe_protocol_error_injection_error_record_t * error_injection_record)
{
    /* First zero the record.
     */
    fbe_zero_memory(error_injection_record, sizeof(fbe_protocol_error_injection_error_record_t));
    fbe_set_memory(&error_injection_record->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command, 0xff, MAX_CMDS);
    /*  Now initialize any fields where 0 is not the desired value.
     */
    error_injection_record->skip_blks = FBE_U32_MAX; /* Initialize to invalid value; Zero is valid. */
    return FBE_STATUS_OK;
}

/*!***************************************************************************
 *          fbe_test_neit_utils_convert_to_physical_drive_range()
 *****************************************************************************
 *
 * @brief   Convert from 520 block size to 4k if the drive is a 4k drive
 *
 * @param   pdo_object_id - The physical drive object id
 * @param   start_lba - Starting lba to convert
 * @param   end_lba - Ending lba to convert
 * @param   physical_start_lba_p - Address of physical start lba to update
 * @param   physical_end_lba_p - Address of physical end lba to update
 *
 * @return  status
 *
 * @author
 *  05/12/2014  Deanna Heng  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_test_neit_utils_convert_to_physical_drive_range(fbe_object_id_t pdo_object_id,
                                                                 fbe_lba_t start_lba,
                                                                 fbe_lba_t end_lba,
                                                                 fbe_lba_t *physical_start_lba_p,
                                                                 fbe_lba_t *physical_end_lba_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_physical_drive_information_t physical_drive_info = { 0 };
   
    status = fbe_api_physical_drive_get_drive_information(pdo_object_id, &physical_drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    switch(physical_drive_info.block_size) {
        case FBE_BE_BYTES_PER_BLOCK:
            *physical_start_lba_p = start_lba;
            *physical_end_lba_p = end_lba;
            break;

        case FBE_4K_BYTES_PER_BLOCK:
            *physical_start_lba_p = start_lba / FBE_520_BLOCKS_PER_4K;
            *physical_end_lba_p = end_lba / FBE_520_BLOCKS_PER_4K;
            break;

        default:
            /* Unsupported block size.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Unsupported physical drive block size: %d",
                       __FUNCTION__, physical_drive_info.block_size);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
    }

    return status;
}
/********************************************************************
 * end fbe_test_neit_utils_convert_to_physical_drive_range()
 ********************************************************************/

/*!***************************************************************************
 *          fbe_test_neit_utils_populate_protocol_error_injection_record()
 *****************************************************************************
 *
 * @brief   Populate error record (after initializing it) with the data passed.
 *
 * @param   disk_location_bus - Bus number of disk to inject error on
 * @param   disk_location_enclosure - Enclosure number of disk to inject error on
 * @param   disk_location_slot - Slot number of disk to inject error on
 * @param   error_record_p - Pointer to array of error record to populate
 * @param   record_handle_p - Pointer to record handle to update
 * @param   user_space_start_lba - First user space lba to inject to 
 *              (must be the same for all raid groups)
 * @param   blocks - Number of blocks to inject for
 * @param   protocol_error_injection_error_type - Currently must be SCSI 
 * @param   scsi_command - SCSI command type to inject for
 * @param   scsi_sense_key - Sense key to inject
 * @param   scsi_additional_sense_code - Sense code to inject
 * @param   scsi_additional_sense_code_qualifier - Sense qualifier to inject
 * @param   port_status - Port status to inject
 * @param   num_of_times_to_insert - Number of times to inject error
 *
 * @return  status - Typically FBE_STATUS_OK.
 *
 * @author
 *  03/15/2012  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_test_neit_utils_populate_protocol_error_injection_record(fbe_u32_t disk_location_bus,
                                                                          fbe_u32_t disk_location_enclosure,
                                                                          fbe_u32_t disk_location_slot,
                                                              fbe_protocol_error_injection_error_record_t *error_record_p,
                                                              fbe_protocol_error_injection_record_handle_t *record_handle_p,
                                                              fbe_lba_t user_space_start_lba,
                                                              fbe_block_count_t blocks,
                                                              fbe_protocol_error_injection_error_type_t protocol_error_injection_error_type,
                                                              fbe_u8_t scsi_command,
                                                              fbe_scsi_sense_key_t scsi_sense_key,
                                                              fbe_scsi_additional_sense_code_t scsi_additional_sense_code,
                                                              fbe_scsi_additional_sense_code_qualifier_t scsi_additional_sense_code_qualifier,
                                                              fbe_port_request_status_t port_status, 
                                                              fbe_u32_t num_of_times_to_insert)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_lba_t                   start_lba = FBE_LBA_INVALID;
    fbe_lba_t                   end_lba = FBE_LBA_INVALID;
    fbe_u8_t                    scsi_command_to_inject = scsi_command;
    fbe_object_id_t             drive_object_id = 0;
    fbe_object_id_t             pvd_object_id = 0;
    fbe_api_provision_drive_info_t provision_drive_info;
    fbe_lba_t                   default_offset = FBE_LBA_INVALID;
    fbe_block_size_t            block_size = FBE_BE_BYTES_PER_BLOCK;

    /* Validate error type.
     */
    MUT_ASSERT_TRUE_MSG((protocol_error_injection_error_type == FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI),
                        "fbe_test_neit_utils_populate_protocol_error_injection_record: Only SCSI error type supported");

    /* Validate command.
     */
    switch (scsi_command)
    {
        case FBE_SCSI_TEST_UNIT_READY:
            break;
        case FBE_SCSI_READ_6:
        case FBE_SCSI_READ_10:
        case FBE_SCSI_READ_12:
        case FBE_SCSI_READ_16:
            /* Currently the error injection service expected 16-byte cdbs */
            scsi_command_to_inject = FBE_SCSI_READ_16;
            break;
        case FBE_SCSI_VERIFY_10:
        case FBE_SCSI_VERIFY_16:
            /* Currently the error injection service expects 16-byte cdbs */
            scsi_command_to_inject = FBE_SCSI_VERIFY_16;
            break;
        case FBE_SCSI_WRITE_6:
        case FBE_SCSI_WRITE_10:
        case FBE_SCSI_WRITE_12:
        case FBE_SCSI_WRITE_16:
            /* Currently the error injection service expects 16-byte cdbs */
            scsi_command_to_inject = FBE_SCSI_WRITE_16;
            break;
        case FBE_SCSI_WRITE_SAME_10:
        case FBE_SCSI_WRITE_SAME_16:
            /* Currently the error injection service expects 16-byte cdbs */
            scsi_command_to_inject = FBE_SCSI_WRITE_SAME_16;
            break;
        case FBE_SCSI_WRITE_VERIFY_10:
        case FBE_SCSI_WRITE_VERIFY_16:
            /* Currently the error injection service expects 16-byte cdbs */
            scsi_command_to_inject = FBE_SCSI_WRITE_VERIFY_16;
            break;
        default:
            /* Unsupported opcode */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Unsupported SCSI opcode: 0x%x",
                       __FUNCTION__, scsi_command);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
    }

    /* Get the drive object id for the position specified.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(disk_location_bus,
                                                              disk_location_enclosure,
                                                              disk_location_slot, 
                                                              &drive_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Calculate the physical lba to inject to.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(disk_location_bus,
                                                            disk_location_enclosure,
                                                            disk_location_slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    default_offset = provision_drive_info.default_offset;
    start_lba = user_space_start_lba + default_offset;
    end_lba = start_lba + blocks - 1;

    /* We need to convert to the proper physical drive block to inject to.
     */
    status = fbe_test_neit_utils_convert_to_physical_drive_range(drive_object_id,
                                                                 start_lba,
                                                                 end_lba,
                                                                 &start_lba,
                                                                 &end_lba);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize the record
     */
    fbe_test_neit_utils_init_error_injection_record(error_record_p);

    /* Populate the record
     */
    error_record_p->lba_start  = start_lba;
    error_record_p->lba_end    = end_lba;
    error_record_p->object_id  = drive_object_id;
    error_record_p->protocol_error_injection_error_type = protocol_error_injection_error_type;
    error_record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = scsi_command_to_inject;
    error_record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = scsi_sense_key;
    error_record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code = scsi_additional_sense_code;
    error_record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = scsi_additional_sense_code_qualifier;
    error_record_p->num_of_times_to_insert = num_of_times_to_insert;
    error_record_p->secs_to_reactivate = FBE_U32_MAX; /* Don't re-activate.*/

    /* Add the error injection record to the service, which also returns 
     * the record handle for later use.
     */
    status = fbe_api_protocol_error_injection_add_record(error_record_p, record_handle_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Print some information about the record
     */
    mut_printf(MUT_LOG_TEST_STATUS, "neit_utils_populate_protocol: obj: 0x%x lba: 0x%llx end: 0x%llx blksz: %d op: 0x%02x k/c/q:%02x/%02x/%02x times: %d",
               drive_object_id, 
               (unsigned long long)start_lba, (unsigned long long)end_lba, block_size,
               scsi_command_to_inject,
               scsi_sense_key, scsi_additional_sense_code, scsi_additional_sense_code_qualifier, num_of_times_to_insert);

    /* Return success
     */
    return FBE_STATUS_OK;
}
/********************************************************************
 * end fbe_test_neit_utils_populate_protocol_error_injection_record()
 ********************************************************************/

/*********************************
 * end file fbe_test_neit_utils.c
 *********************************/
