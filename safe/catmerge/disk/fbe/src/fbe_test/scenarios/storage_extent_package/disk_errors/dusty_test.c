/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file dusty_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test cases to validate event log messages 
 *  related to corrupt CRC and corrupt Data operation/
 *
 * @version
 *   02/15/2011:  Created - Jyoti Ranjan
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
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_raid_error.h"
#include "fbe_test_common_utils.h"





 
/*************************
 *   TEST DESCRIPTION
 *************************/
char * dusty_test_short_desc = "validates event log messages because of coherency errors";
char * dusty_test_long_desc = 
"dusty_test injects error on a given raid type and issues specific rdgen I/O \n"
"to provoke occurence of error, which also causes RAID library to report event log messages. \n"
"This scenario validates correctness of reported even-log messages on detection of coherency errors \n"
"\n"
"Starting Config:  \n"
"        [PP] armada board \n"
"        [PP] SAS PMC port \n"
"        [PP] viper enclosure \n"
"        [PP] 3 SAS drives (PDO-[0..2]) \n"
"        [PP] 3 logical drives (LD-[0..2]) \n"
"        [SEP] 3 provision drives (PVD-[0..2]) \n"
"        [SEP] 3 virtual drives (VD-[0..2]) \n"
"        [SEP] raid object (RAID5 2+1) \n"
"        [SEP] LUN object (LUN-0) \n"
"\n"
"STEP 1: Create raid group configuration\n"
"STEP 2: Inject error using logical error injection service\n"
"STEP 3: Issue rdgen I/O\n"
"STEP 4: Validate the expected event log message correctness against a pre-defined expected parameters\n"
"STEP 5: Validate rdgen I/O statistics as per predefined values\n";

/************************
 * LITERAL DECLARARION
 ***********************/

/*!*******************************************************************
 * @def DUSTY_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define DUSTY_TEST_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def DUSTY_TEST_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define DUSTY_TEST_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def DUSTY_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with.
 *
 *********************************************************************/
#define DUSTY_TEST_RAID_GROUP_COUNT 15

/*!*******************************************************************
 * @def DUSTY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR
 *********************************************************************
 * @brief Size of a region when mining for a hard error.
 *
 *********************************************************************/
#define DUSTY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR 0x40




/*!*******************************************************************
 * @def DUSTY_TEST_INVALID_FRU_POSITION
 *********************************************************************
 * @brief Invalid disk position. Used in referring a disk position
 *        wherever we do not expect a valid fru position.
 *
 *********************************************************************/
#define DUSTY_TEST_INVALID_FRU_POSITION ((fbe_u32_t) -1)


/*!*******************************************************************
 * @def DUSTY_TEST_INVALID_BITMASK
 *********************************************************************
 * @brief Invalid bitmask. 
 *
 *********************************************************************/
#define DUSTY_TEST_INVALID_BITMASK (0x0)




/**************************
 * FUNCTION DECLARATION(s)
 *************************/
void dusty_test(void);
void dusty_test_setup(void);
void dusty_cleanup(void);
fbe_status_t dusty_validate_rdgen_result(fbe_status_t rdgen_request_status,
                                          fbe_rdgen_operation_status_t rdgen_op_status,
                                          fbe_rdgen_statistics_t *rdgen_stats_p);




/*!*******************************************************************
 * @var b_packages_loaded
 *********************************************************************
 * @brief set to TRUE if the packages have been loaded.
 *
 *********************************************************************/
static fbe_bool_t CSX_MAYBE_UNUSED b_packages_loaded = FBE_FALSE;


/*!*******************************************************************
 * @var dusty_validate_user_io_result_fn_p
 *********************************************************************
 * @brief The function is used to validate I/O statistics after 
 *        user I/O oepration
 *
 *********************************************************************/
fbe_test_event_log_test_validate_rdgen_result_t dusty_validate_user_io_result_fn_p = NULL;



/*******************
 * GLOBAL VARIABLES
 ******************/


/*!*******************************************************************
 * @var dusty_config_array_p
 *********************************************************************
 * @brief Pointer to configuration used to run test cases. It is 
 *        initialized at the time of setup.
 *
 *********************************************************************/
static fbe_test_event_log_test_configuration_t * CSX_MAYBE_UNUSED dusty_config_array_p = NULL;








/*!*******************************************************************
 * @var dusty_raid_group_config_extended
 *********************************************************************
 * @brief This is the raid group configuration we will use to 
 *        valdiate the correctness of event log messages.for 
 *        extended version
 *********************************************************************/
static fbe_test_event_log_test_configuration_t dusty_raid_group_config_extended_qual[] = 
{ 
    /* RAID 3 and RAID 5 configuration (non-degraded) */
    {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,       12,          0},
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,       20,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            NULL,
        }
    },


   /* RAID 6 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {8,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,    FBE_CLASS_ID_PARITY,    520,      6,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            NULL,
        }
   },

   /* RAID 1 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {2,       0x8000,   FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,           8,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            NULL,
        }
   },

   /* RAID 10 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,            block size  RAID-id.  bandwidth.*/
            {6,       0xE000,   FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,        10,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {   
            NULL,       
        }
   },

   /* RAID 0 configuration */
   {
        {
            /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {8,         0x32000,  FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,        0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            NULL,
        }
   },

   { 
        {   
            { FBE_U32_MAX, /* Terminator. */},
        }
   }
};


/*!*******************************************************************
 * @var dusty_raid_group_config_qual
 *********************************************************************
 * @brief This is the raid group configuration we will use to 
 *        valdiate the correctness of event log messages.for 
 *        qual version
 *********************************************************************/
static fbe_test_event_log_test_configuration_t dusty_raid_group_config_qual[] = 
{
    /* RAID 3 and RAID 5 configuration (non-degraded) */
    {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,       12,          0},
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,       20,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            NULL,
        }
    },


   /* RAID 6 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {8,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,    FBE_CLASS_ID_PARITY,    520,      6,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            NULL,
        }
   },

   /* RAID 1 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {2,       0x8000,   FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,           8,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            NULL,
        }
   },

   /* RAID 10 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,            block size  RAID-id.  bandwidth.*/
            {6,       0xE000,   FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,        10,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {   
            NULL,       
        }
   },

   /* RAID 0 configuration */
   {
        {
            /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {8,         0x32000,  FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,        0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            NULL,
        }
   },

   { 
        {   
            { FBE_U32_MAX, /* Terminator. */},
        }
   }
};




/*************************
 * FUNCTION DEFINITION(s)
 *************************/


/*!**************************************************************
 * dusty_validate_rdgen_result()
 ****************************************************************
 * @brief
 * This function validates the IO statistics of the generated IO.
 *
 * @param rdgen_request_status - status of reqquest sent using rdgen
 * @param rdgen_op_status      - rdgen operation status
 * @param rdge_stats_p         - pointer to rdgen I/O statistics
 *
 * @return None.
 *
 * @author
 *  12/15/2010 - Created - Jyoti Ranjan
 *
 ****************************************************************/
fbe_status_t dusty_validate_rdgen_result(fbe_status_t rdgen_request_status,
                                              fbe_rdgen_operation_status_t rdgen_op_status,
                                              fbe_rdgen_statistics_t *rdgen_stats_p)
{
    MUT_ASSERT_INT_EQUAL(rdgen_stats_p->aborted_error_count, 0);
    return FBE_STATUS_OK;
}
/**********************************************
 * end dusty_validate_io_statistics()
 **********************************************/





/*!**************************************************************
 * dusty_test()
 ****************************************************************
 * @brief
 *  Run a event logging for correctable errors test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  12/15/2010 - Created - Jyoti Ranjan
 *
 ****************************************************************/
void dusty_test(void)
{
    fbe_test_event_log_test_configuration_t * dusty_config_array_p = NULL;

    /* We will use same configuration for all test level. But we can change 
     * in future
     */
    if (fbe_sep_test_util_get_raid_testing_extended_level() > 0)
    {
        /* Use extended configuration.
         */
        dusty_config_array_p = dusty_raid_group_config_extended_qual;
    }
    else
    {
        /* Use minimal configuration.
         */
        dusty_config_array_p = dusty_raid_group_config_qual;
    }

    mumford_the_magician_run_tests(dusty_config_array_p);

    return;
}
/*************************
 * end dusty_test()
 ************************/


/*!**************************************************************
 * dusty_setup()
 ****************************************************************
 * @brief
 *  Setup for a event log test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  12/15/2010 - Created - Jyoti Ranjan
 *
 ****************************************************************/
void dusty_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    mumford_the_magician_common_setup();

    /* This test does error injection, turn off debug flags */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        mumford_simulation_setup(dusty_raid_group_config_qual,
                                 dusty_raid_group_config_extended_qual);
    }

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    return;
}
/**************************
 * end dusty_setup()
 *************************/

/*!**************************************************************
 * dusty_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup for the dusty_cleanup().
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  12/15/2010 - Created. Jyoti Ranjan
 *
 ****************************************************************/
void dusty_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    fbe_test_sep_util_restore_sector_trace_level();
    
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    mumford_the_magician_common_cleanup();

    return;
}
/***************************
 * end dusty_cleanup()
 ***************************/




/*****************************************
 * end file mumford_the_magician_test.c
 *****************************************/
