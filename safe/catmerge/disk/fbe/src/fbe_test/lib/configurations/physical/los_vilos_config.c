#include "mut.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"

#define MAUI_DRIVE_MAX 12

fbe_status_t fbe_test_load_los_vilos_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;

    fbe_api_terminator_device_handle_t  port0_handle;
    fbe_api_terminator_device_handle_t  encl0_0_handle;
    fbe_api_terminator_device_handle_t  drive_handle;

    fbe_u32_t                       slot = 0;
    fbe_u8_t encl_uid[FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE];

    /*insert a board*/
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a port*/
    status = fbe_test_pp_util_insert_sas_pmc_port(3, /* io port */
                                         5, /* portal */
                                         0, /* backend number */
                                         &port0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert an enclosure to port 0*/
    encl_uid[0] = 1;
    status = fbe_test_pp_util_insert_sas_enclosure(0, 0, encl_uid, 0x5000097A78009071, FBE_SAS_ENCLOSURE_TYPE_VIPER, port0_handle, &encl0_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (slot = 0; slot < LOS_VILOS_MAX_DRIVES; slot ++) 
    {
        if (slot < (LOS_VILOS_MAX_DRIVES/2)) {
            /*insert a sas_drive to port 0 encl 0 slot */
            status = fbe_test_pp_util_insert_sas_drive_extend(0, 0, slot, 520, 0x10B5C730,
                                                                0x5000097A78000000 + slot,
                                                                FBE_SAS_DRIVE_CHEETA_15K,
                                                                &drive_handle);
        }
        else
        {
            status = fbe_test_pp_util_insert_sas_drive_extend(0, 0, slot, 512, 0x10B5C730,
                                                                0x5000097A78000000 + slot,
                                                                FBE_SAS_DRIVE_UNICORN_512,
                                                                &drive_handle);
        }
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    return status;
}
