/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file the_phantom.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the SPECL interface test.
 *
 * @version
 *   12/01/2009 PHE - Created.
 *
 ***************************************************************************/

#include "physical_package_tests.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_pe_types.h"

char * the_phantom_short_desc = "Read processor enclosure environmental status.";
char * the_phantom_long_desc =
    "\n"
    "\n"
    "The Phantom scenario tests reading all the good processor enclsoure component status from SPECL in the physical package.\n"
    "It includes:\n" 
    "     - reading IO module status;\n"
    "     - reading IO port status;\n"
    "     - reading IO annex status;\n"
    "     - reading power supply status;\n"
    "     - reading management module status;\n"
    "     - reading blower/cooling status;\n"
    "     - reading mezzanine card status;\n"
    "     - reading MCU status;\n"
    "     - reading fault expander status;\n"
    "     - reading suitcase status;\n"
    "     - reading miscellaneous status.\n"
    "\n"
    "Dependencies:\n"
    "    - The SPECL should be able to work on the user space.\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada board with processor enclosure data\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 15 SAS drives (PDO)\n"
    "    [PP] 15 logical drives (LDO)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create the initial physical package config.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "\n"
    "STEP 2: Verify the misellaneous info.\n"
    "    - Verify peer SP is inserted.\n"
    "    - Verify the unsafe to remove LED is ON.\n"
    "    - Verify other fault LEDs is OFF.\n"
    "\n"
    "STEP 3: Verify the state of the IO module.\n"
    "    - Verify the IO Module 0 is installed.\n"
    "    - Verify the IO Module 0 is in good state.\n"
    "\n"
    "STEP 4: Verify the state of the IO annex.\n"
    "    - Verify the IO annex 0 is installed.\n"
    "    - Verify the IO annex 0 is in good state.\n"
    "\n"
    "STEP 5: Verify the state of the mezzanine card.\n"
    "    - Verify the mezzanine card is installed.\n"
    "    - Verify the mezzanine card is in good state.\n"
    "\n" 
    "STEP 6: Verify the state of the power supply.\n"
    "    - Verify the power supplies A & B are installed.\n"
    "    - Verify the power supplies A & B are in good state.\n"
    "\n"
    "STEP 7: Verify the state of the cooling component (blowers).\n"
    "    - Verify the Cooling components A & B are installed.\n"
    "    - Verify the Cooling components A & B are in good state.\n"
    "\n"
    "STEP 8: Verify the state of the management module.\n"
    "    - Verify the management module A & B are installed.\n"
    "    - Verify the management module A & B are in good state.\n"
    "\n"
    "STEP 9: Verify the state of the slave ports.\n"
    "    - Verify the generalStatus of the slave port is 0.\n"
    "    - Verify the componentStatus of the slave port is SW_COMPONENT_K10DGSSP.\n"
    "    - Verify the componentExtStatus of the slave port is SW_APPLICATION_STARTING.\n"
    "\n"
    "STEP 10: Verify the state of the suitcase.\n"
    "    - Verify the Suitcase is installed.\n"
    "    - Verify the Suitcase in in good state.\n"
    "\n"    
    "STEP 11: Verify the state of the MCU.\n"
    "    - Verify the MCU A & B are installed.\n"
    "    - Verify the MCU A & B are in good state.\n"
    "\n"
    "STEP 12: Shutdown the Terminator/Physical package.\n"
    ;
static void verify_platform_info(fbe_object_id_t objectId);
static void verify_misc_info(fbe_object_id_t objectId);
static void verify_io_module_info(fbe_object_id_t objectId);
static void verify_bem_info(fbe_object_id_t objectId);
static void verify_mezzanine_info(fbe_object_id_t objectId);
static void verify_power_supply_info(fbe_object_id_t objectId);
static void verify_cooling_info(fbe_object_id_t objectId);
static void verify_mgmt_module_info(fbe_object_id_t objectId);
static void verify_slave_port_info(fbe_object_id_t objectId);
static void verify_suitcase_info(fbe_object_id_t objectId);

void the_phantom(void)
{
    fbe_status_t                                fbeStatus;
    fbe_object_id_t                             objectId;  

    /* Get handle to the board object */
    fbeStatus = fbe_api_get_board_object_id(&objectId);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != FBE_OBJECT_ID_INVALID);

    verify_platform_info(objectId);
    verify_misc_info(objectId);
    verify_io_module_info(objectId);
    verify_bem_info(objectId);
    verify_mezzanine_info(objectId);
    verify_power_supply_info(objectId);
    verify_cooling_info(objectId);
    verify_mgmt_module_info(objectId);
    verify_slave_port_info(objectId);
    verify_suitcase_info(objectId);
    
    return;
}

static void verify_platform_info(fbe_object_id_t objectId)
{
    fbe_board_mgmt_platform_info_t              platformInfo = {0};
    fbe_status_t                                fbeStatus = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "*** Verifying Platform Info ***\n");
    
    fbeStatus = fbe_api_board_get_platform_info(objectId, &platformInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    return;
}

static void verify_misc_info(fbe_object_id_t objectId)
{
    fbe_board_mgmt_misc_info_t                  miscInfo = {0};
    fbe_status_t                                fbeStatus = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "*** Verifying Miscellanous status ***\n");
    
    fbeStatus = fbe_api_board_get_misc_info(objectId, &miscInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(miscInfo.engineIdFault == FALSE);
    MUT_ASSERT_TRUE(miscInfo.peerInserted == TRUE);     
    MUT_ASSERT_TRUE(miscInfo.SPFaultLED == LED_BLINK_OFF); 
    MUT_ASSERT_TRUE(miscInfo.UnsafeToRemoveLED == LED_BLINK_OFF); 
    MUT_ASSERT_TRUE(miscInfo.EnclosureFaultLED == LED_BLINK_OFF); 
    MUT_ASSERT_TRUE(miscInfo.ManagementPortLED == LED_BLINK_OFF); 
    MUT_ASSERT_TRUE(miscInfo.ServicePortLED == LED_BLINK_OFF); 

    return;
}

static void verify_io_module_info(fbe_object_id_t objectId)
{
    SP_ID                                       sp;
    fbe_u32_t                                   slot;
    fbe_u32_t                                   componentCountPerBlade = 0;
    fbe_board_mgmt_io_comp_info_t               ioModuleInfo = {0};
    fbe_status_t                                fbeStatus = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "*** Verifying IO Module status ***\n");

    fbeStatus = fbe_api_board_get_io_module_count_per_blade(objectId, &componentCountPerBlade);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
   
    mut_printf(MUT_LOG_LOW, "*** Get IO Module count Successfully, Count: %d. ***\n", componentCountPerBlade * SP_ID_MAX);    

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

    return;
}

static void verify_bem_info(fbe_object_id_t objectId)
{
    SP_ID                                       sp;
    fbe_u32_t                                   slot;
    fbe_u32_t                                   componentCountPerBlade = 0;
    fbe_board_mgmt_io_comp_info_t               ioAnnexInfo = {0};
    fbe_status_t                                fbeStatus = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "*** Verifying IO Annex status ***\n");

    fbeStatus = fbe_api_board_get_bem_count_per_blade(objectId, &componentCountPerBlade);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "*** Get IO Annex count Successfully, Count: %d. ***\n", componentCountPerBlade * SP_ID_MAX);

    for(sp = SP_A; sp < SP_ID_MAX; sp ++)
    {
        for(slot = 0; slot < componentCountPerBlade; slot ++)
        {
            ioAnnexInfo.associatedSp = sp;
            ioAnnexInfo.slotNumOnBlade = slot;

            fbeStatus = fbe_api_board_get_bem_info(objectId, &ioAnnexInfo);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(ioAnnexInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
            MUT_ASSERT_TRUE(ioAnnexInfo.inserted == FBE_MGMT_STATUS_TRUE);     
            MUT_ASSERT_TRUE(ioAnnexInfo.faultLEDStatus == LED_BLINK_OFF);
            MUT_ASSERT_TRUE(ioAnnexInfo.powerStatus == FBE_POWER_STATUS_POWER_ON);   
        }
    }

    return;
}

static void verify_mezzanine_info(fbe_object_id_t objectId)
{
    SP_ID                                       sp;
    fbe_u32_t                                   slot;
    fbe_u32_t                                   componentCountPerBlade = 0;
    fbe_board_mgmt_io_comp_info_t               mezzanineInfo = {0};
    fbe_status_t                                fbeStatus = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "*** Verifying Mezzanine status ***\n");

    fbeStatus = fbe_api_board_get_mezzanine_count_per_blade(objectId, &componentCountPerBlade);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "*** Get Mezzanine count Successfully, Count: %d. ***\n", componentCountPerBlade * SP_ID_MAX);

    for(sp = SP_A; sp < SP_ID_MAX; sp ++)
    {
        for(slot = 0; slot < componentCountPerBlade; slot ++)
        {
            mezzanineInfo.associatedSp = sp;
            mezzanineInfo.slotNumOnBlade = slot;

            fbeStatus = fbe_api_board_get_mezzanine_info(objectId, &mezzanineInfo);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(mezzanineInfo.inserted == FBE_MGMT_STATUS_TRUE);     
            MUT_ASSERT_TRUE(mezzanineInfo.uniqueID == GROWLER_DVT); 
        }
    }

    return;
}

static void verify_power_supply_info(fbe_object_id_t objectId)
{
    fbe_u32_t                                   slot;
    fbe_u32_t                                   componentCount = 0;
    fbe_power_supply_info_t                     psInfo = {0};
    fbe_status_t                                fbeStatus = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "*** Verifying Power supply status ***\n");

    fbeStatus = fbe_api_board_get_ps_count(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "*** Got Power supply count Successfully, Count: %d. ***\n", componentCount);

    for(slot = 0; slot < componentCount; slot ++)
    {
        psInfo.slotNumOnEncl = slot;

        fbeStatus = fbe_api_board_get_power_supply_info(objectId, &psInfo);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(psInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
        MUT_ASSERT_TRUE(psInfo.bInserted == TRUE);     
        MUT_ASSERT_TRUE(psInfo.generalFault == FBE_MGMT_STATUS_FALSE);
        MUT_ASSERT_TRUE(psInfo.internalFanFault == FBE_MGMT_STATUS_FALSE);     
        MUT_ASSERT_TRUE(psInfo.ambientOvertempFault == FBE_MGMT_STATUS_FALSE);
        MUT_ASSERT_TRUE(psInfo.DCPresent == FBE_MGMT_STATUS_FALSE);
        MUT_ASSERT_TRUE(psInfo.ACFail == FBE_MGMT_STATUS_FALSE);
        MUT_ASSERT_TRUE(psInfo.ACDCInput == AC_INPUT);
    }
   
    return;
}

static void verify_cooling_info(fbe_object_id_t objectId)
{
    fbe_u32_t                                   slot;
    fbe_u32_t                                   componentCount = 0;
    fbe_cooling_info_t                          CoolingInfo = {0};
    fbe_status_t                                fbeStatus = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "*** Verifying Cooling component status ***\n");

    fbeStatus = fbe_api_board_get_cooling_count(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "*** Got Cooling Component count Successfully, Count: %d. ***\n", componentCount);
   
    if (componentCount > 0)
    {
    for(slot = 0; slot < componentCount; slot ++)
    {
        CoolingInfo.slotNumOnEncl = slot;

        fbeStatus = fbe_api_board_get_cooling_info(objectId, &CoolingInfo);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(CoolingInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
        MUT_ASSERT_TRUE(CoolingInfo.fanFaulted == FBE_MGMT_STATUS_FALSE);     
        MUT_ASSERT_TRUE(CoolingInfo.multiFanModuleFault == FBE_MGMT_STATUS_FALSE);
    }
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "*** NO Cooling Component present in Sentry CPU module. ***\n");
    }

    return;
}

static void verify_mgmt_module_info(fbe_object_id_t objectId)
{
    SP_ID                              sp;
    fbe_u32_t                          slot;
    fbe_u32_t                          componentCountPerBlade = 0;
    fbe_board_mgmt_mgmt_module_info_t  mgmtModuleInfo = {0};
    fbe_status_t                       fbeStatus = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "*** Verifying Management module status ***\n");

    fbeStatus = fbe_api_board_get_mgmt_module_count_per_blade(objectId, &componentCountPerBlade);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "*** Got Management module count Successfully, Count: %d. ***\n", componentCountPerBlade * SP_ID_MAX);

    for(sp = SP_A; sp < SP_ID_MAX; sp ++)
    {
        for(slot = 0; slot < componentCountPerBlade; slot ++)
        {
            mgmtModuleInfo.associatedSp = sp;
            mgmtModuleInfo.mgmtID = slot;

            fbeStatus = fbe_api_board_get_mgmt_module_info(objectId, &mgmtModuleInfo);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(mgmtModuleInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
            MUT_ASSERT_TRUE(mgmtModuleInfo.bInserted == TRUE);     
            MUT_ASSERT_TRUE(mgmtModuleInfo.generalFault == FBE_MGMT_STATUS_FALSE);     
            MUT_ASSERT_TRUE(mgmtModuleInfo.faultLEDStatus == LED_BLINK_OFF);
            MUT_ASSERT_TRUE(mgmtModuleInfo.faultInfoStatus == 10);
            MUT_ASSERT_TRUE(mgmtModuleInfo.managementPort.linkStatus == FALSE);
            MUT_ASSERT_TRUE(mgmtModuleInfo.managementPort.mgmtPortSpeed == 0x0);
            MUT_ASSERT_TRUE(mgmtModuleInfo.managementPort.mgmtPortDuplex == 1);
            MUT_ASSERT_TRUE(mgmtModuleInfo.managementPort.mgmtPortAutoNeg == FBE_PORT_AUTO_NEG_ON);
            MUT_ASSERT_TRUE(mgmtModuleInfo.servicePort.linkStatus == FALSE);
            MUT_ASSERT_TRUE(mgmtModuleInfo.servicePort.portSpeed == 0x0);
            MUT_ASSERT_TRUE(mgmtModuleInfo.servicePort.portDuplex == 1);
            MUT_ASSERT_TRUE(mgmtModuleInfo.servicePort.autoNegotiate == TRUE);
            MUT_ASSERT_TRUE(mgmtModuleInfo.vLanConfigMode == CLARiiON_VLAN_MODE);
            MUT_ASSERT_TRUE(mgmtModuleInfo.firmwareRevMinor == 0);      
            MUT_ASSERT_TRUE(mgmtModuleInfo.firmwareRevMajor == 0);
        }
    }

    return;
}

static void verify_slave_port_info(fbe_object_id_t objectId)
{
    SP_ID                              sp;
    fbe_u32_t                          slot;
    fbe_u32_t                          componentCountPerBlade = 0;
    fbe_board_mgmt_slave_port_info_t   slavePortInfo = {0};
    fbe_status_t                       fbeStatus = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "*** Verifying Slave Port status ***\n");

    fbeStatus = fbe_api_board_get_slave_port_count_per_blade(objectId, &componentCountPerBlade);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "*** Get Slave Port count Successfully, Count: %d. ***\n", componentCountPerBlade * SP_ID_MAX);

    for(sp = SP_A; sp < SP_ID_MAX; sp ++)
    {
        for(slot = 0; slot < componentCountPerBlade; slot ++)
        {
            slavePortInfo.associatedSp = sp;
            slavePortInfo.slavePortID = slot;

            fbeStatus = fbe_api_board_get_slave_port_info(objectId, &slavePortInfo);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(slavePortInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
            MUT_ASSERT_TRUE(slavePortInfo.generalStatus == 0);    
            MUT_ASSERT_TRUE(slavePortInfo.componentStatus == SW_COMPONENT_K10DGSSP);
            MUT_ASSERT_TRUE(slavePortInfo.componentExtStatus == SW_APPLICATION_STARTING);
        }
    }

    return;
}

static void verify_suitcase_info(fbe_object_id_t objectId)
{
    SP_ID                              sp;
    fbe_u32_t                          slot;
    fbe_u32_t                          componentCountPerBlade = 0;
    fbe_board_mgmt_suitcase_info_t     SuitcaseInfo = {0};
    fbe_status_t                       fbeStatus = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "*** Verifying Suitcase status ***\n");

    fbeStatus = fbe_api_board_get_suitcase_count_per_blade(objectId, &componentCountPerBlade);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "*** Got Suitcase count Successfully, Count: %d. ***\n", componentCountPerBlade * SP_ID_MAX);

    for(sp = SP_A; sp < SP_ID_MAX; sp ++)
    {
        for(slot = 0; slot < componentCountPerBlade; slot ++)
        {
            SuitcaseInfo.associatedSp = sp;
            SuitcaseInfo.suitcaseID = slot;

            fbeStatus = fbe_api_board_get_suitcase_info(objectId, &SuitcaseInfo);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(SuitcaseInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
            MUT_ASSERT_TRUE(SuitcaseInfo.Tap12VMissing == FALSE);
            MUT_ASSERT_TRUE(SuitcaseInfo.shutdownWarning == FALSE);
            MUT_ASSERT_TRUE(SuitcaseInfo.ambientOvertempFault == FALSE);
            MUT_ASSERT_TRUE(SuitcaseInfo.fanPackFailure == FALSE);      
            MUT_ASSERT_TRUE(SuitcaseInfo.tempSensor == 30);    
        }
    }

    return;
}
