
/***************************************************************************
* Copyright (C) EMC Corporation 2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_db_transaction_test.c
***************************************************************************
*
* DESCRIPTION
* This is the test of fbe cli "transaction".
* 
* HISTORY
*   08/10/2011:  Created. gaoh1 and Jason Zhang
*
***************************************************************************/

#include "fbe_cli_tests.h"
#include "fbe_database.h"
#include "fbe/fbe_api_database_interface.h"
#include "sep_tests.h"



static fbe_test_rg_configuration_t transaction_rg_configuration[] = 
{
    /* width, capacity,        raid type,                        class,               block size, RAID-id,                        bandwidth.*/
    3,     FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 520,        0, 0,
    /* rg_disk_set */
    {{0,0,4}, {0,0,5}, {0,0,6}}
};

// fbe cli command and keyword array.
static fbe_cli_cmd_t db_pending_transaction_test_cases[] =
{
    {
    "database -getTransaction",                 /*test if the CLI can find the in-progress transaction state*/
    {"DATABASE_TRANSACTION_STATE_ACTIVE"}, 
    {"DATABASE_TRANSACTION_STATE_INACTIVE"} 
    },
    {
    "database -getTransaction",                 /*test if the CLI can find the created LUN in the in-progress transaction*/
    {"DATABASE_CLASS_ID_LUN"},
    {"No valid object entry found"}
    },
    {
    "database -getTransaction",                /*test if the CLI can find the created LUN number in the in-progress transaction*/
    {"123"},
    {"No valid object entry found"}
    },
    {
    "database -getTransaction",               /*test if the CLI can find the entry state correctly in the in-progress transaction*/
    {"DATABASE_CONFIG_ENTRY_CREATE"},
    {"DATABASE_TRANSACTION_STATE_INACTIVE"}
    },
    {NULL, {NULL},{NULL}},
};

// fbe cli command and keyword array.
static fbe_cli_cmd_t db_no_transaction_test_cases[] =
{
    {
    "database -getTransaction",               /*test if the CLI can find the transaction state correctly when no transaction*/
    {"DATABASE_TRANSACTION_STATE_INACTIVE"},
    {"DATABASE_TRANSACTION_STATE_ACTIVE"}
    },
    {"database -getTransaction",              /*test if the CLI can find the entry state correctly when no transaction*/
    {"No valid object entry found"},
    {"DATABASE_TRANSACTION_STATE_ACTIVE"}
    },
    {
    "database -getTransaction -verbose",      /*test if the CLI work correctly with verbose option*/
    {"DATABASE_CONFIG_ENTRY_INVALID"},
    {"DATABASE_TRANSACTION_STATE_ACTIVE"}
    },
    {
    "database -getTransac",                   /*test if the CLI can handle wrong command option*/
    {"usage"},
    {"DATABASE_TRANSACTION_STATE_INACTIVE"}
    },
    {NULL, {NULL},{NULL}},

};
// fbe cli command and keyword array.
static fbe_cli_cmd_t db_post_transaction_test_cases[] =
{
    {
    "database -getTransaction",              /*test if the CLI can find the transaction state correctly when it is aborted*/
    {"DATABASE_TRANSACTION_STATE_INACTIVE"},
    {"DATABASE_TRANSACTION_STATE_ACTIVE"}
    },
    {NULL, {NULL},{NULL}},

};



// "transaction" test setup function
void fbe_cli_db_transaction_setup(void)
{
    shrek_test_setup();

    /* Set up fbecli to run in interactive mode with option "s a"*/
    init_test_control(default_cli_executable_name,default_cli_script_file_name,CLI_DRIVE_TYPE_SIMULATOR, CLI_SP_TYPE_SPA,CLI_MODE_TYPE_INTERACTIVE);

}
// "transaction" test
void fbe_cli_db_transaction_test(void)
{
    fbe_database_transaction_info_t transaction_info = {0};
    fbe_u32_t case_count;
    fbe_status_t status;
    fbe_database_control_lun_t create_lun;

    csx_p_thr_sleep_secs(2); /* SAFEBUG - CGCG - first -getTransaction call causes problems sometimes - need to investigate */

    // test the transaction CLI when no transaction exists
    case_count = fbe_test_cli_cmd_get_count(&db_no_transaction_test_cases[0]);
    fbe_cli_run_all_cli_cmds(transaction_rg_configuration, db_no_transaction_test_cases, case_count);

    // start a transaction
    mut_printf(MUT_LOG_TEST_STATUS, "=== Start transaction for LUN creation ===\n");
    transaction_info.transaction_type = FBE_DATABASE_TRANSACTION_CREATE;
    status = fbe_api_database_start_transaction(&transaction_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    // create a lun in side the transaction
    mut_printf(MUT_LOG_TEST_STATUS, "=== Issue LUN create! ===\n");

    create_lun.lun_number = 123;
    create_lun.lun_set_configuration.config_flags = FBE_LUN_CONFIG_NONE;
    create_lun.lun_set_configuration.capacity = 0x1000;
    create_lun.transaction_id = transaction_info.transaction_id;
    create_lun.object_id = FBE_OBJECT_ID_INVALID;
    status = fbe_api_database_create_lun(create_lun);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN created successfully ===\n");

    // test the transaction CLI when it is in progress
    case_count = fbe_test_cli_cmd_get_count(&db_pending_transaction_test_cases[0]);
    fbe_cli_run_all_cli_cmds(transaction_rg_configuration, db_pending_transaction_test_cases, case_count);


    // abort this transaction
    mut_printf(MUT_LOG_TEST_STATUS, "=== Abort Transaction ===\n");
    status = fbe_api_database_abort_transaction(transaction_info.transaction_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // Test the transaction CLI when it is aborted
    case_count = fbe_test_cli_cmd_get_count(&db_post_transaction_test_cases[0]);
    fbe_cli_run_all_cli_cmds(transaction_rg_configuration, db_post_transaction_test_cases, case_count);

}



// "transaction" test teardown function
void fbe_cli_db_transaction_teardown(void)
{
    shrek_test_cleanup();
}
