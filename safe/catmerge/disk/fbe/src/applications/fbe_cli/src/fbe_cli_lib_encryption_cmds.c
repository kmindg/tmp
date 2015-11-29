/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file fbe_cli_lib_encryption_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contain fbe_cli functions used for encryption command.
 *
 * @version
 *  04-Oct-2013: Created 
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_lib_encryption_cmds.h"
#include "fbe_cli_private.h"
#include "fbe/fbe_cli.h"
#include "fbe/fbe_api_encryption_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "kms_api.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"


/****************************** 
 *    LOCAL FUNCTIONS
 ******************************/
static fbe_status_t fbe_cli_lib_encryption_set_mode(int argc, char** argv, fbe_system_encryption_info_t  encryption_info);

static  fbe_bool_t header_print_done  =  FBE_FALSE;
static 	kms_api_backup_t				backup;
static 	kms_api_log_t				cst_log;


static fbe_status_t fbe_cli_lib_encryption_push_keys_demo_version(void);

static fbe_status_t 
fbe_cli_lib_encryption_generate_key_demo_version(fbe_object_id_t object_id, 
											  fbe_config_generation_t generation_number,
											  fbe_raid_group_number_t   rg_number,
                                              fbe_u32_t width,
											  fbe_database_control_setup_encryption_key_t *key_info);
static fbe_status_t fbe_cli_lib_encryption_push_kek(int argc, char** argv);
static fbe_status_t fbe_cli_lib_encryption_unregister_kek(void);
static fbe_status_t fbe_cli_lib_encryption_push_kek_kek(int argc, char** argv);
static fbe_status_t fbe_cli_lib_encryption_unregister_kek_kek(int argc, char** argv);
static fbe_status_t fbe_cli_lib_encryption_reestablish_key_handle(int argc, char** argv);
static fbe_status_t fbe_cli_lib_encryption_display_ssv(int argc, char** argv);
static fbe_status_t fbe_cli_lib_encryption_remove_keys(int argc, char** argv);
static fbe_status_t fbe_cli_lib_encryption_display_status(int argc, char** argv);
static fbe_status_t fbe_cli_lib_encryption_backup_info(int argc, char** argv);
static fbe_status_t fbe_cli_lib_encryption_check_blocks(int argc, char** argv);
static fbe_status_t fbe_cli_lib_encryption_port_mode(int argc, char** argv);
static fbe_status_t fbe_cli_lib_encryption_push_dek(int argc, char** argv);
static fbe_status_t fbe_cli_lib_encryption_force_state(int argc, char** argv);
static fbe_status_t fbe_cli_lib_encryption_force_mode(int argc, char** argv);
static fbe_u8_t * fbe_cli_lib_convert_port_encryption_mode(fbe_port_encryption_mode_t mode);
static fbe_status_t fbe_cli_lib_encryption_pause(int argc, char** argv);
static fbe_status_t fbe_cli_lib_encryption_resume(int argc, char** argv);

 /*!***************************************************************
 * fbe_cli_encryption(int argc, char** argv)
 ****************************************************************
 * @brief
 *  This function is used to perform encryption operations on the system.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @author
 *  04-Oct-2013: Created
 *
 ****************************************************************/
void fbe_cli_encryption(int argc, char** argv)
{

    fbe_status_t                    status;
    fbe_system_encryption_info_t    encryption_info;

    header_print_done  =  FBE_FALSE;


    if (argc < 1) {
        fbe_cli_printf("%s", ENCRYPTION_USAGE);
        return;
    }

    if ((strcmp(*argv, "-help") == 0) ||(strcmp(*argv, "-h") == 0)) {

        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("%s", ENCRYPTION_USAGE);
        return;
    }// end of -help

    if ((strcmp(*argv, "-mode") == 0)){
        /* Get encryption info */
        status = fbe_api_get_system_encryption_info(&encryption_info);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error( "%s: Status:0x%x to get system encryption info\n", __FUNCTION__, status);
            return;
        }

        status= fbe_cli_lib_encryption_set_mode(argc, argv, encryption_info);

        if(status != FBE_STATUS_OK) {
            fbe_cli_error("Invalid system  operation !!\n");
            return;
        }

        return;
    }//end of -mode operation

	if ((strcmp(*argv, "-push_keys") == 0)){
		fbe_cli_lib_encryption_push_keys_demo_version();
		return;
	}

	if ((strcmp(*argv, "-get_enable_status") == 0)){
		kms_api_status_t kms_api_status, enable_status;
		kms_api_status = kms_api_get_enable_encryption_status(&enable_status);
		if(kms_api_status != KMS_API_STATUS_OK) {
			fbe_cli_error("Invalid kms_api_status %d\n", kms_api_status);
			return;
		}
		fbe_cli_printf("Enable encryption status: %d\n", enable_status);
		return;
	}

	if ((strcmp(*argv, "-rekey") == 0)){
		kms_api_status_t kms_api_status;
		kms_api_status = kms_api_start_rekey();
		if(kms_api_status != KMS_API_STATUS_OK) {
			fbe_cli_error("Invalid kms_api_status %d\n", kms_api_status);
			return;
		}
		return;
	}

	if ((strcmp(*argv, "-remove_keys") == 0)){
		fbe_cli_lib_encryption_remove_keys(argc, argv);
		return;
	}

    if((strcmp(*argv, "-push_kek") == 0)){
        fbe_cli_lib_encryption_push_kek(argc, argv);
        return;
    }

    if((strcmp(*argv, "-unregister_kek") == 0)){
        fbe_cli_lib_encryption_unregister_kek();
        return;
    }

    if((strcmp(*argv, "-push_kek_kek") == 0)){
        fbe_cli_lib_encryption_push_kek_kek(argc, argv);
        return;
    }

    if((strcmp(*argv, "-unregister_kek_kek") == 0)){
        fbe_cli_lib_encryption_unregister_kek_kek(argc, argv);
        return;
    }

    if((strcmp(*argv, "-reestablish") == 0)){
        fbe_cli_lib_encryption_reestablish_key_handle(argc, argv);
        return;
    }
#if 0
    if((strcmp(*argv, "-ssv") == 0)){
        fbe_cli_lib_encryption_display_ssv(argc, argv);
        return;
    }
#endif
    if((strcmp(*argv, "-status") == 0)) {
        fbe_cli_lib_encryption_display_status(argc, argv);
        return;
    }

    if((strcmp(*argv, "-backup_start") == 0)) {
		kms_api_status_t kms_api_status;
        kms_api_status = kms_api_start_backup(&backup);
        if(kms_api_status != KMS_API_STATUS_OK) {
            fbe_cli_error("Invalid kms_api_status %d\n", kms_api_status);
            return;
        }

		fbe_cli_printf("Backup file location: %s\n", backup.fname);
        return;
    }

    if((strcmp(*argv, "-backup_complete") == 0)) {
		kms_api_status_t kms_api_status;
        kms_api_status = kms_api_complete_backup(&backup);
        if(kms_api_status != KMS_API_STATUS_OK) {
            fbe_cli_error("Invalid kms_api_status %d\n", kms_api_status);
            return;
        }
		fbe_cli_printf("Backup completed for file: %s\n", backup.fname);
        return;
    }

    if((strcmp(*argv, "-backup_restore") == 0)) {
		kms_api_status_t kms_api_status;
        if (argc < 2) {
            fbe_cli_error("needs to provide file name\n");
            return;
        }
		fbe_copy_memory(backup.fname, argv[1], KMS_API_FILE_NAME_SIZE);
		fbe_cli_printf("Restore backup file: %s\n", backup.fname);
        kms_api_status = kms_api_restore(&backup);
        if(kms_api_status == KMS_API_STATUS_WRONG_SP) {
            fbe_cli_error("This command should be used on active SP only\n");
            return;
        }
        if(kms_api_status == KMS_API_STATUS_NOT_AVAILABLE) {
            fbe_cli_error("This command cannot be used on live system!\n");
            return;
        }
        if(kms_api_status == KMS_API_STATUS_UNENCRYPTED) {
            fbe_cli_error("Ecnryption not enabled\n");
            return;
        }
        if(kms_api_status != KMS_API_STATUS_OK) {
            fbe_cli_error("Invalid kms_api_status %d\n", kms_api_status);
            return;
        }
        return;
    }

    if((strcmp(*argv, "-log_start") == 0)) {
		kms_api_status_t kms_api_status;
        kms_api_status = kms_api_start_log(&cst_log);
        if(kms_api_status != KMS_API_STATUS_OK) {
            fbe_cli_error("Invalid kms_api_status %d\n", kms_api_status);
            return;
        }

		fbe_cli_printf("Log file location: %s\n", cst_log.fname);
        return;
    }

    if((strcmp(*argv, "-log_complete") == 0)) {
		kms_api_status_t kms_api_status;
        kms_api_status = kms_api_complete_log(&cst_log);
        if(kms_api_status != KMS_API_STATUS_OK) {
            fbe_cli_error("Invalid kms_api_status %d\n", kms_api_status);
            return;
        }
		fbe_cli_printf("Log completed for file: %s\n", cst_log.fname);
        return;
    }

    if((strcmp(*argv, "-log_hash_start") == 0)) {
		kms_api_status_t kms_api_status;
        kms_api_status = kms_api_start_hash_log(&cst_log);
        if(kms_api_status != KMS_API_STATUS_OK) {
            fbe_cli_error("Invalid kms_api_status %d\n", kms_api_status);
            return;
        }
		fbe_cli_printf("Log hash file: %s\n", cst_log.fname);
        return;
    }

    if((strcmp(*argv, "-log_hash_complete") == 0)) {
		kms_api_status_t kms_api_status;
        kms_api_status = kms_api_complete_hash_log(&cst_log);
        if(kms_api_status != KMS_API_STATUS_OK) {
            fbe_cli_error("Invalid kms_api_status %d\n", kms_api_status);
            return;
        }
		fbe_cli_printf("Log hash completed for file: %s\n", cst_log.fname);
        return;
    }



    if((strcmp(*argv, "-backup_info") == 0)) {
        fbe_cli_lib_encryption_backup_info(argc, argv);
        return;
    }

    if((strcmp(*argv, "-check_blocks") == 0)) {
        fbe_cli_lib_encryption_check_blocks(argc, argv);
        return;
    }

    if((strcmp(*argv, "-port") == 0)) {
        fbe_cli_lib_encryption_port_mode(argc, argv);
        return;
    }

    if((strcmp(*argv, "-push_dek") == 0)) {
        fbe_cli_lib_encryption_push_dek(argc, argv);
        return;
    }

    if((strcmp(*argv, "-force_state") == 0)) {
        fbe_cli_lib_encryption_force_state(argc, argv);
        return;
    }

    if((strcmp(*argv, "-force_mode") == 0)) {
        fbe_cli_lib_encryption_force_mode(argc, argv);
        return;
    }

    if((strcmp(*argv, "-pause") == 0)){
        //fbe_cli_lib_encryption_pause(argc, argv);
        return;
    }

    if((strcmp(*argv, "-resume") == 0)){
        //fbe_cli_lib_encryption_resume(argc, argv);
        return;
    }

    fbe_cli_printf("\nInvalid operation!!\n\n");
    fbe_cli_printf("%s", ENCRYPTION_USAGE);
    return;

}


 /*!********************************************************************************
 * fbe_cli_lib_encryption_set_state(int argc, char** argv, fbe_system_encryption_info_t  encryption_info)
***********************************************************************************
 * @brief
 *  This function is used to set the encryption state of the system.
 *
 *  @param    argc :argument count
 *  @param    argv :argument string
 *  @param    encryption_info : encryption related information in the system
 *
 * @author
 *  04-Oct-2013 - created
 *                  
 **********************************************************************************/ 
static fbe_status_t fbe_cli_lib_encryption_set_mode(int argc, char** argv, fbe_system_encryption_info_t  encryption_info)
{
    fbe_status_t   status = FBE_STATUS_OK;
	fbe_u64_t nonce;

    argv++;
    argc--;

    if (argc == 0) {
        switch(encryption_info.encryption_mode) {
            case FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED:
                fbe_cli_printf("\nSystem Encryption is set to ENCRYPTED.  Mode: 0x%lx\n\n", (unsigned long)encryption_info.encryption_mode);
                break;
            case FBE_SYSTEM_ENCRYPTION_MODE_UNENCRYPTED:
                fbe_cli_printf("\nSystem Encryption is set to UNENCRYPTED. Mode: 0x%lx\n\n", (unsigned long)encryption_info.encryption_mode);
                break;
            case FBE_SYSTEM_ENCRYPTION_MODE_INVALID:
                fbe_cli_printf("\nSystem Encryption is set to NONE. Mode: 0x%lx\n\n", (unsigned long)encryption_info.encryption_mode);
                break;
            default:
                fbe_cli_printf("\nNo System Encryption is set to Mode: 0x%lx\n\n", (unsigned long)encryption_info.encryption_mode);
                break;
        }

        return status;
    }

    if(strcmp(*argv, "enable") == 0) 
    {
		kms_api_status_t kms_api_status;
#if 0 /* Disabled for kms_api testing */
        if(encryption_info.encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) {
            fbe_cli_printf("System Encryption State is already set to ENCRYPTED.\n");
            return status;
        }
#endif
		kms_api_activate_encryption(&nonce);
		kms_api_status = kms_api_enable_encryption(nonce);

        //status  = fbe_api_enable_system_encryption();
        
        if (kms_api_status != KMS_API_STATUS_OK) {
            fbe_cli_error( "%s: failed to set Encryption to ENCRYPTED\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_cli_printf("System Encryption successfully set to ENCRYPTED! \n\n");  

        return FBE_STATUS_OK;
    }

    if(strcmp(*argv, "disable") == 0) {
        if(encryption_info.encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_UNENCRYPTED) {
            fbe_cli_printf("System Encryption is already set to UNENCRYPTED.\n");
            return status;
        }

        status  = fbe_api_disable_system_encryption();
        if (status != FBE_STATUS_OK) {
            fbe_cli_error( "%s: failed to set encryption to UNENCRYPTED.\n", __FUNCTION__);
            return status;
        }

        fbe_cli_printf("System encryption successfully sets to UNENCRYPTED!\n\n");

        return status;
    }

    if(strcmp(*argv, "clear") == 0) {
        if(encryption_info.encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_INVALID) {
            fbe_cli_printf("System Encryption is already cleared.\n");
            return status;
        }

        status  = fbe_api_clear_system_encryption();
        if (status != FBE_STATUS_OK) {
            fbe_cli_error( "%s: failed to reset encryption to NONE.\n", __FUNCTION__);
            return status;
        }

        fbe_cli_printf("System encryption resets to NONE successfully!\n\n");

        return status;
    }

    fbe_cli_printf("%s", ENCRYPTION_USAGE);

    return status;
} //end of fbe_cli_lib_encryption_set_state()


static fbe_status_t 
fbe_cli_lib_encryption_generate_key_demo_version(fbe_object_id_t object_id, 
											  fbe_config_generation_t generation_number,
											  fbe_raid_group_number_t   rg_number,
                                              fbe_u32_t width,
											  fbe_database_control_setup_encryption_key_t *key_info)
{
    fbe_u8_t            keys[FBE_ENCRYPTION_KEY_SIZE];
    fbe_u32_t           index;


    fbe_zero_memory(keys, FBE_ENCRYPTION_KEY_SIZE);

    key_info->key_setup.num_of_entries = 1;
    key_info->key_setup.key_table_entry[0].object_id = object_id;
    key_info->key_setup.key_table_entry[0].num_of_keys = width;
    key_info->key_setup.key_table_entry[0].generation_number = generation_number;

    for(index = 0; index < key_info->key_setup.key_table_entry[0].num_of_keys; index++)
    {
        _snprintf(&keys[0], FBE_ENCRYPTION_KEY_SIZE, "FBE_%02d_%02d",index, object_id);
        fbe_copy_memory(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key1, keys, FBE_ENCRYPTION_KEY_SIZE);
    }
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_cli_lib_encryption_push_keys_demo_version(void)
{
	fbe_status_t status;	
	fbe_database_control_setup_encryption_key_t     encryption_key_table;
	fbe_u32_t 						                total_rgs;
	fbe_u32_t							            returned_rgs;
	fbe_database_control_get_locked_object_info_t *	rg_info = NULL;
	fbe_u32_t							i;

	fbe_cli_printf( "CLI push key demo Start ...");

	/* Enumerate all raid groups */
	status = fbe_api_database_get_total_locked_objects_of_class(DATABASE_CLASS_ID_RAID_GROUP, &total_rgs);
	if (status != FBE_STATUS_OK) {
		fbe_cli_printf( "CLI failed to get locked RG count");
		return status;
	}

	if(total_rgs == 0){
		fbe_cli_printf( "CLI push key demo End");
		return FBE_STATUS_OK;
	}

	rg_info = (fbe_database_control_get_locked_object_info_t *)fbe_api_allocate_memory(total_rgs * sizeof(fbe_database_control_get_locked_object_info_t));
	fbe_zero_memory(rg_info, total_rgs * sizeof(fbe_database_control_get_locked_object_info_t));

	status = fbe_api_database_get_locked_object_info_of_class(DATABASE_CLASS_ID_RAID_GROUP, rg_info, total_rgs, &returned_rgs);
	if (status != FBE_STATUS_OK) {
		fbe_api_free_memory(rg_info);
		fbe_cli_printf( "CLI failed to get locked RG's");
		return status;
	}

    for (i = 0; i < returned_rgs; i++){		

		status = fbe_cli_lib_encryption_generate_key_demo_version(rg_info[i].object_id,
																rg_info[i].generation_number, 
																rg_info[i].control_number, 
                                                                rg_info[i].width,
																&encryption_key_table);
		if(status != FBE_STATUS_OK){
			fbe_cli_printf( "CLI failed to generate key for object_id 0x%X", rg_info[i].object_id);
			continue;
		}

		fbe_cli_printf( "CLI push key to object_id 0x%X", rg_info[i].object_id);

		status = fbe_api_database_setup_encryption_key(&encryption_key_table);
		if(status != FBE_STATUS_OK){
			fbe_cli_printf( "CLI failed to push key to object_id 0x%X", rg_info[i].object_id);
			continue;
		}
	} /* for (i = 0; i < returned_rgs; i++) */


	fbe_api_free_memory(rg_info);

	fbe_cli_printf( "CLI push key demo End");

	return FBE_STATUS_OK;
}

static fbe_status_t fbe_cli_lib_encryption_push_kek(int argc, char** argv)
{
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_database_control_setup_kek_t kek_info;
    fbe_database_control_get_be_port_info_t be_port_info;
    fbe_u32_t sp_count = 0;
    fbe_u32_t port_count;
    fbe_u32_t index;
    fbe_u8_t key[FBE_ENCRYPTION_WRAPPED_KEK_SIZE];

    argv++;
    argc--;

    if (argc == 0) {
        /* The user did not provide it, so need to generate one here */
        /**** Insert code that generates KEK here *****/
        fbe_cli_printf( "KEK generation not supported yet \n");
        //fbe_cli_lib_encryption_generate_kek(&kek_info);
    }
    else{
        for (index = 0; index < FBE_ENCRYPTION_WRAPPED_KEK_SIZE ; index ++)
        {
            if (argc <= 0)
            {
                fbe_cli_error("not enough bytes %d. Should be %d bytes\n", index, FBE_ENCRYPTION_WRAPPED_KEK_SIZE);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            key[index] = (fbe_u8_t) fbe_atoh(*argv);
            argc--;
            argv++; 
        }
        be_port_info.sp_id = FBE_CMI_SP_ID_A;
        for (sp_count = 0; sp_count < 2; sp_count++)
        {
            status = fbe_api_database_get_be_port_info(&be_port_info);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Unable to get port info on SP: %d\n", be_port_info.sp_id);
                be_port_info.sp_id = FBE_CMI_SP_ID_B;
                continue;
            }
            fbe_cli_printf("Setting KEKs on SP %s\n", (be_port_info.sp_id == FBE_CMI_SP_ID_A) ? "A" : "B");
            for (port_count = 0; port_count< be_port_info.num_be_ports; port_count++)
            {
                fbe_cli_printf("\tSetting up KEK for BE Port %d SN:%s...", 
                               be_port_info.port_info[port_count].be_number, 
                               be_port_info.port_info[port_count].serial_number);
                fbe_zero_memory(&kek_info, sizeof(fbe_database_control_setup_kek_t));
                kek_info.key_size = FBE_ENCRYPTION_WRAPPED_KEK_SIZE;
                fbe_copy_memory(&kek_info.kek[0], key, FBE_ENCRYPTION_WRAPPED_KEK_SIZE);
                kek_info.port_object_id = be_port_info.port_info[port_count].port_object_id;
                kek_info.sp_id = be_port_info.sp_id;
                status = fbe_api_database_setup_kek(&kek_info);
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("Kek Push failed\n");
                }
                else
                {
                    fbe_cli_printf("Done. KEK Handle 0x%llx\n", (unsigned long long)kek_info.kek_handle);
                }
            }
            be_port_info.sp_id = FBE_CMI_SP_ID_B;
        } 
    }
 
    return status;
}

static fbe_status_t fbe_cli_lib_encryption_unregister_kek(void)
{
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_database_control_destroy_kek_t kek_info;
    fbe_database_control_get_be_port_info_t be_port_info;
    fbe_u32_t sp_count = 0;
    fbe_u32_t port_count;

    be_port_info.sp_id = FBE_CMI_SP_ID_A;
    for (sp_count = 0; sp_count < 2; sp_count++)
    {
        status = fbe_api_database_get_be_port_info(&be_port_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Unable to get port info on SP: %d\n", be_port_info.sp_id);
            continue;
        }
        fbe_cli_printf("Destroying KEKs on SP %s\n", (be_port_info.sp_id == FBE_CMI_SP_ID_A) ? "A" : "B");
        for (port_count = 0; port_count< be_port_info.num_be_ports; port_count++)
        {
            fbe_cli_printf("\tDestroying KEK for BE Port %d...", be_port_info.port_info[port_count].be_number);
            fbe_zero_memory(&kek_info, sizeof(fbe_database_control_destroy_kek_t));
            kek_info.port_object_id = be_port_info.port_info[port_count].port_object_id;
            kek_info.sp_id = be_port_info.sp_id;
            status = fbe_api_database_destroy_kek(&kek_info);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Kek Destroy failed\n");
            }
            else
            {
                fbe_cli_printf("Done\n");
            }
        }
        be_port_info.sp_id = FBE_CMI_SP_ID_B;
    } 

    return status;
}


static fbe_status_t fbe_cli_lib_encryption_push_kek_kek(int argc, char** argv)
{
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_database_control_setup_kek_kek_t kek_info;
    fbe_database_control_get_be_port_info_t be_port_info;
    fbe_database_control_port_encryption_mode_t mode_info;
    fbe_u32_t sp_count = 0;
    fbe_u32_t port_count;
    fbe_u32_t index;
    fbe_u8_t key[FBE_ENCRYPTION_KEK_KEK_SIZE];

    argv++;
    argc--;

    fbe_zero_memory(&kek_info, sizeof(fbe_database_control_setup_kek_kek_t));

    if (argc == 0) {
        /* The user did not provide it, so need to generate one here */
        /**** Insert code that generates KEK here *****/
        fbe_cli_printf( "KEK generation not supported yet \n");
        //fbe_cli_lib_encryption_generate_kek(&kek_info);
    }
    else{
        for (index = 0; index < FBE_ENCRYPTION_KEK_KEK_SIZE ; index ++)
        {
            if (argc <= 0)
            {
                fbe_cli_error("not enough bytes %d. Should be %d bytes\n", index, FBE_ENCRYPTION_KEK_KEK_SIZE);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            key[index] = (fbe_u8_t) fbe_atoh(*argv);
            argc--;
            argv++; 
        }

        be_port_info.sp_id = FBE_CMI_SP_ID_A;

        for (sp_count = 0; sp_count < 2; sp_count++)
        {
            status = fbe_api_database_get_be_port_info(&be_port_info);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Unable to get port info on SP: %d\n", be_port_info.sp_id);
                be_port_info.sp_id = FBE_CMI_SP_ID_B;
                continue;
            }
            fbe_cli_printf("Setting KEKs KEK on SP %s\n", (be_port_info.sp_id == FBE_CMI_SP_ID_A) ? "A" : "B");

            /* we first need to put ALL the ports in a particular mode */
            for (port_count = 0; port_count< be_port_info.num_be_ports; port_count++)
            {
                fbe_cli_printf("\tPutting BE Port %d in Wrapped DEK mode ...", be_port_info.port_info[port_count].be_number);
                mode_info.port_object_id = be_port_info.port_info[port_count].port_object_id;
                mode_info.sp_id = be_port_info.sp_id;
                mode_info.mode = FBE_PORT_ENCRYPTION_MODE_WRAPPED_DEKS;

                status = fbe_api_database_set_port_encryption_mode(&mode_info);
                if (status != FBE_STATUS_OK) {
                    fbe_cli_printf("Mode Set failed\n");
                } 
                else {
                    fbe_cli_printf("Done.\n");
                }
            }

            /* Waiting for all the mode to change */
            fbe_cli_printf("\tWaiting 5s for mode change...\n");
            fbe_api_sleep(5000);

            for (port_count = 0; port_count< be_port_info.num_be_ports; port_count++)
            {
                mode_info.port_object_id = be_port_info.port_info[port_count].port_object_id;
                mode_info.sp_id = be_port_info.sp_id;
                status = fbe_api_database_set_port_encryption_mode(&mode_info);
                if( mode_info.mode != FBE_PORT_ENCRYPTION_MODE_WRAPPED_DEKS) {
                    fbe_cli_printf("\t Mode not set currectly for BE Port %d...", 
                                   be_port_info.port_info[port_count].be_number);
                    continue;
                }

                fbe_cli_printf("\tSetting up KEK KEK for BE Port %d...", be_port_info.port_info[port_count].be_number);
                fbe_zero_memory(&kek_info, sizeof(fbe_database_control_setup_kek_kek_t));
                kek_info.key_size = FBE_ENCRYPTION_KEK_KEK_SIZE;
                fbe_copy_memory(&kek_info.kek_kek[0], key, FBE_ENCRYPTION_KEK_KEK_SIZE);
                kek_info.port_object_id = be_port_info.port_info[port_count].port_object_id;
                kek_info.sp_id = be_port_info.sp_id;
                status = fbe_api_database_setup_kek_kek(&kek_info);
                if (status != FBE_STATUS_OK) {
                    fbe_cli_printf("Kek Kek Push failed\n");
                } 
                else {
                    fbe_cli_printf("Done. KEK KEK Handle 0x%llx\n", (unsigned long long)kek_info.kek_kek_handle);
                }
            }
            
            /* we first need to put ALL the ports in a particular mode */
            for (port_count = 0; port_count< be_port_info.num_be_ports; port_count++)
            {
                fbe_cli_printf("\tPutting BE Port %d in Wrapped KEK-DEK Mode...", be_port_info.port_info[port_count].be_number);
                mode_info.port_object_id = be_port_info.port_info[port_count].port_object_id;
                mode_info.sp_id = be_port_info.sp_id;
                mode_info.mode = FBE_PORT_ENCRYPTION_MODE_WRAPPED_KEKS_DEKS;

                status = fbe_api_database_set_port_encryption_mode(&mode_info);
                if (status != FBE_STATUS_OK){
                    fbe_cli_printf("Mode Set failed\n");
                } 
                else {
                    fbe_cli_printf("Done.\n");
                }
            }
            be_port_info.sp_id = FBE_CMI_SP_ID_B;
        } 
    }
    return status;
}

static fbe_status_t fbe_cli_lib_encryption_unregister_kek_kek(int argc, char** argv)
{
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_database_control_destroy_kek_kek_t kek_info;
    fbe_database_control_get_be_port_info_t be_port_info;
    fbe_u32_t sp_count = 0;
    fbe_u32_t port_count;
    fbe_database_control_port_encryption_mode_t mode_info;

    be_port_info.sp_id = FBE_CMI_SP_ID_A;
    for (sp_count = 0; sp_count < 2; sp_count++)
    {
        status = fbe_api_database_get_be_port_info(&be_port_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Unable to get port info on SP: %d\n", be_port_info.sp_id);
            be_port_info.sp_id = FBE_CMI_SP_ID_B;
            continue;
        }
        fbe_cli_printf("Destroying KEK of KEKs on SP %s\n", (be_port_info.sp_id == FBE_CMI_SP_ID_A) ? "A" : "B");

        /* we first need to put ALL the ports in a particular mode */
        for (port_count = 0; port_count< be_port_info.num_be_ports; port_count++)
        {
            fbe_cli_printf("\tPutting BE Port %d in Wrapped DEK mode ...", be_port_info.port_info[port_count].be_number);
            mode_info.port_object_id = be_port_info.port_info[port_count].port_object_id;
            mode_info.sp_id = be_port_info.sp_id;
            mode_info.mode = FBE_PORT_ENCRYPTION_MODE_WRAPPED_DEKS;

            status = fbe_api_database_set_port_encryption_mode(&mode_info);
            if (status != FBE_STATUS_OK){
                fbe_cli_printf("Mode Set failed\n");
            } else {
                fbe_cli_printf("Done.\n");
            }
        }

        /* Waiting for all the mode to change */
        fbe_cli_printf("\tWaiting 5s for mode change...\n");
        fbe_api_sleep(5000);

        for (port_count = 0; port_count< be_port_info.num_be_ports; port_count++)
        {
            fbe_cli_printf("\tDestroying KEK of KEK for BE Port %d...", be_port_info.port_info[port_count].be_number);
            fbe_zero_memory(&kek_info, sizeof(fbe_database_control_destroy_kek_kek_t));
            kek_info.port_object_id = be_port_info.port_info[port_count].port_object_id;
            kek_info.sp_id = be_port_info.sp_id;
            status = fbe_api_database_destroy_kek_kek(&kek_info);
            if (status != FBE_STATUS_OK){
                fbe_cli_printf("Kek Kek Destroy failed\n");
            }
            else {
                fbe_cli_printf("Done\n");
            }
        }
        be_port_info.sp_id = FBE_CMI_SP_ID_B;
    } 
    return status;
}

static fbe_status_t fbe_cli_lib_encryption_reestablish_key_handle(int argc, char** argv)
{
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_database_control_reestablish_kek_kek_t kek_info;
    fbe_database_control_get_be_port_info_t be_port_info;
    fbe_u32_t sp_count = 0;
    fbe_u32_t port_count;

    be_port_info.sp_id = FBE_CMI_SP_ID_A;
    for (sp_count = 0; sp_count < 2; sp_count++)
    {
        status = fbe_api_database_get_be_port_info(&be_port_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Unable to get port info on SP: %d\n", be_port_info.sp_id);
            be_port_info.sp_id = FBE_CMI_SP_ID_B;
            continue;
        }
        fbe_cli_printf("Reestablish KEK of KEKs on SP %s\n", (be_port_info.sp_id == FBE_CMI_SP_ID_A) ? "A" : "B");
        for (port_count = 0; port_count< be_port_info.num_be_ports; port_count++)
        {
            fbe_cli_printf("Reestablish KEK of KEK for BE Port %d...", be_port_info.port_info[port_count].be_number);
            fbe_zero_memory(&kek_info, sizeof(fbe_database_control_reestablish_kek_kek_t));
            kek_info.port_object_id = be_port_info.port_info[port_count].port_object_id;
            kek_info.sp_id = be_port_info.sp_id;
            status = fbe_api_database_reestablish_kek_kek(&kek_info);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Kek Kek reestablish failed\n");
            }
            else
            {
                fbe_cli_printf("Done\n");
            }
        }
        be_port_info.sp_id = FBE_CMI_SP_ID_B;
    } 
    return status;
}

static fbe_status_t fbe_cli_lib_encryption_push_dek(int argc, char** argv)
{
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_base_port_mgmt_debug_register_dek_t dek_info;
    fbe_database_control_get_be_port_info_t be_port_info;
    fbe_u32_t port_count;
    fbe_u32_t index;
    fbe_u8_t key[FBE_ENCRYPTION_KEY_SIZE];
    fbe_cmi_service_get_info_t cmi_info;

    argv++;
    argc--;

    if (argc == 0) {
        /* The user did not provide it, so need to generate one here */
        /**** Insert code that generates KEK here *****/
        fbe_cli_printf( "KEK generation not supported yet \n");
        //fbe_cli_lib_encryption_generate_kek(&kek_info);
    }
    else{
        for (index = 0; index < FBE_ENCRYPTION_KEY_SIZE ; index ++)
        {
            if (argc <= 0)
            {
                fbe_cli_error("not enough bytes %d. Should be %d bytes\n", index, FBE_ENCRYPTION_KEY_SIZE);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            key[index] = (fbe_u8_t) fbe_atoh(*argv);
            argc--;
            argv++; 
        }
        fbe_api_cmi_service_get_info(&cmi_info);
        be_port_info.sp_id = cmi_info.sp_id;
        status = fbe_api_database_get_be_port_info(&be_port_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Unable to get port info on SP: %d\n", be_port_info.sp_id);
            return status;
        }
        fbe_cli_printf("Setting DEKs on SP %s\n", (be_port_info.sp_id == FBE_CMI_SP_ID_A) ? "A" : "B");
        for (port_count = 0; port_count< be_port_info.num_be_ports; port_count++)
        {
            fbe_cli_printf("\tSetting up DEK for BE Port %d SN:%s...", 
                           be_port_info.port_info[port_count].be_number, 
                           be_port_info.port_info[port_count].serial_number);
            fbe_zero_memory(&dek_info, sizeof(fbe_base_port_mgmt_debug_register_dek_t));
            dek_info.key_size = FBE_ENCRYPTION_KEY_SIZE;
            fbe_copy_memory(&dek_info.key[0], key, FBE_ENCRYPTION_KEY_SIZE);
            status = fbe_api_port_debug_register_dek(be_port_info.port_info[port_count].port_object_id,
                                                     &dek_info);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Dek Push failed\n");
            }
            else
            {
                fbe_cli_printf("Done. DEK Handle 0x%llx\n", (unsigned long long)dek_info.key_handle);
            }
        }
    }
 
    return status;
}

static fbe_status_t fbe_cli_lib_encryption_force_state(int argc, char** argv)
{
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_base_config_encryption_state_t state = FBE_BASE_CONFIG_ENCRYPTION_STATE_INVALID;
    fbe_object_id_t object_id;

    argv++;
    argc--;

    if (argc != 4) {
        fbe_cli_printf("%s", ENCRYPTION_USAGE);
        return status;
    }

    while(argc > 0) {
        if((strcmp(*argv, "-object_id") == 0)) {
            argc--;
            argv++;
            object_id = fbe_atoh(*argv);
        } else if((strcmp(*argv, "-state") == 0)) {
            argc--;
            argv++;
            state = fbe_atoi(*argv);
        }
        argc --;
        argv++;
    }
    if (state == FBE_BASE_CONFIG_ENCRYPTION_STATE_INVALID) {
        fbe_cli_printf("please provide the -state option\n");
        fbe_cli_printf("%s", ENCRYPTION_USAGE);
        return status;
    }
    status = fbe_api_base_config_set_encryption_state(object_id, state);
    if(status == FBE_STATUS_OK) {
        fbe_cli_printf("Setting Encrytpion State to %d for Object ID:0x%08x succeeded\n", state, object_id);
    } else {
        fbe_cli_printf("Setting Encrytpion State failed. Status:%d\n", status );
    }
    return status;
}

static fbe_status_t fbe_cli_lib_encryption_force_mode(int argc, char** argv)
{
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_base_config_encryption_mode_t mode = FBE_BASE_CONFIG_ENCRYPTION_MODE_INVALID;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_object_id_t object_id;

    argv++;
    argc--;

    if (argc != 4) {
        fbe_cli_printf("%s", ENCRYPTION_USAGE);
        return status;
    }

    while(argc > 0) {
        if((strcmp(*argv, "-object_id") == 0)) {
            argc--;
            argv++;
            object_id = fbe_atoh(*argv);
        } else if((strcmp(*argv, "-mode") == 0)) {
            argc--;
            argv++;
            mode = fbe_atoi(*argv);
        }
        argc --;
        argv++;
    }

    if (mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_INVALID){
        fbe_cli_printf("please provide the -mode option\n");
        fbe_cli_printf("%s", ENCRYPTION_USAGE);
        return status;
    }
    status = fbe_api_raid_group_get_info(object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    if(status != FBE_STATUS_OK) {
        fbe_cli_printf("Error in getting RG information for RG Object ID:0x%08x status:%d, Type:%d\n",
                       object_id, status, rg_info.raid_type);
    }
    status = fbe_api_change_rg_encryption_mode(object_id, mode);
    if(status == FBE_STATUS_OK) {
        fbe_cli_printf("Setting Encrytpion Mode to %d for Object ID:0x%08x succeeded\n", mode, object_id);
    } else {
        fbe_cli_printf("Setting Encrytpion Mode failed. Status:%d\n", mode );
    }
    return status;

}
static fbe_status_t fbe_cli_lib_encryption_display_ssv(int argc, char** argv)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_encryption_hardware_ssv_t       ssv;
    
    status = fbe_api_get_hardware_ssv_data(&ssv);
    if (status == FBE_STATUS_OK)
    {
        if(ssv.hw_ssv_midplane_sn[0] == 0) {
            fbe_cli_printf("Midplane Serial Number Unavailable\n");
        } else {
            fbe_cli_printf("Midplane Serial Number: %s\n", ssv.hw_ssv_midplane_sn);
        }
        if(ssv.hw_ssv_spa_sn[0] == 0) {
            fbe_cli_printf("SPA Serial Number Unavailable\n");
        } else {
            fbe_cli_printf("SPA Serial Number: %s\n", ssv.hw_ssv_spa_sn);
        }
        if(ssv.hw_ssv_spb_sn[0] == 0) {
            fbe_cli_printf("SPB Serial Number Unavailable\n");
        } else {
            fbe_cli_printf("SPB Serial Number: %s\n", ssv.hw_ssv_spb_sn);
        }
        if(ssv.hw_ssv_mgmta_sn[0] == 0) {
            fbe_cli_printf("MGMT A Serial Number Unavailable\n");
        } else {
            fbe_cli_printf("MGMT A Serial Number: %s\n", ssv.hw_ssv_mgmta_sn);
        }
        if(ssv.hw_ssv_mgmtb_sn[0] == 0) {
            fbe_cli_printf("MGMT B Serial Number Unavailable\n");
        } else {
            fbe_cli_printf("MGMT B Serial Number: %s\n", ssv.hw_ssv_mgmtb_sn);
        }
       
    }
    else
    {
        fbe_cli_printf("Error in getting the SSV data\n");
    }

    return status;
}

static fbe_status_t fbe_cli_lib_encryption_remove_keys(int argc, char** argv)
{
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_database_control_remove_encryption_keys_t encryption_info;
	fbe_object_id_t rg_id;

    argv++;
    argc--;

    if (argc == 0) {
        fbe_cli_printf("%s", ENCRYPTION_USAGE);
        return status;
    }
    else{
        rg_id = atoi(*argv);
        encryption_info.object_id = rg_id;
        status = fbe_api_database_remove_encryption_keys(&encryption_info);
        if(status != FBE_STATUS_OK) {
            fbe_cli_printf("Remove keys for RG %d failed\n", rg_id);
        }
	}
    return status;
}

static fbe_status_t fbe_cli_lib_encryption_display_status(int argc, char** argv)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_database_system_encryption_info_t sys_encryption_info;
    fbe_system_encryption_info_t    encryption_info;
    fbe_database_system_encryption_progress_t encryption_progress;
    fbe_database_system_scrub_progress_t scrub_progress;
    fbe_u32_t   percent_complete;

    /* Get encryption info */
    status = fbe_api_get_system_encryption_info(&encryption_info);

    if (status != FBE_STATUS_OK) {
        fbe_cli_error( "%s: Status:0x%x. Unable to get system encryption mode\n", __FUNCTION__, status);
        return status;
    }

    if(encryption_info.encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) {
        fbe_cli_printf("Encryption mode not enabled\n");
        return FBE_STATUS_OK;
    }

    status = fbe_api_database_get_system_encryption_info(&sys_encryption_info);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error( "%s: Status:0x%x. Unable to get system encryption info\n", __FUNCTION__, status);
        return status;
    }

    if(sys_encryption_info.system_encryption_state == FBE_SYSTEM_ENCRYPTION_STATE_UNSUPPORTED)
    {
        fbe_cli_printf("Status: Encryption Not Supported\n");
        return FBE_STATUS_OK;
    }

    if(sys_encryption_info.system_encryption_state == FBE_SYSTEM_ENCRYPTION_STATE_ENCRYPTED) {
        fbe_cli_printf("Status: Encryption Completed\n");
        return FBE_STATUS_OK;
    }
    else if(sys_encryption_info.system_encryption_state == FBE_SYSTEM_ENCRYPTION_STATE_IN_PROGRESS) {
        fbe_cli_printf("Status: Encryption In Progress\n");
        status = fbe_api_database_get_system_encryption_progress(&encryption_progress);
        if (status != FBE_STATUS_OK) {
            fbe_cli_error( "%s: Status:0x%x. Unable to get progress info\n", __FUNCTION__, status);
            return status;
        }
        fbe_cli_printf("Blocks Encrypted:%llu\n", (unsigned long long)encryption_progress.blocks_already_encrypted);
        fbe_cli_printf("Total Blocks:%llu\n", (unsigned long long)encryption_progress.total_capacity_in_blocks);
        percent_complete = (fbe_u32_t)((encryption_progress.blocks_already_encrypted * 100)/encryption_progress.total_capacity_in_blocks);
        fbe_cli_printf("Encryption Percent Complete:%d%%\n", percent_complete);
    }
    else if(sys_encryption_info.system_encryption_state == FBE_SYSTEM_ENCRYPTION_STATE_SCRUBBING) {
        fbe_cli_printf("Status: Scrubbing In Progress\n");
        status = fbe_api_database_get_system_encryption_progress(&encryption_progress);
        if (status != FBE_STATUS_OK) {
            fbe_cli_error( "%s: Status:0x%x. Unable to get progress info\n", __FUNCTION__, status);
            return status;
        }
        fbe_cli_printf("Blocks Encrypted:%llu\n", (unsigned long long)encryption_progress.blocks_already_encrypted);
        fbe_cli_printf("Total Blocks:%llu\n", (unsigned long long)encryption_progress.total_capacity_in_blocks);
        percent_complete = (fbe_u32_t)((encryption_progress.blocks_already_encrypted * 100)/encryption_progress.total_capacity_in_blocks);
        fbe_cli_printf("Encryption Percent Complete:%d%%\n", percent_complete);

        status = fbe_api_database_get_system_scrub_progress(&scrub_progress);
        if (status != FBE_STATUS_OK) {
            fbe_cli_error( "%s: Status:0x%x. Unable to get progress info\n", __FUNCTION__, status);
            return status;
        }
        percent_complete = (fbe_u32_t)((scrub_progress.blocks_already_scrubbed * 100)/scrub_progress.total_capacity_in_blocks);
        fbe_cli_printf("Scrubbing Percent Complete:%d%%\n", percent_complete);
    }
    else
    {
        fbe_cli_printf("Unknown Encryption state\n");
    }

    return status;
}

static fbe_status_t fbe_cli_lib_encryption_backup_info(int argc, char** argv)
{
    fbe_status_t               status = FBE_STATUS_OK;
	fbe_database_backup_info_t backup_info;

    /* Get encryption info */
    status = fbe_api_database_get_backup_info(&backup_info);

    if (status != FBE_STATUS_OK) {
        fbe_cli_error( "%s: Status:0x%x. Unable to get backup info\n", __FUNCTION__, status);
        return status;
    }

	switch(backup_info.encryption_backup_state){
		case FBE_ENCRYPTION_BACKUP_INVALID:
			fbe_cli_printf("Backup state INVALID \n");
			break;
		case FBE_ENCRYPTION_BACKUP_REQUIERED:
			fbe_cli_printf("Backup state REQUIERED \n");
			break;

		case FBE_ENCRYPTION_BACKUP_IN_PROGRESS:
			fbe_cli_printf("Backup state IN_PROGRESS \n");
			break;

		case FBE_ENCRYPTION_BACKUP_COMPLETED:
			fbe_cli_printf("Backup state COMPLETED \n");
			break;

		default:
			fbe_cli_printf("Uknown backup state %d \n", backup_info.encryption_backup_state);
	}

	switch(backup_info.primary_SP_ID){
		case SP_A:
			fbe_cli_printf("primary_SP_ID %X SP_A \n", backup_info.primary_SP_ID);
			break;

		case SP_B:
			fbe_cli_printf("primary_SP_ID %X SP_B \n", backup_info.primary_SP_ID);
			break;

		case SP_NA:
			fbe_cli_printf("primary_SP_ID %X SP_NA \n", backup_info.primary_SP_ID);
			break;

		default:
			fbe_cli_printf("Uknown primary_SP_ID %X \n", backup_info.primary_SP_ID);
	}

    return status;
}

static fbe_status_t fbe_cli_lib_encryption_check_blocks(int argc, char** argv)
{
    fbe_block_count_t num_blocks = 0;
    fbe_lba_t lba = FBE_LBA_INVALID;
    fbe_u32_t bus = FBE_BUS_ID_INVALID;
    fbe_u32_t encl = FBE_INVALID_ENCL_NUM;
    fbe_u32_t slot = FBE_INVALID_SLOT_NUM;
    fbe_block_count_t    num_encrypted;
    fbe_status_t status;
    fbe_system_encryption_info_t    encryption_info;

    argc--;
    argv++;

    while(argc) {
            if ((strcmp(*argv, "-num_blocks") == 0))
            {
                argc--;
                argv++;
                num_blocks = fbe_atoi(*argv);
            }
            else if ((strcmp(*argv, "-lba") == 0))
            {
                argc--;
                argv++;
                lba = fbe_atoh(*argv);
            }

            else if ((strcmp(*argv, "-b") == 0))
            {
                argc--;
                argv++;
                bus = fbe_atoi(*argv);
            }
            else if ((strcmp(*argv, "-e") == 0))
            {
                argc--;
                argv++;
                encl = fbe_atoi(*argv);
            }
            else if ((strcmp(*argv, "-s") == 0))
            {
                argc--;
                argv++;
                slot = fbe_atoi(*argv);
            }
            argc--;
            argv++;
    }
    if((num_blocks == 0) ||
       (lba == FBE_LBA_INVALID) ||
       (bus == FBE_BUS_ID_INVALID) ||
       (encl == FBE_INVALID_ENCL_NUM) ||
       (slot == FBE_INVALID_SLOT_NUM)) {
        fbe_cli_printf("%s", ENCRYPTION_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    /* Get encryption info */
    status = fbe_api_get_system_encryption_info(&encryption_info);

    if (status != FBE_STATUS_OK) {
        fbe_cli_error( "%s: Status:0x%x. Unable to get system encryption mode\n", __FUNCTION__, status);
        return status;
    }

    if(encryption_info.encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) {
        fbe_cli_printf("Encryption not enabled\n");
        return FBE_STATUS_OK;
    }

    status = fbe_api_encryption_check_block_encrypted(bus,
                                                      encl,
                                                      slot,
                                                      lba,
                                                      num_blocks,
                                                      &num_encrypted);
    if(status != FBE_STATUS_OK) {
        fbe_cli_printf("Get encrypted blocks failed\n");
    }
    else
    {
        if(num_encrypted == num_blocks) {
            fbe_cli_printf("All %d blocks encrypted\n", (fbe_u32_t)num_blocks);
        }
        else{
            fbe_cli_printf("Blocks to check: %d\n", (fbe_u32_t)num_blocks);
            fbe_cli_printf("Actual Number of blocks encrypted: %d\n", (fbe_u32_t)num_encrypted);
        }
    }
    return status;
}

static fbe_status_t fbe_cli_lib_encryption_port_mode(int argc, char** argv)
{
    fbe_port_encryption_mode_t mode = FBE_PORT_ENCRYPTION_MODE_INVALID;
    fbe_u32_t bus = FBE_BUS_ID_INVALID;
    fbe_status_t status;
    fbe_database_control_get_be_port_info_t be_port_info; 
    fbe_u32_t port_count;
    fbe_bool_t set_mode = FBE_FALSE;
    fbe_bool_t display_info = FBE_FALSE;
    fbe_u32_t sp_count;

    argc--;
    argv++;

    while(argc) {
            if ((strcmp(*argv, "-set_mode") == 0))
            {
                argc--;
                argv++;
                set_mode = FBE_TRUE;
                mode = fbe_atoi(*argv);
            }
            else if ((strcmp(*argv, "-b") == 0))
            {
                argc--;
                argv++;
                bus = fbe_atoi(*argv);
            }
            else if((strcmp(*argv, "-info") == 0)) {
                display_info = FBE_TRUE;
            }
            argc--;
            argv++;
    }

    if(!set_mode && !display_info) {
        fbe_cli_printf("%s", ENCRYPTION_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    be_port_info.sp_id = FBE_CMI_SP_ID_A;

    for (sp_count = 0; sp_count < 2; sp_count++) {
        status = fbe_api_database_get_be_port_info(&be_port_info);
        if (status != FBE_STATUS_OK){
            fbe_cli_printf("Unable to get port info on SP: %d\n", be_port_info.sp_id);
            be_port_info.sp_id = FBE_CMI_SP_ID_B;
            continue;
        }

        fbe_cli_printf("Total Number of ports on SP:%d is %d:\n", be_port_info.sp_id,
                       be_port_info.num_be_ports);

        /* we first need to put ALL the ports in a particular mode */
        for (port_count = 0; port_count< be_port_info.num_be_ports; port_count++){
            if ((bus != FBE_BUS_ID_INVALID) && 
                (bus != be_port_info.port_info[port_count].be_number)){
                /* This is not the bus we want */
                continue;
            }
            if (display_info){
                fbe_cli_printf("\t%d.Object ID:0x%x BE Port: 0x%x SN:%s Mode:%s(%d)\n", 
                               port_count+1,
                               be_port_info.port_info[port_count].port_object_id,
                               be_port_info.port_info[port_count].be_number, 
                               be_port_info.port_info[port_count].serial_number,
                               fbe_cli_lib_convert_port_encryption_mode(be_port_info.port_info[port_count].port_encrypt_mode),
                               be_port_info.port_info[port_count].port_encrypt_mode);
            }
            else {
                fbe_cli_printf("\tSet mode to %s(%d) for BE Port %d...", 
                               fbe_cli_lib_convert_port_encryption_mode(mode), mode, be_port_info.port_info[port_count].be_number);
                status = fbe_api_port_set_port_encryption_mode(be_port_info.port_info[port_count].port_object_id,
                                                               mode);
                if (status == FBE_STATUS_OK){
                    fbe_cli_printf("Done\n");
                }
                else {
                    fbe_cli_printf("Failed\n");
                }
            }
        }
        be_port_info.sp_id = FBE_CMI_SP_ID_B;
    }
    return status;
}

static fbe_status_t fbe_cli_lib_encryption_pause(int argc, char** argv)
{
    fbe_status_t   status = FBE_STATUS_OK;

    fbe_cli_printf("Pausing background encryption...\n");

    status = fbe_api_job_service_set_encryption_paused(FBE_TRUE);
 
    return status;
}

static fbe_status_t fbe_cli_lib_encryption_resume(int argc, char** argv)
{
    fbe_status_t   status = FBE_STATUS_OK;

    fbe_cli_printf("Resuming background encryption...\n");

    status = fbe_api_job_service_set_encryption_paused(FBE_FALSE);
 
    return status;
}

static fbe_u8_t * fbe_cli_lib_convert_port_encryption_mode(fbe_port_encryption_mode_t mode)
{
    switch(mode) {
        case FBE_PORT_ENCRYPTION_MODE_DISABLED:
            return "Encryption Disabled";
            break;
        case FBE_PORT_ENCRYPTION_MODE_NOT_SUPPORTED:
            return "Encryption Not Supported";
            break;
        case FBE_PORT_ENCRYPTION_MODE_PLAINTEXT_KEYS:
            return "PlainText Keys";
            break;
        case FBE_PORT_ENCRYPTION_MODE_WRAPPED_DEKS:
            return "Wrapped DEKs";
            break;
        case FBE_PORT_ENCRYPTION_MODE_WRAPPED_KEKS_DEKS:
            return "Wrapped DEKs KEKs";
            break;
        case FBE_PORT_ENCRYPTION_MODE_UNCOMMITTED:
            return "Port Uncommitted";
            break;
        case FBE_PORT_ENCRYPTION_MODE_INVALID:
        default:
            return "Invalid";
            break;
    }
}
