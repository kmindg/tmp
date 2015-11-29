#include "fbe/fbe_api_common.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_trace.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe/fbe_stat_api.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_emcutil_shell_include.h"

void fbe_stat_test_setup (void);
void fbe_stat_test_teardown(void);
void register_with_config_service(fbe_drive_configuration_handle_t *handle);

void run_stat_tests(fbe_drive_configuration_handle_t handle,
                    fbe_payload_cdb_fis_error_flags_t   error_flags_in,
                    fbe_u8_t *err_type_str,
                    fbe_bool_t test_fading,
                    fbe_u32_t stop_err_after_io_count);

fbe_u8_t * get_action_string(fbe_stat_action_t action);

void get_stat_ptr_and_error_tag (fbe_payload_cdb_fis_error_flags_t error_flags_in,
                            fbe_drive_stat_t drive_stat,
                            fbe_drive_error_t drive_error,
                            fbe_stat_t **stat_ptr,
                            fbe_u64_t *err_tag,
                            fbe_u32_t *threshold);


#define FBE_STAT_TEST_ENCL_UID_BASE         0x70
#define MAX_INTERLEAVE                      10
#define INTERLEAVE_STEP                     2

typedef struct action_to_string_s{
    fbe_stat_action_t   action;
    fbe_u8_t *          action_str;

}action_to_string_t;

action_to_string_t      action_str_table [] =
{
    FBE_API_ENUM_TO_STR(FBE_STAT_ACTION_FLAG_FLAG_NO_ACTION),
    FBE_API_ENUM_TO_STR(FBE_STAT_ACTION_FLAG_FLAG_RESET),
    FBE_API_ENUM_TO_STR(FBE_STAT_ACTION_FLAG_FLAG_POWER_CYCLE),
    FBE_API_ENUM_TO_STR(FBE_STAT_ACTION_FLAG_FLAG_SPINUP),
    FBE_API_ENUM_TO_STR(FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE),
    FBE_API_ENUM_TO_STR(FBE_STAT_ACTION_FLAG_FLAG_FAIL),
    FBE_API_ENUM_TO_STR(FBE_STAT_ACTION_FLAG_FLAG_RESET | FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE),

    {0xFFFF, NULL}
};

int __cdecl main (int argc , char **argv)
{
    fbe_drive_configuration_handle_t handle;

#include "fbe/fbe_emcutil_shell_maincode.h"

    /*build a configuration*/
    fbe_stat_test_setup();

    /*register with the drive configuration*/
    register_with_config_service(&handle);

    /*run the tests for killing the drive*/
    run_stat_tests(handle, FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_RECOVERED, "FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_RECOVERED", FBE_FALSE, 0);
    run_stat_tests(handle, FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_MEDIA, "FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_MEDIA" , FBE_FALSE, 0);
    run_stat_tests(handle, FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE, "FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE" , FBE_FALSE, 0);
    run_stat_tests(handle, FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_DATA, "FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_DATA", FBE_FALSE, 0);

    /*run the tests for showing fading*/
    #if 0 /* enable these on on special purpose, they take lot of time to print*/
    run_stat_tests(handle, FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_RECOVERED, "FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_RECOVERED", FBE_TRUE, 500);
    run_stat_tests(handle, FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_MEDIA, "FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_MEDIA" , FBE_TRUE, 200);
    run_stat_tests(handle, FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE, "FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE" , FBE_TRUE, 50);
    run_stat_tests(handle, FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_DATA, "FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_DATA", FBE_TRUE, 250);
    #endif
    /*destroy the physical package*/
    fbe_drive_configuration_unregister_drive(handle);
    fbe_stat_test_teardown();
}

/* Hardcoded configuration */
void fbe_stat_test_setup (void)
{
    fbe_status_t status;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t port_handle;
    fbe_terminator_api_device_handle_t        encl_id[8];
    fbe_terminator_api_device_handle_t        drive_id[120];
    fbe_u32_t                       i;
    /*fbe_terminator_sata_drive_info_t  sata_drive;*/

    /*before loading the physical package we initialize the terminator*/
    status = fbe_terminator_api_init();
    printf("%s: terminator initialized.", __FUNCTION__);

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DREADNOUGHT_HW_TYPE;
    status  = fbe_terminator_api_insert_board (&board_info);
  
    /*insert a port*/
    sas_port.sas_address = (fbe_u64_t)0x123456789;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 1;
    sas_port.backend_number = 0;

    status  = fbe_terminator_api_insert_sas_port (&sas_port, &port_handle);
    
    /*insert three enclosures */
    for (i = 0; i < 3; i++) {
        sas_encl.backend_number = sas_port.backend_number;
        sas_encl.encl_number = i;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
        sas_encl.sas_address = (fbe_u64_t)0x50000970000000FF + 0x0000000100000000 * i;
        sas_encl.connector_id = 0;
        sprintf(sas_encl.uid,"%02X%02X", FBE_STAT_TEST_ENCL_UID_BASE, i);
        status  = fbe_terminator_api_insert_sas_enclosure (port_handle, &sas_encl, &encl_id[i]);/*add encl on port 0*/
       
    }

    /* insert two drives into first enclosure */
    for(i = 0; i < 2; i++){
        /*insert a drive to the enclosure in slot 0*/
        sas_drive.backend_number = sas_port.backend_number;
        sas_drive.encl_number = 0;
        sas_drive.slot_number = i;
        sas_drive.capacity = 0x2000000;
        sas_drive.block_size = 520;
        sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
        sas_drive.sas_address = (fbe_u64_t)0x500FBE0000000000 + 0x0000000100000000 * i;
        memset(&sas_drive.drive_serial_number, i , sizeof(sas_drive.drive_serial_number));
        status = fbe_terminator_api_insert_sas_drive (encl_id[0], i, &sas_drive, &drive_id[i]);
       
    }

    /* insert two drives into second enclosure */
    for(i = 1 * 15; i < 1 * 15 + 2; i++){
        /*insert a drive to the enclosure in slot 0*/
        sas_drive.backend_number = sas_port.backend_number;
        sas_drive.encl_number = 1;
        sas_drive.slot_number = i - 1 * 15;
        sas_drive.capacity = 0x2000000;
        sas_drive.block_size = 520;
        sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
        sas_drive.sas_address = (fbe_u64_t)0x500FBE0000000000 + 0x0000000100000000 * i;
        memset(&sas_drive.drive_serial_number, i , sizeof(sas_drive.drive_serial_number));
        status = fbe_terminator_api_insert_sas_drive (encl_id[1], i - 1 * 15, &sas_drive, &drive_id[i]);
       
    }
#if 0
    /*insert a SATA drive to the third enclosure in slot 0*/
    sata_drive.backend_number = sas_port.backend_number;
    sata_drive.encl_number = 2;
    sata_drive.drive_type = FBE_SATA_DRIVE_SIM_512;
    sata_drive.sas_address = (fbe_u64_t)0x500FBE0000000000 + 0x0000000100000000 * i;
    sata_drive.capacity = 0x2000000;
    sata_drive.block_size = 512;
    memset(&sata_drive.drive_serial_number, '1' , sizeof(sas_drive.drive_serial_number));
    status = fbe_terminator_api_insert_sata_drive ( encl_id[2], 0, &sata_drive, &drive_id[30]);
    
    printf("=== Insert SATA drive to port 0, enclosure 2, slot 0 ===\n");
#endif
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);

    printf("=== starting physical pacakge ===\n");
    status = physical_package_init();
    

    EmcutilSleep(5000); /* 5 sec should be more than enough */
    printf("=== starting fbe_api ===\n");
    status = fbe_api_common_init();/*this would cause the map to be populated*/
    
    return;
}

void fbe_stat_test_teardown(void)
{
    fbe_status_t status;

    printf("=== destroying all configurations ===\n");

    status = fbe_api_common_destroy();
    status = physical_package_destroy();
    status = fbe_terminator_api_destroy();
   
    return;
}

void register_with_config_service(fbe_drive_configuration_handle_t *handle)
{
    fbe_drive_configuration_registration_info_t registration_info;

    registration_info.drive_type = FBE_DRIVE_TYPE_SAS;
    registration_info.drive_vendor = FBE_DRIVE_VENDOR_INVALID;
    fbe_zero_memory(registration_info.fw_revision, FBE_SCSI_INQUIRY_REVISION_SIZE + 1);
    fbe_zero_memory(registration_info.part_number , FBE_SCSI_INQUIRY_PART_NUMBER_SIZE + 1);
    fbe_zero_memory(registration_info.serial_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1);
    
    fbe_drive_configuration_register_drive(&registration_info, handle);

}

void run_stat_tests(fbe_drive_configuration_handle_t handle,
                    fbe_payload_cdb_fis_error_flags_t   error_flags_in,
                    fbe_u8_t *err_type_str,
                    fbe_bool_t test_fading,
                    fbe_u32_t stop_err_after_io_count)
{
    fbe_drive_error_t                   drive_error;
    fbe_stat_action_t                   stat_action = FBE_STAT_ACTION_FLAG_FLAG_NO_ACTION;
    fbe_payload_cdb_fis_error_flags_t   io_error = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
    fbe_u32_t                           io_count = 0, interleave = 0; 
    fbe_u8_t *                          action_str = NULL;
    fbe_u32_t                           ratio = 0, reset_ratio = 0;
    fbe_drive_stat_t                    drive_stat;
    fbe_stat_t *                        stat_ptr = NULL;
    fbe_u64_t                           err_tag = 0;
    fbe_u32_t                           threshold = 0;

    printf("\n\n====Inserting errors of type %s  ====\n", err_type_str);

    fbe_drive_configuration_get_threshold_info (handle, &drive_stat);

    for (interleave = 2; interleave < MAX_INTERLEAVE; interleave +=INTERLEAVE_STEP) {
        /*start clean*/
        printf("\nstarting new interleave: %d, for error type:%s \n", interleave, err_type_str);
        fbe_stat_drive_error_init(&drive_error);

        io_count = 0;

        /*run forever until the drive dies*/
        while (1) {

            stat_action = FBE_STAT_ACTION_FLAG_FLAG_NO_ACTION;
            /*inject interleaving errors*/
            if ((io_count % interleave) || ((io_count > stop_err_after_io_count) && test_fading)) {
                io_error = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            }else{
                io_error = error_flags_in;
            }

            fbe_stat_io_completion(-1, handle, &drive_error, io_error, &stat_action);

            if (io_error == FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR) {
                fbe_atomic_increment(&drive_error.io_counter);
            }

            action_str = get_action_string(stat_action);

            /*get pointer to the correct stat and error tag*/
            get_stat_ptr_and_error_tag (error_flags_in, drive_stat, drive_error, &stat_ptr, &err_tag, &threshold);

            /*get the error ratio*/
            fbe_stat_get_ratio(stat_ptr, drive_error.io_counter, err_tag, &ratio);
            
            if (stat_action & FBE_STAT_ACTION_FLAG_FLAG_RESET) {
                fbe_stat_drive_reset(handle, &drive_error);
            }

            /*get the reset ratio*/
            fbe_stat_get_ratio(&drive_stat.reset_stat, drive_error.io_counter, drive_error.reset_error_tag, &reset_ratio);

            if (action_str != NULL) {
                printf("IO:%d, Err Inj.:%s, Err-Ratio:%d, Err-Threshold:%d, Reset-ratio.:%d Action:%s\n",
                       io_count, ((io_error == FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR) ? "No" : "Yes"),
                       ratio, threshold, reset_ratio, action_str);
            }else{
                printf("IO:%d, Err Inj.:%s, Err-Ratio:%d, Err-Threshold:%d, Reset-ratio.:%d Multiple Action:0x%X\n",
                       io_count, ((io_error == FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR) ? "No" : "Yes"),
                       ratio, threshold, reset_ratio, stat_action);

            }

            if ((stat_action & FBE_STAT_ACTION_FLAG_FLAG_FAIL) || (test_fading && (io_count > (stop_err_after_io_count + 110000)))) {
                break;
            }

            io_count++;
        }
    }
}

fbe_u8_t * get_action_string(fbe_stat_action_t action)
{
    action_to_string_t * atos  = action_str_table;

    while (atos->action != 0xFFFF) {
        if (atos->action == action) {
            return atos->action_str;
        }

        atos++;
    }

    return NULL;

    
}

void get_stat_ptr_and_error_tag (fbe_payload_cdb_fis_error_flags_t error_flags_in,
                            fbe_drive_stat_t drive_stat,
                            fbe_drive_error_t drive_error,
                            fbe_stat_t **stat_ptr,
                            fbe_u64_t *err_tag,
                            fbe_u32_t *threshold)
{
    switch (error_flags_in) {
    case FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_DATA:
        *stat_ptr = &drive_stat.data_stat;
        *err_tag = drive_error.data_error_tag;
        break;
    case FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_RECOVERED:
        *stat_ptr = &drive_stat.recovered_stat;
        *err_tag = drive_error.recovered_error_tag;
        break;
    case FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_MEDIA:
        *stat_ptr = &drive_stat.media_stat;
        *err_tag = drive_error.media_error_tag;
        break;
    case FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE:
        *stat_ptr = &drive_stat.hardware_stat;
        *err_tag = drive_error.hardware_error_tag;
        break;
    case FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HEALTHCHECK:
        *stat_ptr = &drive_stat.healthCheck_stat;
        *err_tag = drive_error.healthCheck_error_tag;
        break;
    case FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_LINK:
        *stat_ptr = &drive_stat.link_stat;
        *err_tag = drive_error.link_error_tag;
        break;
    default:
        printf("\n\n\nUnknowm error flag\n\n\n");
        exit(0);
    }

    *threshold = (*stat_ptr)->threshold;
}


