/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * wallis_pond.c (Enclosure Firmware download)
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the Wallis Pond scenario that simulates the enclosure
 *  receiving image file from flare shim and sending it down to expander and 
 *  accepting update command from flare shim and sending it down.
 * 
 *  This scenario is for a 1 port, 1 enclosure configuration, aka the maui.xml
 *  configuration.
 * 
 *  The test run in this case is:
 *  1) Load and verify the configuration. (1-Port 1-Enclosure)
 *  2) Read image file.
 *  3) Issue firmware download command to usurper.
 *  4) Check the download status.
 *  5) Send the update command.
 *  6) Logout everything (enclosure and its drive).
 *  7) Verify the drive and the enclosure state.
 *  8) Log everything (enclosure and its drive) back in 10 seconds.
 *  9) Verify the drive and the enclosure state.
 * 10) Shutdown the Terminator/Physical package.
 *
 * HISTORY
 *   08/26/2008:  Created. ArunKumar Selvapillai
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_topology_interface.h"


typedef struct fbe_test_wp_params_s
{
    fbe_enclosure_fw_target_t    target; 
    fbe_u8_t                    *targetDesc;
}fbe_test_wp_params_t;
    

fbe_test_wp_params_t wp_test_table[] = 
{  
    {FBE_FW_TARGET_LCC_MAIN,
     "LCC MAIN UPGRADE TEST",
    },
#if 0
    {FBE_FW_TARGET_SPS_PRIMARY,
     "SPS PRIMARY UPGRADE TEST",
    },
#endif
};
    
/*************************
 *   FUNCTION DEFINITIONS
 *************************/
fbe_u32_t fbe_test_get_num_of_wp_tests(void)
{
     return  (sizeof(wp_test_table))/(sizeof(wp_test_table[0]));
}

/*!***************************************************************
 * @fn wp_test_firmware_send_download_and_activate()
 ****************************************************************
 * @brief
 *  Function to get the ESES info related to the target. 
 *
 * @return
 *  FBE_STATUS_OK if no errors.
 *
 * @version
 *  28-Sep-2012 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t wp_get_taget_eses_info(fbe_enclosure_fw_target_t target, 
                            ses_subencl_type_enum * pSubEnclType,
                            ses_comp_type_enum  * pCompType)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(target) 
    {
        case FBE_FW_TARGET_LCC_MAIN:
            *pSubEnclType = SES_SUBENCL_TYPE_LCC;
            *pCompType = SES_COMP_TYPE_LCC_MAIN;
            break;
        case FBE_FW_TARGET_SPS_PRIMARY:
            *pSubEnclType = SES_SUBENCL_TYPE_UPS;
            *pCompType = SES_COMP_TYPE_SPS_FW;
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}

/*!***************************************************************
 * @fn wp_test_firmware_send_download_and_activate()
 ****************************************************************
 * @brief
 *  Function to send the enclosure firmware download and activate command. 
 *
 * @return
 *  FBE_STATUS_OK if no errors.
 *
 * @version
 *  29-June-2010 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t wp_test_firmware_send_download_and_activate(fbe_u32_t port, fbe_u32_t encl,
                                              fbe_u8_t *image_p, fbe_u32_t image_size, 
                                              fbe_enclosure_fw_target_t target,
                                              fbe_u32_t activate_time_interval_in_ms,
                                              fbe_u32_t reset_time_interval_in_ms,
                                              fbe_bool_t update_rev)
{
    fbe_status_t    status;
    fbe_u32_t       fw_handle;
    fbe_download_status_desc_t          download_stat_desc;
    fbe_enclosure_mgmt_download_op_t    firmware_download;
    fbe_enclosure_mgmt_download_op_t    firmware_activate;
    fbe_api_terminator_device_handle_t  encl_handle;   

    // init the status struct
    fbe_zero_memory(&firmware_download, sizeof(fbe_enclosure_mgmt_download_op_t));
    fbe_zero_memory(&firmware_activate, sizeof(fbe_enclosure_mgmt_download_op_t));

    status = fbe_api_get_enclosure_object_id_by_location(port, encl, &fw_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(fw_handle != FBE_OBJECT_ID_INVALID);

    status = fbe_api_terminator_get_enclosure_handle(port, encl, &encl_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Set up test environment...");

    /* Set up the upgrade environment. */
    /* Set active time interval before the real activating. */
    status = fbe_api_terminator_set_enclosure_firmware_activate_time_interval(encl_handle, activate_time_interval_in_ms);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set reset time interval before reset to create the activate timeout error. */
    status = fbe_api_terminator_set_enclosure_firmware_reset_time_interval(encl_handle, reset_time_interval_in_ms);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set upgrade rev or not. */
    status = fbe_api_terminator_set_need_update_enclosure_firmware_rev(update_rev);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Sending download...");

    /* Issue download command to usurper */
    firmware_download.op_code = FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD;
    firmware_download.size = image_size;
    firmware_download.image_p = image_p;
    firmware_download.target = target;
    if (FBE_SIM_SP_B == fbe_api_sim_transport_get_target_server())
    {
        firmware_download.side_id = 1;
    }
    else
    {
        firmware_download.side_id = 0;
    }
    status = fbe_api_enclosure_firmware_download(fw_handle, &firmware_download);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* The physical package does not poll the download status from the expander after
     * sending the download ctrl page. 
     * So no need to change the download status in the terminator. 
     */
    /* Wait for download status to change to FBE_ENCLOSURE_FW_STATUS_NONE.*/
    status = fbe_api_wait_for_encl_firmware_download_status(fw_handle, 
                              target,
                              FBE_ENCLOSURE_FW_STATUS_NONE, 
                              FBE_ENCLOSURE_FW_EXT_STATUS_IMAGE_LOADED,
                              30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
        "Wait for FBE_ENCLOSURE_FW_STATUS_NONE and FBE_ENCLOSURE_FW_EXT_STATUS_IMAGE_LOADED failed!");

    /* Generate unit attention */
    //status = fbe_api_terminator_eses_set_unit_attention(encl_handle);
    //MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Sending activate...");
    // Issue activate image command
    firmware_activate.op_code = FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE;
    firmware_activate.size = 0;
    firmware_activate.image_p = NULL;
    firmware_activate.target = target;
    if (FBE_SIM_SP_B == fbe_api_sim_transport_get_target_server())
    {
        firmware_activate.side_id = 1;
    }
    else
    {
        firmware_activate.side_id = 0;
    }
    status = fbe_api_enclosure_firmware_update(fw_handle, &firmware_activate);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // set status in the download ucode status desc -IN_PROGRESS
    fbe_zero_memory(&download_stat_desc, sizeof(fbe_download_status_desc_t));
    download_stat_desc.status = ESES_DOWNLOAD_STATUS_IN_PROGRESS;
    download_stat_desc.addl_status = ESES_DOWNLOAD_STATUS_NONE;
    status = fbe_api_terminator_eses_set_download_microcode_stat_page_stat_desc(encl_handle, download_stat_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
 
    status = fbe_api_wait_for_encl_firmware_download_status(fw_handle, 
                              target,
                              FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS, 
                              FBE_ENCLOSURE_FW_EXT_STATUS_REQ,
                              30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
         "Wait for FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS and FBE_ENCLOSURE_FW_EXT_STATUS_REQ failed!");

    return FBE_STATUS_OK;
}


/****************************************************************
 * wp_test_firmware_checksum_error()
 ****************************************************************
 * Description:
 *  Function to test the enclosure firmware download checksum error.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  09/09/2008 - Created. Arunkumar Selvapillai
 *  29-June-2010: PHE - Used fbe_api_wait_for_encl_firmware_download_status
 *
 ****************************************************************/
static fbe_status_t wp_test_firmware_checksum_error(fbe_u32_t port, 
                                                    fbe_u32_t encl, 
                                                    fbe_enclosure_fw_target_t target)
{
    fbe_status_t    status;
    fbe_object_id_t fw_handle;
    fbe_u32_t       image_size;
    fbe_u8_t        *image_p;
    fbe_download_status_desc_t          download_stat_desc;
    fbe_api_terminator_device_handle_t  encl_handle;
    ses_subencl_type_enum subEnclType = SES_SUBENCL_TYPE_INVALID;
    ses_comp_type_enum  compType = SES_COMP_TYPE_OTHER_FW;

    mut_printf(MUT_LOG_LOW, "== Wallis Pond: Test checksum error ==");

    status = wp_get_taget_eses_info(target, &subEnclType, &compType);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set up the image info. */
    image_size = 20000;  /* something big */
    image_p = (fbe_u8_t *)malloc(image_size * sizeof(fbe_u8_t));
    /* The terminator will check the component type. So it needs to be set here. */
    image_p[FBE_ESES_MCODE_IMAGE_COMPONENT_TYPE_OFFSET] = compType;
    /* we don't care what's in the image right now */

    status = fbe_api_get_enclosure_object_id_by_location(port, encl, &fw_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(fw_handle != FBE_OBJECT_ID_INVALID);

    status = fbe_api_terminator_get_enclosure_handle(port, encl, &encl_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set up the upgrade test environment.
     * 20 seconds before the real activating. 
     * 20 seconds before reset. 
     * set update rev to FBE_TRUE. 
     * And send download and activate commands.     
     */                                        
    wp_test_firmware_send_download_and_activate(port, encl, image_p, image_size, target,
                                                20000, 20000, FBE_TRUE);     

    // set status in the terminator download ucode status desc - ERROR_CHECKSUM
    fbe_zero_memory(&download_stat_desc, sizeof(fbe_download_status_desc_t));
    download_stat_desc.status = ESES_DOWNLOAD_STATUS_ERROR_CHECKSUM;
    download_stat_desc.addl_status = ESES_DOWNLOAD_STATUS_NONE;
    status = fbe_api_terminator_get_enclosure_handle(port, encl, &encl_handle); 
    status = fbe_api_terminator_eses_set_download_microcode_stat_page_stat_desc(encl_handle, download_stat_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Verify download status for checksum error");
 
    status = fbe_api_wait_for_encl_firmware_download_status(fw_handle, 
                              target,
                              FBE_ENCLOSURE_FW_STATUS_ACTIVATE_FAILED, 
                              FBE_ENCLOSURE_FW_EXT_STATUS_ERR_CHECKSUM,
                              30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
         "Wait for FBE_ENCLOSURE_FW_STATUS_ACTIVATE_FAILED and FBE_ENCLOSURE_FW_EXT_STATUS_ERR_CHECKSUM failed!");

    /* release memory */
    free(image_p);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn wp_test_firmware_activate_timeout_error()
 ****************************************************************
 * @brief
 *  Function to test the enclosure firmware activate timeout error.
 *
 * @return - FBE_STATUS_OK if no errors.
 *
 * @version
 *  09/09/2008 - Created. Arunkumar Selvapillai
 *  29-June-2010: PHE - Used fbe_api_wait_for_encl_firmware_download_status
 *
 ****************************************************************/
static fbe_status_t wp_test_firmware_activate_timeout_error(fbe_u32_t port, 
                                                            fbe_u32_t encl,
                                                            fbe_enclosure_fw_target_t target)
{
    fbe_status_t    status;
    fbe_object_id_t fw_handle;
    fbe_u32_t       image_size;
    fbe_u8_t        *image_p;
    ses_subencl_type_enum subEnclType = SES_SUBENCL_TYPE_INVALID;
    ses_comp_type_enum  compType = SES_COMP_TYPE_OTHER_FW;
 
    mut_printf(MUT_LOG_LOW, "== Wallis Pond: Test activate timeout error ==");
    status = wp_get_taget_eses_info(target, &subEnclType, &compType);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set up the image info. */
    image_size = 20000;  /* something big */
    image_p = (fbe_u8_t *)malloc(image_size * sizeof(fbe_u8_t));
    /* The terminator will check the component type. So it needs to be set here. */
    image_p[FBE_ESES_MCODE_IMAGE_COMPONENT_TYPE_OFFSET] = SES_COMP_TYPE_LCC_MAIN;
    /* we don't care what's in the image right now */

    status = fbe_api_get_enclosure_object_id_by_location(port, encl, &fw_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(fw_handle != FBE_OBJECT_ID_INVALID);

    /* Set up the upgrade test environment.
     * Set activate time interval to 20 seconds and reset time interval to 40 seconds
     * to create the activation timeout(total 60 seconds) for LCC. 
     * set update rev to FBE_TRUE. 
     * And send download and activate commands.     
     */   

    wp_test_firmware_send_download_and_activate(port, encl, image_p, image_size, target,
                                                20000, 40000, FBE_TRUE);      
        
    mut_printf(MUT_LOG_LOW, "Wait for max 50s to verify download status for timeout error");

    /* Wait for the download status FBE_ENCLOSURE_FW_STATUS_ACTIVATE_FAILED. */
    status = fbe_api_wait_for_encl_firmware_download_status(fw_handle, 
                              target,
                              FBE_ENCLOSURE_FW_STATUS_ACTIVATE_FAILED,
                              FBE_ENCLOSURE_FW_EXT_STATUS_ERR_TO,
                              50000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
        "Wait for FBE_ENCLOSURE_FW_STATUS_ACTIVATE_FAILED and FBE_ENCLOSURE_FW_EXT_STATUS_ERR_TO failed!");    

    /* release memory */
    free(image_p);

    return FBE_STATUS_OK;
}

/****************************************************************
 * wp_test_firmware_upgrade_success()
 ****************************************************************
 * Description:
 *  Function to test the successful enclosure firmware upgrade. 
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  09/09/2008 - Created. Arunkumar Selvapillai
 *  29-June-2010: PHE - Used fbe_api_wait_for_encl_firmware_download_status
 *
 ****************************************************************/
static fbe_status_t wp_test_firmware_upgrade_success(fbe_u32_t port, 
                                                     fbe_u32_t encl,
                                                     fbe_enclosure_fw_target_t target)
{
    fbe_status_t    status;
    fbe_u32_t       fw_handle;
    fbe_u32_t       image_size;
    fbe_u8_t        *image_p;
  //  fbe_download_status_desc_t          download_stat_desc;
  //  ses_ver_desc_struct                 ver_desc;
    fbe_api_terminator_device_handle_t  encl_handle;
    ses_subencl_type_enum subEnclType = SES_SUBENCL_TYPE_INVALID;
    ses_comp_type_enum  compType = SES_COMP_TYPE_OTHER_FW;

    mut_printf(MUT_LOG_LOW, "== Wallis Pond: Test successful upgrade ==");
    status = wp_get_taget_eses_info(target, &subEnclType, &compType);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set up the image info. */
    image_size = 20000;  /* something big */
    image_p = (fbe_u8_t *)malloc(image_size * sizeof(fbe_u8_t));
    /* The terminator will check the component type. So it needs to be set here. */
    image_p[FBE_ESES_MCODE_IMAGE_COMPONENT_TYPE_OFFSET] = compType;
    /* we don't care what's in the image right now */

    status = fbe_api_get_enclosure_object_id_by_location(port, encl, &fw_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(fw_handle != FBE_OBJECT_ID_INVALID);
    
    status = fbe_api_terminator_get_enclosure_handle(port, encl, &encl_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set up the upgrade test environment.
     * 10 seconds before the real activating. 
     * 10 seconds before reset. 
     * set update rev to FBE_TRUE. 
     * And send download and activate commands.     
     */                                        
    wp_test_firmware_send_download_and_activate(port, encl, image_p, image_size, target,
                                                10000, 10000, FBE_TRUE); 

    #if 0
    mut_printf(MUT_LOG_LOW, "Verify download status when updating nonvol");
    // set status in the download ucode status desc-UPDATING_NONVOL
    fbe_zero_memory(&download_stat_desc, sizeof(fbe_download_status_desc_t));
    download_stat_desc.status = ESES_DOWNLOAD_STATUS_UPDATING_NONVOL;
    download_stat_desc.addl_status = ESES_DOWNLOAD_STATUS_NONE;
    status = fbe_api_terminator_eses_set_download_microcode_stat_page_stat_desc(encl_handle, download_stat_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Wait for the download status FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS. */
    status = fbe_api_wait_for_encl_firmware_download_status(fw_handle, 
                              target,
                              FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS,
                              FBE_ENCLOSURE_FW_EXT_STATUS_REQ,
                              30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
        "Wait for FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS and FBE_ENCLOSURE_FW_EXT_STATUS_NONE failed!");

    mut_printf(MUT_LOG_LOW, "Verify download status when updating flash");
    // set status in the download ucode status desc-UPDATING_FLASH
    fbe_zero_memory(&download_stat_desc, sizeof(fbe_download_status_desc_t));
    download_stat_desc.status = ESES_DOWNLOAD_STATUS_UPDATING_FLASH;
    download_stat_desc.addl_status = ESES_DOWNLOAD_STATUS_NONE;
    status = fbe_api_terminator_eses_set_download_microcode_stat_page_stat_desc(encl_handle, download_stat_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Wait for the download status FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS. */
    status = fbe_api_wait_for_encl_firmware_download_status(fw_handle,
                              target, 
                              FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS,
                              FBE_ENCLOSURE_FW_EXT_STATUS_REQ,
                              30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
        "Wait for FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS and FBE_ENCLOSURE_FW_EXT_STATUS_REQ failed!");    
 
    // get a copy of the main vers desc in the terminator.
    status = fbe_api_terminator_eses_get_ver_desc(encl_handle,
                                                    subEnclType,
                                                    0,
                                                    (fbe_u8_t) compType,
                                                    &ver_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Oldrev %.5s", &ver_desc.comp_rev_level[0]);

    // change the firmware rev in the version desc            
    ver_desc.comp_rev_level[3] += 1;
    status = fbe_api_terminator_eses_set_ver_desc(encl_handle,
                                                  subEnclType,
                                                  0,
                                                  (fbe_u8_t) compType,
                                                  ver_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Newrev %.5s", &ver_desc.comp_rev_level[0]);

    /* Increment the generation code */
    status = fbe_api_terminator_eses_increment_config_page_gen_code(encl_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    /* Generate unit attention */
    status = fbe_api_terminator_eses_set_unit_attention(encl_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
#endif
    // clear the error status from the ucode download status desc
    mut_printf(MUT_LOG_LOW, "Verify download status on successful completion ==");
    
    /* Wait for the download status FBE_ENCLOSURE_FW_STATUS_NONE. */
    status = fbe_api_wait_for_encl_firmware_download_status(fw_handle, 
                              target,
                              FBE_ENCLOSURE_FW_STATUS_NONE,
                              FBE_ENCLOSURE_FW_EXT_STATUS_NONE,
                              30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
        "Wait for FBE_ENCLOSURE_FW_STATUS_NONE and FBE_ENCLOSURE_FW_EXT_STATUS_NONE failed!");    

    /* release memory */
    free(image_p);

    return FBE_STATUS_OK;
}


/****************************************************************
 * wp_run()
 ****************************************************************
 * Description:
 *  Function to simulate the enclosure firmware download 
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  08/26/2008 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/

static fbe_status_t wp_run(void)
{
    fbe_status_t    wp_status = FBE_STATUS_OK;
    fbe_u32_t       port = 0;
    fbe_u32_t       encl = 0;
    fbe_u32_t       maxEntries = 0;
    fbe_u32_t       index = 0;

    mut_printf(MUT_LOG_LOW, "Wallis Pond Scenario: Start %s", __FUNCTION__);

    maxEntries = fbe_test_get_num_of_wp_tests();
    for(index = 0; index < maxEntries; index++)
    {
 
        mut_printf(MUT_LOG_LOW, "Wallis Pond Scenario: %s", wp_test_table[index].targetDesc);
    
        wp_status = wp_test_firmware_upgrade_success(port, encl, wp_test_table[index].target);
        MUT_ASSERT_TRUE(wp_status == FBE_STATUS_OK);
    
        wp_status = wp_test_firmware_checksum_error(port, encl, wp_test_table[index].target);
        MUT_ASSERT_TRUE(wp_status == FBE_STATUS_OK);
    
        /* The code in the physical package is not able to detect the LCC activate timeout error.
         * because after the LCC starts reset, the enclosure goes to the activate state.
         * In activate state, it does not have the eses_enclosure_firmware_download_cond 
         * which checks the activate timeout or not.  - PINGHE
         */
        //wp_status = wp_test_firmware_activate_timeout_error(port, encl, target);
        //MUT_ASSERT_TRUE(wp_status == FBE_STATUS_OK);
    }

    mut_printf(MUT_LOG_LOW, "Wallis Pond Scenario: End %s", __FUNCTION__);

    return wp_status;
}


void wallis_pond_setup(void)
{
    fbe_status_t status;
    fbe_u32_t index = 0;
    fbe_u32_t maxEntries = 0;
    fbe_test_params_t * pNaxosTest = NULL;

    // setup SPB side
    mut_printf(MUT_LOG_LOW, " Loading Virtual Config on SPB ===");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);


    /* Load the terminator, the physical package with the naxos config
     * and verify the objects in the physical package.
     */
    maxEntries = fbe_test_get_naxos_num_of_tests() ;
    for(index = 0; index < maxEntries; index++)
    {
        /* Load the configuration for test */
        pNaxosTest =  fbe_test_get_naxos_test_table(index);
        if(pNaxosTest->encl_type == FBE_SAS_ENCLOSURE_TYPE_VIPER)
        {
            status = naxos_load_and_verify_table_driven(pNaxosTest);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            break;
        }
    }

    return;
}

/****************************************************************
 * wallis_pond()
 ****************************************************************
 * Description:
 *  Function to run the Wallis Pond scenario.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  08/26/2008 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/

void wallis_pond(void)
{
    fbe_status_t run_status;

    run_status = wp_run();
    MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);
}
