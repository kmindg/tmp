#include "mut.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"

static fbe_test_params_t *test = NULL;

fbe_status_t fbe_test_load_simple_config_table_data(fbe_test_params_t *test_data)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;

    if(test_data != NULL)
    {
        test = test_data;
    }
    status = fbe_test_load_simple_config();
    return status;
}
fbe_status_t fbe_test_load_simple_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t 	board_info;
    fbe_terminator_sas_port_info_t	sas_port;
    fbe_terminator_sas_encl_info_t	sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;

    fbe_api_terminator_device_handle_t	port0_handle;
    fbe_api_terminator_device_handle_t	encl0_0_handle;
    fbe_api_terminator_device_handle_t	drive_handle;

    fbe_cpd_shim_hardware_info_t                hardware_info;

    fbe_u32_t                       slot = 0;
    fbe_u32_t                           num_handles;
    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DEFIANT_M1_HW_TYPE;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a port*/
    if(test == NULL)	//Load default test data
    {
        sas_port.backend_number = 0;
        sas_port.io_port_number = 3;
        sas_port.portal_number = 5;
        sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
        sas_port.sas_address = 0x5000097A7800903F;
    }
    else
    {           // Load table entry data
        sas_port.backend_number = test->backend_number;
        sas_port.io_port_number = test->io_port_number;
        sas_port.portal_number = test->portal_number;
        sas_port.port_type = test->port_type;       	
        sas_port.sas_address = FBE_BUILD_PORT_SAS_ADDRESS(sas_port.backend_number);
    }
    status  = fbe_api_terminator_insert_sas_port (&sas_port, &port0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* configure port hardware info */
    fbe_zero_memory(&hardware_info,sizeof(fbe_cpd_shim_hardware_info_t));
    hardware_info.vendor = 0x11F8;
    hardware_info.device = 0x8001;
    hardware_info.bus = 1;
    hardware_info.slot = 0;
    hardware_info.function = 0;

    status = fbe_api_terminator_set_hardware_info(port0_handle,&hardware_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert an enclosure to port 0*/
    if(test == NULL)
    {
        sas_encl.backend_number = 0;
        sas_encl.encl_number = 0;
        sas_encl.uid[0] = 1; // some unique ID.
        sas_encl.sas_address = 0x5000097A78009071;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    }
    else
    {
        sas_encl.backend_number = test->backend_number;
        sas_encl.encl_number = test->encl_number;
        sas_encl.uid[0] = ((test->backend_number * 8) + test->encl_number); // some unique ID.
        sas_encl.sas_address =  FBE_BUILD_ENCL_SAS_ADDRESS(test->backend_number, test->encl_number);		// Do we want to keep it same for all encl types??
        sas_encl.encl_type = test->encl_type;
    }

    status  = fbe_test_insert_sas_enclosure(port0_handle, &sas_encl, &encl0_0_handle, &num_handles);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    

    for (slot = 0; slot < SIMPLE_CONFIG_DRIVE_MAX; slot ++) {
    /*insert a sas_drive to port 0 encl 0 slot */
        if(test != NULL)
        {
            sas_drive.backend_number = test->backend_number;
            sas_drive.encl_number =  test->encl_number;
            sas_drive.drive_type = test->drive_type;

        }
        else
        {
            sas_drive.backend_number = 0;
            sas_drive.encl_number = 0;
            sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
        }
        sas_drive.capacity = 0x10BD0;		// In FBE it is ZRT_SAS_DRIVECAPACITY   0x10BD0, so which one to take?
        sas_drive.block_size = 520;
        sas_drive.sas_address = FBE_BUILD_DISK_SAS_ADDRESS(sas_drive.encl_number, sas_drive.encl_number, slot);
        status  = fbe_test_insert_sas_drive (encl0_0_handle, slot, &sas_drive, &drive_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    return status;
}


/*!****************************************************************************
 * @fn      fbe_test_internal_insert_voyager_sas_enclosure ()
 *
 * @brief
 *  This function inserts a voyager enclosure and the corresponding edge expanders.
 *
 * @param parent_handle - parent where the new enclosure will be inserted.
 * @param encl_info - characteristics of the enclosure to insert (ICM)
 * @param ret_handles - fills up with terminator handles for ICM, EE0
 * and EE1.  The num_handles will be set to 3.
 *
 * @return
 *  FBE_STATUS_OK if all goes well.
 *
 * HISTORY
 *  06/01/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_internal_insert_voyager_sas_enclosure (fbe_terminator_api_device_handle_t parent_handle,
                                                      fbe_terminator_sas_encl_info_t *encl_info, 
                                                      fbe_terminator_api_device_handle_t *encl_handle,
                                                      fbe_u32_t *num_handles)
{
    fbe_status_t status;
    fbe_terminator_sas_encl_info_t sas_encl; 
    fbe_terminator_api_device_handle_t term_encl_handle;

    *num_handles = 0;

    memcpy(&sas_encl, encl_info, sizeof(*encl_info));

    /* Insert the VOYAGER ICM into the parent */
    status = fbe_api_terminator_insert_sas_enclosure (parent_handle, encl_info, encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW, "ICM connector_id 0 sas address: %llX\n",
	       (unsigned long long)encl_info->sas_address);


    /* Insert the two VOYAGER edge expanders into the ICM enclosure */
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE;
    //sas_encl.uid[0] = 1; // some unique ID.

    // Insert the first EE
    sas_encl.connector_id = 4;
    //sas_encl.sas_address = encl_info->sas_address + sas_encl.connector_id; // Add in the connector id to make this unique.
    sas_encl.sas_address = FBE_BUILD_LCC_SAS_ADDRESS(encl_info->backend_number, encl_info->encl_number, 0);
    status = fbe_api_terminator_insert_sas_enclosure (*encl_handle, &sas_encl, &term_encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW, "EE0 connector_id 2 sas address: %llX, ee0 handle %llu\n",
	       (unsigned long long)sas_encl.sas_address,
	       (unsigned long long)term_encl_handle);

    // Insert the second EE
    sas_encl.connector_id = 5;
    //sas_encl.sas_address = encl_info->sas_address + sas_encl.connector_id; // Add in the connector id to make this unique.
    sas_encl.sas_address = FBE_BUILD_LCC_SAS_ADDRESS(encl_info->backend_number, encl_info->encl_number, 1);
    status = fbe_api_terminator_insert_sas_enclosure (*encl_handle, &sas_encl, &term_encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW, "EE1 connector_id 3 sas address: %llX, ee1 handle %llu\n",
	       (unsigned long long)sas_encl.sas_address,
	       (unsigned long long)term_encl_handle);

    return status;
}

/*!****************************************************************************
 * @fn      fbe_zrt_internal_insert_voyager_sas_drive ()
 *
 * @brief
 * This function inserts a drive into the appropriate edge expander of
 * the voyager enclosure.  This is based on the slot number.  It is
 * assumed that the encl_handles is already setup.
 *
 * @param encl_handle - the terminator handle associated with the ICM.
 * @param slot_number - a number between 0 and 59 which indicates
 *                      where the drive is to be inserted.
 * @param drive_info - characteristics of the drive to be inserted.
 * @param drive_handle - used to return the handle for the drive inserted.
 *
 * @return
 *   FBE_STATUS_OK if all goes well.
 *
 * HISTORY
 *  06/01/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
static __inline fbe_status_t fbe_test_internal_insert_voyager_sas_drive (fbe_terminator_api_device_handle_t encl_handle,
                                       fbe_u32_t slot_number,
                                       fbe_terminator_sas_drive_info_t *drive_info, 
                                       fbe_terminator_api_device_handle_t *drive_handle)
{
    fbe_status_t status;
    fbe_terminator_api_device_handle_t temp_term_handle;
    fbe_u32_t temp_slot_number = slot_number;

    temp_term_handle = encl_handle;

    //status = fbe_terminator_api_enclosure_find_slot_parent (&temp_term_handle, &temp_slot_number);
    status = fbe_api_terminator_enclosure_find_slot_parent(&temp_term_handle, &temp_slot_number);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return fbe_api_terminator_insert_sas_drive (temp_term_handle, slot_number, drive_info, drive_handle);
}

