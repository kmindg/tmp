/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file diego_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests of simple rebuilds.  
 *
 * @version
 *   12/01/2009 - Created. Jean Montes
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:


#include "sep_rebuild_utils.h"                      //  .h shared between Dora and other NR/rebuild tests
#include "mut.h"                                    //  MUT unit testing infrastructure 
#include "fbe/fbe_types.h"                          //  general types
#include "sep_tests.h"                              //  for dora_setup()
#include "sep_test_io.h"                            //  for fbe_test_sep_io_xxx
#include "fbe/fbe_api_rdgen_interface.h"            //  for fbe_api_rdgen_context_t
#include "fbe_test_configurations.h"                //  for Diego public functions 
#include "fbe/fbe_api_raid_group_interface.h"       //  for fbe_api_raid_group_get_info_t
#include "fbe/fbe_api_database_interface.h"         //  for fbe_api_database_lookup_raid_group_by_number
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_utils.h"                      //  for fbe_api_wait_for_number_of_objects
#include "pp_utils.h"                               //  for fbe_test_pp_util_pull_drive, _reinsert_drive
#include "fbe_private_space_layout.h"				//  for diego_test_get_private_space_luns
#include "fbe/fbe_api_provision_drive_interface.h"	//  for diego_test_remove_db_drive_and_verify
#include "fbe/fbe_api_common.h"						// for diego_create_physical_config
#include "fbe/fbe_api_terminator_interface.h"		// for diego_create_physical_config
#include "fbe/fbe_api_sim_server.h"					// for diego_create_physical_config
#include "fbe/fbe_random.h"                         // for fbe_random
#include "fbe/fbe_api_scheduler_interface.h"		// for fbe_api_scheduler_add_debug_hook

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* diego_short_desc = "RAID Group Rebuild";
char* diego_long_desc =
    "\n"
    "\n"
    "The Diego Scenario is a test of normal rebuild due to disk failure.\n"
    "\n"
    "Configurations tested: R5 (3 drive), R1 (2 and 3 drive), R6 (6 drive), R10 (4 drives).\n"
    "\n"
    "\n"
    "Each RG type is tested as follows:\n"
    "\n"
    "STEP 1: Configure raid group and LUNs. Each test uses 1 RG with 3 LUNs. Each test covers 520 block size.\n"
    "\n"
    "STEP 2: Write a background pattern to all LUNs using rdgen.\n"
    "\n"
    "STEP 3: Pull a drive and verify the drive is in the FAIL state and rebuild logging has started.\n"
    "\n"
    "STEP 4: Configure a hot-spare.\n" 
    "\n"
    "STEP 5: Verify completion of the rebuild operation to the hot-spare.\n" 
    "\n"
    "STEP 6: Verify that the rebuilt data is correct using rdgen.\n" 
    "\n"
    "STEP 7: Reinsert the removed drive.\n"
    "\n"
    "Steps 1-7 are repeated for each RG type with the following additions:\n"
    "\n"
    "           R10, R6: For Step 3, two drives are removed.\n"
    "                    For Step 4, two hot-spare drives are configured.\n"
    "                    For Step 5, rebuild verified on both hot-spares.\n"
    "\n"
    "           R6: Additional test follows the same basic steps; however, the second drive is removed\n"
    "                while the rebuild of the first is in progress.\n"
    "               After the rebuild to first hot-spare completes, data is verified.\n"
    "               Then second hot-spare configured for second drive and after rebuild, data is verified.\n"
    "\n"
    "           R1, 2-way:  The same basic steps are followed; however, the secondary mirror drive is removed first.\n"
    "                       After Step 7, the primary mirror drive is removed to force read from secondary.\n"
    "                       Data is verified on the secondary.\n"
    "                       Then Steps 3-7 are repeated for the primary mirror.\n"
    "\n"
    "           R1, 3-way:  For Step 3, two drives are removed (drive in secondary and tertiary positions).\n"
    "                       For Step 4, two hot-spare drives are configured.\n"
    "                       For Step 5, rebuild verified on both hot-spares.\n"
    "                       Step 6: Drive in primary position is removed to force read from secondary.\n"
    "                       Step 7: Data is verified on secondary position.\n"
    "                       Step 8: Drive in secondary position is removed to force read from tertiary.\n"
    "                       Step 9: Data is verified on tertiary position.\n"
    "                       Then Step 4 and 5 are repeated for the primary and secondary drives that are now removed.\n"
    "\n"
    "           R1, 3-way:  Additional test removes the primary drive while the rebuild of the secondary is in progress.\n"
    "                       After the rebuild to first hot-spare completes, data is verified.\n"
    "                       Then second hot-spare configured for primary drive and after rebuild, data is verified.\n"
    "\n"
    "Added tests for failure of PSM/vault drive where some RG are rebuild logging\n"
    "\n"
    "\n"
    "Outstanding issues:\n"
    "\n"
    "1.  Rebuild of a PSM/vault drive that contains a user RG (this test is commented out).\n" 
    "2.  Self-rebuild (differential rebuild).\n"
    "3.  Self-rebuild is in progress and the disk dies; new disk is inserted/hot \n" 
    "    spare swap in and rebuilds entire disk.\n" 
    "4.  Rebuild to HS is in progress and the HS dies; rebuilds entire disk to a \n"
    "    new HS.\n"
    "5.  Make sure data is written correctly when above and below the checkpoint.\n"
    "6.  A rebuild I/O gets a failure - all RAID types.\n"
    "7.  Persistence of rebuild checkpoint and bitmap data.\n"
    "8.  Support for back-end scheduler interface.\n"
    "9.  Support for I/O load (red/yellow/green).\n"
    "10.  Dual SP including:\n"
    "       a- Active SP dies during rebuild\n"
    "       b- Active SP has a single loop failure during rebuild\n"
    "       (basic dual-SP test commented out)\n"
    "11. Array dies during rebuild - rebuild should start from where it left off.\n"
    "12. Enabling disk caching during rebuild if any supported drives require it.\n"
    "13. Test with mix of 520 and 512 block sizes.\n"
    "\n"
    "\n"
    "The following interactions between rebuild and other services/entities are \n"
    "intended to be covered in scenarios related to those services, or in other \n"
    "'interaction' scenarios:\n"
    "\n"
    "1.  LUN zero is in progress and a disk dies, swaps to a HS, and rebuilds.\n"
    "2.  BV is in progress and a disk dies, swaps to a HS, and rebuilds.\n"
    "3.  Proactive copy is in progress and the proactive candidate disk dies, \n" 
    "    rebuilds to the HS\n"
    "       a- Drive with single RG\n"
    "       b- PSM/vault drive\n"
    "4.  While rebuilding to HS, original disk is re-inserted - rebuild must \n"
    "    continue to the Hot Spare, not original drive.\n"
    "5.  Support for inter-object background priority scheme including:\n"
    "       a- Rebuild of one disk in RAID-5 RG when another disk was proactive \n"
    "          copying; the Paco should stop and the rebuild should run.\n"
"\n"
"Desription last updated: 10/27/2011.\n"    ;


//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:

//  Context for rdgen.  This is an array with an entry for every LUN defined by the test.  The context is used
//  to initialize all the LUNs of the test in a single operation. 
static fbe_api_rdgen_context_t fbe_diego_test_context_g[SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP * SEP_REBUILD_UTILS_NUM_RAID_GROUPS];
                                                        
//  Global variables from the Dora test declared as extern 
extern fbe_u32_t        sep_rebuild_utils_number_physical_objects_g;
extern fbe_u32_t        sep_rebuild_utils_next_hot_spare_slot_g;
extern fbe_test_rg_configuration_t dora_r5_rg_config_g[];
extern fbe_test_rg_configuration_t dora_r5_2_rg_config_g;
extern fbe_test_rg_configuration_t dora_r1_rg_config_g[];
extern fbe_test_rg_configuration_t dora_r10_rg_config_g[];
extern fbe_test_rg_configuration_t dora_triple_mirror_rg_config_g[];
extern fbe_test_rg_configuration_t dora_triple_mirror_2_rg_config_g[];
extern fbe_test_rg_configuration_t dora_r6_rg_config_g[]; 
extern fbe_test_rg_configuration_t dora_r6_2_rg_config_g[]; 
extern fbe_test_rg_configuration_t dora_db_drives_rg_config_g; 

extern fbe_test_raid_group_disk_set_t fbe_dora_r5_2_disk_set_g[SEP_REBUILD_UTILS_R5_RAID_GROUP_WIDTH];
extern fbe_test_raid_group_disk_set_t fbe_dora_db_drives_disk_set_g[SEP_REBUILD_UTILS_DB_DRIVES_RAID_GROUP_WIDTH];

/*!*******************************************************************
 * @def diege_db_drives_rg_config_g
 *********************************************************************
 * @brief the db RG config information .
 *
 *********************************************************************/
 fbe_test_rg_configuration_t diego_db_drives_rg_config_g[2] = 
 {
	//	Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
	{3, 0x057DB800, FBE_RAID_GROUP_TYPE_RAID1,FBE_CLASS_ID_MIRROR, 520, FBE_RAID_GROUP_TYPE_RAID1, 0},
	{FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
 };

/*!*******************************************************************
 * @def diego_vault_drives_rg_config_g
 *********************************************************************
 * @brief the vault RG config information .
 *
 *********************************************************************/
 fbe_test_rg_configuration_t diego_vault_drives_rg_config_g[2] = 
 {
	//	Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
	{4, 0x3c03000, FBE_RAID_GROUP_TYPE_RAID3,FBE_CLASS_ID_PARITY, 520, FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT, 0},
	{FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
 };

/*!*******************************************************************
 * @def fbe_diego_vault_drives_disk_set_g
 *********************************************************************
 * @brief the disk set to be used by the vault RG.
 *
 *********************************************************************/
fbe_test_raid_group_disk_set_t fbe_diego_vault_drives_disk_set_g[4] =
{
    {SEP_REBUILD_UTILS_PORT, SEP_REBUILD_UTILS_ENCLOSURE_FIRST, 0},            // 0-0-0
    {SEP_REBUILD_UTILS_PORT, SEP_REBUILD_UTILS_ENCLOSURE_FIRST, 1},            // 0-0-1
    {SEP_REBUILD_UTILS_PORT, SEP_REBUILD_UTILS_ENCLOSURE_FIRST, 2},            // 0-0-2
    {SEP_REBUILD_UTILS_PORT, SEP_REBUILD_UTILS_ENCLOSURE_FIRST, 3}             // 0-0-3
};  

/*!*******************************************************************
 * @enum diego_system_raid_group_test_index_e
 *********************************************************************
 * @brief This is an test index enum for the diego system raid group test
 *
 *********************************************************************/
typedef enum diego_system_raid_group_test_index_e
{
    DIEGO_SYSTEM_RAID_GROUP_TEST_CASE_1 = 0,
    DIEGO_SYSTEM_RAID_GROUP_TEST_CASE_2 = 1,

    /*!@note Add new test index here.*/
    DIEGO_SYSTEM_RAID_GROUP_TEST_LAST,
} diego_system_raid_group_test_index_t;

/*!*******************************************************************
 * @def DIEGO_SYSTEM_RAID_GROUP_NUM_TEST_CASES
 *********************************************************************
 * @brief Number of test cases for system raid groups.
 *
 *********************************************************************/
 #define DIEGO_SYSTEM_RAID_GROUP_NUM_TEST_CASES 2

/*!*******************************************************************
 * @def ELMO_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects we will create.
 *        (1 board + 1 port + 4 encl + 60 pdo) 
 *
 *********************************************************************/
 #define DIEGO_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66
/*!*******************************************************************
 * @def DIEGO_DRIVES_PER_ENCL
 *********************************************************************
 * @brief Allows us to change how many drives per enclosure we have.
 *
 *********************************************************************/
 #define DIEGO_DRIVES_PER_ENCL 15

 /*!*******************************************************************
 * @def DIEGO_SYSTEM_HS_DISK_SLOT
 *********************************************************************
 * @brief The slot of the system hot spare.
 *        Enclosure 0-0  has been reserved for system Hot Spares
 *
 *********************************************************************/
 #define DIEGO_SYSTEM_HS_DISK_SLOT 			7

 /*!*******************************************************************
 * @def DIEGO_VAULT_DRIVES_RAID_GROUP_WIDTH
 *********************************************************************
 * @brief The width of vault raid group.
 *
 *********************************************************************/
 #define DIEGO_VAULT_DRIVES_RAID_GROUP_WIDTH 4 
 /*!*******************************************************************
 * @def DIEGO_PSM_DRIVES_RAID_GROUP_WIDTH
 *********************************************************************
 * @brief The width of psm (triple mirror) raid group.
 *
 *********************************************************************/
 #define DIEGO_PSM_DRIVES_RAID_GROUP_WIDTH 3 

//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:

//  Perform test on database RG which is without user RG
static void diego_test_db_drives_rebuild( fbe_test_rg_configuration_t*    in_rg_config_p);
//	Verify all the private space LUNs 
static void diego_test_get_private_space_luns(void);
//	Verify all the private space RGs 
static void diego_test_get_private_space_rgs(void);
//	Remove a DB drive and verify the objects change  status 
static void diego_test_remove_db_drive_and_verify(
                                fbe_test_rg_configuration_t*        in_rg_config_p,
                                fbe_u32_t                           in_position,
                                fbe_u32_t                           in_num_objects,
                                fbe_api_terminator_device_handle_t* out_drive_info_p)	;
//	Congfig the disk set of database RG which may be removed
static void diego_test_config_db_rg_disk_set(fbe_test_rg_configuration_t*  in_rg_config_p, fbe_test_raid_group_disk_set_t*  in_rg_disk_set);
//  
static void diego_db_rebuild_wait_for_rb_comp_by_obj_id(
                                    fbe_object_id_t                 in_raid_group_object_id,
                                    fbe_u32_t                       in_position);


//  Perform all tests on a RAID-5 or RAID-3
static void diego_test_main_r5_r3(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, void * context_p);

//  Perform test on a database drive that contains a user RG 
static void diego_test_db_drives(
                                        fbe_test_rg_configuration_t*    in_rg_config_p);

//  Perform basic rebuild tests on a RAID-1
static void diego_test_r1_basic_rebuild(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, void * context_p);

//  Perform tests of error cases on a RAID-1
static void diego_test_r1_error_cases(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, void * context_p);

//  Perform basic rebuild tests on a triple mirror RAID-1
static void diego_test_triple_mirror_basic_rebuild(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, void * context_p);

//  Perform tests of error cases on a triple mirror RAID-1
static void diego_test_triple_mirror_error_cases(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, void * context_p);

//  Perform basic rebuild tests on a RAID-6 or RAID-10
static void diego_test_r6_r10_basic_rebuild(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, void * context_p);

//  Perform tests of error cases on a RAID-6
static void diego_test_r6_error_cases(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, void * context_p);

//  Perform basic rebuild tests on dual sp
static void diego_test_basic_dual_sp_rebuild(
                                        fbe_test_rg_configuration_t*    in_rg_config_p);

//  Perform basic rebuild tests on system raid groups
static void diego_test_private_space_rgs(void);
static void diego_test_private_space_rgs_test_case_1(void);
static void diego_test_private_space_rgs_test_case_2(void);


//-----------------------------------------------------------------------------
//  FUNCTIONS:


/*!****************************************************************************
 *  diego_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Diego test.  The test does the 
 *   following:
 *
 *   - Creates raid groups and Hot Spares as needed 
 *   - For each raid group, tests rebuild of one or more drives  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void diego_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
	fbe_lun_number_t        next_lun_id;        //  sets/gets number of the next lun (0, 1, 2)


    //  Initialize configuration setup parameters - start with 0
    next_lun_id = 0; 
    //------------------------------------------------------------------------
    //  Test RAID-5 
    //

	rg_config_p = &dora_r5_rg_config_g[0];
    //  Set up and perfom tests on RAID-5 raid group
  	fbe_test_run_test_on_rg_config_with_extra_disk_with_time_save(rg_config_p, NULL,diego_test_main_r5_r3 ,
    		SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN,
                                   FBE_FALSE);


    //--------------------------------------------------------------------------
    //  Test RAID-1 2-way mirror - basic rebuilds
    //

    rg_config_p = &dora_r1_rg_config_g[0];
    //  Set up and perfom tests on RAID-5 raid group
    fbe_test_run_test_on_rg_config_with_extra_disk_with_time_save(rg_config_p, NULL,diego_test_r1_basic_rebuild ,
    		SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN,
                                   FBE_FALSE);



    //--------------------------------------------------------------------------
    //  Test RAID-10 - basic rebuilds
    //

    rg_config_p = &dora_r10_rg_config_g[0];
    //  Set up and perfom tests on RAID-10 raid group
    mut_printf(MUT_LOG_TEST_STATUS, "Starting tests for RAID-10\n");
    fbe_test_run_test_on_rg_config_with_extra_disk_with_time_save(rg_config_p, NULL,diego_test_r6_r10_basic_rebuild ,
    		SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN,
                                   FBE_FALSE);

    //------------------------------------------------------------------------
    //  Test rebuild on the database drives with a user RG configured 
    //

/*!@ todo - Temporarily removed to avoid issues on hardware.  7/26/10 JMM. 

    //  Set up a raid group on the DB drives 
    dora_setup_raid_group(&dora_db_drives_rg_config_g, &fbe_dora_db_drives_disk_set_g[0], &next_lun_id);

    //  Perform all tests on the system drives
    diego_test_db_drives(&dora_db_drives_rg_config_g);
*/

    //--------------------------------------------------------------------------
    //  Test RAID-6 - basic rebuilds
    //

    rg_config_p = &dora_r6_rg_config_g[0];
    //  Set up and perfom tests on RAID-5 raid group
    mut_printf(MUT_LOG_TEST_STATUS, "Starting tests for RAID-6\n");
    fbe_test_run_test_on_rg_config_with_extra_disk_with_time_save(rg_config_p, NULL,diego_test_r6_r10_basic_rebuild ,
    		SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN,
                                   FBE_FALSE);

    //--------------------------------------------------------------------------
    //  Test RAID-6 - error cases 
    //

    rg_config_p = &dora_r6_2_rg_config_g[0];
    //  Set up and perfom error tests on  RAID-6 raid group
    fbe_test_run_test_on_rg_config_with_extra_disk_with_time_save(rg_config_p, NULL,diego_test_r6_error_cases ,
    		SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN,
                                   FBE_FALSE);

    //--------------------------------------------------------------------------
    //  Test RAID-1 3-way mirror - basic rebuilds
    //

    rg_config_p = &dora_triple_mirror_rg_config_g[0];
    //  Set up and perfom tests on 3-way mirror RAID-1 triple mirror raid group
    fbe_test_run_test_on_rg_config_with_extra_disk_with_time_save(rg_config_p, NULL,diego_test_triple_mirror_basic_rebuild ,
    		SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN,
                                   FBE_FALSE);
    


    //--------------------------------------------------------------------------
    //  Test RAID-1 3-way mirror - error cases 
    //

    rg_config_p = &dora_triple_mirror_2_rg_config_g[0];
    //  Set up and perfom error tests on 3-way mirror RAID-1 raid group
    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, NULL,diego_test_triple_mirror_error_cases,
    		SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN);

    //--------------------------------------------------------------------------
    //  Test dual SP - basic rebuilds on dual-SP
    //
    
    /*  Removed 3/15/11 as SP B is hitting issues during teardown 
    //  Set up a second RAID-5 raid group
    dora_setup_raid_group(&dora_r5_2_rg_config_g, &fbe_dora_r5_2_disk_set_g[0], &next_lun_id);
    
    // Perform basic rebuild on dual-SP
    mut_printf(MUT_LOG_TEST_STATUS, "Starting rebuild on dual-SP \n");
    diego_test_basic_dual_sp_rebuild(&dora_r5_2_rg_config_g);
    */

    /*Temporarily removed to avoid issues on hardware.
    US701( system hot spare copyback to the reinsert system dirver)can fix the hardware issues.
    SO db driver rebuild can run on hardware when US701 is finished
	//------------------------------------------------------------------------
	//	Test rebuild on the database drives without user RG on the db drives
		
	//	Config the disk set of the system RG which may be removed
	diego_test_config_db_rg_disk_set(&diego_db_drives_rg_config_g[0],&fbe_dora_db_drives_disk_set_g[0]);
	//	Perform all tests on the db drives
	diego_test_db_drives_rebuild(&diego_db_drives_rg_config_g[0]);*/


    //--------------------------------------------------------------------------
    //  Test system raid group cases 
    //
    diego_test_private_space_rgs();

    return;

} //  End diego_test()


/*!**************************************************************
 * diego_create_physical_config()
 ****************************************************************
 * @brief
 *  Configure the diego test's physical configuration. Note that
 *  only 4160 block SAS and 520 block SAS drives are supported.
 *  We make all the disks(0_0_x) have the same capacity in order 
 *  to ensure the system HS drives have the same capacity with 
 *  the removed system drives.If not, we will not get the suitable 
 *  system HS disk.
 *
 * @param b_use_4160 - TRUE if we will use both 4160 and 520.              
 *
 * @return None.
 *
 * @author
 *  10/19/2011 - Created. Zhangy
 *
 ****************************************************************/

void diego_create_physical_config(fbe_bool_t b_use_4160)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;

    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;

    fbe_api_terminator_device_handle_t  port0_handle;
    fbe_api_terminator_device_handle_t  encl0_0_handle;
    fbe_api_terminator_device_handle_t  encl0_1_handle;
    fbe_api_terminator_device_handle_t  encl0_2_handle;
    fbe_api_terminator_device_handle_t  encl0_3_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t slot;
    fbe_u32_t secondary_block_size;

    if (b_use_4160)
    {
        secondary_block_size = 4160;
    }
    else
    {
        secondary_block_size = 520;
    }

    mut_printf(MUT_LOG_LOW, "=== %s configuring terminator ===", __FUNCTION__);
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                 2, /* portal */
                                 0, /* backend number */ 
                                 &port0_handle);

    /* insert an enclosures to port 0
     */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 1, port0_handle, &encl0_1_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 2, port0_handle, &encl0_2_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 3, port0_handle, &encl0_3_handle);

    /* Insert drives for enclosures.
     */
    for ( slot = 0; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
       
	   //disks 0_0_x should have the same capacity to ensure the system HS drives have the same capacity with the removed system drives
	   //diego_test_db_drives_rebuild needs this change
	   fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }

    for ( slot = 0; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 1, slot, 520, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }

    /*! @note Native SATA drives are no longer supported.
     */
    for ( slot = 0; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 2, slot, secondary_block_size, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }

    /*! @note Native SATA drives are no longer supported.
     */
    for ( slot = 0; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 3, slot, secondary_block_size, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }

    mut_printf(MUT_LOG_LOW, "=== starting physical package===");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(DIEGO_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===");

    /* We inserted a armada board so we check for it.
     * board is always assumed to be object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == DIEGO_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");
    fbe_test_pp_util_enable_trace_limits();
    return;
}
/******************************************
 * end diego_create_physical_config()
 *******************************************


/*!****************************************************************************
 *  diego_setup
 ******************************************************************************
 *
 * @brief
 *   This is the common setup function for the Diego test.  It creates the  
 *   physical configuration and loads the packages.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void diego_setup(void)
{  
	//sep_rebuild_utils_setup();

    /* Initialize any required fields and perform cleanup if required
     */
    //fbe_test_common_util_test_setup_init();
    // Temporarily use the setup function diego_create_physical_config to enable the db drive rebuild test  
	if (fbe_test_util_is_simulation())
    {
        //  Load the physical package and create the physical configuration. 
        diego_create_physical_config(FBE_FALSE /* Do not use 4160 block drives */);

        //  Load the logical packages
        sep_config_load_sep_and_neit();
    }

    //  Initialize the next hot spare slot to 0
    sep_rebuild_utils_next_hot_spare_slot_g = 0;

    fbe_test_common_util_test_setup_init();

} //  End diego_setup()


/*!****************************************************************************
*	diego_test_db_drives_rebuild
******************************************************************************
*
* @brief
*	 This is the test function for rebuilding database RG. The test does not setup a
*	 user RG on the db drives,It does the following:
*
*	 - Configures the disk set will be removed in database RG
*	 - Configures a Hot Spare 
*	 - Removes a db drive, which will swap to the Hot Spare 
*    - Waits for the rebuilds of the system RG
*    - Re-insert the removed drive
*	
* @param in_rg_config_p	 - pointer to the system RG configuration info 
*
* @return	None 
*
* 9/27/2011 - Created by zhangy
*****************************************************************************/

static void diego_test_db_drives_rebuild( fbe_test_rg_configuration_t*    in_rg_config_p)
{

    fbe_api_terminator_device_handle_t  drive_info_1;       //  drive info needed for swap-in 
    fbe_object_id_t                     system_raid_group_id;
	fbe_u32_t 							total_objects=0;
	fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);
    //  Set up a system drive hot spare 
    sep_rebuild_utils_setup_hot_spare(0, 0, DIEGO_SYSTEM_HS_DISK_SLOT);	
	//	Verify the system RG 
	diego_test_get_private_space_rgs();
	//	Verify the private LUN
	diego_test_get_private_space_luns();	
	// Get the total number of objects in the system
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);	
	//	Adjust the number of physical objects expected
	total_objects = total_objects - 1;	
	//	Remove a single drive in the RG.  Check the PVD and VD change statues correctly 
	diego_test_remove_db_drive_and_verify(in_rg_config_p,SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS,total_objects,&drive_info_1);	
	// Get the id of the system raid group
	system_raid_group_id=FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR;	
    //  For the system RG, verify that rb logging is set and that the rebuild checkpoint is 0
    sep_rebuild_utils_verify_rb_logging_by_object_id(system_raid_group_id, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);
    //  Wait until the rebuild finishes for the system RG
    diego_db_rebuild_wait_for_rb_comp_by_obj_id(system_raid_group_id, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);
	//	Verify the system RG 
	diego_test_get_private_space_rgs();
	//	Verify the private LUN
	diego_test_get_private_space_luns();
    //  Re-insert the removed drive
    status = fbe_test_pp_util_reinsert_drive(in_rg_config_p->rg_disk_set[SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS].bus,
											 in_rg_config_p->rg_disk_set[SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS].enclosure,
											 in_rg_config_p->rg_disk_set[SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS].slot,
											 drive_info_1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
} //  End diego_test_db_drives_rebuild

/*!****************************************************************************
*	diego_test_get_private_space_luns
******************************************************************************
*
* @brief
*	This function verify all the private luns.
*	
* @param 	None 
*
* @return	None 
*
* 9/27/2011 - Created by zhangy
*****************************************************************************/
static void diego_test_get_private_space_luns(void)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lun_number_t lun_number;
    fbe_object_id_t index;
    fbe_private_space_layout_lun_info_t lun_info;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Test binding of private luns ... ===\n");
    for (index = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_FIRST; index <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_LAST ; index++)
    {
        status = fbe_private_space_layout_get_lun_by_object_id(index, &lun_info);
        if(status == FBE_STATUS_OK) {
            status = fbe_api_database_lookup_lun_by_object_id(index,&lun_number);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }
} //  End diego_test_get_private_space_luns()

/*!****************************************************************************
*	diego_test_get_private_space_rgs
******************************************************************************
*
* @brief
*	This function verify all the private rgs.
*	
* @param 	None 
*
* @return	None 
*
* 9/27/2011 - Created by zhangy
*****************************************************************************/

static void diego_test_get_private_space_rgs(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lun_number_t lun_number;
    fbe_object_id_t index;
    fbe_private_space_layout_region_t region;

    mut_printf(MUT_LOG_TEST_STATUS, "Test all private space RGs ...\n");
    for (index = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_FIRST; index <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_LAST ; index++)
    {
        status = fbe_private_space_layout_get_region_by_raid_group_id(index, &region);
        if(status == FBE_STATUS_OK) {
            status = fbe_api_database_lookup_raid_group_by_object_id(index,&lun_number);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }
}//End diego_test_get_private_space_rgs

/*!****************************************************************************
*	diego_test_remove_db_drive_and_verify
******************************************************************************
*
* @brief
*	This function remove a db drive and verified PVD and VD change statues accordingly.
*	
* @param 	None 
*
* @return	None 
*
* 9/27/2011 - Created by zhangy
*****************************************************************************/
static void diego_test_remove_db_drive_and_verify(
                                fbe_test_rg_configuration_t*        in_rg_config_p,
                                fbe_u32_t                           in_position,
                                fbe_u32_t                           in_num_objects,
                                fbe_api_terminator_device_handle_t* out_drive_info_p)
{

	 	fbe_status_t				 status;							 // fbe status
		fbe_object_id_t 			raid_group_object_id;				// raid group's object id
		fbe_object_id_t 			pvd_object_id;						// pvd's object id
		fbe_object_id_t 			vd_object_id;						// vd's object id

		//	Get the PVD object id before we remove the drive
		status = fbe_api_provision_drive_get_obj_id_by_location(in_rg_config_p->rg_disk_set[in_position].bus,
																in_rg_config_p->rg_disk_set[in_position].enclosure,
																in_rg_config_p->rg_disk_set[in_position].slot,
																&pvd_object_id);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		mut_printf(MUT_LOG_TEST_STATUS, "removing drive: %d - %d - %d\n", in_rg_config_p->rg_disk_set[in_position].bus, in_rg_config_p->rg_disk_set[in_position].enclosure, in_rg_config_p->rg_disk_set[in_position].slot);
		//	Remove the drive
		if (fbe_test_util_is_simulation())
		{
			status = fbe_test_pp_util_pull_drive(in_rg_config_p->rg_disk_set[in_position].bus,
												 in_rg_config_p->rg_disk_set[in_position].enclosure,
												 in_rg_config_p->rg_disk_set[in_position].slot,
												 out_drive_info_p);
			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			
			//	Verify the PDO and LDO are destroyed by waiting for the number of physical objects to be equal to the
			//	value that is passed in. Note that drives are removed differently on hardware; this check applies
			//	to simulation only.
			status = fbe_api_wait_for_number_of_objects(in_num_objects, 10000, FBE_PACKAGE_ID_PHYSICAL);
			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);	   
		}
		else
		{
			status = fbe_test_pp_util_pull_drive_hw(in_rg_config_p->rg_disk_set[in_position].bus,
													in_rg_config_p->rg_disk_set[in_position].enclosure,
													in_rg_config_p->rg_disk_set[in_position].slot);
			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);		 
		}
	
		//	Print message as to where the test is at
		mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d", 
				   in_rg_config_p->rg_disk_set[in_position].bus,
				   in_rg_config_p->rg_disk_set[in_position].enclosure,
				   in_rg_config_p->rg_disk_set[in_position].slot);
	
		//	Get the RG's object id.  We need to this get the VD's object it.
		raid_group_object_id=FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR;
		
		//	Get the VD's object id
		fbe_test_sep_util_get_virtual_drive_object_id_by_position(raid_group_object_id, in_position, &vd_object_id);
	
		//	Print message as to where the test is at
		mut_printf(MUT_LOG_TEST_STATUS, "RG: 0x%X, PVD: 0x%X, VD: 0x%X\n", 
				   raid_group_object_id, pvd_object_id, vd_object_id);
	
		//	Verify that the PVD and VD objects are in the FAIL state
		sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
		sep_rebuild_utils_verify_sep_object_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL);

		return;
	
}//End diego_test_remove_db_drive_and_verify
/*!****************************************************************************
*	diego_test_config_db_rg_disk_set
******************************************************************************
*
* @brief
*	This function configs the disk set of the database RG which may be removed.
*	
* @param 	None 
*
* @return	None 
*
* 9/27/2011 - Created by zhangy
*****************************************************************************/
static void diego_test_config_db_rg_disk_set(fbe_test_rg_configuration_t* in_rg_config_p, fbe_test_raid_group_disk_set_t*  in_rg_disk_set)
{
	fbe_u32_t 							index;
	MUT_ASSERT_TRUE(NULL != in_rg_config_p);
	MUT_ASSERT_TRUE(NULL != in_rg_disk_set);
	fbe_test_sep_util_init_rg_configuration_array(in_rg_config_p);
	for(index =0; index < in_rg_config_p->width; index++){
		in_rg_config_p->rg_disk_set[index].bus=in_rg_disk_set[index].bus;
		in_rg_config_p->rg_disk_set[index].enclosure=in_rg_disk_set[index].enclosure;
		in_rg_config_p->rg_disk_set[index].slot=in_rg_disk_set[index].slot;
	}
	//  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Config the disk set may be moved successfully\n", __FUNCTION__);
	return;
}//End diego_test_config_db_rg_disk_set
/*!**************************************************************************
 *  diego_db_rebuild_wait_for_rb_comp_by_obj_id
 ***************************************************************************
 * @brief
 *   This function waits for the db drive rebuild to complete.
 *   Temporarily, We only wait for the rebuild checkpoint is increasing 
 *   due to the db drive rebiuld takes a very long time may cause over time
 *
 * @param in_raid_group_object_id   - raid group's object id
 * @param in_position               - disk position in the RG
 *
 * @return void
 *
 * 10/19/2011 - Created by zhangy
 ***************************************************************************/
static void diego_db_rebuild_wait_for_rb_comp_by_obj_id(
                                    fbe_object_id_t                 in_raid_group_object_id,
                                    fbe_u32_t                       in_position)
{

    fbe_status_t                    status;                         //  fbe status
    fbe_u32_t                       count;                          //  loop count
    fbe_u32_t                       max_retries;                    //  maximum number of retries
    fbe_api_raid_group_get_info_t   raid_group_info;                //  rg information from "get info" command
    fbe_bool_t                      b_option_found = FBE_FALSE;     //  full rebuild or not
    // If user specify the option"-full_db_drive_rebuild",we will loop until the rebuild checkpoint is set to the logical end marker
    // else, We only wait for the rebuild checkpoint is increasing 
    if (mut_option_selected("-full_db_drive_rebuild"))
    {
        b_option_found = FBE_TRUE;
    }
    //  Set the max retries in a local variable. (This allows it to be changed within the debugger if needed.)
	max_retries = 8000;
    //  Loop until the the rebuild checkpoint is set to the logical end marker for the drive that we are rebuilding.
    if (b_option_found) {
        for (count = 0; count < max_retries; count++)
        {
            //  Get the raid group information
            status = fbe_api_raid_group_get_info(in_raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            mut_printf(MUT_LOG_LOW, "== rg object id: 0x%x pos: %d rebuild checkpoint is at 0x%llx ==", 
                   in_raid_group_object_id, in_position,
		   (unsigned long long)raid_group_info.rebuild_checkpoint[in_position]);

            //  If the rebuild checkpoint is set to the logical marker for the given disk, then the rebuild has
            //  finished - return here
        
            if (raid_group_info.rebuild_checkpoint[in_position] == FBE_LBA_INVALID)
            {
                return;
            }
            //  Wait before trying again
            fbe_api_sleep(SEP_REBUILD_UTILS_RETRY_TIME);
        }
    }else{
        for (count = 0; count < max_retries; count++)
        {
            //  Get the raid group information
            status = fbe_api_raid_group_get_info(in_raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            mut_printf(MUT_LOG_LOW, "== rg object id: 0x%x pos: %d rebuild checkpoint is at 0x%llx ==", 
                   in_raid_group_object_id, in_position,
		   (unsigned long long)raid_group_info.rebuild_checkpoint[in_position]);
            //  We only wait for the rebuild checkpoint is increasing due to the db drive rebiuld 
            //  takes a very long time may cause over time problem
            if (raid_group_info.rebuild_checkpoint[in_position] >= 40960)
            {
                return;
            }
            //  Wait before trying again
            fbe_api_sleep(SEP_REBUILD_UTILS_RETRY_TIME);
        }
    }
    //  The rebuild operation did not finish within the time limit - assert
    MUT_ASSERT_TRUE(0);
    //  Return (should never get here)
    return;
}   // End diego_db_rebuild_wait_for_rb_comp_by_obj_id()

/*!****************************************************************************
 *  diego_test_main_r5_r3
 ******************************************************************************
 *
 * @brief
 *   This is the main test function for R5 and R3 RAID groups.  It does the 
 *   following:
 *
 *   - Writes a seed pattern 
 *   - Configures a Hot Spare
 *   - Removes a drive, which will swap to the Hot Spare 
 *   - Waits for the rebuild to finish
 *   - Verifies that the data is correct 
 *
 * @param in_rg_config_p     - pointer to the RG configuration info 
 *
 * @return  None 
 * 
 * 6/23/2011 - Modified Vishnu Sharma Modified to run on Hardware
 *
 *****************************************************************************/
static void diego_test_main_r5_r3(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, void * context_p)
{

    fbe_status_t                        status;
    fbe_object_id_t                     raid_group_id;

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    if(fbe_test_rg_config_is_enabled(in_rg_config_p)){

        /* set library debug flags 
            */
        status = fbe_api_raid_group_set_library_debug_flags(raid_group_id, 
                                                        (FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING | 
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING |
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING |
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_TRACING));
        /* set raid group debug flags 
         */
        fbe_test_sep_util_set_rg_debug_flags_both_sps(in_rg_config_p, 
                                                      FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING);
            
        //  Get the number of physical objects in existence at this point in time.  This number is 
        //  used when checking if a drive has been removed or has been reinserted.   
        status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        //  Write a seed pattern to the RG 
        sep_rebuild_utils_write_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
        
        fbe_test_sep_util_add_removed_position(in_rg_config_p,SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);
        
        //  Remove a single drive in the RG.  Check the object states change correctly and that rebuild logging is started.
        sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS,
                                                    &in_rg_config_p->drive_handle[SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS]);
       
        //  Set up a hot spare 
        sep_rebuild_utils_setup_hot_spare(in_rg_config_p->extra_disk_set[0].bus,in_rg_config_p->extra_disk_set[0].enclosure ,
                             in_rg_config_p->extra_disk_set[0].slot); 
    

        //  Wait until the rebuild finishes.  We need to pass the disk position of the disk which is rebuilding.  
        sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);
    
        sep_rebuild_utils_check_bits(raid_group_id);

        //  Read the data and make sure it matches the seed pattern
        sep_rebuild_utils_read_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

        //  Reinsert the removed drives
        fbe_test_sep_drive_wait_extra_drives_swap_in(in_rg_config_p,1);
        
    }


    //  Return 
    return;

} //  End diego_test_main_r5_r3()


/*!****************************************************************************
 *  diego_test_db_drives
 ******************************************************************************
 *
 * @brief
 *   This is the test function for rebuilding a database drive.  It does the 
 *   following:
 *
 *   - Writes a seed pattern to the user RG on the drives
 *   - Configures a Hot Spare
 *   - Removes a drive, which will swap to the Hot Spare 
 *   - Waits for the rebuilds of the system RG and the user RG to finish
 *   - Verifies that the data is correct on the user RG
 *
 * @param in_rg_config_p     - pointer to the RG configuration info 
 *
 * @return  None 
 *
 *****************************************************************************/
static void diego_test_db_drives(
                                        fbe_test_rg_configuration_t*    in_rg_config_p)
{

    fbe_api_terminator_device_handle_t  drive_info_1;       //  drive info needed for reinsertion 
    fbe_object_id_t                     system_raid_group_id;
    fbe_object_id_t                     raid_group_id;
    fbe_status_t                        status;

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    //  Write a seed pattern to the user RG 
    sep_rebuild_utils_write_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

    //  Set up a hot spare 
    sep_rebuild_utils_setup_hot_spare(SEP_REBUILD_UTILS_PORT, SEP_REBUILD_UTILS_HOT_SPARE_ENCLOSURE, sep_rebuild_utils_next_hot_spare_slot_g);

    //  Remove a single drive in the RG.  Check the object states change correctly and that rb logging is marked for the
    //  user RG.
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);

    // Get the id of the system raid group
    fbe_api_database_get_system_object_id(&system_raid_group_id);

    //  For the system RG, verify that rb logging is set and that the rebuild checkpoint is 0
    sep_rebuild_utils_verify_rb_logging_by_object_id(system_raid_group_id, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);

    //  Wait until the rebuild finishes for each of the RGs
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);
    sep_rebuild_utils_wait_for_rb_comp_by_obj_id(system_raid_group_id, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);

    sep_rebuild_utils_check_bits(raid_group_id);
    sep_rebuild_utils_check_bits(system_raid_group_id);
    
    //  Read the data for the user RG and make sure it matches the seed pattern
    sep_rebuild_utils_read_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

    //  Return 
    return;

} //  End diego_test_db_drives()


/*!****************************************************************************
 *  diego_test_r1_basic_rebuild
 ******************************************************************************
 *
 * @brief
 *   This function tests basic rebuilds for a R1 RAID group.  It does the 
 *   following:
 *
 *   - Writes a seed pattern
 *   - Configures a Hot Spare
 *   - Removes the drive in the secondary position in the RG, which will swap 
 *     to the Hot Spare 
 *   - Waits for the rebuild to finish
 *   - Removes the drive in the primary position in the RG.  This forces it
 *     to read from the secondary drive.
 *   - Reads from the secondary drive and verifies that the data is correct 
 *   - Configures another hot spare so that the primary drive will rebuild
 *   - Reads (from the primary drive) and verifies that the data is correct 
 *
 *   At the end of this test, the RG is enabled and both positions have been
 *   swapped to permanant spares.
 * 
 * @param in_rg_config_p     - pointer to the RG configuration info 
 *
 * @return  None 
 * 6/23/2011 - Modified Vishnu Sharma Modified to run on Hardware
 *
 *****************************************************************************/
static void diego_test_r1_basic_rebuild(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, void * context_p)
{

    fbe_status_t                        status;
    fbe_object_id_t                     raid_group_id;

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    if(fbe_test_rg_config_is_enabled(in_rg_config_p)){

        /* set library debug flags 
            */
        status = fbe_api_raid_group_set_library_debug_flags(raid_group_id, 
                                                        (FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING | 
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING |
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING |
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_TRACING));
        /* set raid group debug flags 
         */
        fbe_test_sep_util_set_rg_debug_flags_both_sps(in_rg_config_p, 
                                                      FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING);
        //  Get the number of physical objects in existence at this point in time.  This number is 
        //  used when checking if a drive has been removed or has been reinserted.   
        status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        //  Write a seed pattern to the RG 
        sep_rebuild_utils_write_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
    
              
        fbe_test_sep_util_add_removed_position(in_rg_config_p,SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);
        
        //  Remove a single drive in the RG.  Check the object states change correctly and that rb logging 
        //  is marked.
        sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS,
                                             &in_rg_config_p->drive_handle[SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS]);
    
        sep_rebuild_utils_setup_hot_spare(in_rg_config_p->extra_disk_set[0].bus,in_rg_config_p->extra_disk_set[0].enclosure ,
                             in_rg_config_p->extra_disk_set[0].slot);    
    
        //  Wait until the rebuild finishes.  We need to pass the disk position of the disk which is rebuilding.  
        sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);
    
        //  Reinsert the removed drives
        fbe_test_sep_drive_wait_extra_drives_swap_in(in_rg_config_p,1);   

        // Refresh the extra drive info
        // this function does not update sep_rebuild_utils_number_physical_objects_g after insert a drive.
        fbe_test_sep_util_raid_group_refresh_extra_disk_info(in_rg_config_p,1);
        sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;

        fbe_test_sep_util_add_removed_position(in_rg_config_p,SEP_REBUILD_UTILS_POSITION_0);

        //  Remove the drive in position 0 in the RG.  This will force the read to occur from position 1 which
        //  is the drive we rebuilt.  
        sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
        sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0, sep_rebuild_utils_number_physical_objects_g,
                                     &in_rg_config_p->drive_handle[SEP_REBUILD_UTILS_POSITION_0]);
    
        //  Read the data and make sure it matches the seed pattern
        sep_rebuild_utils_read_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
    
                
        sep_rebuild_utils_setup_hot_spare(in_rg_config_p->extra_disk_set[0].bus,in_rg_config_p->extra_disk_set[0].enclosure ,
                             in_rg_config_p->extra_disk_set[0].slot); 
    
        //  Wait until the rebuild finishes 
        sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0);
    
        sep_rebuild_utils_check_bits(raid_group_id);

        //  Read the data and make sure it matches the seed pattern
        sep_rebuild_utils_read_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

        //  Reinsert the removed drives
        fbe_test_sep_drive_wait_extra_drives_swap_in(in_rg_config_p,1);   
    }

    //  Return 
    return;

} //  End diego_test_r1_basic_rebuild()


/*!****************************************************************************
 *  diego_test_r1_error_cases 
 ******************************************************************************
 *
 * @brief
 *   Not yet implemented. 
 * 
 * @param in_rg_config_p     - pointer to the RG configuration info 
 *
 * @return  None 
 *
 *****************************************************************************/
static void diego_test_r1_error_cases(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, void * context_p)
{

    //  Return - test does not do anything yet 
    return;

} //  End diego_test_r1_error_cases()


/*!****************************************************************************
 *  diego_test_triple_mirror_basic_rebuild 
 ******************************************************************************
 *
 * @brief
 *   This function tests basic rebuilds for a triple mirror RAID group.  It does 
 *   the following:
 *
 *   - Writes a seed pattern 
 *   - Removes the drive in the secondary position in the RG
 *   - Removes the drive in the third position in the RG
 *   - Configures two Hot Spares, which will swap in for each of the drives
 *   - Waits for the rebuilds of both drives to finish
 *   - Removes the drive in the primary position in the RG.  This is to force it
 *     to read from the secondary drive.
 *   - Verifies that the data is correct on the secondary drive
 *   - Removes the drive in the secondary position in the RG.  This is to force it
 *     to read from the third drive.
 *   - Verifies that the data is correct on the third drive
 *   - Configures two more Hot Spares, which will swap in for the primary and 
 *     secondary drives which are both removed at this point
 *   - Waits for the two drives to rebuild 
 *
 *   At the end of this test, the RG is enabled and all 3 positions have been
 *   swapped to permanant spares.
 * 
 * @param in_rg_config_p     - pointer to the RG configuration info 
 *
 * @return  None 
 * 6/23/2011 - Modified Vishnu Sharma Modified to run on Hardware
 *
 *****************************************************************************/
static void diego_test_triple_mirror_basic_rebuild(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, void * context_p)
{

    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t                           hot_spare_bus;                  //  bus for the swapped-in hot spare
    fbe_u32_t                           hot_spare_encl;                 //  enclosure for the swapped-in hot spare
    fbe_u32_t                           hot_spare_slot;                 //  slot for the swapped-in hot spare
    fbe_status_t                        status;
    fbe_u32_t                           i;
    

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    if(fbe_test_rg_config_is_enabled(in_rg_config_p)){
        //  Get the number of physical objects in existence at this point in time.  This number is 
        //  used when checking if a drive has been removed or has been reinserted.   
        status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        //  Write a seed pattern to the RG 
        sep_rebuild_utils_write_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
    

        fbe_test_sep_util_add_removed_position(in_rg_config_p,SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);

        //  Remove a single drive in the RG.  Check the object states change correctly and that rb logging 
        //  is marked.
        // sep_rebuild_utils_start_rb_logging_r5_r3_r1 changes sep_rebuild_utils_number_physical_objects_g
        sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS,
                                             &in_rg_config_p->drive_handle[SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS]);
    
        fbe_test_sep_util_add_removed_position(in_rg_config_p,SEP_REBUILD_UTILS_SECOND_REMOVED_DRIVE_POS);

        //  Remove a second drive in the RG.  Check that the RG stays ready and the drive is marked rb logging. 
        // sep_rebuild_utils_triple_mirror_r6_second_drive_removed updates sep_rebuild_utils_number_physical_objects_g.
        sep_rebuild_utils_triple_mirror_r6_second_drive_removed(in_rg_config_p,
                                                        &in_rg_config_p->drive_handle[SEP_REBUILD_UTILS_SECOND_REMOVED_DRIVE_POS]);
    
        //  Set up two hot spares.  They should both swap in and the rebuilds should occur on both drives at once.
        //  Save the HS slot number of the first HS because we'll need it later. 
        for(i=0;i<2;i++){
        
            sep_rebuild_utils_setup_hot_spare(in_rg_config_p->extra_disk_set[i].bus,in_rg_config_p->extra_disk_set[i].enclosure ,
                             in_rg_config_p->extra_disk_set[i].slot); 
        
        }

        //  Wait until the rebuilds finish 
        sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);
        sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_SECOND_REMOVED_DRIVE_POS);
    
        //  Reinsert the removed drives
        fbe_test_sep_drive_wait_extra_drives_swap_in(in_rg_config_p,1);   
        // fbe_test_sep_drive_wait_extra_drives_swap_in does not update the count, add 2 for the two new drive.
        sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 2;

        // Refresh the extra drive info  
        fbe_test_sep_util_raid_group_refresh_extra_disk_info(in_rg_config_p,1);

                      
        fbe_test_sep_util_add_removed_position(in_rg_config_p,SEP_REBUILD_UTILS_POSITION_0);

        //  Remove the drive in position 0 in the RG.  This will force the read to occur from position 1 which was
        //  rebuilt.  
        sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
        sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0, sep_rebuild_utils_number_physical_objects_g,
                                     &in_rg_config_p->drive_handle[SEP_REBUILD_UTILS_POSITION_0]);
                                     
        //  Read the data from position 1 and make sure it matches the seed pattern
        sep_rebuild_utils_read_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
    
        //  Get the Hot Spare's bus-enclosure-slot info
        sep_rebuild_utils_get_drive_loc_for_specific_rg_pos(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS,
            &hot_spare_bus, &hot_spare_encl, &hot_spare_slot);
    
        //  Remove the drive in position 1 in the RG.  This will force the read to occur from position 2.  Note
        //  that position 1 has been swapped to a HS, so that is the disk to remove. 
        sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
    
        sep_rebuild_utils_remove_drive_no_check(hot_spare_bus, hot_spare_encl, hot_spare_slot,
        									sep_rebuild_utils_number_physical_objects_g,
                                            &drive_handle);

        //  Read the data from position 2 and make sure it matches the seed pattern
        sep_rebuild_utils_read_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
    
        //  At this point we have verified that 2 disks in the triple mirror were rebuilt properly.  Positions
        //  0 and 1 are removed and position 2 is swapped to a HS.
        // 
        //  Restore the RG by creating two more Hot Spares and letting positions 0 and 1 rebuild 
        for(i=0;i<2;i++){
            
            sep_rebuild_utils_setup_hot_spare(in_rg_config_p->extra_disk_set[i].bus,in_rg_config_p->extra_disk_set[i].enclosure ,
                             in_rg_config_p->extra_disk_set[i].slot); 
            
        }
        
    
        //  At this point we have verified that 2 disks in the triple mirror were rebuilt properly.  Positions
        //  0 and 1 are removed and position 2 is swapped to a HS.
        // 
        sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0);
        sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_1);

        //  Reinsert the removed drives
        fbe_test_sep_drive_wait_extra_drives_swap_in(in_rg_config_p,1);

        //  Reinsert the removed drives
        sep_rebuild_utils_reinsert_removed_drive(hot_spare_bus, hot_spare_encl, hot_spare_slot,&drive_handle);

    }


    //  Return 
    return;

} //  End diego_test_triple_mirror_basic_rebuild()


/*!****************************************************************************
 *  diego_test_triple_mirror_error_cases 
 ******************************************************************************
 *
 * @brief
 *   This function tests error cases for a triple mirror RAID group.  It does 
 *   the following:
 *
 *   - Writes a seed pattern 
 *   - Configures a HS 
 *   - Removes the drive in the secondary position in the RG which swaps to a HS
 *   - Waits so the rebuild of the secondary drive will start
 *   - Removes the drive in the primary position in the RG to cause a rebuild I/O
 *     failure 
 *   - Waits for the rebuild to finish
 *   - Verifies that the data is correct on the secondary drive
 *   - Allows a Hot Spare to swap in for the primary drive 
 *   - Waits for its rebuild to finish
 *   - Verifies that the data is correct on the primary drive
 *
 * @param in_rg_config_p     - pointer to the RG configuration info 
 *
 * @return  None
 * 6/23/2011 - Modified Vishnu Sharma Modified to run on Hardware
 *
 *****************************************************************************/
static void diego_test_triple_mirror_error_cases(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, void * context_p)
{
    fbe_status_t                        status;
    fbe_object_id_t                     raid_group_id;

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    if(fbe_test_rg_config_is_enabled(in_rg_config_p)){

        //  Get the number of physical objects in existence at this point in time.  This number is 
        //  used when checking if a drive has been removed or has been reinserted.   
        status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        //  Write a seed pattern to the RG 
        sep_rebuild_utils_write_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
    
                
        fbe_test_sep_util_add_removed_position(in_rg_config_p,SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);
        
        //  Remove a single drive in the RG (position 1).  Check the object states change correctly and that
        //  rb logging is marked.
        sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS,
                                             &in_rg_config_p->drive_handle[SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS]);
    
        sep_rebuild_utils_setup_hot_spare(in_rg_config_p->extra_disk_set[0].bus,in_rg_config_p->extra_disk_set[0].enclosure ,
                             in_rg_config_p->extra_disk_set[0].slot); 
        
        //  Wait so that the rebuild can start
        sep_rebuild_utils_wait_for_rb_to_start(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);
    
        fbe_test_sep_util_add_removed_position(in_rg_config_p,SEP_REBUILD_UTILS_POSITION_0);

        //  Remove the drive in position 0 in the RG.  This should take place while position 1 is rebuilding. 
        sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
        sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0, sep_rebuild_utils_number_physical_objects_g,
                                     &in_rg_config_p->drive_handle[SEP_REBUILD_UTILS_POSITION_0]);
                                          
        //  Wait for the rebuild of the first drive to finish 
        sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);
    
        //  Read the data from position 1 and make sure it matches the seed pattern.  The read will occur from 
        //  position 1 since position 0 is removed and there is no HS for it. 
        sep_rebuild_utils_read_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
    
        
        sep_rebuild_utils_setup_hot_spare(in_rg_config_p->extra_disk_set[1].bus,in_rg_config_p->extra_disk_set[1].enclosure ,
                             in_rg_config_p->extra_disk_set[1].slot); 
            
        //  Wait until the rebuild of the position 0 finishes 
        sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0);
        sep_rebuild_utils_check_bits(raid_group_id);
    
        //  Read the data and make sure it matches the seed pattern
        sep_rebuild_utils_read_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

        //  Reinsert the removed drives
        fbe_test_sep_drive_wait_extra_drives_swap_in(in_rg_config_p,1);   

    }

    //  Return 
    return;

} //  End diego_test_triple_mirror_error_cases()


/*!****************************************************************************
 *  diego_test_r6_r10_basic_rebuild 
 ******************************************************************************
 *
 * @brief
 *   This function tests basic rebuilds for a RAID-6 or RAID-10 RAID group.  It 
 *   does the following:
 *
 *   - Writes a seed pattern 
 *   - Removes the drive in position 0 in the RG
 *   - Removes the drive in position 2 in the RG
 *   - Configures two Hot Spares, which will swap in for each of the drives
 *   - Waits for the rebuilds of both drives to finish
 *   - Verifies that the data is correct 
 *
 *   At the end of this test, the RG is enabled and 2 positions have been
 *   swapped to permanant spares.
 * 
 * @param in_rg_config_p     - pointer to the RG configuration info 
 *
 * @return  None 
 * 6/23/2011 - Modified Vishnu Sharma Modified to run on Hardware
 *
 *****************************************************************************/
static void diego_test_r6_r10_basic_rebuild(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, void * context_p)
{

    fbe_status_t                        status;
    fbe_u32_t                           i = 0;
    fbe_object_id_t                     raid_group_id;

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    if(fbe_test_rg_config_is_enabled(in_rg_config_p)){
        /* set library debug flags 
            */
        status = fbe_api_raid_group_set_library_debug_flags(raid_group_id, 
                                                        (FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING | 
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING |
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING |
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_TRACING));
        /* set raid group debug flags 
         */
        fbe_test_sep_util_set_rg_debug_flags_both_sps(in_rg_config_p, 
                                                      FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING);
        //  Get the number of physical objects in existence at this point in time.  This number is 
        //  used when checking if a drive has been removed or has been reinserted.   
        status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        //  Write a seed pattern to the RG 
        sep_rebuild_utils_write_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
    
        
        fbe_test_sep_util_add_removed_position(in_rg_config_p,SEP_REBUILD_UTILS_POSITION_1);
        fbe_test_sep_util_add_removed_position(in_rg_config_p,SEP_REBUILD_UTILS_POSITION_2);


        //  Remove a single drive in the RG.  Check the object states change correctly and that rb logging 
        //  is marked.
        sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_1,
                                             &in_rg_config_p->drive_handle[SEP_REBUILD_UTILS_POSITION_1]);
    
        //  Remove a second drive in the RG.  Check that the RG stays ready and the drive is marked rb logging. 
        sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_2,
                                             &in_rg_config_p->drive_handle[SEP_REBUILD_UTILS_POSITION_2]);
    
        //  Set up two hot spares.
        for(i=0;i<2;i++){
            
            sep_rebuild_utils_setup_hot_spare(in_rg_config_p->extra_disk_set[i].bus,in_rg_config_p->extra_disk_set[i].enclosure ,
                             in_rg_config_p->extra_disk_set[i].slot); 
            
        }
        
    
        //  Wait until the rebuilds finish 
        sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_1);
        sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_2);

        sep_rebuild_utils_check_bits(raid_group_id);
    
        //  Read the data and make sure it matches the seed pattern
        sep_rebuild_utils_read_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
    }


    //  Return 
    return;

} //  End diego_test_r6_r10_basic_rebuild()


/*!****************************************************************************
 *  diego_test_r6_error_cases 
 ******************************************************************************
 *
 * @brief
 *   This function tests error cases for a RAID-6 RAID group.  It does the 
 *   following:
 *
 *   - Writes a seed pattern 
 *   - Configures a HS 
 *   - Removes the drive in the position 1 in the RG which swaps to a HS
 *   - Waits so the rebuild of the position 1 will start
 *   - Removes the drive in position 0 in the RG to cause a rebuild I/O failure
 *   - Waits for the rebuild of position 1 to finish
 *   - Verifies that the data is correct 
 *   - Allows a HS to swap in for position 0
 *   - Waits for the rebuild of position 0 to finish
 *   - Verifies that the data is correct 
 *
 * @param in_rg_config_p     - pointer to the RG configuration info 
 *
 * @return  None 
 * 6/23/2011 - Modified Vishnu Sharma Modified to run on Hardware
 *
 *****************************************************************************/
static void diego_test_r6_error_cases(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, void * context_p)
{
    fbe_status_t                        status;
    fbe_object_id_t                     raid_group_id;

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    if(fbe_test_rg_config_is_enabled(in_rg_config_p)){
        /* set library debug flags 
            */
        status = fbe_api_raid_group_set_library_debug_flags(raid_group_id, 
                                                        (FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING | 
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING |
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING |
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_TRACING));
        /* set raid group debug flags 
         */
        fbe_test_sep_util_set_rg_debug_flags_both_sps(in_rg_config_p, 
                                                      FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING);
        //  Get the number of physical objects in existence at this point in time.  This number is 
        //  used when checking if a drive has been removed or has been reinserted.   
        status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        //  Write a seed pattern to the RG 
        sep_rebuild_utils_write_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
    
           
        fbe_test_sep_util_add_removed_position(in_rg_config_p,SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);

        //  Remove a single drive in the RG (position 1).  Check the object states change correctly and that
        //  rb logging is marked.
        sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS,
                                             &in_rg_config_p->drive_handle[SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS]);
    
        sep_rebuild_utils_setup_hot_spare(in_rg_config_p->extra_disk_set[0].bus,in_rg_config_p->extra_disk_set[0].enclosure ,
                             in_rg_config_p->extra_disk_set[0].slot); 
       

        //  Wait so that the rebuild can start
        sep_rebuild_utils_wait_for_rb_to_start(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);
    
        fbe_test_sep_util_add_removed_position(in_rg_config_p,SEP_REBUILD_UTILS_POSITION_0);
        
        //  Remove the drive in position 0 in the RG.  This should take place while position 1 is rebuilding. 
        sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
        sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0, sep_rebuild_utils_number_physical_objects_g,
                                     &in_rg_config_p->drive_handle[SEP_REBUILD_UTILS_POSITION_0]);
                                          
        //  Wait for the rebuild of the first drive to finish 
        sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);
    
        //  Read the data and make sure it matches the seed pattern
        sep_rebuild_utils_read_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
    
           
        sep_rebuild_utils_setup_hot_spare(in_rg_config_p->extra_disk_set[1].bus,in_rg_config_p->extra_disk_set[1].enclosure ,
                             in_rg_config_p->extra_disk_set[1].slot); 
        
    
        //  Wait until the rebuild of the position 0 finishes 
        sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0);
    
        sep_rebuild_utils_check_bits(raid_group_id);

        //  Read the data and make sure it matches the seed pattern
        sep_rebuild_utils_read_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

        //  Reinsert the removed drives
        fbe_test_sep_drive_wait_extra_drives_swap_in(in_rg_config_p,1); 

    }

    //  Return 
    return;

} //  End diego_test_r6_error_cases()

/*!****************************************************************************
 *  diego_test_basic_dual_sp_rebuild 
 ******************************************************************************
 *
 * @brief
 *  This function runs basic rebuild on dual-SP using Raid 5. It does the 
 *  following:
 *    
 *   - Bring the peer SP up.
 *   - Remove a single drive in the RG on local SP.
 *   - Remove a single drive in the RG on the peer SP.
 *   - Writes a seed pattern on local SP.
 *   - Configures a Hot Spare on local SP.
 *   - The Hot Spare will swap in for the removed drive on local SP. 
 *   - Waits for the rebuild to finish on local SP.
 *   - Verifies that the data is correct on local SP. 
 *   - Destroy peer SP config.
 *   - Set target server back to local SP and let test finish normally.
 * 
 * @param in_rg_config_p    - incoming RG               
 * @param iterations        - max number of quiesce/unquiesce iterations.               
 *
 * @return None
 * 
 * @author
 *   2/23/2011 - Created. Christina Chiang
 *****************************************************************************/
static void diego_test_basic_dual_sp_rebuild(fbe_test_rg_configuration_t*  in_rg_config_p)
{
    fbe_sim_transport_connection_target_t   local_sp;
    fbe_sim_transport_connection_target_t   peer_sp;
    fbe_api_terminator_device_handle_t      drive_info_1;       //  drive info needed for reinsertion  
    fbe_api_terminator_device_handle_t      peer_drive_info;    //  drive info needed for reinsertion 
    fbe_u32_t                               total_objects = 0;
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t   sep_rebuild_utils_number_physical_objects_g_spa;
    fbe_object_id_t                         raid_group_id;

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    //  Get the ID of the local SP
    local_sp = fbe_api_sim_transport_get_target_server();

    //  Get the ID of the peer SP
    if (FBE_SIM_SP_A == local_sp)
    {
        peer_sp = FBE_SIM_SP_B;
    }
    else
    {
        peer_sp = FBE_SIM_SP_A;
    }

    //  Set the target server to the peer SP
    fbe_api_sim_transport_set_target_server(peer_sp);

    //  Load the physical configuration and packages on the peer SP
    mut_printf(MUT_LOG_TEST_STATUS, "%s: setup test configuration on peer SP %d",  __FUNCTION__, peer_sp);
    sep_rebuild_utils_setup();

    //  Set the target server back to local SP
    fbe_api_sim_transport_set_target_server(local_sp);

    //  Remove a single drive in the RG.  Check the object states change correctly and that
    //  rb logging is marked.
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);
   
    //  Set target server to peer for cleanup there
    fbe_api_sim_transport_set_target_server(peer_sp);
    
    // !@TODO: We should have a sep_rebuild_utils_number_physical_objects_g for peer SP
    // Save sep_rebuild_utils_number_physical_objects_g for local SP.
    sep_rebuild_utils_number_physical_objects_g_spa = sep_rebuild_utils_number_physical_objects_g;
    
    // Get the sep_rebuild_utils_number_physical_objects_g for peer SP
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    sep_rebuild_utils_number_physical_objects_g = total_objects;
    
    //  Remove a single drive in the RG on the peer.  Check the object states change correctly and that
    //  rb logging is marked.
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Removing first drive in the RG on PEER. Total objects %d\n", __FUNCTION__, total_objects);
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &peer_drive_info);

    //  Set the target server back to local SP
    fbe_api_sim_transport_set_target_server(local_sp);
    
    // Restore sep_rebuild_utils_number_physical_objects_g for local SP
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g_spa;
    
    //  Write a seed pattern to the RG (Degraded Write)
    // !@TODO: This write should be earlier when the dual-SP infrastructure is working better.  
    sep_rebuild_utils_write_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
    
    //  Set up a hot spare and hit the hot spare limition (over 15 hot spares) on SEP_REBUILD_UTILS_HOT_SPARE_ENCLOSURE.
    //  !@TODO: We should support more than 15 hot spares. 
    //  Use 0_2_3 as suggesting by Jean. 
    sep_rebuild_utils_setup_hot_spare(SEP_REBUILD_UTILS_PORT, SEP_REBUILD_UTILS_ENCLOSURE_THIRD, SEP_REBUILD_UTILS_HS_SLOT_0_2_3);
    
    //  Wait until the rebuild finishes.  We need to pass the disk position of the disk which is rebuilding.  
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);

    sep_rebuild_utils_check_bits(raid_group_id);
   
    //  Read the data and make sure it matches the seed pattern
    sep_rebuild_utils_read_bg_pattern(&fbe_diego_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
    
    //  Set target server to peer for cleanup there
    fbe_api_sim_transport_set_target_server(peer_sp);

    //  Destroy peer SP config now
    mut_printf(MUT_LOG_TEST_STATUS, "%s: destroy config on SP %d",  __FUNCTION__, peer_sp);
    fbe_test_sep_util_destroy_neit_sep_physical();

    //  Set target server back to first SP and let test finish normally
    fbe_api_sim_transport_set_target_server(local_sp);
  
    return;

} //  End diego_test_basic_dual_sp_rebuild()



/*!****************************************************************************
 *  diego_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the cleanup function for the Diego test. 
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void diego_cleanup(void)
{
    if (fbe_test_util_is_simulation())
         fbe_test_sep_util_destroy_neit_sep_physical();
    return;

}

/*!**************************************************************
 * diego_test_private_space_rgs_test_case_1()
 ****************************************************************
 * @brief
 *  This test runs test case for system raid groups with steps 
 *   STEP 1: Randomly remove a drive between 0_0_0 and 0_0_3. 
 *           Observe that all 3 raid groups are not broken, and
 *           rebuild logging is set in that position.
 *   STEP 2: Randomly remove another drive between 0_0_0 and 0_0_3, 
 *           Observe that PSM (triple mirror) RG is not broken, and
 *           rebuild logging is set in that position. 
 *           Observe the other 2 RGs are broken.
 *   STEP 3: Re-insert the second removed drive. 
 *           Observe all 3 raid groups are in ready state.
 *           Observe triple mirror raid group starts rebuilding
 *           in that position.
 *   STEP 4: Re-insert the first removed drive. 
 *           Observe all 3 raid groups are in ready state.
 *           Observe other 2 raid groups starts rebuilding.
 *
 * @param None              
 *
 * @return None.
 *
 * @author
 *  03/05/2012 - Created. chenl6
 *
 ****************************************************************/
static void diego_test_private_space_rgs_test_case_1(void)
{
    fbe_api_terminator_device_handle_t  drive_info_1, drive_info_2;
    fbe_object_id_t                     system_raid_group_id;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t							first_removed_slot, second_removed_slot;
    fbe_u32_t 							total_objects=0;

    diego_test_config_db_rg_disk_set(&diego_vault_drives_rg_config_g[0],&fbe_diego_vault_drives_disk_set_g[0]);

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s ===\n", __FUNCTION__);
    //	Verify the system RGs and private LUNs
    diego_test_get_private_space_rgs();
    diego_test_get_private_space_luns();	
    // Get the total number of objects in the system
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);	

    // Get 2 slots that will be removed
    first_removed_slot = fbe_random() % DIEGO_VAULT_DRIVES_RAID_GROUP_WIDTH;
    do {
        second_removed_slot = fbe_random() % DIEGO_VAULT_DRIVES_RAID_GROUP_WIDTH;
    } while (first_removed_slot == second_removed_slot);

    // Remove first slot
    mut_printf(MUT_LOG_TEST_STATUS, "=== Removing first drive 0_0_%d\n", first_removed_slot);
    //	Remove a single drive in the RG.  Check the PVD and VD change statues correctly 
    sep_rebuild_utils_remove_drive_and_verify(&diego_vault_drives_rg_config_g[0],first_removed_slot,total_objects-1,&drive_info_1);	

    // Get the id of the system raid group
    system_raid_group_id=FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR;	
    if (first_removed_slot < DIEGO_PSM_DRIVES_RAID_GROUP_WIDTH)
    {
        //  For the system RG, verify that rb logging is set
        sep_rebuild_utils_verify_rb_logging_by_object_id(system_raid_group_id, first_removed_slot);
    }
    //  Verify the RG is not broken
    sep_rebuild_utils_verify_sep_object_state(system_raid_group_id, FBE_LIFECYCLE_STATE_READY);

    system_raid_group_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG;
    //  For the system RG, verify that rb logging is set
    sep_rebuild_utils_verify_rb_logging_by_object_id(system_raid_group_id, first_removed_slot);
    //  Verify the RG is not broken
    sep_rebuild_utils_verify_sep_object_state(system_raid_group_id, FBE_LIFECYCLE_STATE_READY);

    system_raid_group_id=FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAW_MIRROR_RG;	
    if (first_removed_slot < DIEGO_PSM_DRIVES_RAID_GROUP_WIDTH)
    {
        //  For the system RG, verify that rb logging is set
        sep_rebuild_utils_verify_rb_logging_by_object_id(system_raid_group_id, first_removed_slot);
    }
    //  Verify the RG is not broken
    sep_rebuild_utils_verify_sep_object_state(system_raid_group_id, FBE_LIFECYCLE_STATE_READY);

    system_raid_group_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG;
    //  For the system RG, verify that rb logging is set
    sep_rebuild_utils_verify_rb_logging_by_object_id(system_raid_group_id, first_removed_slot);
    //  Verify the RG is not broken
    sep_rebuild_utils_verify_sep_object_state(system_raid_group_id, FBE_LIFECYCLE_STATE_READY);

    // Remove second slot
    mut_printf(MUT_LOG_TEST_STATUS, "=== Removing second drive 0_0_%d\n", second_removed_slot);
    sep_rebuild_utils_remove_drive_and_verify(&diego_vault_drives_rg_config_g[0],second_removed_slot,total_objects-2,&drive_info_2);	

    // Get the id of the system raid group
    system_raid_group_id=FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR;	
    if (second_removed_slot < DIEGO_PSM_DRIVES_RAID_GROUP_WIDTH)
    {
        //  For the system RG, verify that rb logging is set
        sep_rebuild_utils_verify_rb_logging_by_object_id(system_raid_group_id, second_removed_slot);
    }

    //  Verify the Raw Mirror RG is not broken
    sep_rebuild_utils_verify_sep_object_state(system_raid_group_id, FBE_LIFECYCLE_STATE_READY);
    system_raid_group_id=FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAW_MIRROR_RG;	
    if (second_removed_slot < DIEGO_PSM_DRIVES_RAID_GROUP_WIDTH)
    {
        //  For the system RG, verify that rb logging is set
        sep_rebuild_utils_verify_rb_logging_by_object_id(system_raid_group_id, second_removed_slot);
    }
    //  Verify the RG is not broken
    sep_rebuild_utils_verify_sep_object_state(system_raid_group_id, FBE_LIFECYCLE_STATE_READY);

    system_raid_group_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG;
    //  Verify the RG is broken
    sep_rebuild_utils_verify_sep_object_state(system_raid_group_id, FBE_LIFECYCLE_STATE_FAIL);

    system_raid_group_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG; 
    //  Verify the RG is broken
    sep_rebuild_utils_verify_sep_object_state(system_raid_group_id, FBE_LIFECYCLE_STATE_FAIL);

    //  Re-insert the second removed drive
    mut_printf(MUT_LOG_TEST_STATUS, "=== Re-inserting second drive 0_0_%d\n", second_removed_slot);
    status = fbe_test_pp_util_reinsert_drive(0, 0, second_removed_slot, drive_info_2);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    //  Verify all 3 RGs in ready state
    for(system_raid_group_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR;
        system_raid_group_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG; 
        system_raid_group_id++)
    {
        //  Verify the RG is ready
        sep_rebuild_utils_verify_sep_object_state(system_raid_group_id, FBE_LIFECYCLE_STATE_READY);
    }

    //  Rebuild started on the triple mirrir RG
    if (second_removed_slot < DIEGO_PSM_DRIVES_RAID_GROUP_WIDTH)
    {
        system_raid_group_id=FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR;	
        sep_rebuild_utils_wait_for_rb_to_start_by_obj_id(system_raid_group_id, second_removed_slot);
    }

    //  Re-insert the first removed drive
    mut_printf(MUT_LOG_TEST_STATUS, "=== Re-inserting first drive 0_0_%d\n", first_removed_slot);
    status = fbe_test_pp_util_reinsert_drive(0, 0, first_removed_slot, drive_info_1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    //  Verify all 3 RGs in ready state
    for(system_raid_group_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR;
        system_raid_group_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG; 
        system_raid_group_id++)
    {
        //  Verify the RG is ready
        sep_rebuild_utils_verify_sep_object_state(system_raid_group_id, FBE_LIFECYCLE_STATE_READY);
    }
    //  Rebuild started on 2 R5 RGs
    for(system_raid_group_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG;
        system_raid_group_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG; 
        system_raid_group_id++)
    {
        if (system_raid_group_id != FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAW_MIRROR_RG)
        {
            sep_rebuild_utils_wait_for_rb_to_start_by_obj_id(system_raid_group_id, first_removed_slot);
        }
    }

    return;
} //  End diego_test_private_space_rgs_test_case_1()

/*!**************************************************************
 * diego_test_private_space_rgs_test_case_2()
 ****************************************************************
 * @brief
 *  This test runs test case for system raid groups with steps 
 *  This test runs test case for system raid groups with steps 
 *   STEP 1: randomly remove a drive between 0_0_0 and 0_0_3, 
 *           observe that all 3 raid groups are not broken, and
 *           rebuild logging is set in that position.
 *   STEP 2: re-insert the drive, allow vault raid group (object 
 *           id 16) to finish rebuild. Hold other 2 RGs in debug
 *           hooks.
 *   STEP 3: Randomly remove another drive between 0_0_0 and 0_0_3. 
 *           Observe that PSM (triple mirror) RG is not broken, and
 *           rebuild logging is set in that position, rebuilding
 *           is processing on the re-inserted drive. 
 *           Observe that vault RG (object id 16) is not broken,and 
 *           rebuild logging is set in that position. 
 *           Observe that other RG (object id 15) is broken. 
 *   STEP 4: Re-insert the second removed drive. 
 *           Observe all 3 raid groups are in ready state.
 *           Observe R3 raid groups starts rebuilding.
 *
 * @param None              
 *
 * @return None.
 *
 * @author
 *  03/05/2012 - Created. chenl6
 *
 ****************************************************************/
static void diego_test_private_space_rgs_test_case_2(void)
{
    fbe_api_terminator_device_handle_t  drive_info_1, drive_info_2 = 0;
    fbe_object_id_t                     system_raid_group_id;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t							first_removed_slot, second_removed_slot;
    fbe_u32_t 							total_objects=0;
    fbe_api_raid_group_get_info_t       rg_info;

    diego_test_config_db_rg_disk_set(&diego_vault_drives_rg_config_g[0],&fbe_diego_vault_drives_disk_set_g[0]);

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s ===\n", __FUNCTION__);
    //	Verify the system RGs and private LUNs
    diego_test_get_private_space_rgs();
    diego_test_get_private_space_luns();	
    // Get the total number of objects in the system
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);	

    // Get 2 slots that will be removed
    first_removed_slot = fbe_random() % DIEGO_VAULT_DRIVES_RAID_GROUP_WIDTH;
    do {
        second_removed_slot = fbe_random() % DIEGO_VAULT_DRIVES_RAID_GROUP_WIDTH;
    } while (first_removed_slot == second_removed_slot);

    // Remove first slot
    mut_printf(MUT_LOG_TEST_STATUS, "=== Removing first drive 0_0_%d\n", first_removed_slot);
    sep_rebuild_utils_remove_drive_and_verify(&diego_vault_drives_rg_config_g[0],first_removed_slot,total_objects-1,&drive_info_1);	

    for(system_raid_group_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR;
        system_raid_group_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG; 
        system_raid_group_id++)
    {
        //  For the system RG, get rg info
        status = fbe_api_raid_group_get_info(system_raid_group_id, &rg_info,
                                         FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        //  For the system RG, verify that rb logging is set
        if (first_removed_slot < rg_info.width)
        {
            sep_rebuild_utils_verify_rb_logging_by_object_id(system_raid_group_id, first_removed_slot);
        }
        //  Verify the RG is not broken
        sep_rebuild_utils_verify_sep_object_state(system_raid_group_id, FBE_LIFECYCLE_STATE_READY);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== Adding Debug Hook...");
    status = fbe_api_scheduler_add_debug_hook(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG,
									SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
									0x800,
									NULL,
									SCHEDULER_CHECK_VALS_LT,
									SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Adding Debug Hook...");
    status = fbe_api_scheduler_add_debug_hook(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
									SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
									0x800,
									NULL,
									SCHEDULER_CHECK_VALS_LT,
									SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Re-insert the removed drive
    mut_printf(MUT_LOG_TEST_STATUS, "=== Re-inserting first drive 0_0_%d\n", first_removed_slot);
    status = fbe_test_pp_util_reinsert_drive(0, 0, first_removed_slot, drive_info_1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // Wait for vault RG finishes rebuild
    system_raid_group_id=FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG;
    sep_rebuild_utils_wait_for_rb_comp_by_obj_id(system_raid_group_id, first_removed_slot);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting Rebuild Debug Hook");
    status = fbe_api_scheduler_del_debug_hook(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                              0x800,
                                              NULL,
                                              SCHEDULER_CHECK_VALS_LT,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting Rebuild Debug Hook");
    status = fbe_api_scheduler_del_debug_hook(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                              0x800,
                                              NULL,
                                              SCHEDULER_CHECK_VALS_LT,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);

    // Remove second slot
    mut_printf(MUT_LOG_TEST_STATUS, "=== Removing second drive 0_0_%d\n", second_removed_slot);
    sep_rebuild_utils_remove_drive_and_verify(&diego_vault_drives_rg_config_g[0],second_removed_slot,total_objects-1,&drive_info_2);	

    // Get the id of the system raid group
    system_raid_group_id=FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR;	
    if (second_removed_slot < DIEGO_PSM_DRIVES_RAID_GROUP_WIDTH)
    {
        //  Verify that rb logging is set for the second removed slot
        sep_rebuild_utils_verify_rb_logging_by_object_id(system_raid_group_id, second_removed_slot);
    }
    if (first_removed_slot < DIEGO_PSM_DRIVES_RAID_GROUP_WIDTH)
    {
        //  Verify that rebuild is started for the first removed slot
        sep_rebuild_utils_wait_for_rb_to_start_by_obj_id(system_raid_group_id, first_removed_slot);
    }
    //  Verify the RG is not broken
    sep_rebuild_utils_verify_sep_object_state(system_raid_group_id, FBE_LIFECYCLE_STATE_READY);

    //  For the vault RG, verify that rb logging is set
    system_raid_group_id=FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG;	
    sep_rebuild_utils_verify_rb_logging_by_object_id(system_raid_group_id, second_removed_slot);
    sep_rebuild_utils_verify_sep_object_state(system_raid_group_id, FBE_LIFECYCLE_STATE_READY);

    //  Verify the RG is broken
    system_raid_group_id=FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG;	
    sep_rebuild_utils_verify_sep_object_state(system_raid_group_id, FBE_LIFECYCLE_STATE_FAIL);

    //  Re-insert the second removed drive
    mut_printf(MUT_LOG_TEST_STATUS, "=== Re-inserting second drive 0_0_%d\n", second_removed_slot);
    status = fbe_test_pp_util_reinsert_drive(0, 0, second_removed_slot, drive_info_2);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    //  Verify all 3 RGs in ready state
    for(system_raid_group_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR;
        system_raid_group_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG; 
        system_raid_group_id++)
    {
        //  Verify the RG is ready
        sep_rebuild_utils_verify_sep_object_state(system_raid_group_id, FBE_LIFECYCLE_STATE_READY);
    }
    //  Rebuild started on 2 R3 RGs
    system_raid_group_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG;
    sep_rebuild_utils_wait_for_rb_to_start_by_obj_id(system_raid_group_id, first_removed_slot);
    system_raid_group_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG;
    sep_rebuild_utils_wait_for_rb_to_start_by_obj_id(system_raid_group_id, second_removed_slot);

    return;
} //  End diego_test_private_space_rgs_test_case_2()

/*!**************************************************************
 * diego_test_private_space_rgs()
 ****************************************************************
 * @brief
 *  This function tests for failure of PSM/vault drive where some 
 *  RG are rebuild logging.  
 *  This test will randomly run test case 1 or test case 2 for 
 *  following system raid groups:
 *  FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR (13) 
 *  FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG (14)
 *  FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAW_MIRROR_RG (15)
 *  FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG (16)
 *
 * @param None              
 *
 * @return None.
 *
 * @author
 *  03/05/2012 - Created. chenl6
 *
 ****************************************************************/
static void diego_test_private_space_rgs(void)
{
    diego_system_raid_group_test_index_t test_index = fbe_random()%DIEGO_SYSTEM_RAID_GROUP_NUM_TEST_CASES;

    // Only support it in simulation for the time being.
    if (!fbe_test_util_is_simulation())
    {
        return;
    }

    switch(test_index)
    {
        case DIEGO_SYSTEM_RAID_GROUP_TEST_CASE_1:
            diego_test_private_space_rgs_test_case_1();
            break;

        case DIEGO_SYSTEM_RAID_GROUP_TEST_CASE_2:
            diego_test_private_space_rgs_test_case_2();
            break;

        default:
            mut_printf(MUT_LOG_LOW, "=== DIEGO system raid group Invalid test index: %d===\n", test_index);
            break;
    }

    return;
} //  End diego_test_private_space_rgs()
