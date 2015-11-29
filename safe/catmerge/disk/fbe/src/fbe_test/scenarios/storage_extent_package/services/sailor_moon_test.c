/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file sailor_moon_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the test scenario when two of the first three system 
 *  drives are dead and the only left one drive has bad blocks on Database LUN,
 *  SEP still try to load the database and the topology as much as possible. And
 *  database state is set to fbe_database_state_corrupt and won't allow any 
 *  configuration change.
 *
 * @version
 *  11/22/2011 - Created. Vera Wang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "sep_tests.h"
#include "sep_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "mut.h"   

#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"

#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"

#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_protocol_error_injection_service.h"
#include "neit_utils.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_random.h"


/*************************
 *   TEST DESCRIPTION
 *************************/
char * sailor_moon_short_desc = "test of recovering from a bad block and degraded 3 way mirror";
char * sailor_moon_long_desc ="\
The sailor_moon tests the scenario when two of the first three system drives are dead and the only left one drive has bad blocks on Database LUN,\n\
we still try to load the database and the topology as much as possible.\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] viper enclosure\n\
        [PP] SAS drives\n\
        [PP] logical drive\n\
        [SEP] provision drive\n\
\n\
STEP 1: Configure all raid groups and LUNs.\n\
        - Create 2 raid groups.\n\
        - Create 2 luns on each raid group.\n\
STEP 2: Destroy SEP.\n\
STEP 3: Pull 2 system drives out.\n\
STEP 4: Inject errors to the only available system drive on DB LUN.\n\
        - Inject media error to the first entry in object table.\n\
        - Inject media error to the entry which belongs to the raid group in user table.\n\
        - Inject media error to the first entry in edge table.\n\
        - Inject media error to the second entry which belongs to global system power saving entry in global info table.\n\
STEP 5: Reload SEP.\n\
STEP 6: Verify the topology.\n\
        - One raid group with bad block in user tables is in specialize state and the two luns belongs to this raid group are also in specialize state.\n\
        - The other raid group without any bad blocks related to it is in read state and the two lun belongs to this raid group are also in ready state.\n\
\n\
\n\
\n\
Description last updated: 02/14/2011.\n";


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static void sailor_moon_test_create_rg(void);
static fbe_status_t sailor_moon_test_create_lun(fbe_raid_group_number_t rg_id,fbe_lun_number_t lun_number);
static void sailor_moon_test_lookup_corrupt_object(fbe_object_id_t * rg_object_id, fbe_object_id_t* lun_object_id);
static void sailor_moon_test_inject_scsi_error(fbe_u32_t bus,fbe_u32_t enclosure,fbe_u32_t slot);
static void sailor_moon_test_get_db_lun_start_lba(fbe_lba_t* start_lba);
static void sailor_moon_test_load_physical_config(void);


/*!*******************************************************************
 * @def SAILOR_MOON_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define SAILOR_MOON_TEST_NS_TIMEOUT        30000 /*wait maximum of 30 seconds*/

/**********************************************************************************/

#define SAILOR_MOON_TEST_ENCL_MAX 4
#define SAILOR_MOON_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66 /* (1 board + 1 port + 4 encl + 60 pdo) */

#define SAILOR_MOON_TEST_RAID_GROUP_COUNT 2
#define SAILOR_MOON_TEST_LUNS_PER_RAID_GROUP 2

static fbe_test_rg_configuration_t sailor_moon_rg_config[] = 
{
    /* width, capacity,        raid type,                        class,        block size, RAID-id,    bandwidth.*/
    {4,      0xE000,     FBE_RAID_GROUP_TYPE_RAID5,    FBE_CLASS_ID_PARITY,    520,            0,          0,
     {{0,0,4}, {0,0,5}, {0,0,6}, {0,0,7}}},
    {4,      0xE000,     FBE_RAID_GROUP_TYPE_RAID5,    FBE_CLASS_ID_PARITY,    520,            1,          0,
     {{0,1,4}, {0,1,5}, {0,1,6}, {0,1,7}}},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,/* Terminator. */},
};


/*!*******************************************************************
 * @def SAILOR_MOON_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define SAILOR_MOON_TEST_DRIVE_CAPACITY TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY

/************************************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 ***********************************************************************************/

/* The different test numbers.
 */
typedef enum sailor_moon_test_enclosure_num_e
{
    SAILOR_MOON_TEST_ENCL1_WITH_SAS_DRIVES = 0,
    SAILOR_MOON_TEST_ENCL2_WITH_SAS_DRIVES,
    SAILOR_MOON_TEST_ENCL3_WITH_SAS_DRIVES,
    SAILOR_MOON_TEST_ENCL4_WITH_SAS_DRIVES
} sailor_moon_test_enclosure_num_t;

typedef enum sailor_moon_test_drive_type_e
{
    SAS_DRIVE,
} sailor_moon_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    sailor_moon_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    sailor_moon_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;

/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        SAILOR_MOON_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        SAILOR_MOON_TEST_ENCL2_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        SAILOR_MOON_TEST_ENCL3_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        SAILOR_MOON_TEST_ENCL4_WITH_SAS_DRIVES,
        520
    },
};

/* Count of rows in the table.
 */
#define SMURDSFS_TEST_ENCL_MAX sizeof(encl_table)/sizeof(encl_table[0])


/*!**************************************************************
 * sailor_moon_test()
 ****************************************************************
 * @brief
 *  Configure the sailor moon test logical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/
void sailor_moon_test(void)
{
    fbe_status_t		                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t                           rg_index;
    fbe_u32_t                           lun_index;
    fbe_u32_t                           lun_number=123;
    fbe_test_rg_configuration_t *       rg_config_p = NULL;
    fbe_object_id_t                     rg_object_id[SAILOR_MOON_TEST_RAID_GROUP_COUNT]; 
    fbe_object_id_t                     lun_object_id[2*SAILOR_MOON_TEST_RAID_GROUP_COUNT]; 
    fbe_system_power_saving_info_t      power_save_info;
    fbe_database_state_t                db_state;                           

    /* Create two raid groups. */
    sailor_moon_test_create_rg();
    for (rg_index = 0; rg_index < SAILOR_MOON_TEST_RAID_GROUP_COUNT; rg_index++)
    {
		rg_config_p = &sailor_moon_rg_config[rg_index];
        /*  Create two luns on each of the two raid groups.*/
        for (lun_index = 0; lun_index < SAILOR_MOON_TEST_LUNS_PER_RAID_GROUP; lun_index++) {
            sailor_moon_test_create_lun(rg_config_p->raid_group_id,lun_number);
            fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id[SAILOR_MOON_TEST_LUNS_PER_RAID_GROUP*rg_index+lun_index]);
            lun_number++;
        }
        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id[rg_index]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* This is used for verify the global information table. */
    fbe_api_enable_system_power_save();

    sep_config_load_neit();
    /* Destroy sep. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Destroy SEP. ===\n");
    fbe_test_sep_util_destroy_sep();

    /* Pull out 2 DB drives. */ 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Pull out 2 system drives. ===\n");
    status = fbe_test_pp_util_pull_drive(0, 0, 0, &drive_handle);
    status = fbe_test_pp_util_pull_drive(0, 0, 1, &drive_handle);

    /* Inject errors to the only available system drive. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Inject errors to the only available system drive. ===\n");
    sailor_moon_test_inject_scsi_error(0,0,2);  
 
    /* Reload sep. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Reload SEP again. ===\n");
    sep_config_load_sep();  

    status = fbe_api_get_system_power_save_info(&power_save_info);
    MUT_ASSERT_INT_EQUAL(power_save_info.enabled, 0);
    
    /* Check database tables given object id and find the corrupt objects, 
       and then verify the corrupt object is in special state. */ 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Verify the topology. ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Verify the database state is FBE_DATABASE_STATE_CORRUPT. \n");
    status = fbe_api_database_get_state(&db_state);
    MUT_ASSERT_INT_EQUAL(db_state, FBE_DATABASE_STATE_SERVICE_MODE);

#if 0
    mut_printf(MUT_LOG_TEST_STATUS, "Verify there are some corrupt objects which are in specialize state and the object header state is DATABASE_CONFIG_ENTRY_CORRUPT. \n");
    sailor_moon_test_lookup_corrupt_object(rg_object_id,lun_object_id);

    /* Try to create a new lun when database is in corrupt state, we expect lun creation failed. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create a new LUN. ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "We are expecting a LUN creation failed since database is in corrupt state.\n");
    status = sailor_moon_test_create_lun(rg_config_p->raid_group_id,234);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);
#endif
    return; 
}


/*!**************************************************************
 * sailor_moon_test_create_rg()
 ****************************************************************
 * @brief
 *  Configure the sailor moon test logical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void sailor_moon_test_create_rg(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                     rg_object_id_from_job = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_status_t                        job_status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *       rg_config_p = NULL;
    fbe_u32_t                           rg_index;

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create a RAID Group ===\n");

    for (rg_index = 0; rg_index < SAILOR_MOON_TEST_RAID_GROUP_COUNT; rg_index++)
    {
		rg_config_p = &sailor_moon_rg_config[rg_index];
        status = fbe_test_sep_util_create_raid_group(rg_config_p);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

        /* wait for notification from job service. */
        status = fbe_api_common_wait_for_job(sailor_moon_rg_config[rg_index].job_number,
                                         SAILOR_MOON_TEST_NS_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         &rg_object_id_from_job);

        MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
        MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

        /* Verify the object id of the raid group */
        status = fbe_api_database_lookup_raid_group_by_number(sailor_moon_rg_config[rg_index].raid_group_id, &rg_object_id);

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(rg_object_id_from_job, rg_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

        /* Verify the raid group get to ready state in reasonable time */
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, SAILOR_MOON_TEST_NS_TIMEOUT,
                                                     FBE_PACKAGE_ID_SEP_0);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object 0x%X\n", rg_object_id);
    }
        mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__); 
    
    return;

} /* End sailor_moon_test_create_rg() */

/*!**************************************************************
 * sailor_moon_test_create_lun()
 ****************************************************************
 * @brief
 *  Lun Creation function for sailor_moon_test.
 *
 * @param fbe_lun_number_t             lun_number
 *        fbe_raid_group_number_t      rg_id
 *
 * @return fbe_status_t    status
 *
 ****************************************************************/
static fbe_status_t sailor_moon_test_create_lun(fbe_raid_group_number_t rg_id, fbe_lun_number_t lun_number)
{
    fbe_status_t                    status;
    fbe_object_id_t					lu_id; 
    fbe_api_lun_create_t            fbe_lun_create_req;
    fbe_job_service_error_type_t    job_error_type;
     
    /* Create a LUN */
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_lun_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
    fbe_lun_create_req.raid_group_id = rg_id; /*sep_standard_logical_config_one_r5 is 5 so this is what we use*/
    fbe_lun_create_req.lun_number = lun_number;
    fbe_lun_create_req.capacity = 0x500;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
    fbe_lun_create_req.world_wide_name.bytes[0] = fbe_random() & 0xf;
    fbe_lun_create_req.world_wide_name.bytes[1] = fbe_random() & 0xf;

    status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, SAILOR_MOON_TEST_NS_TIMEOUT, &lu_id, &job_error_type);
    if(status == FBE_STATUS_OK){
       mut_printf(MUT_LOG_TEST_STATUS, "%s: LUN 0x%X created successfully! \n", __FUNCTION__, lu_id); 	
    }
    else {
       mut_printf(MUT_LOG_TEST_STATUS, "%s: LUN 0x%X created failed! \n", __FUNCTION__, lun_number);
    }

    return status;   
} /* End sailor_moon_test_create_lun() */

static void sailor_moon_test_lookup_corrupt_object(fbe_object_id_t* rg_object_id, fbe_object_id_t* lun_object_id)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_rg_configuration_t *           rg_config_p = NULL;
    fbe_u32_t                               rg_index;
    fbe_u32_t                               lun_index;
    fbe_database_get_tables_t               get_tables;
    fbe_lifecycle_state_t                   current_state;

    for (rg_index = 0; rg_index < SAILOR_MOON_TEST_RAID_GROUP_COUNT; rg_index++)
    {
		rg_config_p = &sailor_moon_rg_config[rg_index];

        /* Check rg object status*/
        status = fbe_api_database_get_tables (rg_object_id[rg_index],&get_tables);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status  = fbe_api_get_object_lifecycle_state(rg_object_id[rg_index], &current_state, FBE_PACKAGE_ID_SEP_0); 
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        if (get_tables.object_entry.header.state == DATABASE_CONFIG_ENTRY_CORRUPT){              
            MUT_ASSERT_INT_EQUAL(current_state, FBE_LIFECYCLE_STATE_SPECIALIZE);
            mut_printf(MUT_LOG_TEST_STATUS, "The corrupt object is the rg object 0x%X and it's in specialize state.\n", rg_object_id[rg_index]);  
        }
        else {
            /* Verify the raid group get to ready state in reasonable time */
            status = fbe_api_wait_for_object_lifecycle_state(rg_object_id[rg_index], 
                                                     FBE_LIFECYCLE_STATE_READY, SAILOR_MOON_TEST_NS_TIMEOUT,
                                                     FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        for (lun_index = 0; lun_index < SAILOR_MOON_TEST_LUNS_PER_RAID_GROUP; lun_index++) {
            status = fbe_api_database_get_tables (lun_object_id[SAILOR_MOON_TEST_LUNS_PER_RAID_GROUP*rg_index+lun_index],&get_tables);
            if (current_state == FBE_LIFECYCLE_STATE_SPECIALIZE){               
                status  = fbe_api_get_object_lifecycle_state(lun_object_id[SAILOR_MOON_TEST_LUNS_PER_RAID_GROUP*rg_index+lun_index], &current_state, FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                MUT_ASSERT_INT_EQUAL(current_state, FBE_LIFECYCLE_STATE_SPECIALIZE);
                mut_printf(MUT_LOG_TEST_STATUS, "The corrupt object is the lun object 0x%X and it's in specialize state.\n", lun_object_id[SAILOR_MOON_TEST_LUNS_PER_RAID_GROUP*rg_index+lun_index]);  
            }
            else{
                /* Verify the lun get to ready state in reasonable time */
                status = fbe_api_wait_for_object_lifecycle_state(lun_object_id[SAILOR_MOON_TEST_LUNS_PER_RAID_GROUP*rg_index+lun_index], 
                                                     FBE_LIFECYCLE_STATE_READY, SAILOR_MOON_TEST_NS_TIMEOUT,
                                                     FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }

        }
    }
    return; 
}

void sailor_moon_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for sailor moon test ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Load the physical configuration */
    sailor_moon_test_load_physical_config();

    /* Load sep and neit packages */
    sep_config_load_sep();

    return;
}

void sailor_moon_cleanup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for sailor_moon test ===\n");
   
    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Destroy the test configuration */
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}

void
sailor_moon_test_fill_error_injection_record(fbe_protocol_error_injection_error_record_t * record,
                                             fbe_lba_t start_lba,
                                             fbe_lba_t end_lba,
                                             fbe_object_id_t object_id)
{
    record->object_id = object_id; 
    record->lba_start = start_lba;
    record->lba_end   = end_lba;
    record->num_of_times_to_insert = 0xFF;   

    record->protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
    record->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = FBE_SCSI_READ_16;
    record->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = FBE_SCSI_SENSE_KEY_MEDIUM_ERROR;
    record->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code = FBE_SCSI_ASC_READ_DATA_ERROR;

}

static void sailor_moon_test_inject_scsi_error(fbe_u32_t bus,fbe_u32_t enclosure,fbe_u32_t slot)
{
    /* In simulation, the terminator always returns a good spinup status so we have to force it for at least one drive
    so later we can test the spin up counter going up*/
    fbe_protocol_error_injection_error_record_t     record_object;        // error injection record for object table 
    fbe_protocol_error_injection_error_record_t     record_user;          // error injection record for user table
    fbe_protocol_error_injection_error_record_t     record_edge;          // error injection record for edge table
    fbe_protocol_error_injection_error_record_t     record_global_info;   // error injection record for global info table
    fbe_protocol_error_injection_record_handle_t    out_record_handle_p_object;
    fbe_protocol_error_injection_record_handle_t    out_record_handle_p_user;                                                                        
    fbe_protocol_error_injection_record_handle_t    out_record_handle_p_edge;
    fbe_protocol_error_injection_record_handle_t    out_record_handle_p_global_info;
    fbe_status_t                                    status;                 // fbe status
    fbe_object_id_t                                 object_id;
    fbe_lba_t                                       db_lun_start_lba;
    fbe_lba_t                                       object_entry_start_lba;
    fbe_lba_t                                       user_entry_start_lba;
    fbe_lba_t                                       edge_entry_start_lba;
    fbe_lba_t                                       global_info_entry_start_lba;

    fbe_test_neit_utils_init_error_injection_record(&record_object);
    fbe_test_neit_utils_init_error_injection_record(&record_user);
    fbe_test_neit_utils_init_error_injection_record(&record_edge);
    fbe_test_neit_utils_init_error_injection_record(&record_global_info);

    /* We'll set a spinup error on PDO 10 so when in ACTIVATE it will explicitly set the not_spinning condition and then 
    the counter will go up once spin up is successfull*/
    status = fbe_api_get_physical_drive_object_id_by_location(bus,
                                                              enclosure,
                                                              slot,
                                                              &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sailor_moon_test_get_db_lun_start_lba(&db_lun_start_lba);

    /* if the database layout changed, these calculation might need to be changed accodingly.*/
    object_entry_start_lba = db_lun_start_lba + 1 + FBE_PERSIST_TRAN_ENTRY_MAX * FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY * 4; //0x97F81
    edge_entry_start_lba = object_entry_start_lba + 640 * FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY; // 0x98c01
    user_entry_start_lba = edge_entry_start_lba + 2560 * FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY; // 0x9be01 
    global_info_entry_start_lba = user_entry_start_lba + 576 * FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY; //0x9c941

    sailor_moon_test_fill_error_injection_record(&record_object,object_entry_start_lba,object_entry_start_lba+1,object_id);
    /* need to corrupt the particular edge. */
    sailor_moon_test_fill_error_injection_record(&record_edge,edge_entry_start_lba+0x28,edge_entry_start_lba+0x28+1,object_id); //0x98c29,0x98c2a 
    /* need to corrupt the particular user edge which belongs to the raid group for further verify. */
    sailor_moon_test_fill_error_injection_record(&record_user,user_entry_start_lba+0x119,user_entry_start_lba+0x119+1,object_id); //0x9bf1a,0x9bf1b  
    sailor_moon_test_fill_error_injection_record(&record_global_info,global_info_entry_start_lba+5,global_info_entry_start_lba+5+1,object_id); //0x9c946,0x9c947  

    /* Add the error injection record to the service, which also returns the record handle for later use */
    status = fbe_api_protocol_error_injection_add_record(&record_object, &out_record_handle_p_object);
    status = fbe_api_protocol_error_injection_add_record(&record_user, &out_record_handle_p_user);
    status = fbe_api_protocol_error_injection_add_record(&record_edge, &out_record_handle_p_edge);
    status = fbe_api_protocol_error_injection_add_record(&record_global_info, &out_record_handle_p_global_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);  

    /* Start the error injection */
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

}

static void sailor_moon_test_get_db_lun_start_lba(fbe_lba_t* start_lba)
{
    fbe_status_t                        status;                 // fbe status
    fbe_private_space_layout_lun_info_t db_lun;
    fbe_private_space_layout_extended_lun_info_t db_lun_ext;

    status = fbe_private_space_layout_get_lun_by_object_id(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE, &db_lun);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_private_space_layout_get_lun_extended_info_by_lun_number(db_lun.lun_number, &db_lun_ext);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    *start_lba = db_lun_ext.starting_block_address;
}

/*!**************************************************************
 * sailor_moon_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the sailor_moon test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void sailor_moon_test_load_physical_config(void)
{

    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                               total_objects = 0;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[SAILOR_MOON_TEST_ENCL_MAX];
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot = 0;
    fbe_u32_t                               enclosure = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== configuring terminator ===\n");
    /* Before loading the physical package we initialize the terminator */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(   1, /* io port */
                                            2, /* portal */
                                            0, /* backend number */ 
                                            &port0_handle);

    /* Insert enclosures to port 0
     */
    for ( enclosure = 0; enclosure < SAILOR_MOON_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < SAILOR_MOON_TEST_ENCL_MAX; enclosure++)
    {
        for ( slot = 0; slot < FBE_TEST_DRIVES_PER_ENCL; slot++)
        {
            fbe_test_pp_util_insert_sas_drive(encl_table[enclosure].port_number, 
                                              encl_table[enclosure].encl_number,   
                                              slot,                                
                                              encl_table[enclosure].block_size,    
                                              SAILOR_MOON_TEST_DRIVE_CAPACITY,      
                                              &drive_handle);
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== set physical package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(SAILOR_MOON_TEST_NUMBER_OF_PHYSICAL_OBJECTS, SAILOR_MOON_TEST_NS_TIMEOUT, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== verifying configuration ===\n");

    /* Inserted a armada board so we check for it;
     * board is always assumed to be object id 0
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == SAILOR_MOON_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End sailor_moon_test_load_physical_config() */


