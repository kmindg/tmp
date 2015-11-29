/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file vayu.c
 ***************************************************************************
 *
 * @brief
 *  Verify the resume prom info for all Resume prom devices. (SP, Enclosure, LCC, 
 *  PS, IO module, IO annex, Mezzanine, Mgmt module, etc) is gathered in ESP.
 * 
 * @version
 *   24-Jan-2011 - Created. Arun S
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_pe_types.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_module_info.h"
#include "esp_tests.h"
#include "fbe_test_configurations.h"
#include "fbe_base_board.h"
#include "specl_sfi_types.h"
#include "fbe/fbe_api_resume_prom_interface.h"
#include "fbe_test_esp_utils.h"
#include "generic_utils_lib.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

char * vayu_short_desc = "Verify the ESP resume prom info, resume prom write and resume prom read initiation";
char * vayu_long_desc ="\
Vayu\n\
        -  is a primary Hindu deity, the Lord of the winds.\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
\n\
STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
        - Start the foll. service in user space:\n\
        - 1. Memory Service\n\
        - 2. Scheduler Service\n\
        - 3. Eventlog service\n\
        - 4. FBE API \n\
        - Verify that the foll. classes are loaded and initialized\n\
        - 1. ERP\n\
        - 2. Enclosure Firmware Download\n\
        - 3. SPS Management\n\
        - 4. Flexports\n\
        - 5. EIR\n\
STEP 2: Get Resume prom info from ESP and verify the status\n\
STEP 3: Verify the Resume prom write\n\
        - Issue resume prom write command\n\
        - Get the resume prom info to verify\n\
STEP 4: Verify the Resume prom force read\n\
        - Update the terminator resume prom buffer\n\
        - Issue resume force read command\n\
        - Get the resue prom info to verify\n\
";


void vayu_test_resume_prom_read(fbe_u64_t deviceType, 
                                    fbe_device_physical_location_t * pLocation,
                                    fbe_resume_prom_status_t expectedStatus)
{
    fbe_esp_base_env_get_resume_prom_info_cmd_t * pGetResumePromInfo = NULL;
    fbe_status_t                                  fbeStatus = FBE_STATUS_OK;

    /* Allocate memory for pGetResumeProm */
    pGetResumePromInfo = (fbe_esp_base_env_get_resume_prom_info_cmd_t *) malloc (sizeof (fbe_esp_base_env_get_resume_prom_info_cmd_t));
    if(pGetResumePromInfo == NULL)
    {
        mut_printf(MUT_LOG_LOW, "%s memory alloc failed.",__FUNCTION__);
        return;
    }

    memset(pGetResumePromInfo, 0, sizeof (fbe_esp_base_env_get_resume_prom_info_cmd_t));

    pGetResumePromInfo->deviceType = deviceType;
    pGetResumePromInfo->deviceLocation = *pLocation;

    /* Get the device Resume Info */
    mut_printf(MUT_LOG_LOW, "Step 1: Get Resume Prom Info from ESP\n");
    fbeStatus = fbe_api_esp_common_get_resume_prom_info(pGetResumePromInfo);
    
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Step 2: Verifying Resume Prom Info from ESP\n");
    MUT_ASSERT_TRUE(pGetResumePromInfo->op_status == expectedStatus);
    
    mut_printf(MUT_LOG_LOW, "Step 3: Verified Resume Info\n");

    return;
}

void vayu_test_all_device_resume_prom_read(void)
{
    fbe_u64_t                             deviceType;
    fbe_device_physical_location_t                location= {0};
    fbe_resume_prom_status_t                      expectedStatus;

    mut_printf(MUT_LOG_LOW, "*** Verifying SPA resume read ***\n");

    deviceType = FBE_DEVICE_TYPE_SP;
    location.sp = 0;
    location.slot = 0;
    expectedStatus = FBE_RESUME_PROM_STATUS_READ_SUCCESS;
    vayu_test_resume_prom_read(deviceType, &location, expectedStatus);

    mut_printf(MUT_LOG_LOW, "*** Verifying SPB resume read ***\n");

    deviceType = FBE_DEVICE_TYPE_SP;
    location.sp = 1;
    location.slot = 0;
    expectedStatus = FBE_RESUME_PROM_STATUS_READ_SUCCESS;
    vayu_test_resume_prom_read(deviceType, &location, expectedStatus);

#if 0 // need to start sep with this configuration, currently drives are not enabling properly.
    mut_printf(MUT_LOG_LOW, "*** Verifying SPA IO Module 0 resume read ***\n");

    deviceType = FBE_DEVICE_TYPE_IOMODULE;
    location.sp = 0;
    location.slot = 0;
    expectedStatus = FBE_RESUME_PROM_STATUS_READ_SUCCESS;
    vayu_test_resume_prom_read(deviceType, &location, expectedStatus);

    mut_printf(MUT_LOG_LOW, "*** Verifying SPB IO Module 0 resume read ***\n");

    deviceType = FBE_DEVICE_TYPE_IOMODULE;
    location.sp = 1;
    location.slot = 0;
    expectedStatus = FBE_RESUME_PROM_STATUS_READ_SUCCESS;
    vayu_test_resume_prom_read(deviceType, &location, expectedStatus);

    mut_printf(MUT_LOG_LOW, "*** Verifying SPA Management Module 0 resume read ***\n");

    deviceType = FBE_DEVICE_TYPE_MGMT_MODULE;
    location.sp = 0;
    location.slot = 0;
    expectedStatus = FBE_RESUME_PROM_STATUS_READ_SUCCESS;
    vayu_test_resume_prom_read(deviceType, &location, expectedStatus);

    mut_printf(MUT_LOG_LOW, "*** Verifying SPB Management Module 0 resume read ***\n");

    deviceType = FBE_DEVICE_TYPE_MGMT_MODULE;
    location.sp = 1;
    location.slot = 0;
    expectedStatus = FBE_RESUME_PROM_STATUS_READ_SUCCESS;
    vayu_test_resume_prom_read(deviceType, &location, expectedStatus);
#endif
    mut_printf(MUT_LOG_LOW, "*** Verifying PE midplane resume read ***\n");


    deviceType = FBE_DEVICE_TYPE_ENCLOSURE;
    location.sp = 0;
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;
    expectedStatus = FBE_RESUME_PROM_STATUS_READ_SUCCESS;
    vayu_test_resume_prom_read(deviceType, &location, expectedStatus);
#if 0 // disabled because test now reads all from various dae types
    mut_printf(MUT_LOG_LOW, "*** Verifying Bus 0 Encl 0 LCCA resume read ***\n");

    deviceType = FBE_DEVICE_TYPE_LCC;
    location.sp = 0;
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;
    expectedStatus = FBE_RESUME_PROM_STATUS_READ_SUCCESS;
    vayu_test_resume_prom_read(deviceType, &location, expectedStatus);

    mut_printf(MUT_LOG_LOW, "*** Verifying Bus 0 Encl 0 LCCB resume read ***\n");

    deviceType = FBE_DEVICE_TYPE_LCC;
    location.sp = 0;
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 1;
    expectedStatus = FBE_RESUME_PROM_STATUS_READ_SUCCESS;
    vayu_test_resume_prom_read(deviceType, &location, expectedStatus);

    mut_printf(MUT_LOG_LOW, "*** Verifying Bus 0 Encl 0 PSA resume read ***\n");

    deviceType = FBE_DEVICE_TYPE_PS;
    location.sp = 0;
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;
    expectedStatus = FBE_RESUME_PROM_STATUS_READ_SUCCESS;
    vayu_test_resume_prom_read(deviceType, &location, expectedStatus);

    mut_printf(MUT_LOG_LOW, "*** Verifying Bus 0 Encl 0 PSB resume read ***\n");

    deviceType = FBE_DEVICE_TYPE_PS;
    location.sp = 0;
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 1;
    expectedStatus = FBE_RESUME_PROM_STATUS_READ_SUCCESS;
    vayu_test_resume_prom_read(deviceType, &location, expectedStatus);
#endif
    return;
}
/*!**************************************************************
 * vayu_test_resume_prom_write() 
 ****************************************************************
 * @brief
 *  Write to the sp midplane, read the data back and compare.
 ****************************************************************/
void vayu_test_resume_prom_write(void)
{
    fbe_resume_prom_cmd_t                         writeResume = {0};
    char data_buf[] = "Test fbe_api write resume sync interface";
    fbe_esp_base_env_get_resume_prom_info_cmd_t * pGetResumePromInfo = NULL;
    fbe_status_t                                  fbeStatus = FBE_STATUS_OK;
    fbe_u8_t                                      is_equal;

    mut_printf(MUT_LOG_LOW, "*** Verifying PE Midplane Resume Write ***\n");

    writeResume.deviceType = FBE_DEVICE_TYPE_ENCLOSURE;
    writeResume.deviceLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
    writeResume.deviceLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    writeResume.deviceLocation.slot = 0;
    writeResume.pBuffer = data_buf;
    writeResume.bufferSize = sizeof(data_buf);
    writeResume.resumePromField = RESUME_PROM_PRODUCT_PN;
    writeResume.offset = 0;
    writeResume.readOpStatus = FBE_RESUME_PROM_STATUS_INVALID;

    // Write resume prom.
    fbe_api_esp_common_write_resume_prom(&writeResume);

    // Sleep 15 seconds for resume prom write and ESP read complete.
    // After the write, the resume prom saved in ESP will be updated.
    fbe_api_sleep(15000);

    /* Allocate memory for pGetResumeProm */
    pGetResumePromInfo = (fbe_esp_base_env_get_resume_prom_info_cmd_t *) malloc (sizeof (fbe_esp_base_env_get_resume_prom_info_cmd_t));
    if(pGetResumePromInfo == NULL)
    {
        mut_printf(MUT_LOG_LOW, "%s memory alloc failed.",__FUNCTION__);
        return;
    }

    // Read the resume prom to verify whether the write is done correctly.
    memset(pGetResumePromInfo, 0, sizeof (fbe_esp_base_env_get_resume_prom_info_cmd_t));

    pGetResumePromInfo->deviceType = FBE_DEVICE_TYPE_ENCLOSURE;
    pGetResumePromInfo->deviceLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
    pGetResumePromInfo->deviceLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    pGetResumePromInfo->deviceLocation.slot = 0;
    
    fbeStatus = fbe_api_esp_common_get_resume_prom_info(pGetResumePromInfo);
    
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    MUT_ASSERT_TRUE(pGetResumePromInfo->op_status == FBE_RESUME_PROM_STATUS_READ_SUCCESS);

    is_equal = memcmp(data_buf, 
                      &pGetResumePromInfo->resume_prom_data.product_part_number, 
                      sizeof(data_buf));

    MUT_ASSERT_INT_EQUAL(0, is_equal);

    return;
}

/*!**************************************************************
 * vayu_test_set_data_read_resume_prom() 
 ****************************************************************
 * @brief
 *  Set up the terminator data for a specified resume prom, read it 
 *  back from ESP and compare. The target device type and id are 
 *  specified as inputs.
 ****************************************************************/
void vayu_test_set_data_read_resume_prom(fbe_device_physical_location_t *pLocation,
                                         fbe_u64_t fbe_device_type, 
                                         terminator_eses_resume_prom_id_t fbe_rp_id)
{
    RESUME_PROM_STRUCTURE                       *write_buf_p;
    fbe_api_terminator_device_handle_t          enclosure_handle;
    fbe_esp_base_env_get_resume_prom_info_cmd_t *pGetResumePromInfo = NULL;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_u8_t                                    is_equal;
    char                                        data_buf_p[] = "Test resume prom force read";
    fbe_u32_t                                   checksum = 0;

    mut_printf(MUT_LOG_LOW, "** Verify Resume Prom Read (rp:%d_%d_%d id:%d)",pLocation->bus,pLocation->enclosure,pLocation->slot,fbe_rp_id);
    /*******************    Terminator fills resume prom data ******************/
    write_buf_p = malloc(sizeof(RESUME_PROM_STRUCTURE));
    if (write_buf_p == NULL)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Not enough memory to allocate for write_buf_p", __FUNCTION__);
        return;
    }    

    // set the resume prom content to the resume prom id, this way
    // the data will change with each resume prom
    memset(write_buf_p, (int)fbe_rp_id, sizeof(RESUME_PROM_STRUCTURE));
    memcpy(write_buf_p, data_buf_p, sizeof(data_buf_p));

    checksum = calculateResumeChecksum(write_buf_p);

    write_buf_p->resume_prom_checksum = checksum;
    status = fbe_api_terminator_get_enclosure_handle(0, pLocation->enclosure, &enclosure_handle);
    
    fbe_api_terminator_set_resume_prom_info(enclosure_handle,
                                            fbe_rp_id,                      // Resume Prom ID to read
                                            (fbe_u8_t *)write_buf_p,
                                            sizeof(RESUME_PROM_STRUCTURE)); // Response buffer size

    /*******************    Issue resume prom force read ******************/
    mut_printf(MUT_LOG_LOW, "Initiate resume prom read.");

    fbe_api_esp_common_initiate_resume_prom_read(fbe_device_type, pLocation);

    /* Wait util there is no resume prom read in progress. */
    status = fbe_test_esp_wait_for_no_resume_prom_read_in_progress(15000);

    //MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for resume prom read to complete failed!");
    //ENCL_CLEANUP - The Naga enclosure doesn't have a proper config page for the resume data it needs to be updated.

    /*******************    Read resume prom to verify the force read ******************/
    mut_printf(MUT_LOG_LOW, "Get resume prom info");

    /* Allocate memory for pGetResumeProm */
    pGetResumePromInfo = (fbe_esp_base_env_get_resume_prom_info_cmd_t *) malloc (sizeof (fbe_esp_base_env_get_resume_prom_info_cmd_t));
    if(pGetResumePromInfo == NULL)
    {
        mut_printf(MUT_LOG_LOW, "%s memory alloc failed.",__FUNCTION__);
        return;
    }

    // Read the resume prom and compare to the write
    memset(pGetResumePromInfo, 0, sizeof (fbe_esp_base_env_get_resume_prom_info_cmd_t));
    pGetResumePromInfo->deviceType = fbe_device_type;
    pGetResumePromInfo->deviceLocation = *pLocation;

    status = fbe_api_esp_common_get_resume_prom_info(pGetResumePromInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(pGetResumePromInfo->op_status == FBE_RESUME_PROM_STATUS_READ_SUCCESS);


    //we will skip the prog_details and num_prog, esp will update fup rev into it and this will cause different value.
    fbe_copy_memory(&write_buf_p->prog_details[0],
                    &pGetResumePromInfo->resume_prom_data.prog_details[0],
                    RESUME_PROM_MAX_PROG_COUNT * sizeof(RESUME_PROM_PROG_DETAILS));

    fbe_copy_memory(write_buf_p->num_prog,
                    pGetResumePromInfo->resume_prom_data.num_prog,
                    RESUME_PROM_NUM_PROG_SIZE);

    is_equal = memcmp(write_buf_p, 
                      &pGetResumePromInfo->resume_prom_data, 
                      sizeof(RESUME_PROM_STRUCTURE));
    MUT_ASSERT_INT_EQUAL(0, is_equal);
    
    mut_printf(MUT_LOG_LOW, "Info Verified");
    return;
} //vayu_test_set_data_read_resume_prom

/*!**************************************************************
 * vayu_test_dae_read_all_resume_prom() 
 ****************************************************************
 * @brief
 *  Read the resume prom for each device in the specified enclosure.
 *  The device count is set according to the enclosure type.
 ****************************************************************/
void vayu_test_dae_read_all_resume_prom(fbe_u8_t enclIndex, fbe_enclosure_types_t enclType)
{
    fbe_u8_t    count;
    fbe_u8_t    fanCount = 0;
    fbe_device_physical_location_t device_location = {0};
    
    if (enclType == FBE_ENCL_SAS_VOYAGER_ICM)
    {
        fanCount = 3;
    }

    // set the enclosure location index
    device_location.enclosure = enclIndex;

    /* Viking PS resume has the special format. Need to update the test for it. */
    if ( (enclType != FBE_ENCL_SAS_VIKING_IOSXP) &&
         (enclType != FBE_ENCL_SAS_NAGA_IOSXP) &&
         (enclType != FBE_ENCL_SAS_NAGA_DRVSXP) )
    {
        // for each power supply
        for( count = 0; count < 2; count++)
        {
            // read the resume prom
            device_location.slot = count;
            vayu_test_set_data_read_resume_prom(&device_location,
                                                FBE_DEVICE_TYPE_PS,
                                                PS_A_RESUME_PROM + count);
        }
    
    
        /* Viking Fan does not have the resume prom. Need to update the test for it. */
        // for each fan
        for(count=0; count<fanCount; count++)
        {
            device_location.slot = count;
            // read the resume prom
            vayu_test_set_data_read_resume_prom(&device_location,
                                                FBE_DEVICE_TYPE_FAN, 
                                                FAN_1_RESUME_PROM + count);
        }
    }
    
    // for each lcc
    if (enclType == FBE_ENCL_SAS_VOYAGER_ICM)
    {
        // voyager has 4 lccs, 0/2 are local, 1/3 are peer
        // read the resume prom
        device_location.slot = 0;
        vayu_test_set_data_read_resume_prom(&device_location,
                                            FBE_DEVICE_TYPE_LCC, 
                                            LCC_A_RESUME_PROM);
        device_location.slot = 1;
        vayu_test_set_data_read_resume_prom(&device_location,
                                            FBE_DEVICE_TYPE_LCC, 
                                            LCC_B_RESUME_PROM);
        device_location.slot = 2;
        vayu_test_set_data_read_resume_prom(&device_location,
                                            FBE_DEVICE_TYPE_LCC, 
                                            EE_LCC_A_RESUME_PROM);
        device_location.slot = 3;
        vayu_test_set_data_read_resume_prom(&device_location,
                                            FBE_DEVICE_TYPE_LCC, 
                                            EE_LCC_B_RESUME_PROM);
    }
    else
    {
        // read the resume prom
        device_location.slot = 0;
        vayu_test_set_data_read_resume_prom(&device_location,
                                            FBE_DEVICE_TYPE_LCC, 
                                            LCC_A_RESUME_PROM);
        device_location.slot = 1;
        vayu_test_set_data_read_resume_prom(&device_location,
                                            FBE_DEVICE_TYPE_LCC, 
                                            LCC_B_RESUME_PROM);
    }
    return;
} //vayu_test_dae_read_all_resume_prom

/*!**************************************************************
 * vayu_test_all_dae_resume_read() 
 ****************************************************************
 * @brief
 *  For each enclosure, read all the resume proms.
 ****************************************************************/
void vayu_test_all_dae_resume_read(void)
{
    fbe_status_t                status;
    fbe_enclosure_types_t       enclType;
    fbe_u32_t                   enclCount;
    fbe_u32_t                   enclIndex;
    //fbe_object_id_t             objectId;
    fbe_object_id_t             object_id_list[500]; // only need to support small config (max is 2000 obj)

    memset(object_id_list, 0, 500);
    // the board object is at [0], the other object ids in the list are for DPE or DAE enclosures.
    status = fbe_api_get_all_enclosure_object_ids(&object_id_list[0], FBE_MAX_PHYSICAL_OBJECTS-1, &enclCount);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    // loop through all enclosures, read all resume proms on each encl
    for(enclIndex = 0; enclIndex < enclCount; enclIndex ++) 
    {
        // get the enclosure type
        status = fbe_api_enclosure_get_encl_type(object_id_list[enclIndex],
                                                 &enclType);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        // only test viper and voyager type enclosures
        // since only voyager has different resume proms
        if ((enclType == FBE_ENCL_SAS_VIPER) || (enclType == FBE_ENCL_SAS_VOYAGER_ICM))
        {
            // read all the resume proms in this enclosure
            vayu_test_dae_read_all_resume_prom(enclIndex, enclType);
        }
    }
    return;
} //vayu_test_all_dae_resume_read

/*!**************************************************************
 * vayu_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   24-Jan-2011 - Created. Arun S
 *   21-Jul-2011 PHE - updated.
 *
 ****************************************************************/
void vayu_test(void)
{
    fbe_status_t status;

    /* Wait util there is no resume prom read in progress. */
    status = fbe_test_esp_wait_for_no_resume_prom_read_in_progress(30000);
    // ENCL_CLEANUP - Power Supply Resume isn't correct in Naga Config Page, This needs updating for this test to work
    //MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for resume prom read to complete failed!");

    // Read SP resume proms
    vayu_test_all_device_resume_prom_read();

    // write to midplane resume prom
    vayu_test_resume_prom_write();

    // read all resume proms in each dae
    vayu_test_all_dae_resume_read();

    mut_printf(MUT_LOG_LOW, "*** Vayu test passed. ***\n");

    return;
}
/******************************************
 * end vayu_test()
 ******************************************/


/*!**************************************************************
 * vayu_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   24-Jan-2011 - Created. Arun S
 *
 ****************************************************************/
void vayu_setup(void)
{
    fbe_test_startEsp_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_MIXED_CONIG,
                                        SPID_UNKNOWN_HW_TYPE,
                                        NULL,
                                        NULL);
}
/**************************************
 * end vayu_setup()
 **************************************/

/*!**************************************************************
 * vayu_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the Vayu test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   24-Jan-2011 - Created. Arun S
 *
 ****************************************************************/

void vayu_destroy(void)
{
    fbe_test_esp_common_destroy();
    fbe_test_esp_delete_registry_file();
    return;
}
/******************************************
 * end vayu_destroy()
 ******************************************/



/*************************
 * end file vayu_test.c
 *************************/


