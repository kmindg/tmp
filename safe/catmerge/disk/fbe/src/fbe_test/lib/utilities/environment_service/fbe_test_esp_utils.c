#include "esp_tests.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_base_object.h"
#include "mut.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_file.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "sep_utils.h"
#include "pp_utils.h"

extern fbe_char_t * lcc_manifest_str;
extern fbe_char_t * ps_manifest_str;
/**************************************
                Local variables
**************************************/
#define FBE_API_POLLING_INTERVAL 200 /*ms*/

// the content of cdes_image_headers is defined in fbe_test_esp_utils.h
static fbe_u8_t cdes_unified_image_headers[][FBE_ENCLOSURE_MAX_IMAGE_HEADER_SIZE]= CDES_UNIFIED_IMAGE_HEADERS_CONTENT;
static fbe_u8_t cdes_1_1_image_headers[][FBE_ENCLOSURE_MAX_IMAGE_HEADER_SIZE]= CDES_1_1_IMAGE_HEADERS_CONTENT;
static fbe_u8_t cdes_image_headers[][FBE_ENCLOSURE_MAX_IMAGE_HEADER_SIZE]= CDES_IMAGE_HEADERS_CONTENT;
static fbe_u8_t jdes_image_headers[][FBE_ENCLOSURE_MAX_IMAGE_HEADER_SIZE]= JDES_IMAGE_HEADERS_CONTENT;
static fbe_u8_t cdes_2_image_headers[][FBE_ENCLOSURE_MAX_IMAGE_HEADER_SIZE]= CDES_2_IMAGE_HEADERS_CONTENT;

fbe_class_id_t test_esp_class_table[] = { FBE_CLASS_ID_ENCL_MGMT,
                                          FBE_CLASS_ID_SPS_MGMT,
                                          FBE_CLASS_ID_DRIVE_MGMT,
                                          FBE_CLASS_ID_MODULE_MGMT,
                                          FBE_CLASS_ID_PS_MGMT,
                                          FBE_CLASS_ID_BOARD_MGMT,
                                          FBE_CLASS_ID_COOLING_MGMT,
                                          FBE_CLASS_ID_INVALID,
                                        };


fbe_class_id_t test_physical_package_class_table[] = {
    FBE_CLASS_ID_HAMMERHEAD_BOARD,
    FBE_CLASS_ID_SLEDGEHAMMER_BOARD,
    FBE_CLASS_ID_JACKHAMMER_BOARD,
    FBE_CLASS_ID_BOOMSLANG_BOARD,
    FBE_CLASS_ID_DELL_BOARD,
    FBE_CLASS_ID_FLEET_BOARD,
    FBE_CLASS_ID_MAGNUM_BOARD,
    FBE_CLASS_ID_ARMADA_BOARD,
    FBE_CLASS_ID_FIBRE_PORT,
    FBE_CLASS_ID_SAS_PORT,
    FBE_CLASS_ID_FC_PORT,
    FBE_CLASS_ID_ISCSI_PORT,
    FBE_CLASS_ID_SAS_LSI_PORT,
    FBE_CLASS_ID_SAS_CPD_PORT,
    FBE_CLASS_ID_SAS_PMC_PORT,
    FBE_CLASS_ID_FC_PMC_PORT,
    FBE_CLASS_ID_FIBRE_XPE_PORT,
    FBE_CLASS_ID_FIBRE_DPE_PORT, 
    FBE_CLASS_ID_LOGICAL_DRIVE,
    FBE_CLASS_ID_SAS_PHYSICAL_DRIVE,
    FBE_CLASS_ID_SAS_FLASH_DRIVE,
    FBE_CLASS_ID_SATA_PHYSICAL_DRIVE,
    FBE_CLASS_ID_SATA_FLASH_DRIVE,
    FBE_CLASS_ID_SAS_BULLET_ENCLOSURE,
    FBE_CLASS_ID_SAS_VIPER_ENCLOSURE,
    FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE,
    FBE_CLASS_ID_SAS_CITADEL_ENCLOSURE,
    FBE_CLASS_ID_SAS_BUNKER_ENCLOSURE,
    FBE_CLASS_ID_SAS_DERRINGER_ENCLOSURE,
    FBE_CLASS_ID_SAS_VOYAGER_ICM_ENCLOSURE,
    FBE_CLASS_ID_SAS_RHEA_ENCLOSURE,
    FBE_CLASS_ID_SAS_BOXWOOD_ENCLOSURE,
    FBE_CLASS_ID_SAS_CALYPSO_ENCLOSURE,
    FBE_CLASS_ID_SAS_TABASCO_ENCLOSURE,
    FBE_CLASS_ID_SAS_ANCHO_ENCLOSURE,
    FBE_CLASS_ID_SAS_VIKING_IOSXP_ENCLOSURE,
    FBE_CLASS_ID_SAS_CAYENNE_IOSXP_ENCLOSURE,
    FBE_CLASS_ID_SAS_NAGA_IOSXP_ENCLOSURE,
    FBE_CLASS_ID_INVALID,
};

/* Do not add FBE_DEVICE_TYPE_FAN to the table for now.
 * because the test configuration does not have the external fan module. 
 * After the support for Voyager ICM & Edge expanders goes in, we can change
 * the test configuration to Voyager and then add FBE_DEVICE_TYPE_FAN to the table.
 */
fbe_u64_t test_esp_fup_device_type_table[] = {
    FBE_DEVICE_TYPE_LCC,
    FBE_DEVICE_TYPE_PS,
    FBE_DEVICE_TYPE_INVALID,
};

static fbe_test_esp_fup_image_param_t test_esp_fup_image_param_table[] = 
{
    /*deviceType, fwTarget, imageSize, pImagePathKey, pImagePathValue, pImagePathAndNameHeader */
    {FBE_DEVICE_TYPE_PS,
     FBE_FW_TARGET_PS,
     0x15858,  // The last two bits must be 0s.
     "CDESPowerSupplyImagePath",
     ".",
     "./SXP36x6g_Power_Supply_000B0015_pid",
    },
    {FBE_DEVICE_TYPE_PS,
     FBE_FW_TARGET_PS,
     0x15858,  // The last two bits must be 0s.
     "CDESPowerSupplyImagePath",
     ".",
     "./SXP36x6g_Power_Supply_000B0019_pid",
    },
    {FBE_DEVICE_TYPE_LCC,
     FBE_FW_TARGET_LCC_MAIN,
     0x15858,  // The last two bits must be 0s. 
     "CDESLccImagePath",
     ".",
     "./SXP36x6G_Bundled_Fw_pid",
    },
    {FBE_DEVICE_TYPE_SP,
     FBE_FW_TARGET_LCC_MAIN,
     0x15858,  // The last two bits must be 0s. 
     "CDESLccImagePath",
     ".",
     "./SXP36x6G_Bundled_Fw_pid",
    },
};
#define FBE_TEST_ESP_FUP_IMAGE_MAX     (sizeof(test_esp_fup_image_param_table)/sizeof(test_esp_fup_image_param_table[0]))

// each entry corresponds to an image file
// the order must match the order of the binary image array
static fbe_test_esp_fup_image_param_t test_esp_fup_cdes11_image_param_table[] = 
{
    /*deviceType, fwTarget, imageSize, pImagePathKey, pImagePathValue, pImagePathAndNameHeader */
    {FBE_DEVICE_TYPE_PS,
     FBE_FW_TARGET_PS,
     0x15858,  // The last two bits must be 0s.
     "CDES11PowerSupplyImagePath",
     ".",
     "./CDES_1_1_Power_Supply_000B0020_pid",
    },
    {FBE_DEVICE_TYPE_LCC,
     FBE_FW_TARGET_LCC_MAIN,
     0x15858,  // The last two bits must be 0s. 
     "CDES11LccImagePath",
     ".",
     "./CDES_1_1_Bundled_Fw_pid",
    },
    {FBE_DEVICE_TYPE_FAN,
     FBE_FW_TARGET_COOLING,
     0x15858,  // The last two bits must be 0s. 
     "CDES11PowerSupplyImagePath",
     ".",
     "./CDES_1_1_Cooling_Module_00110002_pid",
    },
    {FBE_DEVICE_TYPE_SPS,
     FBE_FW_TARGET_SPS_PRIMARY,
     0x15858,  // The last two bits must be 0s. 
     "CDES11PowerSupplyImagePath",
     ".",
     "./CDES_1_1_SPS_FW_000E0008_pid",
    },
    {FBE_DEVICE_TYPE_SPS,
     FBE_FW_TARGET_SPS_SECONDARY,
     0x15858,  // The last two bits must be 0s. 
     "CDES11PowerSupplyImagePath",
     ".",
     "./CDES_1_1_SPS2_FW_000E0008_pid",
    },
    {FBE_DEVICE_TYPE_SPS,
     FBE_FW_TARGET_SPS_BATTERY,
     0x15858,  // The last two bits must be 0s. 
     "CDES11PowerSupplyImagePath",
     ".",
     "./CDES_1_1_SPS_BBU_FW_000E0009_pid",
    },
};

#define FBE_TEST_ESP_FUP_CDES11_IMAGE_MAX     (sizeof(test_esp_fup_cdes11_image_param_table)/sizeof(test_esp_fup_cdes11_image_param_table[0]))


static fbe_test_esp_fup_image_param_t test_esp_fup_jdes_image_param_table[] = 
{
    /*deviceType, fwTarget, imageSize, pImagePathKey, pImagePathValue, pImagePathAndNameHeader */
    {FBE_DEVICE_TYPE_BACK_END_MODULE,
     FBE_FW_TARGET_LCC_MAIN,
     0x15858,  // The last two bits must be 0s. 
     "JDESLccImagePath",
     ".",
     "./JDES_Bundled_FW_pid",
    },
};

#define FBE_TEST_ESP_FUP_JDES_IMAGE_MAX     (sizeof(test_esp_fup_jdes_image_param_table)/sizeof(test_esp_fup_jdes_image_param_table[0]))

static fbe_test_esp_fup_image_param_t test_esp_fup_cdes_unified_image_param_table[] = 
{
    /*deviceType, fwTarget, imageSize, pImagePathKey, pImagePathValue, pImagePathAndNameHeader */
    {FBE_DEVICE_TYPE_PS,
     FBE_FW_TARGET_PS,
     0x15858,  // The last two bits must be 0s.
     "CDESPowerSupplyImagePath",
     ".",
     "./CDES_Power_Supply_000B002F_pid",
    },
    {FBE_DEVICE_TYPE_LCC,
     FBE_FW_TARGET_LCC_MAIN,
     0x15858,  // The last two bits must be 0s. 
     "CDESUnifiedLccImagePath",
     ".",
     "./CDES_Bundled_FW_pid",
    },
};
#define FBE_TEST_ESP_FUP_CDES_UNI_IMAGE_MAX     (sizeof(test_esp_fup_cdes_unified_image_param_table)/sizeof(test_esp_fup_cdes_unified_image_param_table[0]))

static fbe_test_esp_fup_image_param_t test_esp_fup_cdes_2_image_param_table[] = 
{
    /*deviceType, fwTarget, imageSize, pImagePathKey, pImagePathValue, pImagePathAndNameHeader */
    {FBE_DEVICE_TYPE_LCC,
     FBE_FW_TARGET_LCC_FPGA,
     0x15858,  // The last two bits must be 0s. 
     "CDES2LccImagePath",
     ".",
     "./cdef_pid",
    },
    {FBE_DEVICE_TYPE_LCC,
     FBE_FW_TARGET_LCC_FPGA,
     0x15858,  // The last two bits must be 0s. 
     "CDES2LccImagePath",
     ".",
     "./cdef_dual_pid",
    },
    {FBE_DEVICE_TYPE_LCC,
     FBE_FW_TARGET_LCC_INIT_STRING,
     0x15858,  // The last two bits must be 0s. 
     "CDES2LccImagePath",
     ".",
     "./istr_pid",
    },
    {FBE_DEVICE_TYPE_LCC,
     FBE_FW_TARGET_LCC_EXPANDER,
     0x15858,  // The last two bits must be 0s. 
     "CDES2LccImagePath",
     ".",
     "./cdes_rom_pid",
    },
    {FBE_DEVICE_TYPE_PS,
     FBE_FW_TARGET_PS,
     0x15858,  // The last two bits must be 0s. 
     "CDES2PowerSupplyImagePath",
     ".",
     "./laserbeak_pid",
    },
    {FBE_DEVICE_TYPE_PS,
     FBE_FW_TARGET_PS,
     0x15858,  // The last two bits must be 0s. 
     "CDES2PowerSupplyImagePath",
     ".",
     "./octane_pid",
    },
    {FBE_DEVICE_TYPE_PS,
     FBE_FW_TARGET_PS,
     0x15858,  // The last two bits must be 0s. 
     "CDES2PowerSupplyImagePath",
     ".",
     "./juno2_pid",
    },
    {FBE_DEVICE_TYPE_PS,
     FBE_FW_TARGET_PS,
     0x15858,  // The last two bits must be 0s. 
     "CDES2PowerSupplyImagePath",
     ".",
     "./3gve_pid",
    }
};
#define FBE_TEST_ESP_FUP_CDES_2_IMAGE_MAX     (sizeof(test_esp_fup_cdes_2_image_param_table)/sizeof(test_esp_fup_cdes_2_image_param_table[0]))

#define FBE_TEST_ESP_FUP_CDES_2_LCC_MANIFEST_FILE_PATH_AND_NAME    "./manifest_lcc_pid"
#define FBE_TEST_ESP_FUP_CDES_2_PS_MANIFEST_FILE_PATH_AND_NAME    "./manifest_ps_pid"

/*!**************************************************************
 * @fn fbe_test_startPhyPkg()
 ****************************************************************
 * @brief
 *  Load physical Package.
 *
 * @param none.
 *
 * @return fbe_status_t - FBE_STATUS_OK/FBE_STATUS_GENERIC_FAILURE.
 *
 * HISTORY:
 *
 ****************************************************************/
fbe_status_t fbe_test_startPhyPkg(fbe_bool_t waitForObjectReady, fbe_u32_t num_objects)
{
    fbe_status_t status;
    fbe_u32_t total_objects, class_table_index, obj_count, total;
    fbe_object_id_t *obj_array = NULL;
    
    // Start Physical Package
    mut_printf(MUT_LOG_LOW, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (waitForObjectReady)
    {

        status = fbe_api_wait_for_number_of_objects(num_objects, 100000, FBE_PACKAGE_ID_PHYSICAL);
        /* Loop through the list of classes for Physical Package */
        for (class_table_index = 0; test_physical_package_class_table[class_table_index] != FBE_CLASS_ID_INVALID; class_table_index++) {
            mut_printf(MUT_LOG_LOW, "Getting the list of object\n");
            status = fbe_api_get_total_objects_of_class(test_physical_package_class_table[class_table_index],
                                                        FBE_PACKAGE_ID_PHYSICAL, 
                                                        &total_objects);
            mut_printf(MUT_LOG_LOW, "Total Object in class:%d is :%d\n", test_physical_package_class_table[class_table_index], total_objects);
            if(total_objects > 0)
            {
                status = fbe_api_enumerate_class(test_physical_package_class_table[class_table_index], FBE_PACKAGE_ID_PHYSICAL, &obj_array, &total);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                /* Loop through all the object in the class */
                for(obj_count = 0; obj_count < total_objects; obj_count++) {
                    mut_printf(MUT_LOG_LOW, "Getting life cycle state for Class:%d, Obj ID:%d\n", test_physical_package_class_table[class_table_index], obj_array[obj_count]);
                    status = fbe_api_wait_for_object_lifecycle_state(obj_array[obj_count], FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_PHYSICAL);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                }
                fbe_api_free_memory(obj_array);
            }
        }
    }

    return status;
}

fbe_status_t fbe_test_startEnvMgmtPkg(fbe_bool_t waitForObjectReady)
{
    fbe_status_t status;
    fbe_u32_t total_objects, class_table_index, obj_count, total;
    fbe_object_id_t *obj_array = NULL;

    /* If the registry file does not exist, create one */
    fbe_test_esp_create_registry_file(FBE_FALSE);

    mut_printf(MUT_LOG_LOW, "=== starting ESP ===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set ESP entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    is_esp_loaded = FBE_TRUE;

    // Init fbe api on server (need it to confirm object)
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (waitForObjectReady)
    {
        /* Loop through the list of classes for ESP */
        for (class_table_index = 0; test_esp_class_table[class_table_index] != FBE_CLASS_ID_INVALID; class_table_index++) {
            mut_printf(MUT_LOG_LOW, "Getting the list of object for Class %d\n", test_esp_class_table[class_table_index]);
            status = fbe_api_get_total_objects_of_class(test_esp_class_table[class_table_index],
                                                        FBE_PACKAGE_ID_ESP, 
                                                        &total_objects);
            mut_printf(MUT_LOG_LOW, "Total Object in class:%d is :%d\n", test_esp_class_table[class_table_index], total_objects);
            if(total_objects > 0)
            {
                status = fbe_api_enumerate_class(test_esp_class_table[class_table_index], FBE_PACKAGE_ID_ESP, &obj_array, &total);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                MUT_ASSERT_INT_EQUAL(total_objects, total);

                /* Loop through all the object in the class */
                for(obj_count = 0; obj_count < total_objects; obj_count++) {
                    mut_printf(MUT_LOG_LOW, "Getting life cycle state for Class:%d, Obj ID:%d\n", test_esp_class_table[class_table_index], obj_array[obj_count]);
                    if((test_esp_class_table[class_table_index] != FBE_CLASS_ID_MODULE_MGMT) &&
                        (test_esp_class_table[class_table_index] != FBE_CLASS_ID_ENCL_MGMT) &&
                        (test_esp_class_table[class_table_index] != FBE_CLASS_ID_SPS_MGMT)) 
                    {
                        status = fbe_api_wait_for_object_lifecycle_state(obj_array[obj_count], FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_ESP);
                        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    }
                }
                fbe_api_free_memory(obj_array);
            }
        }
    }

    return status;
}

fbe_status_t fbe_test_wait_for_all_esp_objects_ready(void)
{
    fbe_status_t status;
    fbe_u32_t total_objects, class_table_index, obj_count, total;
    fbe_object_id_t *obj_array = NULL;
    /* Loop through the list of classes for ESP */
    for (class_table_index = 0; test_esp_class_table[class_table_index] != FBE_CLASS_ID_INVALID; class_table_index++) {
        mut_printf(MUT_LOG_LOW, "Getting the list of object for Class %d\n", test_esp_class_table[class_table_index]);
        status = fbe_api_get_total_objects_of_class(test_esp_class_table[class_table_index],
                                                    FBE_PACKAGE_ID_ESP, 
                                                    &total_objects);
        mut_printf(MUT_LOG_LOW, "Total Object in class:%d is :%d\n", test_esp_class_table[class_table_index], total_objects);
        if(total_objects > 0)
        {
            status = fbe_api_enumerate_class(test_esp_class_table[class_table_index], FBE_PACKAGE_ID_ESP, &obj_array, &total);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_EQUAL(total_objects, total);

            /* Loop through all the object in the class */
            for(obj_count = 0; obj_count < total_objects; obj_count++) {
                mut_printf(MUT_LOG_LOW, "Getting life cycle state for Class:%d, Obj ID:%d\n", test_esp_class_table[class_table_index], obj_array[obj_count]);

                status = fbe_api_wait_for_object_lifecycle_state(obj_array[obj_count], FBE_LIFECYCLE_STATE_READY, 40000, FBE_PACKAGE_ID_ESP);

                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            }
            fbe_api_free_memory(obj_array);
        }
    }
    return status;
}


fbe_status_t fbe_test_startAutoFupEnvMgmtPkg(fbe_bool_t waitForObjectReady)
{
    fbe_status_t status;
    fbe_u32_t total_objects, class_table_index, obj_count, total;
    fbe_object_id_t *obj_array = NULL;

    /* If the registry file does not exist, create one */
    fbe_test_esp_create_registry_file(FBE_FALSE);
    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_FAN);
    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_LCC);
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_LCC);

    mut_printf(MUT_LOG_LOW, "=== starting ESP ===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set ESP entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // Init fbe api on server (need it to confirm object)
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (waitForObjectReady)
    {
        /* Loop through the list of classes for ESP */
        for (class_table_index = 0; test_esp_class_table[class_table_index] != FBE_CLASS_ID_INVALID; class_table_index++) {
            mut_printf(MUT_LOG_LOW, "Getting the list of object for Class %d\n", test_esp_class_table[class_table_index]);
            status = fbe_api_get_total_objects_of_class(test_esp_class_table[class_table_index],
                                                        FBE_PACKAGE_ID_ESP, 
                                                        &total_objects);
            mut_printf(MUT_LOG_LOW, "Total Object in class:%d is :%d\n", test_esp_class_table[class_table_index], total_objects);
            if(total_objects > 0)
            {
                status = fbe_api_enumerate_class(test_esp_class_table[class_table_index], FBE_PACKAGE_ID_ESP, &obj_array, &total);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                MUT_ASSERT_INT_EQUAL(total_objects, total);

                /* Loop through all the object in the class */
                for(obj_count = 0; obj_count < total_objects; obj_count++) {
                    mut_printf(MUT_LOG_LOW, "Getting life cycle state for Class:%d, Obj ID:%d\n", test_esp_class_table[class_table_index], obj_array[obj_count]);
                    if((test_esp_class_table[class_table_index] != FBE_CLASS_ID_MODULE_MGMT) &&
                        (test_esp_class_table[class_table_index] != FBE_CLASS_ID_ENCL_MGMT) &&
                        (test_esp_class_table[class_table_index] != FBE_CLASS_ID_SPS_MGMT)) 
                    {
                        status = fbe_api_wait_for_object_lifecycle_state(obj_array[obj_count], FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_ESP);
                        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    }
                }
                fbe_api_free_memory(obj_array);
            }
        }
    }

    return status;
}

void fbe_test_esp_common_destroy(void)
{
    fbe_status_t status;
    fbe_test_package_list_t package_list;

    fbe_zero_memory(&package_list, sizeof(package_list));
        
    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

    package_list.number_of_packages = 2;
    package_list.package_list[0] = FBE_PACKAGE_ID_ESP; 
    package_list.package_list[1] = FBE_PACKAGE_ID_PHYSICAL;

    fbe_test_common_util_package_destroy(&package_list);
    status = fbe_api_terminator_destroy();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "destroy terminator api failed");

    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);

    mut_printf(MUT_LOG_LOW, "=== %s completes ===\n", __FUNCTION__);

    return;
}


void fbe_test_esp_common_destroy_dual_sp(void)
{
    fbe_status_t status;
    fbe_test_package_list_t package_list;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

    fbe_zero_memory(&package_list, sizeof(package_list));
    package_list.number_of_packages = 2;
    package_list.package_list[0] = FBE_PACKAGE_ID_ESP; 
    package_list.package_list[1] = FBE_PACKAGE_ID_PHYSICAL;

    // destroy SPA things
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    fbe_test_common_util_package_destroy(&package_list);
    status = fbe_api_terminator_destroy();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "destroy terminator api failed");

    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);

    // destroy SPB things
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    fbe_test_common_util_package_destroy(&package_list);
    status = fbe_api_terminator_destroy();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "destroy terminator api failed");

    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    mut_printf(MUT_LOG_LOW, "=== %s completes ===\n", __FUNCTION__);

    return;
}

void fbe_test_esp_common_destroy_all(void)
{
    fbe_status_t status;
    fbe_test_package_list_t package_list;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

    fbe_zero_memory(&package_list,sizeof(package_list));
    package_list.number_of_packages = 4;
    package_list.package_list[0] = FBE_PACKAGE_ID_NEIT;
    package_list.package_list[1] = FBE_PACKAGE_ID_SEP_0;
    package_list.package_list[2] = FBE_PACKAGE_ID_ESP;
    package_list.package_list[3] = FBE_PACKAGE_ID_PHYSICAL;

    fbe_test_common_util_package_destroy(&package_list);
    status = fbe_api_terminator_destroy();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "destroy terminator api failed");

    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);

    mut_printf(MUT_LOG_LOW, "=== %s completes ===\n", __FUNCTION__);

    return;
}

void fbe_test_esp_common_destroy_all_dual_sp(void)
{
    /* First execute teardown on SP B then on SP A
    */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_destroy_neit_sep_physical();

    /* Now execute teardown on SP A
    */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_sep_util_destroy_neit_sep_physical();

    return;
}

/*!********************************************************************
 *  @fn fbe_test_esp_wait_for_no_upgrade_in_progress ()
 *********************************************************************
 *  @brief
 *    This function poll the ESP for no upgrade in progress.
 *
 *  @param timeoutMs - timeout value in mili-second
 *
 *  @return fbe_status_t
 *
 *  @version
 *    01-Sept-2010: PHE - Created
 *    
 *********************************************************************/
fbe_status_t fbe_test_esp_wait_for_no_upgrade_in_progress(fbe_u32_t timeoutMs)
{
    fbe_status_t                        status = FBE_STATUS_OK; 

    mut_printf(MUT_LOG_LOW, "=== Check LCC FUP.. ===\n");
    status = fbe_test_esp_wait_for_any_upgrade_in_progress_status(FBE_DEVICE_TYPE_LCC, FALSE, timeoutMs);
                        
    if(status!= FBE_STATUS_OK)
    {
        return status;
    }

    mut_printf(MUT_LOG_LOW, "=== Check PS FUP.. ===\n");
    status = fbe_test_esp_wait_for_any_upgrade_in_progress_status(FBE_DEVICE_TYPE_PS, FALSE, timeoutMs);

    if(status!= FBE_STATUS_OK)
    {
        return status;
    }

    mut_printf(MUT_LOG_LOW, "=== Check FAN FUP.. ===\n");
    status = fbe_test_esp_wait_for_any_upgrade_in_progress_status(FBE_DEVICE_TYPE_FAN, FALSE, timeoutMs);

    if(status!= FBE_STATUS_OK)
    {
        return status;
    }


    return FBE_STATUS_OK;
}

/*!********************************************************************
 *  @fn fbe_test_esp_wait_for_any_upgrade_in_progress_status ()
 *********************************************************************
 *  @brief
 *    This function poll the ESP for any upgrade in progress status.
 *
 *  @param deviceType - 
 *  @param expectedAnyUpgradeInProgress
 *  @param timeoutMs - timeout value in mili-second
 *
 *  @return fbe_status_t
 *
 *  @version
 *    22-Jul-2010: PHE - Created
 *    
 *********************************************************************/
fbe_status_t fbe_test_esp_wait_for_any_upgrade_in_progress_status(fbe_u64_t deviceType, 
                             fbe_bool_t expectedAnyUpgradeInProgress,
                             fbe_u32_t timeoutMs)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                           currentTime = 0;
    fbe_bool_t                          anyUpgradeInProgress = FALSE;

    while (currentTime < timeoutMs){
        status = fbe_api_esp_common_get_any_upgrade_in_progress(deviceType, &anyUpgradeInProgress);
 
        if (status != FBE_STATUS_OK){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                          "%s: Get any upgrade in progress returned with status 0x%X.\n", 
                          __FUNCTION__, status); 
            return status;
        }
            
        if (anyUpgradeInProgress == expectedAnyUpgradeInProgress){
            return status;
        }
        
        currentTime = currentTime + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: wait for upgrade in progress (%s) failed within the expected %d ms!\n",
                  __FUNCTION__,
                  (expectedAnyUpgradeInProgress == TRUE)? "Yes" : "No",
                  timeoutMs);

    return FBE_STATUS_GENERIC_FAILURE;
}


/*!********************************************************************
 *  @fn fbe_test_esp_wait_for_fup_work_state ()
 *********************************************************************
 *  @brief
 *    This function poll the ESP for firmware upgrade work state
 *    for the specified device.
 *
 *  @param deviceType
 *  @param pLocation
 *  @param expectedWorkState
 *  @param timeout_ms - timeout value in mili-second
 *
 *  @return fbe_status_t
 *
 *  @version
 *    22-Jul-2010: PHE - Created
 *    
 *********************************************************************/
fbe_status_t fbe_test_esp_wait_for_fup_work_state(fbe_u64_t deviceType,
                                             fbe_device_physical_location_t * pLocation,
                                             fbe_base_env_fup_work_state_t expectedWorkState,
                                             fbe_u32_t timeoutMs)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                               currentTime = 0;
    fbe_base_env_fup_work_state_t           workState = FBE_BASE_ENV_FUP_WORK_STATE_NONE;

    while (currentTime < timeoutMs){
        status = fbe_api_esp_common_get_fup_work_state(deviceType, pLocation, &workState);
 
        if (status != FBE_STATUS_OK){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                          "%s: Get fup work state returned with status 0x%X.\n", 
                          __FUNCTION__, status); 
            return status;
        }
            
        if (workState == expectedWorkState){
            return status;
        }
        
        currentTime += 50;
        fbe_api_sleep (50);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                  "%s: WorkState Act:0x%x Exp:0x%X after %d ms!\n",
                  __FUNCTION__,
                  workState,
                  expectedWorkState,
                  timeoutMs);

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!********************************************************************
 *  @fn fbe_test_esp_wait_for_fup_completion_status ()
 *********************************************************************
 *  @brief
 *    This function poll the ESP for firmware upgrade completion status
 *    for the specified device.
 *
 *  @param deviceType
 *  @param pLocation
 *  @param expectedCompletionStatus
 *  @param timeout_ms - timeout value in mili-second
 *
 *  @return fbe_status_t
 *
 *  @version
 *    22-Jul-2010: PHE - Created
 *    
 *********************************************************************/
fbe_status_t fbe_test_esp_wait_for_fup_completion_status(fbe_u64_t deviceType,
                                                    fbe_device_physical_location_t * pLocation,
                                                    fbe_base_env_fup_completion_status_t expectedCompletionStatus,
                                                    fbe_u32_t timeoutMs)
{
    fbe_status_t                         status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                            currentTime = 0;
    fbe_esp_base_env_get_fup_info_t      getFupInfo = {0};
    fbe_u8_t                              idx=0;

    getFupInfo.deviceType = deviceType;
    getFupInfo.location = *pLocation;

    while (currentTime < timeoutMs){
        status = fbe_api_esp_common_get_fup_info(&getFupInfo);
 
        if (status != FBE_STATUS_OK){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                          "%s: Get fup info with status 0x%X.\n",
                          __FUNCTION__, status); 
            return status;
        }

        for(idx = 0; idx < getFupInfo.programmableCount; idx ++) 
        {
            if(getFupInfo.fupInfo[idx].componentId == pLocation->componentId) 
            {
                break;
            }
        }

        if (getFupInfo.fupInfo[idx].completionStatus == expectedCompletionStatus){
            return status;
        }
        currentTime += 50;
        fbe_api_sleep (50);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: wait for fup completion status 0x%X failed within the expected %d ms!\n",
                  __FUNCTION__,
                  expectedCompletionStatus,
                  timeoutMs);

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!********************************************************************
 *  @fn fbe_test_esp_wait_for_ps_status ()
 *********************************************************************
 *  @brief
 *    This function poll the ESP for the ps status.
 *
 *  @param pLocation
 *  @param expectedInserted
 *  @param expectedFaulted
 *  @param timeout_ms - timeout value in mili-second
 *
 *  @return fbe_status_t
 *
 *  @version
 *    22-Jul-2010: PHE - Created
 *    
 *********************************************************************/
fbe_status_t fbe_test_esp_wait_for_ps_status(fbe_device_physical_location_t * pLocation,
                                             fbe_bool_t expectedInserted,
                                             fbe_mgmt_status_t expectedFaulted,
                                             fbe_u32_t timeoutMs)
{
    fbe_status_t                         status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                            currentTime = 0;
    fbe_esp_ps_mgmt_ps_info_t            getPsInfo = {0};

    getPsInfo.location = *pLocation;

    while (currentTime < timeoutMs){
        status = fbe_api_esp_ps_mgmt_getPsInfo(&getPsInfo);

        if (status != FBE_STATUS_OK){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                          "%s: Get PS info returned with status 0x%X.\n",
                          __FUNCTION__, status); 
            return status;
        }
            
        if((getPsInfo.psInfo.bInserted == expectedInserted) &&
           (getPsInfo.psInfo.generalFault == expectedFaulted)){
            return status;
        }
        
        currentTime = currentTime + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: wait for ps inserted %d, faulted %d failed within the expected %d ms!\n",
                  __FUNCTION__,
                  expectedInserted,
                  expectedFaulted,
                  timeoutMs);

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!********************************************************************
 *  @fn fbe_test_esp_wait_for_no_resume_prom_read_in_progress ()
 *********************************************************************
 *  @brief
 *    This function poll the ESP for no resume prom read in progress.
 *
 *  @param timeoutMs - timeout value in millisecond
 *
 *  @return fbe_status_t
 *
 *  @version
 *    15-Nov-2011: loul - Created
 *
 *********************************************************************/
fbe_status_t fbe_test_esp_wait_for_no_resume_prom_read_in_progress(fbe_u32_t timeoutMs)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u32_t class_table_index;
    fbe_class_id_t classes_to_be_checked[] = {FBE_CLASS_ID_ENCL_MGMT,
                                              FBE_CLASS_ID_MODULE_MGMT,
                                              FBE_CLASS_ID_PS_MGMT,
                                              FBE_CLASS_ID_BOARD_MGMT,
                                              FBE_CLASS_ID_COOLING_MGMT,
                                              FBE_CLASS_ID_INVALID,
                                             };

    for (class_table_index = 0; classes_to_be_checked[class_table_index] != FBE_CLASS_ID_INVALID; class_table_index++) {
        mut_printf(MUT_LOG_LOW,
                "=== Check Resume PROM Read for Class %d ... ===\n",
                classes_to_be_checked[class_table_index]);
            status = fbe_test_esp_wait_for_any_resume_prom_read_in_progress_status(classes_to_be_checked[class_table_index], FALSE, timeoutMs);

            if(status!= FBE_STATUS_OK)
            {
                return status;
            }
        }

    return FBE_STATUS_OK;
}

/*!********************************************************************
 *  @fn fbe_test_esp_wait_for_any_resume_prom_read_in_progress_status ()
 *********************************************************************
 *  @brief
 *    This function poll the ESP for any resume prom read in progress status.
 *
 *  @param classId -
 *  @param expectedAnyReadInProgress
 *  @param timeoutMs - timeout value in millisecond
 *
 *  @return fbe_status_t
 *
 *  @version
 *    15-Nov-2011: loul - Created
 *
 *********************************************************************/
fbe_status_t fbe_test_esp_wait_for_any_resume_prom_read_in_progress_status(fbe_class_id_t classId,
        fbe_bool_t expectedAnyReadInProgress,
        fbe_u32_t timeoutMs)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           currentTime = 0;
    fbe_bool_t                          anyReadInProgress = FALSE;

    while (currentTime < timeoutMs){
        status = fbe_api_esp_common_get_any_resume_prom_read_in_progress(classId, &anyReadInProgress);

        if (status != FBE_STATUS_OK){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                          "%s: Get any resume prom read in progress returned with status 0x%X.\n",
                          __FUNCTION__, status);
            return status;
        }

        if (anyReadInProgress == expectedAnyReadInProgress){
            return status;
        }

        currentTime = currentTime + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: wait for resume prom read in progress (%s) timeout was exceeded (%d ms)!\n",
                  __FUNCTION__,
                  (expectedAnyReadInProgress == TRUE)? "Yes" : "No",
                  timeoutMs);

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!**************************************************************
 * fbe_test_esp_initiate_upgrade() 
 ****************************************************************
 * @brief
 *  Initiates the upgrade for specified device type and location.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  18-Oct-2010 PHE - Created. 
 *
 ****************************************************************/
void fbe_test_esp_initiate_upgrade(fbe_u64_t deviceType,
                                       fbe_device_physical_location_t * pLocation,
                                       fbe_u32_t forceFlags)
{
    fbe_status_t status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "Initiating upgrade for %s(%d_%d_%d) cid %d", 
               fbe_base_env_decode_device_type(deviceType),
               pLocation->bus,
               pLocation->enclosure,
               pLocation->slot,
               pLocation->componentId);

    status = fbe_api_esp_common_initiate_upgrade(deviceType, pLocation, forceFlags);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to initiate upgrade!");
    mut_printf(MUT_LOG_LOW, "Upgrade for %s(%d_%d_%d) cid %d is initiated", 
               fbe_base_env_decode_device_type(deviceType),
               pLocation->bus,
               pLocation->enclosure,
               pLocation->slot,
               pLocation->componentId);

    return;
}

/*!**************************************************************
 * fbe_test_esp_initiate_upgrade_for_all() 
 ****************************************************************
 * @brief
 *  Initiates the upgrade for all the firmware upgrade device types
 *  in the specified location.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  18-Oct-2010 PHE - Created. 
 *
 ****************************************************************/
void fbe_test_esp_initiate_upgrade_for_all(fbe_device_physical_location_t * pLocation,
                                                           fbe_u32_t forceFlags)
{
    fbe_u32_t index;
    fbe_u64_t deviceType;

    for(index = 0; test_esp_fup_device_type_table[index] != FBE_DEVICE_TYPE_INVALID; index ++)
    {
        deviceType = test_esp_fup_device_type_table[index];

        fbe_test_esp_initiate_upgrade(deviceType, pLocation, forceFlags);
    }

    return;
}

/*!**************************************************************
 * fbe_test_esp_verify_fup_completion_status() 
 ****************************************************************
 * @brief
 *  Verifies the completion status for all the firmware upgrade device types
 *  in the specified location.
 * 
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  18-Oct-2010 PHE - Created. 
 *
 ****************************************************************/
void fbe_test_esp_verify_fup_completion_status(fbe_u64_t deviceType,
                                   fbe_device_physical_location_t * pLocation,
                                   fbe_base_env_fup_completion_status_t expectedCompletionStatus)
{
    fbe_status_t status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "Verifying %s(%d_%d_%d) cid %d completion status %s", 
               fbe_base_env_decode_device_type(deviceType),
               pLocation->bus,
               pLocation->enclosure,
               pLocation->slot,
               pLocation->componentId,
               fbe_base_env_decode_fup_completion_status(expectedCompletionStatus));

    /* Wait for 120 seconds to get the completion status.
     */
    status = fbe_test_esp_wait_for_fup_completion_status(deviceType, 
                                                    pLocation, 
                                                    expectedCompletionStatus,
                                                    120000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Wait for completion status failed!");

    mut_printf(MUT_LOG_LOW, "%s(%d_%d_%d) completion status is verified.", 
               fbe_base_env_decode_device_type(deviceType),
               pLocation->bus,
               pLocation->enclosure,
               pLocation->slot);

    return;
}

/*!**************************************************************
 * fbe_test_esp_verify_fup_completion_status_for_all() 
 ****************************************************************
 * @brief
 *  Verifies the completion status for all the firmware upgrade device types
 *  in the specified location.
 * 
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  18-Oct-2010 PHE - Created. 
 *
 ****************************************************************/
void fbe_test_esp_verify_fup_completion_status_for_all(fbe_device_physical_location_t * pLocation,
                 fbe_base_env_fup_completion_status_t expectedCompletionStatus)
{
    fbe_u32_t index;
    fbe_u64_t deviceType;

    for(index = 0; test_esp_fup_device_type_table[index] != FBE_DEVICE_TYPE_INVALID; index ++)
    {
        deviceType = test_esp_fup_device_type_table[index];

        fbe_test_esp_verify_fup_completion_status(deviceType, pLocation, expectedCompletionStatus);
    }

    return;
}


fbe_status_t
fbe_test_esp_registry_write(fbe_u8_t* pRegPath,
                   fbe_u8_t* pKey,
                   fbe_registry_value_type_t valueType,
                   void *value_p,
                   fbe_u32_t length)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_sim_server_pid sp_pid = 0;
    fbe_u8_t pid_str[16] = {'\0'};

    status = fbe_api_sim_server_get_pid(&sp_pid);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: failed to get SP PID.\n",
                      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_sprintf(pid_str, sizeof(pid_str), "%llu", (unsigned long long)sp_pid);

    fbe_registry_write(pid_str, pRegPath, pKey, valueType, value_p, length);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_test_esp_registry_read(fbe_u8_t* pRegPath,
                    fbe_u8_t* pKey,
                    void* pBuffer,
                    fbe_u32_t bufferLength,
                    fbe_registry_value_type_t ValueType,
                    void* defaultValue_p,
                    fbe_u32_t defaultLength,
                    fbe_bool_t createKey)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_sim_server_pid sp_pid = 0;
    fbe_u8_t pid_str[16] = {'\0'};

    status = fbe_api_sim_server_get_pid(&sp_pid);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: failed to get SP PID.\n",
                      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_sprintf(pid_str, sizeof(pid_str), "%llu", (unsigned long long)sp_pid);

    fbe_registry_read(pid_str, pRegPath, pKey, pBuffer, bufferLength, ValueType,
            defaultValue_p, defaultLength, createKey);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_test_esp_create_registry_file(fbe_bool_t recreate)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_sim_server_pid sp_pid = 0;
    fbe_u8_t file_path[64] = {'\0'};
    fbe_file_handle_t fp = NULL;
    csx_status_e dir_status;  

    status = fbe_api_sim_server_get_pid(&sp_pid);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: failed to get SP PID.\n",
                      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Need to create sp_sim_pid directory if it doesn't exist.
     */
    sprintf(file_path, "./sp_sim_pid%d", (int)sp_pid);

    dir_status = csx_p_dir_create(file_path);
    if(!(CSX_SUCCESS(dir_status) || (dir_status == CSX_STATUS_OBJECT_ALREADY_EXISTS)))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: failed to create sp_sim_pid.\n",
                      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Then create the file
     */
    fbe_sprintf(file_path, sizeof(file_path), "./sp_sim_pid%d/esp_reg_pid%d.txt", (int)sp_pid, (int)sp_pid);

    if(recreate)
    {
        fp = fbe_file_creat(file_path, FBE_FILE_RDWR);
        fbe_file_close(fp);
    }
    else
    {
        fp = fbe_file_open(file_path, FBE_FILE_RDWR, 0, NULL);
        if(fp == FBE_FILE_INVALID_HANDLE)
        {
            fp = fbe_file_creat(file_path, FBE_FILE_RDWR);
            fbe_file_close(fp);
        }
        else
        {
            fbe_file_close(fp);
        }
    }

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_test_esp_delete_registry_file(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_sim_server_pid sp_pid = 0;
    fbe_u8_t file_path[64] = {'\0'};

    status = fbe_api_sim_server_get_pid(&sp_pid);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: failed to get SP PID.\n",
                      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_sprintf(file_path, sizeof(file_path), "./sp_sim_pid%d/esp_reg_pid%d.txt", (int)sp_pid, (int)sp_pid);

    fbe_file_delete(file_path);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_test_esp_create_esp_lun(fbe_bool_t recreate)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_sim_server_pid sp_pid = 0;
    fbe_u8_t file_path[64] = {'\0'};
    csx_status_e dir_status;  

    status = fbe_api_sim_server_get_pid(&sp_pid);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: failed to get SP PID.\n",
                      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Need to create sp_sim_pid directory if it doesn't exist.
     */
    sprintf(file_path, "./sp_sim_pid%d", (int)sp_pid);

    dir_status = csx_p_dir_create(file_path);
    if(!(CSX_SUCCESS(dir_status) || (dir_status == CSX_STATUS_OBJECT_ALREADY_EXISTS)))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: failed to create sp_sim_pid.\n",
                      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Then create the file
     */
    fbe_sprintf(file_path, sizeof(file_path), "./sp_sim_pid%d/esp_lun_pid%d.txt", (int)sp_pid, (int)sp_pid);

    if(recreate)
    {
        fbe_file_creat(file_path, FBE_FILE_RDWR);
    }
    else
    {
        fbe_file_handle_t fp = NULL;
        fp = fbe_file_open(file_path, FBE_FILE_RDWR, 0, NULL);
        if(fp == FBE_FILE_INVALID_HANDLE)
        {
            fbe_file_creat(file_path, FBE_FILE_RDWR);
        }
        else
        {
            fbe_file_close(fp);
        }
    }

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_test_esp_delete_esp_lun(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_sim_server_pid sp_pid = 0;
    fbe_u8_t file_path[64] = {'\0'};

    status = fbe_api_sim_server_get_pid(&sp_pid);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: failed to get SP PID.\n",
                      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_sprintf(file_path, sizeof(file_path), "./sp_sim_pid%d/esp_lun_pid%d.txt", (int)sp_pid, (int)sp_pid);

    fbe_file_delete(file_path);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_test_esp_build_image_file_path_and_name(char * image_file_path_and_name,
                                            fbe_u32_t image_file_path_and_name_length,
                                            char * image_file_path_and_name_header)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_sim_server_pid pid = 0;
    fbe_char_t file_name_extra_info[32] = "\0";

    image_file_path_and_name[0] = '\0';


    status = fbe_api_sim_server_get_pid(&pid);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to get SP PID.");

    sprintf(file_name_extra_info, "%llu.bin", (unsigned long long)pid);

    MUT_ASSERT_TRUE_MSG(strlen(image_file_path_and_name_header) + strlen(file_name_extra_info)
            <  image_file_path_and_name_length, "Failed to build image file name: insufficient buffer.");

    strcat(image_file_path_and_name, image_file_path_and_name_header);
    strcat(image_file_path_and_name, file_name_extra_info);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_test_esp_build_manifest_file_path_and_name(char * manifest_file_path_and_name,
                                            fbe_u32_t manifest_file_path_and_name_length,
                                            char * manifest_file_path_and_name_header)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_sim_server_pid pid = 0;
    fbe_char_t file_name_extra_info[32] = "\0";

    manifest_file_path_and_name[0] = '\0';


    status = fbe_api_sim_server_get_pid(&pid);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to get SP PID.");

    sprintf(file_name_extra_info, "%llu.json", (unsigned long long)pid);

    MUT_ASSERT_TRUE_MSG(strlen(manifest_file_path_and_name_header) + strlen(file_name_extra_info)
            <  manifest_file_path_and_name_length, "Failed to build manifest file name: insufficient buffer.");

    strcat(manifest_file_path_and_name, manifest_file_path_and_name_header);
    strcat(manifest_file_path_and_name, file_name_extra_info);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_test_esp_create_dmo_lun(fbe_bool_t recreate)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_sim_server_pid sp_pid = 0;
    fbe_u8_t file_path[64] = {'\0'};
    csx_status_e dir_status;  

    status = fbe_api_sim_server_get_pid(&sp_pid);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: failed to get SP PID.\n",
                      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Need to create sp_sim_pid directory if it doesn't exist.
     */
    sprintf(file_path, "./sp_sim_pid%d", (int)sp_pid);

    dir_status = csx_p_dir_create(file_path);
    if(!(CSX_SUCCESS(dir_status) || (dir_status == CSX_STATUS_OBJECT_ALREADY_EXISTS)))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: failed to create sp_sim_pid.\n",
                      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Then create the file
     */
    fbe_sprintf(file_path, sizeof(file_path), "./sp_sim_pid%d/dmo_lun_pid%d.txt", (int)sp_pid, (int)sp_pid);

    if(recreate)
    {
        fbe_file_creat(file_path, FBE_FILE_RDWR);
    }
    else
    {
        fbe_file_handle_t fp = NULL;
        fp = fbe_file_open(file_path, FBE_FILE_RDWR, 0, NULL);
        if(fp == FBE_FILE_INVALID_HANDLE)
        {
            fbe_file_creat(file_path, FBE_FILE_RDWR);
        }
        else
        {
            fbe_file_close(fp);
        }
    }

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_test_esp_delete_dmo_lun(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_sim_server_pid sp_pid = 0;
    fbe_u8_t file_path[64] = {'\0'};

    status = fbe_api_sim_server_get_pid(&sp_pid);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: failed to get SP PID.\n",
                      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_sprintf(file_path, sizeof(file_path), "./sp_sim_pid%d/dmo_lun_pid%d.txt", (int)sp_pid, (int)sp_pid);

    fbe_file_delete(file_path);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_test_esp_setup_terminator_upgrade_env() 
 ****************************************************************
 * @brief
 *  Set up the terminator upgrade environment.
 *
 * @param pLocation - the pointer to the enclosure location.               
 * @param activate_time_interval (in milliseconds) - the time before the real activate.
 * @param reset_time_interval (in milliseconds) - the time before reset.
 * @param update_rev (in milliseconds) - update the rev or not.
 * 
 * @return None.
 *
 * @author
 *  22-Jul-2010 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_test_esp_setup_terminator_upgrade_env(fbe_device_physical_location_t * pLocation,
                                 fbe_u32_t activate_time_interval,
                                 fbe_u32_t reset_time_interval,
                                 fbe_bool_t update_rev)
{
    fbe_api_terminator_device_handle_t   terminatorEnclHandle;
    fbe_status_t status = FBE_STATUS_OK;

    /* Get the terminator enclosure handle. */
    status = fbe_api_terminator_get_enclosure_handle(pLocation->bus, pLocation->enclosure, &terminatorEnclHandle); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set 20 seconds before the real activating. */
    status = fbe_api_terminator_set_enclosure_firmware_activate_time_interval(terminatorEnclHandle, activate_time_interval);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set 20 seconds before reset to create the activate timeout error. */
    status = fbe_api_terminator_set_enclosure_firmware_reset_time_interval(terminatorEnclHandle, reset_time_interval);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set upgrade rev to TRUE. */
    status = fbe_api_terminator_set_need_update_enclosure_firmware_rev(update_rev);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_test_esp_load_basic_terminator_config()
 ****************************************************************
 * @brief
 *  This function creates a basic configuration required to run
 *  IO in the physical package as part of a basic sanity for
 *  these tests.  
 *
 * @param platform_type - Hardware platform defaults to load.               
 *
 * @return None.   
 *
 * @author
 *   09/01/2010 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fbe_test_esp_load_basic_terminator_config(SPID_HW_TYPE platform_type)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t         board_info;
    fbe_terminator_sas_port_info_t      sas_port;
    fbe_terminator_sas_encl_info_t      sas_encl;

    fbe_api_terminator_device_handle_t  port0_handle;
    fbe_api_terminator_device_handle_t  encl0_0_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_cpd_shim_hardware_info_t        hardware_info;
    fbe_u32_t                           slot = 0;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = platform_type;
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

    /* configure port hardware info */
    fbe_zero_memory(&hardware_info,sizeof(fbe_cpd_shim_hardware_info_t));
    hardware_info.vendor = 0x11F8;
    hardware_info.device = 0x8001;
    hardware_info.bus = 0x34;
    hardware_info.slot = 0;
    hardware_info.function = 0;

    status = fbe_api_terminator_set_hardware_info(port0_handle,&hardware_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert an enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1; // some unique ID.
    sas_encl.sas_address = 0x5000097A78009071;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    status  = fbe_api_terminator_insert_sas_enclosure (port0_handle, &sas_encl, &encl0_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (slot = 0; slot < SIMPLE_CONFIG_DRIVE_MAX; slot ++) {
        /*insert a sas_drive to port 0 encl 0 slot */
        status  = fbe_test_pp_util_insert_sas_drive(0,
                                                    0,
                                                    slot,
                                                    520,
                                                    TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                    &drive_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    return status;
}

/*!**************************************************************
 * fbe_test_esp_load_simple_config()
 ****************************************************************
 * @brief
 *  This function creates the configuration based on the platform type.
 *
 * @param platform_type -               
 *
 * @return fbe_status_t -   
 *
 * @author
 *   24-Jan-2011: PHE Created. 
 *
 ****************************************************************/
fbe_status_t fbe_test_esp_load_simple_config(SPID_HW_TYPE platform_type, fbe_sas_enclosure_type_t enclosureType)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_terminator_board_info_t         board_info;
    fbe_terminator_sas_port_info_t      sas_port;
    fbe_terminator_sas_encl_info_t      sas_encl;

    fbe_api_terminator_device_handle_t  port0_handle;
    fbe_api_terminator_device_handle_t  encl0_0_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_cpd_shim_hardware_info_t        hardware_info;
    fbe_u32_t                           slot = 0;
    fbe_u32_t                           num_handles = 0;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = platform_type;
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

    /* configure port hardware info */
    fbe_zero_memory(&hardware_info,sizeof(fbe_cpd_shim_hardware_info_t));
    hardware_info.vendor = 0x11F8;
    hardware_info.device = 0x8001;
    hardware_info.bus = 0x34;
    hardware_info.slot = 0;
    hardware_info.function = 0;

    status = fbe_api_terminator_set_hardware_info(port0_handle,&hardware_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert an enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1; // some unique ID.
    sas_encl.sas_address = 0x5000097A78009071;
    sas_encl.connector_id = 0;
    if(platform_type == SPID_DEFIANT_M1_HW_TYPE)
    {
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_FALLBACK;
    }
    else if(platform_type == SPID_OBERON_1_HW_TYPE)
    {
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_RHEA;
    }
    else if(platform_type == SPID_HYPERION_1_HW_TYPE)
    {
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_CALYPSO;
    }

    else 
    {
        sas_encl.encl_type = enclosureType;
    }

    //status  = fbe_api_terminator_insert_sas_enclosure (port0_handle, &sas_encl, &encl0_0_handle);
    status = fbe_test_insert_sas_enclosure(port0_handle, &sas_encl, &encl0_0_handle, &num_handles);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (slot = 0; slot < SIMPLE_CONFIG_DRIVE_MAX; slot ++) {
        /*insert a sas_drive to port 0 encl 0 slot */
        status  = fbe_test_pp_util_insert_sas_drive(0,
                                                    0,
                                                    slot,
                                                    520,
                                                    TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                    &drive_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    return status;
}

/*!**************************************************************
 * fbe_test_esp_load_simple_config_with_voyager()
 ****************************************************************
 * @brief
 *  This function creates the configuration based on the platform type. And insert
 *  a voyager DAE to the array.
 *
 * @param platform_type -               
 *
 * @return fbe_status_t -   
 *
 * @author
 *   24-July-2012: Rui Chang Created. 
 *
 ****************************************************************/
fbe_status_t fbe_test_esp_load_simple_config_with_voyager(SPID_HW_TYPE platform_type)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_terminator_board_info_t         board_info;
    fbe_terminator_sas_port_info_t      sas_port;
    fbe_terminator_sas_encl_info_t      sas_encl;

    fbe_api_terminator_device_handle_t  port0_handle;
    fbe_api_terminator_device_handle_t  encl0_0_handle;
    fbe_api_terminator_device_handle_t  encl0_1_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_cpd_shim_hardware_info_t        hardware_info;
    fbe_u32_t                           slot = 0;
    fbe_u32_t                           num_handles;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = platform_type;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_private_space_layout_initialize_library(board_info.platform_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a port*/
    sas_port.backend_number = 0;
    sas_port.io_port_number = 3;
    sas_port.portal_number = 5;
    sas_port.sas_address = FBE_BUILD_PORT_SAS_ADDRESS(sas_port.backend_number);
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    status  = fbe_api_terminator_insert_sas_port (&sas_port, &port0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* configure port hardware info */
    fbe_zero_memory(&hardware_info,sizeof(fbe_cpd_shim_hardware_info_t));
    hardware_info.vendor = 0x11F8;
    hardware_info.device = 0x8001;
    hardware_info.bus = 0x34;
    hardware_info.slot = 0;
    hardware_info.function = 0;

    status = fbe_api_terminator_set_hardware_info(port0_handle,&hardware_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

     /*insert an enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1; // some unique ID.
    sas_encl.sas_address = FBE_BUILD_ENCL_SAS_ADDRESS(sas_encl.backend_number, sas_encl.encl_number);
    if(platform_type == SPID_DEFIANT_M1_HW_TYPE)
    {
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_FALLBACK;
    }
    else if(platform_type == SPID_OBERON_1_HW_TYPE)
    {
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_RHEA;
    }
    else if(platform_type == SPID_HYPERION_1_HW_TYPE)
    {
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_CALYPSO;
    }
    else 
    {
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    }

        status  = fbe_test_insert_sas_enclosure (port0_handle, &sas_encl, &encl0_0_handle, &num_handles);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (slot = 0; slot < SIMPLE_CONFIG_DRIVE_MAX; slot ++) {
        /*insert a sas_drive to port 0 encl 0 slot */
        status  = fbe_test_pp_util_insert_sas_drive(0,
                                                    0,
                                                    slot,
                                                    520,
                                                    TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                    &drive_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /*insert an voyager enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 1;
    sas_encl.uid[0] = 2; // some unique ID.
    sas_encl.sas_address = FBE_BUILD_ENCL_SAS_ADDRESS(sas_encl.backend_number, sas_encl.encl_number);
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM;


    status  = fbe_test_insert_sas_enclosure (port0_handle, &sas_encl, &encl0_1_handle, &num_handles);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
 
    for (slot = 0; slot < SIMPLE_CONFIG_DRIVE_MAX; slot ++) {
        status  = fbe_test_pp_util_insert_sas_drive(0,
                                                    1,
                                                    slot,
                                                    520,
                                                    TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                    &drive_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    } 

    return status;
}

/*!********************************************************************
 *  @fn fbe_test_esp_wait_for_sps_test_to_complete ()
 *********************************************************************
 *  @brief
 *    This function poll the ESP for the SPS status info
 *    to check whether the SPS test has completed.
 * 
 *  @param spsIndex - 0 for local and 1 for peer
 *  @param timeout_ms - timeout value in mili-second
 *
 *  @return fbe_status_t
 *
 *  @version
 *    06-April-2011: PHE - Created
 *    
 *********************************************************************/
fbe_status_t fbe_test_esp_wait_for_sps_test_to_complete(fbe_bool_t xpeArray,
                                                        fbe_u8_t spsIndex, 
                                                        fbe_u32_t timeoutMs)
{
    fbe_status_t                         status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                            currentTime = 0;
    fbe_esp_sps_mgmt_spsStatus_t         spsStatusInfo = {0};

    while (currentTime < timeoutMs)
    {
        fbe_zero_memory(&spsStatusInfo, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
        if (xpeArray)
        {
            spsStatusInfo.spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
            spsStatusInfo.spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        }
        spsStatusInfo.spsLocation.slot = spsIndex;

        status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo);
 
        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                          "%s: Get fup completion status returned with status 0x%X.\n",
                          __FUNCTION__, status); 
            return status;
        }
            
        if((spsStatusInfo.spsModuleInserted == TRUE) &&
           (spsStatusInfo.spsStatus == SPS_STATE_AVAILABLE) &&
           (spsStatusInfo.spsCablingStatus != FBE_SPS_CABLING_UNKNOWN))
        {
            return status;
        }
        
        currentTime = currentTime + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: wait for spsStatusInfo inserted %d, spsStatus %d, spsCablingStatus %d failed within the expected %d ms!\n",
                  __FUNCTION__,
                  spsStatusInfo.spsModuleInserted,
                  spsStatusInfo.spsStatus,
                  spsStatusInfo.spsCablingStatus,
                  timeoutMs);

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!**************************************************************
 * fbe_test_esp_create_registry_image_path(fbe_u64_t deviceType) 
 ****************************************************************
 * @brief
 *  Creates the image path in the registry for the specified device type.
 *  Need a seperate path created for cdes and cdes11 imge types.
 *  Cooling modules (fans) are cdes11 only.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  22-Jul-2010 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_test_esp_create_registry_image_path(fbe_u64_t deviceType)
{
    fbe_u32_t index = 0;
    fbe_test_esp_fup_image_param_t * pImageParam = NULL;
    /* used to validate the registry write. */
    fbe_u8_t  regReadSzKeyValue[FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH] = {0};  

    mut_printf(MUT_LOG_LOW, "=== Creating CDES %s registry file and image path key ===",
               fbe_base_env_decode_device_type(deviceType));

    
    // cdes registry (viper, derringer, etc)
    for(index = 0; index < FBE_TEST_ESP_FUP_IMAGE_MAX; index ++) 
    {
        if(test_esp_fup_image_param_table[index].deviceType == deviceType)
        {
            pImageParam = &test_esp_fup_image_param_table[index];
        
            fbe_test_esp_registry_write(K10DriverConfigRegPath,
                               pImageParam->pImagePathKey, 
                               FBE_REGISTRY_VALUE_SZ, 
                               pImageParam->pImagePathValue,
                               0);   
             
            fbe_test_esp_registry_read(K10DriverConfigRegPath,
                              pImageParam->pImagePathKey, 
                              &regReadSzKeyValue[0], 
                              FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH, 
                              FBE_REGISTRY_VALUE_SZ,
                              NULL,
                              0,
                              FALSE);
            MUT_ASSERT_STRING_EQUAL(&regReadSzKeyValue[0], 
                                    pImageParam->pImagePathValue);
        }
    }
    
    mut_printf(MUT_LOG_LOW, "=== Creating CDES11 %s registry file and image path key ===",
               fbe_base_env_decode_device_type(deviceType));

    //cdes11 registry (voyager)
    for(index = 0; index < FBE_TEST_ESP_FUP_CDES11_IMAGE_MAX; index ++) 
    {
        if(test_esp_fup_cdes11_image_param_table[index].deviceType == deviceType)
        {
            pImageParam = &test_esp_fup_cdes11_image_param_table[index];
            
            MUT_ASSERT_TRUE(pImageParam != NULL);
        
            fbe_test_esp_registry_write(K10DriverConfigRegPath,
                               pImageParam->pImagePathKey, 
                               FBE_REGISTRY_VALUE_SZ, 
                               pImageParam->pImagePathValue,
                               0);   
        
            fbe_test_esp_registry_read(K10DriverConfigRegPath,
                              pImageParam->pImagePathKey, 
                              &regReadSzKeyValue[0], 
                              FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH, 
                              FBE_REGISTRY_VALUE_SZ,
                              NULL,
                              0,
                              FALSE);
        
            MUT_ASSERT_STRING_EQUAL(&regReadSzKeyValue[0], 
                                    pImageParam->pImagePathValue);
        }
    }

    mut_printf(MUT_LOG_LOW, "=== Creating CDES Unified %s registry file and image path key ===",
               fbe_base_env_decode_device_type(deviceType));

    //cdes unified registry - Viking
    for(index = 0; index < FBE_TEST_ESP_FUP_CDES_UNI_IMAGE_MAX; index ++) 
    {
        if(test_esp_fup_cdes_unified_image_param_table[index].deviceType == deviceType)
        {
            pImageParam = &test_esp_fup_cdes_unified_image_param_table[index];
            
            MUT_ASSERT_TRUE(pImageParam != NULL);
        
            fbe_test_esp_registry_write(K10DriverConfigRegPath,
                               pImageParam->pImagePathKey, 
                               FBE_REGISTRY_VALUE_SZ, 
                               pImageParam->pImagePathValue,
                               0);   
        
            fbe_test_esp_registry_read(K10DriverConfigRegPath,
                              pImageParam->pImagePathKey, 
                              &regReadSzKeyValue[0], 
                              FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH, 
                              FBE_REGISTRY_VALUE_SZ,
                              NULL,
                              0,
                              FALSE);
        
            MUT_ASSERT_STRING_EQUAL(&regReadSzKeyValue[0], 
                                    pImageParam->pImagePathValue);
        }
    }

    mut_printf(MUT_LOG_LOW, "=== Creating JDES %s registry file and image path key ===\n",
               fbe_base_env_decode_device_type(deviceType));

    //jdes registry (jetfire)
    for(index = 0; index < FBE_TEST_ESP_FUP_JDES_IMAGE_MAX; index ++) 
    {
        if(test_esp_fup_jdes_image_param_table[index].deviceType == deviceType)
        {
            pImageParam = &test_esp_fup_jdes_image_param_table[index];
            
            MUT_ASSERT_TRUE(pImageParam != NULL);
        
            fbe_test_esp_registry_write(K10DriverConfigRegPath,
                               pImageParam->pImagePathKey, 
                               FBE_REGISTRY_VALUE_SZ, 
                               pImageParam->pImagePathValue,
                               0);   
        
            fbe_test_esp_registry_read(K10DriverConfigRegPath,
                              pImageParam->pImagePathKey, 
                              &regReadSzKeyValue[0], 
                              FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH, 
                              FBE_REGISTRY_VALUE_SZ,
                              NULL,
                              0,
                              FALSE);
        
            MUT_ASSERT_STRING_EQUAL(&regReadSzKeyValue[0], 
                                    pImageParam->pImagePathValue);
        }
    }

    mut_printf(MUT_LOG_LOW, "=== Creating CDES-2 %s registry file and image path key ===",
               fbe_base_env_decode_device_type(deviceType));

    //cdes-2 registry - 12G DAEs and 12G DPEs
    for(index = 0; index < FBE_TEST_ESP_FUP_CDES_2_IMAGE_MAX; index ++) 
    {
        if(test_esp_fup_cdes_2_image_param_table[index].deviceType == deviceType)
        {
            pImageParam = &test_esp_fup_cdes_2_image_param_table[index];
            
            MUT_ASSERT_TRUE(pImageParam != NULL);
        
            fbe_test_esp_registry_write(K10DriverConfigRegPath,
                               pImageParam->pImagePathKey, 
                               FBE_REGISTRY_VALUE_SZ, 
                               pImageParam->pImagePathValue,
                               0);   
        
            fbe_test_esp_registry_read(K10DriverConfigRegPath,
                              pImageParam->pImagePathKey, 
                              &regReadSzKeyValue[0], 
                              FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH, 
                              FBE_REGISTRY_VALUE_SZ,
                              NULL,
                              0,
                              FALSE);
        
            MUT_ASSERT_STRING_EQUAL(&regReadSzKeyValue[0], 
                                    pImageParam->pImagePathValue);
        }
    }

    MUT_ASSERT_TRUE(pImageParam != NULL);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_test_esp_create_image_file(fbe_u64_t deviceType) 
 ****************************************************************
 * @brief
 *  Creates the image file for the specified device type.
 *  The header of the file is valid and
 *  the rest of the bytes in the image file are all 0s because
 *  we don't care what's in the image other than the header.
 *
 * @param pImagePathAndName
 * @param imagePathAndNameLength 
 * @param deviceType
 * 
 * @return fbe_status_t.
 *
 * @author
 *  22-Jul-2010 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_test_esp_create_image_file(fbe_u64_t deviceType)
{
    fbe_u32_t            index = 0;
    fbe_test_esp_fup_image_param_t * pImageParam = NULL;
    fbe_u8_t            *pImageHeader = NULL;

    mut_printf(MUT_LOG_LOW, "=== Creating CDES %s image file ===", 
               fbe_base_env_decode_device_type(deviceType));

    // create the cdes type image (viper, derringer, etc)
    for(index = 0; index < FBE_TEST_ESP_FUP_IMAGE_MAX; index ++) 
    {
        // since there can be multiple images for a device type, stay 
        // in this loop when creating the image
        if(test_esp_fup_image_param_table[index].deviceType == deviceType)
        {
            pImageParam = &test_esp_fup_image_param_table[index];
            pImageHeader = cdes_image_headers[index];
            fbe_esp_test_make_image(pImageParam, pImageHeader, deviceType);
        }
    }

    mut_printf(MUT_LOG_LOW, "=== Creating CDES11 %s image file ===", 
               fbe_base_env_decode_device_type(deviceType));

    // create the cdes11 type image (not just voyager)
    for(index = 0; index < FBE_TEST_ESP_FUP_CDES11_IMAGE_MAX; index ++) 
    {
        if(test_esp_fup_cdes11_image_param_table[index].deviceType == deviceType)
        {
            pImageParam = &test_esp_fup_cdes11_image_param_table[index];
            pImageHeader = cdes_1_1_image_headers[index];
            fbe_esp_test_make_image(pImageParam, pImageHeader, deviceType);
        }
    }

    mut_printf(MUT_LOG_LOW, "=== Creating CDES Unified %s image file ===", 
               fbe_base_env_decode_device_type(deviceType));

    // create the cdes unified type image 
    for(index = 0; index < FBE_TEST_ESP_FUP_CDES_UNI_IMAGE_MAX; index ++) 
    {
        // since there can be multiple images for a device type, stay 
        // in this loop when creating the image
        if(test_esp_fup_cdes_unified_image_param_table[index].deviceType == deviceType)
        {
            pImageParam = &test_esp_fup_cdes_unified_image_param_table[index];
            pImageHeader = cdes_unified_image_headers[index];
            fbe_esp_test_make_image(pImageParam, pImageHeader, deviceType);
        }
    }

    mut_printf(MUT_LOG_LOW, "=== Creating JDES %s image file ===", 
               fbe_base_env_decode_device_type(deviceType));

    // create the jdes type image (for jetfire)
    for(index = 0; index < FBE_TEST_ESP_FUP_JDES_IMAGE_MAX; index ++) 
    {
        if(test_esp_fup_jdes_image_param_table[index].deviceType == deviceType)
        {
            pImageParam = &test_esp_fup_jdes_image_param_table[index];
            pImageHeader = jdes_image_headers[index];
            fbe_esp_test_make_image(pImageParam, pImageHeader, deviceType);
        }
    }    

    mut_printf(MUT_LOG_LOW, "=== Creating CDES-2 %s image file ===", 
               fbe_base_env_decode_device_type(deviceType));

    // create the CDES-2 image (for 12G DAEs and 12G DPEs)
    for(index = 0; index < FBE_TEST_ESP_FUP_CDES_2_IMAGE_MAX; index ++) 
    {
        // since there can be multiple images for a device type, stay 
        // in this loop when creating the image
        if(test_esp_fup_cdes_2_image_param_table[index].deviceType == deviceType)
        {
            pImageParam = &test_esp_fup_cdes_2_image_param_table[index];
            pImageHeader = cdes_2_image_headers[index];
            fbe_esp_test_cdes_2_make_image(pImageParam, pImageHeader, test_esp_fup_cdes_2_image_param_table[index].fwTarget);
        }
    } 
    MUT_ASSERT_TRUE(pImageParam != NULL);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_esp_test_make_image(fbe_u64_t deviceType) 
 ****************************************************************
 * @brief
 *  Creates the image file for the specified device type.
 *  Update the header information and create the file based on
 *  image and path name.
 *
 * @param pImageParam
 * @param pImageHeader 
 * @param deviceType
 * 
 * @return fbe_status_t.
 *
 * @author
 *  05-Dec-2011 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_esp_test_make_image(fbe_test_esp_fup_image_param_t *pImageParam, fbe_u8_t *pImageHeader, fbe_u64_t deviceType)
{
    fbe_u32_t            imageSize = 0;
    fbe_u32_t            tempImageSize = 0;
    fbe_file_handle_t    fileHandle;
    fbe_char_t         * pFirmwareRev = NULL;
    fbe_u8_t           * pImage = NULL;
    fbe_char_t           imagePathAndName[FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH] = "\0";
    fbe_u32_t            status = FBE_STATUS_OK;
    fbe_u8_t            idIndex;
    fbe_u8_t            maxIds;

    imageSize = pImageParam->imageSize; 
    pImage = (fbe_u8_t *)malloc(imageSize * sizeof(fbe_u8_t));
    memset(pImage, 0, imageSize);

    switch(deviceType) 
    {
    case FBE_DEVICE_TYPE_PS:
        pFirmwareRev = " 988 ";
        break;

    case FBE_DEVICE_TYPE_LCC:
        if(pImageHeader[32] != 0)
        {
            pFirmwareRev = " 999 ";
        }
        else
        {
            pFirmwareRev = "1599 ";
        }
        break;
        
    case FBE_DEVICE_TYPE_FAN:
        pFirmwareRev = " 899 "; //&fanInfo.firmwareRev[0];
        break;

    case FBE_DEVICE_TYPE_SPS:
        pFirmwareRev = " 887 ";
        break;

    case FBE_DEVICE_TYPE_SP:
        pFirmwareRev = " 998 ";
        break;

    case FBE_DEVICE_TYPE_BACK_END_MODULE:
        pFirmwareRev = "1499 ";
        break;
        
    default:
        mut_printf(MUT_LOG_LOW, "Unsupported device type %lld", deviceType); 
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    // copy the real header into the allocated space
    fbe_copy_memory(pImage,
                    pImageHeader, 
                    FBE_ENCLOSURE_MAX_IMAGE_HEADER_SIZE);
    // update the revision field and the image size field

    /* Populate the image rev in the image header. */
    fbe_copy_memory(pImage + FBE_ESES_MCODE_IMAGE_REV_OFFSET, 
                    pFirmwareRev, 
                    FBE_ESES_FW_REVISION_1_0_SIZE);
    /* Populate the image size in the image header. */
    tempImageSize = swap32(imageSize);
    fbe_copy_memory(pImage + FBE_ESES_MCODE_IMAGE_SIZE_OFFSET,
                    &tempImageSize,
                    FBE_ESES_MCODE_IMAGE_SIZE_BYTES);

    fbe_test_esp_build_image_file_path_and_name(&imagePathAndName[0],
                                                FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH,
                                                pImageParam->pImagePathAndNameHeader);

    fileHandle = fbe_file_creat(&imagePathAndName[0], FBE_FILE_RDWR);

    status = fbe_file_write(fileHandle, pImage, imageSize, NULL);
    MUT_ASSERT_TRUE_MSG(status == imageSize, "Failed to write to the image file.");

    status = fbe_file_close(fileHandle);
    MUT_ASSERT_TRUE_MSG(status == 0, "Failed to close the image file.");

    mut_printf(MUT_LOG_LOW, "Image file created with rev:%.5s, imageSize:0x%X", 
                            (pImage + FBE_ESES_MCODE_IMAGE_REV_OFFSET), 
                            //(pImage + FBE_ESES_MCODE_SUBENCL_PRODUCT_ID_OFFSET),
                             imageSize);

    maxIds = pImage[FBE_ESES_MCODE_NUM_SUBENCL_PRODUCT_ID_OFFSET];
    mut_printf(MUT_LOG_LOW, "  %d Product Ids from image file:", maxIds);
    for(idIndex = 0; idIndex < maxIds; idIndex++)
    {
        mut_printf(MUT_LOG_LOW, "     %s", 
                  (pImage + FBE_ESES_MCODE_SUBENCL_PRODUCT_ID_OFFSET + (idIndex * FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE)));
    }
    free(pImage);
    
    return status;
}


/*!**************************************************************
 * fbe_esp_test_cdes_2_make_image(fbe_u64_t deviceType) 
 ****************************************************************
 * @brief
 *  Creates the image file for the specified device type.
 *  Update the header information and create the file based on
 *  image and path name.
 *
 * @param pImageParam
 * @param pImageHeader 
 * @param deviceType
 * 
 * @return fbe_status_t.
 *
 * @author
 *  21-Aug-2014 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_esp_test_cdes_2_make_image(fbe_test_esp_fup_image_param_t *pImageParam, 
                                            fbe_u8_t *pImageHeader, 
                                            fbe_enclosure_fw_target_t fwTarget)
{
    fbe_u32_t            imageSize = 0;
    fbe_file_handle_t    fileHandle;
    fbe_char_t         * pFirmwareRev = NULL;
    fbe_u8_t           * pImage = NULL;
    fbe_char_t           imagePathAndName[FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH] = "\0";
    fbe_u32_t            status = FBE_STATUS_OK;
    fbe_u8_t            idIndex;
    fbe_u8_t            maxIds;
    fbe_u32_t           imageHeaderSize;
    fbe_u32_t           tempImageHeaderSize;
    fbe_u32_t           imageDataSize;
    fbe_u32_t           tempImageDataSize;

    imageSize = pImageParam->imageSize; 
    imageHeaderSize = 400;  // Random value.
    imageDataSize = imageSize - imageHeaderSize;
    pImage = (fbe_u8_t *)malloc(imageSize * sizeof(fbe_u8_t));
    memset(pImage, 0, imageSize);

    switch(fwTarget) 
    {
    case FBE_FW_TARGET_LCC_FPGA:
        pFirmwareRev = "00.77";
        break;

    case FBE_FW_TARGET_LCC_INIT_STRING:
        pFirmwareRev = "0.88.0";
        break;
        
    case FBE_FW_TARGET_LCC_EXPANDER:
        pFirmwareRev = "5.99.0";
        break;

    case FBE_FW_TARGET_PS:
        pFirmwareRev = " 988 ";
        break;
    
    default:
        mut_printf(MUT_LOG_LOW, "Unsupported fwTarget %d", fwTarget); 
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    // copy the real header into the allocated space
    fbe_copy_memory(pImage,
                    pImageHeader, 
                    FBE_ENCLOSURE_MAX_IMAGE_HEADER_SIZE);
    // update the revision field and the image size field

    /* Populate the image rev in the image header. */
    fbe_copy_memory(pImage + FBE_ESES_CDES2_MCODE_IMAGE_REV_OFFSET, 
                    pFirmwareRev, 
                    FBE_ESES_FW_REVISION_2_0_SIZE);

    /* Populate the image header size in the image header. */
    tempImageHeaderSize = swap32(imageHeaderSize);
    fbe_copy_memory(pImage + FBE_ESES_CDES2_MCODE_IMAGE_HEADER_SIZE_OFFSET,
                    &tempImageHeaderSize,
                    FBE_ESES_CDES2_MCODE_IMAGE_HEADER_SIZE_BYTES);

    /* Populate the image data size in the image header. */
    tempImageDataSize = swap32(imageDataSize);
    fbe_copy_memory(pImage + FBE_ESES_CDES2_MCODE_DATA_SIZE_OFFSET,
                    &tempImageDataSize,
                    FBE_ESES_CDES2_MCODE_DATA_SIZE_BYTES);

    fbe_test_esp_build_image_file_path_and_name(&imagePathAndName[0],
                                                FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH,
                                                pImageParam->pImagePathAndNameHeader);

    fileHandle = fbe_file_creat(&imagePathAndName[0], FBE_FILE_RDWR);

    status = fbe_file_write(fileHandle, pImage, imageSize, NULL);
    MUT_ASSERT_TRUE_MSG(status == imageSize, "Failed to write to the image file.");

    status = fbe_file_close(fileHandle);
    MUT_ASSERT_TRUE_MSG(status == 0, "Failed to close the image file.");

    mut_printf(MUT_LOG_LOW, "%s Image file created with rev:%.16s, imageSize:0x%X", 
                             fbe_base_env_decode_firmware_target(fwTarget),
                            (pImage + FBE_ESES_CDES2_MCODE_IMAGE_REV_OFFSET), 
                            //(pImage + FBE_ESES_MCODE_SUBENCL_PRODUCT_ID_OFFSET),
                             imageSize);

    maxIds = pImage[FBE_ESES_CDES2_MCODE_NUM_SUBENCL_PRODUCT_ID_OFFSET];
    mut_printf(MUT_LOG_LOW, "  %d Product Ids from image file:", maxIds);
    for(idIndex = 0; idIndex < maxIds; idIndex++)
    {
        mut_printf(MUT_LOG_LOW, "     %s", 
                  (pImage + FBE_ESES_CDES2_MCODE_SUBENCL_PRODUCT_ID_OFFSET + (idIndex * FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE)));
    }
    free(pImage);
    
    return status;
}

/*!**************************************************************
 * fbe_test_esp_delete_image_file(fbe_u64_t deviceType) 
 ****************************************************************
 * @brief
 *  Deletes the image for the specified device type. 
 *
 * @param deviceType.               
 *
 * @return None.
 *
 * @author
 *  22-Jul-2010 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_test_esp_delete_image_file(fbe_u64_t deviceType)
{
    fbe_u32_t                       index = 0;
    fbe_u32_t                       maxImages = FBE_TEST_ESP_FUP_IMAGE_MAX;
    fbe_test_esp_fup_image_param_t * pImageParam = NULL;
    fbe_char_t                      imagePathAndName[FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH] = "\0";

    mut_printf(MUT_LOG_LOW, "=== Deleting %s image file ===", 
               fbe_base_env_decode_device_type(deviceType));
    pImageParam = NULL;

    if(maxImages < FBE_TEST_ESP_FUP_CDES11_IMAGE_MAX)
    {
        maxImages = FBE_TEST_ESP_FUP_CDES11_IMAGE_MAX;
    }
    for(index = 0; index < FBE_TEST_ESP_FUP_CDES11_IMAGE_MAX; index ++) 
    {
        // delete the cdes_1_1 image
        if(test_esp_fup_cdes11_image_param_table[index].deviceType == deviceType)
        {
            pImageParam = &test_esp_fup_cdes11_image_param_table[index];
            fbe_test_esp_build_image_file_path_and_name(&imagePathAndName[0],
                                                FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH,
                                                pImageParam->pImagePathAndNameHeader);

            fbe_file_delete(&imagePathAndName[0]);
        }
    }
    for(index = 0; index < FBE_TEST_ESP_FUP_IMAGE_MAX; index ++) 
    {
        // delete the cdes image
        if(test_esp_fup_image_param_table[index].deviceType == deviceType)
        {
            memset(&imagePathAndName[0], 0, FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH);
            pImageParam = &test_esp_fup_image_param_table[index];
            fbe_test_esp_build_image_file_path_and_name(&imagePathAndName[0],
                                                        FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH,
                                                        pImageParam->pImagePathAndNameHeader);

            fbe_file_delete(&imagePathAndName[0]);
            memset(&imagePathAndName[0], 0, FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH);
        }
    }
    for(index = 0; index < FBE_TEST_ESP_FUP_JDES_IMAGE_MAX; index ++) 
    {
        // delete the jdes image
        if(test_esp_fup_jdes_image_param_table[index].deviceType == deviceType)
        {
            memset(&imagePathAndName[0], 0, FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH);
            pImageParam = &test_esp_fup_jdes_image_param_table[index];
            fbe_test_esp_build_image_file_path_and_name(&imagePathAndName[0],
                                                        FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH,
                                                        pImageParam->pImagePathAndNameHeader);

            fbe_file_delete(&imagePathAndName[0]);
            memset(&imagePathAndName[0], 0, FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH);
        }
    }

    for(index = 0; index < FBE_TEST_ESP_FUP_CDES_UNI_IMAGE_MAX; index ++) 
    {
        // delete the CDES unified images
        if(test_esp_fup_cdes_unified_image_param_table[index].deviceType == deviceType)
        {
            memset(&imagePathAndName[0], 0, FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH);
            pImageParam = &test_esp_fup_cdes_unified_image_param_table[index];
            fbe_test_esp_build_image_file_path_and_name(&imagePathAndName[0],
                                                        FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH,
                                                        pImageParam->pImagePathAndNameHeader);

            fbe_file_delete(&imagePathAndName[0]);
            memset(&imagePathAndName[0], 0, FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH);
        }
    }
    
    for(index = 0; index < FBE_TEST_ESP_FUP_CDES_2_IMAGE_MAX; index ++) 
    {
        // delete the CDES-2 images
        if(test_esp_fup_cdes_2_image_param_table[index].deviceType == deviceType)
        {
            memset(&imagePathAndName[0], 0, FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH);
            pImageParam = &test_esp_fup_cdes_2_image_param_table[index];
            fbe_test_esp_build_image_file_path_and_name(&imagePathAndName[0],
                                                        FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH,
                                                        pImageParam->pImagePathAndNameHeader);

            fbe_file_delete(&imagePathAndName[0]);
            memset(&imagePathAndName[0], 0, FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH);
        }
    }

    if(pImageParam == NULL)
    {
        mut_printf(MUT_LOG_LOW, "=== %s image file not present!", 
                   fbe_base_env_decode_device_type(deviceType));
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_test_esp_create_manifest_file(fbe_u64_t deviceType) 
 ****************************************************************
 * @brief
 *  Creates the manifest file for the specified device type.
 *
 * @param deviceType
 * 
 * @return fbe_status_t.
 *
 * @author
 *  12-Sep-2014 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_test_esp_create_manifest_file(fbe_u64_t deviceType)
{
    fbe_u32_t            manifestFileSize = 0;
    fbe_file_handle_t    fileHandle;
    fbe_char_t           manifestFilePathAndName[FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH] = "\0";
    fbe_u32_t            status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "=== Creating CDES-2 %s manifest file ===", 
    fbe_base_env_decode_device_type(deviceType));
    
    if(deviceType == FBE_DEVICE_TYPE_LCC) 
    {
        manifestFileSize = (fbe_u32_t)strlen(lcc_manifest_str);
        fbe_test_esp_build_manifest_file_path_and_name(&manifestFilePathAndName[0],
                                                    FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH,
                                                    FBE_TEST_ESP_FUP_CDES_2_LCC_MANIFEST_FILE_PATH_AND_NAME);

        fileHandle = fbe_file_creat(&manifestFilePathAndName[0], FBE_FILE_RDWR);
    
        status = fbe_file_write(fileHandle, lcc_manifest_str, manifestFileSize, NULL);
        MUT_ASSERT_TRUE_MSG(status == manifestFileSize, "Failed to write to the manifest file.");
    
        status = fbe_file_close(fileHandle);
        MUT_ASSERT_TRUE_MSG(status == 0, "Failed to close the manifest file.");
    }
    else if(deviceType == FBE_DEVICE_TYPE_PS)
    {
        manifestFileSize = (fbe_u32_t)strlen(ps_manifest_str);
        fbe_test_esp_build_manifest_file_path_and_name(&manifestFilePathAndName[0],
                                                    FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH,
                                                    FBE_TEST_ESP_FUP_CDES_2_PS_MANIFEST_FILE_PATH_AND_NAME);

        fileHandle = fbe_file_creat(&manifestFilePathAndName[0], FBE_FILE_RDWR);
    
        status = fbe_file_write(fileHandle, ps_manifest_str, manifestFileSize, NULL);
        MUT_ASSERT_TRUE_MSG(status == manifestFileSize, "Failed to write to the manifest file.");
    
        status = fbe_file_close(fileHandle);
        MUT_ASSERT_TRUE_MSG(status == 0, "Failed to close the manifest file.");
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "Can not create %s manifest file!", 
                             fbe_base_env_decode_device_type(deviceType));

        return FBE_STATUS_GENERIC_FAILURE;
    }

    mut_printf(MUT_LOG_LOW, "%s manifest file created, fileSize:0x%X", 
                             fbe_base_env_decode_device_type(deviceType),
                             manifestFileSize);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_test_esp_delete_image_file(fbe_u64_t deviceType) 
 ****************************************************************
 * @brief
 *  Deletes the image for the specified device type. 
 *
 * @param deviceType.               
 *
 * @return None.
 *
 * @author
 *  22-Jul-2010 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_test_esp_delete_manifest_file(fbe_u64_t deviceType)
{
    fbe_char_t           manifestFilePathAndName[FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH] = "\0";

    mut_printf(MUT_LOG_LOW, "=== Deleting CDES-2 %s manifest file ===", 
               fbe_base_env_decode_device_type(deviceType));

    if(deviceType == FBE_DEVICE_TYPE_LCC) 
    {
        fbe_test_esp_build_manifest_file_path_and_name(&manifestFilePathAndName[0],
                                                    FBE_TEST_ESP_FUP_IMAGE_PATH_AND_NAME_LENGTH,
                                                    FBE_TEST_ESP_FUP_CDES_2_LCC_MANIFEST_FILE_PATH_AND_NAME);

        fbe_file_delete(&manifestFilePathAndName[0]);
    }
    else if(deviceType == FBE_DEVICE_TYPE_PS)
    {
        /* delete the PS manifest file here. */
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "Can not delete %s manifest file!", 
                             fbe_base_env_decode_device_type(deviceType));

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

 /*!*************************************************************
 *  fbe_test_esp_check_mgmt_config_completion_event() 
 ****************************************************************
 * @brief
 *  This function check for Mgmt port speed completion event
 *
 * @param None.
 *
 * @return FBE_STATUS_OK - if event message is present.
 *
 * @author
 *  2-June-2011 - Created. Vaibhav Gaonkar
 *
 ****************************************************************/ 
fbe_status_t
fbe_test_esp_check_mgmt_config_completion_event()
{
    fbe_status_t    status;
    fbe_u32_t   currentTime = 0;
    fbe_bool_t  isMsgPresent = FBE_FALSE;

    while(currentTime < 45000)
    {
        /* Check for event message */
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                             &isMsgPresent,
                                             ESP_INFO_MGMT_PORT_CONFIG_COMPLETED,
                                            "SPA management module 0");
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        if (isMsgPresent == FBE_TRUE){
            return FBE_STATUS_OK;
        }
        currentTime = currentTime + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
     }
    return FBE_STATUS_GENERIC_FAILURE;
}
/**********************************************************
 *  end of fbe_test_esp_check_mgmt_config_completion_event
 *********************************************************/

void fbe_test_esp_cru_off(fbe_device_physical_location_t *pLocation,
                          fbe_u32_t lastSlot)
{
    fbe_status_t                        status;
    fbe_object_id_t                     pdo_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                     object_id = FBE_OBJECT_ID_INVALID;

    // get DRIVE Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // make sure DMO is ready
    status = fbe_api_wait_for_object_lifecycle_state(object_id,FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // Get pdo object ID
    status = fbe_api_get_physical_drive_object_id_by_location(pLocation->bus, 
                                                              pLocation->enclosure, 
                                                              pLocation->slot, 
                                                              &pdo_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(pdo_object_id != FBE_OBJECT_ID_INVALID);


    //send comamnd to remove the drive
    status = fbe_api_esp_drive_mgmt_send_debug_control(pLocation->bus, 
                                                       pLocation->enclosure, 
                                                       pLocation->slot, 
                                                       lastSlot, 
                                                       FBE_DRIVE_ACTION_REMOVE,
                                                       FBE_FALSE, FBE_FALSE);


    // wait for the pdo to transition from ready state
    status = fbe_wait_for_object_to_transiton_from_lifecycle_state(pdo_object_id,
                                                                   FBE_LIFECYCLE_STATE_READY,
                                                                   20000,
                                                                   FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    

    return;
}

void fbe_test_esp_cru_on(fbe_device_physical_location_t *pLocation,
                         fbe_u32_t lastSlot)
{
    fbe_status_t                        status;

    //send comamnd to insert the drive
    status = fbe_api_esp_drive_mgmt_send_debug_control(pLocation->bus, 
                                                       pLocation->enclosure, 
                                                       pLocation->slot, 
                                                       lastSlot, 
                                                       FBE_DRIVE_ACTION_INSERT,
                                                       FBE_FALSE, FBE_FALSE);


    return;
}


/*!**************************************************************
*          fbe_test_esp_util_get_cmd_option()
****************************************************************
* @brief
*  Determine if an option is set.
*
* @param option_name_p - name of option to get
*
* @return fbe_bool_t - FBE_TRUE if value found FBE_FALSE otherwise.
*
****************************************************************/
fbe_bool_t fbe_test_esp_util_get_cmd_option(char *option_name_p)
{
    if (mut_option_selected(option_name_p))
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
