/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp2_sudsakorn_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify that IO module, IO annex, resume prom info is gathered in ESP.
 * 
 * @version
 *   05/26/2010 - Created. Nayana Chaudhari
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_pe_types.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_module_info.h"
#include "esp_tests.h"
#include "fbe_test_configurations.h"
#include "fp_test.h"
#include "fbe_test_esp_utils.h"
#include "sep_tests.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static void fp2_sudsakorn_load_specl_config(void);

char * fp2_sudsakorn_short_desc = "Verify the Gathering of IO Module, IO annex and resume prom information in ESP";
char * fp2_sudsakorn_long_desc ="\
Sud Sakorn\n\
        - is a fictional character in Sunthorn Phu's story of Phra Aphai Mani.\n\
        - Sudsakorn, the son of a mermaid and a minstrel prince, was born at \n\
        - Ko Kaeo Pisadan without ever seeing his father. \n\
        - When he grew up he starts his adventure to seek his father.\n\
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
STEP 2: Get IO module info from physical package into management module object\n\
STEP 3: Get IO port info for that IO module from PP into management module object\n\
STEP 4: Get IO module info into ESP\n\
        - Verify that the module is inserted\n\
STEP 5: Get IO port info for that io module into ESP \n\
        - Verify that the port is present\n\
";

/*!**************************************************************
 * fp2_sudsakorn_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   05/26/2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/

void fp2_sudsakorn_test(void)
{
    fbe_status_t                                status;
    fbe_esp_module_io_module_info_t             esp_io_module_info = {0};
    fbe_esp_module_io_module_info_t             esp_bem_info = {0};
    fbe_esp_module_io_module_info_t             esp_mezzanine_info = {0};

    fbe_esp_module_io_port_info_t               esp_io_port_info = {0};
    SP_ID                                       sp;
    fbe_u32_t                                   slot;
    fbe_u32_t                                   componentCountPerBlade = 0;
    fbe_u32_t                                   IOMCountPerBlade = 0;
    fbe_board_mgmt_io_comp_info_t               ioModuleInfo = {0};
    fbe_status_t                                fbeStatus = FBE_STATUS_OK;
    fbe_object_id_t                             objectId;
    fbe_board_mgmt_io_port_info_t               ioPortInfo = {0};
    fbe_esp_module_mgmt_module_status_t         module_status = {0};


    // STEP 1 - verify the io module info
    mut_printf(MUT_LOG_LOW, "*** Verifying IO Module status ***\n");

    fbeStatus = fbe_api_get_board_object_id(&objectId);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != FBE_OBJECT_ID_INVALID);
    fbeStatus = fbe_api_board_get_io_module_count_per_blade(objectId, &componentCountPerBlade);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
   
    mut_printf(MUT_LOG_LOW, "*** Get IO Module count Successfully, Count: %d. ***\n", componentCountPerBlade * SP_ID_MAX);    

    IOMCountPerBlade = componentCountPerBlade;
    for(sp = SP_A; sp < SP_ID_MAX; sp ++)
    {
        for(slot = 0; slot < componentCountPerBlade; slot ++)
        {
            ioModuleInfo.associatedSp = sp;
            ioModuleInfo.slotNumOnBlade = slot;

            fbeStatus = fbe_api_board_get_iom_info(objectId, &ioModuleInfo);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(ioModuleInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
            MUT_ASSERT_TRUE(ioModuleInfo.inserted == FBE_MGMT_STATUS_TRUE);     
            MUT_ASSERT_TRUE(ioModuleInfo.faultLEDStatus == LED_BLINK_OFF);
            MUT_ASSERT_TRUE(ioModuleInfo.powerStatus == FBE_POWER_STATUS_POWER_ON);   
        }
    } 

    mut_printf(MUT_LOG_LOW, "*** Step 1: got IO Module Info from PP ***\n");

   // STEP -2 get the io port info for SPA, iom 0, port 3
    ioPortInfo.associatedSp = SP_A;
    ioPortInfo.slotNumOnBlade = 0;
    ioPortInfo.portNumOnModule = 3;
    ioPortInfo.deviceType = FBE_DEVICE_TYPE_IOMODULE;
    fbeStatus = fbe_api_board_get_io_port_info(objectId, &ioPortInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "*** Step 2: got IO Port Info from PP ***\n");

    //specify to get information about IO Module 0 on SP A
    esp_io_module_info.header.sp = 0;
    esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
    esp_io_module_info.header.slot = 0;

    // verify the io module info
    status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
    mut_printf(MUT_LOG_LOW, "Io module inserted 0x%x, status 0x%x\n", 
        esp_io_module_info.io_module_phy_info.inserted, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);
    
    mut_printf(MUT_LOG_LOW, "*** Step 3: got IO Module Info into ESP ***\n");
    mut_printf(MUT_LOG_LOW, "*** Step 3-1: Verified IO module info ***\n");


    // get the port info for that IO module
    esp_io_port_info.header.sp = 0;
    esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
    esp_io_port_info.header.slot = 0;
    esp_io_port_info.header.port = 1;

    status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
    mut_printf(MUT_LOG_LOW, "Io port present 0x%x, status 0x%x\n", 
        esp_io_port_info.io_port_info.present, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

    mut_printf(MUT_LOG_LOW, "*** Step 4: got IO Port Info into ESP ***\n");
    mut_printf(MUT_LOG_LOW, "*** Step 4-1: Verified IO port info ***\n");


    // get IO annex info from PP
    componentCountPerBlade = 0;
    fbeStatus = fbe_api_board_get_bem_count_per_blade(objectId, &componentCountPerBlade);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
   
    mut_printf(MUT_LOG_LOW, "*** Get IO BEM count Successfully, BEM Count: %d. ***\n", componentCountPerBlade * SP_ID_MAX);    

    /* For SENTRY_CPU_MODULE, we don't have BEM and Count will be Zero. 
     * We only need to exercise this code only if the component count is > 0
     */
    if(componentCountPerBlade > 0)
    {
        for(sp = SP_A; sp < SP_ID_MAX; sp ++)
        {
            for(slot = 0; slot < componentCountPerBlade; slot ++)
            {
                ioModuleInfo.associatedSp = sp;
                ioModuleInfo.slotNumOnBlade = slot;
    
                fbeStatus = fbe_api_board_get_bem_info(objectId, &ioModuleInfo);
                MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
                MUT_ASSERT_TRUE(ioModuleInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
                MUT_ASSERT_TRUE(ioModuleInfo.inserted == FBE_MGMT_STATUS_TRUE);     
                MUT_ASSERT_TRUE(ioModuleInfo.faultLEDStatus == LED_BLINK_OFF);
                MUT_ASSERT_TRUE(ioModuleInfo.powerStatus == FBE_POWER_STATUS_POWER_ON);   
            }
        } 
    
        mut_printf(MUT_LOG_LOW, "*** Step 5: got bem Info from PP ***\n");
    
        // verify the io annex info into ESP
        esp_bem_info.header.sp = 0;
        esp_bem_info.header.type = FBE_DEVICE_TYPE_BACK_END_MODULE;
        esp_bem_info.header.slot = 0;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_bem_info);
        mut_printf(MUT_LOG_LOW, "Io BEM inserted 0x%x, status 0x%x\n", 
        esp_bem_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_bem_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);
        
        mut_printf(MUT_LOG_LOW, "*** Step 6: Verified IO BEM info\n");
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "*** Step 6: No bem found for this CPU module\n");
    }
        // get Mezzanine info from PP
    componentCountPerBlade = 0;
    fbeStatus = fbe_api_board_get_mezzanine_count_per_blade(objectId, &componentCountPerBlade);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
   
    mut_printf(MUT_LOG_LOW, "*** Get Mezzanine count Successfully, Mezzanine Count: %d. ***\n", componentCountPerBlade * SP_ID_MAX);    

    for(sp = SP_A; sp < SP_ID_MAX; sp ++)
    {
        for(slot = 0; slot < componentCountPerBlade; slot ++)
        {
            ioModuleInfo.associatedSp = sp;
            ioModuleInfo.slotNumOnBlade = slot;

            fbeStatus = fbe_api_board_get_mezzanine_info(objectId, &ioModuleInfo);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(ioModuleInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
            MUT_ASSERT_TRUE(ioModuleInfo.inserted == FBE_MGMT_STATUS_TRUE);     
        }
    } 

    mut_printf(MUT_LOG_LOW, "*** Step 7: got Mezzanine Info from PP ***\n");

        // verify the mezzanine info into ESP
    esp_mezzanine_info.header.sp = 0;
    esp_mezzanine_info.header.type = FBE_DEVICE_TYPE_MEZZANINE;
    esp_mezzanine_info.header.slot = 0;

    status = fbe_api_esp_module_mgmt_getMezzanineInfo(&esp_mezzanine_info);
    mut_printf(MUT_LOG_LOW, "Mezzanine inserted 0x%x, status 0x%x\n", 
        esp_mezzanine_info.io_module_phy_info.inserted, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(esp_mezzanine_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);
    
    mut_printf(MUT_LOG_LOW, "*** Step 8: Verified Mezzanine info\n");

    mut_printf(MUT_LOG_LOW, "*** Step 9: check IO module protocols");

    //specify to get information about Mezzanine on SP A
    module_status.header.sp = 0;
    module_status.header.type = FBE_DEVICE_TYPE_MEZZANINE;
    module_status.header.slot = 0;

    // verify the io module info
    fbe_api_esp_module_mgmt_get_module_status(&module_status);

    MUT_ASSERT_INT_EQUAL(module_status.protocol, FBE_IO_MODULE_PROTOCOL_MULTI);

    //specify to get information about SLIC 0 on SP A
    module_status.header.sp = 0;
    module_status.header.type = FBE_DEVICE_TYPE_IOMODULE;
    module_status.header.slot = 0;

    // verify the io module info
    fbe_api_esp_module_mgmt_get_module_status(&module_status);

    MUT_ASSERT_INT_EQUAL(module_status.protocol, FBE_IO_MODULE_PROTOCOL_FCOE);

    //specify to get information about SLIC 0 on SP A
    module_status.header.sp = 0;
    module_status.header.type = FBE_DEVICE_TYPE_IOMODULE;
    module_status.header.slot = 1;

    // verify the io module info
    fbe_api_esp_module_mgmt_get_module_status(&module_status);

    MUT_ASSERT_INT_EQUAL(module_status.protocol, FBE_IO_MODULE_PROTOCOL_ISCSI);

    mut_printf(MUT_LOG_LOW, "*** Step 9: check IO module protocols passed");

    mut_printf(MUT_LOG_LOW, "*** Sudsakorn test passed. ***\n");


    return;
}
/******************************************
 * end fp2_sudsakorn_test()
 ******************************************/
/*!**************************************************************
 * fp2_sudsakorn_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   05/26/2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
void fp2_sudsakorn_setup(void)
{
    fbe_status_t status;
    fbe_test_load_fp_config(SPID_OBERON_3_HW_TYPE);

    fp2_sudsakorn_load_specl_config();

    fp_test_start_physical_package();

	/* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();

}
/**************************************
 * end fp2_sudsakorn_setup()
 **************************************/

/*!**************************************************************
 * fp2_sudsakorn_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the fp2_sudsakorn test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   05/26/2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/

void fp2_sudsakorn_destroy(void)
{
    fbe_test_esp_common_destroy_all();
    return;
}
/******************************************
 * end fp2_sudsakorn_cleanup()
 ******************************************/

static void fp2_sudsakorn_load_specl_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       slot = 0, sp = 0;
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return;
    }

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    for(sp=0;sp<2;sp++)
    {
        sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioModuleCount = TOTAL_IO_MOD_PER_BLADE;
        for(slot = 0; slot < TOTAL_IO_MOD_PER_BLADE; slot++)
        {
            if(slot == 0)
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = HEATWAVE;
            }
            else if(slot == 1)
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = SUPERCELL;
            }
            else
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = NO_MODULE;
            }
        }
    }
    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_LOAD_GOOD_DATA;
    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
}
/*************************
 * end file fp2_sudsakorn_test.c
 *************************/


