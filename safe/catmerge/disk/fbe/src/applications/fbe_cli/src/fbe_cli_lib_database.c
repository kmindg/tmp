/***************************************************************************
* Copyright (C) EMC Corporation 2011 
* All rights reserved. 
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!*************************************************************************
*                 @file fbe_cli_lib_database.c
****************************************************************************
*
* @brief
*  This file contains cli functions for the database related features in
*  FBE CLI.
*
* @ingroup fbe_cli
*
* @date
*  07/12/2011 - Created. Vera Wang
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cli_private.h"
#include "fbe_cli_database.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_system_limits.h"
#include "fbe_private_space_layout_generated.h"
#include "fbe_api_physical_drive_interface.h"
#include "fbe_api_discovery_interface.h"
#include "fbe/fbe_api_cmi_interface.h"

#define INVALID_WWN_SEED        0xFFFFFFFF


/**************************
*   LOCAL FUNCTIONS
**************************/
static void fbe_cli_print_database_get_state(void);
static void fbe_cli_print_database_get_tables(fbe_object_id_t object_id, fbe_u32_t verbose);
static fbe_char_t * fbe_cli_database_get_database_state_string(fbe_database_state_t state);
static fbe_char_t * fbe_convert_db_class_id_to_string(database_class_id_t db_class_id);
static fbe_char_t * fbe_convert_config_type_to_string(fbe_provision_drive_config_type_t config_type);
static fbe_char_t * fbe_convert_database_config_entry_state_to_string(database_config_entry_state_t state);
fbe_char_t * fbe_convert_configured_physical_block_size_to_string
	(fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size);
static fbe_char_t * fbe_cli_database_vd_configuration_mode_to_string
	(fbe_virtual_drive_configuration_mode_t configuration_mode);
static fbe_char_t * fbe_convert_update_pvd_type_to_string(fbe_update_pvd_type_t update_type);
static fbe_char_t * fbe_convert_raid_group_type_to_string(fbe_raid_group_type_t raid_type);
static fbe_char_t * fbe_convert_update_raid_type_to_string(fbe_update_raid_type_t update_type);
static fbe_char_t * fbe_convert_raid_group_debug_flags_to_string(fbe_raid_group_debug_flags_t debug_flags);
static void fbe_cli_print_database_user_entry_lu(database_user_entry_t *user_entry);


static void fbe_cli_print_database_transaction_info(fbe_u32_t verbose);
static void fbe_cli_print_user_entry_lu(database_user_entry_t *user_entry);
static void fbe_cli_print_user_entry_rg(database_user_entry_t *user_entry);
static void fbe_cli_print_user_entry_pvd(database_user_entry_t *user_entry);
static void fbe_cli_print_object_entry_lu(database_object_entry_t *obj_entry);
static void fbe_cli_print_object_entry_rg(database_object_entry_t *obj_entry);
static void fbe_cli_print_object_entry_vd(database_object_entry_t *obj_entry);
static void fbe_cli_print_object_entry_pvd(database_object_entry_t *obj_entry);
static fbe_char_t * fbe_cli_get_database_transaction_state_string(database_transaction_state_t state);
static void fbe_cli_print_global_info_entry(database_global_info_entry_t *global_entry);

static void fbe_cli_print_database_chassis_replacement_flag(void);

static void fbe_cli_set_database_chassis_replacement_flag(fbe_u32_t  set);

static void fbe_cli_print_database_fru_descriptor(fbe_u32_t  slot_no);

static void fbe_cli_clear_database_fru_descriptor(void);
static void fbe_cli_set_database_fru_descriptor(void);


static void fbe_cli_get_prom_wwnseed(void);
static void fbe_cli_set_prom_wwnseed(fbe_u32_t wwnseed);

static void fbe_cli_print_system_db_header_info(fbe_u32_t verbose);

static void fbe_cli_lib_database_add_hook(char * argv);
static void fbe_cli_lib_database_remove_hook(char * argv);

static void fbe_cli_print_database_stats(void);

static void fbe_cli_print_database_cap_limit(void);
static void fbe_cli_set_database_cap_limit(fbe_u32_t cap_limit);

static void fbe_cli_print_database_supported_drive_types(void);
static void fbe_cli_set_database_supported_drive_type(fbe_database_control_set_supported_drive_type_t *set_drive_type_p);

/*!**********************************************************************
*                      fbe_cli_system_db_header()                 
*************************************************************************
*
*  @brief
*            fbe_cli_system_db_header - Print, recover and set system db header
	
*  @param    argc - argument count
*  @param    argv - argument string
*
*  @return
*            None
*  @date                
*            May 7, 2013 - Created. Yingying Song
************************************************************************/
void fbe_cli_system_db_header(fbe_s32_t argc, fbe_s8_t ** argv)
{
    database_system_db_header_t system_db_header = {0};
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_state_t db_state;

    if (argc < 1)
    {
        fbe_cli_printf("Not enough arguments to print/recover/set system db header\n");
        fbe_cli_printf("%s", FBE_CLI_SYSTEM_DB_HEADER_USAGE);
        return;
    }

    if ((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0))
    {
        /* If they are asking for help, just display help and exit */
        fbe_cli_printf("%s", FBE_CLI_SYSTEM_DB_HEADER_USAGE);
        return;
    }
    else if(strncmp(*argv, "print", 5) == 0)
    {
        fbe_cli_print_system_db_header_info(1);
        return;
    }
    else if(strncmp(*argv, "recover", 7) == 0)
    {
        argc--;
        argv++;
        if(argc > 0)
        {
            if(strncmp(*argv, "-magic_num", 10) != 0)
            {
                fbe_cli_error("The argument <%s> is invalid\n", *argv);
                fbe_cli_printf("%s", FBE_CLI_SYSTEM_DB_HEADER_USAGE);
                return;
            } 
            argc--;
            argv++;
        }

        status = fbe_api_database_init_system_db_header(&system_db_header);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error ("%s:Recover failed. Reason: Failed to init system db header\n", 
                           __FUNCTION__);
            return;
        }

        status = fbe_api_database_persist_system_db_header(&system_db_header);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error ("%s:Recover failed. Reason: Failed to persist system db header\n", 
                           __FUNCTION__);
            return;
        }

        fbe_cli_printf("Recover system db header successfully.\n");
        return;
    }       
    else if(strncmp(*argv, "set", 3) == 0)
    {
        argc--;
        argv++;
        if(argc < 1)
        {
            fbe_cli_error("Not enough arguments to set system db header\n");
            fbe_cli_printf("%s", FBE_CLI_SYSTEM_DB_HEADER_USAGE);
            return;
        }

        status = fbe_api_database_get_state(&db_state);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error ("%s:Set failed. Reason: Failed to get database state\n", 
                           __FUNCTION__);
            return;
        }

        if (db_state == FBE_DATABASE_STATE_READY) 
        {
            status = fbe_api_database_get_system_db_header(&system_db_header);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error ("%s:Set failed. Reason: Failed to get system db header\n", 
                               __FUNCTION__);
                return;
            }
        }
        else
        { 
            fbe_api_database_init_system_db_header(&system_db_header);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error ("%s:Set failed. Reason: Failed to init system db header\n", 
                               __FUNCTION__);
                return;
            }
        }

        while(argc > 0)
        {
            if (strncmp(*argv, "-magic_num", 10) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("magic_num argument is expected\n");
                    return;
                }
                system_db_header.magic_number = _strtoui64(*argv, NULL, 10);
                argc--;
                argv++;
            }
            else if (strncmp(*argv, "-version_header_size", 20) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("version_header_size argument is expected\n");
                    return;
                }
                system_db_header.version_header.size = atoi(*argv);
                argc--;
                argv++;
            }            
            else if (strncmp(*argv, "-persisted_sep_version", 22) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("persisted_sep_version argument is expected\n");
                    return;
                }
                system_db_header.persisted_sep_version = _strtoui64(*argv, NULL, 10);
                argc--;
                argv++;
            }
            else if(strncmp(*argv, "-bvd_interface_object_entry_size", 32) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("bvd_interface_object_entry_size argument is expected\n");
                    return;
                }
                system_db_header.bvd_interface_object_entry_size = atoi(*argv);
                argc--;
                argv++;
            }
            else if(strncmp(*argv, "-pvd_object_entry_size", 22) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("pvd_object_entry_size argument is expected\n");
                    return;
                }
                system_db_header.pvd_object_entry_size = atoi(*argv);
                argc--;
                argv++;
            }
            else if(strncmp(*argv, "-vd_object_entry_size", 21) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("vd_object_entry_size argument is expected\n");
                    return;
                }
                system_db_header.vd_object_entry_size = atoi(*argv);
                argc--;
                argv++;
            }
            else if(strncmp(*argv, "-rg_object_entry_size", 21) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("rg_object_entry_size argument is expected\n");
                    return;
                }
                system_db_header.rg_object_entry_size = atoi(*argv);
                argc--;
                argv++;
            }
            else if(strncmp(*argv, "-lun_object_entry_size", 22) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("lun_object_entry_size argument is expected\n");
                    return;
                }
                system_db_header.lun_object_entry_size = atoi(*argv);
                argc--;
                argv++;
            }
            else if(strncmp(*argv, "-edge_entry_size", 16) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("edge_entry_size argument is expected\n");
                    return;
                }
                system_db_header.edge_entry_size = atoi(*argv);
                argc--;
                argv++;
            }
            else if(strncmp(*argv, "-pvd_user_entry_size", 20) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("pvd_user_entry_size argument is expected\n");
                    return;
                }
                system_db_header.pvd_user_entry_size = atoi(*argv);
                argc--;
                argv++;
            }
            else if(strncmp(*argv, "-rg_user_entry_size", 19) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("rg_user_entry_size argument is expected\n");
                    return;
                }
                system_db_header.rg_user_entry_size = atoi(*argv);
                argc--;
                argv++;
            }
            else if(strncmp(*argv, "-lun_user_entry_size", 20) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("lun_user_entry_size argument is expected\n");
                    return;
                }
                system_db_header.lun_user_entry_size = atoi(*argv);
                argc--;
                argv++;
            }
            else if(strncmp(*argv, "-power_save_info_global_info_entry_size", 39) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("power_save_info_global_info_entry_size argument is expected\n");
                    return;
                }
                system_db_header.power_save_info_global_info_entry_size = atoi(*argv);
                argc--;
                argv++;
            }
            else if(strncmp(*argv, "-spare_info_global_info_entry_size", 34) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("spare_info_global_info_entry_size argument is expected\n");
                    return;
                }
                system_db_header.spare_info_global_info_entry_size = atoi(*argv);
                argc--;
                argv++;
            }
            else if(strncmp(*argv, "-generation_info_global_info_entry_size", 39) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("generation_info_global_info_entry_size argument is expected\n");
                    return;
                }
                system_db_header.generation_info_global_info_entry_size = atoi(*argv);
                argc--;
                argv++;
            }
            else if(strncmp(*argv, "-time_threshold_info_global_info_entry_size", 43) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("time_threshold_info_global_info_entry_size is expected\n");
                    return;
                }
                system_db_header.time_threshold_info_global_info_entry_size = atoi(*argv);
                argc--;
                argv++;
            }
            else if(strncmp(*argv, "-pvd_nonpaged_metadata_size", 27) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("pvd_nonpaged_metadata_size is expected\n");
                    return;
                }
                system_db_header.pvd_nonpaged_metadata_size = atoi(*argv);
                argc--;
                argv++;
            }
            else if(strncmp(*argv, "-lun_nonpaged_metadata_size", 27) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("lun_nonpaged_metadata_size is expected\n");
                    return;
                }
                system_db_header.lun_nonpaged_metadata_size = atoi(*argv);
                argc--;
                argv++;
            }
            else if(strncmp(*argv, "-raid_nonpaged_metadata_size",28) == 0)
            {
                argc--;
                argv++;
                /* argc should be greater than 0 here */
                if (argc <= 0)
                {
                    fbe_cli_error("raid_nonpaged_metadata_size is expected\n");
                    return;
                }
                system_db_header.raid_nonpaged_metadata_size = atoi(*argv);
                argc--;
                argv++;
            }
            else
            {
                fbe_cli_error("The argument <%s> is invalid.\n",*argv);
                fbe_cli_printf("%s", FBE_CLI_SYSTEM_DB_HEADER_USAGE);
                return;
            }
        }

        status = fbe_api_database_set_system_db_header(&system_db_header);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error ("%s:Set failed. Reason: Failed to set system db header in memory\n", 
                           __FUNCTION__);
            return;
        }

        status = fbe_api_database_persist_system_db_header(&system_db_header);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error ("%s:Set failed. Reason: Failed to persist system db header\n", 
                           __FUNCTION__);
            return;
        }

        fbe_cli_printf("Set system db header successfully.\n");
        return;
    }
    else
    {
        fbe_cli_error("The argument <%s> is invalid.\n",*argv);
        fbe_cli_printf("%s", FBE_CLI_SYSTEM_DB_HEADER_USAGE);
        return;
    }   
}



/*!**********************************************************************
*                      fbe_cli_database_get_state ()                 
*************************************************************************
*
*  @brief
*    fbe_cli_database - Get database information in database service
	
*  @param    argc - argument count
*  @param    argv - argument string
*
*  @return
*            None
************************************************************************/
void fbe_cli_database(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_object_id_t   object_id_for_get_tables;
    fbe_u32_t max_objects;
    fbe_u32_t with_verbose;


    fbe_system_limits_get_max_objects (FBE_PACKAGE_ID_SEP_0, &max_objects);

    if (argc < 1)
    {
        /*If there are no arguments show usage*/
        fbe_cli_printf("%s", FBE_CLI_DATABASE_USAGE);
        return;
    }
    
    if ((strncmp(*argv, "-getState", 10) == 0))
    {
        fbe_cli_print_database_get_state();
        return;
    }
    else if ((strncmp(*argv, "-getTables", 11) == 0))
    {
        /* Next argument is object id value(interpreted in HexaDECIMAL).
        */
        argv++;
        with_verbose = 0;

        if (*argv != NULL)
        {
            object_id_for_get_tables = fbe_atoh(*argv);
            if (object_id_for_get_tables > max_objects)
            {
                fbe_cli_printf("\n");
                fbe_cli_error("Invalid Object Id.\n\n");
                fbe_cli_printf("%s", FBE_CLI_DATABASE_USAGE);
                return;
            }

            /*check the -verbose option*/
            if (argc == 3)
            {
                argv ++;
                if (*argv != NULL && strncmp(*argv, "-verbose", 9) == 0)
                    with_verbose = 1;
                else
                {
                    fbe_cli_printf("%s", FBE_CLI_DATABASE_USAGE);
                    return;
                }

            }
            fbe_cli_print_database_get_tables(object_id_for_get_tables, with_verbose);
        }
        else
        {  
            fbe_cli_error("\nYou need to add an object_id to the command\n\n");
        }

        return;
        
    }
    else if ((strncmp(*argv, "-getTransaction", 16) == 0))
    {
        with_verbose = 0;
        if (argc >= 2)
        {
            argv ++;
            if (*argv != NULL){
                if ((strncmp(*argv, "-verbose", 9) == 0))
                {
                    with_verbose = 1;
                }
                else
                {
                    fbe_cli_error("Wrong command: \n %s", FBE_CLI_DATABASE_USAGE);
                    return;
                }
            }
        }

        fbe_cli_print_database_transaction_info(with_verbose);
    }
    else if ((strncmp(*argv, "-get_chassis_replacement_flag", 30) == 0)) {
        fbe_cli_print_database_chassis_replacement_flag();
        return;
    }
    else if ((strncmp(*argv, "-set_chassis_replacement_flag", 30) == 0)) {
        if (argc >= 2) //Next argument must be set value
        {
            argv++;
            fbe_cli_set_database_chassis_replacement_flag(fbe_atoi(*argv));
        }
        else
            fbe_cli_error("You must specify whether set or clear the flag\n");
    }
    else if ((strncmp(*argv, "-get_fru_descriptor", 20) == 0)) {
        if (argc >= 2) //Next argument maybe fru slot no
        {
            argv++;
            fbe_cli_print_database_fru_descriptor(fbe_atoi(*argv));
        }
        else
            fbe_cli_print_database_fru_descriptor(10);
    }
    else if ((strncmp(*argv, "-clear_fru_descriptor", 22) == 0)) {
        fbe_cli_clear_database_fru_descriptor();
    }
    else if ((strncmp(*argv, "-set_fru_descriptor", 20) == 0)) {
        fbe_cli_set_database_fru_descriptor();
    }
    else if ((strncmp(*argv, "-get_prom_wwnseed", 18) == 0)) {
            fbe_cli_get_prom_wwnseed();
            return;
    }
    else if ((strncmp(*argv, "-set_prom_wwnseed", 18) == 0)) {
        if (argc == 2) /*Next argument is wwn seed*/
        {
            argv++;
            fbe_cli_set_prom_wwnseed(fbe_atoh(*argv));
        }
        else
            fbe_cli_error("Wrong command: \n %s", FBE_CLI_DATABASE_USAGE);
    }
    else if ((strncmp(*argv, "-get_system_db_header", 22) == 0)) {
        with_verbose = 0;
        if (argc >= 2)
        {
            argv ++;
            if (*argv != NULL){
                if ((strncmp(*argv, "-verbose", 9) == 0))
                {
                    with_verbose = 1;
                }
                else
                {
                    fbe_cli_error("Wrong command: \n %s", FBE_CLI_DATABASE_USAGE);
                    return;
                }
            }
        }
        fbe_cli_print_system_db_header_info(with_verbose);
    }
    else if ((strncmp(*argv, "-getStats", 10) == 0)) {
       fbe_cli_print_database_stats();
       return;
    }
    else if ((strncmp(*argv, "-get_cap_limit", 14) == 0)) {
        fbe_cli_print_database_cap_limit();
        return;
    }
    else if ((strncmp(*argv, "-set_cap_limit", 14) == 0)) {
        if (argc < 2) {
            fbe_cli_error("Wrong command: \n %s", FBE_CLI_DATABASE_USAGE);
            return;
        }
       fbe_cli_set_database_cap_limit(atoi(argv[1]));
       return;
    }
    else if ((strncmp(*argv, "-get_additional_supported_drive_types", 38) == 0)) {
        fbe_cli_print_database_supported_drive_types();
    }
    else if ((strncmp(*argv, "-set_additional_supported_drive_type", 37) == 0)) {
        fbe_database_control_set_supported_drive_type_t set_drive_type = {0};

        if (argc < 3) {
            fbe_cli_error("Wrong command: \n %s", FBE_CLI_DATABASE_USAGE);
            return;
        }

        if ((strncmp(argv[1], "-enable", 8) == 0)) {
            set_drive_type.do_enable = FBE_TRUE;
        }
        else if ((strncmp(argv[1], "-disable", 9) == 0)){
            set_drive_type.do_enable = FBE_FALSE;
        }
        else {
            fbe_cli_error("Wrong command: Invalid enable arg\n %s", FBE_CLI_DATABASE_USAGE);
            return;
        }

        if ((strncmp(argv[2], "le", 3) == 0)) {
            set_drive_type.type = FBE_DATABASE_DRIVE_TYPE_SUPPORTED_LE;
        }
        else if ((strncmp(argv[2], "ri", 3) == 0)) {
            set_drive_type.type = FBE_DATABASE_DRIVE_TYPE_SUPPORTED_RI;
        }
        else if (strncmp(argv[2], "520hdd", 7) == 0) {
            set_drive_type.type = FBE_DATABASE_DRIVE_TYPE_SUPPORTED_520_HDD;
        }
        else {
            fbe_cli_error("Wrong command: Invalid drive type\n %s", FBE_CLI_DATABASE_USAGE);
            return;
        }        

        fbe_cli_set_database_supported_drive_type(&set_drive_type);
    }
    else
    {
        /*If there are no arguments show usage*/
        fbe_cli_error("%s", FBE_CLI_DATABASE_USAGE);
        return;
    }
}

/*!**************************************************************
 * fbe_cli_print_database_get_state()
 ****************************************************************
 * @brief
 *  This function prints out the database state.
 *
 * @param 
 *        None
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_print_database_get_state(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_state_t state;
    status = fbe_api_database_get_state(&state);

    if ((status == FBE_STATUS_OK))
    {
        fbe_cli_printf("\nDatabase state is : %s\n",fbe_cli_database_get_database_state_string(state));    
    }
    else
    {
        fbe_cli_printf("\nFailed to get database state ... Error: %d\n", status);            
    }

    if (state == FBE_DATABASE_STATE_SERVICE_MODE) {
        fbe_database_control_get_service_mode_reason_t ctrl_reason;

        status = fbe_api_database_get_service_mode_reason(&ctrl_reason);
        if (status == FBE_STATUS_OK)
            fbe_cli_printf("Database degraded reason is : %s\n", ctrl_reason.reason_str);
        else
            fbe_cli_printf("Failed to get database degraded reason ... Error: %d\n", status);
    }
    return;
}


/*!**************************************************************
 * fbe_cli_print_database_get_tables()
 ****************************************************************
 * @brief
 *  This function prints out the database tables given an object id.
 *
 * @param 
 *        None
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_print_database_get_tables(fbe_object_id_t object_id, fbe_u32_t verbose)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t edge_i = 0;
    fbe_u32_t found_entry = 0;
    fbe_database_get_tables_t get_tables;


    status = fbe_api_database_get_tables (object_id,&get_tables);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed to get database tables ... Error: %d\n", status);
    }
    
    /* Print out the object entry */ 
    fbe_cli_printf("\nDatabase object table\n");
    if (get_tables.object_entry.header.state != DATABASE_CONFIG_ENTRY_INVALID || verbose != 0)
    {
        fbe_cli_printf("\tdb_config_entry_state is : %s\n",
                        fbe_convert_database_config_entry_state_to_string(get_tables.object_entry.header.state));
        fbe_cli_printf("\tdb_config_entry_versioned_size is: 0x%x\n",
                        get_tables.object_entry.header.version_header.size);
        switch (get_tables.object_entry.db_class_id)
        {
            case    DATABASE_CLASS_ID_PROVISION_DRIVE:
                fbe_cli_printf("\tdb_class_id : %s\n", "DATABASE_CLASS_ID_PROVISION_DRIVE");
                fbe_cli_print_object_entry_pvd(&get_tables.object_entry);
                break;
            case    DATABASE_CLASS_ID_VIRTUAL_DRIVE:
                fbe_cli_printf("\tdb_class_id : %s\n", "DATABASE_CLASS_ID_VIRTUAL_DRIVE");
                fbe_cli_print_object_entry_vd(&get_tables.object_entry);
                break;
            case    DATABASE_CLASS_ID_MIRROR:
            case    DATABASE_CLASS_ID_STRIPER:
            case    DATABASE_CLASS_ID_PARITY:
                fbe_cli_printf("\tdb_class_id : %s\n",
                                fbe_convert_db_class_id_to_string(get_tables.object_entry.db_class_id));
                fbe_cli_print_object_entry_rg(&get_tables.object_entry);
                break;
            case    DATABASE_CLASS_ID_LUN:
                fbe_cli_printf("\tdb_class_id : %s\n", "DATABASE_CLASS_ID_LUN");
                fbe_cli_print_object_entry_lu(&get_tables.object_entry);
                break;
            case    DATABASE_CLASS_ID_BVD_INTERFACE:
                fbe_cli_printf("\tdb_class_id : %s\n", "DATABASE_CLASS_ID_BVD_INTERFACE");
                break;
            default:
                fbe_cli_printf("\tdb_class_id : %s\n", "UNKNOWN CLASS ID");
                break;
        }
		
		fbe_cli_printf("\tencryption_mode : %x\n", get_tables.object_entry.base_config.encryption_mode);		
    }

    /* Print out the user entry */ 
    fbe_cli_printf("\nDatabase user table\n");
    if (get_tables.user_entry.header.state != DATABASE_CONFIG_ENTRY_INVALID || verbose != 0)
    {
        fbe_cli_printf("\tdb_config_entry_state is : %s\n", 
                        fbe_convert_database_config_entry_state_to_string(get_tables.user_entry.header.state));
        fbe_cli_printf("\tdb_config_entry_versioned_size is: 0x%x\n",
                        get_tables.user_entry.header.version_header.size);
        switch (get_tables.user_entry.db_class_id)
        {
            case    DATABASE_CLASS_ID_PROVISION_DRIVE:
                fbe_cli_printf("\tdb_class_id : %s\n", "DATABASE_CLASS_ID_PROVISION_DRIVE");
                fbe_cli_print_user_entry_pvd(&get_tables.user_entry);
                break;
            case    DATABASE_CLASS_ID_MIRROR:
            case    DATABASE_CLASS_ID_STRIPER:
            case    DATABASE_CLASS_ID_PARITY:
                fbe_cli_printf("\tdb_class_id : %s\n",
                                fbe_convert_db_class_id_to_string(get_tables.user_entry.db_class_id));
                fbe_cli_print_user_entry_rg(&get_tables.user_entry);
                break;
            case    DATABASE_CLASS_ID_LUN:
                fbe_cli_printf("\tdb_class_id : %s\n", "DATABASE_CLASS_ID_LUN");
                fbe_cli_print_database_user_entry_lu(&get_tables.user_entry);
                break;
            case    DATABASE_CLASS_ID_BVD_INTERFACE:
                fbe_cli_printf("\tdb_class_id : %s\n", "DATABASE_CLASS_ID_BVD_INTERFACE");
                break;
            default:
                fbe_cli_printf("\tdb_class_id : %s\n", "UNKNOWN CLASS ID");
                break;
        }
    }

    /* Print out the edge entry */
    fbe_cli_printf("\n\nDatabase edge table\n\n");
    found_entry = 0;
    for (edge_i = 0; edge_i < DATABASE_MAX_EDGE_PER_OBJECT; edge_i++)
    {
        if (get_tables.edge_entry[edge_i].header.state == DATABASE_CONFIG_ENTRY_INVALID && verbose == 0)
            continue;

        found_entry ++;
        fbe_cli_printf("\tedge_index : %d\n", edge_i);
        fbe_cli_printf("\tdb_config_entry_state is : %s\n\n",
                    fbe_convert_database_config_entry_state_to_string(get_tables.edge_entry[edge_i].header.state));
        fbe_cli_printf("\tdb_config_entry_versioned_size is : 0x%x\n",
                        get_tables.edge_entry[edge_i].header.version_header.size);
        if (get_tables.edge_entry[edge_i].server_id != 0)
        {
            fbe_cli_printf("\t\tserver_id : 0x%x\n", get_tables.edge_entry[edge_i].server_id);
            fbe_cli_printf("\t\tclient_index : %d\n", get_tables.edge_entry[edge_i].client_index);
            fbe_cli_printf("\t\tcapacity : 0x%llX\n",
			   (unsigned long long)get_tables.edge_entry[edge_i].capacity);
            fbe_cli_printf("\t\toffset : 0x%llX\n\n",
			   (unsigned long long)get_tables.edge_entry[edge_i].offset);
        }
        else
            fbe_cli_printf("\t\tThe edge has server id 0x0\n");

        fbe_cli_printf("\n");
    }

    if (found_entry == 0)
        fbe_cli_printf("\n     No valid edge entry found\n");
    

    return;
}

/*!**************************************************************
 * fbe_cli_database_get_database_state_string()
 ****************************************************************
 * @brief
 *  This function converts database state to a text string.
 *
 * @param 
 *        fbe_database_state_t state
 * 
 * @return
 *        fbe_char_t *  A string for db state
 ***************************************************************/
static fbe_char_t * fbe_cli_database_get_database_state_string(fbe_database_state_t state)
{
    switch(state)
    {
        case FBE_DATABASE_STATE_INVALID:
            return "FBE_DATABASE_STATE_INVALID";
        case FBE_DATABASE_STATE_INITIALIZING:
            return "FBE_DATABASE_STATE_INITIALIZING";
        case FBE_DATABASE_STATE_INITIALIZED:
            return "FBE_DATABASE_STATE_INITIALIZED";
        case FBE_DATABASE_STATE_READY:
            return "FBE_DATABASE_STATE_READY";
        case FBE_DATABASE_STATE_FAILED:
            return "FBE_DATABASE_STATE_FAILED";
        case FBE_DATABASE_STATE_UPDATING_PEER:
            return "FBE_DATABASE_STATE_UPDATING_PEER";
        case FBE_DATABASE_STATE_WAITING_FOR_CONFIG:
            return "FBE_DATABASE_STATE_WAITING_FOR_CONFIG";
        case FBE_DATABASE_STATE_DEGRADED:
            return "FBE_DATABASE_STATE_DEGRADED";
        case FBE_DATABASE_STATE_DESTROYING:
            return "FBE_DATABASE_STATE_DESTROYING";
        case FBE_DATABASE_STATE_DESTROYING_SYSTEM:
            return "FBE_DATABASE_STATE_DESTROYING_SYSTEM";
        case FBE_DATABASE_STATE_WAITING_ON_KMS:
            return "FBE_DATABASE_STATE_WAITING_ON_KMS";
        case FBE_DATABASE_STATE_KMS_APPROVED:
            return "FBE_DATABASE_STATE_KMS_APPROVED";
        case FBE_DATABASE_STATE_SERVICE_MODE:
            /* In database service, we call it service mode. 
                      * But outside of database, people only can find degraded mode instead of service mode */
            return "FBE_DATABASE_STATE_DEGRADED_MODE";
        case FBE_DATABASE_STATE_CORRUPT:
        return "FBE_DATABASE_STATE_CORRUPT";

        default:
            return "FBE_DATABASE_STATE_LAST";
    }
}

/*!**************************************************************
 * fbe_convert_db_class_id_to_string()
 ****************************************************************
 * @brief
 *  This function converts database class id to a text string.
 *
 * @param 
 *        database_class_id_t db_class_id
 * 
 * @return
 *        fbe_char_t * A string for db class id
 ***************************************************************/
static fbe_char_t * fbe_convert_db_class_id_to_string(database_class_id_t db_class_id)
{
    switch(db_class_id)
    {
        case DATABASE_CLASS_ID_INVALID:
            return "DATABASE_CLASS_ID_INVALID";
        case DATABASE_CLASS_ID_BVD_INTERFACE:
            return "DATABASE_CLASS_ID_BVD_INTERFACE";
        case DATABASE_CLASS_ID_LUN:
            return "DATABASE_CLASS_ID_LUN";
		case DATABASE_CLASS_ID_RAID_START:
            return "DATABASE_CLASS_ID_RAID_START";
        case DATABASE_CLASS_ID_MIRROR:
            return "DATABASE_CLASS_ID_MIRROR";
        case DATABASE_CLASS_ID_STRIPER:
            return "DATABASE_CLASS_ID_STRIPER";
		case DATABASE_CLASS_ID_PARITY:
            return "DATABASE_CLASS_ID_PARITY";
        case DATABASE_CLASS_ID_RAID_END:
            return "DATABASE_CLASS_ID_RAID_END";
        case DATABASE_CLASS_ID_VIRTUAL_DRIVE:
            return "DATABASE_CLASS_ID_VIRTUAL_DRIVE";
		case DATABASE_CLASS_ID_PROVISION_DRIVE:
            return "DATABASE_CLASS_ID_PROVISION_DRIVE";
		default :
			return "DATABASE_CLASS_ID_LAST";
    }
}

/*!**************************************************************
 * fbe_convert_config_type_to_string()
 ****************************************************************
 * @brief
 *  This function converts config type to a text string.
 *
 * @param 
 *        fbe_provision_drive_config_type_t config_type
 * 
 * @return
 *        fbe_char_t * A string for config type
 ***************************************************************/
static fbe_char_t * fbe_convert_config_type_to_string(fbe_provision_drive_config_type_t config_type)
{
    switch(config_type)
    {
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID:
            return "FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID";
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED:
            return "FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED";
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE:
            return "FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE";
		default :
			return "FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID";
    }
}

/*!**************************************************************
 * fbe_convert_database_config_entry_state_to_string()
 ****************************************************************
 * @brief
 *  This function converts database state to a text string.
 *
 * @param 
 *        database_config_entry_state_t state
 * 
 * @return
 *        fbe_char_t *  A string for db config entry state
 ***************************************************************/
static fbe_char_t * fbe_convert_database_config_entry_state_to_string(database_config_entry_state_t state)
{
    switch(state)
    {
        case DATABASE_CONFIG_ENTRY_INVALID:
            return "DATABASE_CONFIG_ENTRY_INVALID";
        case DATABASE_CONFIG_ENTRY_VALID:
            return "DATABASE_CONFIG_ENTRY_VALID";
        case DATABASE_CONFIG_ENTRY_CREATE:
            return "DATABASE_CONFIG_ENTRY_CREATE";
        case DATABASE_CONFIG_ENTRY_DESTROY:
            return "DATABASE_CONFIG_ENTRY_DESTROY";
        case DATABASE_CONFIG_ENTRY_UNCOMMITTED:
            return "DATABASE_CONFIG_ENTRY_UNCOMMITTED";
        default:
            return "DATABASE_CONFIG_ENTRY_MODIFY";
    }
}

/*!**************************************************************
 * fbe_convert_configured_physical_block_size_to_string()
 ****************************************************************
 * @brief
 *  This function converts physical block size to a text string.
 *
 * @param 
 *        fbe_provision_drive_configured_physical_block_size_t 
 *		  configured_physical_block_size
 * 
 * @return
 *        fbe_char_t * A string for physical block size
 ***************************************************************/
fbe_char_t * fbe_convert_configured_physical_block_size_to_string
(fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size)
{
	switch(configured_physical_block_size)
    {
        case FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_INVALID:
            return "FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_INVALID";
        case FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520:
            return "FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520";
        case FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_4160:
            return "FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_4160";
		default:
            return "FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_UNSUPPORTED";
	}
}

/*!**************************************************************
 * fbe_convert_update_pvd_type_to_string()
 ****************************************************************
 * @brief
 *  This function converts update pvd type to a text string.
 *
 * @param 
 *        fbe_update_pvd_type_t update_type
 * 
 * @return
 *        fbe_char_t * A string for update pvd type
 ***************************************************************/
static fbe_char_t * fbe_convert_update_pvd_type_to_string(fbe_update_pvd_type_t update_type)
{
	switch(update_type)
    {
        case FBE_UPDATE_VD_INVALID:
            return "FBE_UPDATE_VD_INVALID";
		default:
            return "FBE_UPDATE_VD_MODE";
	}
}

/*!**************************************************************
 * fbe_cli_database_vd_configuration_mode_to_string()
 ****************************************************************
 * @brief
 *  This function converts update pvd type to a text string.
 *
 * @param 
 *        fbe_virtual_drive_configuration_mode_t configuration_mode
 * 
 * @return
 *        fbe_char_t * A string for vd_configuration_mode
 ***************************************************************/
static fbe_char_t * fbe_cli_database_vd_configuration_mode_to_string
	(fbe_virtual_drive_configuration_mode_t configuration_mode)
{
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN:
            return "FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN";
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            return "FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE";
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            return "FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE";
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            return "FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE";
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            return "FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE";
        default:
            return "FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN";
    }
}

/*!**************************************************************
 * fbe_convert_raid_group_type_to_string()
 ****************************************************************
 * @brief
 *  This function converts raid group type to a text string.
 *
 * @param 
 *        fbe_raid_group_type_t raid_type
 * 
 * @return
 *        fbe_char_t * A string for raid type
 ***************************************************************/
static fbe_char_t * fbe_convert_raid_group_type_to_string(fbe_raid_group_type_t raid_type)
{
	switch(raid_type)
    {
        case FBE_RAID_GROUP_TYPE_UNKNOWN:
            return "FBE_RAID_GROUP_TYPE_UNKNOWN";
		case FBE_RAID_GROUP_TYPE_RAID1:
            return "FBE_RAID_GROUP_TYPE_RAID1";
		case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
            return "FBE_RAID_GROUP_TYPE_RAW_MIRROR";
		case FBE_RAID_GROUP_TYPE_RAID10:
            return "FBE_RAID_GROUP_TYPE_RAID10";
		case FBE_RAID_GROUP_TYPE_RAID3:
            return "FBE_RAID_GROUP_TYPE_RAID3";
		case FBE_RAID_GROUP_TYPE_RAID0:
            return "FBE_RAID_GROUP_TYPE_RAID0";
		case FBE_RAID_GROUP_TYPE_RAID5:
            return "FBE_RAID_GROUP_TYPE_RAID5";
		case FBE_RAID_GROUP_TYPE_RAID6:
            return "FBE_RAID_GROUP_TYPE_RAID6";
		case FBE_RAID_GROUP_TYPE_SPARE:
            return "FBE_RAID_GROUP_TYPE_SPARE";
		case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
            return "FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER";
		case FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR:
            return "FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR";
		default:
            return "FBE_RAID_GROUP_TYPE_NON_PAGED_METADATA_MIRROR";
	}
}

/*!**************************************************************
 * fbe_convert_update_raid_type_to_string()
 ****************************************************************
 * @brief
 *  This function converts update raid type to a text string.
 *
 * @param 
 *        fbe_update_raid_type_t update_type
 * 
 * @return
 *        fbe_char_t * A string for update raid type
 ***************************************************************/
static fbe_char_t * fbe_convert_update_raid_type_to_string(fbe_update_raid_type_t update_type)
{
	switch(update_type)
    {
        case FBE_UPDATE_RAID_TYPE_INVALID:
            return "FBE_UPDATE_RAID_TYPE_INVALID";
		case FBE_UPDATE_RAID_TYPE_ENABLE_POWER_SAVE:
            return "FBE_UPDATE_RAID_TYPE_ENABLE_POWER_SAVE";
		case FBE_UPDATE_RAID_TYPE_DISABLE_POWER_SAVE:
            return "FBE_UPDATE_RAID_TYPE_DISABLE_POWER_SAVE";
		default:
            return "FBE_UPDATE_RAID_TYPE_CHANGE_IDLE_TIME";
	}
}

/*!**************************************************************
 * fbe_convert_raid_group_debug_flags_to_string()
 ****************************************************************
 * @brief
 *  This function converts raid group debug_flags to a text string.
 *
 * @param 
 *        fbe_raid_group_debug_flags_t debug_flags
 * 
 * @return
 *        fbe_char_t * A string for debug_flags
 ***************************************************************/
static fbe_char_t * fbe_convert_raid_group_debug_flags_to_string(fbe_raid_group_debug_flags_t debug_flags)
{
	switch(debug_flags)
    {
        case FBE_RAID_LIBRARY_DEBUG_FLAG_NONE:
            return "FBE_RAID_LIBRARY_DEBUG_FLAG_NONE";
		case FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING :
            return "FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING";
		case FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING:
            return "FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING";
		case FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING:
            return "FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING";
		case FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_TRACING:
            return "FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_TRACING";
		case FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_ERROR_TRACING:
            return "FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_ERROR_TRACING";
		case FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING:
            return "FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING";
		case FBE_RAID_LIBRARY_DEBUG_FLAG_WRITE_SECTOR_CHECKING:
            return "FBE_RAID_LIBRARY_DEBUG_FLAG_WRITE_SECTOR_CHECKING";
		case FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_INFO_TRACING:
            return "FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_INFO_TRACING";
		case FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACKING:
            return "FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACKING";
		case FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACING:
            return "FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACING";
		case FBE_RAID_LIBRARY_DEBUG_FLAG_WRITE_JOURNAL_TRACING:
            return "FBE_RAID_LIBRARY_DEBUG_FLAG_WRITE_JOURNAL_TRACING";
		case FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_LOG_TRACING:
            return "FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_LOG_TRACING";
		default:
            return "FBE_RAID_LIBRARY_DEBUG_FLAG_LAST";
	}
}




/*!**************************************************************
 * fbe_cli_print_database_user_entry_lu()
 ****************************************************************
 * @brief
 *  This function prints out the database user entry for lun.
 *
 * @param 
 *        database_user_entry_t    *user_entry
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_print_database_user_entry_lu(database_user_entry_t *user_entry)
{
    fbe_u32_t name_index = 0;
    fbe_u32_t name_size = 0;
    fbe_u32_t wwn_index = 0;

    if (!user_entry)
        return;

    fbe_cli_printf("\tlun user data \n");
    fbe_cli_printf("\t\texport_device_b : %s\n", 
                  (user_entry->user_data_union.lu_user_data.export_device_b)?"true":"false");
    fbe_cli_printf("\t\tlun_number : %d\n", user_entry->user_data_union.lu_user_data.lun_number);
    fbe_cli_printf("\t\tbind_time : 0x%llx\n", (unsigned long long)user_entry->user_data_union.lu_user_data.bind_time);
    fbe_cli_printf("\t\tuser_private_b : %s\n", 
                  (user_entry->user_data_union.lu_user_data.user_private)?"true":"false");
    fbe_cli_printf("\t\tattributes : 0x%X\n", user_entry->user_data_union.lu_user_data.attributes);

    // print user_defined_name 
    if(strlen(user_entry->user_data_union.lu_user_data.user_defined_name.name) > 0)
    {
        name_size = sizeof(user_entry->user_data_union.lu_user_data.user_defined_name.name);
        fbe_cli_printf("\t\tuser_defined_name:");

        for(name_index = 0; name_index < name_size && (user_entry->user_data_union.lu_user_data.user_defined_name.name[name_index] != (char)0); ++name_index)
        {
            if((user_entry->user_data_union.lu_user_data.user_defined_name.name[name_index] < ' ') || 
                (user_entry->user_data_union.lu_user_data.user_defined_name.name[name_index] > 'z'))
            {
                fbe_cli_printf(".");
            }
            else
            {
                fbe_cli_printf("%c",user_entry->user_data_union.lu_user_data.user_defined_name.name[name_index]);
            }
        }
        fbe_cli_printf("\n");
    }
    else
    {
        fbe_cli_printf("\t\tuser_defined_name: NULL\n");
    }

    // print world_wide_name 
    fbe_cli_printf("\t\tworld_wide_name: ");
    for(wwn_index = 0; wwn_index < FBE_WWN_BYTES; ++wwn_index)
    {
        fbe_cli_printf("%02x",(fbe_u32_t)(user_entry->user_data_union.lu_user_data.world_wide_name.bytes[wwn_index]));
        fbe_cli_printf(":");
    }
    fbe_cli_printf("\n");
}


/*!**************************************************************
 * fbe_cli_print_object_entry_pvd()
 ****************************************************************
 * @brief
 *  This function prints out the database or transaction object entry for pvd.
 *
 * @param 
 *        database_object_entry_t *object_entry
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_print_object_entry_pvd(database_object_entry_t *object_entry)
{
    if (object_entry != NULL)
    {
        fbe_cli_printf("\tpvd config information \n");
        fbe_cli_printf("\t\tconfig_type : %s\n",
                        fbe_convert_config_type_to_string(object_entry->set_config_union.pvd_config.config_type));
        fbe_cli_printf("\t\tconfig_capacity : 0x%llX\n", 
                        (unsigned long long)object_entry->set_config_union.pvd_config.configured_capacity);
        fbe_cli_printf("\t\tconfigured_physical_block_size : %s\n", 
                        fbe_convert_configured_physical_block_size_to_string(object_entry->set_config_union.pvd_config.configured_physical_block_size));
        fbe_cli_printf("\t\tsniff_verify_state : %s\n", 
                        (object_entry->set_config_union.pvd_config.sniff_verify_state)?"true":"false");
        fbe_cli_printf("\t\tgeneration_number : %llu\n",
		       (unsigned long long)object_entry->set_config_union.pvd_config.generation_number);
        fbe_cli_printf("\t\tserial_num : %s\n", object_entry->set_config_union.pvd_config.serial_num);
        fbe_cli_printf("\t\tupdate_type : %s\n",
                        fbe_convert_update_pvd_type_to_string(object_entry->set_config_union.pvd_config.update_type));
    }
}

/*!**************************************************************
 * fbe_cli_print_object_entry_vd()
 ****************************************************************
 * @brief
 *  This function prints out the database or transaction object entry for vd.
 *
 * @param 
 *        database_object_entry_t *object_entry
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_print_object_entry_vd(database_object_entry_t *object_entry)
{
    if (object_entry != NULL)
    {
        fbe_cli_printf("\tvd config information \n");
        fbe_cli_printf("\t\tconfiguration_mode : %s\n", 
                fbe_cli_database_vd_configuration_mode_to_string(object_entry->set_config_union.vd_config.configuration_mode));
        fbe_cli_printf("\t\tupdate_type : %s\n", 
                fbe_convert_update_pvd_type_to_string(object_entry->set_config_union.vd_config.update_vd_type));

        /*VD configuration itself contains a RAID Group configuration at the start, print it according to rg style*/
        fbe_cli_print_object_entry_rg(object_entry);
    }
}

/*!**************************************************************
 * fbe_cli_print_object_entry_rg()
 ****************************************************************
 * @brief
 *  This function prints out the database or transaction object entry for rg.
 *
 * @param 
 *        database_object_entry_t *obj_entry
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_print_object_entry_rg(database_object_entry_t *obj_entry)
{
    if (obj_entry != NULL)
    {
        fbe_cli_printf("\traid group config information \n");
        fbe_cli_printf("\t\twidth : %d\n", obj_entry->set_config_union.rg_config.width);
        fbe_cli_printf("\t\tcapacity : 0x%llX\n",
		       (unsigned long long)obj_entry->set_config_union.rg_config.capacity);
        fbe_cli_printf("\t\tchunk_size : %d\n", obj_entry->set_config_union.rg_config.chunk_size);
        fbe_cli_printf("\t\traid_type : %s\n",
                        fbe_convert_raid_group_type_to_string(obj_entry->set_config_union.rg_config.raid_type));
        fbe_cli_printf("\t\telement_size : %d\n", obj_entry->set_config_union.rg_config.element_size);
        fbe_cli_printf("\t\telements_per_parity : %d\n", 
                        obj_entry->set_config_union.rg_config.elements_per_parity);
        fbe_cli_printf("\t\tdebug_flags : %s\n",
                        fbe_convert_raid_group_debug_flags_to_string(obj_entry->set_config_union.rg_config.debug_flags));
        fbe_cli_printf("\t\tpower_saving_idle_time_in_seconds : %llu\n", 
                        (unsigned long long)obj_entry->set_config_union.rg_config.power_saving_idle_time_in_seconds);
        fbe_cli_printf("\t\tpower_saving_enabled : %s\n", 
                        (obj_entry->set_config_union.rg_config.power_saving_enabled)?"true":"false");
        fbe_cli_printf("\t\tmax_raid_latency_time_in_sec : %d\n", 
                        (int)obj_entry->set_config_union.rg_config.max_raid_latency_time_in_sec);
        fbe_cli_printf("\t\tgeneration_number : %d\n", 
                        (int)obj_entry->set_config_union.rg_config.generation_number);
        fbe_cli_printf("\t\tupdate_type : %s\n", 
                        fbe_convert_update_raid_type_to_string(obj_entry->set_config_union.rg_config.update_type));
        fbe_cli_printf("\t\traid_group_drive_type : %d\n", 
                        obj_entry->set_config_union.rg_config.raid_group_drives_type);
    }
}

/*!**************************************************************
 * fbe_cli_print_object_entry_lu()
 ****************************************************************
 * @brief
 *  This function prints out the database or transaction object entry for lun.
 *
 * @param 
 *        database_object_entry_t *object_entry
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_print_object_entry_lu(database_object_entry_t *object_entry)
{
    if (object_entry != NULL)
    {

        fbe_cli_printf("\tlun config information \n");

        fbe_cli_printf("\t\tcapacity : 0x%llX\n", (unsigned long long)object_entry->set_config_union.lu_config.capacity);
        fbe_cli_printf("\t\tndb_b : %s\n", 
                        (object_entry->set_config_union.lu_config.config_flags&FBE_LUN_CONFIG_NO_USER_ZERO)?"true":"false");
        fbe_cli_printf("\t\tgeneration_number : %llu\n", 
                        (unsigned long long)object_entry->set_config_union.lu_config.generation_number);
        fbe_cli_printf("\t\tpower_save_io_drain_delay_in_sec : %llu\n", 
                        (unsigned long long)object_entry->set_config_union.lu_config.power_save_io_drain_delay_in_sec);
        fbe_cli_printf("\t\tmax_lun_latency_time_is_sec : %llu\n", 
                        (unsigned long long)object_entry->set_config_union.lu_config.max_lun_latency_time_is_sec);
        fbe_cli_printf("\t\tpower_saving_idle_time_in_seconds : %llu\n", 
                        (unsigned long long)object_entry->set_config_union.lu_config.power_saving_idle_time_in_seconds);
        fbe_cli_printf("\t\tpower_saving_enabled : %s\n", 
                        (object_entry->set_config_union.lu_config.power_saving_enabled)?"true":"false");
        fbe_cli_printf("\t\tNO_INITIAL_VERIFY : %s\n", 
                        (object_entry->set_config_union.lu_config.config_flags&FBE_LUN_CONFIG_NO_INITIAL_VERIFY)?"true":"false");
        fbe_cli_printf("\t\tNO_BACKGROUND_ZERO : %s\n", 
                        (object_entry->set_config_union.lu_config.config_flags&FBE_LUN_CONFIG_CLEAR_NEED_ZERO)?"true":"false");

    }
}

/*!**************************************************************
 * fbe_cli_print_user_entry_pvd()
 ****************************************************************
 * @brief
 *  This function prints out the database or transaction user entry for pvd.
 *
 * @param 
 *        database_user_entry_t *user_entry
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_print_user_entry_pvd(database_user_entry_t *user_entry)
{
    if (user_entry != NULL)
    {
        fbe_cli_printf("\tpvd user data \n");
        fbe_cli_printf("\t\tpool_id : %d\n", user_entry->user_data_union.pvd_user_data.pool_id);
    }
}

/*!**************************************************************
 * fbe_cli_print_user_entry_rg()
 ****************************************************************
 * @brief
 *  This function prints out the database or transaction user entry for rg.
 *
 * @param 
 *        database_user_entry_t *user_entry
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_print_user_entry_rg(database_user_entry_t *user_entry)
{
    if (user_entry != NULL)
    {
        fbe_cli_printf("\trg user data \n");
        fbe_cli_printf("\t\traid_group_number : %d\n", 
                        user_entry->user_data_union.rg_user_data.raid_group_number);
        fbe_cli_printf("\t\tis_system : %s\n", 
                        (user_entry->user_data_union.rg_user_data.is_system)?"true":"false");
    }
}

/*!**************************************************************
 * fbe_cli_print_user_entry_lu()
 ****************************************************************
 * @brief
 *  This function prints out the database or transaction user entry for lun.
 *
 * @param 
 *        database_user_entry_t *user_entry
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_print_user_entry_lu(database_user_entry_t *user_entry)
{
    fbe_u32_t name_index = 0;
    fbe_u32_t name_size = 0;

    if (user_entry != NULL)
    {
        fbe_cli_printf("\tlun user data \n");
        fbe_cli_printf("\t\texport_device_b : %s\n", 
                        (user_entry->user_data_union.lu_user_data.export_device_b)?"true":"false");
        fbe_cli_printf("\t\tlun_number : %d\n", user_entry->user_data_union.lu_user_data.lun_number);

        // print user_defined_name 
        if(strlen(user_entry->user_data_union.lu_user_data.user_defined_name.name) > 0)
        {
            name_size = sizeof(user_entry->user_data_union.lu_user_data.user_defined_name.name);
            fbe_cli_printf("\t\tuser_defined_name:");

            for(name_index = 0; name_index < name_size && (user_entry->user_data_union.lu_user_data.user_defined_name.name[name_index] != (char)0); ++name_index)
            {
                if((user_entry->user_data_union.lu_user_data.user_defined_name.name[name_index] < ' ') || 
                    (user_entry->user_data_union.lu_user_data.user_defined_name.name[name_index] > 'z'))
                {
                    fbe_cli_printf(".");
                }
                else
                {
                    fbe_cli_printf("%c", user_entry->user_data_union.lu_user_data.user_defined_name.name[name_index]);
                }
            }
            fbe_cli_printf("\n");
        }
        else
        {
            fbe_cli_printf("\t\tuser_defined_name: NULL\n");
        }

    }
}



/*!**************************************************************
 * fbe_cli_print_global_info_entry()
 ****************************************************************
 * @brief
 *  This function prints out the database or transaction user entry for lun.
 *
 * @param 
 *        database_global_info_entry_t *global_entry
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_print_global_info_entry(database_global_info_entry_t *global_entry)
{

    if (global_entry != NULL)
    {
        fbe_cli_printf("\tdb_config_entry_state is : %s\n",fbe_convert_database_config_entry_state_to_string(
                            global_entry->header.state));
        switch(global_entry->type)
        {
        case    DATABASE_GLOBAL_INFO_TYPE_INVALID:
            fbe_cli_printf("\t\tDATABASE_GLOBAL_INFO_TYPE_INVALID\n");
            break;
        case    DATABASE_GLOBAL_INFO_TYPE_SYSTEM_POWER_SAVE:
            fbe_cli_printf("\t\tDATABASE_GLOBAL_INFO_TYPE_SYSTEM_POWER_SAVE\n");
            fbe_cli_printf("\t\tenabled : %s\n", 
                        (global_entry->info_union.power_saving_info.enabled)?"true":"false");
            fbe_cli_printf("\t\thibernation_wake_up_time_in_minutes: 0x%x\n", 
                        (unsigned int)global_entry->info_union.power_saving_info.hibernation_wake_up_time_in_minutes);
            fbe_cli_printf("\t\tpower saving statistics is: %s\n", 
                        (global_entry->info_union.power_saving_info.stats_enabled)?"enabled":"disabled");
            break;
        case    DATABASE_GLOBAL_INFO_TYPE_SYSTEM_ENCRYPTION:
            fbe_cli_printf("\t\tDATABASE_GLOBAL_INFO_TYPE_SYSTEM_ENCRYPTION\n");
            fbe_cli_printf("\t\tencryption mode : 0x%x\n", 
                        global_entry->info_union.encryption_info.encryption_mode);
            fbe_cli_printf("\t\tencryption paused : 0x%x\n", 
                        global_entry->info_union.encryption_info.encryption_paused);
            break;
        case    DATABASE_GLOBAL_INFO_TYPE_SYSTEM_SPARE:
            fbe_cli_printf("\t\tDATABASE_GLOBAL_INFO_TYPE_SYSTEM_SPARE\n");
            fbe_cli_printf("\t\tpermanent_spare_trigger_time: 0x%llx\n",
                        (unsigned long long)global_entry->info_union.spare_info.permanent_spare_trigger_time);
            break;
        case    DATABASE_GLOBAL_INFO_TYPE_SYSTEM_GENERATION:
            fbe_cli_printf("\t\tDATABASE_GLOBAL_INFO_TYPE_SYSTEM_GENERATION\n");
            fbe_cli_printf("\t\tcurrent_generation_number: 0x%llx\n",
                        (unsigned long long)global_entry->info_union.generation_info.current_generation_number);
            break;
        case    DATABASE_GLOBAL_INFO_TYPE_GLOBAL_PVD_CONFIG:
            fbe_cli_printf("\t\tDATABASE_GLOBAL_INFO_TYPE_SYSTEM_PVD\n");
            fbe_cli_printf("\t\tuser capacity limit : 0x%llx\n", 
                        (unsigned long long)global_entry->info_union.pvd_config.user_capacity_limit);
            break;
        default:
            fbe_cli_printf("\t\tDATABASE_GLOBAL_INFO_TYPE not recognized\n");
            break;
        }
    }
}




/*!**************************************************************
 * fbe_cli_get_database_transaction_state_string()
 ****************************************************************
 * @brief
 *  This function converts database transaction state to a text string.
 *
 * @param 
 *        fbe_database_state_t state
 * 
 * @return
 *        fbe_char_t *  A string for db transaction state
 ***************************************************************/
static fbe_char_t * fbe_cli_get_database_transaction_state_string(database_transaction_state_t state)
{
    switch(state)
    {
        case DATABASE_TRANSACTION_STATE_INACTIVE:
            return "DATABASE_TRANSACTION_STATE_INACTIVE";
        case DATABASE_TRANSACTION_STATE_ACTIVE:
            return "DATABASE_TRANSACTION_STATE_ACTIVE";
        case DATABASE_TRANSACTION_STATE_COMMIT:
            return "DATABASE_TRANSACTION_STATE_COMMIT";
        case DATABASE_TRANSACTION_STATE_ROLLBACK:
            return "DATABASE_TRANSACTION_STATE_ROLLBACK";

        default:
            return "unkown database transaction state";
    }
}



/*!**************************************************************
 * fbe_cli_print_database_transaction_info()
 ****************************************************************
 * @brief
 *  This function prints out the database user entry for rg.
 *
 * @param 
 *        in verbose  //  we  print the invalid entries
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_print_database_transaction_info(fbe_u32_t verbose)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_transaction_t           trans_info;
    database_user_entry_t            *transaction_user_table_ptr = NULL;
    database_object_entry_t          *transaction_object_table_ptr = NULL;
    database_edge_entry_t            *transaction_edge_table_ptr = NULL;
    database_global_info_entry_t     *global_info_table_ptr = NULL;
    fbe_u64_t index;
    fbe_u32_t found_entry;

    status = fbe_api_database_get_transaction_info(&trans_info);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nFailed to get transaction info ... Error: %d\n", status);
        return;
    }
    else
    {
        fbe_cli_printf("Database transaction state and its table \n");
        fbe_cli_printf("Transaction id : 0x%llx \n",
		       (unsigned long long)trans_info.transaction_id);
        fbe_cli_printf("Transaction state : %s \n", fbe_cli_get_database_transaction_state_string(trans_info.state)); 

        transaction_edge_table_ptr = trans_info.edge_entries;
        transaction_user_table_ptr = trans_info.user_entries;
        transaction_object_table_ptr = trans_info.object_entries;
        global_info_table_ptr = trans_info.global_info_entries;

        /*print the object entries*/
        fbe_cli_printf("\nDatabase transaction object table\n");
        found_entry = 0;
        for (index = 0; index < DATABASE_TRANSACTION_MAX_OBJECTS; index ++)
        {
            if (transaction_object_table_ptr[index].header.state == DATABASE_CONFIG_ENTRY_INVALID && verbose == 0)
                continue;
            found_entry ++;

            fbe_cli_printf("\tobject_entry_index : %llu\n",
			   (unsigned long long)index);
            fbe_cli_printf("\tdb_config_entry_state is : %s\n",fbe_convert_database_config_entry_state_to_string(
                transaction_object_table_ptr[index].header.state));
            fbe_cli_printf("\tobject_id : 0x%x\n", transaction_object_table_ptr[index].header.object_id);
            switch (transaction_object_table_ptr[index].db_class_id)
            {
            case    DATABASE_CLASS_ID_PROVISION_DRIVE:
                fbe_cli_printf("\tdb_class_id : %s\n", "DATABASE_CLASS_ID_PROVISION_DRIVE");
                fbe_cli_print_object_entry_pvd(transaction_object_table_ptr + index);
                break;
            case    DATABASE_CLASS_ID_VIRTUAL_DRIVE:
                fbe_cli_printf("\tdb_class_id : %s\n", "DATABASE_CLASS_ID_VIRTUAL_DRIVE");
                fbe_cli_print_object_entry_vd(transaction_object_table_ptr + index);
                break;
            case    DATABASE_CLASS_ID_MIRROR:
            case    DATABASE_CLASS_ID_STRIPER:
            case    DATABASE_CLASS_ID_PARITY:
                fbe_cli_printf("\tdb_class_id : %s\n", fbe_convert_db_class_id_to_string
                                (transaction_object_table_ptr[index].db_class_id));
                fbe_cli_print_object_entry_rg(transaction_object_table_ptr + index);
                break;
            case    DATABASE_CLASS_ID_LUN:
                fbe_cli_printf("\tdb_class_id : %s\n", "DATABASE_CLASS_ID_LUN");
                fbe_cli_print_object_entry_lu(transaction_object_table_ptr + index);
                break;
            case    DATABASE_CLASS_ID_BVD_INTERFACE:
                fbe_cli_printf("\tdb_class_id : %s\n", "DATABASE_CLASS_ID_BVD_INTERFACE");
                break;
            default:
                fbe_cli_printf("\tdb_class_id : %s\n", "UNKNOWN CLASS ID");
                break;
            }
            fbe_cli_printf("\n");
        }

        if (found_entry == 0)
            fbe_cli_printf("\n     No valid object entry found\n");

        /* print out the user entries */ 
        fbe_cli_printf("\nDatabase transaction user table\n");
        found_entry = 0;
        for (index = 0; index < DATABASE_TRANSACTION_MAX_USER_ENTRY; index ++)
        {
            if (transaction_user_table_ptr[index].header.state == DATABASE_CONFIG_ENTRY_INVALID && verbose == 0)
                continue;

            found_entry ++;
            fbe_cli_printf("\tuser_entry_index : %llu\n",
			   (unsigned long long)index);
            fbe_cli_printf("\tdb_config_entry_state is : %s\n", fbe_convert_database_config_entry_state_to_string
                            (transaction_user_table_ptr[index].header.state));
            fbe_cli_printf("\tobject_id : 0x%x\n", transaction_user_table_ptr[index].header.object_id);
            switch (transaction_user_table_ptr[index].db_class_id)
            {
            case    DATABASE_CLASS_ID_PROVISION_DRIVE:
                fbe_cli_printf("\tdb_class_id : %s\n", "DATABASE_CLASS_ID_PROVISION_DRIVE");
                fbe_cli_print_user_entry_pvd(transaction_user_table_ptr + index);
                break;
            case    DATABASE_CLASS_ID_MIRROR:
            case    DATABASE_CLASS_ID_STRIPER:
            case    DATABASE_CLASS_ID_PARITY:
                fbe_cli_printf("\tdb_class_id : %s\n", fbe_convert_db_class_id_to_string
                                (transaction_user_table_ptr[index].db_class_id));
                fbe_cli_print_user_entry_rg(transaction_user_table_ptr + index);
                break;
            case    DATABASE_CLASS_ID_LUN:
                fbe_cli_printf("\tdb_class_id : %s\n", "DATABASE_CLASS_ID_LUN");
                fbe_cli_print_user_entry_lu(transaction_user_table_ptr + index);
                break;
            case    DATABASE_CLASS_ID_BVD_INTERFACE:
                fbe_cli_printf("\tdb_class_id : %s\n", "DATABASE_CLASS_ID_BVD_INTERFACE");
                break;
            default:
                fbe_cli_printf("\tdb_class_id : %s\n", "UNKNOWN CLASS ID");
                break;
            }
            fbe_cli_printf("\n");
        }

        if (found_entry == 0)
            fbe_cli_printf("\n     No valid user entry found\n");

        /* print out the edge entries */
        fbe_cli_printf("\nDatabase transaction edge table\n");
        found_entry = 0;
        for (index = 0; index < DATABASE_TRANSACTION_MAX_EDGES; index ++)
        {
            if (transaction_edge_table_ptr[index].header.state == DATABASE_CONFIG_ENTRY_INVALID && verbose == 0)
                continue;
            found_entry ++;

            fbe_cli_printf("\tedge_index : %llu\n", (unsigned long long)index);
            fbe_cli_printf("\tdb_config_entry_state is : %s\n", fbe_convert_database_config_entry_state_to_string
                            (transaction_edge_table_ptr[index].header.state));

            if (transaction_edge_table_ptr[index].server_id != 0)
            {
                fbe_cli_printf("\t\tserver_id : 0x%x\n", transaction_edge_table_ptr[index].server_id);
                fbe_cli_printf("\t\tclient_index : %d\n", transaction_edge_table_ptr[index].client_index);
                fbe_cli_printf("\t\tcapacity : 0x%llX\n",
			       (unsigned long long)transaction_edge_table_ptr[index].capacity);
                fbe_cli_printf("\t\toffset : 0x%llX\n\n",
			       (unsigned long long)transaction_edge_table_ptr[index].offset);		
            }
            else
                fbe_cli_printf("\t\tThe edge has server id 0x0\n");

            fbe_cli_printf("\n");

        }

        if (found_entry == 0)
            fbe_cli_printf("\n     No valid edge entry found\n");

        /* print out global info entries */
        fbe_cli_printf("\nDatabase transaction global info table\n");
        found_entry = 0;
        for (index = 0; index < DATABASE_TRANSACTION_MAX_GLOBAL_INFO; index ++)
        {
            if (global_info_table_ptr[index].header.state == DATABASE_CONFIG_ENTRY_INVALID && verbose == 0)
                continue;

            found_entry ++;
            fbe_cli_printf("\tglobal_info_entry_index : %llu\n",
			   (unsigned long long)index);
            fbe_cli_print_global_info_entry(global_info_table_ptr + index);
            fbe_cli_printf("\n");
        }
        
        if (found_entry == 0)
            fbe_cli_printf("\n     No valid global info entry found \n");

    }

    return;
}


static void fbe_cli_print_database_chassis_replacement_flag(void)
{
    fbe_homewrecker_get_fru_disk_mask_t read_slot = FBE_FRU_DISK_ALL;
    fbe_homewrecker_fru_descriptor_t descriptor = {0};
    fbe_bool_t warning = FBE_TRUE;
    fbe_status_t status = FBE_STATUS_INVALID;

    status = fbe_api_database_get_fru_descriptor_info(&descriptor, read_slot, &warning);
    if(FBE_STATUS_OK != status)
    {
        fbe_cli_error("\nFailed to get fru descriptor ... Error: %d\n", status);
        return;
    }
    if(FBE_CHASSIS_REPLACEMENT_FLAG_TRUE == descriptor.chassis_replacement_movement)
        fbe_cli_printf("\nChassis replacement flag is set \n");
    else if(FBE_CHASSIS_REPLACEMENT_FLAG_INVALID == descriptor.chassis_replacement_movement)
        fbe_cli_printf("\nChassis replacement flag is not set \n");
    else
        fbe_cli_printf("\nChassis replacement flag has a invalid value %u\n", descriptor.chassis_replacement_movement);

    if (warning == FBE_TRUE)
        fbe_cli_printf("\nRaw mirror access has warning, please check system disk\n");

    return;
}


static void fbe_cli_set_database_chassis_replacement_flag(fbe_u32_t  set)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_database_control_set_fru_descriptor_t descriptor = {0};
    fbe_database_control_set_fru_descriptor_mask_t modify = FBE_FRU_CHASSIS_REPLACEMENT;
    fbe_cmi_service_get_info_t cmi_info;

    /* if cmi is not in service mode, only ACTIVE side can triger this CLI
      * that is becaused, only ACTIVE side DB would send in-memory to
      * PASSIVE side
       */

    status = fbe_api_cmi_service_get_info(&cmi_info);
    if(FBE_STATUS_OK != status)
    {
        fbe_cli_error("\nFailed to get cmi info ... Error: %d\nPlease use this cli on active SP\n", status);
        return;
    }

    if(FBE_CMI_STATE_ACTIVE != cmi_info.sp_state && FBE_CMI_STATE_SERVICE_MODE != cmi_info.sp_state)
    {
        fbe_cli_error("\n\tThis cli can only be used on active side or degraded mode\n");
        return;
    }
    
    if(set > 0)
        descriptor.chassis_replacement_movement = FBE_CHASSIS_REPLACEMENT_FLAG_TRUE;
    else
        descriptor.chassis_replacement_movement = FBE_CHASSIS_REPLACEMENT_FLAG_INVALID;
        
    status = fbe_api_database_set_fru_descriptor_info(&descriptor, modify);
    if (status == FBE_STATUS_OK)
    {
        if(set > 0)
            fbe_cli_printf("\nSet chassis replacement flag successfully\n");
        else
            fbe_cli_printf("\nClear chassis replacement flag successfully\n");
    }
    else
    {
        if(set > 0)
            fbe_cli_printf("\nSet chassis replacement flag failed\n");
        else
            fbe_cli_printf("\nClear chassis replacement flag failed\n");
    }

    return;
}


static void fbe_cli_print_database_fru_descriptor(fbe_u32_t  slot_no)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t   count = 0;
    fbe_homewrecker_get_fru_disk_mask_t read_slot = FBE_FRU_DISK_INVALID;
    fbe_homewrecker_fru_descriptor_t descriptor = {0};
    fbe_bool_t warning = FBE_TRUE;
    fbe_cmi_service_get_info_t cmi_info;

    status = fbe_api_cmi_service_get_info(&cmi_info);
    if(FBE_STATUS_OK != status)
    {
        fbe_cli_error("\nFailed to get cmi info ... Error: %d\nPlease use this cli on active SP\n", status);
        return;
    }

    if(FBE_CMI_STATE_ACTIVE != cmi_info.sp_state
        && FBE_CMI_STATE_SERVICE_MODE != cmi_info.sp_state)
    {
        fbe_cli_error("\n\tThis cli can only be used on active side or degraded mode\n");
        return;
    }

    if(slot_no >= FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER) /*print all the fru descriptors*/
    {
        read_slot = FBE_FRU_DISK_ALL;
        status = fbe_api_database_get_fru_descriptor_info(&descriptor, read_slot, &warning);
        if(FBE_STATUS_OK != status)
        {
            fbe_cli_error("\nFailed to get fru descriptor ... Error: %d\n", status);
            return;
        }
    }
    else
    {
        switch (slot_no)
        {
            case 0:
                read_slot = FBE_FRU_DISK_0;
                break;
            case 1:
                read_slot = FBE_FRU_DISK_1;
                break;
            case 2:
                read_slot = FBE_FRU_DISK_2;
                break;
            default:
                read_slot = FBE_FRU_DISK_INVALID;
        }
        status = fbe_api_database_get_fru_descriptor_info(&descriptor, read_slot, &warning);
        if(FBE_STATUS_OK != status)
        {
            fbe_cli_error("\nFailed to get fru descriptor with slot_no %d ... Error: %d\n", slot_no, status);
            return;
        }
    }

    if (read_slot == FBE_FRU_DISK_ALL)
    {
        fbe_cli_printf("\nFRU DESCRIPTOR :\n\n");
        for (count = 0; count < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; count++)
        {
            fbe_cli_printf("\tserial number on slot %d: %s\n", count, descriptor.system_drive_serial_number[count]);
        }
    }
    else
    {
        fbe_cli_printf("\nFRU DESCRIPTOR on slot %d:\n\n", slot_no);
        fbe_cli_printf("\tserial number on slot %d: %s\n", slot_no, descriptor.system_drive_serial_number[slot_no]);
    }
    
    fbe_cli_printf("\tmagic string: %s\n", descriptor.magic_string);
    fbe_cli_printf("\twwn seed: 0x%x\n", descriptor.wwn_seed);
    fbe_cli_printf("\tchassis replacemen movement: 0x%x\n", descriptor.chassis_replacement_movement);
    fbe_cli_printf("\tsturcture version: 0x%x\n", descriptor.structure_version);
    
    if (warning == FBE_TRUE)
        fbe_cli_printf("\nRaw mirror access has warning, please check system disk\n");

    return;
}


static void fbe_cli_clear_database_fru_descriptor(void)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_database_control_set_fru_descriptor_t descriptor = {0};
    fbe_database_control_set_fru_descriptor_mask_t modify = FBE_FRU_CHASSIS_REPLACEMENT|FBE_FRU_WWN_SEED|
                                                            FBE_FRU_STRUCTURE_VERSION;
    fbe_cmi_service_get_info_t cmi_info;

    status = fbe_api_cmi_service_get_info(&cmi_info);
    if(FBE_STATUS_OK != status)
    {
        fbe_cli_error("\nFailed to get cmi info ... Error: %d\nPlease use this cli on active SP\n", status);
        return;
    }

    if(FBE_CMI_STATE_ACTIVE != cmi_info.sp_state && FBE_CMI_STATE_SERVICE_MODE != cmi_info.sp_state)
    {
        fbe_cli_error("\n\tThis cli can only be used on active side or degraded mode\n");
        return;
    }

    descriptor.chassis_replacement_movement = FBE_CHASSIS_REPLACEMENT_FLAG_INVALID;
    descriptor.structure_version = FBE_FRU_DESCRIPTOR_STRUCTURE_VERSION;
    descriptor.wwn_seed = INVALID_WWN_SEED;

    status = fbe_api_database_set_fru_descriptor_info(&descriptor, modify);
    if(FBE_STATUS_OK != status)
    {
        fbe_cli_printf("\nFailed to clear fru descriptor, status: %d\n", status);
        return;
    }
    
    return;
}

static void fbe_cli_set_database_fru_descriptor(void)
{
    fbe_u32_t       system_drive_index = 0;
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_database_control_set_fru_descriptor_t descriptor = {0};
    fbe_database_control_set_fru_descriptor_mask_t modify = FBE_FRU_CHASSIS_REPLACEMENT|FBE_FRU_WWN_SEED|
                                                            FBE_FRU_STRUCTURE_VERSION|FBE_FRU_SERIAL_NUMBER;
    fbe_physical_drive_attributes_t drv_info;
    fbe_object_id_t objid;
    fbe_cmi_service_get_info_t cmi_info;

    status = fbe_api_cmi_service_get_info(&cmi_info);
    if(FBE_STATUS_OK != status)
    {
        fbe_cli_error("\nFailed to get cmi info ... Error: %d\nPlease use this cli on active SP\n", status);
        return;
    }

    if(FBE_CMI_STATE_ACTIVE != cmi_info.sp_state && FBE_CMI_STATE_SERVICE_MODE != cmi_info.sp_state)
    {
        fbe_cli_error("\n\tThis cli can only be used on active side or degraded mode\n");
        return;
    }
    
    descriptor.chassis_replacement_movement = FBE_CHASSIS_REPLACEMENT_FLAG_INVALID;
    descriptor.structure_version = FBE_FRU_DESCRIPTOR_STRUCTURE_VERSION;

    for (system_drive_index = 0;
         system_drive_index < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; system_drive_index++)    
    {
        status = fbe_api_get_physical_drive_object_id_by_location(0,0,system_drive_index, &objid);
        if(FBE_STATUS_OK != status)
        {
            fbe_cli_printf("\nFailed to get LDO objectid\n");
            return;
        }
        
        status = fbe_api_physical_drive_get_attributes(objid, &drv_info);
        if(FBE_STATUS_OK != status)
        {
            fbe_cli_printf("\nFailed to get drive serial number\n");
            return;
        }
        
        fbe_copy_memory(descriptor.system_drive_serial_number[system_drive_index], 
                        drv_info.initial_identify.identify_info, sizeof(serial_num_t));
    }
    

    status = fbe_api_database_system_obtain_prom_wwnseed_cmd(&descriptor.wwn_seed);
    if(FBE_STATUS_OK != status)
    {
        fbe_cli_printf("\nFailed to get wwn seed from Midplane Resume PROM\n");
        return;
    }


    status = fbe_api_database_set_fru_descriptor_info(&descriptor, modify);
    if(FBE_STATUS_OK != status)
    {
        fbe_cli_printf("\nFailed to set fru descriptor, status: %d\n", status);
        return;
    }
    
    return;

}


static void fbe_cli_get_prom_wwnseed(void)
{
    fbe_status_t                                status = FBE_STATUS_OK;

    fbe_u32_t get_wwn_seed = 0;
    status = fbe_api_database_system_obtain_prom_wwnseed_cmd(&get_wwn_seed);
    if(FBE_STATUS_OK != status)
    {
        fbe_cli_printf("\nFailed to get wwn seed from Midplane Resume PROM\n");
        return;
    }
    fbe_cli_printf("Midplane Resume PROM WWN Seed: 0x%x(Hex)\n", get_wwn_seed);
   
    return;
}

static void fbe_cli_set_prom_wwnseed(fbe_u32_t wwnseed)
{
    fbe_status_t                                status = FBE_STATUS_OK;

    fbe_cli_printf("Write WWN SEED to Midplane Resume Prom:0x%x \n", wwnseed);
    status = fbe_api_database_system_persist_prom_wwnseed_cmd(&wwnseed);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error ("%s:Failed to set Midplane Resume Prom WWN SEED\n", 
            __FUNCTION__);
        return;
    }
    else
    {
        fbe_cli_printf("Successfully write\n");
    }
    return;
}

/*!**************************************************************
 * fbe_cli_print_system_db_header_info()
 ****************************************************************
 * @brief
 *  This function prints out the system db header information.
 *
 * @param 
 *        in verbose  // we will print the entry size /nonpaged metadata size value. 
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_print_system_db_header_info(fbe_u32_t verbose)
{
    database_system_db_header_t system_db_header;
    fbe_status_t    status;

    status = fbe_api_database_get_system_db_header(&system_db_header);
    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("\nFailed to get system db header, status = %d\n",
                        status);
        return;
    }

    fbe_cli_printf("Database system db header \n");
    fbe_cli_printf("\tMagic number: 0x%x\n", (unsigned int)system_db_header.magic_number);
    fbe_cli_printf("\tVersioned size of system db header: 0x%x\n", system_db_header.version_header.size);
    fbe_cli_printf("\tPersisted sep version: 0x%x\n", (unsigned int)system_db_header.persisted_sep_version);

    if (verbose) {
        fbe_cli_printf("\nObject entry size\n ");
        fbe_cli_printf("\tBVD_interface object entry size: 0x%x\n", system_db_header.bvd_interface_object_entry_size);
        fbe_cli_printf("\tPVD object entry size: 0x%x\n", system_db_header.pvd_object_entry_size);
        fbe_cli_printf("\tVD object entry size: 0x%x\n", system_db_header.vd_object_entry_size);
        fbe_cli_printf("\tRAID object entry size: 0x%x\n", system_db_header.rg_object_entry_size);
        fbe_cli_printf("\tLUN object entry size: 0x%x\n", system_db_header.lun_object_entry_size);

       
        fbe_cli_printf("\nEdge entry size: 0x%x\n", system_db_header.edge_entry_size);

        fbe_cli_printf("\nUser entry size\n");
        fbe_cli_printf("\tPVD user entry size: 0x%x\n", system_db_header.pvd_user_entry_size);
        fbe_cli_printf("\tRAID user entry size: 0x%x\n", system_db_header.rg_user_entry_size);
        fbe_cli_printf("\tLUN user entry size: 0x%x\n", system_db_header.lun_user_entry_size);

        fbe_cli_printf("\nGlobal info entry size\n");
        fbe_cli_printf("\tPower save info entry size: 0x%x\n", system_db_header.power_save_info_global_info_entry_size);
        fbe_cli_printf("\tSpare info entry size: 0x%x\n", system_db_header.spare_info_global_info_entry_size);
        fbe_cli_printf("\tGeneration info entry size: 0x%x\n", system_db_header.generation_info_global_info_entry_size);
        fbe_cli_printf("\tTime threshold info entry size: 0x%x\n", system_db_header.time_threshold_info_global_info_entry_size);
        
        fbe_cli_printf("\nNonpaged metadata size\n");
        fbe_cli_printf("\tPVD nonpaged metadata size: 0x%x\n", system_db_header.pvd_nonpaged_metadata_size);
        fbe_cli_printf("\tLUN nonpaged metadata size: 0x%x\n", system_db_header.lun_nonpaged_metadata_size);
        fbe_cli_printf("\tRAID nonpaged metadata size: 0x%x\n", system_db_header.raid_nonpaged_metadata_size);
    }
    return;    
}

void fbe_cli_cmd_online_planned_drive(int argc , char ** argv)
{
    fbe_database_control_online_planned_drive_t online_drive;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    /* give the description of this commands */
    if (argc < 1)
    {
        fbe_cli_printf("This commands make a planned drive online, which can be used in following case:\n\
User inserts a drive into current array, homewrecker detects it is an illegal drive\
and blocks it online. However, user knows the risks and makes this drive insertion\
intentionally. Then the user can use this cli to force this drive online.\n\n\
[switches]\n\
Switches:\n\
-d bus enclosure slot\n\n");
        return;
    }

    if(4 != argc)
    {
        fbe_cli_printf("Invalid argument number\n\
[valid switches]\n\
Switches:\n\
-d bus enclosure slot\n\n");
        return;
    }

    if (0 != strcmp(argv[0], "-d"))
    {
        fbe_cli_printf("Invalid switch\n\
[valid switches]\n\
Switches:\n\
-d bus enclosure slot\n\n");
        return;
    }

    online_drive.port_number = fbe_atoi(argv[1]);
    online_drive.enclosure_number = fbe_atoi(argv[2]);
    online_drive.slot_number = fbe_atoi(argv[3]);
    online_drive.pdo_object_id = FBE_OBJECT_ID_INVALID;

    status = fbe_api_database_online_planned_drive(&online_drive);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error ("Failed to make drive online. Please make sure you input the correct drive location and try again.\n\n");
        return;
    }
    else
    {
        fbe_cli_printf("Successfully unblock the drive online. Please check the status of the drive later.\n\n");
    }
    return;

}

/*!**********************************************************************
*                      fbe_cli_database_hook                 
*************************************************************************
*
*  @brief
*    fbe_cli_database_hook - set/unset hook in database service
	
*  @param    argc - argument count
*  @param    argv - argument string
*
*  @return
*            None
************************************************************************/
void fbe_cli_database_hook(fbe_s32_t argc, fbe_s8_t ** argv)
{
    if (argc < 1)
    {
        /*If there are no arguments show usage*/
        fbe_cli_printf("%s", FBE_CLI_DATABASE_HOOK_USAGE);
        return;
    }
    
    if ((strncmp(*argv, "-set_debug_hook", 16) == 0))
    {
        argc--;
        argv++;
        
        fbe_cli_lib_database_add_hook(*argv);
        argc--;
        argv++;
        
        return;
    }
    else if ((strncmp(*argv, "-remove_debug_hook", 19) == 0))
    {
        argc--;
        argv++;
        
        fbe_cli_lib_database_remove_hook(*argv);
        argc--;
        argv++;
        
        return;
    }    
    else
    {
        /*If there are no arguments show usage*/
        fbe_cli_error("%s", FBE_CLI_DATABASE_HOOK_USAGE);
        return;
    }
}


static void fbe_cli_lib_database_add_hook(char * argv)
{
    fbe_status_t    status;

    if(strcmp(argv, "panic_in_active_tran") == 0)
    {
        status = fbe_api_database_add_hook(FBE_DATABASE_HOOK_TYPE_PANIC_IN_UPDATE_TRANSACTION);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to add hook for database service!\n");
            return;
        }

        fbe_cli_printf("Successfully add hook in database service!\n");
        return;
    }
    else if(strcmp(argv, "panic_before_persist") == 0)
    {
        status = fbe_api_database_add_hook(FBE_DATABASE_HOOK_TYPE_PANIC_BEFORE_TRANSACTION_PERSIST);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to add hook for database service!\n");
            return;
        }

        fbe_cli_printf("Successfully add hook in database service!\n");
        return;
    }
    else
    {
        fbe_cli_error("Unrecognized parameter \"%s\".\n", argv);
        return;
    }
    
}

static void fbe_cli_lib_database_remove_hook(char * argv)
{
    fbe_status_t    status;

    if(strcmp(argv, "panic_in_active_tran") == 0)
    {
        status = fbe_api_database_remove_hook(FBE_DATABASE_HOOK_TYPE_PANIC_IN_UPDATE_TRANSACTION);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to remove hook for database service!\n");
            return;
        }

        fbe_cli_printf("Successfully remove hook in database service!\n");
        return;
    }
    else if(strcmp(argv, "panic_before_persist") == 0)
    {
        status = fbe_api_database_remove_hook(FBE_DATABASE_HOOK_TYPE_PANIC_BEFORE_TRANSACTION_PERSIST);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to remove hook for database service!\n");
            return;
        }

        fbe_cli_printf("Successfully remove hook in database service!\n");
        return;
    }
    else
    {
        fbe_cli_error("Unrecognized parameter \"%s\".\n", argv);
        return;
    }
    
}

/*!**************************************************************
 * fbe_cli_print_database_stats()
 ****************************************************************
 * @brief
 *  This function prints out the database stats.
 *
 * @param 
 *        None
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_print_database_stats(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_get_stats_t get_stats;
    
    status = fbe_api_database_get_stats(&get_stats);
    
    if ((status == FBE_STATUS_OK))
    {
       fbe_cli_printf("\tNumber of system luns: %d\n", get_stats.num_system_luns);
       fbe_cli_printf("\tNumber of user luns: %d\n", get_stats.num_user_luns);
       fbe_cli_printf("\tNumber of system raids: %d\n", get_stats.num_system_raids);
       fbe_cli_printf("\tNumber of user raids: %d\n", get_stats.num_user_raids);
       fbe_cli_printf("\tMax allowed luns per RG: %d\n", get_stats.max_allowed_luns_per_rg);
       fbe_cli_printf("\tMax allowed user luns: %d\n", get_stats.max_allowed_user_luns);
       fbe_cli_printf("\tMax allowed user rgs: %d\n", get_stats.max_allowed_user_rgs);
       fbe_cli_printf("\tLast poll time: 0x%llu\n", (unsigned long long)get_stats.last_poll_time);
       fbe_cli_printf("\tLast poll bitmap: %d\n", get_stats.last_poll_bitmap);
       fbe_cli_printf("\tThreshold count: %d\n", get_stats.threshold_count);
    }
    else
    {
        fbe_cli_printf("\nFailed to get database stats ... Error: %d\n", status);            
    }
    return;
}

/*!**************************************************************
 * fbe_cli_print_database_cap_limit()
 ****************************************************************
 * @brief
 *  This function prints out the pvd capacity limit.
 *
 * @param 
 *        None
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_print_database_cap_limit(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_capacity_limit_t get_limit;
    
    status = fbe_api_database_get_capacity_limit(&get_limit);
    
    if ((status == FBE_STATUS_OK))
    {
       fbe_cli_printf("\tPVD capacity limit: %d\n", get_limit.cap_limit);
    }
    else
    {
        fbe_cli_printf("\nFailed to get database capacity limit ... Error: %d\n", status);            
    }
    return;
}

/*!**************************************************************
 * fbe_cli_print_database_cap_limit()
 ****************************************************************
 * @brief
 *  This function prints out the pvd capacity limit.
 *
 * @param 
 *        cap_limit - capacity limit
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_set_database_cap_limit(fbe_u32_t cap_limit)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_capacity_limit_t set_limit;

    set_limit.cap_limit = cap_limit;
    status = fbe_api_database_set_capacity_limit(&set_limit);
    
    if ((status == FBE_STATUS_OK))
    {
       fbe_cli_printf("\t Setting PVD capacity limit: %d\n", set_limit.cap_limit);
    }
    else
    {
        fbe_cli_printf("\nFailed to set database capacity limit ... Error: %d\n", status);            
    }
    return;
}

static fbe_status_t fbe_cli_lib_database_debug_flag_to_string(fbe_database_debug_flags_t debug_flag,
                                                              const fbe_u8_t **debug_flag_string_pp)

{
    switch(debug_flag) {
        case FBE_DATABASE_DEBUG_FLAG_NONE:
            *debug_flag_string_pp = "FLAG_NONE";
            break;
        case FBE_DATABASE_DEBUG_FLAG_VALIDATE_DATABASE_ON_PEER_LOSS:
            *debug_flag_string_pp = "VALIDATE_DATABASE_ON_PEER_LOSS";
            break;
        case FBE_DATABASE_DEBUG_FLAG_VALIDATE_DATABASE_ON_DESTROY:
            *debug_flag_string_pp = "VALIDATE_DATABASE_ON_DESTROY_JOB";
            break;

        case FBE_DATABASE_DEBUG_FLAG_LAST:
        default:
            *debug_flag_string_pp = "UNSUPPORTED DATABASE DEBUG FLAG";
            return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}


static void fbe_cli_lib_database_debug_display_debug_flags(fbe_database_debug_flags_t debug_flags)
{
    fbe_status_t    status;
    fbe_u32_t       shiftBit, index;
    const fbe_u8_t *debug_flag_string = NULL;
    fbe_u32_t       shift_bit_max = FBE_DATABASE_DEBUG_FLAG_LAST / 2;

    for (index = 0; index < shift_bit_max; index++) {
        shiftBit = (1 << index);
        if (debug_flags & shiftBit) {
            status = fbe_cli_lib_database_debug_flag_to_string((debug_flags & shiftBit),
                                                               &debug_flag_string);
            if (status != FBE_STATUS_OK) {
                fbe_cli_printf("\t UNSUPPORTED FLAG:0x%x \n", (debug_flags & shiftBit)); 
            } else {
                fbe_cli_printf("\t %s \n", debug_flag_string);
            }
        }
    }

    return;
}

static void fbe_cli_lib_set_database_debug_flags(fbe_s32_t argc, char * argv)
{
    fbe_status_t                status = FBE_STATUS_OK;
	fbe_database_debug_flags_t debug_flags;

    if (argc < 1) {
        fbe_cli_error("-set_debug_flags requires a flags parameter\n");
        return;
    }

    debug_flags = (fbe_database_debug_flags_t)strtoul(argv, 0, 0);
    
    if (debug_flags >= FBE_DATABASE_DEBUG_FLAG_LAST ) {
		fbe_cli_printf("Invalid set debug flags: 0x%x\n", debug_flags);
		return;
    }

    fbe_cli_printf("Set database service debug flags to: 0x%x \n", debug_flags);
    fbe_cli_lib_database_debug_display_debug_flags(debug_flags);
    status = fbe_api_database_set_debug_flags(debug_flags);
    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("Failed to set database debug flags - status: 0x%x \n", status);
    }
	return;
}

static void fbe_cli_lib_get_database_debug_flags(fbe_s32_t argc, char *argv)
{
    fbe_status_t                status = FBE_STATUS_OK;
	fbe_database_debug_flags_t  debug_flags;

    if (argc > 0) {
        fbe_cli_error("-get_debug_flags doesn't expect an argument: %s\n", argv);
        return;
    }

    status = fbe_api_database_get_debug_flags(&debug_flags);
    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("Failed to get database debug flags - status: 0x%x \n", status);
        return;
    }

    fbe_cli_printf("Database service debug flags are set to: 0x%x \n", debug_flags);
    fbe_cli_lib_database_debug_display_debug_flags(debug_flags);
    return;
}

static void fbe_cli_lib_validate_database(fbe_s32_t argc, const char *action_string, const char *caller_string)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_validate_failure_action_t  failure_action = FBE_DATABASE_VALIDATE_FAILURE_ACTION_NONE;
    const fbe_u8_t                         *failure_action_string = NULL;
    fbe_database_validate_request_type_t    request_type = FBE_DATABASE_VALIDATE_REQUEST_TYPE_INVALID;
    const fbe_u8_t                         *request_type_string = NULL;

    /* Convert the action to a value.*/
    if (strncmp(action_string, "e", 1) == 0) {
        failure_action = FBE_DATABASE_VALIDATE_FAILURE_ACTION_ERROR_TRACE;
        failure_action_string = "Error Trace";
    } else if (strncmp(action_string, "d", 1) == 0) {
        failure_action = FBE_DATABASE_VALIDATE_FAILURE_ACTION_ENTER_DEGRADED_MODE;
        failure_action_string = "Enter Degraded Mode";
    } else if (strncmp(action_string, "p", 1) == 0) {
        failure_action = FBE_DATABASE_VALIDATE_FAILURE_ACTION_PANIC_SP;
        failure_action_string = "Enter Degraded Mode - Then PANIC";
    } else if (strncmp(action_string, "c", 1) == 0) {
        fbe_cli_printf("Validate database failure action of `correct' not supported.\n");
        return;
    } else {
        fbe_cli_printf("Validate database failure action: %s not supported\n",action_string);
        return;
    }

    /* Convert the caller to a value.*/
    if (strncmp(caller_string, "c", 1) == 0) {
        request_type = FBE_DATABASE_VALIDATE_REQUEST_TYPE_SP_COLLECT;
        request_type_string = "SP Collect";
    } else if (strncmp(caller_string, "p", 1) == 0) {
        request_type = FBE_DATABASE_VALIDATE_REQUEST_TYPE_PEER_LOST;
        request_type_string = "Peer Lost";
    } else if (strncmp(caller_string, "u", 1) == 0) {
        request_type = FBE_DATABASE_VALIDATE_REQUEST_TYPE_USER;
        request_type_string = "User Request";
    } else {
        fbe_cli_printf("Validate database caller: %s not supported\n", caller_string);
        return;
    }

    /* Send the request. */
    status = fbe_api_database_validate_database(request_type, failure_action);
    fbe_cli_printf("Validate database caller: %s failure action of: %s complete with status: 0x%x\n", 
                   request_type_string, failure_action_string, status);
	return;
}

/*!**********************************************************************
*                      fbe_cli_database_debug()            
*************************************************************************
*
*  @brief
*            fbe_cli_database_debug - Debug commands and information
	
*  @param    argc - argument count
*  @param    argv - argument string
*
*  @return
*            None
*  @author 
*   06/30/2014  Ron Proulx  - Created 
************************************************************************/
void fbe_cli_database_debug(fbe_s32_t argc, fbe_s8_t ** argv)
{
    const char *action_string = NULL;
    const char *caller_string = "u";

    if (argc < 1) {
        fbe_cli_printf("%s", FBE_CLI_DATABASE_DEBUG_USAGE);
        return;
    }

    if ((strcmp(*argv, "-help") == 0) ||(strcmp(*argv, "-h") == 0)) {
        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("%s", FBE_CLI_DATABASE_DEBUG_USAGE);
        return;
    }

    /* Process the command line. */
    if ((strncmp(*argv, "-set_debug_flags", 16) == 0) ||
        (strncmp(*argv, "-sdf", 4) == 0)                 ) {
        /* Get the flags from the command line*/
        argc--;
        argv++;
        fbe_cli_lib_set_database_debug_flags(argc, *argv);
    } else if((strncmp(*argv, "-get_debug_flags", 16) == 0) ||
            (strncmp(*argv, "-gdf", 4) == 0)                   ) {
        /* Get the flags */
        argc--;
        argv++;
        fbe_cli_lib_get_database_debug_flags(argc, *argv);
    } else if(strncmp(*argv, "-collect_validate", 17) == 0) {
        /* Special version of validate used during spcollect */
        argc--;
        argv++;
        /*! @note The current action is to log failure and generate an error trace !!*/
        action_string = "e";
        caller_string = "c";
        fbe_cli_lib_validate_database(argc, action_string, caller_string);
    } else if(strncmp(*argv, "-validate", 9) == 0) {
        /* Validate database and take requested action if it fails.*/
        argc--;
        argv++;
        if (argc < 1) {
            fbe_cli_error("Please select the action if validation fails \n");
            return;
        }
        action_string = *argv;
        argc--;
        argv++;
        if ((argc > 0)                          &&
            (strncmp(*argv, "-caller", 7) == 0)    ) {
            argc--;
            argv++;
            if (argc < 1) {
                fbe_cli_error("Please select the caller if validation fails \n");
                return;
            }
            caller_string = *argv;
        }
        fbe_cli_lib_validate_database(argc, action_string, caller_string);
    } else {
        fbe_cli_printf("%s is not a supported request\n", *argv);
        return;
    }

    fbe_cli_printf("\n");		
    return;
    
};


/*!**********************************************************************
*                      fbe_cli_print_database_additional_supported_drive_types()            
*************************************************************************
*
*  @brief
*            prints the additional drive types that are supported
	
*  @param    void
*
*  @return
*            None
*  @author 
*   08/19/2015  Wayne Garrett  - Created 
************************************************************************/
static void fbe_cli_print_database_supported_drive_types(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_additional_drive_types_supported_t types;
    status = fbe_api_database_get_additional_supported_drive_types(&types);

    if ((status != FBE_STATUS_OK))
    {
        fbe_cli_printf("\nFailed to get database supported drive types ... Error: %d\n", status);            
    }
    else
    {
        fbe_cli_printf("Supported Drive Types: 0x%x\n", types);
        fbe_cli_printf("   SSD LE: %s\n", (types&FBE_DATABASE_DRIVE_TYPE_SUPPORTED_LE) ? "yes" : "no");
        fbe_cli_printf("   SSD RI: %s\n", (types&FBE_DATABASE_DRIVE_TYPE_SUPPORTED_RI) ? "yes" : "no");
        fbe_cli_printf("   HDD 520: %s\n", (types&FBE_DATABASE_DRIVE_TYPE_SUPPORTED_520_HDD) ? "yes" : "no");
    }
    return;
}

/*!**********************************************************************
*                      fbe_cli_set_database_additional_supported_drive_type()            
*************************************************************************
*
*  @brief
*            Sets additional supported drive types.
	
*  @param    set_drive_type_p - drive type to either enable or disable
*
*  @return
*            None
*  @author 
*   08/19/2015  Wayne Garrett  - Created 
************************************************************************/
static void fbe_cli_set_database_supported_drive_type(fbe_database_control_set_supported_drive_type_t *set_drive_type_p)
{
    fbe_status_t status;

    status = fbe_api_database_set_additional_supported_drive_types(set_drive_type_p);

    if ((status != FBE_STATUS_OK))
    {
        fbe_cli_printf("\nFailed to set database supported drive types ... Error: %d\n", status);            
    }
}

/*************************
* end file fbe_cli_lib_database.c
*************************/
