#ifndef __FBE_CLI_DATABASE_H__
#define __FBE_CLI_DATABASE_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_database.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of commands of FBE Cli related to database
 *
 * @version
 *   07/12/2011:  Created. Vera Wang
 *
 ***************************************************************************/

void fbe_cli_database(fbe_s32_t argc, fbe_s8_t ** argv);
void fbe_cli_database_hook(fbe_s32_t argc, fbe_s8_t ** argv);
void fbe_cli_system_db_header(fbe_s32_t argc, fbe_s8_t ** argv);
void fbe_cli_cmd_online_planned_drive(int argc , char ** argv);
void fbe_cli_database_debug(int argc , char ** argv);

#define FBE_CLI_DATABASE_USAGE  "\
database or db - Show DATABASE information\n\
usage:  database -getState                              - Get database state.\n\
        database -getTables object_id                   - Get database tables given an object_id and print valid entries.\n\
        database -getTables object_id -verbose          - Get database tables given an object_id and print all entries. \n\
        database -getTransaction                        - Get the database transaction table and display the valid entries. \n\
        database -getTransaction -verbose               - Get the database transaction table and display all the entries.  \n\
        database -get_chassis_replacement_flag          - Get the chassis replacement flag. \n\
        database -set_chassis_replacement_flag  value   - Set the chassis replacement flag. if value = 0, clear the flag; otherwise, set the flag to TRUE\n\
        database -get_fru_descriptor slot_no            - Get the fru descriptor of the slot_no's system drive.  \n\t @This cli can only be used on active SP !@\n\t slot_no: 0-2. If slot_no is not specified or greater than 2, print all fru descriptors\n\
        database -clear_fru_descriptor                  - Clear the fru descriptor of the slot_no's system drive. Be careful,for test only. \n\
        database -set_fru_descriptor                    - Write fru descriptor to DB disks\n\
        database -get_prom_wwnseed                      - Get the Midplane Resume Prom WWN SEED\n\
        database -set_prom_wwnseed wwnseed              - Set the Midplane Resume Prom WWN SEED as the parameter(Hex)\n\
        database -get_system_db_header -verbose         - Get the system db header information.\n\
        database -getStats                              - Get database stats.\n\
        database -get_additional_supported_drive_types  - Get supported drive types.\n\
        database -set_additional_supported_drive_type {-enable|-disable} {<type>}  - Enable/Disable a supported drive type.\n\
                     <type>  supported drive types\n\
                         le:        Low Endurance\n\
                         ri:        Read Intensive\n\
                         520hdd:    520 bps for HDD\n\
                         6g:        6G Max Link Speed\n\
"

#define FBE_CLI_DATABASE_HOOK_USAGE  "\
database_debug_hook or db_hook <operation> <hook_type>\n\
Set/Unset hook in database service\n\
usage:  database_debug_hook -set_debug_hook hook_type              - Set debug hook in database service\n\
            e.g.\n\
            db_hook -set_debug_hook panic_in_active_tran: panic SP when db transaction is in active state\n\
            db_hook -set_debug_hook panic_before_persist: panic SP before db transaction is persisted\n\
        database_debug_hook -remove_debug_hook hook_type              - Remove debug hook in database service\n\
            e.g.\n\
            db_hook -remove_debug_hook panic_in_active_tran: remove panic SP when db transaction is in active state\n\
            db_hook -remove_debug_hook panic_before_persist: remoev panic SP before db transaction is persisted\n\
"


#define FBE_CLI_SYSTEM_DB_HEADER_USAGE  "\
system_db_header \n\
Print/Recover/Set system db header\n\
usage:  system_db_header print                - Print the system db header \n\
        system_db_header recover -magic_num   - Recover the system db header \n\
        system_db_header set <-element_name> <element_value>   -Set elements of system db header \n\
            system_db_header set <-element_name> <element_value> options: \n\
                -magic_number \n\
                -version_header_size \n\
                -persisted_sep_version \n\
                -bvd_interface_object_entry_size \n\
                -pvd_object_entry_size \n\
                -vd_object_entry_size \n\
                -rg_object_entry_size \n\
                -lun_object_entry_size \n\
                -edge_entry_size \n\
                -pvd_user_entry_size \n\
                -rg_user_entry_size \n\
                -lun_user_entry_size \n\
                -power_save_info_global_info_entry_size \n\
                -spare_info_global_info_entry_size \n\
                -generation_info_global_info_entry_size \n\
                -time_threshold_info_global_info_entry_size \n\
                -pvd_nonpaged_metadata_size \n\
                -lun_nonpaged_metadata_size \n\
                -raid_nonpaged_metadata_size \n\
            e.g. \n\
            system_db_header set -pvd_object_entry_size 50  -rg_object_entry_size 100 \n\
"

#define FBE_CLI_DATABASE_DEBUG_USAGE  "\
database_debug or db_dbg - Database debug commands, information\n\
usage:  database_debug -validate <failure action> [caller] Validate the on-disk \n\
                                                  entries against the in-memory entries.\n\
                                 <failure action>: Take this action if there is a validation failure. \n\
                                    e: Generate a error trace\n\
                                    d: Put the SP into degraded mode\n\
                                    p: Put the SP into degraded mode then PANIC SP\n\
                                    c: Correct the error(currently not supported)\n\
                                 [caller]:         Optional value that identifies caller\n\
                                    c: SP Collect process invoked validation\n\
                                    p: Peer shutdown caused validation\n\
                                    u: (default) User requested validation\n\
        database_debug -collect_validate Special version of validate used for spcollect\n\
        database_debug -set_debug_flags (-sdf) <debugs flags>\n\
                                 <debug flags>: Set debug flags in database which control various behavior.\n\
        database_debug -get_debug_flags (-gdf) Returns the current value of the database debug flags.\n\
"

#endif /* end __FBE_CLI_DATABASE_H__ */
/*************************
 * end file fbe_cli_database.h
 *************************/
