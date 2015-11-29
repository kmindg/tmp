#define I_AM_NATIVE_CODE
#include <windows.h>
#include "mut.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_service_manager_interface.h"
#include "fbe_trace.h"
#include "sep_dll.h"
#include "esp_dll.h"
#include "neit_dll.h"
#include "kms_dll.h"
#include "physical_package_dll.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_neit.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_registry.h"
#include "fbe_test_package_config.h"

#define FBE_TEST_ICA_STAMP_SYS_DRIVE_COUNT  3
#define FBE_TEST_FIRMWARE_LUN_SYS_DRIVE_COUNT  3

#define FBE_MAX_REGISTRY_STRING_LEN 256


#include "EmcUTIL_CsxShell_Interface.h"
#include "EmcUTIL_DllLoader_Interface.h"

/*the context for zeroing firmware lun regions on the raw disks*/
typedef struct fbe_test_util_zero_firmware_lun_context_s
{
    fbe_semaphore_t semaphore; /*synchronize ios sent to different ldos*/
    fbe_spinlock_t  lock; /*protect the followinng three members*/
    fbe_u32_t        total_complete_count; /*how many packets are completed*/
    fbe_u32_t        success_complete_count; /*how many packets are successfully completed*/
    fbe_u32_t        complete_count_limit; /*how many packets are sent out totally*/
}fbe_test_util_zero_firmware_lun_context_t;


static EMCUTIL_DLL_LOADER_PARAMS sep_handle;               /* Handle to SEP DLL */
fbe_service_control_entry_t sep_control_entry = NULL;
fbe_io_entry_function_t sep_io_entry = NULL;

static EMCUTIL_DLL_LOADER_PARAMS esp_handle;               /* Handle to ESP DLL */
fbe_service_control_entry_t esp_control_entry = NULL;
fbe_io_entry_function_t esp_io_entry = NULL;

static EMCUTIL_DLL_LOADER_PARAMS physical_package_handle;               /* Handle to PP DLL */
fbe_service_control_entry_t physical_package_control_entry = NULL;
fbe_io_entry_function_t		physical_package_io_entry = NULL;

static EMCUTIL_DLL_LOADER_PARAMS neit_handle;               /* Handle to NEIT DLL */
fbe_service_control_entry_t neit_package_control_entry = NULL;

static EMCUTIL_DLL_LOADER_PARAMS kms_handle;               /* Handle to KMS DLL */
fbe_service_control_entry_t kms_control_entry = NULL;
fbe_io_entry_function_t kms_io_entry = NULL;

fbe_sep_shim_sim_map_device_name_to_block_edge_func_t  lun_map_entry = NULL;
fbe_sep_shim_sim_cache_io_entry_t sep_shim_cache_io_entry = NULL;

fbe_bool_t is_ica_stamp_generated = FBE_FALSE;

static fbe_status_t fbe_test_utils_generate_ica_stamp(void);

static fbe_status_t fbe_test_utils_zero_firmware_luns_completion(fbe_packet_t * packet_p,
                                                               fbe_packet_completion_context_t context);

static fbe_status_t fbe_test_load_kms(fbe_kms_package_load_params_t *params_p);

static csx_pvoid_t fbe_test_lookup_symbol(PEMCUTIL_DLL_LOADER_PARAMS loader_params, csx_string_t symbol_name)
{
    csx_status_e status;
    csx_pvoid_t symbol_ptr;
    status = emcutil_dll_loader_lookup(symbol_name, &symbol_ptr, loader_params);
    if (CSX_SUCCESS(status)) {
        return symbol_ptr;
    } else {
        return CSX_NULL;
    }
}

static void fbe_test_unload_wait(fbe_package_id_t package)
{
#ifdef C4_INTEGRATED
    unsigned int wait_time = 5000;

    printf("%s: ***** Waiting %ums before unloading package %u *****\n", __FUNCTION__, wait_time, (fbe_u32_t)package);
    printf("%s: ***** We should remove this waiting *****\n", __FUNCTION__);
    fbe_api_sleep(wait_time); /* SAFEBUG - work around races where code in DLL is still cleaning up! */
#else
    /* We needn't wait in rockies, or we may hide some big problems */
#endif
}

fbe_status_t fbe_test_load_physical_package(void)
{
    fbe_status_t status;
    csx_status_e csx_status;
    physical_package_dll_init_function_t		physical_package_dll_init_function = NULL;
    physical_package_dll_get_control_entry_function_t physical_package_dll_get_control_entry_function = NULL;
    physical_package_dll_get_io_entry_function_t physical_package_dll_get_io_entry_function = NULL;
    physical_package_dll_get_specl_sfi_entry_function_t physical_package_dll_get_specl_sfi_entry_function = NULL;
    physical_package_dll_set_miniport_interface_port_shim_sim_pointers_function_t physical_package_dll_set_miniport_interface_port_shim_sim_pointers = NULL;
    physical_package_dll_set_terminator_api_get_board_info_function_t physical_package_dll_set_terminator_api_get_board_info_function = NULL;
    physical_package_dll_set_terminator_api_get_sp_id_function_t physical_package_dll_set_terminator_api_get_sp_id_function = NULL;
    physical_package_dll_set_terminator_api_is_single_sp_system_function_t physical_package_dll_set_terminator_api_is_single_sp_system_function = NULL;
    physical_package_dll_set_terminator_api_process_specl_sfi_mask_data_queue_function_t physical_package_dll_set_terminator_api_process_specl_sfi_mask_data_queue_function = NULL;
    speclSFIEntryType specl_sfi_entry = NULL; // PP dll does not keep SPECL SFI entry, so be a local variable

    fbe_terminator_miniport_interface_port_shim_sim_pointers_t miniport_pointers;

    csx_status = emcutil_dll_loader_load("PhysicalPackage", CSX_FALSE, &physical_package_handle);
    if (CSX_FAILURE(csx_status))
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

    physical_package_dll_get_control_entry_function = (physical_package_dll_get_control_entry_function_t)fbe_test_lookup_symbol(&physical_package_handle, "physical_package_dll_get_control_entry");
    if (physical_package_dll_get_control_entry_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}
    status = physical_package_dll_get_control_entry_function(&physical_package_control_entry);
    if ((physical_package_control_entry == NULL)||(status != FBE_STATUS_OK))
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}


    physical_package_dll_get_io_entry_function = (physical_package_dll_get_io_entry_function_t)fbe_test_lookup_symbol(&physical_package_handle, "physical_package_dll_get_io_entry");
    if (physical_package_dll_get_io_entry_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}
    status = physical_package_dll_get_io_entry_function(&physical_package_io_entry);
    if ((physical_package_io_entry == NULL)||(status != FBE_STATUS_OK))
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

    physical_package_dll_get_specl_sfi_entry_function = (physical_package_dll_get_specl_sfi_entry_function_t)fbe_test_lookup_symbol(&physical_package_handle, "physical_package_dll_get_specl_sfi_entry");
    if (physical_package_dll_get_specl_sfi_entry_function == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = physical_package_dll_get_specl_sfi_entry_function(&specl_sfi_entry);
    if ((physical_package_io_entry == NULL)||(status != FBE_STATUS_OK))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_terminator_api_set_specl_sfi_entry_func(specl_sfi_entry);

    /* Initialize terminator pointers */	
    physical_package_dll_set_miniport_interface_port_shim_sim_pointers = 
	    (physical_package_dll_set_miniport_interface_port_shim_sim_pointers_function_t)
	    fbe_test_lookup_symbol(&physical_package_handle, "physical_package_dll_set_miniport_interface_port_shim_sim_pointers");
    if (physical_package_dll_set_miniport_interface_port_shim_sim_pointers == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

    miniport_pointers.terminator_miniport_api_port_init_function = fbe_terminator_miniport_api_port_init;
    miniport_pointers.terminator_miniport_api_port_destroy_function = fbe_terminator_miniport_api_port_destroy;
    miniport_pointers.terminator_miniport_api_register_callback_function = fbe_terminator_miniport_api_register_callback;
    miniport_pointers.terminator_miniport_api_unregister_callback_function = fbe_terminator_miniport_api_unregister_callback;
    miniport_pointers.terminator_miniport_api_register_payload_completion_function = fbe_terminator_miniport_api_register_payload_completion;
    miniport_pointers.terminator_miniport_api_unregister_payload_completion_function = fbe_terminator_miniport_api_unregister_payload_completion;
    miniport_pointers.terminator_miniport_api_enumerate_cpd_ports_function = fbe_terminator_miniport_api_enumerate_cpd_ports;
    miniport_pointers.terminator_miniport_api_register_sfp_event_callback_function = fbe_terminator_miniport_api_register_sfp_event_callback;
    miniport_pointers.terminator_miniport_api_unregister_sfp_event_callback_function = fbe_terminator_miniport_api_unregister_sfp_event_callback;

    miniport_pointers.terminator_miniport_api_get_port_type_function = fbe_terminator_miniport_api_get_port_type;
    miniport_pointers.terminator_miniport_api_remove_port_function = fbe_terminator_miniport_api_remove_port;
    miniport_pointers.terminator_miniport_api_port_inserted_function = fbe_terminator_miniport_api_port_inserted;
    miniport_pointers.terminator_miniport_api_set_speed_function = fbe_terminator_miniport_api_set_speed;
    miniport_pointers.terminator_miniport_api_get_port_info_function = fbe_terminator_miniport_api_get_port_info;
    miniport_pointers.terminator_miniport_api_send_payload_function = fbe_terminator_miniport_api_send_payload;
    miniport_pointers.terminator_miniport_api_send_fis_function = fbe_terminator_miniport_api_send_fis;
    miniport_pointers.terminator_miniport_api_reset_device_function = fbe_terminator_miniport_api_reset_device;
    miniport_pointers.terminator_miniport_api_get_port_address_function = fbe_terminator_miniport_api_get_port_address;
    miniport_pointers.terminator_miniport_api_get_hardware_info_function = fbe_terminator_miniport_api_get_hardware_info;
    miniport_pointers.terminator_miniport_api_get_sfp_media_interface_info_function = fbe_terminator_miniport_api_get_sfp_media_interface_info;
    miniport_pointers.terminator_miniport_api_port_configuration_function = fbe_terminator_miniport_api_port_configuration;
    miniport_pointers.terminator_miniport_api_get_port_link_info_function = fbe_terminator_miniport_api_get_port_link_info;
    miniport_pointers.terminator_miniport_api_register_keys_function      = fbe_terminator_miniport_api_register_keys;
    miniport_pointers.terminator_miniport_api_unregister_keys_function    = fbe_terminator_miniport_api_unregister_keys;
    miniport_pointers.terminator_miniport_api_reestablish_key_handle_function = fbe_terminator_miniport_api_reestablish_key_handle;

    status = physical_package_dll_set_miniport_interface_port_shim_sim_pointers(&miniport_pointers);
    if (status != FBE_STATUS_OK)
	{
		return status;
	}
    
    physical_package_dll_set_terminator_api_get_board_info_function = 
	    (physical_package_dll_set_terminator_api_get_board_info_function_t)
	    fbe_test_lookup_symbol(&physical_package_handle, "physical_package_dll_set_terminator_api_get_board_info");
    if (physical_package_dll_set_terminator_api_get_board_info_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}
    status = physical_package_dll_set_terminator_api_get_board_info_function(fbe_terminator_api_get_board_info);
    if (status != FBE_STATUS_OK)
	{
		return status;
	}

    physical_package_dll_set_terminator_api_get_sp_id_function = 
	    (physical_package_dll_set_terminator_api_get_sp_id_function_t)
	    fbe_test_lookup_symbol(&physical_package_handle, "physical_package_dll_set_terminator_api_get_sp_id");
    if (physical_package_dll_set_terminator_api_get_sp_id_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}
    status = physical_package_dll_set_terminator_api_get_sp_id_function(fbe_terminator_api_get_sp_id);
    if (status != FBE_STATUS_OK)
	{
		return status;
	}

    physical_package_dll_set_terminator_api_is_single_sp_system_function =
        (physical_package_dll_set_terminator_api_is_single_sp_system_function_t)
        fbe_test_lookup_symbol(&physical_package_handle, "physical_package_dll_set_terminator_api_is_single_sp_system");
    if (physical_package_dll_set_terminator_api_is_single_sp_system_function == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = physical_package_dll_set_terminator_api_is_single_sp_system_function(fbe_terminator_api_is_single_sp_system);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    physical_package_dll_set_terminator_api_process_specl_sfi_mask_data_queue_function =
        (physical_package_dll_set_terminator_api_process_specl_sfi_mask_data_queue_function_t)
        fbe_test_lookup_symbol(&physical_package_handle, "physical_package_dll_set_terminator_api_process_specl_sfi_mask_data_queue_function");
    if (physical_package_dll_set_terminator_api_process_specl_sfi_mask_data_queue_function == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = physical_package_dll_set_terminator_api_process_specl_sfi_mask_data_queue_function(fbe_terminator_api_process_specl_sfi_mask_data_queue);
    if (status != FBE_STATUS_OK)
	{
		return status;
	}

	/* start physical package after all the pointers are set up */
	physical_package_dll_init_function = (physical_package_dll_init_function_t)fbe_test_lookup_symbol(&physical_package_handle, "physical_package_dll_init");
    if (physical_package_dll_init_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}
    status = physical_package_dll_init_function();
	return status;
}

fbe_status_t fbe_test_unload_physical_package(void)
{
	fbe_status_t status;
	physical_package_dll_destroy_function_t	physical_package_dll_destroy_function = NULL;

	physical_package_dll_destroy_function = (physical_package_dll_destroy_function_t)fbe_test_lookup_symbol(&physical_package_handle, "physical_package_dll_destroy");
    if (physical_package_dll_destroy_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

	physical_package_control_entry = NULL;

	status = physical_package_dll_destroy_function();
    if (status != FBE_STATUS_OK)
	{
		return status;
	}
    fbe_test_unload_wait(FBE_PACKAGE_ID_PHYSICAL);
	emcutil_dll_loader_unload(&physical_package_handle);	
	return status;
}
fbe_status_t fbe_test_load_sep(fbe_sep_package_load_params_t *params_p)
{
	fbe_status_t status;
    csx_status_e csx_status;
    fbe_bool_t bl_enable_bootflags_mode = FBE_FALSE;

	sep_dll_set_physical_package_control_entry_function_t sep_dll_set_physical_package_control_entry_function = NULL;
	sep_dll_set_physical_package_io_entry_function_t sep_dll_set_physical_package_io_entry_function = NULL;
    sep_dll_set_esp_package_control_entry_function_t sep_dll_set_esp_package_control_entry_function = NULL;
	sep_dll_init_function_t		sep_dll_init_function = NULL;
	sep_dll_get_control_entry_function_t sep_dll_get_control_entry_function = NULL;
	sep_dll_get_io_entry_function_t sep_dll_get_io_entry_function = NULL;
	sep_dll_set_terminator_api_get_sp_id_function_t sep_dll_set_terminator_api_get_sp_id_func = NULL;
	sep_dll_set_terminator_api_get_cmi_port_base_function_t sep_dll_set_terminator_api_get_cmi_port_base_func = NULL;
	sep_dll_get_sep_shim_lun_info_entry_t sep_dll_get_sep_shim_lun_info_entry = NULL;
	sep_dll_get_sep_cache_io_entry_t sep_dll_get_cache_io_entry = NULL;
	esp_dll_set_sep_package_control_entry_function_t esp_dll_set_sep_package_control_entry_function = NULL;
    
	#if 0 /*CMS disabled*/
	fbe_cms_memory_persistence_pointers_t fbe_cms_memory_persistence_pointers;
    sep_dll_set_memory_persistence_pointers_function_t sep_dll_set_memory_persistence_pointers_function = NULL;
	#endif

	/*before loading SEP we want to generate the ICA stamp on the first 3 logical drives, but only if there is no stamp there already*/
	printf("%s: fbe_test_utils_generate_ica_stamp\n", __FUNCTION__);
    status = fbe_test_utils_generate_ica_stamp();
	if (status != FBE_STATUS_OK)
	{
		printf("Failed to read or generate ICA stamp\n");
		return FBE_STATUS_GENERIC_FAILURE;
	}
    else
    {
        is_ica_stamp_generated = FBE_TRUE;
    }

    if (params_p && (params_p->flags & FBE_SEP_LOAD_FLAG_BOOTFLASH_MODE))
    {
        printf("%s: booting SP to bootflash mode\n", __FUNCTION__);
        bl_enable_bootflags_mode = FBE_TRUE;
    }
    else
    {
        printf("%s: booting SP to normal mode\n", __FUNCTION__);
        bl_enable_bootflags_mode = FBE_FALSE;
    }
    status = fbe_registry_write(NULL,
                                SERVICES_REG_PATH,
                                BOOT_FROM_FLASH_KEY,
                                FBE_REGISTRY_VALUE_BINARY,
                                &bl_enable_bootflags_mode,
                                sizeof(fbe_bool_t));

    if(status != FBE_STATUS_OK)
    {
        printf("Failed to write the BOOT_FROM_FLASH_KEY registery \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
	printf("%s: LoadLibrary\n", __FUNCTION__);
    csx_status = emcutil_dll_loader_load("sep", CSX_FALSE, &sep_handle);
    if (CSX_FAILURE(csx_status))
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

	#if 0/*CMS disabled*/
    /* We need to hook SEP up directly to the terminator's persistent memory interface */
	sep_dll_set_memory_persistence_pointers_function = 
		(sep_dll_set_memory_persistence_pointers_function_t)fbe_test_lookup_symbol(&sep_handle, "sep_dll_set_memory_persistence_pointers");
    if (sep_dll_set_memory_persistence_pointers_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

	
    fbe_cms_memory_persistence_pointers.get_persistence_request = fbe_terminator_persistent_memory_get_persistence_request;
    fbe_cms_memory_persistence_pointers.set_persistence_request = fbe_terminator_persistent_memory_set_persistence_request;
    fbe_cms_memory_persistence_pointers.get_persistence_status = fbe_terminator_persistent_memory_get_persistence_status;
    fbe_cms_memory_persistence_pointers.get_sgl = fbe_terminator_persistent_memory_get_sgl;

	status = sep_dll_set_memory_persistence_pointers_function(&fbe_cms_memory_persistence_pointers);
    if (status != FBE_STATUS_OK)
	{
		return status;
	}
	#endif /*CMS disabled*/

	/* We need to initialize physical package control and entries.
		In kernel SEP will find it out from Windows driver interface.
	*/
	sep_dll_set_physical_package_control_entry_function = 
		(sep_dll_set_physical_package_control_entry_function_t)fbe_test_lookup_symbol(&sep_handle, "sep_dll_set_physical_package_control_entry");
    if (sep_dll_set_physical_package_control_entry_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}
	status = sep_dll_set_physical_package_control_entry_function(physical_package_control_entry);
    if (status != FBE_STATUS_OK)
	{
		return status;
	}

	sep_dll_set_physical_package_io_entry_function = 
		(sep_dll_set_physical_package_io_entry_function_t)fbe_test_lookup_symbol(&sep_handle, "sep_dll_set_physical_package_io_entry");
    if (sep_dll_set_physical_package_io_entry_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = sep_dll_set_physical_package_io_entry_function(physical_package_io_entry);
    if (status != FBE_STATUS_OK)
	{
		return status;
	}

	/*set esp entries so we can call it for the purpose of PS and SPS info for the clustered memory service*/
    sep_dll_set_esp_package_control_entry_function = 
	(sep_dll_set_esp_package_control_entry_function_t)fbe_test_lookup_symbol(&sep_handle, "sep_dll_set_esp_package_control_entry");

	if (sep_dll_set_esp_package_control_entry_function == NULL)
	{
        printf("%s: ESP package control entry for SEP not found\n", __FUNCTION__);
        // ESP is no longer required for all SEP tests
		//return FBE_STATUS_GENERIC_FAILURE;
	}
    else
    {
    
        status = sep_dll_set_esp_package_control_entry_function(esp_control_entry);
        if (status != FBE_STATUS_OK)
        {
            printf("%s: SEP cannot set ESP package control entry \n", __FUNCTION__);
            // ESP is no longer required for all SEP tests
            //return status;
        }
    }

    sep_dll_set_terminator_api_get_sp_id_func =  (sep_dll_set_terminator_api_get_sp_id_function_t)
		fbe_test_lookup_symbol(&sep_handle, "sep_dll_set_terminator_api_get_sp_id");

    if (sep_dll_set_terminator_api_get_sp_id_func == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = sep_dll_set_terminator_api_get_sp_id_func(fbe_terminator_api_get_sp_id);

	sep_dll_set_terminator_api_get_cmi_port_base_func =  (sep_dll_set_terminator_api_get_cmi_port_base_function_t)
        fbe_test_lookup_symbol(&sep_handle, "sep_dll_set_terminator_api_get_cmi_port_base");

    if (sep_dll_set_terminator_api_get_cmi_port_base_func == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = sep_dll_set_terminator_api_get_cmi_port_base_func(fbe_terminator_api_get_cmi_port_base);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

	/* Initialize sep services */
	sep_dll_init_function = (sep_dll_init_function_t)fbe_test_lookup_symbol(&sep_handle, "sep_dll_init");
    if (sep_dll_init_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

	printf("%s: sep_dll_init_function\n", __FUNCTION__);
	status = sep_dll_init_function(params_p);
    if (status != FBE_STATUS_OK)
	{
		return status;
	}

	sep_dll_get_control_entry_function = (sep_dll_get_control_entry_function_t)fbe_test_lookup_symbol(&sep_handle, "sep_dll_get_control_entry");
    if (sep_dll_get_control_entry_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}
	status = sep_dll_get_control_entry_function(&sep_control_entry);
    if ((sep_control_entry == NULL)||(status != FBE_STATUS_OK))
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

   
    /*set the function entries in ESP to be able to access SEP so it can acess the PVDs*/
    esp_dll_set_sep_package_control_entry_function = 
            (esp_dll_set_sep_package_control_entry_function_t)fbe_test_lookup_symbol(&esp_handle, "esp_dll_set_sep_package_control_entry");
    if (esp_dll_set_sep_package_control_entry_function == NULL)
    {
        printf("%s: SEP package control entry for ESP not found\n", __FUNCTION__);
        //ESP is no longer required for all SEP tests
                //return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        status = esp_dll_set_sep_package_control_entry_function(sep_control_entry);
        if (status != FBE_STATUS_OK)
        {
            printf("%s: ESP cannot set SEP package control entry \n", __FUNCTION__);
            // ESP is no longer required for all SEP tests
            //return status;
        }
    }

	sep_dll_get_io_entry_function = (sep_dll_get_io_entry_function_t)fbe_test_lookup_symbol(&sep_handle, "sep_dll_get_io_entry");
    if (sep_dll_get_io_entry_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}
	status = sep_dll_get_io_entry_function(&sep_io_entry);
    if (sep_io_entry == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*sep shim entry for cache IO related stuff*/
	sep_dll_get_sep_shim_lun_info_entry = (sep_dll_get_sep_shim_lun_info_entry_t)fbe_test_lookup_symbol(&sep_handle, "sep_dll_get_sep_shim_lun_info_entry");
    if (sep_dll_get_control_entry_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}
	status = sep_dll_get_sep_shim_lun_info_entry(&lun_map_entry);
    if ((lun_map_entry == NULL)||(status != FBE_STATUS_OK))
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

	sep_dll_get_cache_io_entry = (sep_dll_get_sep_cache_io_entry_t)fbe_test_lookup_symbol(&sep_handle, "sep_dll_get_sep_shim_cache_io_entry");
    if (sep_dll_get_control_entry_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}
	status = sep_dll_get_cache_io_entry(&sep_shim_cache_io_entry);
    if ((lun_map_entry == NULL)||(status != FBE_STATUS_OK))
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

	printf("%s: Done.\n", __FUNCTION__);

	return status;
}

fbe_status_t fbe_test_load_sep_package(fbe_packet_t *packet_p)
{
    fbe_sep_package_load_params_t *params_p = NULL;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t buffer_length;

    if (packet_p != NULL)
    {
        sep_payload_p = fbe_transport_get_payload_ex(packet_p);
        control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

        fbe_payload_control_get_buffer_length(control_operation_p, &buffer_length);
        if (buffer_length != 0)
        {
            fbe_payload_control_get_buffer(control_operation_p, &params_p);
    
            /* The buffer is optional.  Only validate the length if the buffer is set.
             */
            if ((params_p != NULL) &&
                (buffer_length != sizeof(fbe_sep_package_load_params_t)))
            {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, load sep package params len %d != %llu\n", 
                              __FUNCTION__, buffer_length,
			      (unsigned long long)sizeof(fbe_sep_package_load_params_t));
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }
    return fbe_test_load_sep(params_p);
}
fbe_status_t fbe_test_unload_sep(void)
{
	fbe_status_t status;
	sep_dll_destroy_function_t	sep_dll_destroy_function = NULL;
	esp_dll_set_sep_package_control_entry_function_t esp_dll_set_sep_package_control_entry_function = NULL;

	sep_dll_destroy_function = (sep_dll_destroy_function_t)fbe_test_lookup_symbol(&sep_handle, "sep_dll_destroy");
    if (sep_dll_destroy_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

	sep_control_entry = NULL;
	sep_io_entry = NULL;

	esp_dll_set_sep_package_control_entry_function = 
		(esp_dll_set_sep_package_control_entry_function_t)fbe_test_lookup_symbol(&esp_handle, "esp_dll_set_sep_package_control_entry");
    if (esp_dll_set_sep_package_control_entry_function != NULL)
	{
        status = esp_dll_set_sep_package_control_entry_function(NULL);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }

	status = sep_dll_destroy_function();
    if (status != FBE_STATUS_OK)
	{
		return status;
	}
    fbe_test_unload_wait(FBE_PACKAGE_ID_SEP_0);
	emcutil_dll_loader_unload(&sep_handle);
	return status;
}

fbe_status_t fbe_test_load_esp(void)
{
	fbe_status_t status;
    csx_status_e csx_status;
	esp_dll_set_physical_package_control_entry_function_t esp_dll_set_physical_package_control_entry_function = NULL;
    esp_dll_set_physical_package_io_entry_function_t esp_dll_set_physical_package_io_entry_function = NULL;
	esp_dll_init_function_t		esp_dll_init_function = NULL;
	esp_dll_get_control_entry_function_t esp_dll_get_control_entry_function = NULL;
	esp_dll_get_io_entry_function_t esp_dll_get_io_entry_function = NULL;
	esp_dll_set_terminator_api_get_sp_id_function_t esp_dll_set_terminator_api_get_sp_id_func = NULL;
	esp_dll_set_terminator_api_get_cmi_port_base_function_t esp_dll_set_terminator_api_get_cmi_port_base_func = NULL;
    esp_dll_set_sep_package_control_entry_function_t esp_dll_set_sep_package_control_entry_function = NULL;

    /*  Create the registry file */
    //fbe_file_creat("./esp_reg.txt", FBE_FILE_RDWR);
    csx_status = emcutil_dll_loader_load("espkg", CSX_FALSE, &esp_handle);
    if (CSX_FAILURE(csx_status))
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/* We need to initialize physical package control and entries.
		In kernel esp will find it out from Windows driver interface.
	*/
	esp_dll_set_physical_package_control_entry_function = 
		(esp_dll_set_physical_package_control_entry_function_t)fbe_test_lookup_symbol(&esp_handle, "esp_dll_set_physical_package_control_entry");
    if (esp_dll_set_physical_package_control_entry_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}
	status = esp_dll_set_physical_package_control_entry_function(physical_package_control_entry);
    if (status != FBE_STATUS_OK)
	{
		return status;
	}

	esp_dll_set_physical_package_io_entry_function = 
		(esp_dll_set_physical_package_io_entry_function_t)fbe_test_lookup_symbol(&esp_handle, "esp_dll_set_physical_package_io_entry");
    if (esp_dll_set_physical_package_io_entry_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = esp_dll_set_physical_package_io_entry_function(physical_package_io_entry);
    if (status != FBE_STATUS_OK)
	{
		return status;
	}

    /*set the function entries in ESP to be able to access SEP so it can acess the PVDs*/
	esp_dll_set_sep_package_control_entry_function = 
		(esp_dll_set_sep_package_control_entry_function_t)fbe_test_lookup_symbol(&esp_handle, "esp_dll_set_sep_package_control_entry");
    if (esp_dll_set_sep_package_control_entry_function == NULL)
	{
        printf("%s: SEP package control entry for ESP not found\n", __FUNCTION__);
        //ESP is no longer required for all SEP tests
		//return FBE_STATUS_GENERIC_FAILURE;
	}
    else
    {
        status = esp_dll_set_sep_package_control_entry_function(sep_control_entry);
        if (status != FBE_STATUS_OK)
        {
            printf("%s: ESP cannot set SEP package control entry \n", __FUNCTION__);
            // ESP is no longer required for all SEP tests
            //return status;
        }
    }

	esp_dll_set_terminator_api_get_sp_id_func =  (esp_dll_set_terminator_api_get_sp_id_function_t)
		fbe_test_lookup_symbol(&esp_handle, "esp_dll_set_terminator_api_get_sp_id");

    if (esp_dll_set_terminator_api_get_sp_id_func == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = esp_dll_set_terminator_api_get_sp_id_func(fbe_terminator_api_get_sp_id);

    esp_dll_set_terminator_api_get_cmi_port_base_func =  (esp_dll_set_terminator_api_get_cmi_port_base_function_t)
        fbe_test_lookup_symbol(&esp_handle, "esp_dll_set_terminator_api_get_cmi_port_base");

    if (esp_dll_set_terminator_api_get_cmi_port_base_func == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = esp_dll_set_terminator_api_get_cmi_port_base_func(fbe_terminator_api_get_cmi_port_base);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

	/* Initialize esp services */
	esp_dll_init_function = (esp_dll_init_function_t)fbe_test_lookup_symbol(&esp_handle, "esp_dll_init");
    if (esp_dll_init_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = esp_dll_init_function();
    if (status != FBE_STATUS_OK)
	{
		return status;
	}

	esp_dll_get_control_entry_function = (esp_dll_get_control_entry_function_t)fbe_test_lookup_symbol(&esp_handle, "esp_dll_get_control_entry");
    if (esp_dll_get_control_entry_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}
	status = esp_dll_get_control_entry_function(&esp_control_entry);
    if ((esp_control_entry == NULL)||(status != FBE_STATUS_OK))
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

	esp_dll_get_io_entry_function = (esp_dll_get_io_entry_function_t)fbe_test_lookup_symbol(&esp_handle, "esp_dll_get_io_entry");
    if (esp_dll_get_io_entry_function == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}
	status = esp_dll_get_io_entry_function(&esp_io_entry);
    if (esp_io_entry == NULL)
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

    

	return status;
}

fbe_status_t fbe_test_unload_esp(void)
{
    fbe_status_t status;
    esp_dll_destroy_function_t	esp_dll_destroy_function = NULL;
    esp_dll_set_sep_package_control_entry_function_t esp_dll_set_sep_package_control_entry_function = NULL;

    esp_dll_destroy_function = (esp_dll_destroy_function_t)fbe_test_lookup_symbol(&esp_handle, "esp_dll_destroy");
    if (esp_dll_destroy_function == NULL)
    {
            return FBE_STATUS_GENERIC_FAILURE;
    }

    esp_control_entry = NULL;

    esp_dll_set_sep_package_control_entry_function = 
		(esp_dll_set_sep_package_control_entry_function_t)fbe_test_lookup_symbol(&esp_handle, "esp_dll_set_sep_package_control_entry");
    if (esp_dll_set_sep_package_control_entry_function != NULL)
    {
        status = esp_dll_set_sep_package_control_entry_function(NULL);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }

    status = esp_dll_destroy_function();
    if (status != FBE_STATUS_OK)
    {
            return status;
    }
    fbe_test_unload_wait(FBE_PACKAGE_ID_ESP);
    emcutil_dll_loader_unload(&esp_handle);
    fbe_zero_memory(&esp_handle, sizeof(EMCUTIL_DLL_LOADER_PARAMS));
    return status;
}

fbe_status_t fbe_test_load_neit(fbe_neit_package_load_params_t *params_p)
{
    fbe_status_t status;
    csx_status_e csx_status;
    neit_dll_set_physical_package_control_entry_function_t neit_dll_set_physical_package_control_entry_function = NULL;
    neit_dll_set_physical_package_io_entry_function_t neit_dll_set_physical_package_io_entry_function = NULL;
    neit_dll_set_sep_io_entry_function_t neit_dll_set_sep_io_entry_function = NULL;
    neit_dll_set_sep_control_entry_function_t neit_dll_set_sep_control_entry_function = NULL;
    neit_dll_init_function_t		neit_dll_init_function = NULL;
    neit_dll_get_control_entry_function_t neit_dll_get_control_entry_function = NULL;
    neit_dll_set_terminator_api_get_sp_id_function_t neit_dll_set_terminator_api_get_sp_id_func = NULL;
    neit_dll_set_terminator_api_get_cmi_port_base_function_t neit_dll_set_terminator_api_get_cmi_port_base_func = NULL;
    sep_dll_set_neit_package_control_entry_function_t sep_dll_set_neit_package_control_entry_function;

    csx_status = emcutil_dll_loader_load("new_neit", CSX_FALSE, &neit_handle);
    if (CSX_FAILURE(csx_status))
	{
		return FBE_STATUS_GENERIC_FAILURE;
	}

    /* We need to initialize physical package control and entries.
	    In kernel neit will find it out from Windows driver interface.
    */
    neit_dll_set_physical_package_control_entry_function = 
		(neit_dll_set_physical_package_control_entry_function_t)fbe_test_lookup_symbol(&neit_handle, "neit_dll_set_physical_package_control_entry");
    if (neit_dll_set_physical_package_control_entry_function == NULL)
    {
	    return FBE_STATUS_GENERIC_FAILURE;
    }
    status = neit_dll_set_physical_package_control_entry_function(physical_package_control_entry);
    if (status != FBE_STATUS_OK)
    {
	    return status;
    }
    /* We need to initialize sep control and entries.
	    In kernel neit will find it out from Windows driver interface.
    */
    neit_dll_set_sep_control_entry_function = 
	    (neit_dll_set_sep_control_entry_function_t)fbe_test_lookup_symbol(&neit_handle, "neit_dll_set_sep_control_entry");
    if (neit_dll_set_sep_control_entry_function == NULL)
    {
	    return FBE_STATUS_GENERIC_FAILURE;
    }
    status = neit_dll_set_sep_control_entry_function(sep_control_entry);
    if (status != FBE_STATUS_OK)
    {
	    return status;
    }

    neit_dll_set_physical_package_io_entry_function = 
	(neit_dll_set_physical_package_io_entry_function_t)fbe_test_lookup_symbol(&neit_handle, "neit_dll_set_physical_package_io_entry");
    if (neit_dll_set_physical_package_io_entry_function == NULL)
    {
	    return FBE_STATUS_GENERIC_FAILURE;
    }
    status = neit_dll_set_physical_package_io_entry_function(physical_package_io_entry);
    if (status != FBE_STATUS_OK)
    {
	    return status;
    }

    neit_dll_set_sep_io_entry_function = 
	(neit_dll_set_sep_io_entry_function_t)fbe_test_lookup_symbol(&neit_handle, "neit_dll_set_sep_io_entry");
    if (neit_dll_set_sep_io_entry_function == NULL)
    {
	    return FBE_STATUS_GENERIC_FAILURE;
    }
    if (sep_io_entry != NULL)
    {
        status = neit_dll_set_sep_io_entry_function(sep_io_entry);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }

    neit_dll_set_terminator_api_get_sp_id_func =  (neit_dll_set_terminator_api_get_sp_id_function_t)
        fbe_test_lookup_symbol(&neit_handle, "neit_dll_set_terminator_api_get_sp_id");

    if (neit_dll_set_terminator_api_get_sp_id_func == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = neit_dll_set_terminator_api_get_sp_id_func(fbe_terminator_api_get_sp_id);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    neit_dll_set_terminator_api_get_cmi_port_base_func =  (neit_dll_set_terminator_api_get_cmi_port_base_function_t)
        fbe_test_lookup_symbol(&neit_handle, "neit_dll_set_terminator_api_get_cmi_port_base");

    if (neit_dll_set_terminator_api_get_cmi_port_base_func == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = neit_dll_set_terminator_api_get_cmi_port_base_func(fbe_terminator_api_get_cmi_port_base);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Initialize neit services */
    neit_dll_init_function = (neit_dll_init_function_t)fbe_test_lookup_symbol(&neit_handle, "neit_dll_init");
    if (neit_dll_init_function == NULL)
    {
	    return FBE_STATUS_GENERIC_FAILURE;
    }

    status = neit_dll_init_function(params_p);
    if (status != FBE_STATUS_OK)
    {
	    return status;
    }

    neit_dll_get_control_entry_function = (neit_dll_get_control_entry_function_t)fbe_test_lookup_symbol(&neit_handle, "neit_dll_get_control_entry");
    if (neit_dll_get_control_entry_function == NULL)
    {
	    return FBE_STATUS_GENERIC_FAILURE;
    }
    status = neit_dll_get_control_entry_function(&neit_package_control_entry);
    if ((neit_package_control_entry == NULL) || (status != FBE_STATUS_OK))
    {
	    return FBE_STATUS_GENERIC_FAILURE;
    }

	sep_dll_set_neit_package_control_entry_function = 
		(sep_dll_set_neit_package_control_entry_function_t)fbe_test_lookup_symbol(&sep_handle, "sep_dll_set_neit_package_control_entry");
    if (sep_dll_set_neit_package_control_entry_function == NULL)
	{
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s Cannot get SEP neit package entry\n", __FUNCTION__);
		return FBE_STATUS_OK;
	}
	status = sep_dll_set_neit_package_control_entry_function(neit_package_control_entry);
    if (status != FBE_STATUS_OK)
	{
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s Cannot set SEP neit package entry\n", __FUNCTION__);
        return FBE_STATUS_OK;
	}
    return status;
}
fbe_status_t fbe_test_load_neit_package(fbe_packet_t *packet_p)
{
    fbe_neit_package_load_params_t *params_p = NULL;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t buffer_length;

    if (packet_p != NULL)
    {
        sep_payload_p = fbe_transport_get_payload_ex(packet_p);
        control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

        fbe_payload_control_get_buffer_length(control_operation_p, &buffer_length);
        if (buffer_length != 0)
        {
            fbe_payload_control_get_buffer(control_operation_p, &params_p);
    
            /* The buffer is optional.  Only validate the length if the buffer is set.
             */
            if ((params_p != NULL) &&
                (buffer_length != sizeof(fbe_neit_package_load_params_t)))
            {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, load neit package params len %d != %llu\n",
                              __FUNCTION__, buffer_length,
			      (unsigned long long)sizeof(fbe_sep_package_load_params_t));
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }
    return fbe_test_load_neit(params_p);
}

fbe_status_t fbe_test_disable_neit_package(void)
{
    fbe_status_t status;
    neit_dll_disable_function_t	neit_dll_disable_function = NULL;
    neit_dll_destroy_function_t	neit_dll_destroy_function = NULL;

    neit_dll_disable_function = (neit_dll_disable_function_t)fbe_test_lookup_symbol(&neit_handle, "neit_dll_disable");
    if (neit_dll_disable_function == NULL) {
	    return FBE_STATUS_GENERIC_FAILURE;
    }

    status = neit_dll_disable_function();
    if (status != FBE_STATUS_OK)
    {
	    return status;
    }

    neit_dll_destroy_function = (neit_dll_destroy_function_t)fbe_test_lookup_symbol(&neit_handle, "neit_dll_destroy");
    if (neit_dll_destroy_function == NULL)
    {
	    return FBE_STATUS_GENERIC_FAILURE;
    }

    status = neit_dll_destroy_function();
    if (status != FBE_STATUS_OK)
    {
	    return status;
    }
    return status;
}

fbe_status_t fbe_test_unload_neit_package(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    neit_dll_destroy_function_t	neit_dll_destroy_function = NULL;

    neit_dll_destroy_function = (neit_dll_destroy_function_t)fbe_test_lookup_symbol(&neit_handle, "neit_dll_destroy");
    if (neit_dll_destroy_function == NULL)
    {
	    return FBE_STATUS_GENERIC_FAILURE;
    }

    neit_package_control_entry = NULL;
    fbe_test_unload_wait(FBE_PACKAGE_ID_NEIT);
    emcutil_dll_loader_unload(&neit_handle);
    return status;
}

static fbe_status_t fbe_test_utils_generate_ica_stamp(void)
{
    fbe_object_id_t 		object_id[FBE_TEST_ICA_STAMP_SYS_DRIVE_COUNT];
    fbe_status_t			status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t				slot;
    fbe_u32_t				slot2;
    fbe_imaging_flags_t 	ica_stamp[FBE_TEST_ICA_STAMP_SYS_DRIVE_COUNT];
    fbe_u32_t               retry_count;
    fbe_bool_t              missing_drive_in_slot[FBE_TEST_ICA_STAMP_SYS_DRIVE_COUNT] = {FBE_FALSE}; 
    fbe_u32_t               no_of_missing_drives = 0;
    fbe_bool_t              valid_magic_array[FBE_TEST_ICA_STAMP_SYS_DRIVE_COUNT] = {FBE_FALSE};
    fbe_bool_t              all_ica_stamp_invalid = FBE_TRUE;
    fbe_char_t              mdb_init[FBE_MAX_REGISTRY_STRING_LEN];
    

    for (slot = 0; slot < FBE_TEST_ICA_STAMP_SYS_DRIVE_COUNT; slot++) {
        retry_count = 5;
        while(retry_count != 0){
            status = fbe_api_get_physical_drive_object_id_by_location(0, 0, slot, &object_id[slot]);
            if (status != FBE_STATUS_OK){
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                    "%s, get pdo object id by loc failed: slot %d\n", 
                    __FUNCTION__, slot);
                return status;
            }

            if (object_id[slot] != FBE_OBJECT_ID_INVALID){
                status = fbe_api_wait_for_object_lifecycle_state_warn_trace(object_id[slot], 
                    FBE_LIFECYCLE_STATE_READY, 1500, FBE_PACKAGE_ID_PHYSICAL);
                if (status == FBE_STATUS_OK)
                {
                    missing_drive_in_slot[slot] = FBE_FALSE;
                    break;     
                }
            }

            fbe_api_sleep(100);
            retry_count --;

            if(retry_count == 0)
            {
                missing_drive_in_slot[slot] = FBE_TRUE;
                no_of_missing_drives++;
            }
        }
    }

    /* See if we have atleast one DB drive to boot-up the system. */
    if(no_of_missing_drives == FBE_TEST_ICA_STAMP_SYS_DRIVE_COUNT)
        return FBE_STATUS_GENERIC_FAILURE;

    /* Let's read the ica stamp. */
    for (slot = 0; slot < FBE_TEST_ICA_STAMP_SYS_DRIVE_COUNT; slot++) 
    {
        if(!missing_drive_in_slot[slot])
        {
            /* Now, let's read the ICA stamp and see if it's there. If so, we 
            don't touch the drive */
            status = fbe_api_physical_drive_interface_read_ica_stamp(object_id[slot], &ica_stamp[slot]);
            if (status != FBE_STATUS_OK)
                    return status;

            if(!fbe_compare_string(ica_stamp[slot].magic_string, FBE_IMAGING_FLAGS_MAGIC_STRING_LENGTH,
                    FBE_IMAGING_FLAGS_MAGIC_STRING, FBE_IMAGING_FLAGS_MAGIC_STRING_LENGTH, 
                    FBE_TRUE) )
            {
                    valid_magic_array[slot] = FBE_TRUE;
                    all_ica_stamp_invalid = FBE_FALSE;
            }
        }
    }

    /*let's compare the valid ica stamp(invalid ica stamp is from blank or failed drive)*/
    for (slot = 0; slot < FBE_TEST_ICA_STAMP_SYS_DRIVE_COUNT; slot++) 
    {
        if(FBE_FALSE == valid_magic_array[slot])
            continue;

        for(slot2 = slot + 1; slot2 < FBE_TEST_ICA_STAMP_SYS_DRIVE_COUNT; slot2++)
        {
            if(FBE_FALSE == valid_magic_array[slot2])
                continue;
            if(!fbe_equal_memory(&ica_stamp[slot], &ica_stamp[slot2], sizeof(fbe_imaging_flags_t)))
                return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if(!all_ica_stamp_invalid)
        return FBE_STATUS_OK;

    /* send the ICA stamp API*/
    for (slot = 0; slot < FBE_TEST_ICA_STAMP_SYS_DRIVE_COUNT; slot++) 
    {
        if(!missing_drive_in_slot[slot])
        {
            status = fbe_api_physical_drive_interface_generate_ica_stamp(object_id[slot]);
            if (status != FBE_STATUS_OK)
                return status;
        }
    }

    /* write the MDBInit */
    fbe_sprintf(mdb_init, FBE_MAX_REGISTRY_STRING_LEN, "%d", 1);
    status = fbe_registry_write(NULL,
                                C4_MIRROR_REG_PATH,
                                C4_MIRROR_REINIT_KEY, 
                                FBE_REGISTRY_VALUE_SZ,
                                mdb_init, 
                                (fbe_u32_t)strlen(mdb_init));
    if(status != FBE_STATUS_OK)
    {
        printf("Failed to write the C4_MIRROR_REINIT_KEY registery \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_test_utils_zero_firmware_luns_completion
 ****************************************************************
 * @brief
 *  this is the completion function for zeroing the firmware lun private space regions before loading SEP package
 *  There are four firmware luns:
 *  "CLARiiON_DDBS" "CLARiiON_BIOS" "CLARiiON_POST" "CLARiiON_GPS"
 * @param[out]  packet_p
 * @param[in]    context
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no errror.
 *
 * @version
 *  03/02/2012 - Created. zphu.
 *
 ****************************************************************/
static fbe_status_t fbe_test_utils_zero_firmware_luns_completion(fbe_packet_t * packet_p,
                                                               fbe_packet_completion_context_t context)
{
    fbe_status_t    status;
    fbe_payload_control_status_t    payload_status;
    fbe_test_util_zero_firmware_lun_context_t  *zero_context = (fbe_test_util_zero_firmware_lun_context_t*)context;
    fbe_payload_ex_t*    payload = NULL;
    fbe_payload_control_operation_t* control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet_p);
    
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_spinlock_lock(&zero_context->lock);

    zero_context->total_complete_count++; /*@zphu: record how many packets are completed*/

    status = fbe_transport_get_status_code(packet_p);
    if(FBE_STATUS_OK == status)
    {        
        fbe_payload_control_get_status(control_operation, &payload_status);
        if(FBE_PAYLOAD_CONTROL_STATUS_OK == payload_status)
            zero_context->success_complete_count++;  /*@zphu: record how many packets are completed successfully*/
    }

    /*if all sent packets completed, release the semaphore so the zeroing requester can go on working*/
    if(zero_context->total_complete_count == zero_context->complete_count_limit)
    {
        fbe_spinlock_unlock(&zero_context->lock); /*@zphu: must unlock before semaphore release because when semaphore release, this lock would be destroyed*/
        fbe_semaphore_release(&zero_context->semaphore, 0, 1, FALSE);
        return status;
    }

    fbe_spinlock_unlock(&zero_context->lock);   

    return status;

}

fbe_status_t fbe_test_load_kms_package(fbe_packet_t *packet_p)
{
    fbe_kms_package_load_params_t *params_p = NULL;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t buffer_length;

    if (packet_p != NULL)
    {
        sep_payload_p = fbe_transport_get_payload_ex(packet_p);
        control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

        fbe_payload_control_get_buffer_length(control_operation_p, &buffer_length);
        if (buffer_length != 0)
        {
            fbe_payload_control_get_buffer(control_operation_p, &params_p);
    
            /* The buffer is optional.  Only validate the length if the buffer is set.
             */
            if ((params_p != NULL) &&
                (buffer_length != sizeof(fbe_kms_package_load_params_t)))
            {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, load kms params len %d != %llu\n",
                              __FUNCTION__, buffer_length,
			      (unsigned long long)sizeof(fbe_kms_package_load_params_t));
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }
    return fbe_test_load_kms(params_p);
}

fbe_status_t fbe_test_load_kms(fbe_kms_package_load_params_t *params_p)
{
    fbe_status_t status;
    csx_status_e csx_status;
    kms_dll_init_function_t		kms_dll_init_function = NULL;
    kms_dll_get_control_entry_function_t kms_dll_get_control_entry_function = NULL;
    kms_dll_set_sep_io_entry_function_t kms_dll_set_sep_io_entry_function = NULL;
    kms_dll_set_sep_control_entry_function_t kms_dll_set_sep_control_entry_function = NULL;

    printf("%s: LoadLibrary\n", __FUNCTION__);
    csx_status = emcutil_dll_loader_load("kms", CSX_FALSE, &kms_handle);
    if (CSX_FAILURE(csx_status))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We need to initialize sep control and IO entries.
    */
    kms_dll_set_sep_control_entry_function = 
	    (kms_dll_set_sep_control_entry_function_t)fbe_test_lookup_symbol(&kms_handle, "kms_dll_set_sep_control_entry");
    if (kms_dll_set_sep_control_entry_function == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = kms_dll_set_sep_control_entry_function(sep_control_entry);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    kms_dll_set_sep_io_entry_function = 
	(kms_dll_set_sep_io_entry_function_t)fbe_test_lookup_symbol(&kms_handle, "kms_dll_set_sep_io_entry");
    if (kms_dll_set_sep_io_entry_function == NULL)
    {
	    return FBE_STATUS_GENERIC_FAILURE;
    }
    if (sep_io_entry != NULL)
    {
        status = kms_dll_set_sep_io_entry_function(sep_io_entry);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }

    /* Initialize kms services */
    kms_dll_init_function = (kms_dll_init_function_t)fbe_test_lookup_symbol(&kms_handle, "kms_dll_init");
    if (kms_dll_init_function == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = kms_dll_init_function(params_p);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Get kms control entry */
    kms_dll_get_control_entry_function = (kms_dll_get_control_entry_function_t)fbe_test_lookup_symbol(&kms_handle, "kms_dll_get_control_entry");
    if (kms_dll_get_control_entry_function == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = kms_dll_get_control_entry_function(&kms_control_entry);
    if ((kms_control_entry == NULL)||(status != FBE_STATUS_OK))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    printf("%s: Done.\n", __FUNCTION__);

    return status;
}

fbe_status_t fbe_test_unload_kms(void)
{
    fbe_status_t status;
    kms_dll_destroy_function_t	kms_dll_destroy_function = NULL;

    kms_dll_destroy_function = (kms_dll_destroy_function_t)fbe_test_lookup_symbol(&kms_handle, "kms_dll_destroy");
    if (kms_dll_destroy_function == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    kms_control_entry = NULL;

    status = kms_dll_destroy_function();
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    emcutil_dll_loader_unload(&kms_handle);
    return status;
}
