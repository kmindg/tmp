/***************************************************************************
 * Copyright (C) EMC Corporation 2009 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_drive_mgmt_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains cli functions for the drive management object.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *   03/28/2011:  chenl6 - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe_transport_trace.h"
#include "fbe_base_object_trace.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "generic_utils_lib.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"

#define FBE_DMO_CMD_MAX_STR_LEN  64

/*! @enum fbe_cli_dmo_cmd_type  
 *  @brief This is the set of operations that we can perform via the
 *         physical drive cli command.
 */
typedef enum 
{
    /* user access commands */
    FBE_CLI_DMO_CMD_TYPE_INVALID = 0,
    FBE_CLI_DMO_CMD_TYPE_GET_DRIVE_INFO,
    FBE_CLI_DMO_CMD_TYPE_GET_DRIVE_LOG,
    FBE_CLI_DMO_CMD_TYPE_SET_FUEL_GAUGE_ENABLE,
    FBE_CLI_DMO_CMD_TYPE_GET_FUEL_GAUGE,
    FBE_CLI_DMO_CMD_TYPE_GET_FUEL_GAUGE_POLL_INTERVAL,
    FBE_CLI_DMO_CMD_TYPE_SET_FUEL_GAUGE_POLL_INTERVAL,
    FBE_CLI_DMO_CMD_TYPE_DIEH_LOAD_CONFIG,
    FBE_CLI_DMO_CMD_TYPE_DIEH_DISPLAY_CONFIG,
    FBE_CLI_DMO_CMD_TYPE_DIEH_GET_STATS, 
    FBE_CLI_DMO_CMD_TYPE_DIEH_CLEAR_STATS,
    FBE_CLI_DMO_CMD_TYPE_GET_LOGICAL_CRC_ACTION,
    FBE_CLI_DMO_CMD_TYPE_SET_LOGICAL_CRC_ACTION,    
    FBE_CLI_DMO_CMD_TYPE_SET_POLICY_STATE,
    FBE_CLI_DMO_CMD_TYPE_SET_DEBUG_CONTROL,

    FBE_CLI_DMO_CMD_TYPE_MAX
}
fbe_cli_dmo_cmd_type;

#define MAX_ACTION_STR_LEN 64

typedef struct fbe_pdo_logical_error_action_table_s{
    fbe_pdo_logical_error_action_t action;
    char action_str[MAX_ACTION_STR_LEN];
}fbe_pdo_logical_error_action_table_t;


fbe_pdo_logical_error_action_table_t fbe_pdo_logical_error_action_table[] = {
{FBE_PDO_ACTION_KILL_ON_MULTI_BIT_CRC, "KILL_MULTI_CRC"}, 
    {FBE_PDO_ACTION_KILL_ON_SINGLE_BIT_CRC, "KILL_SINGLE_CRC"},
    {FBE_PDO_ACTION_KILL_ON_OTHER_CRC, "KILL_OTHER_CRC"},
    {FBE_PDO_ACTION_LAST, ""},
};


typedef struct fbe_stat_action_flag_str_entry_s{
    fbe_stat_action_t val;
    fbe_u8_t          str[64];
}fbe_stat_action_flag_str_entry_t;

fbe_stat_action_flag_str_entry_t fbe_stat_action_flag_str_table[] =
{
    {FBE_STAT_ACTION_FLAG_FLAG_NO_ACTION, "FBE_STAT_ACTION_FLAG_FLAG_NO_ACTION"},
    {FBE_STAT_ACTION_FLAG_FLAG_RESET, "FBE_STAT_ACTION_FLAG_FLAG_RESET"},
    {FBE_STAT_ACTION_FLAG_FLAG_POWER_CYCLE, "FBE_STAT_ACTION_FLAG_FLAG_POWER_CYCLE"},
    {FBE_STAT_ACTION_FLAG_FLAG_SPINUP, "FBE_STAT_ACTION_FLAG_FLAG_SPINUP"},
    {FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE, "FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE"},
    {FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE_CALL_HOME, "FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE_CALL_HOME"},
    {FBE_STAT_ACTION_FLAG_FLAG_FAIL, "FBE_STAT_ACTION_FLAG_FLAG_FAIL"},
    {FBE_STAT_ACTION_FLAG_FLAG_FAIL_CALL_HOME, "FBE_STAT_ACTION_FLAG_FLAG_FAIL_CALL_HOME"},
    {FBE_STAT_ACTION_FLAG_FLAG_LAST, "Undefined Flag"},
};


/***********************************************
 *   LOCAL FUNCTION PROTOTYPES.
 ***********************************************/
static fbe_status_t fbe_cli_drive_mgmt_get_specific_drive_info(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
static fbe_status_t fbe_cli_drive_mgmt_get_drive_info(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
static fbe_status_t fbe_cli_drive_mgmt_get_drive_log(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
static fbe_status_t fbe_cli_drive_mgmt_set_fuel_gauge_enable(fbe_u32_t enable);
static fbe_status_t fbe_cli_drive_mgmt_get_fuel_gauge(void);
static fbe_status_t fbe_cli_drive_mgmt_get_fuel_gauge_poll_interval(void);
static fbe_status_t fbe_cli_drive_mgmt_set_fuel_gauge_poll_interval(fbe_u32_t interval);
static fbe_status_t fbe_cli_drive_mgmt_dieh_load_config(fbe_u8_t *file);
static fbe_status_t fbe_cli_drive_mgmt_dieh_display_config(void);
static void print_drive_configuration_record(fbe_drive_configuration_handle_t handle, fbe_drive_configuration_record_t *record);
static fbe_status_t fbe_cli_drive_mgmt_dieh_get_stats(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
static fbe_status_t fbe_cli_drive_mgmt_dieh_clear_stats(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
static fbe_status_t fbe_cli_drive_mgmt_dieh_get_crc_actions(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_pdo_logical_error_action_t action);
static fbe_status_t fbe_cli_drive_mgmt_dieh_set_crc_actions(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_pdo_logical_error_action_t action);
static void build_action_str(fbe_pdo_logical_error_action_t action, char * out_str, fbe_u32_t out_buff_len);
static fbe_status_t fbe_cli_drive_mgmt_add_record(void);
static fbe_status_t fbe_cli_drive_mgmt_remove_record(void);
static fbe_status_t fbe_cli_drive_mgmt_mode_select_all(void);
static fbe_status_t fbe_cli_drive_mgmt_set_policy_state(fbe_u8_t *policy_name_str, fbe_bool_t policy_state);
static fbe_status_t fbe_cli_drive_mgmt_get_drive_counts(void);

/*!**************************************************************
 * fbe_cli_cmd_drive_mgmt()
 ****************************************************************
 * @brief
 *   This function allows viewing and changing attributes for drive
 *   management object.
 *  
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  03/28/2011 - Created. chenl6
 *
 ****************************************************************/

void fbe_cli_cmd_drive_mgmt(int argc , char ** argv)
{
    fbe_u32_t bus =       FBE_BUS_ID_INVALID;
    fbe_u32_t enclosure = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_u32_t slot =      FBE_SLOT_NUMBER_INVALID;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_cli_dmo_cmd_type command = FBE_CLI_DMO_CMD_TYPE_INVALID;
    fbe_u8_t *file = NULL;
    fbe_pdo_logical_error_action_t action = FBE_PDO_ACTION_LAST;    
    fbe_u8_t *policy_name_str = NULL;
    fbe_bool_t set_state_on = FBE_FALSE;
    fbe_u32_t enable = 1;
    fbe_u32_t interval = 0;
    fbe_base_object_mgmt_drv_dbg_action_t debug_action = FBE_DRIVE_ACTION_NONE;
    fbe_bool_t debug_action_defer = FBE_FALSE;

    /*
     * Parse the command line.
     */
    while (argc > 0)
    {
        /*todo: replace strcmp with fbe_compare_string*/ 
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.
             */
            fbe_cli_printf("%s", DRIVE_MGMT_USAGE);
            return;
        }
        else if (strcmp(*argv, "-b") == 0)
        {
            /* Get bus argument.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-b, expected bus, too few arguments \n");
                return;
            }
            bus = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if (strcmp(*argv, "-e") == 0)
        {
            /* Get Enclosure argument.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-e, expected enclosure, too few arguments \n");
                return;
            }
            enclosure = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if (strcmp(*argv, "-s") == 0)
        {
            /* Get Slot argument.
             */
            argc--;
            argv++;
            if (argc == 0)
            {
                fbe_cli_error("-s, expected slot, too few arguments \n");
                return;
            }
            slot = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if (strcmp(*argv, "-drive_info") == 0)
        {
            command = FBE_CLI_DMO_CMD_TYPE_GET_DRIVE_INFO;
        }
        else if (strcmp(*argv, "-drivegetlog") == 0)
        {
            command = FBE_CLI_DMO_CMD_TYPE_GET_DRIVE_LOG;
        }
        else if (strcmp(*argv, "-getfuelgauge") == 0)
        {
            command = FBE_CLI_DMO_CMD_TYPE_GET_FUEL_GAUGE;
        }
        else if ((strcmp(*argv, "-getfuelgaugeinterval") == 0) || 
                 (strcmp(*argv, "-getfgi") == 0))
        {
            command = FBE_CLI_DMO_CMD_TYPE_GET_FUEL_GAUGE_POLL_INTERVAL;
        }
        else if (strcmp(*argv, "-fuelgauge") == 0)
        {
            argc--;
            argv++;
            
            if(argc == 0)
            {
                fbe_cli_error("-fuelgauge expected 0 for disabled or 1 for enabled \n");
                return;
            }
            
            /* Context is the state to set to.
             */
            enable = (fbe_u32_t)strtoul(*argv, 0, 0);
            
            if ((enable != 0) && (enable != 1))
            {
                fbe_cli_printf("DMO command invalid argument %s\n", *argv);
                return;
            }
            
            command = FBE_CLI_DMO_CMD_TYPE_SET_FUEL_GAUGE_ENABLE;
        }
        else if ((strcmp(*argv, "-setfuelgaugeinterval") == 0)||
                (strcmp(*argv, "-setfgi") == 0))
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-setfuelgaugeinterval expected a decimal number, too few arguments. \n");
                return;
            }
            
            interval = (fbe_u32_t)strtoul(*argv, 0, 0);
            command = FBE_CLI_DMO_CMD_TYPE_SET_FUEL_GAUGE_POLL_INTERVAL;
        }
        else if (strcmp(*argv, "-dieh_load") == 0)
        {
            command = FBE_CLI_DMO_CMD_TYPE_DIEH_LOAD_CONFIG;

            if(argc > 1 && strncmp(argv[1],"-",1) != 0 )   /* optional file provided */
            {
                argc--;
                argv++;
                file = *argv;
            }
        }
        else if (strcmp(*argv, "-dieh_display") == 0)
        {
            command = FBE_CLI_DMO_CMD_TYPE_DIEH_DISPLAY_CONFIG;
        }
        else if (strcmp(*argv, "-dieh_get_stats") == 0)
        {
            command = FBE_CLI_DMO_CMD_TYPE_DIEH_GET_STATS;
        }
        else if (strcmp(*argv, "-dieh_clear_stats") == 0)
        {
            command = FBE_CLI_DMO_CMD_TYPE_DIEH_CLEAR_STATS;
        }
        else if (strcmp(*argv, "-get_crc_actions") == 0)
        {
            command = FBE_CLI_DMO_CMD_TYPE_GET_LOGICAL_CRC_ACTION;
        }
        else if (strcmp(*argv, "-set_crc_actions") == 0)
        {
            command = FBE_CLI_DMO_CMD_TYPE_SET_LOGICAL_CRC_ACTION;
            /* get arg */
            argc--;
            argv++;
            if (argc > 0)  //optional arg.  
            { 
                action = (fbe_u32_t)strtoul(*argv, 0, 0);
            }
            else
            {
                action = FBE_PDO_ACTION_USE_REGISTRY;
            }
        }
        else if (strcmp(*argv, "-set_policy_state") == 0)
        {
            command = FBE_CLI_DMO_CMD_TYPE_SET_POLICY_STATE;
            /* get arg */
            argc--;
            argv++;
            if (argc == 0)
            { 
                fbe_cli_error("Expected policy name, too few arguments \n");
                fbe_cli_error(" Available policy types names are:  \n");
                fbe_cli_error("     verify_capacity \n");                
                fbe_cli_error("     verify_eq \n");
                fbe_cli_error("     system_qdepth\n \n");
                return;
            }
            policy_name_str = *argv;

            argc--;
            argv++;
            if (argc == 0)
            { 
                fbe_cli_error("Expected new policy state. Options are on and off.\n");
                return;
            }
            if (strcmp(*argv, "on") == 0){
                set_state_on = FBE_TRUE;
            }
            else if (strcmp(*argv, "off") == 0){
                set_state_on = FBE_FALSE;
            }
            else{
                fbe_cli_error("Expected new policy state. Options are on and off.\n");
                return;
            }           
        }
        else if(strcmp(*argv, "-drive_debug_ctrl") == 0)
        {
        	command = FBE_CLI_DMO_CMD_TYPE_SET_DEBUG_CONTROL;
            argc--;
            argv++;
            if (argc == 0)
            {
                fbe_cli_error("Expected debug ctrl command. Options are remove, remove_delay, insert and insert_delay.\n");
                return;
            }

            if (strcmp(*argv, "remove") == 0){
            	debug_action = FBE_DRIVE_ACTION_REMOVE;
            	debug_action_defer = FBE_FALSE;
            }
            else if (strcmp(*argv, "remove_defer") == 0){
            	debug_action = FBE_DRIVE_ACTION_REMOVE;
            	debug_action_defer = FBE_TRUE;
            }
            else if (strcmp(*argv, "insert") == 0){
            	debug_action = FBE_DRIVE_ACTION_INSERT;
            	debug_action_defer = FBE_FALSE;
            }
            else if (strcmp(*argv, "insert_defer") == 0){
            	debug_action = FBE_DRIVE_ACTION_INSERT;
            	debug_action_defer = FBE_TRUE;
            }
            else{
                fbe_cli_error("Expected debug ctrl command. Options are remove, remove_delay, insert and insert_delay..\n");
                return;
            }
        }

        argc--;
        argv++;

    }   /* end of while */

    if ((command == FBE_CLI_DMO_CMD_TYPE_GET_DRIVE_LOG)  ||
        (command == FBE_CLI_DMO_CMD_TYPE_DIEH_CLEAR_STATS))

    {
        if ((bus == FBE_BUS_ID_INVALID) || (enclosure == FBE_ENCLOSURE_NUMBER_INVALID) || (slot == FBE_SLOT_NUMBER_INVALID))
        {
            fbe_cli_error("\nTo specify a single drive input -b port -e enclosure -s slot\n");
            return;
        }
    }

    if (command == FBE_CLI_DMO_CMD_TYPE_GET_DRIVE_INFO)
    {
        status = fbe_cli_drive_mgmt_get_drive_info(bus, enclosure, slot);
    }
    else if (command == FBE_CLI_DMO_CMD_TYPE_GET_DRIVE_LOG)
    {
        status = fbe_cli_drive_mgmt_get_drive_log(bus, enclosure, slot);
    }
    else if (command == FBE_CLI_DMO_CMD_TYPE_SET_FUEL_GAUGE_ENABLE)
    {
        status = fbe_cli_drive_mgmt_set_fuel_gauge_enable(enable);
    }
    else if (command == FBE_CLI_DMO_CMD_TYPE_GET_FUEL_GAUGE)
    {
        status = fbe_cli_drive_mgmt_get_fuel_gauge();
    }
    else if (command == FBE_CLI_DMO_CMD_TYPE_GET_FUEL_GAUGE_POLL_INTERVAL)
    {
        status = fbe_cli_drive_mgmt_get_fuel_gauge_poll_interval();
    }
    else if (command == FBE_CLI_DMO_CMD_TYPE_SET_FUEL_GAUGE_POLL_INTERVAL)
    {
        status = fbe_cli_drive_mgmt_set_fuel_gauge_poll_interval(interval);
    }
    else if (command == FBE_CLI_DMO_CMD_TYPE_DIEH_LOAD_CONFIG)
    {
        status = fbe_cli_drive_mgmt_dieh_load_config(file);
    }
    else if (command == FBE_CLI_DMO_CMD_TYPE_DIEH_DISPLAY_CONFIG)
    {
        status = fbe_cli_drive_mgmt_dieh_display_config();
    }
    else if (command == FBE_CLI_DMO_CMD_TYPE_DIEH_GET_STATS)
    {        
        status = fbe_cli_drive_mgmt_dieh_get_stats(bus, enclosure, slot);
    }
    else if (command == FBE_CLI_DMO_CMD_TYPE_DIEH_CLEAR_STATS)
    {
        status = fbe_cli_drive_mgmt_dieh_clear_stats(bus, enclosure, slot);
    }
    else if (command == FBE_CLI_DMO_CMD_TYPE_GET_LOGICAL_CRC_ACTION)
    {
        status = fbe_cli_drive_mgmt_dieh_get_crc_actions(bus, enclosure, slot, action);
    }
    else if (command == FBE_CLI_DMO_CMD_TYPE_SET_LOGICAL_CRC_ACTION)
    {
        status = fbe_cli_drive_mgmt_dieh_set_crc_actions(bus, enclosure, slot, action);
    }
    else if (command == FBE_CLI_DMO_CMD_TYPE_SET_POLICY_STATE)
    {
        status = fbe_cli_drive_mgmt_set_policy_state(policy_name_str, set_state_on);
    }
    else if (command == FBE_CLI_DMO_CMD_TYPE_SET_DEBUG_CONTROL)
    {
    	status = fbe_api_esp_drive_mgmt_send_debug_control(bus, enclosure, slot, 0, debug_action, debug_action_defer,FBE_TRUE);
    }
    else
    {
        fbe_cli_error("\nNo valid command specified\n");
        fbe_cli_printf("%s", DRIVE_MGMT_USAGE);
        return;
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nCommand failed status: 0x%x.\n", status);
        return;
    }
    return;
}      
/******************************************
 * end fbe_cli_cmd_drive_mgmt()
 ******************************************/

/*!**************************************************************
 * fbe_cli_cmd_drive_mgmt_srvc()
 ****************************************************************
 * @brief
 *   This function allows viewing and changing attributes for drive
 *   management object in service access mode (aka engineering mode).
 *   Use "acc" command to enable this mode.
 *  
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  06/14/2012 - Wayne Garrett : Created
 *
 ****************************************************************/
void fbe_cli_cmd_drive_mgmt_srvc(int argc , char ** argv)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /*
     * Parse the command line.
     */
    while (argc > 0)
    {
        /*todo: replace strcmp with fbe_compare_string*/ 
        if ((strncmp(*argv, "-help", FBE_DMO_CMD_MAX_STR_LEN) == 0) ||
            (strncmp(*argv, "-h", FBE_DMO_CMD_MAX_STR_LEN) == 0))
        {
            /* If they are asking for help, just display help and exit.
             */
            fbe_cli_printf("%s", DRIVE_MGMT_SRVC_USAGE);
            return;
        }
        else if (strncmp(*argv, "-dieh_add_record", FBE_DMO_CMD_MAX_STR_LEN) == 0)
        {
            status = fbe_cli_drive_mgmt_add_record();
            break;
        }
        else if (strncmp(*argv, "-dieh_remove_record", FBE_DMO_CMD_MAX_STR_LEN) == 0)
        {
            status = fbe_cli_drive_mgmt_remove_record();
            break;
        }
        else if (strncmp(*argv, "-mode_select_all", FBE_DMO_CMD_MAX_STR_LEN) == 0)
        {
            status = fbe_cli_drive_mgmt_mode_select_all();
            break;
        }        
        else
        {
            fbe_cli_error("Unrecognized command: %s\n", *argv);
            return;
        }
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nCommand failed status: 0x%x.\n", status);
        fbe_cli_printf("%s", DRIVE_MGMT_SRVC_USAGE);
        return;
    }
}
/******************************************
 * end fbe_cli_cmd_drive_mgmt_srvc()
 ******************************************/

/*!**************************************************************
 * fbe_cli_drive_mgmt_get_specific_drive_info()
 ****************************************************************
 * @brief
 *  This function sends a drive get info command to ESP to get one drive's info.
 *
 * @param bus - bus number.
 * @param enclosure - enclosure position.
 * @param slot - slot number.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  07/03/2012 - Created. Rui Chang
 *
 ****************************************************************/
fbe_status_t fbe_cli_drive_mgmt_get_specific_drive_info(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_esp_drive_mgmt_drive_info_t drive_info={0};
    fbe_physical_drive_information_t physical_drive_info = {0}; 
    fbe_bool_t b_print_physical_info = FBE_FALSE;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t drive_writes_per_day = 0;

    drive_info.location.bus = (fbe_u8_t)(bus & 0xff);
    drive_info.location.enclosure = (fbe_u8_t)(encl & 0xff);
    drive_info.location.slot = (fbe_u8_t)(slot & 0xff);

    status = fbe_api_esp_drive_mgmt_get_drive_info(&drive_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to get drive information.\n", __FUNCTION__);
        return status;
    }

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &object_id);
    if (FBE_STATUS_OK == status)
    {
        status = fbe_api_physical_drive_get_drive_information(object_id, &physical_drive_info, FBE_PACKET_FLAG_NO_ATTRIB);

        if (status == FBE_STATUS_OK)
        {
            drive_writes_per_day = (fbe_u32_t)(physical_drive_info.drive_parameter_flags & FBE_PDO_FLAG_FLASH_DRIVE_WRITES_PER_DAY);
            if((physical_drive_info.drive_parameter_flags & FBE_PDO_FLAG_FLASH_DRIVE) || 
                                     (physical_drive_info.drive_parameter_flags & FBE_PDO_FLAG_MLC_FLASH_DRIVE) ||
                                     (physical_drive_info.drive_RPM == 0))
            {
                b_print_physical_info = FBE_TRUE;
            }
        }
        else
        {
            fbe_cli_error("\nERROR: failed status 0x%x to get physical drive information for object_id: 0x%x\n", status, object_id);
        }
    }
    else
    {
        fbe_cli_error("\nDrive %d_%d_%d does not exist, status=%d\n", bus, encl, slot, status);
    }
    fbe_cli_printf("\n*************** DRIVE INFORMATION *****************\n");
    fbe_cli_printf("DRIVE LOCATION: BUS %d ENCL %d SLOT %d\n", drive_info.location.bus, drive_info.location.enclosure, drive_info.location.slot);
    fbe_cli_printf("INSERTED: %d\n", drive_info.inserted);
    fbe_cli_printf("LOOP A VALID: %d\n", drive_info.loop_a_valid);
    fbe_cli_printf("LOOP B VALID: %d\n", drive_info.loop_b_valid);
    fbe_cli_printf("LOOP A BYPASS ENABLED: %d\n", drive_info.bypass_enabled_loop_a);
    fbe_cli_printf("LOOP B BYPASS ENABLED: %d\n", drive_info.bypass_enabled_loop_b);
    fbe_cli_printf("LOOP A BYPASS REQUESTED: %d\n", drive_info.bypass_requested_loop_a);
    fbe_cli_printf("LOOP B BYPASS REQUESTED: %d\n", drive_info.bypass_requested_loop_b);
    fbe_cli_printf("LIFECYCLE STATE: %d ", drive_info.state);
    fbe_base_object_trace_state(drive_info.state, fbe_cli_trace_func, NULL);
    fbe_cli_printf("\n\n");

    fbe_cli_printf("DRIVE TYPE: %d (%s)\n", drive_info.drive_type, fbe_cli_convert_drive_type_enum_to_string(drive_info.drive_type));
    fbe_cli_printf("GROSS CAPACITY: %llu\n", (unsigned long long)drive_info.gross_capacity);
    fbe_cli_printf("VENDOR: %s\n", drive_info.vendor_id);
    fbe_cli_printf("PRODUCT ID: %s\n", drive_info.product_id);
    fbe_cli_printf("PART NUMBER: %s\n", drive_info.dg_part_number_ascii);
    fbe_cli_printf("TLA: %s\n", drive_info.tla_part_number);
    fbe_cli_printf("REVISION: %s\n", drive_info.product_rev);
    fbe_cli_printf("SERIAL NUMBER: %s\n", drive_info.product_serial_num);   
    fbe_cli_printf("BLOCK SIZE: %d\n", drive_info.block_size);
    fbe_cli_printf("QUEUE DEPTH: %d\n", drive_info.drive_qdepth);
    fbe_cli_printf("DRIVE RPM: %d\n", drive_info.drive_RPM);    
    fbe_cli_printf("SPEED CAPABILITY: %d\n", drive_info.speed_capability);
    fbe_cli_printf("BRIDGE HW REVISION: %s\n", drive_info.bridge_hw_rev);
    fbe_cli_printf("DEATH REASON: %d\n\n", drive_info.death_reason);  
    if(b_print_physical_info == FBE_TRUE)
    {
        fbe_cli_printf("WRITES PER DAY: %d\n", drive_writes_per_day);
    }
    else
    {
        fbe_cli_printf("WRITES PER DAY: N/A\n");
    }
    fbe_cli_printf("SPINDOWN QUALIFIED: %d\n", drive_info.spin_down_qualified);
    fbe_cli_printf("SPINUP TIME IN MINUTES: %d\n", drive_info.spin_up_time_in_minutes);
    fbe_cli_printf("STANDBY TIME IN MINUTES: %d\n", drive_info.stand_by_time_in_minutes);
    fbe_cli_printf("SPINUP COUNT: %d\n\n", drive_info.spin_up_count);
    fbe_cli_printf("LOGICAL STATE: %d\n\n", drive_info.logical_state);
    
    return status;
}
/******************************************
 * end fbe_cli_drive_mgmt_get_specific_drive_info()
 ******************************************/


/*!**************************************************************
 * fbe_cli_drive_mgmt_get_drive_info()
 ****************************************************************
 * @brief
 *  This function gets drive info for multiple drives or one drive from ESP package.
 *
 * @param bus - bus number.
 * @param enclosure - enclosure position.
 * @param slot - slot number.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  03/28/2011 - Created. chenl6
 *  07/03/2012 - Modified. Rui Chang. Add support to get multiple drive info in one time
 *
 ****************************************************************/
fbe_status_t fbe_cli_drive_mgmt_get_drive_info(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t bus_temp=FBE_BUS_ID_INVALID;
    fbe_u32_t encl_temp=FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_u32_t slot_temp=FBE_SLOT_NUMBER_INVALID;
    fbe_u32_t max_slot = 0;
    fbe_device_physical_location_t  location = {0};

    if((bus == FBE_BUS_ID_INVALID) && 
       (encl == FBE_ENCLOSURE_NUMBER_INVALID) &&
       (slot == FBE_SLOT_NUMBER_INVALID))
    {
        fbe_cli_drive_mgmt_get_drive_counts();
    }

    for (bus_temp=0;bus_temp<FBE_API_PHYSICAL_BUS_COUNT;bus_temp++)
    {
        if((bus != FBE_BUS_ID_INVALID) && (bus_temp != bus))
        {
            continue;
        }
        for(encl_temp=0;encl_temp<FBE_API_MAX_ALLOC_ENCL_PER_BUS;encl_temp++)
        {
            if((encl != FBE_ENCLOSURE_NUMBER_INVALID) && (encl_temp != encl))
            {
                continue;
            }

            location.bus = bus_temp;
            location.enclosure= encl_temp;
            fbe_api_esp_encl_mgmt_get_drive_slot_count(&location, &max_slot);
            if (max_slot <= 0)
            {
                continue;
            }
            
            for(slot_temp=0;slot_temp<max_slot;slot_temp++)
            {
                if((slot != FBE_SLOT_NUMBER_INVALID) && (slot_temp != slot))
                {
                    continue;
                }
                status = fbe_cli_drive_mgmt_get_specific_drive_info(bus_temp, encl_temp, slot_temp);
            }
        }
    }

    return status;
}
/******************************************
 * end fbe_cli_drive_mgmt_get_drive_info()
 ******************************************/

/*!**************************************************************
 * fbe_cli_drive_mgmt_get_drive_log()
 ****************************************************************
 * @brief
 *  This function sends a drivegetlog command to DMO.
 *
 * @param bus - bus number.
 * @param enclosure - enclosure position.
 * @param slot - slot number.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  03/28/2011 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t fbe_cli_drive_mgmt_get_drive_log(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_device_physical_location_t location;

    location.bus = (fbe_u8_t)(bus & 0xff);
    location.enclosure = (fbe_u8_t)(encl & 0xff);
    location.slot = (fbe_u8_t)(slot & 0xff);
    status = fbe_api_esp_drive_mgmt_get_drive_log(&location);
    if (status != FBE_STATUS_OK) {
        if(status == FBE_STATUS_NO_OBJECT){
            fbe_cli_error("\n Failed to get drivelog, drive(%d_%d_%d) doesn't exist, status 0x%x\n",
                          location.bus,
                          location.enclosure,
                          location.slot,
                          status);
        }
        if(status == FBE_STATUS_GENERIC_FAILURE)
        {
            fbe_cli_error("\n Failed to get drivelog, Unsupported Drive(%d_%d_%d), status 0x%x\n",
                          location.bus,
                          location.enclosure,
                          location.slot,
                          status);
        }       
        return status;
    }
    
    fbe_cli_printf("Saved drivelog for Drive(%d_%d_%d)\n",
                   location.bus,
                   location.enclosure,
                   location.slot);

    return status;
}
/******************************************
 * end fbe_cli_drive_mgmt_get_drive_log()
 ******************************************/

/*!**************************************************************
 * fbe_cli_drive_mgmt_get_enable_fuel_gauge()
 ****************************************************************
 * @brief
 *  This function sends enable fuel gauge funcation.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  11/05/2012 - Created. Christina Chiang
 *
 ****************************************************************/
fbe_status_t fbe_cli_drive_mgmt_set_fuel_gauge_enable(fbe_u32_t enable)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    status = fbe_api_esp_drive_mgmt_enable_fuel_gauge(enable);

    return status;
}
/******************************************
 * end fbe_cli_drive_mgmt_get_enable_fuel_gauge()
 ******************************************/

/*!**************************************************************
 * fbe_cli_drive_mgmt_get_fuel_gauge()
 ****************************************************************
 * @brief
 *  This function sends a get fuel gauge command to DMO.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  07/28/2012 - Created. Christina Chiang
 *
 ****************************************************************/
fbe_status_t fbe_cli_drive_mgmt_get_fuel_gauge(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t enable;
    
    status = fbe_api_esp_drive_mgmt_get_fuel_gauge(&enable);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to get fuel gauge data.\n", __FUNCTION__);
        return status;
    }
    
    if (!enable)
    {   
          fbe_cli_printf("The fuel gauge is disabled. \n");
    }
    
    fbe_cli_printf("\n\n");

    return status;
}
/******************************************
 * end fbe_cli_drive_mgmt_get_fuel_gauge()
 ******************************************/

/*!**************************************************************
 * fbe_cli_drive_mgmt_get_fuel_gauge_poll_interval()
 ****************************************************************
 * @brief
 *  This function get minimum fuel gauge poll interval from register.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  11/05/2012 - Created. Christina Chiang
 *
 ****************************************************************/
fbe_status_t fbe_cli_drive_mgmt_get_fuel_gauge_poll_interval(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t interval;
    
    status = fbe_api_esp_drive_mgmt_get_fuel_gauge_poll_interval(&interval);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to get poll interval.\n", __FUNCTION__);
        return status;
    }
    
    fbe_cli_printf("Minimum poll interval = %d \n", interval);
    fbe_cli_printf("\n\n");

    return status;
}
/******************************************
 * end fbe_cli_drive_mgmt_get_fuel_gauge_poll_interval()
 ******************************************/

/*!**************************************************************
 * fbe_cli_drive_mgmt_set_fuel_gauge_poll_interval()
 ****************************************************************
 * @brief
 *  This function set a minimum fuel gauge poll interval on memory.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  11/05/2012 - Created. Christina Chiang
 *
 ****************************************************************/
fbe_status_t fbe_cli_drive_mgmt_set_fuel_gauge_poll_interval(fbe_u32_t interval)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    status = fbe_api_esp_drive_mgmt_set_fuel_gauge_poll_interval(interval);

    return status;
}
/******************************************
 * end fbe_cli_drive_mgmt_set_fuel_gauge_poll_interval()
 ******************************************/


/*!**************************************************************
 * fbe_cli_drive_mgmt_dieh_load_config()
 ****************************************************************
 * @brief
 *  This function loads a DIEH (Drive Improved Error Handling) XML
 *  configuration. 
 *
 * @param file - File to load.   If NULL, it will load a default XML
 *  file.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  03/20/2012 : Wayne Garrett -- Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cli_drive_mgmt_dieh_load_config(fbe_u8_t *file)  
{
    fbe_dieh_load_status_t dieh_status;
    fbe_drive_mgmt_dieh_load_config_t  config_info = {0};
   
    if (file != NULL)
    {
        strncpy(config_info.file, file, sizeof(config_info.file)-1);
    }

    dieh_status = fbe_api_esp_drive_mgmt_dieh_load_config_file(&config_info);
    switch (dieh_status)
    {
        case FBE_DMO_THRSH_NO_ERROR:
            fbe_cli_printf("Loaded successfully.\n");
            break;
        case FBE_DMO_THRSH_FILE_INVD_PATH_ERROR:
            fbe_cli_printf("Failed to load: Invalid path.\n");
            break;
        case FBE_DMO_THRSH_FILE_READ_ERROR:
            fbe_cli_printf("Failed to load: Read error.\n");
            break;
        case FBE_DMO_THRSH_XML_ELEMENT_PARSE_ERROR:
            fbe_cli_printf("Failed to load: Parse error.\n");
            break;
        case FBE_DMO_THRSH_MEM_ERROR:
            fbe_cli_printf("Failed to load: Memory Error.\n");
            break;
        case FBE_DMO_THRSH_UPDATE_ALREADY_IN_PROGRESS:
            fbe_cli_printf("Failed to load: Update already in progress.\n");
            break;
        case FBE_DMO_THRSH_ERROR:
            fbe_cli_printf("Failed to load: Generic Error.\n");
            break;
        default:
            fbe_cli_printf("Failed to load: Unknown Error %d\n", dieh_status);
            break;
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_drive_mgmt_dieh_load_config()
 ******************************************/

/*!**************************************************************
 * fbe_cli_drive_mgmt_dieh_display_config()
 ****************************************************************
 * @brief
 *  This function displays a DIEH (Drive Improved Error Handling) 
 *  XML configuration. 
 *
 * @param none
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  03/20/2012 : Wayne Garrett -- Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cli_drive_mgmt_dieh_display_config(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t                                   i = 0;
    fbe_api_drive_config_get_handles_list_t     handles;
    fbe_drive_configuration_record_t            record;
    fbe_stat_action_flag_str_entry_t          * action_str_table = fbe_stat_action_flag_str_table;
    fbe_esp_drive_mgmt_driveconfig_info_t       *driveconfig_info_p;
    
    driveconfig_info_p = fbe_api_allocate_memory(sizeof(fbe_esp_drive_mgmt_driveconfig_info_t));
    if (NULL == driveconfig_info_p)
    {
        fbe_cli_error("%s failed driveconfig alloc.\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(driveconfig_info_p, sizeof(fbe_esp_drive_mgmt_driveconfig_info_t));


    fbe_cli_printf("\n*************** DIEH CONFIGURATION *****************\n");

    status = fbe_api_esp_drive_mgmt_get_drive_configuration_info(driveconfig_info_p);
    fbe_cli_printf("XML File Info\n");
    fbe_cli_printf("   Description: %s\n", driveconfig_info_p->xml_info.description);
    fbe_cli_printf("   Version: %s\n", driveconfig_info_p->xml_info.version);
    fbe_cli_printf("   File: %s\n", driveconfig_info_p->xml_info.filepath);
            
    /* print action table for user's reference */
    fbe_cli_printf("threshold action (a:) lookup table \n");
    while (FBE_STAT_ACTION_FLAG_FLAG_LAST != action_str_table->val)
    {
        fbe_cli_printf("   0x%08x\t%s\n", action_str_table->val, action_str_table->str);
        action_str_table++;
    }


    /* first read all the handles.  Then get and print each record
    */
    
    status = fbe_api_drive_configuration_interface_get_handles(&handles, FBE_API_HANDLE_TYPE_DRIVE);

    if (handles.total_count == 0)
    {
        fbe_cli_printf("No records found\n");
    }
    else
    {
        for (i = 0; i<handles.total_count; i++ )
        {
            status = fbe_api_drive_configuration_interface_get_drive_table_entry(handles.handles_list[i], &record);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Error getting record for handle 0x%x\n", handles.handles_list[i]);
            }
            else
            {
               print_drive_configuration_record(handles.handles_list[i], &record);   
            }
        }
    }

    fbe_api_free_memory(driveconfig_info_p);
    return status;
}
/******************************************
 * end fbe_cli_drive_mgmt_dieh_display_config()
 ******************************************/



static void print_data_stat(fbe_stat_t *stat, fbe_u8_t *desc)
{
    fbe_u32_t i;

    fbe_cli_printf("%s interval=%lld, weight=%d\n", 
                   desc, (long long)stat->interval, (int)stat->weight);

    fbe_cli_printf("\tactions= ");
    for (i=0; i < stat->actions.num; i++)
    {
        fbe_cli_printf("[%d]r:%d/%d a:0x%x,  ",i, stat->actions.entry[i].ratio, stat->actions.entry[i].reactivate_ratio, stat->actions.entry[i].flag);
    }    
    fbe_cli_printf("\n");

    fbe_cli_printf("\tweight except= ");
    for(i=0; i < FBE_STAT_MAX_WEIGHT_EXCEPTION_CODES; i++)
    {
        if (FBE_STAT_WEIGHT_EXCEPTION_INVALID == stat->weight_exceptions[i].type)
        {
            break; /* no more entries. exit loop. */
        }

        switch (stat->weight_exceptions[i].type)
        {
            case FBE_STAT_WEIGHT_EXCEPTION_CC_ERROR:
                fbe_cli_printf("[%d]cc:%02x|%02x|[%02x..%02x],value:%d  ",i, 
                               stat->weight_exceptions[i].u.sense_code.sense_key,
                               stat->weight_exceptions[i].u.sense_code.asc,
                               stat->weight_exceptions[i].u.sense_code.ascq_start,
                               stat->weight_exceptions[i].u.sense_code.ascq_end,
                               stat->weight_exceptions[i].change);
                break;

            case FBE_STAT_WEIGHT_EXCEPTION_PORT_ERROR:
                fbe_cli_printf("[%d]port:%d,value:%d  ", i,
                               stat->weight_exceptions[i].u.port_error,
                               stat->weight_exceptions[i].change);
                break;
    
            case FBE_STAT_WEIGHT_EXCEPTION_OPCODE:
                fbe_cli_printf("[%d]opcode:0x%x,value:%d  ", i,
                               stat->weight_exceptions[i].u.opcode,
                               stat->weight_exceptions[i].change);
                break;
    
            default:
                fbe_cli_printf("[%d]unknown type:%d,value:%d  ", i,
                               stat->weight_exceptions[i].type,
                               stat->weight_exceptions[i].change);
                break;
        }
         
    }
    fbe_cli_printf("\n");

}

/*!**************************************************************
 * print_drive_configuration_record()
 ****************************************************************
 * @brief
 *  This function prints the drive config record to the fbe_cli
 *  console.
 *
 * @param handle - handle for the configuration record
 * @param record - the drive configuration record
 *
 * @return none
 *
 * @author
 *  03/20/2012 : Wayne Garrett -- Created.
 *
 ****************************************************************/
static void print_drive_configuration_record(fbe_drive_configuration_handle_t handle, fbe_drive_configuration_record_t *record)
{
    fbe_u32_t e;

    fbe_cli_printf("handle: 0x%x\n", handle);
    fbe_cli_printf("   drive info: type=%d, vendor=%d, part_num=%s, rev=%s, sn=[%s..%s]\n", 
                   record->drive_info.drive_type, record->drive_info.drive_vendor, record->drive_info.part_number, record->drive_info.fw_revision, 
                   record->drive_info.serial_number_start, record->drive_info.serial_number_end);
    print_data_stat(&record->threshold_info.io_stat,           "   cumulative: ");
    print_data_stat(&record->threshold_info.recovered_stat,    "   recovered:  ");
    print_data_stat(&record->threshold_info.media_stat,        "   media:      ");
    print_data_stat(&record->threshold_info.hardware_stat,     "   hardware:   ");
    print_data_stat(&record->threshold_info.healthCheck_stat,  "   healthCheck:");
    print_data_stat(&record->threshold_info.link_stat,         "   link:       ");
    print_data_stat(&record->threshold_info.data_stat,         "   data:       ");
    fbe_cli_printf("   flags: 0x%x\n", record->threshold_info.record_flags);

    /* print exceptions */
    for (e = 0; e < MAX_EXCEPTION_CODES; e++) 
    {
                if (FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR != record->category_exception_codes[e].type_and_action)
                {
                    fbe_cli_printf("   exception: %02x/%02x..%02x/%02x..%02x, status=%d, action=0x%x \n", 
                                   record->category_exception_codes[e].scsi_fis_union.scsi_code.sense_key,
                                   record->category_exception_codes[e].scsi_fis_union.scsi_code.asc_range_start,
                                   record->category_exception_codes[e].scsi_fis_union.scsi_code.asc_range_end,
                                   record->category_exception_codes[e].scsi_fis_union.scsi_code.ascq_range_start,
                                   record->category_exception_codes[e].scsi_fis_union.scsi_code.ascq_range_end,
                                   record->category_exception_codes[e].status,
                                   record->category_exception_codes[e].type_and_action);
               }
                
    }

}
/******************************************
 * end print_drive_configuration_record()
 ******************************************/


/*!**************************************************************
 * fbe_cli_drive_mgmt_dieh_get_stats()
 ****************************************************************
 * @brief
 *  This function gets the DIEH (Drive Improved Error Handling) 
 *  health info.
 *
 * @param bus, enclosure, slot -  drive location
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  04/13/2012 : Wayne Garrett -- Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cli_drive_mgmt_dieh_get_stats(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)    
{
    fbe_status_t status;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;

    if (bus != FBE_BUS_ID_INVALID && 
        encl != FBE_ENCLOSURE_NUMBER_INVALID && 
        slot != FBE_SLOT_NUMBER_INVALID)
    {   
        status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &object_id);
        
        if (FBE_STATUS_OK != status)
        {
            fbe_cli_printf("Drive %d_%d_%d does not exist, status=%d\n", bus, encl, slot, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        fbe_cli_printf("DIEH STATS: %d_%d_%d \n", bus, encl, slot);
        status = fbe_cli_physical_drive_display_stats(object_id);
        
        if (FBE_STATUS_OK != status)
        {
            fbe_cli_printf("Error displaying dieh status, object id=0x%x, status=%d\n", object_id, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else  /* user didn't provide b-e-s. get stats for all drives */
    {
        for (bus=0; bus < FBE_API_PHYSICAL_BUS_COUNT; bus++)
        {
            for (encl=0; encl < FBE_API_ENCLOSURES_PER_BUS; encl++)
            {
                for (slot=0; slot < FBE_API_ENCLOSURE_SLOTS; slot++)
                {
                    if (fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &object_id) == FBE_STATUS_OK)
                    {
                        if (object_id != FBE_OBJECT_ID_INVALID)
                        {                       
                            fbe_cli_printf("DIEH STATS: %d_%d_%d \n", bus, encl, slot);
                            status = fbe_cli_physical_drive_display_stats(object_id);
                            
                            if (FBE_STATUS_OK != status)
                            {
                                fbe_cli_printf("Error displaying dieh status, object id=0x%x, status=%d\n", object_id, status);                                
                            }
                        }
                    }
                }
            }
        }
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_drive_mgmt_dieh_get_stats()
 ******************************************/


/*!**************************************************************
 * fbe_cli_drive_mgmt_dieh_clear_stats()
 ****************************************************************
 * @brief
 *  This function clears the DIEH (Drive Improved Error Handling) 
 *  error category stats. 
 *
 * @param bus, enclosure, slot -  drive location
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  04/13/2012 : Wayne Garrett -- Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cli_drive_mgmt_dieh_clear_stats(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)  
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     object_id = FBE_OBJECT_ID_INVALID;

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &object_id);

    if (FBE_STATUS_OK != status)
    {
        fbe_cli_printf("Error getting object ID for %d_%d_%d, status=%d\n", bus, encl, slot, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_physical_drive_clear_error_counts(object_id, FBE_PACKET_FLAG_NO_ATTRIB);

    if (FBE_STATUS_OK != status)
    {
        fbe_cli_printf("Error clearing error tags, object id=0x%x, status=%d\n", object_id, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_drive_mgmt_dieh_clear_stats()
 ******************************************/


/*!**************************************************************
 * fbe_cli_drive_mgmt_dieh_get_crc_actions()
 ****************************************************************
 * @brief
 *  This function gets the action to be used by PDO when its
 *  notified of a logical crc error.
 *
 * @param bus, enclosure, slot -  drive location
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  05/07/2012 : Wayne Garrett -- Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cli_drive_mgmt_dieh_get_crc_actions(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_pdo_logical_error_action_t action)  
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     object_id = FBE_OBJECT_ID_INVALID;
    fbe_physical_drive_dieh_stats_t dieh_stats = {0};
    char    action_str[256] = {0};

    fbe_cli_printf("Logical CRC Actions: \n");

    if (bus != FBE_BUS_ID_INVALID && 
        encl != FBE_ENCLOSURE_NUMBER_INVALID && 
        slot != FBE_SLOT_NUMBER_INVALID)
    {  

        status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &object_id);
        
        if (FBE_STATUS_OK != status)
        {
            fbe_cli_printf("Error getting object ID for %d_%d_%d, status=%d\n", bus, encl, slot, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        status = fbe_api_physical_drive_get_dieh_info(object_id, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
        
        if (FBE_STATUS_OK != status)
        {
            fbe_cli_printf("Error getting dieh info for %d_%d_%d, status=%d\n", bus, encl, slot, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }       

        build_action_str(dieh_stats.crc_action, action_str, sizeof(action_str));
        fbe_cli_printf("   %d_%d_%d = %s", bus, encl, slot, action_str);        
    }
    else  /* user didn't provide b-e-s. get stats for all drives */
    {
        for (bus=0; bus < FBE_API_PHYSICAL_BUS_COUNT; bus++)
        {
            for (encl=0; encl < FBE_API_ENCLOSURES_PER_BUS; encl++)
            {
                for (slot=0; slot < FBE_API_ENCLOSURE_SLOTS; slot++)
                {
                    if (fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &object_id) == FBE_STATUS_OK  &&
                        object_id != FBE_OBJECT_ID_INVALID)
                    {                       
                        status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &object_id);
                        
                        if (FBE_STATUS_OK != status)
                        {
                            fbe_cli_printf("Error getting object ID for %d_%d_%d, status=%d\n", bus, encl, slot, status);
                            continue;
                        }
                
                        status = fbe_api_physical_drive_get_dieh_info(object_id, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
                        
                        if (FBE_STATUS_OK != status)
                        {
                            fbe_cli_printf("Error getting dieh info for %d_%d_%d, status=%d\n", bus, encl, slot, status);
                            continue;
                        }       
                
                        build_action_str(dieh_stats.crc_action, action_str, sizeof(action_str));
                        fbe_cli_printf("   %d_%d_%d = %s\n", bus, encl, slot, action_str); 
                    }                    
                }
            }
        }
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_drive_mgmt_dieh_get_crc_actions()
 ******************************************/

/*!**************************************************************
 * build_action_str()
 ****************************************************************
 * @brief
 *  Builds the fcli string for printing action values
 *
 * @param action -  (in) action bit map
 * @param out_str - (out) output str to be returned
 * @param out_buff_len - (in) size of out_str buffer
 *
 * @return void
 *
 * @author
 *  05/07/2012 : Wayne Garrett -- Created.
 *
 ****************************************************************/
static void build_action_str(fbe_pdo_logical_error_action_t action, char * out_str, fbe_u32_t out_buff_len)
{
    fbe_pdo_logical_error_action_table_t *item = fbe_pdo_logical_error_action_table;
    size_t len = 0;
    char tempstr[256] = {0};        

    while (item->action != FBE_PDO_ACTION_LAST)   
    {
        if (action & item->action)   /* if bit set, then add to string */
        {
            /* do we have enough space to log msg.  */
            if (sizeof(tempstr)-len < strlen(item->action_str)+2) // +2 is for concat of '|'
            {
                fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Not enough space in buffer to log flags.\n", __FUNCTION__);
                break;  /*break and log what we have*/
            }
            strncat(tempstr, item->action_str, MAX_ACTION_STR_LEN);
            strncat(tempstr, "|", 2);
            len = strlen(tempstr);
        }
        item++;
    }

    fbe_sprintf(out_str, out_buff_len, "0x%x (%s)\n", action, tempstr);
}
 
 
/*!**************************************************************
 * fbe_cli_drive_mgmt_dieh_set_crc_actions()
 ****************************************************************
 * @brief
 *  This function set the action to be used by PDO when its
 *  notified of a logical crc error.
 *
 * @param bus, enclosure, slot -  drive location
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  05/07/2012 : Wayne Garrett -- Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cli_drive_mgmt_dieh_set_crc_actions(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_pdo_logical_error_action_t action)  
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     object_id = FBE_OBJECT_ID_INVALID;

    if (bus != FBE_BUS_ID_INVALID && 
        encl != FBE_ENCLOSURE_NUMBER_INVALID && 
        slot != FBE_SLOT_NUMBER_INVALID)
    {  
        status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &object_id);
        
        if (FBE_STATUS_OK != status)
        {
            fbe_cli_printf("Error getting object ID for %d_%d_%d, status=%d\n", bus, encl, slot, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        status = fbe_api_physical_drive_set_crc_action(object_id, action);
        
        if (FBE_STATUS_OK != status)
        {
            fbe_cli_printf("Error sending set_crc_action for %d_%d_%d, status=%d\n", bus, encl, slot, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }       
    }
    else  /* user didn't provide b-e-s. set stats for all drives */
    {
        fbe_api_esp_drive_mgmt_set_crc_actions(&action);        
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_drive_mgmt_dieh_set_crc_actions()
 ******************************************/


/*!**************************************************************
 * fbe_cli_drive_mgmt_add_record()
 ****************************************************************
 * @brief
 *  Add a DIEH config record.  Gets details from user.
 *
 * @param none
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  06/14/2012 :  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cli_drive_mgmt_add_record(void)
{
    fbe_drive_configuration_record_t      record = {0};
    fbe_drive_configuration_handle_t      handle;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dcs_dieh_status_t dcs_status;


    /*todo: get values from user*/
    //fbe_shim_utils_get_table_record_arguments(record, default_record, TRUE);
  

    /* hack to quickly change values.  get this from user */
    handle = 0;
    while (1)
    {    
       status = fbe_api_drive_configuration_interface_get_drive_table_entry(handle, &record);
       if (FBE_STATUS_OK != status)
       {
           return FBE_STATUS_GENERIC_FAILURE;
       }

       /* modify seagate record */
       if (FBE_DRIVE_TYPE_SAS == record.drive_info.drive_type &&
           FBE_DRIVE_VENDOR_INVALID == record.drive_info.drive_vendor )
       {
           break; /* found it */
       }
       handle++;
    }

    record.threshold_info.media_stat.actions.entry[0].ratio = 30; 
    record.threshold_info.media_stat.actions.entry[0].reactivate_ratio = FBE_U32_MAX;
    record.threshold_info.media_stat.actions.entry[0].flag = FBE_STAT_ACTION_FLAG_FLAG_RESET;

    record.threshold_info.media_stat.actions.entry[1].ratio = 60;
    record.threshold_info.media_stat.actions.entry[1].reactivate_ratio = FBE_U32_MAX;
    record.threshold_info.media_stat.actions.entry[1].flag = FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE;

    record.threshold_info.media_stat.actions.entry[2].ratio = 90;
    record.threshold_info.media_stat.actions.entry[2].reactivate_ratio = FBE_U32_MAX;
    record.threshold_info.media_stat.actions.entry[2].flag = FBE_STAT_ACTION_FLAG_FLAG_RESET;

    record.threshold_info.media_stat.actions.entry[3].ratio = 100;
    record.threshold_info.media_stat.actions.entry[3].reactivate_ratio = FBE_U32_MAX;
    record.threshold_info.media_stat.actions.entry[3].flag = FBE_STAT_ACTION_FLAG_FLAG_FAIL;

    record.threshold_info.media_stat.actions.num = 4;


    dcs_status = fbe_api_drive_configuration_interface_start_update();

    if ( dcs_status != FBE_DCS_DIEH_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, failed to start_update, status:%d\n", __FUNCTION__, dcs_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_api_drive_configuration_interface_add_drive_table_entry(&record, &handle);

    fbe_api_drive_configuration_interface_end_update();

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_cli_drive_mgmt_remove_record()
 ****************************************************************
 * @brief
 *  Remove a DIEH config record.  Gets details from user.
 *
 * @param none
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  06/14/2012 :  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cli_drive_mgmt_remove_record(void)
{
    /*TODO: implement.  currently there is a design issue 
      with removing records.   There is an assumption in the
      code that if there N records, then records will be located
      at positions 0..N-1 in the array.  Removing a record will probably
      have some undesirable effects. */
    return FBE_STATUS_GENERIC_FAILURE;
}


/*!**************************************************************
 * fbe_cli_drive_mgmt_mode_select_all()
 ****************************************************************
 * @brief
 *  Sends mode select to all drives.  The system drives will be done one at a
 *  time, then the remaining will be issued in parallel.
 *
 * @param none
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  03/29/2013 :  Wayne Garrett - Created.
 * 
 * @note This function is only intended for engineering testing.
 ****************************************************************/
static fbe_status_t fbe_cli_drive_mgmt_mode_select_all(void)
{
    fbe_u32_t bus, encl, slot;
    fbe_object_id_t object_id;

    fbe_cli_printf("Command may take a few seconds to execute...\n");

    /* first, send reset to system drives one at a time to avoid an ntmirror panic.*/
    for (slot=0; slot<4; slot++)
    {   
        if (fbe_api_get_physical_drive_object_id_by_location(0, 0, slot, &object_id) == FBE_STATUS_OK)
        {
            if (object_id != FBE_OBJECT_ID_INVALID)
            {                       
                fbe_cli_printf("mode select %d_%d_%d \n", 0, 0, slot);
                fbe_api_physical_drive_mode_select(object_id);
                fbe_api_sleep(1000);    
            }
        }
    }

    /* next, send reset to all drives except for system drives. */
    for (bus=0; bus < FBE_API_PHYSICAL_BUS_COUNT; bus++)
    {
        for (encl=0; encl < FBE_API_ENCLOSURES_PER_BUS; encl++)
        {
            for (slot=0; slot < FBE_API_ENCLOSURE_SLOTS; slot++)
            {
                if (0 == bus && 
                    0 == encl && 
                    slot < 4)
                {
                    /* skip system drives */
                    continue;
                }

                if (fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &object_id) == FBE_STATUS_OK)
                {
                    if (object_id != FBE_OBJECT_ID_INVALID)
                    {                       
                        fbe_cli_printf("resetting %d_%d_%d \n", bus, encl, slot);
                        fbe_api_physical_drive_mode_select(object_id);
                    }
                }
            }
        }
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_cli_drive_mgmt_set_policy_state()
 ****************************************************************
 * @brief
 *  This function enables/disables drive mgmt policies. 
 *  NOTE: This is a temporary option for testing purposes.
 * @param none
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  01/15/2013 :  Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cli_drive_mgmt_set_policy_state(fbe_u8_t *policy_name_str, fbe_bool_t policy_state)
{
    fbe_status_t                 status = FBE_STATUS_OK;
    fbe_drive_mgmt_policy_t      policy;
    fbe_drive_mgmt_policy_id_t   policy_id;

    if (policy_name_str == NULL){
        fbe_cli_printf("%s Error. Invalid parameter.\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (strcmp(policy_name_str, "verify_capacity") == 0){
        policy_id = FBE_DRIVE_MGMT_POLICY_VERIFY_CAPACITY;
    }
    else if (strcmp(policy_name_str, "verify_eq") == 0){
        policy_id = FBE_DRIVE_MGMT_POLICY_VERIFY_ENHANCED_QUEUING;
    }
    else if (strcmp(policy_name_str, "system_qdepth") == 0){
        policy_id = FBE_DRIVE_MGMT_POLICY_SET_SYSTEM_DRIVE_IO_MAX;
    }
    else{
        fbe_cli_printf("%s Error Unknown policy name.\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    policy.id =     policy_id;
    policy.value =  (policy_state ? FBE_DRIVE_MGMT_POLICY_ON : FBE_DRIVE_MGMT_POLICY_OFF);

    status = fbe_api_esp_drive_mgmt_change_policy(&policy);
    if (status == FBE_STATUS_OK){
        fbe_cli_printf("\n %s Set policy  %s  state to 0x%x.\n",__FUNCTION__,policy_name_str,policy_state);        
    }else{
        fbe_cli_printf("\n %s Error setting policy. Status: 0x%x.\n",__FUNCTION__, status);
    }

    return status;

}

/*!**************************************************************
 * fbe_cli_drive_mgmt_get_drive_counts()
 ****************************************************************
 * @brief
 *  This function displays the system level drive counts. 
 *
 * @param none
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  14-May-2013 : PHE  -- Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cli_drive_mgmt_get_drive_counts(void)
{
    fbe_status_t       status = FBE_STATUS_OK;
    fbe_u32_t          maxPlatformDriveCount = 0;
    fbe_u32_t          totalDriveCountOnSystem = 0;

    fbe_cli_printf("\n*************** DRIVE COUNTS *****************\n");

    status = fbe_api_esp_drive_mgmt_get_max_platform_drive_count(&maxPlatformDriveCount);
    
    if(status != FBE_STATUS_OK) 
    {
        fbe_cli_printf("Failed to get maxPlatformDriveCount, status 0x%X.\n", status);
    }
    else
    {
        fbe_cli_printf("maxPlatformDriveCount : %d.\n", maxPlatformDriveCount);
    }

    status = fbe_api_esp_drive_mgmt_get_total_drives_count(&totalDriveCountOnSystem);
    
    if(status != FBE_STATUS_OK) 
    {
        fbe_cli_printf("Failed to get totalDriveCountOnSystem, status 0x%X.\n", status);
    }
    else
    {
        fbe_cli_printf("totalDriveCountOnSystem : %d.\n", totalDriveCountOnSystem);
    }
   
    return FBE_STATUS_OK;
}

/*************************
 * end file fbe_cli_lib_drive_mgmt_cmds.c
 *************************/
