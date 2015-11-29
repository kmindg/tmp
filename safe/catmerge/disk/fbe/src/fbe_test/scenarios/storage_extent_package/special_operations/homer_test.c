/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * homer_test.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains test code for homer test which handles
 *  get raid info IOCTL
 *
 * HISTORY
 *   11/17/2009:  Created. Ashok Tamilarasan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe_test_common_utils.h"

char * homer_short_desc = "This scenario will verify FLARE_RAID_INFO IOCTL processing";
char * homer_long_desc = 
    "\n"
    "\t This tests validates the ability of the SEP to accept the \n"
    "\t IOCTL_FLARE_GET_RAID_INFO IOCTL and forwards them to the \n"
    "\t appropriate LUN and RAID objects and returns the required \n"
    "\t information back to the upper level drivers \n"  
    "\n"
    "Dependencies:\n"
    "\t - Ability to create multiple RAID and LUN objects.\n"
    "\n"
    "Starting Config:\n"
    "\t [PP] armada board\n"
    "\t [PP] SAS PMC port\n"
    "\t [PP] viper enclosure\n"
    "\t [PP] 7 SAS drives\n"
    "\t [PP] 7 logical drives\n"
    "\t [SEP] 7 provisioned drives\n"
    "\t [SEP] 7 virtual drives\n"
    "\t [SEP] 2 RAID Objects  (Raid 5 and Raid 1)\n"
    "\t [SEP] 4 LUN Objects (2 for each Raid Object)\n"
    "\n"
    "Test Sequence:\n"
    "\tSTEP 1: Create the initial topology.\n"
    "\t\t - Create and verify the initial physical package config\n"
    "\t\t - Create provisioned drives and attach edges to logical drives\n"
    "\t\t - Create virtual drives and attach edges to provisioned drives\n"
    "\t\t - Create a 5 drive raid-5 raid group object and attach edges \n"
    "\t\t   from raid 5 to virtual drives. \n"
    "\t\t - Create and attach 2 LUNs to the raid 5 object. \n"
    "\t\t - Create a 2 drive raid-1 raid group object and attach edges \n"
    "\t\t   from raid 1 to virtual drives. \n"
    "\t\t - Create and attach 2 LUNs to the raid 1 object. \n"

    "\n"
    "\tSTEP 2: Connect to the BVD/Upper Shim \n"
    "\t\t - Veryify that the test code can attach to the BVD object \n"
    "\t\t - Obtain the list of LUNs and validate if all the LUNs can be \n"
    "\t\t   seen \n"
    "\n"
    "\tSTEP 3: Pass the IOCTL and get the required information.\n"
    "\t\t - For each LUN issue IOCTL_FLARE_GET_RAID_INFO IOCTL \n"
    "\t\t   information \n"
    "\t\t - Verify that the LUN/RG objects returns the required \n"
    "\t\t   information(e.g. WWN, RG Type, Bind Time etc.,) \n"
	"Last update 12/15/11\n"
	;

/*!*******************************************************************
 * @def HOMER_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of physical package objects we will create.
 *        (1 board + 1 port + 1 encl + 15 pdo) 
 *
 *********************************************************************/
#define HOMER_TEST_NUMBER_OF_PHYSICAL_OBJECTS 28
/* The number of drives in our raid group.
 */
#define HOMER_TEST_RAID_GROUP_WIDTH                   5 

/* The number of LUNs in our raid group.
 */
#define HOMER_TEST_LUNS_PER_RAID_GROUP                3 

/* Element size of our LUNs.
 */
#define HOMER_TEST_ELEMENT_SIZE                       128

/* Maximum number of block we will test with.
 */
#define HOMER_MAX_IO_SIZE_BLOCKS                      1024

/* Max number of raid groups we will test with.
 */
#define HOMER_TEST_RAID_GROUP_COUNT             2

/* Max number of drives we will test with.
 */
#define HOMER_TEST_MAX_WIDTH               16

/* The number of blocks in a raid group bitmap chunk.
 */
#define HOMER_TEST_RAID_GROUP_CHUNK_SIZE  (8 * FBE_RAID_SECTORS_PER_ELEMENT)


#define HOMER_TEST_CHUNKS_PER_LUN            3


/*!*******************************************************************
 * @var homer_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t homer_raid_group_config[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,     520,            0,          0},
    {5,       0xF000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,      520,            1,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static void homer_set_trace_level(fbe_trace_type_t trace_type, 
                                                 fbe_u32_t id, 
                                                 fbe_trace_level_t level);

static void homer_test_validate_lun_info(fbe_database_lun_info_t *lun_info,
                                         fbe_raid_group_type_t type,
                                         fbe_u32_t width,
                                         fbe_block_count_t element_size);
static void homer_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);
/*!****************************************************************************
 * homer_test()
 ******************************************************************************
 * @brief
 *  Run drive sparing test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  25/07/2011 - Created. Vishnu Sharma
 ******************************************************************************/
void homer_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &homer_raid_group_config[0];
    
    fbe_test_run_test_on_rg_config(rg_config_p,NULL,homer_run_tests,
                                   HOMER_TEST_LUNS_PER_RAID_GROUP,
                                   HOMER_TEST_CHUNKS_PER_LUN);
    return;    

}

static void homer_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_status_t status;
    fbe_database_lun_info_t lun_info;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t * current_rg_configuration_p = NULL;
    fbe_u32_t num_raid_groups = 0;
   

    homer_set_trace_level (FBE_TRACE_TYPE_DEFAULT,
                           FBE_OBJECT_ID_INVALID,
                           FBE_TRACE_LEVEL_INFO);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Start ...\n", __FUNCTION__);

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);

    for(rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        current_rg_configuration_p = rg_config_ptr + rg_index;

        if(fbe_test_rg_config_is_enabled(current_rg_configuration_p))
        {

            fbe_api_database_lookup_lun_by_number(current_rg_configuration_p->logical_unit_configuration_list[0].lun_number,
                                         &lun_object_id);

            lun_info.lun_object_id = lun_object_id;
            
            status = fbe_api_database_get_lun_info(&lun_info);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            mut_printf(MUT_LOG_TEST_STATUS, "%s:\n","Lun validation Started");
            homer_test_validate_lun_info(&lun_info, 
                                         current_rg_configuration_p->raid_type,
                                         current_rg_configuration_p->width, 
                                         HOMER_TEST_ELEMENT_SIZE);
            mut_printf(MUT_LOG_TEST_STATUS, "%s:\n","Lun validation Ends");
        }
    }
    return;
}

static void homer_test_validate_lun_info(fbe_database_lun_info_t *lun_info,
                                         fbe_raid_group_type_t type,
                                         fbe_u32_t width,
                                         fbe_block_count_t element_size)
{
    MUT_ASSERT_INT_EQUAL_MSG(lun_info->raid_info.raid_type, type, "Raid group type validation failed");
    MUT_ASSERT_UINT64_EQUAL_MSG(lun_info->raid_info.element_size, element_size, "Element Size validation failed");
    MUT_ASSERT_INT_EQUAL_MSG(lun_info->raid_info.width, width, "Width validation failed");
}

void homer_setup(void)
{
    

    if (fbe_test_util_is_simulation())
    {
        
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        rg_config_p = &homer_raid_group_config[0];
       
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();

        /* Set the trace level to info. */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
        
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return;
}

void homer_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

static void 
homer_set_trace_level(fbe_trace_type_t trace_type,
                                     fbe_u32_t id,
                                     fbe_trace_level_t level)
{
    fbe_api_trace_level_control_t level_control;
    fbe_status_t status;

    level_control.trace_type = trace_type;
    level_control.fbe_id = id;
    level_control.trace_level = level;
    status = fbe_api_trace_set_level(&level_control, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}

/*************************
 * end file homer_test.c
 *************************/
    
