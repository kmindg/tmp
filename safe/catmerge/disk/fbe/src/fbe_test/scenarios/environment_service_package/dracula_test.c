/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file eva_01_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify B side configuration page for Voyager and Derringer.
 *
 * @version
 *   10/25/2011 - Created. dongz
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_sim_transport.h"
#include "pp_utils.h"
#include "fbe/fbe_esp_encl_mgmt.h"
#include "fbe_test_esp_utils.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_configurations.h"

#include "fbe_test_esp_utils.h"
#include "sep_tests.h"
#include "fp_test.h"

/*************************
 *   GLOBALS
 *************************/

char * dracula_short_desc = "Verify ESP system id info process.";
char * dracula_long_desc ="\
EVA 01\n\
        -You must know who is Duke Dracula \n\
\n\
Using Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
\n\
   1) initializes the terminator\n\
   2) loads the test configuration\n\
            a) inserts a board\n\
            b) inserts a port\n\
            c) inserts an enclosure to port 0\n\
            d) inserts 12 sas drives to port 0 encl 0\n\
\n\
   3) Initializes the physical package\n\
   4) initializes fbe_api\n\
   5) wait for the expected number of objects to be created\n\
   6) checks for the fleet board inserted\n\
   7) validates the ports added\n\
        - Loops through all the enclosures and their drives to check their state\n\
            a) Validates that the enclosure is in READY/ACTIVATE state\n\
            b) Validate that the physical drives are in READY/ACTIVATE state\n\
            c) Validate that the logical drives are in READY/ACTIVATE state\n\
        - if the number of enclosures is > 1 (i.e. other than 0th enclosure),\n\
          Get the handle of the port and validate enclosure exists or not\n\
\n\
   8) verifies the number of objects created.\n";

#define FBE_ENCL_MGMT_SYSTEM_INFO_SN_KEY "ESPEnclMgmtSysInfoSN"
#define FBE_ENCL_MGMT_SYSTEM_INFO_PN_KEY "ESPEnclMgmtSysInfoPN"
#define FBE_ENCL_MGMT_SYSTEM_INFO_REV_KEY "ESPEnclMgmtSysInfoRev"
#define FBE_ENCL_MGMT_USER_MODIFIED_SYSTEM_ID_INFO_KEY "ESPEnclMgmtUserModifiedSystemIdInfo"

typedef struct system_info_test_case_s{
    fbe_esp_encl_mgmt_system_id_info_t resume_prom_system_info;
    fbe_esp_encl_mgmt_system_id_info_t persistent_storage_system_info;
    fbe_bool_t user_modified;
    fbe_esp_encl_mgmt_system_id_info_t expected_rp_result;
    fbe_esp_encl_mgmt_system_id_info_t expected_persistent_result;
}system_info_test_case_t;

void dracula_test_setup(void);
fbe_status_t fbe_test_load_dracula_config(void);
fbe_bool_t dracula_system_id_info_equal_check(fbe_esp_encl_mgmt_system_id_info_t sys_id_1,
                                              fbe_esp_encl_mgmt_system_id_info_t sys_id_2);
void dracula_test_table_driven(system_info_test_case_t test_case);
void dracula_test(int begin, int end);

system_info_test_case_t system_info_test_table[] =
{
    //truth table:
    //p: persistent storage, rp: resume prom, 0 means invalid

    //user modify: FALSE
    //SN:
    //             rp    p
    //  before      x    y
    //  -------------------
    //  after       y    y
    //PN and Rev:
    //    before     |     after
    //    rp    p    |    rp    p
    //    -----------------------
    //     0    0    |     0    0
    //     0    y    |     y    y
    //     x    0    |     x    0
    //     x    y    |     y    y
    //0, 1
    {
        { "resume_prom_sn", "resume_prom_pn", "rp" }, //resume_prom_system_info
        { "persistent_sn",  "persistent_pn",  "p" },  //persistent_storage_system_info
        FALSE,                                        //user_modified
        { "persistent_sn", "persistent_pn", "p" },    //expected_rp_result
        { "persistent_sn", "persistent_pn", "p" },    //expected_persistent_result
    },
    {
        { "resume_prom_sn", {0},            "rp" },    //resume_prom_system_info
        { "persistent_sn", "persistent_pn", {0} },     //persistent_storage_system_info
        FALSE,                                         //user_modified
        { "persistent_sn", "persistent_pn", "rp" },    //expected_rp_result
        { "persistent_sn", "persistent_pn", {0} },     //expected_persistent_result
    },

    //user modify: FALSE
    //SN:
    //             rp    p
    //  before      x    0
    //  -------------------
    //  after       x    x
    //PN and Rev:
    //    before     |     after
    //    rp    p    |    rp    p
    //    -----------------------
    //     0    0    |     0    0
    //     0    y    |     0    y
    //     x    0    |     x    x
    //     x    y    |     x    x
    //2, 3
    {
        { "resume_prom_sn", "resume_prom_pn", "rp" },    //resume_prom_system_info
        { {0},              "persistent_pn",  "p"  },    //persistent_storage_system_info
        FALSE,                                           //user_modified
        { "resume_prom_sn", "resume_prom_pn", "rp" },      //expected_rp_result
        { "resume_prom_sn", "resume_prom_pn", "rp" },      //expected_persistent_result
    },
    {
        { "resume_prom_sn", {0},             "rp" },      //resume_prom_system_info
        { {0},              "persistent_pn", {0}  },      //persistent_storage_system_info
        FALSE,                                            //user_modified
        { "resume_prom_sn", {0},             "rp" },      //expected_rp_result
        { "resume_prom_sn", {0}, "rp" },                  //expected_persistent_result
    },

    //user modify: FALSE
    //SN:
    //             rp    p
    //  before      0    y
    //  -------------------
    //  after       y    y
    //PN and Rev:
    //    before     |     after
    //    rp    p    |    rp    p
    //    -----------------------
    //     0    0    |     0    0
    //     0    y    |     y    y
    //     x    0    |     x    0
    //     x    y    |     x    y
    //4, 5
    {
        { {0},             "resume_prom_pn", "rp" },  //resume_prom_system_info
        { "persistent_sn", "persistent_pn",  "p"  },  //persistent_storage_system_info
        FALSE,                                        //user_modified
        { "persistent_sn", "resume_prom_pn", "rp" },  //expected_rp_result
        { "persistent_sn", "persistent_pn",  "p"  },  //expected_persistent_result
    },
    {
        { {0},             {0},             "rp" },   //resume_prom_system_info
        { "persistent_sn", "persistent_pn", {0} },    //persistent_storage_system_info
        FALSE,                                        //user_modified
        { "persistent_sn", "persistent_pn", "rp" },   //expected_rp_result
        { "persistent_sn", "persistent_pn", {0} },    //expected_persistent_result
    },

    //user modify: TRUE
    //SN:
    //             rp    p
    //  before      x    y
    //  -------------------
    //  after       x    x
    //PN and Rev:
    //    before     |     after
    //    rp    p    |    rp    p
    //    -----------------------
    //     0    0    |     0    0
    //     0    y    |     0    y
    //     x    0    |     x    x
    //     x    y    |     x    x
    //6, 7
    {
        { "resume_prom_sn", "resume_prom_pn", "rp" }, //resume_prom_system_info
        { "persistent_sn",  "persistent_pn",  "p" },  //persistent_storage_system_info
        TRUE,                                        //user_modified
        { "resume_prom_sn", "resume_prom_pn", "rp" },    //expected_rp_result
        { "resume_prom_sn", "resume_prom_pn", "rp" },    //expected_persistent_result
    },
    {
        { "resume_prom_sn", {0},             "rp" },    //resume_prom_system_info
        { "persistent_sn",  "persistent_pn", {0}  },     //persistent_storage_system_info
        TRUE,                                         //user_modified
        { "resume_prom_sn", {0},             "rp" },    //expected_rp_result
        { "resume_prom_sn", "persistent_pn", "rp" },     //expected_persistent_result
    },

    //user modify: TRUE
    //SN:
    //             rp    p
    //  before      x    0
    //  -------------------
    //  after       x    x
    //PN and Rev:
    //    before     |     after
    //    rp    p    |    rp    p
    //    -----------------------
    //     0    0    |     0    0
    //     0    y    |     0    y
    //     x    0    |     x    x
    //     x    y    |     x    x
    //8, 9
    {
        { "resume_prom_sn", "resume_prom_pn", "rp" },    //resume_prom_system_info
        { {0},              "persistent_pn",  "p"  },    //persistent_storage_system_info
        TRUE,                                           //user_modified
        { "resume_prom_sn", "resume_prom_pn", "rp" },      //expected_rp_result
        { "resume_prom_sn", "resume_prom_pn", "rp" },      //expected_persistent_result
    },
    {
        { "resume_prom_sn", {0},             "rp" },      //resume_prom_system_info
        { {0},              "persistent_pn", {0}  },      //persistent_storage_system_info
        TRUE,                                            //user_modified
        { "resume_prom_sn", {0},             "rp" },      //expected_rp_result
        { "resume_prom_sn", {0}, "rp" },      //expected_persistent_result
    },

    //user modify: TRUE
    //SN:
    //             rp    p
    //  before      0    y
    //  -------------------
    //  after       y    y
    //PN and Rev:
    //    before     |     after
    //    rp    p    |    rp    p
    //    -----------------------
    //     0    0    |     0    0
    //     0    y    |     0    y
    //     x    0    |     x    0
    //     x    y    |     x    y
    //10, 11
    {
        { {0},             "resume_prom_pn", "rp" },  //resume_prom_system_info
        { "persistent_sn", "persistent_pn",  "p"  },  //persistent_storage_system_info
        TRUE,                                        //user_modified
        { "persistent_sn", "resume_prom_pn", "rp" },  //expected_rp_result
        { "persistent_sn", "persistent_pn",  "p"  },  //expected_persistent_result
    },
    {
        { {0},             {0},             "rp" },   //resume_prom_system_info
        { "persistent_sn", "persistent_pn", {0}  },    //persistent_storage_system_info
        TRUE,                                        //user_modified
        { "persistent_sn", {0},             "rp" },   //expected_rp_result
        { "persistent_sn", "persistent_pn", {0}  },    //expected_persistent_result
    },

     //user modify: FALSE
    //SN:
    //             rp    p
    //  before      y    0
    //  -------------------
    //  after       y    y
    //PN and Rev:
    //    before     |     after
    //    rp    p    |    rp    p
    //    -----------------------
    //     0    0    |     0    0
    //     0    y    |     0    y
    //     x    0    |     x    0
    //     x    y    |     x    y
    //12, 13
    {
        { "!@bad_number!?", "resume_prom_pn", "rp" },  //resume_prom_system_info
        { "persistent_sn", "persistent_pn",  "p"  },  //persistent_storage_system_info
        FALSE,                                        //user_modified
        { "persistent_sn", "resume_prom_pn", "rp" },  //expected_rp_result
        { "persistent_sn", "persistent_pn",  "p"  },  //expected_persistent_result
    },
    {
        { "resume_prom_sn", "resume_prom_pn", "rp" }, //resume_prom_system_info
        { "!@bad_persist?", "persistent_pn", "p"  },  //persistent_storage_system_info
        FALSE,                                        //user_modified
        { "resume_prom_sn", "resume_prom_pn", "rp" }, //expected_rp_result
        { "resume_prom_sn", "resume_prom_pn", "rp" }, //expected_persistent_result
    },

};

#define system_info_test_table_len (sizeof(system_info_test_table) / sizeof(system_info_test_case_t))

/*!**************************************************************
 * dracula_test_part1()
 ****************************************************************
 * @brief
 *  Verify ESP system id info process, part1, execute first 6 tests.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   8/25/2012 - Created. dongz
 *
 ****************************************************************/

void dracula_test_part1()
{
    dracula_test(0, system_info_test_table_len / 3 - 1);
}
/******************************************
 * end dracula_test_part1()
 ******************************************/

/*!**************************************************************
 * dracula_test_part2()
 ****************************************************************
 * @brief
 *  Verify ESP system id info process, part1, execute last 6 tests.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   8/25/2012 - Created. dongz
 *
 ****************************************************************/

void dracula_test_part2()
{
    dracula_test(system_info_test_table_len / 3, system_info_test_table_len / 3 * 2 - 1);
}
/******************************************
 * end dracula_test_part2()
 ******************************************/

/*!**************************************************************
 * dracula_test_part3()
 ****************************************************************
 * @brief
 *  Verify ESP system id info process, part1, execute last 6 tests.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   8/25/2012 - Created. dongz
 *
 ****************************************************************/

void dracula_test_part3()
{
    dracula_test( system_info_test_table_len / 3 * 2, system_info_test_table_len /3 * 3 - 1);
}
/******************************************
 * end dracula_test_part3()
 ******************************************/

/*!**************************************************************
 * dracula_test_part4()
 ****************************************************************
 * @brief
 *  Verify ESP system id info process, part1, execute last 6 tests.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   8/25/2012 - Created. dongz
 *
 ****************************************************************/

void dracula_test_part4()
{
    dracula_test( system_info_test_table_len / 3 * 3, system_info_test_table_len - 1);
}
/******************************************
 * end dracula_test_part4()
 ******************************************/

/*!**************************************************************
 * dracula_test()
 ****************************************************************
 * @brief
 *  Verify ESP system id info process.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   8/25/2012 - Created. dongz
 *
 ****************************************************************/

void dracula_test(int begin, int end)
{
    fbe_u32_t index;
    for(index = begin; index <= end; index++)
    {
        mut_printf(MUT_LOG_LOW, "=== start test index %d ===\n", index);
        dracula_test_table_driven(system_info_test_table[index]);
    }
}
/******************************************
 * end dracula_test()
 ******************************************/

/*!**************************************************************
 * dracula_test_table_driven()
 ****************************************************************
 * @brief
 *  Verify ESP system id info process by test case.
 *
 * @param test_case - test case
 *
 * @return None.
 *
 * @author
 *   8/25/2012 - Created. dongz
 *
 ****************************************************************/
void dracula_test_table_driven(system_info_test_case_t test_case)
{
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};
    fbe_esp_encl_mgmt_system_id_info_t read_persistent_sys_id = {0};
    fbe_esp_encl_mgmt_system_id_info_t read_rp_sys_id = {0};
    fbe_test_package_list_t                      package_list;
    fbe_status_t status;

	/* Initialize the structure before use */
	fbe_zero_memory(&package_list, sizeof(package_list));

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //debug only
    mut_printf(MUT_LOG_LOW, "=== configured terminator ===\n");

    status = fbe_test_load_dracula_config();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // debug only
    mut_printf(MUT_LOG_LOW, "=== simple config loading is done ===\n");

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return;
    }

    //set resume prom info, this time we use resume prom to preset persistent storage
    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    sfi_mask_data->structNumber = SPECL_SFI_RESUME_STRUCT_NUMBER;
    sfi_mask_data->smbDevice = MIDPLANE_RESUME_PROM;
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    fbe_copy_memory(sfi_mask_data->sfiSummaryUnions.resumeStatus.resumePromData.product_serial_number,
                    test_case.persistent_storage_system_info.serialNumber,
                    RESUME_PROM_PRODUCT_SN_SIZE);

    fbe_copy_memory(sfi_mask_data->sfiSummaryUnions.resumeStatus.resumePromData.product_part_number,
                    test_case.persistent_storage_system_info.partNumber,
                    RESUME_PROM_PRODUCT_PN_SIZE);

    fbe_copy_memory(sfi_mask_data->sfiSummaryUnions.resumeStatus.resumePromData.product_revision,
                    test_case.persistent_storage_system_info.revision,
                    RESUME_PROM_PRODUCT_REV_SIZE);

    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);

    mut_printf(MUT_LOG_LOW,
            "Setting resume SN: %.16s, PN: %.16s, Rev: %.3s\n",
            test_case.persistent_storage_system_info.serialNumber,
            test_case.persistent_storage_system_info.partNumber,
            test_case.persistent_storage_system_info.revision);



    status = fbe_test_startPhyPkg(TRUE, FP_MAX_OBJECTS);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_LOW, "=== phy pkg is started ===\n");

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();

    fbe_api_sleep(15000);

    //reset resume prom info, this will used as the rp info
    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    sfi_mask_data->structNumber = SPECL_SFI_RESUME_STRUCT_NUMBER;
    sfi_mask_data->smbDevice = MIDPLANE_RESUME_PROM;
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    fbe_copy_memory(sfi_mask_data->sfiSummaryUnions.resumeStatus.resumePromData.product_serial_number,
                    test_case.resume_prom_system_info.serialNumber,
                    RESUME_PROM_PRODUCT_SN_SIZE);

    fbe_copy_memory(sfi_mask_data->sfiSummaryUnions.resumeStatus.resumePromData.product_part_number,
                    test_case.resume_prom_system_info.partNumber,
                    RESUME_PROM_PRODUCT_PN_SIZE);

    fbe_copy_memory(sfi_mask_data->sfiSummaryUnions.resumeStatus.resumePromData.product_revision,
                    test_case.resume_prom_system_info.revision,
                    RESUME_PROM_PRODUCT_REV_SIZE);

    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);

    mut_printf(MUT_LOG_LOW,
            "Setting resume SN: %.16s, PN: %.16s, Rev: %.3s\n",
            test_case.resume_prom_system_info.serialNumber,
            test_case.resume_prom_system_info.partNumber,
            test_case.resume_prom_system_info.revision);

    //set user modified system id info key
    fbe_test_esp_registry_write(ESP_REG_PATH,
                   FBE_ENCL_MGMT_USER_MODIFIED_SYSTEM_ID_INFO_KEY,
                   FBE_REGISTRY_VALUE_DWORD,
                   &test_case.user_modified,
                   sizeof(fbe_bool_t));

    //restart esp
    mut_printf(MUT_LOG_LOW, "=== restart esp ===\n");
    fbe_test_esp_restart();
    mut_printf(MUT_LOG_LOW, "=== esp restart finished ===\n");

    fbe_api_esp_encl_mgmt_get_system_id_info(&read_persistent_sys_id);

    mut_printf(MUT_LOG_LOW,
            "Test Set persistent SN: %.16s, PN: %.16s, Rev: %.3s\n",
            test_case.persistent_storage_system_info.serialNumber,
            test_case.persistent_storage_system_info.partNumber,
            test_case.persistent_storage_system_info.revision);

    mut_printf(MUT_LOG_LOW,
            "Test Set resume SN: %.16s, PN: %.16s, Rev: %.3s\n",
            test_case.resume_prom_system_info.serialNumber,
            test_case.resume_prom_system_info.partNumber,
            test_case.resume_prom_system_info.revision);



    mut_printf(MUT_LOG_LOW,
            "Sys id info from persistent SN: %.16s, PN: %.16s, Rev: %.3s\n",
            read_persistent_sys_id.serialNumber,
            read_persistent_sys_id.partNumber,
            read_persistent_sys_id.revision);

    mut_printf(MUT_LOG_LOW,
            "Expected Sys id from persistent SN: %.16s, PN: %.16s, Rev: %.3s\n",
            test_case.expected_persistent_result.serialNumber,
            test_case.expected_persistent_result.partNumber,
            test_case.expected_persistent_result.revision);
    

    if(test_case.expected_persistent_result.serialNumber == 0)
    {
        MUT_ASSERT_INT_EQUAL(strncmp(read_persistent_sys_id.serialNumber, "INVALID_SER_NUM", strlen("INVALID_SER_NUM")), 0);
    }
    else
    {
        MUT_ASSERT_TRUE(dracula_system_id_info_equal_check(read_persistent_sys_id, test_case.expected_persistent_result));
    }

    //verify resume prom info
    //get resume prom info
    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    sfi_mask_data->structNumber = SPECL_SFI_RESUME_STRUCT_NUMBER;
    sfi_mask_data->smbDevice = MIDPLANE_RESUME_PROM;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;

    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);

    fbe_copy_memory(read_rp_sys_id.serialNumber,
                    sfi_mask_data->sfiSummaryUnions.resumeStatus.resumePromData.product_serial_number,
                    RESUME_PROM_PRODUCT_SN_SIZE);

    fbe_copy_memory(read_rp_sys_id.partNumber,
                    sfi_mask_data->sfiSummaryUnions.resumeStatus.resumePromData.product_part_number,
                    RESUME_PROM_PRODUCT_PN_SIZE);

    fbe_copy_memory(read_rp_sys_id.revision,
                    sfi_mask_data->sfiSummaryUnions.resumeStatus.resumePromData.product_revision,
                    RESUME_PROM_PRODUCT_REV_SIZE);

    mut_printf(MUT_LOG_LOW,
            "Sys id info from rp SN: %.16s, PN: %.16s, Rev: %.3s\n",
            read_rp_sys_id.serialNumber,
            read_rp_sys_id.partNumber,
            read_rp_sys_id.revision);

    if(test_case.expected_rp_result.serialNumber == 0)
    {
        MUT_ASSERT_INT_EQUAL(strncmp(read_rp_sys_id.serialNumber, "INVALID_SER_NUM", strlen("INVALID_SER_NUM")), 0);
    }
    else
    {
        MUT_ASSERT_TRUE(dracula_system_id_info_equal_check(read_rp_sys_id, test_case.expected_rp_result));
    }

    mut_printf(MUT_LOG_LOW, "=== unload esp ===\n");
    fbe_test_esp_unload();

    //unload physical package

    package_list.number_of_packages = 1;
    package_list.package_list[0] = FBE_PACKAGE_ID_PHYSICAL;

    fbe_test_common_util_package_destroy(&package_list);
    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);

    mut_printf(MUT_LOG_LOW, "=== %s physical package destroy completes ===\n", __FUNCTION__);
    fbe_api_terminator_destroy();
    free(sfi_mask_data);
    fbe_test_esp_delete_registry_file();
}
/******************************************
 * end dracula_test_table_driven()
 ******************************************/

/*!**************************************************************
 * fbe_test_load_dracula_config()
 ****************************************************************
 * @brief
 *  Setup with mustang default settings, and create one port,
 *  one enclosure, and 15 drives.
 *  copied from fbe_test_load_fflewddur_fflam_config
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   08/18/2012 - Created. dongz
 *
 ****************************************************************/
fbe_status_t fbe_test_load_dracula_config(void)
{
    fbe_status_t status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t board_info;
    fbe_terminator_sas_port_info_t sas_port;
    fbe_terminator_sas_encl_info_t sas_encl;

    fbe_api_terminator_device_handle_t port_handle;
    fbe_api_terminator_device_handle_t encl_handle;
    fbe_api_terminator_device_handle_t drive_handle;

    fbe_u32_t no_of_ports = 0;
    fbe_u32_t no_of_encls = 0;
    fbe_u32_t slot = 0;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_PROMETHEUS_M1_HW_TYPE;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_private_space_layout_initialize_library(board_info.platform_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (no_of_ports = 0; no_of_ports < FP_MAX_PORTS; no_of_ports ++)
    {
        /*insert a port*/
        sas_port.backend_number = no_of_ports;
        sas_port.io_port_number = 3 + no_of_ports;
        sas_port.portal_number = 5 + no_of_ports;
        sas_port.sas_address = 0x5000097A7800903F + no_of_ports;
        sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
        status  = fbe_api_terminator_insert_sas_port (&sas_port, &port_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        for ( no_of_encls = 0; no_of_encls < FP_MAX_ENCLS; no_of_encls++ )
        {
            /*insert an enclosure to port 0*/
            sas_encl.backend_number = no_of_ports;
            sas_encl.encl_number = no_of_encls;
            sas_encl.uid[0] = no_of_encls; // some unique ID.
            sas_encl.sas_address = 0x5000097A79000000 + ((fbe_u64_t)(sas_encl.encl_number) << 16);
            sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
            status  = fbe_api_terminator_insert_sas_enclosure (port_handle, &sas_encl, &encl_handle);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            for (slot = 0; slot < FP_MAX_DRIVES; slot ++)
            {
                if(no_of_encls < 2 || (no_of_encls == 2 && slot >=2 && slot <= 13))
                {
                    status  = fbe_test_pp_util_insert_sas_drive(no_of_ports,
                                                                 no_of_encls,
                                                                 slot,
                                                                 520,
                                                                 TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                                 &drive_handle);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                }
            }
        }
    }
    return status;
}
/******************************************
 * end fbe_test_load_dracula_config()
 ******************************************/

/*!**************************************************************
 * dracula_system_id_info_equal_check()
 ****************************************************************
 * @brief
 *  Check if system id is equal or not.
 *
 * @param sys_id_1 - system id 1
 * @param sys_id_2 - system id 2
 *
 * @return fbe_bool_t - is two system id equal?
 *
 * @author
 *   8/25/2012 - Created. dongz
 *
 ****************************************************************/
fbe_bool_t dracula_system_id_info_equal_check(fbe_esp_encl_mgmt_system_id_info_t sys_id_1,
                                              fbe_esp_encl_mgmt_system_id_info_t sys_id_2)
{
    fbe_bool_t result = TRUE;
    result &= strncmp(sys_id_1.serialNumber, sys_id_2.serialNumber, RESUME_PROM_PRODUCT_SN_SIZE) == 0;
    result &= strncmp(sys_id_1.partNumber, sys_id_2.partNumber, RESUME_PROM_PRODUCT_PN_SIZE) == 0;
    result &= strncmp(sys_id_1.revision, sys_id_2.revision, RESUME_PROM_PRODUCT_REV_SIZE) == 0;
    return result;
}
/******************************************
 * end dracula_system_id_info_equal_check()
 ******************************************/

/*************************
 * end file dracula_test.c
 *************************/
