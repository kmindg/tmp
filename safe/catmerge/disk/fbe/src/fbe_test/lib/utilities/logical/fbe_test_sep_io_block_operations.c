/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_sep_io_block_operations.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions for testing SEP block operations.
 * 
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "sep_tests.h"
#include "fbe_test_common_utils.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "sep_test_io_private.h"
#include "fbe/fbe_api_transport.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe/fbe_random.h"

/*****************************************
 * DEFINITIONS
 *****************************************/

/*************************
 *   GLOBALS
 *************************/
static fbe_test_sep_io_block_operations_info_t fbe_test_io_block_operations_test_status = {0};
static fbe_u32_t                               fbe_test_io_block_operations_min_num_of_luns_to_test = 1;

/*! @todo Due to the following known issues with `Corrupt Data' operation testing is 
 *        currently disabled.
 *          o Mirror read optimization sends second siots to position 1 which does
 *            not have `Corrupt Data'.  Thus unexpected good data is returned.  Need
 *            to add a new payload block operation flag for the read request to turn
 *            off mirror optimization or reject `Corrupt Data' requests that are 
 *            greater than a `chunk'.
 */
static fbe_bool_t                               fbe_test_io_block_operations_enable_corrupt_data = FBE_TRUE;

/*! @todo Due to the fact that striped mirrors have a different logical layout,
 *        using a random corrupt bitmap currently does not work (previously the
 *        corrupt bitmap was -1 (corrupt all blocks).
 */
static fbe_bool_t                               fbe_test_io_block_operations_raid10_random_bitmap = FBE_FALSE;


/*************************
 *   FUNCTIONS
 *************************/

/*!***************************************************************************
 *          fbe_test_sep_io_init_block_operations_array()
 *****************************************************************************
 *
 * @brief   Initialize the global array of block opertions that have been
 *          tested.
 *
 * @param   None
 *
 * @return  status 
 *
 * @author
 *  09/27/2011  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_io_init_block_operations_array(void)
{
    fbe_payload_block_operation_opcode_t    opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;

    /*! @note First zero the entire status structure.
     *        This implies that the default is to check the opcode.
     */
    fbe_zero_memory((void *)&fbe_test_io_block_operations_test_status, sizeof(fbe_test_sep_io_block_operations_info_t));

    /* For each opcode set the appropriate status for the `don't check checksum'
     * (currently assumed to be the default) case.
     */
    for (opcode = (FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID + 1); 
         opcode < FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_LAST;
         opcode++)
    {
        /*! Now handle the cases we do actually test or cannot test from the 
         *  lun object.
         * 
         *  @note We purposefully don't have a default: case so that the compilier
         *        will detect when a new opcode is added or changed!
         */
        switch(opcode)
        {
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID:
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
                /* We must check this opcode without `check checksum' */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_TESTED;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
                /*! @note We must check this opcode without `check checksum'.
                 */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_TESTED;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
                /*! @todo We should be testing this opcode without `check checksum' set*/
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_FORCE_ZERO:
                /*! @todo Currently not used and should be removed */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_CURRENTLY_UNUSED;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_BACKGROUND_ZERO:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
                /* Verify is only allowed as a background operation initiated by the monitor. */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE:
                /*! @todo We should be testing this opcode without `check checksum' set*/
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WITH_BUFFER:
                /*! @todo Currently not used and should be removed */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_CURRENTLY_UNUSED;
                break;
                   
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_SET_CHKPT:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_CONSUMED_ZERO:
                /*! @todo We should be testing this opcode without `check checksum' set at the raid group object */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_IDENTIFY:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY:
                /*! @todo We should be testing this opcode without `check checksum' set at the raid group object */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY:
                /*! @todo We should be testing this opcode without `check checksum' set at the raid group object */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY:
                /*! @todo We should be testing this opcode without `check checksum' set at the raid group object */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY:
                /*! @todo We should be testing this opcode without `check checksum' set at the raid group object */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;                

            case FBE_PAYLOAD_BLOCK_OPERATION_MARK_FOR_REBUILD:
                /*! @todo We should be testing this opcode without `check checksum' set at the raid group object */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA:
                /*! @todo We should be testing this opcode without `check checksum' set at the raid group object */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA:
                /*! @todo We should be testing this opcode without `check checksum' set at the raid group object */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED:
                /* We must check this opcode without `check checksum' at raid group object */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_TESTED;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA:
                /* We must check this opcode without `check checksum' at raid group object */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_TESTED;
                /*! @todo Currently `Corrupt Data' testing is disabled.
                 */
                if (fbe_test_io_block_operations_enable_corrupt_data == FBE_FALSE)
                {
                    fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                }
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_HDR_RD:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_DISK_ZERO:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_CONSUMED:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO:
                /*! @todo We should be testing this opcode */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_RO_VERIFY:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_USER_VERIFY:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_SYSTEM_VERIFY:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_ERROR_VERIFY:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_INCOMPLETE_WRITE_VERIFY:
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INIT_EXTENT_METADATA:
                fbe_test_io_block_operations_test_status.block_operation_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_LAST:
                MUT_ASSERT_TRUE(FBE_FALSE);
                break;

        } /*! @note Do not add a default: case!! */

    } /* end of initialize block status array (opcodes without the `check checksum' flag set) */

    /* For each opcode set the appropriate status for the `check checksum' opcode
     */
    for (opcode = (FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID + 1); 
         opcode < FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_LAST;
         opcode++)
    {
        /*! Now handle the cases we do actually test or cannot test from the 
         *  lun object.
         * 
         *  @note We purposefully don't have a default: case so that the compilier
         *        will detect when a new opcode is added or changed!
         */
        switch(opcode)
        {
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID:
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
                /* We must check this opcode with the `check checksum' flag set */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_TESTED;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
                /* We must check this opcode with the `check checksum' flag set */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_TESTED;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
                /*! @todo We should be testing this opcode with the `check checksum' set*/
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_FORCE_ZERO:
                /*! @todo Currently not used and should be removed */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_CURRENTLY_UNUSED;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_BACKGROUND_ZERO:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
                /* Verify is only allowed as a background operation initiated by the monitor. */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE:
                /*! @todo We should be testing this opcode with the `check checksum' set*/
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WITH_BUFFER:
                /*! @todo Currently not used and should be removed */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_CURRENTLY_UNUSED;
                break;
                   
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_SET_CHKPT:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_CONSUMED_ZERO:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD:
                /*! @todo We should be testing this opcode with the `check checksum' set at the raid group object */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_IDENTIFY:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY:
                /*! @todo We should be testing this opcode with the `check checksum' set at the raid group object */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY:
                /*! @todo We should be testing this opcode with the `check checksum' set at the raid group object */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY:
                /*! @todo We should be testing this opcode with the `check checksum' set at the raid group object */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY:
                /*! @todo We should be testing this opcode with the `check checksum' set at the raid group object */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;                

            case FBE_PAYLOAD_BLOCK_OPERATION_MARK_FOR_REBUILD:
                /*! @todo We should be testing this opcode with the `check checksum' set at the raid group object */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA:
                /*! @todo We should be testing this opcode with the `check checksum' set at the raid group object */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA:
                /*! @todo We should be testing this opcode with the `check checksum' set at the raid group object */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED:
                /* We must check this opcode with the `check checksum' at raid group object */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_TESTED;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA:
                /* We must check this opcode with the `check checksum' at raid group object */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_TESTED;
                /*! @todo Currently `Corrupt Data' testing is disabled.
                 */
                if (fbe_test_io_block_operations_enable_corrupt_data == FBE_FALSE)
                {
                    fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                }
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_HDR_RD:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_DISK_ZERO:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_CONSUMED:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;                

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO:
                /*! @todo We should be testing this opcode */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE:
                /* Not applicable to the lun or raid group objects */
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;                
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_RO_VERIFY:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_USER_VERIFY:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_SYSTEM_VERIFY:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_ERROR_VERIFY:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_INCOMPLETE_WRITE_VERIFY:
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INIT_EXTENT_METADATA:
                fbe_test_io_block_operations_test_status.check_checksum_status[opcode] = FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE;
                break;
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_LAST:
                MUT_ASSERT_TRUE(FBE_FALSE);
                break;

        } /*! @note Do not add a default: case!! */

    } /* end of initialize block status array (opcodes with the `check checksum' flag set) */

    return FBE_STATUS_OK;
}
/*****************************************************
 * end of fbe_test_sep_io_init_block_operations_array()
 *****************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_print_block_operation_stats()
 *****************************************************************************
 *
 * @brief   Now print the test results for this set of raid groups.
 *
 * @param   sequence_config_p - Pointer to sequence configuration
 *
 * @return  None
 *
 * @author
 *  09/27/2011  Ron Proulx  - Created
 *
 *****************************************************************************/
static void fbe_test_sep_io_print_block_operation_stats(fbe_test_sep_io_sequence_config_t *sequence_config_p)
{
    fbe_payload_block_operation_opcode_t    opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    const fbe_char_t *raid_type_string_p = NULL;
    /*! @note Subtract (1) for unused 'FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID' */
    fbe_u32_t   total_block_operations = (FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_LAST - 1);
    fbe_u32_t   block_operations_tested = 0;
    fbe_u32_t   block_operations_not_tested = 0;
    fbe_u32_t   block_operations_not_applicable = 0;
    fbe_u32_t   block_operations_to_do = 0;
    fbe_u32_t   block_operations_check_checksum_tested = 0;
    fbe_u32_t   block_operations_check_checksum_not_tested = 0;
    fbe_u32_t   block_operations_check_checksum_not_applicable = 0;
    fbe_u32_t   block_operations_check_checksum_to_do = 0;

    /* Get the raid group type string
     */
    fbe_test_sep_util_get_raid_type_string(sequence_config_p->rg_config_p->raid_type, &raid_type_string_p);

    /* For each opcode set the appropriate status for the `check checksum' opcode
     */
    for (opcode = (FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID + 1); 
         opcode < FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_LAST;
         opcode++)
    {
        /* Check the status for default (i.e. `check checksum' not set).
         */
        switch(fbe_test_io_block_operations_test_status.block_operation_status[opcode])
        {
            case FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_TESTED:
                block_operations_not_tested++;
                break;
            case FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_CANNOT_TEST:
            case FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE:
            case FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_CURRENTLY_UNUSED:
                block_operations_not_applicable++;
                break;
            case FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO:
                block_operations_to_do++;
                break;
            case FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TESTED:
                block_operations_tested++;
                break;
            default:
                /* This should never happen 
                 */
                MUT_ASSERT_TRUE_MSG(FBE_FALSE, " Unexpected block operation status");
                break;
        }

        /* Check the status for operations sent with the `check checksum' flag set.
         */
        switch(fbe_test_io_block_operations_test_status.check_checksum_status[opcode])
        {
            case FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_TESTED:
                block_operations_check_checksum_not_tested++;
                break;
            case FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_CANNOT_TEST:
            case FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_APPLICABLE:
            case FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_CURRENTLY_UNUSED:
                block_operations_check_checksum_not_applicable++;
                break;
            case FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TO_DO:
                block_operations_check_checksum_to_do++;
                break;
            case FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TESTED:
                block_operations_check_checksum_tested++;
                break;
            default:
                /* This should never happen 
                 */
                MUT_ASSERT_TRUE_MSG(FBE_FALSE, " Unexpected block operation status");
                break;
        }

    } /* end for all block operations */

    /* Now display the results.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "test block operations: Complete for raid type: %s", 
               raid_type_string_p); 

    mut_printf(MUT_LOG_TEST_STATUS, "tested block operations: w/o  `check checksum' tested: %d missed: %d todo: %d n/a: %d  out of total: %d", 
               block_operations_tested, block_operations_not_tested, block_operations_to_do, 
               block_operations_not_applicable, total_block_operations);

    mut_printf(MUT_LOG_TEST_STATUS, "tested block operations: with `check checksum' tested: %d missed: %d todo: %d n/a: %d  out of total: %d", 
               block_operations_check_checksum_tested, block_operations_check_checksum_not_tested, block_operations_check_checksum_to_do, 
               block_operations_check_checksum_not_applicable, total_block_operations);

    /* Return
     */
    return;
}
/*****************************************************
 * end of fbe_test_sep_io_print_block_operation_stats()
 *****************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_check_block_operation_results()
 *****************************************************************************
 *
 * @brief   Now validate that all operation codes have been tested.
 *
 * @param   sequence_config_p - Pointer to sequence configuration
 *
 * @return  status 
 *
 * @author
 *  09/27/2011  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_io_check_block_operation_results(fbe_test_sep_io_sequence_config_t *sequence_config_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_payload_block_operation_opcode_t opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    const fbe_char_t   *raid_type_string_p = NULL;

    /* Get the raid group type string
     */
    fbe_test_sep_util_get_raid_type_string(sequence_config_p->rg_config_p->raid_type, &raid_type_string_p);

    /* For each opcode set the appropriate status for the `check checksum' opcode
     */
    for (opcode = (FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID + 1); 
         opcode < FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_LAST;
         opcode++)
    {
        /* Check the status for default (i.e. `check checksum' not set).
         */
        switch(fbe_test_io_block_operations_test_status.block_operation_status[opcode])
        {
            /* Log an error for the `not tested' case
             */
            case FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_TESTED:
                mut_printf(MUT_LOG_TEST_STATUS, "_check_block_operation_results: block opcode 0x%x", opcode); 
                status = FBE_STATUS_GENERIC_FAILURE;
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                         " Unexpected `not tested' status for block operation");
                break;

            default:
                /* Nothing to note
                 */
                break;
        }

        /* Check the status for operations sent with the `check checksum' flag set.
         */
        switch(fbe_test_io_block_operations_test_status.check_checksum_status[opcode])
        {
            /* Log an error for the `not tested' case
             */
            case FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_NOT_TESTED:
                mut_printf(MUT_LOG_TEST_STATUS, "_check_block_operation_results: block opcode 0x%x", opcode);                 
                status = FBE_STATUS_GENERIC_FAILURE;
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                         " Unexpected `not tested' status for block operation");
                break;

            default:
                /* Nothing to note
                 */
                break;
        }

    } /* end for all block operations */

    /* Now display the results.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "test block operations: Success for raid type: %s", 
               raid_type_string_p); 

    /* Return the status
     */
    return status;
}
/********************************************************
 * end of fbe_test_sep_io_check_block_operation_results()
 ********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_send_block_operation()
 *****************************************************************************
 *
 * @brief   Send the requested block operation to the object and package
 *          specified.  Based on the b_check_checksum flag will will set the
 *          test status is the global test status array for this opcode
 *
 * @param   object_id - Either the lun or raid group object to send request to
 * @param   package_id - Package id to send the request to
 * @param   block_operation_p - Pointer to block operation to send
 * @param   block_operation_info_p - Pointer to block operation info to populate
 * @param   sg_p - Pointer to sgl to use
 * @param   b_check_checksum - FBE_TRUE - Set the `check checksum' flag in the
 *                                  request.
 *                             FBE_FALSE - Clear the `check checksum' flag in the
 *                                  request.
 *
 * @return  status - Return status from request
 *
 * @author
 *  09/27/2011  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_io_send_block_operation(fbe_object_id_t  object_id,
                                                         fbe_package_id_t package_id,
                                                         fbe_payload_block_operation_t *block_operation_p,
                                                         fbe_block_transport_block_operation_info_t *block_operation_info_p,
                                                         fbe_raid_verify_error_counts_t *verify_err_counts_p,
                                                         fbe_sg_element_t  *sg_p,
                                                         fbe_bool_t b_check_checksum)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /*! @note Assumes that the block operation has been populated.
     *        Based on the b_check_checksum flag set or clear the
     *        flag in the request.
     */
    if (b_check_checksum == FBE_TRUE)
    {
        /* Set the `check checksum' flag.
         */
        fbe_payload_block_set_flag(block_operation_p,
                                   FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM);
    }
    else
    {
        /* Else clear the `check checksum' flag.
         */
        fbe_payload_block_clear_flag(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM);
    }

    /* We always update block operation test status (even if the request failed).
     */
    if (block_operation_p->block_opcode >= FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_LAST)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s block opcode: %d greater than max: %d", 
                   __FUNCTION__, block_operation_p->block_opcode, (FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_LAST - 1));
        MUT_ASSERT_TRUE(block_operation_p->block_opcode < FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_LAST);
        return status;
    }

    /* Check if `check checksum' is supported for this operation or not.
     */
    if (b_check_checksum)
    {
        /* Set the status in the `check checksum' array.
         */
        fbe_test_io_block_operations_test_status.check_checksum_status[block_operation_p->block_opcode] =
                            FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TESTED;
    }
    else
    {
        /* Else set the status in the default (`check checksum' not set) array.
         */
        fbe_test_io_block_operations_test_status.block_operation_status[block_operation_p->block_opcode] =
                            FBE_TEST_SEP_IO_BLOCK_OPERATION_STATUS_TESTED;
    }
    
    /* Now complete the block operation and send it.
     */
    block_operation_info_p->block_operation = *block_operation_p;
    block_operation_info_p->verify_error_counts = *verify_err_counts_p;
    status = fbe_api_block_transport_send_block_operation(object_id, 
                                                          package_id,
                                                          block_operation_info_p, 
                                                          sg_p);

    /* Just return the execution status
     */
    return status;
}
/***********************************************
 * end of fbe_test_sep_io_send_block_operation()
 ***********************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_get_expected_block_operation_status()
 *****************************************************************************
 *
 * @brief   Based on the opcode, raid type, data pattern flags and `check
 *          checksum' values, determine the expected block status and qualifier.
 *
 * @param   block_opcode - The FBE block operation code for write request
 * @param   raid_type - Used to make a decision on expected `Corrupt Data' status
 * @param   data_pattern_flags - Special data pattern flags for request
 * @param   b_check_checksum - FBE_TRUE - Issue the original request with the
 *                                  `check checksum' flag set in the payload.
 *                             FBE_FALSE - Clear the `check checksum' flag in the
 *                                  payload.
 * @param   expected_status_p - Pointer to expected status to update
 * @param   expected_qualifier_p - Pointer to expectged qualifer to update
 *
 * @return  status  - Typically FBE_STATUS_OK
 *
 * @author
 *  09/27/2011  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_io_get_expected_block_operation_status(fbe_payload_block_operation_opcode_t block_opcode,
                                               fbe_raid_group_type_t raid_type,
                                               fbe_data_pattern_flags_t data_pattern_flags,
                                               fbe_bool_t b_check_checksum,
                                               fbe_payload_block_operation_status_t *expected_status_p,
                                               fbe_payload_block_operation_qualifier_t *expected_qualifier_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Clear any extra data pattern flags.
     */
    data_pattern_flags &= ~(FBE_DATA_PATTERN_FLAGS_DO_NOT_CHECK_INVALIDATION_REASON);

    /* First set the status and qualifier to an unexpected state.
     */
    *expected_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    *expected_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;

    /* First determine if this is a media modify operation.  For all media
     * modify operation we expect success.
     */
    if (fbe_payload_block_operation_opcode_is_media_modify(block_opcode) == FBE_TRUE)
    {
        /* We expect all media modify requests to succeed.
         */
        *expected_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
        *expected_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
    }
    else
    {
        /* Else for media read operations we should get success if the 
         * `check checksum' flag is not set.
         */
        if (b_check_checksum == FBE_FALSE)
        {
            *expected_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
            *expected_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
        }
        else
        {
            /* Else if the `check checksum' is set, if the data pattern flags
             * are 0, it should succeed.
             */
            switch(data_pattern_flags)
            {
                case 0:
                    *expected_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
                    *expected_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
                    break;

                case FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC:
                case FBE_DATA_PATTERN_FLAGS_RAID_VERIFY:
                case FBE_DATA_PATTERN_FLAGS_DATA_LOST:
                    /* All `Corrupt CRC' with `check checksum' should fail.
                     */
                    *expected_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR;
                    *expected_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST;
                    break;

                case FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA:
                    /* The status for a `Corrupt Data' request is based on
                     * raid type.  RAID types with redundancy succeed because the
                     * `Corrupt Data' only corrupts the non-redundant data position.
                     * Therefore only non-redundant raid types will fail.
                     */
                    switch(raid_type)
                    {
                        case FBE_RAID_GROUP_TYPE_RAID0:
                            /* Non-redundant raid types fail.*/
                            *expected_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR;
                            *expected_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST;
                            break;

                        case FBE_RAID_GROUP_TYPE_RAID1:
                        case FBE_RAID_GROUP_TYPE_RAID10:
                        case FBE_RAID_GROUP_TYPE_RAID3:
                        case FBE_RAID_GROUP_TYPE_RAID5:
                        case FBE_RAID_GROUP_TYPE_RAID6:
                            /* Redundant raid types succeed.*/
                            *expected_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
                            *expected_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
                            break;

                        default:
                            /* We don't expect these raid types.*/
                            status = FBE_STATUS_GENERIC_FAILURE;
                            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                                     " test block operation: Unsupported raid type");
                            break;

                    } /* end switch on raid type */
                    break;

                default:
                    /* Unsupported data pattern flags. */
                    status = FBE_STATUS_GENERIC_FAILURE;
                    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                             " test block operation: Unsupported data pattern flags");
                    break;

            } /* end switch data pattern flags*/

        } /* end else if `check checksum' flag is set*/

    } /* end else if non-media modify operation */

    /* Always return the status
     */
    return status;
}
/**************************************************************
 * end of fbe_test_sep_io_get_expected_block_operation_status()
 **************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_get_expected_corrupt_bitmap()
 *****************************************************************************
 *
 * @brief   Based on the opcode, raid type, data pattern flags and `check
 *          checksum' values, determine the expected corrupt bitmap.
 *
 * @param   block_opcode - The FBE block operation code for write request
 * @param   raid_type - Used to make a decision on expected `Corrupt Data' status
 * @param   data_pattern_flags - Special data pattern flags for request
 * @param   b_check_checksum - FBE_TRUE - Issue the original request with the
 *                                  `check checksum' flag set in the payload.
 *                             FBE_FALSE - Clear the `check checksum' flag in the
 *                                  payload.
 * @param   expected_corrupted_blocks_bitmap_p - Pointer to expected corrupt bitmap 
 *                              that might be updated.
 *
 * @return  status  - Typically FBE_STATUS_OK
 *
 * @author
 *  09/27/2011  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_io_get_expected_corrupt_bitmap(fbe_payload_block_operation_opcode_t block_opcode,
                                               fbe_raid_group_type_t raid_type,
                                               fbe_data_pattern_flags_t data_pattern_flags,
                                               fbe_bool_t b_check_checksum,
                                               fbe_u64_t *expected_corrupted_blocks_bitmap_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Clear any extra data pattern flags.
     */
    data_pattern_flags &= ~(FBE_DATA_PATTERN_FLAGS_DO_NOT_CHECK_INVALIDATION_REASON);

    /* First determine if this is a media modify operation.  For all media
     * modify operation we expect success.
     */
    if (fbe_payload_block_operation_opcode_is_media_modify(block_opcode) == FBE_TRUE)
    {
        /* Leave corrupt bitmap alone
         */
    }
    else
    {
        /* Else for media read operations we should get success if the 
         * `check checksum' flag is not set.
         */
        if (b_check_checksum == FBE_FALSE)
        {
            /* Leave corrupt bitmap alone
             */
        }
        else
        {
            /* Else if the `check checksum' is set, if the data pattern flags
             * are 0, it should succeed.
             */
            switch(data_pattern_flags)
            {
                case 0:
                    /* Leave corrupt bitmap alone
                     */
                    break;

                case FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC:
                case FBE_DATA_PATTERN_FLAGS_RAID_VERIFY:
                case FBE_DATA_PATTERN_FLAGS_DATA_LOST:
                    /* Leave corrupt bitmap alone
                     */
                    break;

                case FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA:
                    /* The status for a `Corrupt Data' request is based on
                     * raid type.  RAID types with redundancy succeed because the
                     * `Corrupt Data' only corrupts the non-redundant data position.
                     * Therefore only non-redundant raid types will fail.
                     */
                    switch(raid_type)
                    {
                        case FBE_RAID_GROUP_TYPE_RAID0:
                            /* Leave corrupt bitmap alone
                             */
                            break;

                        case FBE_RAID_GROUP_TYPE_RAID1:
                        case FBE_RAID_GROUP_TYPE_RAID10:
                        case FBE_RAID_GROUP_TYPE_RAID3:
                        case FBE_RAID_GROUP_TYPE_RAID5:
                        case FBE_RAID_GROUP_TYPE_RAID6:
                            /* Redundant raid types succeed.  Therefore no 
                             * blocks will be corrupted.
                             */
                            *expected_corrupted_blocks_bitmap_p = 0;
                            break;

                        default:
                            /* We don't expect these raid types.*/
                            status = FBE_STATUS_GENERIC_FAILURE;
                            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                                     " test block operation: Unsupported raid type");
                            break;

                    } /* end switch on raid type */
                    break;

                default:
                    /* Unsupported data pattern flags. */
                    status = FBE_STATUS_GENERIC_FAILURE;
                    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                             " test block operation: Unsupported data pattern flags");
                    break;

            } /* end switch data pattern flags*/

        } /* end else if `check checksum' flag is set*/

    } /* end else if non-media modify operation */

    /* Always return the status
     */
    return status;
}
/**************************************************************
 * end of fbe_test_sep_io_get_expected_corrupt_bitmap()
 **************************************************************/

/*!**************************************************************
 * fbe_test_sep_io_block_operation_free_resources()
 ****************************************************************
 * @brief
 *  Free the memory allocated
 *
 * @param read_memory_ptr
 * @param write_memory_ptr
 *
 * @return None.
 *
 * @author
 *  8/1/2010 - Created. Swati Fursule
 *
 ****************************************************************/
static void fbe_test_sep_io_block_operation_free_resources(fbe_u8_t **read_memory_ptr,
                                                           fbe_u8_t **write_memory_ptr)
{
    if(*read_memory_ptr != NULL)
    {
        fbe_api_free_memory(*read_memory_ptr);
        *read_memory_ptr = NULL;
    }
    if(*write_memory_ptr != NULL)
    {
        fbe_api_free_memory(*write_memory_ptr);
        *write_memory_ptr = NULL;
    }
    return;
}
/******************************************************
 * end fbe_test_sep_io_block_operation_free_resources()
 *****************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_block_operation_check_data()
 *****************************************************************************
 *
 * @brief   Check the data read back.  For either corrupt data or corrupt CRC
 *          we also issue the read without the check checksum set.
 *
 * @param   object_id - The object id to send the write/read/compare request to
 * @param   lba - the lba of the ibject to send request to
 * @param   block_count - The number of blocks to write/read/compare
 * @param   block_size - The block size of the object being tested
 * @param   optimum_block_size - The optimal block size of the objectg being tested
 * @param   raid_type - Used to make a decision on expected `Corrupt Data' status
 * @param   read_memory_pp - Address of read pointer to update
 * @param   write_memory_pp - Address of write pointer if error occurs
 * @param   pattern - The dasta pattern used for the write
 * @param   data_pattern_flags - The data pattern flags used for the write
 * @param   corrupted_blocks_bitmap - The `corrupt' blocks bitmap generated for the
 *              write request.
 *
 * @return  status  - Typically FBE_STATUS_OK
 *
 * @author
 *  8/1/2010 - Created. Swati Fursule
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_io_block_operation_check_data(fbe_object_id_t object_id,
                                                         fbe_lba_t lba,
                                                         fbe_block_count_t block_count,
                                                         fbe_block_size_t block_size,
                                                         fbe_block_size_t optimum_block_size,
                                                         fbe_raid_group_type_t raid_type,
                                                         fbe_u8_t  **read_memory_pp,
                                                         fbe_u8_t  **write_memory_pp,
                                                         fbe_data_pattern_t pattern,
                                                         fbe_data_pattern_flags_t data_pattern_flags,
                                                         fbe_u64_t corrupted_blocks_bitmap)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u64_t                       header_array[FBE_DATA_PATTERN_MAX_HEADER_PATTERN];
    fbe_sg_element_t                read_sg_list[FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS+1] = {0};
    fbe_u8_t                       *read_memory_ptr = NULL;
    fbe_sg_element_t               *sg_p;
    fbe_package_id_t                package_id = FBE_PACKAGE_ID_SEP_0;
    fbe_payload_block_operation_t   block_operation;
    fbe_block_transport_block_operation_info_t block_operation_info;
    fbe_raid_verify_error_counts_t  verify_err_counts={0};
    fbe_data_pattern_info_t         data_pattern_info = {0};
    fbe_payload_block_operation_status_t expected_read_with_check_checksum_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
    fbe_payload_block_operation_qualifier_t expected_read_with_check_checksum_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
    fbe_u32_t                       bytes_to_allocate = 0;
    fbe_bool_t                      b_did_write_have_corrupt_blocks = FBE_FALSE;
    fbe_bool_t                      b_check_checksum = FBE_FALSE;

    /*! @note Currently the maximum request size in bytes is 2^32
     */
    if ((fbe_u64_t)(block_count * block_size) > FBE_U32_MAX)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                 " Transfer size to large");
        return status;
    }
    bytes_to_allocate = (fbe_u32_t)(block_count * block_size);

    /* For any corrupt operation we expect errors when the `check checksum' 
     * flag is set.
     */
    b_did_write_have_corrupt_blocks = (corrupted_blocks_bitmap != 0) ? FBE_TRUE : FBE_FALSE;


    /* For all other operations (including zero) read and compare the data
     */
    read_memory_ptr = (fbe_u8_t*)fbe_api_allocate_memory(bytes_to_allocate);
    MUT_ASSERT_NOT_NULL(read_memory_ptr);
    *read_memory_pp = read_memory_ptr;

    /* Fill the memory with the standard pattern
     */
    sg_p = (fbe_sg_element_t*)read_sg_list;
    status = fbe_data_pattern_sg_fill_with_memory(sg_p,
                                    read_memory_ptr,
                                    block_count,
                                    block_size,
                                    FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                    FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG);
    if( status != FBE_STATUS_OK)
    {
        fbe_test_sep_io_block_operation_free_resources(read_memory_pp,
                                                       write_memory_pp);
    }
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                 " Failed to fill read memory ");

    /* Turns out that the data pattern information is updated.  Need to reset
     * back to the expected pattern (specifically starting lba).
     */
    header_array[0] = FBE_TEST_HEADER_PATTERN;
    header_array[1] = object_id;
    status = fbe_data_pattern_build_info(&data_pattern_info,
                                         pattern,
                                         data_pattern_flags,
                                         lba,
                                         0x55, /*sequence_id*/
                                         2, /*num_header_words*/
                                         header_array);
    if( status != FBE_STATUS_OK)
    {
        fbe_test_sep_io_block_operation_free_resources(read_memory_pp,
                                                       write_memory_pp);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                 " Failed to build read pattern info ");
    }

    /* Now build the block operation for the read request
     */
    status = fbe_payload_block_build_operation(&block_operation,
                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                         lba,
                         block_count,
                         block_size,
                         optimum_block_size,
                         NULL);
    if( status != FBE_STATUS_OK)
    {
        fbe_test_sep_io_block_operation_free_resources(read_memory_pp,
                                                       write_memory_pp);
    }
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                       " Error while building read block operation");

    /* The first read is sent with the `check checksum' flag set to
     * FBE_FALSE.
     */
    status = fbe_test_sep_io_send_block_operation(object_id,
                                                  package_id,
                                                  &block_operation,
                                                  &block_operation_info,
                                                  &verify_err_counts,
                                                  sg_p,
                                                  b_check_checksum);
    if( status != FBE_STATUS_OK)
    {
        fbe_test_sep_io_block_operation_free_resources(read_memory_pp,
                                                       write_memory_pp);
    }

    /* Validate that the request was sent successfully and that read
     * block operations status and qualifier are as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                             " Error while sending read block operation");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, 
                             block_operation_info.block_operation.status,
                             " Error found in read block operation");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, 
                             block_operation_info.block_operation.status_qualifier,
                             " Error found in read block operation");

    /* Now validate the data after writing and then reading it back.
     * For zero operations we validate the data is zeros.  For all other
     * operations we validate the expected data pattern (populated for the write
     * above).
     */
    status = fbe_data_pattern_check_sg_list(sg_p, /*read sg list*/
                                            &data_pattern_info,
                                            block_size,
                                            corrupted_blocks_bitmap,
                                            object_id,
                                            FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                            FBE_TRUE /* panic on data miscompare */);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                             " Error while Checking sg list operation");

    /* If there were any purposefully corrupted blocks in the write data
     * re-issue the read with the `check checksum' flag set.
     */
    if (b_did_write_have_corrupt_blocks == FBE_TRUE)
    {
        /* Fill the memory with the standard pattern
         */
        sg_p = (fbe_sg_element_t*)read_sg_list;
        status = fbe_data_pattern_sg_fill_with_memory(sg_p,
                                                      read_memory_ptr,
                                                      block_count,
                                                      block_size,
                                                      FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                                      FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG);
        if( status != FBE_STATUS_OK)
        {
            fbe_test_sep_io_block_operation_free_resources(read_memory_pp,
                                                           write_memory_pp);
        }
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                 " Failed to fill read memory ");

        /* Turns out that the data pattern information is updated.  Need to reset
         * back to the expected pattern (specifically starting lba).
         */
        header_array[0] = FBE_TEST_HEADER_PATTERN;
        header_array[1] = object_id;
        status = fbe_data_pattern_build_info(&data_pattern_info,
                                             pattern,
                                             data_pattern_flags,
                                             lba,
                                             0x55, /*sequence_id*/
                                             2, /*num_header_words*/
                                             header_array);
        if( status != FBE_STATUS_OK)
        {
            fbe_test_sep_io_block_operation_free_resources(read_memory_pp,
                                                           write_memory_pp);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                 " Failed to build read pattern info ");
        }

        /* Now build the block operation for the read request
         */
        status = fbe_payload_block_build_operation(&block_operation,
                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                         lba,
                         block_count,
                         block_size,
                         optimum_block_size,
                         NULL);
        if( status != FBE_STATUS_OK)
        {
            fbe_test_sep_io_block_operation_free_resources(read_memory_pp,
                                                           write_memory_pp);
        }
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                       " Error while building read block operation");

        /* Now set the `check checksum' flag
         */
        b_check_checksum = FBE_TRUE;
        fbe_payload_block_set_flag(&block_operation,
                                   FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM);

        /* Populate the expected status and qualifier
         */
        status = fbe_test_sep_io_get_expected_block_operation_status(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                                     raid_type,
                                                                     data_pattern_flags,
                                                                     b_check_checksum,
                                                                     &expected_read_with_check_checksum_status,
                                                                     &expected_read_with_check_checksum_qualifier);
        if( status != FBE_STATUS_OK)
        {
            fbe_test_sep_io_block_operation_free_resources(read_memory_pp,
                                                           write_memory_pp);
        }
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                       " Get expected block operation status failed");

        /* Now complete the block operation and send it.
         * This is with b_check_checksum set.
         */
        status = fbe_test_sep_io_send_block_operation(object_id,
                                                      package_id,
                                                      &block_operation,
                                                      &block_operation_info,
                                                      &verify_err_counts,
                                                      sg_p,
                                                      b_check_checksum);
        if( status != FBE_STATUS_OK)
        {
            fbe_test_sep_io_block_operation_free_resources(read_memory_pp,
                                                           write_memory_pp);
        }

        /* Validate that the request was sent successfully and that read
         * block operations status and qualifier are as expected.
         */
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                 " Error while sending read block operation");
        MUT_ASSERT_INT_EQUAL_MSG(expected_read_with_check_checksum_status, 
                                 block_operation_info.block_operation.status,
                                 " Error found in read block operation");
        MUT_ASSERT_INT_EQUAL_MSG(expected_read_with_check_checksum_qualifier, 
                                 block_operation_info.block_operation.status_qualifier,
                                 " Error found in read block operation");

        /* Now validate the data after writing and then reading it back.
         * For zero operations we validate the data is zeros.  For all other
         * operations we validate the expected data pattern (populated for the write
         * above).
         */
        status = fbe_test_sep_io_get_expected_corrupt_bitmap(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                             raid_type,
                                                             data_pattern_flags,
                                                             b_check_checksum,
                                                             &corrupted_blocks_bitmap);
        if( status != FBE_STATUS_OK)
        {
            fbe_test_sep_io_block_operation_free_resources(read_memory_pp,
                                                           write_memory_pp);
        }
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                       " Get expected corrupt bitmap failed");

        /* Now issue the check data pattern request
         */
        status = fbe_data_pattern_check_sg_list(sg_p, /*read sg list*/
                                            &data_pattern_info,
                                            block_size,
                                            corrupted_blocks_bitmap,
                                            object_id,
                                            FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                            FBE_TRUE /* panic on data miscompare */);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                             " Error while Checking sg list operation");

    } /* end if there were corrupt blocks in the write data */

    /* Return the status
     */
    return status;
}
/**************************************************
 * end fbe_test_sep_io_block_operation_check_data()
 **************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_test_block_operation()
 *****************************************************************************
 *
 * @brief   Run block operations over raid group/LUN.  The method purposefully
 *          does NOT use rdgen so we directly test send the requested
 *          operation thru the block transport.
 *
 * @param   object_id - The object id to send the write/read/compare request to
 * @param   block_opcode - The FBE block operation code for write request
 * @param   lba - the lba of the ibject to send request to
 * @param   block_count - The number of blocks to write/read/compare
 * @param   block_size - The block size of the object being tested
 * @param   optimum_block_size - The optimal block size of the objectg being tested
 * @param   raid_type - Used to make a decision on expected `Corrupt Data' status
 * @param   data_pattern_flags - Special data pattern flags for request
 * @param   b_check_checksum - FBE_TRUE - Issue the original request with the
 *                                  `check checksum' flag set in the payload.
 *                             FBE_FALSE - Clear the `check checksum' flag in the
 *                                  payload.
 *
 * @return  status  - Typically FBE_STATUS_OK
 *
 * @author
 *  8/1/2010 - Created. Swati Fursule
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_io_test_block_operation(fbe_object_id_t object_id,
                                  fbe_payload_block_operation_opcode_t block_opcode,
                                  fbe_lba_t lba,
                                  fbe_block_count_t block_count,
                                  fbe_block_size_t block_size,
                                  fbe_block_size_t optimum_block_size,
                                  fbe_raid_group_type_t raid_type,
                                  fbe_data_pattern_flags_t data_pattern_flags,
                                  fbe_bool_t b_check_checksum)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u64_t                       header_array[FBE_DATA_PATTERN_MAX_HEADER_PATTERN];
    fbe_sg_element_t                write_sg_list[FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS+1] = {0};
    fbe_u8_t                       *read_memory_ptr =NULL;
    fbe_u8_t                       *write_memory_ptr=NULL;
    fbe_sg_element_t               *sg_p;
    fbe_package_id_t                package_id = FBE_PACKAGE_ID_SEP_0;
    fbe_payload_block_operation_t   block_operation;
    fbe_block_transport_block_operation_info_t block_operation_info;
    fbe_raid_verify_error_counts_t  verify_err_counts={0};
    fbe_data_pattern_info_t         data_pattern_info = {0};
    fbe_u64_t                       corrupted_blocks_bitmap = 0;
    fbe_data_pattern_t              pattern = FBE_DATA_PATTERN_LBA_PASS;
    fbe_u32_t                       bytes_to_allocate = 0;
    fbe_u32_t                       block_index;
    fbe_u32_t                       i = 0;

    /*! @note Currently the maximum request size in bytes is 2^32
     */
    if ((fbe_u64_t)(block_count * block_size) > FBE_U32_MAX)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                 " Transfer size to large");
        return status;
    }
    bytes_to_allocate = (fbe_u32_t)(block_count * block_size);

    /* Only verify and rebuild do not require data buffers.  All other
     * operations require data buffers for the write and read requests.
     */
    if ((block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY)  &&
        (block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD)    )
    {
        /* Validate supported data pattern flags
         */
        switch(data_pattern_flags)
        {   
            case FBE_DATA_PATTERN_FLAGS_INVALID:
                break;

            case FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA:
                /* we always correct the whole things, no pick and choose here */
                corrupted_blocks_bitmap = -1;
                break;
            case FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC:
            case FBE_DATA_PATTERN_FLAGS_DATA_LOST:
            case FBE_DATA_PATTERN_FLAGS_RAID_VERIFY:
                /* Only the above, single bit flags are supported.  Randomize the
                 * `Corrupted' blocks bitmap.
                 */
                do 
                {
                    corrupted_blocks_bitmap = fbe_random_64();
                    /* Make sure we corrupt at least one block */
                    block_index = 0;
                    while (block_index < block_count)
                    {
                        if (((fbe_u64_t)1 << (block_index % (sizeof(fbe_u64_t) * BITS_PER_BYTE))) & corrupted_blocks_bitmap)
                        {
                            break;  // found one
                        }
                        block_index++;
                    }
                    if (block_index<block_count)
                    {
                        break;  // found valid block to corrupt
                    }
                    corrupted_blocks_bitmap /= 2;
                    /* if we can't find match, let's start over again. */
                    if (corrupted_blocks_bitmap == 0)
                    {
                        corrupted_blocks_bitmap = fbe_random_64();
                    }
                }
                while (corrupted_blocks_bitmap!=0);

                /*! @todo Currently we cannot randomize the corrupt bitmap for RAID-10.
                 */
                if ((raid_type == FBE_RAID_GROUP_TYPE_RAID10)                        &&
                    (fbe_test_io_block_operations_raid10_random_bitmap == FBE_FALSE)    )
                {
                    /*! @todo Also do not validate the invalidation reason.
                     */
                    corrupted_blocks_bitmap = -1;
                    data_pattern_flags |= FBE_DATA_PATTERN_FLAGS_DO_NOT_CHECK_INVALIDATION_REASON;
                }
                break;

            default:
                status = FBE_STATUS_GENERIC_FAILURE;
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                         " Unsupported data pattern flags ");
                return status;
        }

        /*
         * sg list for write operation
         */
        write_memory_ptr = (fbe_u8_t*)fbe_api_allocate_memory(bytes_to_allocate);
        MUT_ASSERT_NOT_NULL(write_memory_ptr);
        sg_p = (fbe_sg_element_t*)write_sg_list;

        /* First plant the sgls
         */
        status = fbe_data_pattern_sg_fill_with_memory(sg_p,
                                        write_memory_ptr,
                                        block_count,
                                        block_size,
                                        FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                        FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG);
        if( status != FBE_STATUS_OK)
        {
            fbe_test_sep_io_block_operation_free_resources(&read_memory_ptr,
                                                           &write_memory_ptr);
        }
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                 " Failed to fill write memory ");

        /* For zero operations will will expecte zeros.
         */
        if (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO)
        {
            pattern = FBE_DATA_PATTERN_ZEROS;
        }

        /* Fill data pattern information structure
         */
        header_array[0] = FBE_TEST_HEADER_PATTERN;
        header_array[1] = object_id;
        status = fbe_data_pattern_build_info(&data_pattern_info,
                                             pattern,
                                             data_pattern_flags,
                                             lba,
                                             0x55, /*sequence_id*/
                                             2, /*num_header_words*/
                                             header_array);
        if( status != FBE_STATUS_OK)
        {
            fbe_test_sep_io_block_operation_free_resources(&read_memory_ptr,
                                                           &write_memory_ptr);
        }
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                 " Failed to build write pattern info ");


        /* Now populate the sgls and randomly corrupt blocks as requested
         */
        status = fbe_data_pattern_fill_sg_list(sg_p,
                                               &data_pattern_info,
                                               block_size,
                                               corrupted_blocks_bitmap,
                                               FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS);
        if( status != FBE_STATUS_OK)
        {
            fbe_test_sep_io_block_operation_free_resources(&read_memory_ptr,
                                                           &write_memory_ptr);
        }
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                   " Error while Filling sg list operation");
    }
    else
    {
        sg_p = NULL;
    }

    /* Build the block operation for the original request.
     */
    status = fbe_payload_block_build_operation(&block_operation,
                         block_opcode,
                         lba,
                         block_count,
                         block_size,
                         optimum_block_size,
                         NULL);
    if( status != FBE_STATUS_OK)
    {
        fbe_test_sep_io_block_operation_free_resources(&read_memory_ptr,
                                                       &write_memory_ptr);
    }
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
               " Error while building write block operation");

    /* Finish populating the block operation and send it
     */
    status = fbe_test_sep_io_send_block_operation(object_id,
                                                  package_id,
                                                  &block_operation,
                                                  &block_operation_info,
                                                  &verify_err_counts,
                                                  sg_p,
                                                  b_check_checksum);

    /* It is possible for verify to get quiesced errors. So retry.
     */
    while ((i < 30) &&
           (status == FBE_STATUS_QUIESCED || block_operation_info.block_operation.status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED) && 
           (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY))
    {
        fbe_api_sleep(100);
        status = fbe_test_sep_io_send_block_operation(object_id,
                                                      package_id,
                                                      &block_operation,
                                                      &block_operation_info,
                                                      &verify_err_counts,
                                                      sg_p,
                                                      b_check_checksum);
        i++;
    }

    if( status != FBE_STATUS_OK)
    {
        fbe_test_sep_io_block_operation_free_resources(&read_memory_ptr,
                                                       &write_memory_ptr);
    }
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                      " Error while sending write block operation");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, 
                       block_operation_info.block_operation.status,
                      " Error found in write block operation");
    /*MUT_ASSERT_INT_EQUAL_MSG(FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, 
                       block_operation_info.block_operation.status_qualifier,
                      " Error found in write block operation");*/
    
    /* For verify and rebuild we do not validate the data
     */
    if ( (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY) ||
         (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD) )
    {
        fbe_test_sep_io_block_operation_free_resources(&read_memory_ptr,
                                                       &write_memory_ptr);
        return status;
    }

    /* Validate that the request was sent successfully and that read
     * block operations status and qualifier are as expected.
     */
    status = fbe_test_sep_io_block_operation_check_data(object_id,
                                                        lba,
                                                        block_count,
                                                        block_size,
                                                        optimum_block_size,
                                                        raid_type,
                                                        &read_memory_ptr,
                                                        &write_memory_ptr,
                                                        pattern,
                                                        data_pattern_flags,
                                                        corrupted_blocks_bitmap);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                       " Check data failed ");

    /* Now free the memory
     */
    fbe_test_sep_io_block_operation_free_resources(&read_memory_ptr,
                                                   &write_memory_ptr);

    /* Return the test status
     */
    return status;
}
/************************************************ 
 * end of fbe_test_sep_io_test_block_operation()
 ************************************************/

/*!**************************************************************
 * fbe_test_sep_io_run_block_operations_test()
 ****************************************************************
 * @brief
 *  Run block operations over raid group/LUN.
 *
 * @param sequence_config_p
 * @param num_of_blocks
 *
 * @return None.
 *
 * @author
 *  8/1/2010 - Created. Swati Fursule
 *
 ****************************************************************/
static fbe_status_t fbe_test_sep_io_run_block_operations_test(fbe_test_sep_io_sequence_config_t *sequence_config_p,
                                                              fbe_block_count_t num_of_blocks)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_block_count_t               max_support_blocks = 4096;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_block_size_t                block_size;
    fbe_block_size_t                optimal_block_size;
    fbe_block_count_t               block_count;
    fbe_u32_t                       lba = 0;
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    fbe_u32_t                       rg_index, lun_index;
    fbe_u32_t                       raid_group_count;
    fbe_u32_t                       number_of_luns_to_test = 0;
    fbe_raid_group_type_t           raid_type;
    fbe_api_raid_group_get_info_t   rg_info = {0};

    /* Maximum block size for teh block operation is 4096, 
     * hence return if it is greater
     */
    block_count = num_of_blocks;
    if (block_count > max_support_blocks )
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Reducing requested blocks: 0x%llx to supported: 0x%llx", 
               __FUNCTION__, (unsigned long long)block_count,
	       (unsigned long long)max_support_blocks);
        block_count = max_support_blocks;
    }

    /* Initialize our global structure of block operations tested.
     */
    fbe_test_sep_io_init_block_operations_array();

    /* Get the array of raid group configurations
     */
    rg_config_p = sequence_config_p->rg_config_p;
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);


    /* Test all raid groups configured
     */
    for ( rg_index = 0; rg_index < raid_group_count; rg_index ++,
                                                     rg_config_p ++)
    {
        fbe_test_logical_unit_configuration_t* logical_unit_configuration_p = NULL;

        if (!fbe_test_rg_config_is_enabled(rg_config_p))
        {
            rg_config_p++;
            continue;
        }

        /* Get the raid group id
         */
        fbe_api_database_lookup_raid_group_by_number(
            rg_config_p->raid_group_id,
            &rg_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

        /* Raid always exports 520 (FBE_BE_BYTES_PER_BLOCK), with an optimal block size 
         * that is set to the raid group's optimal size, which gets set by the 
         * monitor's "get block size" condition. */
        status = fbe_api_raid_group_get_info(rg_object_id, 
                                             &rg_info,
                                             FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


        /* Get the block size and raid type
         */
        block_size = rg_info.exported_block_size;
        raid_type = rg_info.raid_type;
        optimal_block_size = rg_info.optimal_block_size;

        /* Wait for any verifies to complete.
         */
        status = fbe_test_sep_util_wait_for_raid_group_verify(rg_object_id, FBE_TEST_WAIT_TIMEOUT_SECONDS);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Send I/O to the minimum number of luns in each raid group
         */
        logical_unit_configuration_p =  rg_config_p->logical_unit_configuration_list;
        number_of_luns_to_test = FBE_MIN(rg_config_p->number_of_luns_per_rg, fbe_test_io_block_operations_min_num_of_luns_to_test);
        for (lun_index = 0; lun_index < number_of_luns_to_test; lun_index++)
        {
            /* Now run the test for this test case.
             */
            fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number,
                                                  &lun_object_id);
            MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
            mut_printf(MUT_LOG_TEST_STATUS, "== test write opcodes for LUN: %d object: 0x%x rg: %d object: 0x%x",
                       rg_config_p->logical_unit_configuration_list->lun_number, lun_object_id, 
                       rg_config_p->raid_group_id, rg_object_id);
            /* Test 1:  Standard WRITE operation with `check checksum'
             */
            status = fbe_test_sep_io_test_block_operation(lun_object_id,
                                          FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                          lba,
                                          block_count,
                                          block_size,
                                          optimal_block_size,
                                          raid_type,
                                          0,        /* No special data pattern flags*/
                                          FBE_TRUE  /* Check the checksum of the write data */);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Test 2:  Standard WRITE operation w/o `check checksum' flag
             */
            status = fbe_test_sep_io_test_block_operation(lun_object_id,
                                          FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                          lba,
                                          block_count,
                                          block_size,
                                          optimal_block_size,
                                          raid_type,
                                          0,        /* No special data pattern flags*/
                                          FBE_FALSE /* Don't check the checksum of the write data */);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Test 3:  Standard Non-Cached WRITE operation
             */
            status = fbe_test_sep_io_test_block_operation(lun_object_id,
                                          FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED,
                                          lba,
                                          block_count,
                                          block_size,
                                          optimal_block_size,
                                          raid_type,
                                          0,        /* No special data pattern flags*/
                                          FBE_TRUE  /* Check the checksum of the write data */);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Test 4:  Standard Non-Cached WRITE operation w/o `check checksum'
             */
            status = fbe_test_sep_io_test_block_operation(lun_object_id,
                                          FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED,
                                          lba,
                                          block_count,
                                          block_size,
                                          optimal_block_size,
                                          raid_type,
                                          0,        /* No special data pattern flags*/
                                          FBE_FALSE /* Don't the checksum of the write data */);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            mut_printf(MUT_LOG_TEST_STATUS, "== test write opcodes..Done for LUN: %d object: 0x%x rg: %d object: 0x%x",
                       rg_config_p->logical_unit_configuration_list->lun_number, lun_object_id, 
                       rg_config_p->raid_group_id, rg_object_id);

            /* There are current (3) `invalidated' data patterns that can be 
             * received by SEP in write data:
             *  o `Data Lost'   - `Data Lost - Invalidated' without events
             *  o `Corrupt CRC' - `Data Lost - Invalidated' with events
             *  o `RAID Verify' - `RAID Verify - Invalidated' with events
             */

            /*! @note Corrupt data is enabled
             */
            /* Test 5:  `Corrupt Data' operation
             */
            if (fbe_test_io_block_operations_enable_corrupt_data == FBE_TRUE)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "== test corrupt data for LUN: %d object: 0x%x rg: %d object: 0x%x",
                       rg_config_p->logical_unit_configuration_list->lun_number, lun_object_id, 
                       rg_config_p->raid_group_id, rg_object_id);
                if (raid_type == FBE_RAID_GROUP_TYPE_RAID1 || raid_type == FBE_RAID_GROUP_TYPE_RAID10)
                {
                    /* Set mirror perfered position to disable mirror read optimization */
                    status = fbe_api_raid_group_set_mirror_prefered_position(rg_object_id, 1);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                }

                /* Test 5a:  `Corrupt Data' operation w/o `check checksum'
                 */
                status = fbe_test_sep_io_test_block_operation(lun_object_id,
                                            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA,
                                            lba,
                                            64,  // something smaller than element_size so that we don't corrupt more than 1 disk
                                            block_size,
                                            optimal_block_size,
                                            raid_type,
                                            FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA, /* Set `Corrupt Data' flags*/
                                            FBE_FALSE  /* Don't check the checksum of the write data */);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                /*! @note fbe_test_sep_io_block_operation_check_data Issues 
                 */
                /* Wait to make sure all verifies triggered by above operation are completed. */
                status = fbe_test_sep_util_wait_for_raid_group_verify(rg_object_id, FBE_TEST_WAIT_TIMEOUT_SECONDS);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                /* Test 5b: `Corrupt Data' operation with `check checksum'.
                 *           The data wil be re-constructed.
                 */
                status = fbe_test_sep_io_test_block_operation(lun_object_id,
                                            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA,
                                            lba,
                                            64, // something smaller than element_size so that we don't corrupt more than 1 disk
                                            block_size,
                                            optimal_block_size,
                                            raid_type,
                                            FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA, /* Set `Corrupt Data' flags*/
                                            FBE_TRUE  /* Check the checksum of the write data */);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                /* Wait to make sure all verifies triggered by above operation are completed. */
                status = fbe_test_sep_util_wait_for_raid_group_verify(rg_object_id, FBE_TEST_WAIT_TIMEOUT_SECONDS);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                if (raid_type == FBE_RAID_GROUP_TYPE_RAID1 || raid_type == FBE_RAID_GROUP_TYPE_RAID10)
                {
                    /* Set mirror perfered position to enable mirror read optimization */
                    status = fbe_api_raid_group_set_mirror_prefered_position(rg_object_id, 0);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                }
                mut_printf(MUT_LOG_TEST_STATUS, "== test corrupt data...Done for LUN: %d object: 0x%x rg: %d object: 0x%x",
                           rg_config_p->logical_unit_configuration_list->lun_number, lun_object_id, 
                           rg_config_p->raid_group_id, rg_object_id);
            } /*! @note end if `Corrupt Data' testing is enabled */

            mut_printf(MUT_LOG_TEST_STATUS, "== test corrupt crc opcodes. for LUN: %d object: 0x%x rg: %d object: 0x%x",
                       rg_config_p->logical_unit_configuration_list->lun_number, lun_object_id, 
                       rg_config_p->raid_group_id, rg_object_id);
            /* Test 6: `Corrupt CRC' - Write data with `Corrupt CRC' blocks
             */ 
            status = fbe_test_sep_io_test_block_operation(lun_object_id,
                                        FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                        lba,
                                        block_count,
                                        block_size,
                                        optimal_block_size,
                                        raid_type,
                                        FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC, /* Set `Corrupt CRC' flags*/
                                        FBE_TRUE  /* Check the checksum of the write data */);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Wait to make sure all verifies triggered by above operation are completed. */
            status = fbe_test_sep_util_wait_for_raid_group_verify(rg_object_id, FBE_TEST_WAIT_TIMEOUT_SECONDS);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Test 7a: `Data Lost' - Write data with `Data Lost - Invalidated' blocks
             */ 
            status = fbe_test_sep_io_test_block_operation(lun_object_id,
                                        FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                        lba,
                                        block_count,
                                        block_size,
                                        optimal_block_size,
                                        raid_type,
                                        FBE_DATA_PATTERN_FLAGS_DATA_LOST, /* Set `Data Lost' flags*/
                                        FBE_TRUE  /* Check the checksum of the write data */);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Wait to make sure all verifies triggered by above operation are completed. */
            status = fbe_test_sep_util_wait_for_raid_group_verify(rg_object_id, FBE_TEST_WAIT_TIMEOUT_SECONDS);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Test 7b: `Data Lost' - Write data with `Data Lost - Invalidated' blocks
             *         Test w/o `check checksum' flag set
             */ 
            status = fbe_test_sep_io_test_block_operation(lun_object_id,
                                        FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                        lba,
                                        block_count,
                                        block_size,
                                        optimal_block_size,
                                        raid_type,
                                        FBE_DATA_PATTERN_FLAGS_DATA_LOST, /* Set `Data Lost' flags*/
                                        FBE_FALSE  /* Don't check the checksum of the write data */);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Wait to make sure all verifies triggered by above operation are completed. */
            status = fbe_test_sep_util_wait_for_raid_group_verify(rg_object_id, FBE_TEST_WAIT_TIMEOUT_SECONDS);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Test 8: `RAID Verify' - Write data with `RAID Verify - Invalidated' blocks
             */ 
            status = fbe_test_sep_io_test_block_operation(lun_object_id,
                                        FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                        lba,
                                        block_count,
                                        block_size,
                                        optimal_block_size,
                                        raid_type,
                                        FBE_DATA_PATTERN_FLAGS_RAID_VERIFY, /* Set `RAID Verify' flags*/
                                        FBE_TRUE  /* Check the checksum of the write data */);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Wait to make sure all verifies triggered by above operation are completed. */
            status = fbe_test_sep_util_wait_for_raid_group_verify(rg_object_id, FBE_TEST_WAIT_TIMEOUT_SECONDS);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            mut_printf(MUT_LOG_TEST_STATUS, "== test corrupt crc opcodes..Done for LUN: %d object: 0x%x rg: %d object: 0x%x",
                       rg_config_p->logical_unit_configuration_list->lun_number, lun_object_id, 
                       rg_config_p->raid_group_id, rg_object_id);

            mut_printf(MUT_LOG_TEST_STATUS, "== test write/verify opcode. for LUN: %d object: 0x%x rg: %d object: 0x%x",
                       rg_config_p->logical_unit_configuration_list->lun_number, lun_object_id, 
                       rg_config_p->raid_group_id, rg_object_id);
            /* Test 9:  Standard VERIFY-WRITE operation
             */
            status = fbe_test_sep_io_test_block_operation(lun_object_id,
                                          FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE,
                                          lba,
                                          block_count,
                                          block_size,
                                          optimal_block_size,
                                          raid_type,
                                          0,        /* No special data pattern flags*/
                                          FBE_TRUE  /* Check the checksum of the write data */);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Wait to make sure all verifies triggered by above operation are completed. */
            status = fbe_test_sep_util_wait_for_raid_group_verify(rg_object_id, FBE_TEST_WAIT_TIMEOUT_SECONDS);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            mut_printf(MUT_LOG_TEST_STATUS, "== test write/verify opcode..Done for LUN: %d object: 0x%x rg: %d object: 0x%x",
                       rg_config_p->logical_unit_configuration_list->lun_number, lun_object_id, 
                       rg_config_p->raid_group_id, rg_object_id);

            mut_printf(MUT_LOG_TEST_STATUS, "== test zero opcode for LUN: %d object: 0x%x rg: %d object: 0x%x",
                       rg_config_p->logical_unit_configuration_list->lun_number, lun_object_id, 
                       rg_config_p->raid_group_id, rg_object_id);
            /* Test 10: Zero the blocks
            */
            status = fbe_test_sep_io_test_block_operation(lun_object_id,
                                          FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO,
                                          lba,
                                          block_count,
                                          block_size,
                                          optimal_block_size,
                                          raid_type,
                                          0,        /* No special data pattern flags*/
                                          FBE_TRUE  /* Check the checksum of the write data */);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            mut_printf(MUT_LOG_TEST_STATUS, "== test zero opcode..Done for LUN: %d object: 0x%x rg: %d object: 0x%x",
                       rg_config_p->logical_unit_configuration_list->lun_number, lun_object_id, 
                       rg_config_p->raid_group_id, rg_object_id);

        } /* end for all luns in the raid groups */

#if 0 /* It is a bug. the function will send Zero request to raid and than will try to read it back. The I/O size is too big */
        /* Issue zero requests.
         */
         status = fbe_test_sep_io_test_block_operation(rg_object_id,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO,
                                      lba,
                                      block_count,
                                      block_size,
                                      optimal_block_size,
                                      raid_type,
                                      0,        /* No special data pattern flags*/
                                      FBE_TRUE  /* Check the checksum of the write data */);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
#endif

    }  /* end for all raid groups */

    /* Now print the test results for this set of raid groups.
     */
    fbe_test_sep_io_print_block_operation_stats(sequence_config_p);

    /* Now validate that all operation codes have been tested.
     */
    status = fbe_test_sep_io_check_block_operation_results(sequence_config_p);

    /* Return the execution status*/
    return status;
}
/*!**************************************************
 * end of fbe_test_sep_io_run_block_operations_test()
 ****************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_run_block_operations_sequence()
 *****************************************************************************
 *
 * @brief
 *  Run block operations over raid group/LUN.
 *
 * @param rg_config_p
 * @param small_io_size
 * @param large_io_size
 *
 * @return None.
 *
 * @author
 *  8/1/2010 - Created. Swati Fursule
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_io_run_block_operations_sequence(fbe_test_sep_io_sequence_config_t *sequence_config_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_test_sep_io_sequence_state_t    sequence_io_state;

    /* First validate that the test sequence has been configured
     */
    sequence_io_state = fbe_test_sep_io_get_sequence_io_state(sequence_config_p);
    MUT_ASSERT_TRUE(sequence_io_state >= FBE_TEST_SEP_IO_SEQUENCE_STATE_IO_READY);

    /* Step 1)  Execute a block op/read/compare I/O for small I/O
     *
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running block op/read/compare small I/Os: %llu max blocks", 
               __FUNCTION__,
	       (unsigned long long)sequence_config_p->max_blocks_for_small_io);
    status = fbe_test_sep_io_run_block_operations_test(sequence_config_p, sequence_config_p->max_blocks_for_small_io);

    /* Make sure there were no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running block op/read/compare small I/Os to all LUNs...Finished", __FUNCTION__);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Step 2)  Execute a block op/read/compare I/O for large I/O
     *
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running block op/read/compare large I/Os: %llu max blocks", 
               __FUNCTION__,
	       (unsigned long long)sequence_config_p->max_blocks_for_large_io);
    status = fbe_test_sep_io_run_block_operations_test(sequence_config_p, sequence_config_p->max_blocks_for_large_io); 
    
    /* Make sure there were no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running block op/read/compare large I/Os to all LUNs...Finished", __FUNCTION__);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return FBE_STATUS_OK;
}
/********************************************************
 * end of fbe_test_sep_io_run_block_operations_sequence()
 ********************************************************/


/************************************************
 * end of file fbe_test_sep_io_block_operations.c
 ************************************************/


