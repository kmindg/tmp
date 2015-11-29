/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file hiro_hamada_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests for unmap
 *
 * @version
 *   05/06/2015 - Created. Deanna Heng
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "fbe_test_package_config.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "sep_utils.h"
#include "sep_rebuild_utils.h"
#include "sep_zeroing_utils.h"
#include "sep_tests.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_provision_drive.h"

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* hiro_hamada_short_desc = "Unmapping of ssd drives";
char* hiro_hamada_long_desc =
    "\n"
    "\n"
    "This scenario validates unmap zeroing operations for ssds \n"
    "\n"
    "\n"
    "******* Hiro Hamada *******************************************************\n"
    "\n"
    "\n"
    "\n"
    "STEP A: Setup - Bring up the initial topology for local SP\n"
    "    - Insert a new enclosure having non configured drives\n"
    "    - Create a PVD object\n"
    "    - Create a RAID GROUP with a LUN\n"
    "    - Create a spare drives\n"
    "\n"
    "TEST 1: Background zeroing unmaps on unconsumed PVD\n"
    "\n"
    "STEP 1: Set up the PVD for zeroing\n"
    "    - Check that PVD object should be in READY state\n"
    "    - Disable Zeroing on the PVD\n"
    "    - Set the zeroing checkpoint to the default offset\n"
    "    - Set up hook on the PVD zeroing occurs\n"
    "STEP 2: Remove zeroing hook and wait for zeroing to complete\n"
    "STEP 3: Verify the paged metadata of the pvd\n"
    "    - All chunks should have the need zero bit set in the paged\n"
    "STEP 4: Verify that reading the entire unconsumed region of the pvd returns all zeros\n"
    "STEP 5: Verify sniff verify runs without errors\n"
    "    - Initiate sniff on the pvd\n"
    "    - Verify that there are no errors in the verify report\n"
    "\n"
    "TEST 2: Zero requests on unconsumed PVD\n"
    "\n"
    "STEP 1: Verify small zero request to the PVD that needs zeroing\n"
    "    - Issue a zero request less than a chunk to the PVD\n"
    "    - Verify that the needs zero bit is still set for the chunk\n"
    "    - Verify that the entire chunk contains all zeroes\n"
    "STEP 2: Verify that a zero request of one chunk to the PVD results in write same with unmap\n"
    "    - Issue a zero request of a chunk to the PVD\n"
    "    - Verify that the needs zero bit is still set for the chunk\n"
    "\n"
    "TEST 3: Background zeroing unmaps on consumed PVD\n"
    "\n"
    "STEP 1: PVDs are consumed by raid groups and LUNS\n"
    "    - Verify all PVDs and LUNs in the raid group have finished zeroing\n"
    "STEP 2: Verify that reading from the LUNs returns the zero pattern \n"
    "STEP 3: Verify the paged metadata of the PVD is as expected for the LUN.\n"
    "    - The user are of the lun is marked as nz:1 and cu:1.\n"
    "    - The lun metadata is marked as nz:0 and cu: 1.\n"
    "STEP 4: Verify the paged metadata of the PVD is as expected for the raid group\n"
    "    - The unconsumed area between the lun and the rg is marked as nz:1 cu:0\n"
    "    - The rg metadata is marked as nz: 0 cu: 1\n"
    "    - The unconsumed area between the rg md and vd md is marked as nz: 1 cu: 0\n"
    "\n"
    "TEST 4: Verify that small IOs to unmapped regions results in ZOD\n"
    "\n"
    "STEP 1: Entire region of lun area is zeroed\n"
    "STEP 2: Write a area smaller than an unmapped chunk\n"
    "STEP 3: Verify chunk written to has nz bit set to 0\n"
    "STEP 4: Verify chunks not written to still has nz bit set to 1\n"
    "\n"
    "TEST 5: Zero requests on consumed PVD\n"
    "\n"
    "STEP 1: Verify that a small zero request to lun with data results in write same\n"
    "STEP 2: Verify that a zero request resulting in a chunk on the pvd results in unmap\n"
    "\n"
    "TEST 6: Verify different flavors of verify comple on consumed unmapped PVD\n"
    "\n"
    "STEP 1: Run system verify, error verify, user ro verify, read write verify etc\n"
    "\n"
    "TEST 7: Verify rebuild runs to completion on unmapped PVD\n"
    "\n"
    "STEP 1: Write background data pattern to lun\n"
    "STEP 2: Run a full rebuild to an unmapped PVD\n"
    "STEP 3: Verify paged bits of pvd are correct\n"
    "\n"
    "TEST 8: Verify proactive copy runs to completion on unmapped PVD\n"
    "\n"
    "STEP 1: Write background data pattern to lun\n"
    "STEP 1: Run a proactive copy to an unmapped PVD\n"
    "STEP 2: Verify paged bits of pvd are correct\n"
    "\n";

/*!*******************************************************************
 * @def HIRO_HAMADA_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define HIRO_HAMADA_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def HIRO_HAMADA_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define HIRO_HAMADA_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def HIRO_HAMADA_CHUNK_SIZE
 *********************************************************************
 * @brief Size of one chunk
 *
 *********************************************************************/
#define HIRO_HAMADA_CHUNK_SIZE 0x800

/*!*******************************************************************
 * @def HIRO_HAMADA_ZERO_WAIT_TIME
 *********************************************************************
 * @brief Maximum wait time for zeroing
 *
 *********************************************************************/
#define HIRO_HAMADA_ZERO_WAIT_TIME 200000

/*!*******************************************************************
 * @def HIRO_HAMADA_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define HIRO_HAMADA_MAX_IO_SIZE_BLOCKS (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE 

/*!*******************************************************************
 * @var hiro_hamada_raid_group_config_random
 *********************************************************************
 * @brief This is the array of random configurations made from above configs
 *
 *********************************************************************/
fbe_test_rg_configuration_t hiro_hamada_raid_group_config_random[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE];

/*!*******************************************************************
 * @var hiro_hamada_rdgen_contexts
 *********************************************************************
 * @brief Rdgen context used to run IO
 *
 *********************************************************************/
static fbe_api_rdgen_context_t hiro_hamada_rdgen_contexts[HIRO_HAMADA_LUNS_PER_RAID_GROUP];

/*!*******************************************************************
 * @var sep_rebuild_utils_number_physical_objects_g
 *********************************************************************
 * @brief Number of expected physical objects
 *
 *********************************************************************/
extern fbe_u32_t        sep_rebuild_utils_number_physical_objects_g;


/*****************************************
 * EXTERNAL FUNCTIONS
 *****************************************/
extern void biff_insert_new_drive(fbe_sas_drive_type_t drive_type, fbe_terminator_sas_drive_type_default_info_t *new_page_info_p, fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
extern void biff_verify_drive_is_logically_online(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
extern void biff_remove_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_bool_t wait_for_destroy);
extern void kit_cloudkicker_setup_pvd_for_zeroing(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot, 
                                                  fbe_object_id_t * pvd_obj_id, fbe_api_provision_drive_info_t * pvd_info);
extern void kit_cloudkicker_set_zeroing_hook(fbe_object_id_t object_id, fbe_lba_t lba_checkpoint);
extern void kit_cloudkicker_get_zeroing_hook(fbe_object_id_t object_id, fbe_lba_t lba_checkpoint, fbe_bool_t was_hit);
extern void kit_cloudkicker_delete_zeroing_hook(fbe_object_id_t object_id, fbe_lba_t lba_checkpoint);
extern void kit_cloudkicker_wait_for_hook(fbe_object_id_t object_id, fbe_lba_t lba_checkpoint);
extern void skippy_doo_wait_for_verify_pass_completion(fbe_object_id_t in_pvd_object_id,
                                                       fbe_u32_t       in_timeout_sec);
extern void bilbo_baggins_multiple_verify_test(fbe_test_rg_configuration_t *rg_config_p);
extern void diabolicaldiscdemon_test_simple_proactive_copy(fbe_test_rg_configuration_t *rg_config_p,
                                                           fbe_u32_t  drive_pos,
                                                           fbe_bool_t is_user_initiated_copy);

/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/

void hiro_hamada_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void hiro_hamada_run_new_ssd_background_zeroing_with_unmap(fbe_test_rg_configuration_t *rg_config_p);
void hiro_hamada_verify_background_zeroing_on_consumed(fbe_test_rg_configuration_t *rg_config_p);
void hiro_hamada_verify_pvd_metadata(fbe_object_id_t pvd_object_id, fbe_lba_t pba_start, fbe_lba_t pba_end, 
                                     fbe_u16_t nz, fbe_u16_t cu, fbe_u16_t uz);
void hiro_hamada_verify_zeroed_area(fbe_object_id_t object_id, fbe_lba_t start_lba, fbe_lba_t end_lba);
void hiro_hamada_verify_zeroing_complete_on_rg_pvds(fbe_test_rg_configuration_t *rg_config_p);
void hiro_hamada_verify_consumed_extent(fbe_test_rg_configuration_t *rg_config_p, fbe_u16_t lun_nz);
void hiro_hamada_zero_request(fbe_test_rg_configuration_t *rg_config_p);
void hiro_hamada_zero_request_on_unconsumed_pvd(fbe_test_rg_configuration_t *rg_config_p);
void hiro_hamada_run_rebuild(fbe_test_rg_configuration_t *rg_config_p);
void hiro_hamada_run_sparing_tests(fbe_test_rg_configuration_t *rg_config_p);

/*!****************************************************************************
 * hiro_hamada_verify_pvd_metadata()
 ******************************************************************************
 * @brief
 *  Verify the paged metadata bits
 *
 * @param   pvd_object_id   - pvd to check
 * @param   pba_start       - pba to check from
 * @param   pba_end         - last pba 
 * @param   nz              - expected need zero bit
 * @param   cu              - expected consumed bit
 * @param   uz              - expected user zero bit
 *
 * @return  None
 *
 * @author
 *  05/14/2015 - Created. Deanna Heng
 ******************************************************************************/
void hiro_hamada_verify_pvd_metadata(fbe_object_id_t pvd_object_id, fbe_lba_t pba_start, fbe_lba_t pba_end, 
                                     fbe_u16_t nz, fbe_u16_t cu, fbe_u16_t uz)
{

    fbe_api_provisional_drive_paged_bits_t       *pvd_metadata;
    fbe_api_base_config_metadata_paged_get_bits_t paged_get_bits;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index = 0;

    mut_printf(MUT_LOG_LOW, "=== verify PVD obj id 0x%x paged metadata, chunks 0x%llx to 0x%llx\n", 
               pvd_object_id, (pba_start/HIRO_HAMADA_CHUNK_SIZE), (pba_end/HIRO_HAMADA_CHUNK_SIZE));
    for (index = (fbe_u32_t)(pba_start/HIRO_HAMADA_CHUNK_SIZE); index < (pba_end/HIRO_HAMADA_CHUNK_SIZE); index++ )
    {
        paged_get_bits.metadata_offset = index * sizeof(fbe_provision_drive_paged_metadata_t);
        paged_get_bits.metadata_record_data_size = sizeof(fbe_api_provisional_drive_paged_bits_t);
        * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
        paged_get_bits.get_bits_flags = 0;
        status = fbe_api_base_config_metadata_paged_get_bits(pvd_object_id, &paged_get_bits);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        pvd_metadata = (fbe_api_provisional_drive_paged_bits_t *)&paged_get_bits.metadata_record_data[0];
        mut_printf(MUT_LOG_LOW, "pvd 0x%x chunk %d paged bits nz:%d cu:%d v:%d uz:%d\n", 
                   pvd_object_id, index, pvd_metadata->need_zero_bit, pvd_metadata->consumed_user_data_bit, 
                   pvd_metadata->valid_bit, pvd_metadata->user_zero_bit);
        MUT_ASSERT_INT_EQUAL(pvd_metadata->need_zero_bit, nz);
        MUT_ASSERT_INT_EQUAL(pvd_metadata->consumed_user_data_bit, cu);
        MUT_ASSERT_INT_EQUAL(pvd_metadata->valid_bit, 1);
        MUT_ASSERT_INT_EQUAL(pvd_metadata->user_zero_bit, uz);

    }

}
/***************************************************************
 * end hiro_hamada_verify_pvd_metadata()
 ***************************************************************/

/*!****************************************************************************
 * hiro_hamada_verify_zeroed_area()
 ******************************************************************************
 * @brief
 *  Check that sectors contains zero pattern
 *
 * @param   object_id   - object to run io against
 * @param   start_lba   - lba to check from
 * @param   end_lba     - last lba to check
 *
 * @return  None
 *
 * @author
 *  05/14/2015 - Created. Deanna Heng
 ******************************************************************************/
void hiro_hamada_verify_zeroed_area(fbe_object_id_t object_id, fbe_lba_t start_lba, fbe_lba_t end_lba)
{
    fbe_api_rdgen_context_t            *context_p = &hiro_hamada_rdgen_contexts[0];
    fbe_status_t                        status = FBE_STATUS_OK;

    /* Read back the pattern and check it was written OK.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             object_id,
                                             FBE_CLASS_ID_INVALID, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_READ_CHECK,
                                             FBE_RDGEN_PATTERN_ZEROS,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             start_lba,    /* Start lba*/
                                             start_lba,    /* Min lba */
                                             end_lba,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                             2048,    /* min blocks */
                                             2048    /* max blocks */ );
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
}
/***************************************************************
 * end hiro_hamada_verify_zeroed_area()
 ***************************************************************/

/*!****************************************************************************
 * hiro_hamada_verify_zeroing_complete_on_rg_pvds()
 ******************************************************************************
 * @brief
 *   Verify that all pvds in the raid group has finished zeroing
 *
 * @param   rg_config_p - rg config to run the test against
 *
 * @return  None
 *
 * @author
 *  05/14/2015 - Created. Deanna Heng
 ******************************************************************************/
void hiro_hamada_verify_zeroing_complete_on_rg_pvds(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index = 0;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_lba_t   checkpoint = 0;

    for (index =0; index < rg_config_p->width; index++)
    {
        /* get the PVD object ID */
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[index].bus, 
                                                                rg_config_p->rg_disk_set[index].enclosure, 
                                                                rg_config_p->rg_disk_set[index].slot, 
                                                                &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* wait for disk zeroing to complete */
        status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_id, HIRO_HAMADA_ZERO_WAIT_TIME, FBE_LBA_INVALID);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* get the current disk zeroing checkpoint */
    	fbe_api_provision_drive_get_zero_checkpoint(pvd_object_id, &checkpoint);
        MUT_ASSERT_UINT64_EQUAL(checkpoint, FBE_LBA_INVALID);
    }
}
/***************************************************************
 * end hiro_hamada_verify_zeroing_complete_on_rg_pvds()
 ***************************************************************/

/*!****************************************************************************
 * hiro_hamada_verify_consumed_extent()
 ******************************************************************************
 * @brief
 *   Verify paged metadata of pvd area 
 *
 * @param   rg_config_p - rg config to run the test against
 * @param   lun_nz - nz bit. 0 if no data, 1 if data written to entire area
 *
 * @return  None
 *
 * @author
 *  05/14/2015 - Created. Deanna Heng
 ******************************************************************************/
void hiro_hamada_verify_consumed_extent(fbe_test_rg_configuration_t *rg_config_p, fbe_u16_t lun_nz)
{

    fbe_u32_t index = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_provision_drive_info_t      pvd_info = {0};
    fbe_lun_number_t lun_number;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_lun_get_lun_info_t lun_info = {0};
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_raid_group_get_info_t rg_info = {0};
    fbe_u32_t lun_index = 0;
    
    fbe_lba_t lun_pba_start = 0x10000;
    fbe_lba_t lun_pba_md_start = 0x10000;
    fbe_lba_t lun_pba_md_end = 0x10000;
    fbe_lba_t rg_pba_md_start = 0x10000;
    fbe_lba_t rg_pba_md_end = 0x10000;

    hiro_hamada_verify_zeroing_complete_on_rg_pvds(rg_config_p);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(rg_object_id,
                                             &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    for (lun_index = 0; lun_index < HIRO_HAMADA_LUNS_PER_RAID_GROUP; lun_index++)
    {
        lun_number = rg_config_p->logical_unit_configuration_list[lun_index].lun_number; 
        MUT_ASSERT_INT_EQUAL(rg_config_p->logical_unit_configuration_list[lun_index].raid_group_id, rg_config_p->raid_group_id);

        /* First make sure we finished the zeroing.
         */
        fbe_test_sep_util_wait_for_lun_zeroing_complete(lun_number, 60000, FBE_PACKAGE_ID_SEP_0);

        status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_lun_get_lun_info(lun_object_id, &lun_info);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_raid_group_get_physical_from_logical_lba(rg_object_id, lun_info.offset, &lun_pba_start);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        lun_pba_md_start = lun_pba_start + HIRO_HAMADA_CHUNKS_PER_LUN * 0x800 * fbe_test_sep_util_get_chunks_per_rebuild();
        
        status = fbe_api_raid_group_get_physical_from_logical_lba(rg_object_id, lun_info.offset + lun_info.capacity, &lun_pba_md_end);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        for (index = 0; index < rg_info.width; index++)
        {
            /* get the PVD object ID */
            status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[index].bus, 
                                                                    rg_config_p->rg_disk_set[index].enclosure, 
                                                                    rg_config_p->rg_disk_set[index].slot, 
                                                                    &pvd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


            mut_printf(MUT_LOG_TEST_STATUS, "Verify PVD 0x%x consumed lun area with cu and nz bits set", pvd_object_id); 
            hiro_hamada_verify_pvd_metadata(pvd_object_id, lun_pba_start, lun_pba_md_start, lun_nz, 1, 0);

            mut_printf(MUT_LOG_TEST_STATUS, "Verify PVD 0x%x consumed lun metadata area with cu bit set", pvd_object_id);
            hiro_hamada_verify_pvd_metadata(pvd_object_id, lun_pba_md_start, lun_pba_md_end, 0, 1, 0);

        }
    }

    status = fbe_api_raid_group_get_physical_from_logical_lba(rg_object_id, rg_info.paged_metadata_start_lba, &rg_pba_md_start);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //status = fbe_api_raid_group_get_physical_from_logical_lba(rg_object_id, rg_info.total_chunks * 0x800, &rg_pba_md_end);
    //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    rg_pba_md_end = rg_info.physical_offset + rg_info.imported_blocks_per_disk;

    for (index = 0; index < rg_info.width; index++)
    {
        /* get the PVD object ID */
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[index].bus, 
                                                                    rg_config_p->rg_disk_set[index].enclosure, 
                                                                    rg_config_p->rg_disk_set[index].slot, 
                                                                    &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        fbe_api_provision_drive_get_info(pvd_object_id, &pvd_info);

        mut_printf(MUT_LOG_TEST_STATUS, "Verify PVD 0x%x unconsumed area with nz bit set", pvd_object_id); 
        hiro_hamada_verify_pvd_metadata(pvd_object_id, lun_pba_md_end, rg_pba_md_start, 1, 0, 0);

        mut_printf(MUT_LOG_TEST_STATUS, "Verify PVD 0x%x consumed rg metadata area with cu bit set", pvd_object_id);
        hiro_hamada_verify_pvd_metadata(pvd_object_id, rg_pba_md_start, rg_pba_md_end, 0, 1, 0);

        /* Let's do half (also last chunk is vd metadata) 
         */
        mut_printf(MUT_LOG_TEST_STATUS, "Verify PVD 0x%x unconsumed area with nz and cu bits set", pvd_object_id);
        hiro_hamada_verify_pvd_metadata(pvd_object_id, rg_pba_md_end, pvd_info.total_chunks/2 * 0x800, 1, 0, 0);

    }
}
/***************************************************************
 * end hiro_hamada_verify_consumed_extent()
 ***************************************************************/

/*!****************************************************************************
 * hiro_hamada_run_new_ssd_background_zeroing_with_unmap()
 ******************************************************************************
 * @brief
 *   Verify paged bits of unmapped chunks and run sniff
 *
 * @param   rg_config_p - rg config to run the test against
 *
 * @return  None
 *
 * @author
 *  05/14/2015 - Created. Deanna Heng
 ******************************************************************************/
void hiro_hamada_run_new_ssd_background_zeroing_with_unmap(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t                           bus = rg_config_p->extra_disk_set[0].bus;
    fbe_u32_t                           encl = rg_config_p->extra_disk_set[0].enclosure;
    fbe_u32_t                           slot = rg_config_p->extra_disk_set[0].slot;
    fbe_object_id_t                     pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_provision_drive_info_t      pvd_info = {0};
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_provision_drive_verify_report_t verify_report = {0};

    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);
    kit_cloudkicker_setup_pvd_for_zeroing(bus, encl, slot, &pvd_object_id, &pvd_info);

    /* set the hooks for the pvd to replace and the spare so that we can check they were zeroing
     */
	kit_cloudkicker_set_zeroing_hook(pvd_object_id, pvd_info.default_offset+0x800);

    status = fbe_api_base_config_enable_background_operation(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_zeroing_utils_wait_for_disk_zeroing_to_start(pvd_object_id, HIRO_HAMADA_ZERO_WAIT_TIME);

    /* wait for disk zeroing to get to the hook 
     */
    status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_id, HIRO_HAMADA_ZERO_WAIT_TIME, pvd_info.default_offset+0x800);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    kit_cloudkicker_wait_for_hook(pvd_object_id, pvd_info.default_offset+0x800); 
    kit_cloudkicker_get_zeroing_hook(pvd_object_id, pvd_info.default_offset+0x800, FBE_TRUE);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_PROVISION_DRIVE_FLAG_UNMAP_SUPPORTED, pvd_info.flags & FBE_PROVISION_DRIVE_FLAG_UNMAP_SUPPORTED,
                             "unmap supported flag not set"); 
 
    kit_cloudkicker_delete_zeroing_hook(pvd_object_id, pvd_info.default_offset+0x800);

    /* wait for disk zeroing to complete 
     */
    status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_id, HIRO_HAMADA_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    hiro_hamada_verify_pvd_metadata(pvd_object_id, 0x10000, pvd_info.total_chunks*HIRO_HAMADA_CHUNK_SIZE, 
                                     1, 0, 0);

    /* Verify that beginning to end of pvd area is unmapped properly 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Verify PVD 0x%x area returns all zeroed when read", pvd_object_id); 
    hiro_hamada_verify_zeroed_area(pvd_object_id, 0, FBE_LBA_INVALID);

    fbe_test_sep_util_provision_drive_clear_verify_report(pvd_object_id);

    fbe_test_sep_util_provision_drive_get_verify_report(pvd_object_id, &verify_report);
    MUT_ASSERT_INT_EQUAL(verify_report.pass_count, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "Verify sniff runs without errors on PVD 0x%x", pvd_object_id); 
    status = fbe_api_base_config_enable_background_operation(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    skippy_doo_wait_for_verify_pass_completion(pvd_object_id, HIRO_HAMADA_ZERO_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_base_config_disable_background_operation(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_test_sep_util_provision_drive_get_verify_report(pvd_object_id, &verify_report);
    MUT_ASSERT_INT_EQUAL(verify_report.totals.recov_read_count, 0);
    MUT_ASSERT_INT_EQUAL(verify_report.totals.unrecov_read_count, 0); 

    hiro_hamada_verify_pvd_metadata(pvd_object_id, 0x10000, pvd_info.total_chunks*HIRO_HAMADA_CHUNK_SIZE, 
                                     1, 0, 0);

}
/***************************************************************
 * end hiro_hamada_run_new_ssd_background_zeroing_with_unmap()
 ***************************************************************/

/*!****************************************************************************
 * hiro_hamada_zero_request_on_unconsumed_pvd()
 ******************************************************************************
 * @brief
 *  Verify that small zero requests to a consumed area results in an on demand zero
 *  Verify that at least chunk size zero to the pvd results in a write same
 *
 * @param   rg_config_p - rg config to run the test against
 *
 * @return  None
 *
 * @author
 *  05/14/2015 - Created. Deanna Heng
 ******************************************************************************/
void hiro_hamada_zero_request_on_unconsumed_pvd(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_object_id_t                     pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_provision_drive_info_t      pvd_info = {0};
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t * context_p = &hiro_hamada_rdgen_contexts[0];

    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Verify user zero requests on unconsumed pvd 0x%x===", pvd_object_id); 

	/* get the PVD object ID 
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->extra_disk_set[0].bus, 
                                                            rg_config_p->extra_disk_set[0].enclosure, 
                                                            rg_config_p->extra_disk_set[0].slot, 
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_id, HIRO_HAMADA_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_provision_drive_get_info(pvd_object_id, &pvd_info);

    /* Perform the requested I/O and make sure there were no errors 
     */
    status = fbe_api_rdgen_send_one_io(context_p, 
                                       pvd_object_id, 
                                       FBE_CLASS_ID_INVALID, 
                                       FBE_PACKAGE_ID_SEP_0, 
                                       FBE_RDGEN_OPERATION_ZERO_READ_CHECK, 
                                       FBE_RDGEN_PATTERN_ZEROS,
                                       0x10000,
                                       0x80, /* write an element */
                                       FBE_RDGEN_LBA_SPEC_FIXED, 0, 0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "Verify PVD 0x%x that nz bit set for small user zero request to zeroed area", pvd_object_id); 
    hiro_hamada_verify_pvd_metadata(pvd_object_id, 0x10000, 0x10800, 1, 0, 0);

    hiro_hamada_verify_zeroed_area(pvd_object_id, 0x10000, 0x10800);


    /* Perform the requested I/O and make sure there were no errors 
     */
    status = fbe_api_rdgen_send_one_io(context_p, 
                                       pvd_object_id, 
                                       FBE_CLASS_ID_INVALID, 
                                       FBE_PACKAGE_ID_SEP_0, 
                                       FBE_RDGEN_OPERATION_ZERO_READ_CHECK, 
                                       FBE_RDGEN_PATTERN_ZEROS,
                                       0x10000,
                                       HIRO_HAMADA_CHUNK_SIZE, /* write at least a chunk to each pvd */
                                       FBE_RDGEN_LBA_SPEC_FIXED, 0, 0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* user zero bit will get set and bgz check point moves to beginning 
     * wait until the user zero bit gets processed 
     */
    status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_id, HIRO_HAMADA_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Verify PVD 0x%x that nz bit set on zero request for chunk requests", pvd_object_id); 
    hiro_hamada_verify_pvd_metadata(pvd_object_id, 0x10000, 0x10800, 1, 1, 0);

}
/***************************************************************
 * end hiro_hamada_zero_request_on_unconsumed_drive()
 ***************************************************************/

/*!****************************************************************************
 * hiro_hamada_verify_background_zeroing_on_consumed()
 ******************************************************************************
 * @brief
 *  Verify that consumed and unconsumed areas are unmapped correctly
 *
 * @param   rg_config_p - rg config to run the test against
 *          context_p - pointer to test case index
 *
 * @return  None
 *
 * @author
 *  05/06/2015 - Created. Deanna Heng
 ******************************************************************************/
void hiro_hamada_verify_background_zeroing_on_consumed(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lun_number_t lun_number;
    fbe_object_id_t lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_lun_get_lun_info_t lun_info = {0};
    fbe_u32_t lun_index = 0;
    fbe_lun_verify_report_t   verify_report = {0};

    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);

    for (lun_index = 0; lun_index < HIRO_HAMADA_LUNS_PER_RAID_GROUP; lun_index++)
    {
        lun_number = rg_config_p->logical_unit_configuration_list[lun_index].lun_number; 
        MUT_ASSERT_INT_EQUAL(rg_config_p->logical_unit_configuration_list[lun_index].raid_group_id, rg_config_p->raid_group_id);

        /* First make sure we finished the zeroing.
         */
        fbe_test_sep_util_wait_for_lun_zeroing_complete(lun_number, 60000, FBE_PACKAGE_ID_SEP_0);

        status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        /* Then make sure we finished the verify.  This waits for pass 1 to complete.
         */
        fbe_test_sep_util_wait_for_lun_verify_completion(lun_object_id, &verify_report, 1);

        /* Verify that beginning to end of lun area is unmapped properly */
        mut_printf(MUT_LOG_TEST_STATUS, "Verify LUN 0x%x area returns all zeroed when read", lun_object_id); 
        hiro_hamada_verify_zeroed_area(lun_object_id, 0, FBE_LBA_INVALID);

        /* Next make sure that the LUN was requested to do initial verify and that it performed it already.
         */
        status = fbe_api_lun_get_lun_info(lun_object_id, &lun_info);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(lun_info.b_initial_verify_already_run, 1);
        MUT_ASSERT_INT_EQUAL(lun_info.b_initial_verify_requested, 1);
    }

    /* Verify paged bits of area on pvd
     */
    hiro_hamada_verify_consumed_extent(rg_config_p, 1 /* no data written to lun*/);
        
}

/*!****************************************************************************
 * hiro_hamada_zod_on_unmapped_drive()
 ******************************************************************************
 * @brief
 *  Verify that a small write to an unmapped chunk results in a zero on demand
 *
 * @param   rg_config_p - rg config to run the test against
 *
 * @return  None
 *
 * @author
 *  05/14/2015 - Created. Deanna Heng
 ******************************************************************************/
void hiro_hamada_zod_on_unmapped_drive(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_lun_number_t lun_number;
    fbe_api_raid_group_get_info_t rg_info = {0};
    fbe_api_lun_get_lun_info_t lun_info = {0};
    fbe_api_rdgen_context_t * context_p = &hiro_hamada_rdgen_contexts[0];
    fbe_lba_t pba = FBE_LBA_INVALID;
    fbe_u32_t lun_index = 0;
    
    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Verify that PVD paged is correct after ZOD 0x%x===", pvd_object_id); 

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    lun_number = rg_config_p->logical_unit_configuration_list[lun_index].lun_number; 
    MUT_ASSERT_INT_EQUAL(rg_config_p->logical_unit_configuration_list[lun_index].raid_group_id, rg_config_p->raid_group_id);
    status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_lun_get_lun_info(lun_object_id, &lun_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Perform the requested I/O and make sure there were no errors 
     */
    status = fbe_api_rdgen_send_one_io(context_p, 
                                       lun_object_id, 
                                       FBE_CLASS_ID_INVALID, 
                                       FBE_PACKAGE_ID_SEP_0, 
                                       FBE_RDGEN_OPERATION_WRITE_READ_CHECK, 
                                       FBE_RDGEN_PATTERN_LBA_PASS,
                                       0,
                                       rg_info.width * rg_info.element_size, /* write at least a stripe */
                                       FBE_RDGEN_LBA_SPEC_FIXED, 0, 0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[0].bus, 
                                                   rg_config_p->rg_disk_set[0].enclosure, 
                                                   rg_config_p->rg_disk_set[0].slot, 
                                                   &pvd_object_id);

    status = fbe_api_raid_group_get_physical_from_logical_lba(rg_object_id, lun_info.offset, &pba);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Verify PVD 0x%x that nz bit cleared from zod", pvd_object_id); 
    hiro_hamada_verify_pvd_metadata(pvd_object_id, pba, pba+0x800, 0, 1, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "Verify PVD 0x%x that nz still set on next chunk", pvd_object_id); 
    hiro_hamada_verify_pvd_metadata(pvd_object_id, pba+0x800, pba+0x1000, 1, 1, 0);
}
/***************************************************************
 * end hiro_hamada_zod_on_unmapped_drive()
 ***************************************************************/

/*!****************************************************************************
 * hiro_hamada_run_zero_request()
 ******************************************************************************
 * @brief
 *  Verify that small zero requests to a consumed area results in an on demand zero
 *  Verify that at least chunk size zero to the pvd results in a write same
 *
 * @param   rg_config_p - rg config to run the test against
 *
 * @return  None
 *
 * @author
 *  05/14/2015 - Created. Deanna Heng
 ******************************************************************************/
void hiro_hamada_zero_request(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_lun_number_t lun_number;
    fbe_api_raid_group_get_info_t rg_info = {0};
    fbe_api_lun_get_lun_info_t lun_info = {0};
    fbe_api_rdgen_context_t * context_p = &hiro_hamada_rdgen_contexts[0];
    fbe_lba_t pba = FBE_LBA_INVALID;
    fbe_u32_t lun_index = 0;
    fbe_u16_t data_disks = 0;
    fbe_lba_t capacity = FBE_LBA_INVALID;

    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Verify user zero requests 0x%x ===", pvd_object_id); 

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_get_raid_class_info(rg_config_p->raid_type,
                                                   rg_config_p->class_id,
                                                   rg_config_p->width,
                                                   rg_config_p->capacity,
                                                   &data_disks,
                                                   &capacity);

    lun_number = rg_config_p->logical_unit_configuration_list[lun_index].lun_number; 
    MUT_ASSERT_INT_EQUAL(rg_config_p->logical_unit_configuration_list[lun_index].raid_group_id, rg_config_p->raid_group_id);
    status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_lun_get_lun_info(lun_object_id, &lun_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    big_bird_write_background_pattern();

    /* Perform the requested I/O and make sure there were no errors 
     */
    status = fbe_api_rdgen_send_one_io(context_p, 
                                       lun_object_id, 
                                       FBE_CLASS_ID_INVALID, 
                                       FBE_PACKAGE_ID_SEP_0, 
                                       FBE_RDGEN_OPERATION_ZERO_READ_CHECK, 
                                       FBE_RDGEN_PATTERN_ZEROS,
                                       0,
                                       data_disks * rg_info.element_size, /* write at least a stripe */
                                       FBE_RDGEN_LBA_SPEC_FIXED, 0, 0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Perform the requested I/O and make sure there were no errors 
     */
    status = fbe_api_rdgen_send_one_io(context_p, 
                                       lun_object_id, 
                                       FBE_CLASS_ID_INVALID, 
                                       FBE_PACKAGE_ID_SEP_0, 
                                       FBE_RDGEN_OPERATION_READ_CHECK, 
                                       FBE_RDGEN_PATTERN_LBA_PASS,
                                       data_disks * rg_info.element_size,
                                       data_disks * rg_info.element_size, /* check at least a stripe */
                                       FBE_RDGEN_LBA_SPEC_FIXED, 0, 0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[0].bus, 
                                                   rg_config_p->rg_disk_set[0].enclosure, 
                                                   rg_config_p->rg_disk_set[0].slot, 
                                                   &pvd_object_id);

    status = fbe_api_raid_group_get_physical_from_logical_lba(rg_object_id, lun_info.offset, &pba);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Verify PVD 0x%x that nz bit cleared for small user zero request", pvd_object_id); 
    hiro_hamada_verify_pvd_metadata(pvd_object_id, pba, pba+0x800, 0, 1, 0);


    /* Perform the requested I/O and make sure there were no errors 
     */
    status = fbe_api_rdgen_send_one_io(context_p, 
                                       lun_object_id, 
                                       FBE_CLASS_ID_INVALID, 
                                       FBE_PACKAGE_ID_SEP_0, 
                                       FBE_RDGEN_OPERATION_ZERO_READ_CHECK, 
                                       FBE_RDGEN_PATTERN_ZEROS,
                                       0,
                                       data_disks * HIRO_HAMADA_CHUNK_SIZE, /* write at least a chunk to each pvd */
                                       FBE_RDGEN_LBA_SPEC_FIXED, 0, 0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_id, HIRO_HAMADA_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* NOTE: LUN no longer splits requests into parity stripes which is half a chunk at the pvd 
     *       Change this if behavior changes
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Verify PVD 0x%x that nz bit set on zero request for pvd chunk sizes ", pvd_object_id); 
    hiro_hamada_verify_pvd_metadata(pvd_object_id, pba, pba+0x800, 1, 1, 0);
    //mut_printf(MUT_LOG_TEST_STATUS, "Verify PVD 0x%x that nz bit not set on zero request for pvd chunk sizes", pvd_object_id); 
    //hiro_hamada_verify_pvd_metadata(pvd_object_id, pba, pba+0x800, 0, 1, 0);

}
/***************************************************************
 * end hiro_hamada_zero_request()
 ***************************************************************/

/*!****************************************************************************
 * hiro_hamada_run_rebuild()
 ******************************************************************************
 * @brief
 *  Run rebuild on unmapped drives
 *
 * @param   rg_config_p - rg config to run the test against
 *
 * @return  None
 *
 * @author
 *  05/14/2015 - Created. Deanna Heng
 ******************************************************************************/
void hiro_hamada_run_rebuild(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t position = 0;
    fbe_api_terminator_device_handle_t drive_info = {0};
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;

    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);

    /* book keeping to keep track of how many objects to wait for later 
     */
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_test_sep_util_update_permanent_spare_trigger_timer(1);
    status = fbe_api_base_config_disable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_REBUILD);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	/* Remove a single drive in the RG.  Check the object states change correctly and that rb logging
	 * is marked. 
     */
    sep_rebuild_utils_number_physical_objects_g -= 1;
	sep_rebuild_utils_remove_drive_and_verify(rg_config_p, position, sep_rebuild_utils_number_physical_objects_g, &drive_info);        

    /* the spare pvd should've swapped in...
     */
    fbe_test_sep_drive_wait_permanent_spare_swap_in(rg_config_p, position);

    fbe_test_sep_util_wait_for_raid_group_to_start_rebuild(rg_object_id);
    status = fbe_api_base_config_enable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_REBUILD);

    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, position);

    sep_rebuild_utils_check_bits(rg_object_id);

    /* set the permanent spare trigger timer back to default. */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    sep_rebuild_utils_number_physical_objects_g += 1;
	/* Reinsert the drive and wait for the rebuild to start */
	sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, position, sep_rebuild_utils_number_physical_objects_g, &drive_info);

}
/***************************************************************
 * end hiro_hamada_run_rebuild()
 ***************************************************************/

/*!****************************************************************************
 * hiro_hamada_run_sparing_tests()
 ******************************************************************************
 * @brief
 *  Run rebuild and paco on unmapped drive and verify bits
 *
 * @param   rg_config_p - rg config to run the test against
 *
 * @return  None
 *
 * @author
 *  05/06/2015 - Created. Deanna Heng
 ******************************************************************************/
void hiro_hamada_run_sparing_tests(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);


    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);

    /*  Set up a spare drives 
     */
    sep_rebuild_utils_setup_hot_spare(rg_config_p->extra_disk_set[0].bus,
                                      rg_config_p->extra_disk_set[0].enclosure,
                                      rg_config_p->extra_disk_set[0].slot); 
    sep_rebuild_utils_setup_hot_spare(rg_config_p->extra_disk_set[1].bus,
                                      rg_config_p->extra_disk_set[1].enclosure,
                                      rg_config_p->extra_disk_set[1].slot); 

    big_bird_write_background_pattern();

    hiro_hamada_run_rebuild(rg_config_p);

    /* refresh drive info
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);
    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

    /* Verify paged bits of area on pvd
     */
    hiro_hamada_verify_consumed_extent(rg_config_p, 0 /* data written to lun*/);

    diabolicaldiscdemon_test_simple_proactive_copy(rg_config_p, 0, FBE_TRUE);

    /* refresh drive info
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);
    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);
    
    /* Verify paged bits of area on pvd
     */
    hiro_hamada_verify_consumed_extent(rg_config_p, 0 /* data written to lun*/);
}
/***************************************************************
 * end hiro_hamada_run_sparing_tests()
 ***************************************************************/

/*!****************************************************************************
 * hiro_hamada_run_tests()
 ******************************************************************************
 * @brief
 *  Run unmap zeroing tests
 *
 * @param   rg_config_p - rg config to run the test against
 *          context_p - pointer to test case index
 *
 * @return  None
 *
 * @author
 *  05/06/2015 - Created. Deanna Heng
 ******************************************************************************/
void hiro_hamada_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p) 
{
    hiro_hamada_run_new_ssd_background_zeroing_with_unmap(rg_config_p);
    /* This may be a useless test */
    hiro_hamada_zero_request_on_unconsumed_pvd(rg_config_p);
    hiro_hamada_verify_background_zeroing_on_consumed(rg_config_p);
    hiro_hamada_zod_on_unmapped_drive(rg_config_p);
    hiro_hamada_zero_request(rg_config_p);
    bilbo_baggins_multiple_verify_test(rg_config_p);
    hiro_hamada_run_sparing_tests(rg_config_p); 
}
/***************************************************************
 * end hiro_hamada_run_tests()
 ***************************************************************/

/*!****************************************************************************
 * hiro_hamada_test()
 ******************************************************************************
 * @brief
 *  Run unmap zeroing tests
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *  05/06/2015 - Created. Deanna Heng
 ******************************************************************************/
void hiro_hamada_test(void)
{

    /* Since these test do not generate corrupt crc etc, we don't expect
     * ANY errors.  So stop the system if any sector errors occur.
     */
    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_TRUE);
    fbe_test_run_test_on_rg_config_with_extra_disk_with_time_save(&hiro_hamada_raid_group_config_random[0],
                                                              NULL,hiro_hamada_run_tests,
                                                              HIRO_HAMADA_LUNS_PER_RAID_GROUP,
                                                              HIRO_HAMADA_CHUNKS_PER_LUN,
                                                              FBE_FALSE);

    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_FALSE);

    return;
}
/***************************************************************
 * end hiro_hamada_test()
 ***************************************************************/

/*!****************************************************************************
 *  hiro_hamada_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the hiro_hamada test.
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *  05/06/2015 - Created. Deanna Heng
 *****************************************************************************/
void hiro_hamada_setup(void)
{
    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t raid_group_count = 1;
        
        darkwing_duck_create_random_config(&hiro_hamada_raid_group_config_random[0], raid_group_count);

        /* Initialize the raid group configuration 
         */
        fbe_test_sep_util_init_rg_configuration_array(&hiro_hamada_raid_group_config_random[0]);

        /* initialize the number of extra drive required by each rg 
         */
        fbe_test_sep_util_populate_rg_num_extra_drives(&hiro_hamada_raid_group_config_random[0]);
        fbe_test_pp_utils_set_default_520_sas_drive(FBE_SAS_DRIVE_SIM_520_FLASH_UNMAP);

        /* Setup the physical config for the raid groups
         */
        elmo_create_physical_config_for_rg(&hiro_hamada_raid_group_config_random[0], 
                                           raid_group_count);
        elmo_load_sep_and_neit();

        /* Set the trace level to info. 
         */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);        
    }

    /* update the permanent spare trigger timer so we don't need to wait long for the swap
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    //fbe_test_sep_util_disable_system_drive_zeroing();
    fbe_test_sep_drive_disable_sniff_verify_for_all_pvds();
    //fbe_test_sep_util_set_pvd_class_debug_flags(0xffff);

    /* After sep is loaded setup notifications.
     */
    fbe_test_common_setup_notifications(FBE_FALSE /* This is a single SP test*/);

    return;
}
/***************************************************************
 * end hiro_hamada_setup()
 ***************************************************************/

/*!****************************************************************************
 *  hiro_hamada_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the hiro_hamada test.
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *  05/06/2015 - Created. Deanna Heng
 *****************************************************************************/
void hiro_hamada_cleanup(void)
{
    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);
    fbe_test_common_cleanup_notifications(FBE_FALSE /* This is a single SP test*/);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/***************************************************************
 * end hiro_hamada_cleanup()
 ***************************************************************/
