/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file parinacota.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Parinacota test, which is a fbe_test for
 *  injecting media errors.
 *
 * @revision
 *   1/12/2009:  Created. RPF
 *   2/15/2011:  Migrated from zeus_ready_test. Pradnya Patankar
 *   4/30/2014 - Wayne Garrett - Added 4K support and major cleanup.  Moved
 *                     to sep disk_error test suite.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "fbe_test_package_config.h"
#include "fbe_test_io_api.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe_media_error_edge_tap.h"
#include "fbe_test_common_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_protocol_error_injection_service.h"
#include "neit_utils.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "sep_tests.h"  /* neit_config_fill_load_params */


/*-----------------------------------------------------------------------------
 *    TEST DESCRIPTION:
 */
 char * parinacota_short_desc = 
     "Tests media errors (PBA->LBA conversion) for 520 and 4K drives.";
 char * parinacota_long_desc =
     "\n"
     "\n"
     "This test will inject media errors into 520 and 4K drives and verify that PVD converts the PBA to LBA correctly.\n"
     "\n";



#define PARINACOTA_OPTIMAL_4K_CLIENT_BLOCKS  8
#define PARINACOTA_OPTIMAL_520_CLIENT_BLOCKS  1


/*! @enum PARINACOTA_PDO_BLOCKS_REMAPPED_PER_WRITE 
  *  @brief The physical drive will allow this many blocks to get remapped per request.
  */
enum { PARINACOTA_PDO_BLOCKS_REMAPPED_PER_WRITE = 8};


/*! @struct media_error_test_case_t  
 *  @brief This is the structure that defines a test case for
 *         testing media errors.
 */
typedef struct media_error_test_case_s
{    
    fbe_block_size_t exported_block_size;  /*! Size of block exported in bytes. */    
    fbe_block_size_t imported_block_size; /*! The size in bytes of the imported block. */

    fbe_lba_t read_lba; /*!< Lba to read from. */
    fbe_block_count_t read_blocks; /*!< Blocks to read. */

    fbe_lba_t write_lba;/*!< Lba to write to. */
    fbe_block_count_t write_blocks;/*!< Blocks to write. */

    fbe_lba_t error_pba; /*!< Start physical lba to inject errors. */
    fbe_block_count_t error_blocks; /*! Number of blocks to inject errors. */
}
media_error_test_case_t;

/*! @var media_error_520_io_case_array  
 *  @brief This is the set of test cases for testing 520 byte block drives. 
 */
static media_error_test_case_t media_error_520_io_case_array[] =
{
    /* 
     * exp  imp   read read  write  write Error Error 
     * bl   block lba  bls   lba    bls   lba   Bl    
     * size size                                      
     *  
     * Simple small block count error cases. 
     * We try to cover cases where the read starts at the beginning 
     * of the error and where the read starts mid-way through the error. 
     * The write always covers the same range as the read so that the 
     * write will fix all the errors. 
     */


    {520,    520,  0,   1,     0,    1,    0,    1},
    {520,    520,  0,   2,     0,    2,    0,    2},
    {520,    520,  1,   1,     0,    2,    0,    2},
    {520,    520,  1,   1,     1,    1,    1,    1},
    {520,    520,  1,   1,     1,    1,    1,    1},
    {520,    520,  2,   1,     2,    2,    2,    2},
    {520,    520,  2,   2,     2,    2,    2,    2},
    {520,    520,  0,   6,     0,    6,    1,    5},
    {520,    520,  0,   2,     0,    2,    0,    2}, 

    /* 
     * exp  imp   read      read  write    write Error     Error 
     * bl   block lba       bls   lba      bls   lba       Bl    
     * size size                                          
     *  
     * Simple small block count error cases. 
     * We try to cover cases where the read starts at the beginning 
     * of the error and where the read starts mid-way through the error. 
     * The write always covers the same range as the read so that the 
     * write will fix all the errors. 
     */
    {520,    520,  0x1000,   1,  0x1000,    1,    0x1000,    1},
    {520,    520,  0x1000,   2,  0x1000,    2,    0x1000,    2},
    {520,    520,  0x1001,   1,  0x1000,    2,    0x1000,    2},
    {520,    520,  0x1001,   1,  0x1001,    1,    0x1001,    1},
    {520,    520,  0x1001,   1,  0x1001,    1,    0x1001,    1},
    {520,    520,  0x1002,   1,  0x1002,    2,    0x1002,    2},
    {520,    520,  0x1002,   2,  0x1002,    2,    0x1002,    2},
    {520,    520,  0x1000,   6,  0x1000,    6,    0x1001,    5},
    {520,    520,  0x1000,   2,  0x1000,    2,    0x1000,    2}, 

    /* 
     * exp  imp   read read  write  write Error Error 
     * bl   block lba  bls   lba    bls   lba   Bl    
     * size size                            
     *  
     * Larger block count cases. 
     */
    {520,    520,  0,   1,     0,    7,    0,    7},
    {520,    520,  4,   1,     0,    8,    0,    8},
    {520,    520,  3,   6,     0,    9,    0,    9},
    {520,    520,  6,   2,     0,    16,   0,    16},
    {520,    520,  4,   1,     0,    17,   0,    17},
    {520,    520,  5,   64,    0,    128,  5,    64},
    {520,    520,  5,   64,    0,    128,  3,    64},
    {520,    520,  5,   64,    0,    128,  10,   64},
    {520,    520,  5,   64,    0,    128,  20,   64},

    /* 
     * exp  imp   read       read    write     write Error     Error 
     * bl   block lba        bls     lba       bls   lba       Bl 
     * size size 
     *  
     * Larger block count cases. 
     */
    {520,    520,  0x1000,   1,     0x1000,    7,    0x1000,    7},
    {520,    520,  0x1004,   1,     0x1000,    8,    0x1000,    8},
    {520,    520,  0x1003,   6,     0x1000,    9,    0x1000,    9},
    {520,    520,  0x1006,   2,     0x1000,    16,   0x1000,    16},
    {520,    520,  0x1004,   1,     0x1000,    17,   0x1000,    17},
    {520,    520,  0x1005,   64,    0x1000,    128,  0x1005,    64},
    {520,    520,  0x1005,   64,    0x1000,    128,  0x1003,    64},
    {520,    520,  0x1005,   64,    0x1000,    128,  0x1010,    64},
    {520,    520,  0x1005,   64,    0x1000,    128,  0x1020,    64},

    /* This is the terminator record, it always should be zero.
     */
    {FBE_TEST_IO_INVALID_FIELD, 0,         0,    0,    0,       0, 0},
   
};
/* end media_error_520_io_case_array[] */

/*! @var media_error_4k_io_case_array  
 *  @brief This is the set of test cases for testing 4k (4160) byte block drives. 
 */
static media_error_test_case_t media_error_4k_io_case_array[] =
{
    /* 
     * exp  imp   read read  write  write Error Error 
     * bl   block lba  bls   lba    bls   lba   Bl    
     * size size     
     */

    /* small block count cases */   
    {520,   4160,  0,   8,    0,    8,    0,    1},  /* algined read.  error pba 0*/
    {520,   4160,  0,   1,    0,    8,    0,    1},  /* unaligned - 1st 520 pba 0*/
    {520,   4160,  8,   1,    8,    8,    1,    1},  /* unaligned - 1st 520 pba 1*/
    {520,   4160,  5,   1,    0,    8,    0,    1},  /* unaligned - middle 4k block*/
    {520,   4160,  6,   1,    0,    8,    0,    1},  /* unaligned - last 520*/
    {520,   4160,  6,   4,    0,    16,   0,    1},  /* unaligned - overlap front*/
    {520,   4160,  6,   4,    0,    16,   1,    1},  /* unaligned - overlap back*/
    {520,   4160,  6,   16,   0,    24,   0,    1},  /* unaligned both ends - error at beginning*/
    {520,   4160,  6,   16,   0,    24,   1,    1},  /* unaligned both ends - error at middle*/
    {520,   4160,  6,   16,   0,    24,   2,    1},  /* unaligned both ends - error at end*/

    /* 
     * exp  imp   read       read  write      write Error    Error 
     * bl   block lba        bls   lba        bls   lba      Bl 
     * size size                                  
     */

    /* large block count cases */
    {520,   4160,  0x1000,   128,  0x1000,    128,  0x200,   8},  /* algined read.  error pba 0*/
    {520,   4160,  0x1008,   130,  0x1008,    136,  0x200,   9},  /* unaligned - overlap back*/
    {520,   4160,  0x1006,   130,  0x1000,    136,  0x201,   8},  /* unaligned - overlap front*/
    {520,   4160,  0x1006,   136,  0x1000,    144,  0x200,   10},  /* unaligned both ends */ 

    /* This is the terminator record, it always should be zero.
     */
    {FBE_TEST_IO_INVALID_FIELD, 0,         0,    0,    0,       0, 0},

};
/* end media_error_4k_io_case_array[]  */


/*!*******************************************************************
 * @var parinacota_test_rdgen_context
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t parinacota_test_rdgen_context;



/*!**************************************************************
 * @fn parinacota_disable_background_services()
 ****************************************************************
 * @brief
 *   Disables PVD's background services
 * 
 * @param bus, encl, slot - drive location
 * 
 * @author
 *   05/09/2014:  Wayne Garrett - Created.
 *
 ****************************************************************/
void parinacota_disable_background_services(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_status_t status;
    fbe_object_id_t pvd;

    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus, encl, slot, &pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_base_config_disable_background_operation(pvd, FBE_PROVISION_DRIVE_BACKGROUND_OP_ALL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}


/*!**************************************************************
 * @fn parinacota_get_media_error_lba()
 ****************************************************************
 * @brief
 *  Return the client's LBA where the media error occurred.
 *  
 * @param alignment_count - number of blocks to align client LBA with physical LBA.
 * @param io_start_lba - Client's start LBA.
 * @param error_pba - Physical LBA where error occurred.
 * 
 * @return fbe_lba_t - media error LBA.
 *
 * @note  It's required that the clients block size is 
 *        - less than or euqal to physical block size.  
 *        - a multiple of the physical block size. 
 *
 * HISTORY:
 *  5/9/2014 - Created. Wayne Garrett
 *
 ****************************************************************/
fbe_lba_t parinacota_get_media_error_lba(fbe_block_count_t alignment_count, fbe_lba_t io_start_lba,  fbe_lba_t error_pba)
{
    fbe_lba_t error_lba;
    fbe_lba_t aligned_start_lba;

    /* convert error PBA back to client LBA*/
    error_lba = error_pba * alignment_count;

    /* Get the aligned read LBA.  Convert read LBA to physical then back to client LBA.  */
    aligned_start_lba = (io_start_lba/alignment_count) * alignment_count;
    
    /* Can't inject an error before the start lba, so return the one that is greater.*/  
    
    return ( aligned_start_lba > error_lba )? aligned_start_lba : error_lba;
}



/*!**************************************************************
 * @fn parinacota_issue_write_verify(media_error_test_case_t * const case_p,
 *                                   fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  Issue the write and verify command.  This will retry if
 *  any errors are encountered on the initial write.
 *  
 * @param case_p - Test case to execute.
 * @param object_id - Object id to execute for.
 * 
 * @return fbe_status_t - FBE_STATUS_OK on success,
 *                        any other value implies error.
 *
 * HISTORY:
 *  2/15/2011 - Created. Pradnya Patankar
 *
 ****************************************************************/

fbe_status_t parinacota_issue_write_verify(media_error_test_case_t * const case_p, fbe_object_id_t pdo, fbe_object_id_t pvd, fbe_block_size_t opt_exp_blk_size)
{
    fbe_status_t status;
    fbe_lba_t current_error_pba;    
    fbe_api_rdgen_context_t *context_p = &parinacota_test_rdgen_context;
    fbe_rdgen_options_t rdgen_options = FBE_RDGEN_OPTIONS_DO_NOT_ALIGN_TO_OPTIMAL;
    fbe_lba_t media_error_lba;

    /* Tracks the number of errors left to remap.
     */
    fbe_block_count_t error_blocks_remaining;

    /* Now issue a write/verify to the same range.
     * We expect this to fail with a media error for block_count - 1. 
     * We will continue retrying this write/verify until it succeeds. 
     */
    error_blocks_remaining = case_p->error_blocks;
    current_error_pba = case_p->error_pba;

    while (error_blocks_remaining > 0)
    {
        status = fbe_api_physical_drive_clear_error_counts(pdo, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_rdgen_send_one_io(context_p,
                                           pvd,                                             // object id
                                           FBE_CLASS_ID_INVALID,                            // class id
                                           FBE_PACKAGE_ID_SEP_0,                            // package id
                                           FBE_RDGEN_OPERATION_WRITE_VERIFY_READ_CHECK,     // rdgen opcode
                                           FBE_RDGEN_PATTERN_LBA_PASS,                      // data pattern to generate
                                           case_p->write_lba,                               // start lba
                                           case_p->write_blocks,                            // min blocks
                                           rdgen_options,                                   // rdgen options
                                           0,0,
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        
        if (error_blocks_remaining <= PARINACOTA_PDO_BLOCKS_REMAPPED_PER_WRITE)
        {
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        else
        {
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);	
        }
        /* Decrement the number of errors remaining.
         * The physical drive will allow this many blocks to get remapped per request.
         */
        if (error_blocks_remaining < PARINACOTA_PDO_BLOCKS_REMAPPED_PER_WRITE)
        {
            error_blocks_remaining = 0;
        }
        else
        {
            error_blocks_remaining -= PARINACOTA_PDO_BLOCKS_REMAPPED_PER_WRITE;
            current_error_pba += PARINACOTA_PDO_BLOCKS_REMAPPED_PER_WRITE;

        }

        if (error_blocks_remaining == 0)
        {
            /* On the last iteration we expect the write/verify to succeed.
            */
            MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
            MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
        }
        else
        {
            /* If we still have errors remaining, then make sure we received a media error
            * status. 
            */            
            MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
            MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);     
            
            media_error_lba = parinacota_get_media_error_lba(opt_exp_blk_size, case_p->read_lba, current_error_pba);
            MUT_ASSERT_INTEGER_EQUAL(context_p->start_io.status.media_error_lba, media_error_lba);
        }
    } /* End while we still have errors. */

    return FBE_STATUS_OK;
}
/**************************************
 * end parinacota_issue_write_verify()
 **************************************/

/*!**************************************************************
 * @fn parinacota_inject_scsi_error_start(media_error_test_case_t *const case_p,
 *                          fbe_u32_t slot)
 ****************************************************************
 * @brief
 *   This function starts the protocol error injection.
 *  
 * @param case_p - The test case which contains the area to
 *                 inject errors on, to read from and write to.
 * @param record_p - error record to be inserted.
 * @param pdo_object_id - pdo object on which error will be inserted.
 *
 * @return - fbe_status_t - FBE_STATUS_OK on success,
 *                        any other value implies error.
 *
 * HISTORY:
 *  2/15/2011 - Created. Pradnya Patankar
 *
 ****************************************************************/
fbe_status_t parinacota_inject_scsi_error_start(media_error_test_case_t *const case_p,
                                                fbe_protocol_error_injection_error_record_t* record_p,
                                                fbe_object_id_t pdo_object_id)
{
    fbe_u32_t i;
    fbe_protocol_error_injection_record_handle_t record_handle = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    
    /* Initialize the protocol error injection record.
     */
    fbe_test_neit_utils_init_error_injection_record(record_p);

    /* Put PDO object ID into the error record
     */
    record_p->object_id = pdo_object_id;

    /* Fill in the fields in the error record that are common to all
     * the error cases.
     */
    record_p->protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
    record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.port_status =
        FBE_PORT_REQUEST_STATUS_SUCCESS;

    /* Inject the error
     */
    record_p->lba_start = case_p->error_pba;
    record_p->lba_end = (case_p->error_pba + case_p->error_blocks) ;
    record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
        FBE_SCSI_READ_10;
    record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[1] = 
        FBE_SCSI_WRITE_VERIFY;
    record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[2] = 
        FBE_SCSI_REASSIGN_BLOCKS;
    record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = 
        0x03 /* FBE_SCSI_SENSE_KEY_MEDIUM_ERROR */;
    record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code = 
        ( 0x011 )/* (FBE_SCSI_ASC_READ_DATA_ERROR )*/;
    record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = 
        0x00; 
    record_p->num_of_times_to_insert = 1;
    record_p->reassign_count = 0;

    for(i =0; i <PROTOCOL_ERROR_INJECTION_MAX_ERRORS ; i++)
    {
        if (i <= (record_p->lba_end - record_p->lba_start)) 
        {
            record_p->b_active[i] = FBE_TRUE;
        }
    } 

     record_p->b_test_for_media_error = FBE_TRUE;
    
    /* Add the error record
     */
    status = fbe_api_protocol_error_injection_add_record(record_p, &record_handle);	
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return FBE_STATUS_OK;
}
/**************************************
 * end parinacota_inject_scsi_error_start()
 **************************************/


/*!**************************************************************
 * @fn parinacota_inject_scsi_error_stop()
 ****************************************************************
 * @brief
 *   This function stops the protocol error injection.
 *  
 * @return - fbe_status_t - FBE_STATUS_OK on success,
 *                        any other value implies error.
 *
 * HISTORY:
 *  2/15/2011 - Created. Pradnya Patankar
 *
 ****************************************************************/
fbe_status_t parinacota_inject_scsi_error_stop(void)
{
    fbe_status_t   status = FBE_STATUS_OK;

    /*set the error using NEIT package. */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: stop error injection.", __FUNCTION__);

    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    return status;
}
/**************************************
 * end parinacota_inject_scsi_error_stop()
 **************************************/


/*!**************************************************************
 * @fn parinacota_test_case(media_error_test_case_t *const case_p,
 *                          fbe_u32_t slot)
 ****************************************************************
 * @brief
 *   This tests a single case of the parinacota test.
 *   We insert an error, read from an area, write to an area
 *   and then read back the data written.
 *  
 * @param case_p - The test case which contains the area to
 *                 inject errors on, to read from and write to.
 * @param slot - The drive slot to test.  The test setup two
 *               drives on slots 0, 1.
 *
 * @return - None.
 *
 *
 ****************************************************************/

void parinacota_test_case(media_error_test_case_t *const case_p, fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, 
                              fbe_block_size_t opt_exp_blk_size)
{
    fbe_status_t status;
    fbe_object_id_t pdo;
    fbe_object_id_t pvd;    
    fbe_protocol_error_injection_error_record_t record;
    fbe_api_rdgen_context_t *context_p = &parinacota_test_rdgen_context;
    fbe_api_dieh_stats_t dieh_stats;
    fbe_rdgen_options_t rdgen_options = FBE_RDGEN_OPTIONS_DO_NOT_ALIGN_TO_OPTIMAL;
    fbe_lba_t media_error_lba;

    
    /* Get the physical drive object id.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(bus, encl, slot, &pvd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Paint the lba so that the paged is not set to need zero
     */
    status = fbe_api_rdgen_send_one_io(context_p,
                                       pvd,                                              // object id
                                       FBE_CLASS_ID_INVALID,                             // class id
                                       FBE_PACKAGE_ID_SEP_0,                      // package id
                                       FBE_RDGEN_OPERATION_WRITE_ONLY,          // rdgen opcode
                                       FBE_RDGEN_PATTERN_LBA_PASS,                // data pattern to generate
                                       case_p->write_lba,                                            // start lba
                                       case_p->write_blocks,                               // min blocks
                                       rdgen_options,                   // rdgen options
                                       0,0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_HIGH, "%s %s start error injection!",__FILE__, __FUNCTION__);

    /* Inject scsi error
     */
    status = parinacota_inject_scsi_error_start(case_p, &record, pdo);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Start the error injection 
     */
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Issue read.  We expect this to get a media error because of the error injected
     * above. 
     */
    status = fbe_api_rdgen_send_one_io(context_p,
                                    pvd,                                              // object id
                                    FBE_CLASS_ID_INVALID,                             // class id
                                    FBE_PACKAGE_ID_SEP_0,                      // package id
                                    FBE_RDGEN_OPERATION_READ_ONLY,          // rdgen opcode
                                    FBE_RDGEN_PATTERN_LBA_PASS,                // data pattern to generate
                                    case_p->read_lba,                                     // start lba
                                    case_p->read_blocks,                                // min blocks
                                    rdgen_options,                   // rdgen options
                                    0,0,
                                    FBE_API_RDGEN_PEER_OPTIONS_INVALID);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);

    media_error_lba = parinacota_get_media_error_lba(opt_exp_blk_size, case_p->read_lba, case_p->error_pba);
    MUT_ASSERT_INTEGER_EQUAL(context_p->start_io.status.media_error_lba, media_error_lba);

    mut_printf(MUT_LOG_LOW, "=== %s:  initial read passed. ===", __FUNCTION__);
    
    /* Issue write/verify/read check.
     */
    mut_printf(MUT_LOG_LOW, "=== %s:  starting write_verify to %d byte per block drive. ===", __FUNCTION__, case_p->imported_block_size);

    status = parinacota_issue_write_verify(case_p, pdo, pvd, opt_exp_blk_size);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== %s:  write_verify to %d byte per block drive passed. ===", __FUNCTION__, case_p->imported_block_size);
    mut_printf(MUT_LOG_LOW, "=== %s:  read test passed. ===", __FUNCTION__);

    /* Get DIEH information. */
    status = fbe_api_physical_drive_get_dieh_info(pdo, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_LOW, "%s: get remapped block counts: %d.", 
                   __FUNCTION__, dieh_stats.error_counts.physical_drive_error_counts.remap_block_count); 
    mut_printf(MUT_LOG_LOW,"%s: get media error ratio: %d.",
                 __FUNCTION__, dieh_stats.error_counts.media_error_ratio);

    /* Issue read/check to make sure the write succeeded.
     * We do not expect an error since all of the blocks should have been remapped. 
     * We read back in exactly what we wrote in order to check the write. 
     */

    status = fbe_api_rdgen_send_one_io(context_p,
                                pvd,                                                      // object id
                                FBE_CLASS_ID_INVALID,                                     // class id
                                FBE_PACKAGE_ID_SEP_0,                              // package id
                                FBE_RDGEN_OPERATION_READ_CHECK,               // rdgen opcode
                                FBE_RDGEN_PATTERN_LBA_PASS,                        // data pattern to generate
                                case_p->write_lba,                                            // start lba
                                case_p->write_blocks,                                       // blocks
                                rdgen_options,                          // rdgen options
                                0,0,
                                FBE_API_RDGEN_PEER_OPTIONS_INVALID);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    if (context_p->start_io.status.block_qualifier!= FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_COMPLETE_WITH_REMAP)
    {
         MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier,FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    }

    status = parinacota_inject_scsi_error_stop();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_protocol_error_injection_remove_all_records();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_physical_drive_clear_error_counts(pdo, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    mut_printf(MUT_LOG_LOW, "=== %s: successfully completed test for pdo:0x%x pvd:0x%x. ===\n", __FUNCTION__, pdo, pvd);
    return;
}
/**************************************
 * end parinacota_test_case()
 **************************************/

/*!**************************************************************
 * @fn parinacota_test()
 ****************************************************************
 * @brief
 *  This is the main test function of the parinacota test.
 *  This test will test media error handling.
 *
 * @param  - None.
 *
 * @return - None.
 *
 ****************************************************************/

void parinacota_test(void)
{
    //parinacota_test_case_t *xxx = parinacota_test_case_array;
    media_error_test_case_t *case_p = NULL;
    fbe_u32_t i;
    fbe_status_t status;
    fbe_test_discovered_drive_locations_t   drive_locations;
    fbe_test_raid_group_disk_set_t          disk_set;    

    /* fill out  all available drive information
    */
    fbe_test_sep_util_discover_all_drive_information(&drive_locations);
    
  
    status = fbe_test_get_520_drive_location(&drive_locations, &disk_set);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "No 520 drives available to test");
    fbe_test_sep_util_wait_for_pvd_state(disk_set.bus, disk_set.enclosure, disk_set.slot, FBE_LIFECYCLE_STATE_READY, 10000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    parinacota_disable_background_services(disk_set.bus, disk_set.enclosure, disk_set.slot);

    mut_printf(MUT_LOG_LOW, "=== %s: starting 520 test cases with %d_%d_%d ===\n", __FUNCTION__, disk_set.bus, disk_set.enclosure, disk_set.slot);    


    /* Loop over the 520 test cases until we hit an invalid record marking the end.
     */
    i = 0;
    case_p = media_error_520_io_case_array;
    while( case_p->exported_block_size != FBE_TEST_IO_INVALID_FIELD )
    {
        mut_printf(MUT_LOG_LOW, "== %s: 520: table_index:%d ==", __FUNCTION__, i);
        parinacota_test_case(case_p, disk_set.bus, disk_set.enclosure, disk_set.slot, PARINACOTA_OPTIMAL_520_CLIENT_BLOCKS);
        case_p++;
        i++;
    }

    mut_printf(MUT_LOG_LOW, "=== %s: finished 520 test cases ===\n\n", __FUNCTION__);


    status = fbe_test_get_4160_drive_location(&drive_locations, &disk_set);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "No 4K drives available to test");
    fbe_test_sep_util_wait_for_pvd_state(disk_set.bus, disk_set.enclosure, disk_set.slot, FBE_LIFECYCLE_STATE_READY, 10000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    parinacota_disable_background_services(disk_set.bus, disk_set.enclosure, disk_set.slot);



    mut_printf(MUT_LOG_LOW, "=== %s: starting 4K test cases with %d_%d_%d ===\n", __FUNCTION__, disk_set.bus, disk_set.enclosure, disk_set.slot);    

    /* Loop over the 4k test cases until we hit an invalid record marking the end.
     */
    i = 0;
    case_p = media_error_4k_io_case_array;
    while( case_p->exported_block_size != FBE_TEST_IO_INVALID_FIELD )
    {
        mut_printf(MUT_LOG_LOW, "== %s: 4K: table_index:%d ==", __FUNCTION__, i);
        parinacota_test_case(case_p, disk_set.bus, disk_set.enclosure, disk_set.slot, PARINACOTA_OPTIMAL_4K_CLIENT_BLOCKS);
        case_p++;
        i++;
    }
    mut_printf(MUT_LOG_LOW, "=== %s: finished 4K test cases ===\n", __FUNCTION__);
    
    return;
}
/**************************************
 * end parinacota_test()
 **************************************/

/*!**************************************************************
 * @fn parinacota_setup()
 ****************************************************************
 * @brief
 *   This function creates the configuration needed for this test.
 *   We create two 520 and 4K SAS drives.
 *
 * @param  - None.
 *
 * @return - fbe_status_t
 *
 *
 ****************************************************************/

void parinacota_setup(void)
{
    /* error injected during the test */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {       
        fbe_test_pp_util_create_physical_config_for_disk_counts(1 /*520*/,1 /*4k*/, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);
        elmo_load_sep_and_neit();
    }

    fbe_test_common_util_test_setup_init();

}
/******************************************
 * end parinacota_setup()
 ******************************************/

void parinacota_cleanup(void)
{
    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    //fbe_test_sep_util_restore_sector_trace_level();
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

}



/*!**************************************************************
/*************************
 * end file parinacota.c
 *************************/
