/***************************************************************************
 * Copyright (C) EMC Corporation 2009 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_src_base_object_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains cli functions for the base object.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *   05/04/2009:  rfoley - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_transport_trace.h"
#include "fbe_base_object_trace.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_api_base_config_interface.h"



/*! @enum fbe_cli_base_cmd_type  
 *  @brief This is the set of operations that we can perform via the
 *         base object cli command.
 */
typedef enum 
{
    FBE_CLI_BASE_CMD_TYPE_INVALID = 0,
    FBE_CLI_BASE_CMD_TYPE_DISPLAY_INFO,
    FBE_CLI_BASE_CMD_TYPE_SET_STATE,
    FBE_CLI_BASE_CMD_TYPE_SET_TRACE_LEVEL,
    FBE_CLI_BASE_CMD_TYPE_MAX
}
fbe_cli_base_cmd_type;

/*****************************************
 *   LOCAL FUNCTION PROTOTYPES.
 *****************************************/
static char * fbe_cli_base_object_get_command_string(fbe_cli_base_cmd_type cmd_type);
static fbe_cli_command_fn fbe_cli_base_object_get_command_function(fbe_cli_base_cmd_type cmd_type);
static void fbe_cli_base_object_display_table_header(void);
static fbe_status_t fbe_cli_base_object_display_terse(fbe_u32_t object_id, fbe_u32_t context,fbe_package_id_t package_id);
static void fbe_cli_base_object_display_header_info(fbe_cli_base_cmd_type command);

/*!**************************************************************
 * fbe_cli_base_object_get_command_string()
 ****************************************************************
 * @brief
 *  Convert a base object cli command to a string.
 *
 * @param cmd_type - command to decode.               
 *
 * @return - string for this command
 *
 * @author
 *  5/04/2009 - Created. rfoley
 *
 ****************************************************************/

static char * fbe_cli_base_object_get_command_string(fbe_cli_base_cmd_type cmd_type)
{
    /* Convert a command type into the string that represents this command.
     */
    switch (cmd_type)
    {
        case FBE_CLI_BASE_CMD_TYPE_INVALID:
            return("FBE_CLI_BASE_CMD_TYPE_INVALID");
            break;
        case FBE_CLI_BASE_CMD_TYPE_SET_STATE:
            return("FBE_CLI_BASE_CMD_TYPE_SET_STATE");
            break;
        case FBE_CLI_BASE_CMD_TYPE_DISPLAY_INFO:
            return("FBE_CLI_BASE_CMD_TYPE_DISPLAY_INFO");
            break;
        case FBE_CLI_BASE_CMD_TYPE_SET_TRACE_LEVEL:
            return("FBE_CLI_BASE_CMD_TYPE_SET_TRACE_LEVEL");
            break;
        case FBE_CLI_BASE_CMD_TYPE_MAX:
            return("FBE_CLI_BASE_CMD_TYPE_MAX");
            break;
        default:
            fbe_cli_printf("base object cmd_type 0x%x unknown.\n", cmd_type);
            return("UNKNOWN");
    }; /* End of switch */
}
/******************************************
 * end fbe_cli_base_object_get_command_string()
 ******************************************/

/*!**************************************************************
 * fbe_cli_base_object_get_command_function()
 ****************************************************************
 * @brief
 *  Convert a base object cli command to a command function.
 *
 * @param cmd_type - command to decode.               
 *
 * @return - command function for this command type.
 *
 * @author
 *  5/04/2009 - Created. rfoley
 *
 ****************************************************************/

static fbe_cli_command_fn fbe_cli_base_object_get_command_function(fbe_cli_base_cmd_type cmd_type)
{
    /* Convert a command type into the function ptr to execute this command.
     */
    switch (cmd_type)
    {
        case FBE_CLI_BASE_CMD_TYPE_SET_STATE:
            return(fbe_cli_set_lifecycle_state);
            break;
        case FBE_CLI_BASE_CMD_TYPE_DISPLAY_INFO:
            return(fbe_cli_base_object_display_terse);
            break;
        case FBE_CLI_BASE_CMD_TYPE_SET_TRACE_LEVEL:
            return(fbe_cli_set_trace_level);
            break;
        case FBE_CLI_BASE_CMD_TYPE_MAX:
        case FBE_CLI_BASE_CMD_TYPE_INVALID:
        default:
            fbe_cli_printf("base object cli cmd_type 0x%x no valid command function.\n", cmd_type);
            return(NULL);
    }; /* End of switch */
    return(NULL);
}
/******************************************
 * end fbe_cli_base_object_get_command_function()
 ******************************************/

/*!**************************************************************
 * fbe_cli_base_object_display_table_header()
 ****************************************************************
 * @brief
 *  Just trace out some header info for where we are displaying
 *  a table of base objects.
 *
 * @param - None.
 *
 * @return None.   
 *
 * @author
 *  5/04/2009 - Created. rfoley
 *
 ****************************************************************/

static void fbe_cli_base_object_display_table_header(void)
{
    fbe_cli_printf("| Object  | Class   | Class                     |bus|enc |slot| Lifecycle       | Trace             |\n");
    fbe_cli_printf("| ID      | ID      | Name                      |   |    |    | State           | Level             |\n");
    fbe_cli_printf("|---------|---------|---------------------------|---|----|----|-----------------|-------------------|\n");
    return;
}
/******************************************
 * end fbe_cli_base_object_display_table_header()
 ******************************************/

/*!**************************************************************
 * fbe_cli_base_object_display_terse()
 ****************************************************************
 * @brief
 *  This function does a terse display of the base object info
 *  suitable for display in a table.
 *
 * @param object_id - the object to display.
 * @param context - in this case we do not use the context.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  5/04/2009 - Created. rfoley
 *
 ****************************************************************/

static fbe_status_t fbe_cli_base_object_display_terse(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lifecycle_state_t  lifecycle_state;
    fbe_api_trace_level_control_t trace_control;
    fbe_const_class_info_t *class_info_p;
    fbe_port_number_t port_number = FBE_PORT_NUMBER_INVALID;
    fbe_enclosure_number_t enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_enclosure_slot_number_t slot_number = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;

    /* Get our lifecycle state.
     */
    status = fbe_api_get_object_lifecycle_state(object_id, &lifecycle_state, package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("can't get object lifecycle_state for id [%x], status: %x\n",
                      object_id, status);
        return status;
    }

    /* Fetch the trace level.
     */
    trace_control.trace_type = FBE_TRACE_TYPE_OBJECT;
    trace_control.fbe_id = object_id;
    trace_control.trace_level = FBE_TRACE_LEVEL_INVALID;
    status = fbe_api_trace_get_level(&trace_control, package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed to get trace control for object_id: 0x%x status: 0x%x\n",
                      object_id, status);
        return status;
    }

    /* Make sure this is a valid object.
     */
    status = fbe_cli_get_class_info(object_id, package_id, &class_info_p);

    if (status != FBE_STATUS_OK)
    {
        /* Just return the above function displayed an error.
         */
        return status;
    }

    status = fbe_cli_get_bus_enclosure_slot_info(object_id, 
                                                 class_info_p->class_id,
                                                 &port_number,
                                                 &enclosure_number,
                                                 &slot_number,
                                                 package_id);
    if (status != FBE_STATUS_OK)
    {
        /* Just return the above function displayed an error.
         */
        return status;
    }

    /* Display the row of info for this object.
     */
    fbe_cli_printf("|  0x%04x | 0x%04x  | %25s ",
                   object_id,    /* Hex display */
                   class_info_p->class_id,
                   class_info_p->class_name);

    if (port_number == FBE_PORT_NUMBER_INVALID)
    {
        /* If port number is no good, we do not display anything.
         */
        fbe_cli_printf("|   ");
    }
    else if (port_number == FBE_PORT_ENCL_SLOT_NA)
    {
        fbe_cli_printf("|N/A");
    }
    else 
    {
        fbe_cli_printf("| %d ", port_number);
    }
    if (enclosure_number == FBE_ENCLOSURE_NUMBER_INVALID)
    {
        fbe_cli_printf("|    ");
    }
    else if (enclosure_number == FBE_PORT_ENCL_SLOT_NA)
    {
        fbe_cli_printf("|N/A ");
    }
    else
    {
        fbe_cli_printf("| %02d ", enclosure_number);
    }

    if (slot_number == FBE_ENCLOSURE_SLOT_NUMBER_INVALID)
    {
        fbe_cli_printf("|   ");
    }
    else if (slot_number == FBE_PORT_ENCL_SLOT_NA)
    {
        fbe_cli_printf("|N/A");
    }
    else
    {
        fbe_cli_printf("| %02d", slot_number);
    }

    fbe_cli_printf("| 0x%02x %10s | %d %15s |\n",
                   lifecycle_state,    /* Hex display */
                   fbe_base_object_trace_get_state_string(lifecycle_state),
                   trace_control.trace_level,
                   fbe_base_object_trace_get_level_string(trace_control.trace_level));
    return status;
}
/******************************************
 * end fbe_cli_base_object_display_terse()
 ******************************************/

/*!**************************************************************
 * fbe_cli_base_object_display_header_info()
 ****************************************************************
 * @brief
 *  Display information that belongs prior to the body of
 *  information being displayed by this command.
 *
 * @param  command - type of command being executed.               
 *
 * @return none.   
 *
 * @author
 *  5/04/2009 - Created. rfoley
 *
 ****************************************************************/

static void fbe_cli_base_object_display_header_info(fbe_cli_base_cmd_type command)
{
    /* Depending on the command display some initial header info.
     */
    switch(command)
    {
        case FBE_CLI_BASE_CMD_TYPE_DISPLAY_INFO:
            fbe_cli_base_object_display_table_header();
            break;
        default:
            /* Nothing to do for the default case.
             */
            break;
    };
    return;
}
/******************************************
 * end fbe_cli_base_object_display_header_info()
 ******************************************/

/*!**************************************************************
 * fbe_cli_cmd_base_object()
 ****************************************************************
 * @brief
 *   This function will allow viewing and changing state for the base object.
 *   All base object functionality will be able to be accessed for
 *   all objects through this commnad.
 *  
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  5/04/2009 - Created. rfoley
 *
 ****************************************************************/

void fbe_cli_cmd_base_object(int argc , char ** argv)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_cli_base_cmd_type command = FBE_CLI_BASE_CMD_TYPE_DISPLAY_INFO;
	fbe_package_id_t package_id = FBE_PACKAGE_ID_INVALID;

    /* Port, enclosure and slot are not used but necessary to call
     * fbe_cli_execute_command_for_objects(), defaults to all port,
     * enclosure and slot.
     */
	fbe_u32_t port = FBE_API_PHYSICAL_BUS_COUNT;
    fbe_u32_t enclosure = FBE_API_ENCLOSURES_PER_BUS;
    fbe_u32_t slot = FBE_API_ENCLOSURE_SLOTS;

    /* Context for the command.
     */
    fbe_u32_t context = 0;

    /* Determines if we should target this command at all object.
     * If the object id is given, then we will use that single object id as the 
     * target. 
     */
    fbe_bool_t b_target_all = FBE_TRUE;

    /* Determines if we should target a given class. 
     * If so, then class_id has the class to target. 
     */
    fbe_bool_t b_target_class = FBE_FALSE;
    fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;

    fbe_cli_command_fn command_fn = NULL;

    /* If we are not given any args, then we will just do a display
     * of all objects based on the defaults set above for command and 
     * b_target_all. 
     */

    /*
     * Parse the command line.
     */
    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.
             */
            fbe_cli_printf("%s", BASE_OBJECT_USAGE);
            return;
        }
        else if ((strcmp(*argv, "-object") == 0) ||
                 (strcmp(*argv, "-o") == 0))
        {
            /* Filter by one object id.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-object, expected object_id, too few arguments \n");
                return;
            }
            object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
            b_target_all = FBE_FALSE;
        }
        else if ((strcmp(*argv, "-class") == 0) ||
                 (strcmp(*argv, "-c") == 0))
        {
            /* Filter by a specific class.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-class, expected class, too few arguments \n");
                return;
            }
            class_id = (fbe_u32_t)strtoul(*argv, 0, 0);
            b_target_all = FBE_FALSE;
            b_target_class = FBE_TRUE;
        }
        else if ((strcmp(*argv, "-trace_level") == 0) ||
                 (strcmp(*argv, "-t") == 0))
        {
            /* Set the trace level.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-trace_level, expected trace level argument. \n");
                return;
            }
            /* Context is the new trace level to set.
             */
            context = (fbe_u32_t)strtoul(*argv, 0, 0);
            command = FBE_CLI_BASE_CMD_TYPE_SET_TRACE_LEVEL;
        }
        else if ((strcmp(*argv, "-set_state") == 0) ||
                 (strcmp(*argv, "-set") == 0))
        {
            /* change the lifecycle state.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-set_state, expected lifecycle state argument. \n");
                return;
            }
            /* Context is the lifecycle state to set to.
             */
            context = (fbe_u32_t)strtoul(*argv, 0, 0);
            command = FBE_CLI_BASE_CMD_TYPE_SET_STATE;
        }
        else if (strcmp(*argv, "-all") == 0)
        {
            /* Display all objects.
             */
            b_target_all = FBE_TRUE;
        }
        else if ((strcmp(*argv, "-display") == 0) ||
                 (strcmp(*argv, "-d") == 0))
        {
            /* Just display a table of info on these objects.
             */
            command = FBE_CLI_BASE_CMD_TYPE_DISPLAY_INFO;
        }
		else if ((strcmp(*argv, "-package") == 0) ||
                 (strcmp(*argv, "-p") == 0))
        {
            /* Filter by one object id.
             */
            argc--;
            argv++;
            if(argc == 0)
           {
               fbe_cli_error("-package, expected package_id, too few arguments \n");
               return;
           }
           if((strcmp(*argv, "phy") == 0) ||
              (strcmp(*argv, "pp") == 0))
            {
              package_id = FBE_PACKAGE_ID_PHYSICAL;
            }
           else if(strcmp(*argv, "sep") == 0)
            {
              package_id = FBE_PACKAGE_ID_SEP_0;
            }
            else if (strcmp(*argv, "esp") == 0)
            {
              package_id = FBE_PACKAGE_ID_ESP;
            }
        }

        argc--;
        argv++;

    }   /* end of while */
	if(package_id == FBE_PACKAGE_ID_INVALID)
	{
	   fbe_cli_printf("%s", BASE_OBJECT_USAGE);
       return; 
	}
    if (b_target_class == FBE_TRUE)
    {
        fbe_const_class_info_t *class_info_p;

        /* Get the information on this class.
         */
        status = fbe_get_class_by_id(class_id, &class_info_p);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("can't get object class info for class [%x], status: %x\n",
                          class_id, status);
            return;
        }
    }
    else if (b_target_all == FBE_FALSE)
    {
        fbe_const_class_info_t *class_info_p;

        if ((object_id == FBE_OBJECT_ID_INVALID) || ((object_id > FBE_MAX_PHYSICAL_OBJECTS) && package_id == FBE_PACKAGE_ID_PHYSICAL)
			||((object_id > FBE_MAX_SEP_OBJECTS) && package_id == FBE_PACKAGE_ID_SEP_0) || ((object_id > FBE_MAX_ESP_OBJECTS) && package_id == FBE_PACKAGE_ID_ESP))
        {
            /* Just return the above function displayed an error.
             */
            return;
        }

        /* Make sure this is a valid object.
         */
        status = fbe_cli_get_class_info(object_id, package_id, &class_info_p);

        if (status != FBE_STATUS_OK)
        {
            /* Just return the above function displayed an error.
             */
            return;
        }
    } 
    else
    {
        /* The operation will be performed to all objects.
         */
    }

    /* Convert the command to a function.
     */
    command_fn = fbe_cli_base_object_get_command_function(command);

    if (command_fn == NULL)
    {
        fbe_cli_error("No command specified: command %d status: %x\n", command, status);
        fbe_cli_printf("%s", BASE_OBJECT_USAGE);
        return;
    }

    if (b_target_all || b_target_class)
    {
        fbe_cli_base_object_display_header_info(command);

        /* Execute the command against all objects matching this class specifier.
         */
        status = fbe_cli_execute_command_for_objects(class_id,
                                                     package_id,
                                                     port,
                                                     enclosure,
                                                     slot,
                                                     command_fn,
                                                     context /* no context */);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nbase object command %s to all drives failed status: 0x%x.\n",
                           fbe_cli_base_object_get_command_string(command), status);
            return;
        }
    }
    else
    {
        /* Execute command to a single object.
         */
        fbe_cli_base_object_display_header_info(command);

        /* Execute the command against this single object.
         */
        status = (command_fn)(object_id, context, package_id);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nCommand %s failed to object: 0x%x status: 0x%x.\n",
                           fbe_cli_base_object_get_command_string(command), object_id, status);
            return;
        }
    }
    return;
}      
/******************************************
 * end fbe_cli_cmd_base_object()
 ******************************************/
static fbe_u8_t * fbe_cli_cmd_base_config_map_ps_state_to_str(fbe_power_save_state_t state)
{
	switch(state) {
	case FBE_POWER_SAVE_STATE_INVALID:
		return "Invalid";
	case FBE_POWER_SAVE_STATE_NOT_SAVING_POWER:
		return "Not Saving";
	case FBE_POWER_SAVE_STATE_SAVING_POWER:
		return "Saving";
	case FBE_POWER_SAVE_STATE_ENTERING_POWER_SAVE:
		return "Entering";
	case FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE:
		return "Exiting";
	default:
		return "ERROR!";
	}

}

static fbe_u8_t * fbe_cli_cmd_base_config_map_encryption_state_to_str(fbe_base_config_encryption_state_t state)
{
	switch(state) {
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_INVALID:
		return "Invalid";
	case FBE_BASE_CONFIG_ENCRYPTION_STATE_UNENCRYPTED:
		return "Unencrypted";
	case FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_NEW_KEYS_REQUIRED:
		return "New Key Needed";
    case FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_CURRENT_KEYS_REQUIRED:
        return "Curr Key Needed";
	case FBE_BASE_CONFIG_ENCRYPTION_STATE_ENCRYPTION_IN_PROGRESS:
		return "Encryption INP";
	case FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEYING_IN_PROGRESS:
		return "ReKey INP";
    case FBE_BASE_CONFIG_ENCRYPTION_STATE_ENCRYPTED:
        return "Encrypted";
    case FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED:
        return "Keys Present";
    case FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_PUSH_KEYS_DOWNSTREAM:
        return "Push Keys DS";
    case FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE:
        return "Rekey Complete";
	default:
		return "ERROR!";
	}

}

static fbe_u8_t * fbe_cli_cmd_base_config_map_encryption_mode_to_str(fbe_base_config_encryption_mode_t mode)
{
	switch(mode) {
        case FBE_BASE_CONFIG_ENCRYPTION_MODE_INVALID:
		return "INVALID";
	case FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED:
		return "Unencrypted";
    case FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED:
        return "Encrypted";
    case FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS:
        return "Encrypt INP";
    case FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS:
        return "Rekey INP";
	default:
		return "ERROR!";
	}

}

static void fbe_cli_cmd_base_config_display_all_objects(fbe_object_id_t *object_array, fbe_u32_t total_objects, fbe_const_class_info_t* class_info_p)
{
	fbe_u32_t										obj = 0;
	fbe_status_t									status;
	fbe_base_config_object_power_save_info_t		object_ps_info;
	fbe_time_t										hibernation_time = 0;
	fbe_class_id_t									class_id;
	fbe_const_class_info_t *						temp_class_info;
	fbe_u64_t										idle_time;
	fbe_u32_t										sec_since_last_io;
	fbe_u8_t *										ps_state;
    fbe_u8_t *                                      encryption_state_str;
    fbe_u8_t *                                      encryption_mode_str;
	fbe_api_base_config_upstream_object_list_t 		up_edges;
	fbe_api_base_config_downstream_object_list_t	down_edges;
    fbe_status_t                                    lifecycle_status = FBE_STATUS_INVALID;
    fbe_lifecycle_state_t                           lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_base_config_encryption_state_t              encryption_state;
    fbe_base_config_encryption_mode_t               encryption_mode;

	for (obj = 0; obj < total_objects; obj++) {
        
        lifecycle_status = fbe_api_get_object_lifecycle_state(object_array[obj], 
                                                    &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
        if(lifecycle_status != FBE_STATUS_OK)
        {
            fbe_cli_error("Fail to get the Lifecycle State for object id:0x%x, status:0x%x\n",
                           object_array[obj],lifecycle_status);
            continue;
        }
        else if (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
        {
            fbe_cli_printf("|  0x%04X | %-19s| %4llu|   %4llu|   %4d| %s  |%-10s| 0x%08X| %4d| %4d| %-11s| %-11s|\n",
                           object_array[obj],    /* Hex display */
                           "Unknown", (long long)0, (long long)0, 0, "N/A", "N/A", 0, 0, 0,"INVALID", "NONE");
            continue;
        }

		status = fbe_api_get_object_power_save_info(object_array[obj], &object_ps_info);
		if (status != FBE_STATUS_OK) {
			fbe_cli_error("Failed to get object ps info\n");
			return;
		}

		if (object_ps_info.hibernation_time != 0) {
			hibernation_time = fbe_get_elapsed_seconds(object_ps_info.hibernation_time);
			hibernation_time/=60;
		}else{
			hibernation_time = 0;
		}

		/*check the class id*/
		status = fbe_api_get_object_class_id(object_array[obj], &class_id, FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK) {
            fbe_cli_error("Failed to get object class id info\n");
            return;
        }
		if (class_info_p == NULL) {
			status = fbe_get_class_by_id(class_id, &temp_class_info);
			if (status != FBE_STATUS_OK) {
				fbe_cli_error("Failed to get object ps info\n");
				return;
			}
		}else{
			if (class_id == class_info_p->class_id) {
				temp_class_info = class_info_p;
			}else{
				continue;
			}
			
		}

		/*power saving information*/
		if (object_ps_info.power_saving_enabled) {
			idle_time = object_ps_info.configured_idle_time_in_seconds / 60;
			sec_since_last_io = object_ps_info.seconds_since_last_io / 60;
			ps_state = fbe_cli_cmd_base_config_map_ps_state_to_str(object_ps_info.power_save_state);
		}else{
			idle_time = 0;
			sec_since_last_io = 0;
			ps_state = "N/A";
		}

		/*edges information*/
		status = fbe_api_base_config_get_upstream_object_list(object_array[obj], &up_edges);
		if (status != FBE_STATUS_OK) {
			fbe_cli_error("Failed to get upstream info\n");
			return;
		}

		status = fbe_api_base_config_get_downstream_object_list(object_array[obj], &down_edges);
		if (status != FBE_STATUS_OK) {
			fbe_cli_error("Failed to get downstream info\n");
			return;
		}

        status = fbe_api_base_config_get_encryption_state(object_array[obj], &encryption_state);
		if (status != FBE_STATUS_OK) {
			fbe_cli_error("Failed to get encryption state info\n");
			return;
		}
        status = fbe_api_base_config_get_encryption_mode(object_array[obj], &encryption_mode);
		if (status != FBE_STATUS_OK) {
			fbe_cli_error("Failed to get encryption state info\n");
			return;
		}
        encryption_state_str = fbe_cli_cmd_base_config_map_encryption_state_to_str(encryption_state);
        encryption_mode_str = fbe_cli_cmd_base_config_map_encryption_mode_to_str(encryption_mode);
        fbe_cli_printf("|  0x%04X | %-19s| %4llu|   %4llu|   %4d| %s  |%-10s| 0x%08llX| %4d| %4d| %-11s| %-11s|\n",
			object_array[obj],    /* Hex display */
			temp_class_info->class_name,
			(unsigned long long)idle_time,
			(unsigned long long)hibernation_time,
			sec_since_last_io,
			object_ps_info.power_saving_enabled ? "Yes" : "No ",
			ps_state,
			(unsigned long long)object_ps_info.flags,
			up_edges.number_of_upstream_objects,
			down_edges.number_of_downstream_objects,
            encryption_state_str,
            encryption_mode_str);
		
	}

}

static void fbe_cli_cmd_base_config_display_table(fbe_object_id_t object_id,fbe_const_class_info_t* class_info_p)
{
	fbe_u32_t 			total_objects = 0, actual_objects = 0;
	fbe_object_id_t	*	object_array = NULL;
	fbe_status_t		status;

    /*prepare for going over all the objects*/
	if (object_id == FBE_OBJECT_ID_INVALID) {
		if (class_info_p != NULL) {
			status = fbe_api_get_total_objects_of_class(class_info_p->class_id, FBE_PACKAGE_ID_SEP_0, &total_objects );
			if (status != FBE_STATUS_OK) {
				fbe_cli_error("Failed to get total objects of class:%s\n", class_info_p->class_name);
				return;
			}

			status = fbe_api_enumerate_class(class_info_p->class_id, FBE_PACKAGE_ID_SEP_0, &object_array, &total_objects);
			if (status != FBE_STATUS_OK) {
				fbe_cli_error("Failed to enumerate class:%s\n", class_info_p->class_name);
                if(object_array != NULL)
                {
                    fbe_api_free_memory(object_array);
                }
				return;
			}
		}else{
			status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_SEP_0);
			if (status != FBE_STATUS_OK) {
				fbe_cli_error("Failed to get total objects\n");
				return;
			}
			object_array = fbe_api_allocate_memory(sizeof(fbe_object_id_t) * total_objects);
			status = fbe_api_enumerate_objects(object_array, total_objects, &actual_objects, FBE_PACKAGE_ID_SEP_0);
			if (status != FBE_STATUS_OK) {
				fbe_cli_error("Failed to enumerate objects\n");
                if(object_array != NULL)
                {
                    fbe_api_free_memory(object_array);
                }
				return;
			}
		}

		
	}else{
		object_array = fbe_api_allocate_memory(sizeof(fbe_object_id_t));
		fbe_copy_memory(object_array, &object_id, sizeof(fbe_object_id_t));
		total_objects = 1;
	}

	fbe_cli_printf("| Object  | Class              |Idle |Sleep  |minutes|Power |Power     | Flags     |Up.  |Down |Encryption  |Encryption  |\n");
    fbe_cli_printf("| ID      | Name               |Time |Time   |since  |Save  |Save      |           |Edges|Edges|State       |Mode        |\n");
	fbe_cli_printf("|         |                    |mnts.|minutes|last IO|Enable|State     |           |     |     |            |            |\n");
    fbe_cli_printf("|---------|--------------------|-----|-------|-------|------|----------|-----------|-----|-----|------------|------------|\n");

	fbe_cli_cmd_base_config_display_all_objects(object_array, total_objects, class_info_p);

    if(object_array != NULL)
    {
        fbe_api_free_memory(object_array);
    }

}

/*!**************************************************************
 * fbe_cli_cmd_base_config()
 ****************************************************************
 * @brief
 *   This function will allow viewing base config related information.
 *   The user can use the base_object command for other basic operations.
 *  
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  10/22/2010 - Created. sharel
 *
 ****************************************************************/

void fbe_cli_cmd_base_config(int argc , char ** argv)
{
	fbe_status_t 			status = FBE_STATUS_OK;
    fbe_u32_t				object_id = FBE_OBJECT_ID_INVALID;
	fbe_const_class_info_t *class_info_p = NULL;

    /* Determines if we should target this command at all object.
     * If the object id is given, then we will use that single object id as the 
     * target. 
     */
    fbe_bool_t 		b_target_all = FBE_TRUE;

    /* Determines if we should target a given class. 
     * If so, then class_id has the class to target. 
     */
    fbe_bool_t		 b_target_class = FBE_FALSE;
    fbe_class_id_t 	class_id = FBE_CLASS_ID_INVALID;
    fbe_status_t lifecycle_status = FBE_STATUS_INVALID;
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

   
    /* If we are not given any args, then we will just do a display
     * of all objects based on the defaults set above for command and 
     * b_target_all. 
     */

    /*
     * Parse the command line.
     */
    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.
             */
            fbe_cli_printf("%s", BASE_CONFIG_USAGE);
            return;
        }
        else if ((strcmp(*argv, "-object") == 0) ||
                 (strcmp(*argv, "-o") == 0))
        {
            /* Filter by one object id.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-object, expected object_id, too few arguments \n");
                return;
            }
            object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
            b_target_all = FBE_FALSE;
        }
        else if ((strcmp(*argv, "-class") == 0) ||
                 (strcmp(*argv, "-c") == 0))
        {
            /* Filter by a specific class.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-class, expected class, too few arguments \n");
                return;
            }
            class_id = (fbe_u32_t)strtoul(*argv, 0, 0);
            b_target_all = FBE_FALSE;
            b_target_class = FBE_TRUE;
        }

		argc--;
        argv++;
	}

	if (b_target_class == FBE_TRUE)
    {
        
        /* Get the information on this class.
         */
        status = fbe_get_class_by_id(class_id, &class_info_p);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("can't get object class info for class [%x], status: %x\n",
                          class_id, status);
            return;
        }
    }
    else if (b_target_all == FBE_FALSE)
    {
        
        if ((object_id == FBE_OBJECT_ID_INVALID) || (object_id > FBE_MAX_SEP_OBJECTS))
        {
            /* Just return the above function displayed an error.
             */
            return;
        }

        /* Make sure this is a valid object.
         */
        lifecycle_status = fbe_api_get_object_lifecycle_state(object_id, 
                                                    &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
        if(lifecycle_status != FBE_STATUS_OK)
        {
            fbe_cli_error("Fail to get the Lifecycle State for object id:0x%x, status:0x%x\n",
                           object_id,lifecycle_status);
            return;
        }
        else if (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
        {
            fbe_cli_printf("Object ID 0x%x is in SPECIALIZE\n", object_id);
            return;
        }
        
        status = fbe_cli_get_class_info(object_id, FBE_PACKAGE_ID_SEP_0, &class_info_p);
        if (status != FBE_STATUS_OK)
        {
            /* Just return the above function displayed an error.
             */
            return;
        }
    } 
    else
    {
        /* The operation will be performed to all objects.
         */
    }

	fbe_cli_cmd_base_config_display_table(object_id, class_info_p);
}



/*************************
 * end file fbe_cli_src_base_object_cmds.c
 *************************/
