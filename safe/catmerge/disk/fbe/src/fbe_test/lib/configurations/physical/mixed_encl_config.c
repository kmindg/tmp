
#include "mut.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe_test_configurations.h"
#include "physical_package_tests.h"
#include "pp_utils.h"


// BE AWARE some configurations may exceed slot limit as determined by platform type in fbe_test_load_mixed_config()
// init the list to the desired mix of enclosures
#if 1
//  dae < 150 slots
mixed_encl_config_type_t encl_config_list[] =
    {ENCL_CONFIG_VIPER, ENCL_CONFIG_DERRINGER, ENCL_CONFIG_VOYAGER, ENCL_CONFIG_ANCHO, ENCL_CONFIG_TABASCO};
#else
mixed_encl_config_type_t encl_config_list[] =
    {ENCL_CONFIG_VIPER, ENCL_CONFIG_NAGA, ENCL_CONFIG_DERRINGER, ENCL_CONFIG_VOYAGER, ENCL_CONFIG_ANCHO, ENCL_CONFIG_TABASCO, ENCL_CONFIG_VIKING, ENCL_CONFIG_CAYENNE};
// VNX supported dae
mixed_encl_config_type_t encl_config_list[] =
    {ENCL_CONFIG_VIPER, ENCL_CONFIG_DERRINGER, ENCL_CONFIG_VOYAGER, ENCL_CONFIG_VIKING};
// VNXe supported dae
mixed_encl_config_type_t encl_config_list[] =
    {ENCL_CONFIG_NAGA, ENCL_CONFIG_DERRINGER, ENCL_CONFIG_PINECONE, ENCL_CONFIG_ANCHO, ENCL_CONFIG_TABASCO, ENCL_CONFIG_CAYENNE};
// SAS2 dae
mixed_encl_config_type_t encl_config_list[] =
    {ENCL_CONFIG_NAGA, ENCL_CONFIG_ANCHO, ENCL_CONFIG_TABASCO, ENCL_CONFIG_CAYENNE};
#endif

fbe_u32_t mixed_config_get_encl_config_list_size(void)
{
     return  (sizeof(encl_config_list))/(sizeof(encl_config_list[0]));

}

fbe_sas_enclosure_type_t mixed_config_get_enclosure_type(mixed_encl_config_type_t config_item)
{
    fbe_sas_enclosure_type_t encl_type;
    switch (config_item)
    {
        case ENCL_CONFIG_VIPER:
            encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
            break;
        case ENCL_CONFIG_DERRINGER:
            encl_type = FBE_SAS_ENCLOSURE_TYPE_DERRINGER;
            break;
        case ENCL_CONFIG_VOYAGER:
            encl_type = FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM;
            break;
        case ENCL_CONFIG_BUNKER:
            encl_type = FBE_SAS_ENCLOSURE_TYPE_BUNKER;
            break;
        case ENCL_CONFIG_CITADEL:
            encl_type = FBE_SAS_ENCLOSURE_TYPE_CITADEL;
            break;
        case ENCL_CONFIG_TABASCO:
            encl_type = FBE_SAS_ENCLOSURE_TYPE_TABASCO;
            break;
        case ENCL_CONFIG_ANCHO:
            encl_type = FBE_SAS_ENCLOSURE_TYPE_ANCHO;
            break;
        case ENCL_CONFIG_VIKING:
            encl_type = FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP;
            break;
        case ENCL_CONFIG_CAYENNE:
            encl_type = FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP;
            break;
        case ENCL_CONFIG_NAGA:
            encl_type = FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP;
            break;
        case ENCL_CONFIG_PINECONE:
            encl_type = FBE_SAS_ENCLOSURE_TYPE_PINECONE;
            break;
        default:
            encl_type = FBE_SAS_ENCLOSURE_TYPE_UNKNOWN;
            break;
    }
    return(encl_type);
}

char *fbe_sas_encl_type_to_string(fbe_sas_enclosure_type_t sas_encl_type)
{
    char *string;
    
    switch(sas_encl_type)
    {
        case FBE_SAS_ENCLOSURE_TYPE_VIPER:
            string = "Viper";
            break;
        case FBE_SAS_ENCLOSURE_TYPE_DERRINGER:
            string = "Derringer";
            break;
        case FBE_SAS_ENCLOSURE_TYPE_TABASCO:
            string = "Tabasco";
            break;
        case FBE_SAS_ENCLOSURE_TYPE_BUNKER:
            string = "Bunker";
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CITADEL:
            string = "Citadel";
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM:
            string = "Voyager";
            break;
        case FBE_SAS_ENCLOSURE_TYPE_ANCHO:
            string = "Ancho";
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP:
            string = "Viking";
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP:
            string = "Cayenne";
            break;
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP:
            string = "Naga";
            break;
        default:
            string = "Unsupported Encl Type";
            break;
    }
    return(string);
}

fbe_u32_t mixed_config_get_drive_count(mixed_encl_config_type_t config_item)
{
    fbe_u32_t drive_count;
    switch (config_item)
    {
        case ENCL_CONFIG_VIPER:
            drive_count = MAX_DRIVE_COUNT_VIPER;
            break;
        case ENCL_CONFIG_DERRINGER:
            drive_count = MAX_DRIVE_COUNT_DERRINGER;
            break;
        case ENCL_CONFIG_TABASCO:
            drive_count = MAX_DRIVE_COUNT_TABASCO;
            break;
        case ENCL_CONFIG_VOYAGER:
            drive_count = MAX_DRIVE_COUNT_VOYAGER;
            break;
        case ENCL_CONFIG_VIKING:
            drive_count = MAX_DRIVE_COUNT_VIKING;
            break;
        case ENCL_CONFIG_CAYENNE:
            drive_count = MAX_DRIVE_COUNT_CAYENNE;
            break;
        case ENCL_CONFIG_NAGA:
            drive_count = MAX_DRIVE_COUNT_NAGA;
            break;
        case ENCL_CONFIG_BUNKER:
            drive_count = MAX_DRIVE_COUNT_BUNKER;
            break;
        case ENCL_CONFIG_CITADEL:
            drive_count = MAX_DRIVE_COUNT_CITADEL;
            break;        
        case ENCL_CONFIG_ANCHO:
            drive_count = MAX_DRIVE_COUNT_ANCHO;
            break;
        case ENCL_CONFIG_PINECONE:
            drive_count = MAX_DRIVE_COUNT_PINECONE;
            break;
        default:
            drive_count = 0;
            break;
    }
    return(drive_count);
}

fbe_u64_t mixed_config_get_drive_mask (mixed_encl_config_type_t config_item)
{
    fbe_u64_t drive_mask;
    switch (config_item)
    {
        case ENCL_CONFIG_VIPER:
            drive_mask = VIPER_MED_DRVCNT_MASK;
            break;
        case ENCL_CONFIG_DERRINGER:
            drive_mask = DERRINGER_MED_DRVCNT_MASK;
            break;
        case ENCL_CONFIG_TABASCO:
            drive_mask = TABASCO_MED_DRVCNT_MASK;
            break;
        case ENCL_CONFIG_VOYAGER:
            drive_mask = VOYAGER_MED_DRVCNT_MASK;
            break;
        case ENCL_CONFIG_BUNKER:
            drive_mask = BUNKER_MED_DRVCNT_MASK;
            break;
        case ENCL_CONFIG_CITADEL:
            drive_mask = CITADEL_MED_DRVCNT_MASK;
            break;
        case ENCL_CONFIG_ANCHO:
            drive_mask = ANCHO_MED_DRVCNT_MASK;
            break;
        case ENCL_CONFIG_VIKING:
            drive_mask = VIKING_MED_DRVCNT_MASK;
            break;
        case ENCL_CONFIG_CAYENNE:
            drive_mask = CAYENNE_MED_DRVCNT_MASK;
            break;
        case ENCL_CONFIG_NAGA:
            drive_mask = NAGA_MED_DRVCNT_MASK;
            break;
        case ENCL_CONFIG_PINECONE:
            drive_mask = PINECONE_MED_DRVCNT_MASK;
            break;
        default:
            drive_mask = 0;
            break;
    }
    return(drive_mask);
}

fbe_u32_t mixed_config_number_of_configured_drives(fbe_u32_t encl_index, fbe_u32_t num_slots)
{
    fbe_u64_t   drive_mask;
    fbe_u32_t   slot;
    fbe_u32_t   num_drives = 0;
    
    drive_mask = mixed_config_get_drive_mask(encl_config_list[encl_index]);
    for ( slot = 0; slot < num_slots; slot++ )
    {
        if (drive_mask & BIT(slot))
        {
            num_drives++;
        }
    }
    return(num_drives);
} 

/*!***************************************************************
 * @fn verify_mixed_encl_config()
 ****************************************************************
 * @brief
 *  Function to get and verify the state of the enclosure 
 *  and its drives for naxos configuration.
 *
 * @return- FBE_STATUS_OK if no errors.
 *
 * @version
 *  27-Apr-2011: PHE - Created. 
 ****************************************************************/
fbe_status_t verify_mixed_encl_config(fbe_u32_t port, 
                                      fbe_lifecycle_state_t state,
                                      fbe_u32_t timeout,
                                      fbe_u32_t limit)
{
    fbe_status_t    status;
    fbe_u64_t       drive_mask;
    fbe_u32_t       max_drives;
    fbe_u32_t       encl_idx;
    fbe_u32_t       slot_idx;
    fbe_u32_t       object_handle = 0;

    /* Loop through all the enclosures and its drives to check its state */
    for ( encl_idx = 0; encl_idx < FBE_TEST_ENCLOSURES_PER_BUS; encl_idx ++ )
    {
        if(encl_idx < limit)
        {
            /* Validate that the enclosure is in READY/ACTIVATE state */
            status = fbe_zrt_wait_enclosure_status(port, encl_idx, state, timeout);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            max_drives = mixed_config_get_drive_count(encl_config_list[encl_idx]);
            drive_mask = mixed_config_get_drive_mask(encl_config_list[encl_idx]);
            for ( slot_idx = 0; slot_idx < max_drives; slot_idx++ )
            {
                if (drive_mask & BIT(slot_idx))
                {
                    /* Validate that the Physical drives are in READY/ACTIVATE state */
                    status = fbe_test_pp_util_verify_pdo_state(port, encl_idx, slot_idx, state, timeout);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
                }
            }
        }
        else
        {
            /* Get the handle of the port and validate enclosure exists or not */
            status = fbe_api_get_enclosure_object_id_by_location (port, encl_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
        }
    }

    return FBE_STATUS_OK;
}


fbe_status_t fbe_test_load_mixed_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_class_id_t                  class_id;
    fbe_terminator_board_info_t         board_info;
    fbe_terminator_sas_port_info_t      sas_port;
    fbe_terminator_sas_encl_info_t      sas_encl;


    fbe_api_terminator_device_handle_t  port0_handle;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_api_terminator_device_handle_t  drive_handle;

    fbe_terminator_device_count_t dev_counts = {0};

    fbe_port_number_t               port_number;
    fbe_u64_t                       drive_mask;
    fbe_u32_t                       max_drives;
    fbe_u32_t                       total_objects;
    fbe_u32_t                       limit;
    fbe_u32_t                       port_idx;
    fbe_u32_t                       no_of_encls = 0;
    fbe_u32_t                       slot = 0;
    fbe_u32_t                       object_handle = 0;
    fbe_u32_t                       num_handles = 0;

    /* Before initializing the physical package, initialize the terminator. */
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: terminator initialized. for mixed Encl Config", __FUNCTION__);

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_PROMETHEUS_M1_HW_TYPE;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_private_space_layout_initialize_library(board_info.platform_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a port*/
    sas_port.backend_number = 0;
    sas_port.io_port_number = 3;
    sas_port.portal_number = 5;
    sas_port.sas_address = 0x5000097A7800903F;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    status  = fbe_api_terminator_insert_sas_port (&sas_port, &port0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    limit = mixed_config_get_encl_config_list_size();

    for ( no_of_encls = 0; no_of_encls < limit; no_of_encls++ )
    {
        /*insert an enclosure to port 0*/
        sas_encl.backend_number = 0;
        sas_encl.encl_number = no_of_encls;
        sas_encl.uid[0] = no_of_encls; // some unique ID.
        sas_encl.sas_address = FBE_TEST_BUILD_ENCL_SAS_ADDRESS(0, no_of_encls);
        sas_encl.encl_type = mixed_config_get_enclosure_type(encl_config_list[no_of_encls]);
        sas_encl.connector_id = 0;
        status = fbe_test_insert_sas_enclosure(port0_handle, &sas_encl, &encl_handle, &num_handles);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        drive_mask = mixed_config_get_drive_mask(encl_config_list[no_of_encls]);
        max_drives = mixed_config_get_drive_count(encl_config_list[no_of_encls]);
        for (slot = 0; slot < max_drives; slot++) 
        {
            if (drive_mask & BIT(slot))
            {
                status  = fbe_test_pp_util_insert_sas_drive(0,
                                                            no_of_encls,
                                                            slot,
                                                            520,
                                                            TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                            &drive_handle);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
        }
    }
    //return status;


    /* Initialize the fbe_api on server. */ 
    mut_printf(MUT_LOG_LOW, "%s: starting fbe_api...", __FUNCTION__);
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize the physical package. */
    mut_printf(MUT_LOG_LOW, "%s: starting physical package...", __FUNCTION__);
    fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);

    fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_terminator_get_terminator_device_count(&dev_counts);    /* Count number of objects in Terminator */  
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(dev_counts.total_devices, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "%s: starting configuration verification...", __FUNCTION__);

    /*we inserted a fleet board so we check for it*/
    /* board is always object id 0 ??? Maybe we need an interface to get the board object id*/
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* Check for the enclosures, physical drives and logical drives */
    for (port_idx = 0; port_idx < FBE_TEST_PHYSICAL_BUS_COUNT; port_idx++) 
    {
        if ( port_idx < 1 )
        {
            /*get the handle of the port and validate port exists*/
            status = fbe_api_get_port_object_id_by_location (port_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

            /* Use the handle to get lifecycle state*/
            status = fbe_api_wait_for_object_lifecycle_state (object_handle, 
                        FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            /* Use the handle to get port number*/
            status = fbe_api_get_object_port_number (object_handle, &port_number);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(port_number == port_idx);

            /* Verify the existence of the enclosure and its drives */
            status = verify_mixed_encl_config(port_idx,
                                              FBE_LIFECYCLE_STATE_READY,
                                              READY_STATE_WAIT_TIME,
                                              limit);

            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        }
        else
        {
            /*get the handle of the port and validate port exists or not*/
            status = fbe_api_get_port_object_id_by_location (port_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
        }
    }


    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == dev_counts.total_devices);
    mut_printf(MUT_LOG_LOW, "Verifying object count (%d)...Passed",total_objects);

    mut_printf(MUT_LOG_LOW, "%s: configuration verification SUCCEEDED!", __FUNCTION__);

    return(status);

} //fbe_test_load_mixed_config

