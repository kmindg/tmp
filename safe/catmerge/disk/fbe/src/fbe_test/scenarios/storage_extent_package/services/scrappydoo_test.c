/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file scrappydoo_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains metadata I/O tests for the virtual drive object.
 *
 * @version
 *  10/02/2009  Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common_transport.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * scrappydoo_short_desc = "This scenario will drive metadata I/O from rdgen to (1) ID LUN in a (1) drive mirror group";
char * scrappydoo_long_desc ="\
The ScrappyDoo scenario tests (simple) metadata I/O to the virtual drive object\n\
This includes:\n\
        - raid 1 small write (up to 256 blocks).\n\
        - raid 1 read (up to 256 blocks).\n\
	    - 520 byte per block SAS drives only.\n\
\n\
Dependencies:\n\
        - The Stripe Locking service must support single sp in simulation.\n\
        - The Mirror raid group must support read and small write.\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] viper enclosure\n\
        [PP] 1 SAS drive\n\
        [PP] 1 logical drive\n\
        [SEP] 1 provisioned drives\n\
        [SEP] 1 virtual drives (1) in various sparing modes\n\
        [SEP] mirror raid group (individual disk)\n\
        [SEP] 1 luns\n\
\n\
STEP 1: Bring up the initial topology.\n\
    - Create and verify the initial physical package config.\n\
	- Create provisioned drives and attach edges to logical drives.\n\
    - Create virtual drives and attach edges to provisioned drives.\n\
    - Create raid 1 raid group object and attach edges from raid 1 to virtual drives.\n\
    - raid group attributes:  width: 1 drives of capacity: 0xFC00 blocks\n\
    - Create and attach 1 LUNs to the raid 1 object.\n\
	- Each LUN will have capacity of 0x2000 blocks and element size of 128.\n\
STEP 2: Start rdgen write/read/compare test to all LUNs in the raid group.\n\
	- I/O will run in parallel to all LUNs in the raid group.\n\
	- I/O will be 1 thread per LUN, with max size of 128 blocks.\n\
	- Rdgen will choose lbas and block counts randomly for each pass.\n\
	- Each pass will perform a write, read and compare.\n\
	- We will run I/O for 100 passes on each LUN.\n\
STEP 3: Verify that rdgen I/O did not encounter errors.\n";

/* The number of raid groups.
 */
#define SCRAPPYDOO_TEST_RAID_GROUP_COUNT       1 

/* The number of drives in the mirror raid group.
 */
#define SCRAPPYDOO_TEST_RAID_GROUP_WIDTH       1 

/* The number of blocks in a raid group bitmap chunk.
 */
#define SCRAPPYDOO_TEST_RAID_GROUP_CHUNK_SIZE  (FBE_RAID_DEFAULT_CHUNK_SIZE)

/* The number of drive in the mirror (this case be 3 for the PSM raid group!)
 */
#define SCRAPPYDOO_TEST_MIRROR_WIDTH           1

/* The number of LUNs in our raid group.
 */
#define SCRAPPYDOO_TEST_LUNS_PER_RAID_GROUP    1

/* Element size of our LUNs.
 */
#define SCRAPPYDOO_TEST_ELEMENT_SIZE           128 

/* The RG and LU capacity is 1/3rd the size of the virtual drive.
 * This allows the raid group metadata data to eb the same size as
 * the exported LUN, RG and allows for VD and PVD metadata.
 */
#define SCRAPPYDOO_TEST_RAID_GROUP_CAPACITY_IN_BLOCKS   (0x5000) 

/* Capacity of the VD (virtual drive) is (3) times the size of the 
 * LU and RG.
 */
#define SCRAPPYDOO_TEST_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS (SCRAPPYDOO_TEST_RAID_GROUP_CAPACITY_IN_BLOCKS * 3)


/* Number of copies of the metadata (i.e. width)
 * Currently it is (1) since the plan is to have only (1) copy of the bitmap.
 */
#define SCRAPPYDOO_TEST_METADATA_COPIES         (1)

/*!*******************************************************************
 * @var scrappydoo_capacity_array
 *********************************************************************
 * @brief This is the set of capacities we use for each rg.
 *
 *********************************************************************/
static fbe_lba_t scrappydoo_capacity_array[SCRAPPYDOO_TEST_RAID_GROUP_COUNT] = {SCRAPPYDOO_TEST_RAID_GROUP_CAPACITY_IN_BLOCKS};

/*!*******************************************************************
 * @var scoobydoo_disk_set
 *********************************************************************
 * @brief This is the disk set for the 520 RG.
 *
 *********************************************************************/
#if 0
static fbe_test_raid_group_disk_set_t scrappyydoo_disk_set[SCRAPPYDOO_TEST_RAID_GROUP_COUNT][SCRAPPYDOO_TEST_RAID_GROUP_WIDTH] = 
{
    /* width = 1, 520-bps */
    { {0,0,3}},  
};
#endif

/*!*******************************************************************
 * @var scrappydoo_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t scrappydoo_test_contexts[SCRAPPYDOO_TEST_LUNS_PER_RAID_GROUP];

/*!*******************************************************************
 * @var SCRAPPYDOO_THREADS
 *********************************************************************
 * @brief Number of threads we will run for I/O.
 *
 *********************************************************************/
fbe_u32_t SCRAPPYDOO_THREADS = 1;

/*!*******************************************************************
 * @def SCRAPPYOO_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define SCRAPPYDOO_MAX_IO_SIZE_BLOCKS (FBE_RAID_MAX_BE_XFER_SIZE) // 4096 


/*!**************************************************************
 * scrappydoo_write_background_pattern()
 ***************************************************************
 * @brief
 *  Run a background pattern to a set of raid 1 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  10/02/2009   Ron Proulx  - Created.
 *
 ****************************************************************/

void scrappydoo_write_background_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &scrappydoo_test_contexts[0];
    fbe_status_t status;

    /* Write a background pattern to the LUNs.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            SCRAPPYDOO_TEST_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_READ_CHECK,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            SCRAPPYDOO_TEST_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    
    return;
}
/******************************************
 * end scrappydoo_write_background_pattern()
 ******************************************/

/*!**************************************************************
 * scrappydoo_run_raid_1_rdgen_io()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 1 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  10/02/2009   Ron Proulx  - Created
 *
 ****************************************************************/

void scrappydoo_run_raid_1_rdgen_io(void)
{
    fbe_api_rdgen_context_t *context_p = &scrappydoo_test_contexts[0];
    fbe_status_t status;

    /*! @note Currently this test doesn't support peer I/O
     */
    status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_FALSE, FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Setup the abort mode to false
     */
    status = fbe_test_sep_io_configure_abort_mode(FBE_FALSE, FBE_TEST_RANDOM_ABORT_TIME_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                       FBE_OBJECT_ID_INVALID,
                                       FBE_CLASS_ID_LUN,
                                       FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                       FBE_LBA_INVALID,    /* use capacity */
                                       0,    /* run forever*/ 
                                       SCRAPPYDOO_THREADS, /* threads */
                                       SCRAPPYDOO_MAX_IO_SIZE_BLOCKS,
                                       FBE_FALSE, /* Don't inject aborts */
                                       FBE_FALSE  /* Peer I/O not supported*/);
    /* Run our I/O test.
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
 
    /* Run I/O briefly.
     */
    EmcutilSleep(5000);

    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end scrappydoo_run_raid_1_rdgen_io()
 ******************************************/

/*!**************************************************************
 * scrappydoo_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 1 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/17/2009   Ron Proulx  - Created
 *
 ****************************************************************/

void scrappydoo_test(void)
{
    /* Write a background pattern to the LUNs.
     */
    scrappydoo_write_background_pattern();

    /* Run our I/O test.
     */
    scrappydoo_run_raid_1_rdgen_io();

    return;
}
/******************************************
 * end scrappydoo_test()
 ******************************************/

/*!**************************************************************
 *          scrappydoo_create_raid_1_with_debug_flags()
 ****************************************************************
 * @brief
 *  Create the raid group for the ScrappyDoo (i.e. debug flags) test.
 *  This creates PVDs, VDs, raid group (raid 1) and lun objects.
 *
 * @param first_object_id - first object id to use to create objects.
 * @param num_luns - number of luns per rg.
 * @param raid_group_id - id number of the raid group
 * @param first_lun_number - first lun number to use when creating luns
 * @param element_size - blocks per element.
 * @param virtual_drive_capacity - blocks in each virtual drive
 * @param raid_group_capacity - blocks in each rg.
 * @param lun_capacity - blocks in each lun.
 * @param width - Number of drives in rg.
 * @param raid_group_chunk_size - Number of blocks in raid bitmap chunk
 * @param debug_flags - raid group debug flags
 *
 * @return None.   
 *
 * @author
 *  02/22/2010   Ron Proulx  - Copied from grover_create_raid_group.
 *
 ****************************************************************/
static fbe_status_t scrappydoo_create_raid_1_with_debug_flags(fbe_u32_t num_luns,
                                                              fbe_raid_group_number_t *raid_group_id,
                                                              fbe_lun_number_t *lun_number,
                                                              fbe_u32_t element_size,
                                                              fbe_lba_t virtual_drive_capacity,
                                                              fbe_lba_t raid_group_capacity,
                                                              fbe_lba_t lun_capacity,
                                                              fbe_u32_t width,
                                                              fbe_chunk_size_t raid_group_chunk_size,
                                                              fbe_raid_group_debug_flags_t debug_flags)
{
    fbe_status_t                    status = FBE_STATUS_OK;
#if 0
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_object_id_t                 vd_first_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 lun_first_object_id = FBE_OBJECT_ID_INVALID;
    fbe_database_transaction_id_t     transaction_id;
    fbe_object_id_t                 object_id;
    fbe_u32_t                       index;
    fbe_u32_t                       starting_num_objects = 0;
    fbe_u32_t                       num_objects = 0;
    fbe_u32_t                       pvds[SCRAPPYDOO_TEST_RAID_GROUP_WIDTH];
    
    /* Since there are global databases save on a mirror, we must get the starting
     * number of objects.
     */
    status = fbe_api_get_total_objects(&starting_num_objects, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_database_transaction_start(&transaction_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Start ...\n", __FUNCTION__);


    /* Create virtual drives */
    for(index = 0; index < width; ++index)
    {
        fbe_test_raid_group_disk_set_t  drive_info;
        
        object_id = FBE_OBJECT_ID_INVALID;
        status = fbe_test_sep_util_create_virtual_drive(transaction_id, 
                                                        &object_id, 
                                                        virtual_drive_capacity,
                                                        raid_group_chunk_size,
                                                        FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE,
                                                        FBE_RAID_GROUP_DEBUG_FLAG_NONE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        if ( vd_first_object_id == FBE_OBJECT_ID_INVALID )
        {
            vd_first_object_id = object_id;
        }

        drive_info = scrappyydoo_disk_set[0][index];
        status = fbe_api_provision_drive_get_obj_id_by_location(drive_info.bus,    /* bus */
                                                                drive_info.enclosure,
                                                                drive_info.slot, 
                                                                &pvds[index]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_database_create_edge(transaction_id,
                                                   object_id,
                                                   pvds[index],
                                                   0,    /* The first edge of VD */
                                                   virtual_drive_capacity,
                                                   0 /* offset */);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Now create the raid-1 group with the debug flags supplied.
     */
    status = fbe_test_sep_util_create_raid1(transaction_id, &rg_object_id,
                                            raid_group_id,
                                            raid_group_capacity, 
                                            width, 
                                            element_size,
                                            raid_group_chunk_size, 
                                            debug_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Create edges from raid to virtual drives */
    for(index = 0; index < width; ++index)
    {
        status = fbe_api_database_create_edge(transaction_id,
                                                   rg_object_id, /* client */
                                                   vd_first_object_id + index, /* server */
                                                   index,
                                                   virtual_drive_capacity,
                                                   0 /* offset */);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    } /* End of edge creation from raid to VD's */

    /* Create lun object and edge from lun to raid object.
     */
    for(index = 0; index < num_luns; ++index)
    {
        object_id = FBE_OBJECT_ID_INVALID;
        status = fbe_test_sep_util_create_lun(transaction_id, 
                                              &object_id, 
                                              lun_number, 
                                              lun_capacity);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if ( lun_first_object_id == FBE_OBJECT_ID_INVALID )
        {
            lun_first_object_id = object_id;
        }
        
        status = fbe_api_database_create_edge(transaction_id,
                                                   object_id,
                                                   rg_object_id,
                                                   0, /* index of the client edge (first edge) */
                                                   lun_capacity,
                                                   lun_capacity * index /* offset */);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    } /* End of edge creation from raid to VD's */

    /* Must commit the transation before waiting on the objects.
     */
    status = fbe_api_database_transaction_commit(transaction_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for all PVD objects to be ready before continuing.
     */
    for (index = 0; index < width; ++index)
    {
        /* Wait up to 20 seconds for this object to become ready.
         */
        status = fbe_api_wait_for_object_lifecycle_state(pvds[index],
                                                         FBE_LIFECYCLE_STATE_READY, 
                                                         20000, 
                                                         FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    
    /* Wait for all VD objects to be ready before continuing.
     */
    for (index = 0; index < width; ++index)
    {
        /* Wait up to 20 seconds for this object to become ready.
         */
        status = fbe_api_wait_for_object_lifecycle_state(vd_first_object_id + index, 
                                                         FBE_LIFECYCLE_STATE_READY, 
                                                         20000, 
                                                         FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Wait for the RG.
     * Wait up to 20 seconds for this object to become ready.
     */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 
                                                     20000, 
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for all lun objects to be ready before continuing.
     */
    for (index = 0; index < num_luns; ++index)
    {
        /* Wait up to 20 seconds for this object to become ready.
         */
        status = fbe_api_wait_for_object_lifecycle_state(lun_first_object_id + index, 
                                                         FBE_LIFECYCLE_STATE_READY, 
                                                         20000, 
                                                         FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Success! \n", __FUNCTION__);

#endif
    return(status);
}
/***************************************************
 * end of scrappydoo_create_raid_1_with_debug_flags()
 ***************************************************/

/*!**************************************************************
 * scrappydoo_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 1 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  10/01/2009   Ron Proulx  - Created.
 *
 ****************************************************************/
void scrappydoo_setup(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_lba_t       lun_capacity;
    fbe_u32_t       width = SCRAPPYDOO_TEST_RAID_GROUP_WIDTH;
    fbe_lba_t       rg_capacity;
    fbe_lun_number_t lun_number = 0;
    fbe_raid_group_number_t raid_group_id = 0;
            
    /* Get the raw capacity of the raid group.
     */
    rg_capacity = scrappydoo_capacity_array[0];
    
    /* Get the lun capacity.
     */
    lun_capacity =  fbe_test_sep_util_get_blocks_per_lun(rg_capacity,
                                                         SCRAPPYDOO_TEST_LUNS_PER_RAID_GROUP,
                                                         1,
                                                         SCRAPPYDOO_TEST_ELEMENT_SIZE);

    /* Load the physical package and create the physical configuration. 
     */
    elmo_physical_config();

    /* Load the logical packages. 
     */
    sep_config_load_sep_and_neit();

    /* Config a raid-1 with the `force metadata I/O ' flag set.
     */
    status = scrappydoo_create_raid_1_with_debug_flags(SCRAPPYDOO_TEST_LUNS_PER_RAID_GROUP,
                                                       &raid_group_id,
                                                       &lun_number,
                                                       SCRAPPYDOO_TEST_ELEMENT_SIZE,
                                                       SCRAPPYDOO_TEST_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS,
                                                       rg_capacity,
                                                       lun_capacity,
                                                       width,
                                                       SCRAPPYDOO_TEST_RAID_GROUP_CHUNK_SIZE,
                                                       0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return;                                                                  
}                                                                            
/**************************************                                      
 * end scrappydoo_setup()                                                     
 **************************************/

/*!**************************************************************
 * scrappydoo_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the big_bird test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from big_bird_test.c
 *
 ****************************************************************/
void scrappydoo_cleanup(void)
{
    /* Destroy and unload: sep, neit and physical.
     */
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/******************************************
 * end scrappydoo_cleanup()
 ******************************************/

/***************************
 * end file scrappydoo_test.c
 ***************************/


