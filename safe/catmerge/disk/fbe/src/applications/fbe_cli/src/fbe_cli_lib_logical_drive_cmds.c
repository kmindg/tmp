/***************************************************************************
 * Copyright (C) EMC Corporation 2009 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_src_logical_drive_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains cli functions for the logical drive object.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *   04/24/2009:  rfoley - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_logical_drive_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_transport_trace.h"
#include "fbe_base_object_trace.h"
#include "fbe_block_transport_trace.h"
#include "fbe_logical_drive_trace.h"
#include "fbe/fbe_api_trace.h"
#include "fbe/fbe_api_common.h"


/*! @enum fbe_cli_ldo_cmd_type  
 *  @brief This is the set of operations that we can perform via the
 *         logical drive cli command.
 */
typedef enum 
{
    FBE_CLI_LDO_CMD_TYPE_INVALID = 0,
    FBE_CLI_LDO_CMD_TYPE_DISPLAY_VERBOSE_INFO,
    FBE_CLI_LDO_CMD_TYPE_DISPLAY_TERSE_INFO,
    FBE_CLI_LDO_CMD_TYPE_SET_STATE,
    FBE_CLI_LDO_CMD_TYPE_SET_TRACE_LEVEL,
    FBE_CLI_LDO_CMD_TYPE_MAX
}
fbe_cli_ldo_cmd_type;

/***********************************************
 *   LOCAL FUNCTION PROTOTYPES.
 ***********************************************/
static char * fbe_cli_logical_drive_get_command_string(fbe_cli_ldo_cmd_type cmd_type);
static fbe_cli_command_fn fbe_cli_logical_drive_get_command_function(fbe_cli_ldo_cmd_type cmd_type);
static fbe_status_t fbe_cli_logical_drive_display_verbose(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static void fbe_cli_logical_drive_display_table_header(void);
static fbe_status_t fbe_cli_logical_drive_display_terse(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_logical_drive_get_object_id(fbe_u32_t *object_handle_p,
                                                        fbe_u32_t port, 
                                                        fbe_u32_t enclosure, 
                                                        fbe_u32_t slot);
static void fbe_cli_logical_drive_display_header_info(fbe_cli_ldo_cmd_type command);

/*!**************************************************************
 * fbe_cli_logical_drive_get_command_string()
 ****************************************************************
 * @brief
 *  Convert a logical drive cli command to a string.
 *
 * @param cmd_type - command to decode.               
 *
 * @return - string for this command
 *
 * @author
 *  4/29/2009 - Created. rfoley
 *
 ****************************************************************/

static char * fbe_cli_logical_drive_get_command_string(fbe_cli_ldo_cmd_type cmd_type)
{
    /* Convert a command type into the string that represents this command.
     */
    switch (cmd_type)
    {
        case FBE_CLI_LDO_CMD_TYPE_INVALID:
            return("FBE_CLI_LDO_CMD_TYPE_INVALID");
            break;
        case FBE_CLI_LDO_CMD_TYPE_DISPLAY_VERBOSE_INFO:
            return("FBE_CLI_LDO_CMD_TYPE_DISPLAY_VERBOSE_INFO");
            break;
        case FBE_CLI_LDO_CMD_TYPE_SET_STATE:
            return("FBE_CLI_LDO_CMD_TYPE_SET_STATE");
            break;
        case FBE_CLI_LDO_CMD_TYPE_DISPLAY_TERSE_INFO:
            return("FBE_CLI_LDO_CMD_TYPE_DISPLAY_TERSE_INFO");
            break;
        case FBE_CLI_LDO_CMD_TYPE_SET_TRACE_LEVEL:
            return("FBE_CLI_LDO_CMD_TYPE_SET_TRACE_LEVEL");
            break;
        case FBE_CLI_LDO_CMD_TYPE_MAX:
            return("FBE_CLI_LDO_CMD_TYPE_MAX");
            break;
        default:
            fbe_cli_printf("logical drive cmd_type 0x%x unknown.\n", cmd_type);
            return("UNKNOWN");
    }; /* End of switch */
    return("UNKNOWN");
}
/******************************************
 * end fbe_cli_logical_drive_get_command_string()
 ******************************************/

/*!**************************************************************
 * fbe_cli_logical_drive_get_command_function()
 ****************************************************************
 * @brief
 *  Convert a logical drive cli command to a command function.
 *
 * @param cmd_type - command to decode.               
 *
 * @return - command function for this command type.
 *
 * @author
 *  4/30/2009 - Created. rfoley
 *
 ****************************************************************/

static fbe_cli_command_fn fbe_cli_logical_drive_get_command_function(fbe_cli_ldo_cmd_type cmd_type)
{
    /* Convert a command type into the function ptr to execute this command.
     */
    switch (cmd_type)
    {
        case FBE_CLI_LDO_CMD_TYPE_DISPLAY_VERBOSE_INFO:
            return(fbe_cli_logical_drive_display_verbose);
            break;
        case FBE_CLI_LDO_CMD_TYPE_SET_STATE:
            return(fbe_cli_set_lifecycle_state);
            break;
        case FBE_CLI_LDO_CMD_TYPE_DISPLAY_TERSE_INFO:
            return(fbe_cli_logical_drive_display_terse);
            break;
        case FBE_CLI_LDO_CMD_TYPE_SET_TRACE_LEVEL:
            return(fbe_cli_set_trace_level);
            break;
        case FBE_CLI_LDO_CMD_TYPE_MAX:
        case FBE_CLI_LDO_CMD_TYPE_INVALID:
        default:
            fbe_cli_printf("logical drive cmd_type 0x%x no valid command function.\n", cmd_type);
            return(NULL);
    }; /* End of switch */
    return(NULL);
}
/******************************************
 * end fbe_cli_logical_drive_get_command_function()
 ******************************************/

/*!**************************************************************
 * fbe_cli_logical_drive_display_verbose()
 ****************************************************************
 * @brief
 *  Display all info on the logical drive with this object id.
 *
 * @param object_id - id of logical drive to display.  
 * @param context - Not used but this is needed to
 *                  for this function to be of type fbe_cli_command_fn().
 *
 * @return fbe_status_t   
 *
 * @author
 *  4/30/2009 - Created. rfoley
 *
 ****************************************************************/

static fbe_status_t fbe_cli_logical_drive_display_verbose(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_lifecycle_state_t                   lifecycle_state;
    fbe_logical_drive_attributes_t          attributes;
    fbe_api_get_block_edge_info_t           block_edge_info;
    fbe_api_get_discovery_edge_info_t       discovery_edge_info;
    fbe_port_number_t                       port = FBE_PORT_NUMBER_INVALID;
    fbe_enclosure_number_t                  enc  = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_enclosure_slot_number_t             slot = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;

    fbe_cli_printf("----------------------------------------------------------------------------------------\n");
    fbe_cli_printf("Object id:                0x%x\n", object_id);

    /* Get our lifecycle state.
     */
    status = fbe_api_get_object_lifecycle_state(object_id, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("can't get object lifecycle_state for id [%x], status: %x\n",
                      object_id, status);
    }
    fbe_cli_printf("Lifecycle State:          0x%x ", lifecycle_state);
    fbe_base_object_trace_state(lifecycle_state,
                                fbe_cli_trace_func,
                                NULL);
    fbe_cli_printf("\n");

    /* Get the slot information.
     */
    status = fbe_cli_get_bus_enclosure_slot_info(object_id, 
                                                 FBE_CLASS_ID_LOGICAL_DRIVE,
                                                 &port,
                                                 &enc,
                                                 &slot,
                                                 package_id);
    if (status != FBE_STATUS_OK)
    {
        /* Just return the above function displayed an error.
         */
        return status;
    }
    fbe_cli_printf("Port:                     0x%x (%d)\n", port, port);
    fbe_cli_printf("Enclosure:                0x%x (%d)\n", enc, enc);
    fbe_cli_printf("Slot:                     0x%x (%d)\n", slot, slot);

    /* Get and display the logical drive attributes.
     */
    status = fbe_api_logical_drive_get_attributes(object_id, &attributes);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nERROR: failed status 0x%x to get logical drive object attributes for object_id: 0x%x\n",
                      status, object_id);
        return status;
    }

    /* Display this structure.
     */
    fbe_cli_printf("\n");
    fbe_logical_drive_trace_attributes(&attributes, fbe_cli_trace_func, NULL);

    /* Get the block edge info and display it.
     */
    status = fbe_api_get_block_edge_info(object_id, 0, &block_edge_info, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nERROR: failed to get logical drive object edge info for object_id: 0x%x\n",
                      object_id);
        return status;
    }
    /* Trace out the edge information.
     */
    fbe_cli_printf("\n");
    fbe_api_trace_block_edge_info(&block_edge_info, fbe_cli_trace_func, NULL);

    /* Get the block edge info and display it.
     */
    status = fbe_api_get_discovery_edge_info(object_id, &discovery_edge_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nERROR: failed to get logical drive object edge info for object_id: 0x%x\n",
                      object_id);
        return status;
    }
    /* Trace out the edge information.
     */
    fbe_cli_printf("\n");
    fbe_api_trace_discovery_edge_info(&discovery_edge_info, fbe_cli_trace_func, NULL);

    fbe_cli_printf("----------------------------------------------------------------------------------------\n");
    fbe_cli_printf("\n");
    return  status;
}
/******************************************
 * end fbe_cli_logical_drive_display_verbose()
 ******************************************/

/*!**************************************************************
 * fbe_cli_logical_drive_display_table_header()
 ****************************************************************
 * @brief
 *  Just trace out some header info for where we are displaying
 *  a table of logical drives.
 *
 * @param - None.
 *
 * @return None.   
 *
 * @author
 *  4/30/2009 - Created. rfoley
 *
 ****************************************************************/

static void fbe_cli_logical_drive_display_table_header(void)
{
    fbe_cli_printf("| Object  |bus_    | Imported   | Imported  | Lifecycle       | Trace             |\n");
    fbe_cli_printf("| ID      |enc_slot| Capacity   | Block Size| State           | Level             |\n");
    fbe_cli_printf("|---------|--------|------------|-----------|-----------------|-------------------|\n");
    return;
}
/******************************************
 * end fbe_cli_logical_drive_display_table_header()
 ******************************************/

/*!**************************************************************
 * fbe_cli_logical_drive_display_terse()
 ****************************************************************
 * @brief
 *  This function does a terse display of the logical drive info
 *  suitable for display in a table.
 *
 * @param object_id - the object to display.
 * @param context - in this case we do not use the context.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  4/30/2009 - Created. rfoley
 *
 ****************************************************************/

static fbe_status_t fbe_cli_logical_drive_display_terse(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lifecycle_state_t  lifecycle_state;
    fbe_logical_drive_attributes_t attributes;
    fbe_api_trace_level_control_t trace_control;
    fbe_port_number_t port_number = FBE_PORT_NUMBER_INVALID;
    fbe_enclosure_number_t enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_enclosure_slot_number_t slot_number = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;

    /* Get our lifecycle state.
     */
    status = fbe_api_get_object_lifecycle_state(object_id, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("can't get object lifecycle_state for id [%x], status: %x\n",
                      object_id, status);
        return status;
    }

    /* Get the slot information.
     */
    status = fbe_cli_get_bus_enclosure_slot_info(object_id, 
                                                 FBE_CLASS_ID_LOGICAL_DRIVE,
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

    /* Get the logical drive attributes.
     */
    status = fbe_api_logical_drive_get_attributes(object_id, &attributes);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nERROR: failed to get logical drive object attributes for object_id: 0x%x status: 0x%x\n",
                      object_id, status);
        return status;
    }

    /* Fetch the trace level.
     */
    trace_control.trace_type = FBE_TRACE_TYPE_OBJECT;
    trace_control.fbe_id = object_id;
    trace_control.trace_level = FBE_TRACE_LEVEL_INVALID;
    status = fbe_api_trace_get_level(&trace_control, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nERROR: failed to get trace control for object_id: 0x%x status: 0x%x\n",
                      object_id, status);
        return status;
    }

    /* Display the row of info for this object.
     */
    fbe_cli_printf("|  0x%04x | %1d_%d_%-2d | 0x%08llx | %04d      | 0x%02x %10s | %d %15s |\n",
                   object_id,    /* Hex display */
                   port_number, enclosure_number, slot_number,
                   (unsigned long long)attributes.imported_capacity,
                   attributes.imported_block_size,
                   lifecycle_state,    /* Hex display */
                   fbe_base_object_trace_get_state_string(lifecycle_state),
                   trace_control.trace_level,
                   fbe_base_object_trace_get_level_string(trace_control.trace_level));
    return status;
}
/******************************************
 * end fbe_cli_logical_drive_display_terse()
 ******************************************/

/*!**************************************************************
 * fbe_cli_logical_drive_get_object_id()
 ****************************************************************
 * @brief
 *  Fetch the object id for the logical drive at this particular
 *  port, enclosure, and slot.
 *
 * @param port - Port of this logical drive.  
 * @param enclosure - Enclosure of this logical drive.
 * @param slot - Slot # of this logical drive.
 *
 * @return fbe_status_t - Status of the operation.
 *
 * @author
 *  4/30/2009 - Created. rfoley
 *
 ****************************************************************/

static fbe_status_t fbe_cli_logical_drive_get_object_id(fbe_u32_t *object_handle_p,
                                                        fbe_u32_t port, 
                                                        fbe_u32_t enclosure, 
                                                        fbe_u32_t slot)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* We are targeting a single logical drive. Make sure the logical drive target,
     * port, enclosure, slot is valid. 
     */
    if ((port >= 8) ||
        (enclosure >= 8) ||
        (slot >= 15))
    {
        fbe_cli_error("\nTo specify a single logical drive input -p port -e enclosure -s slot\n");
        fbe_cli_printf("To specify all logical drives, select -all\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the object ID for the specified logical drive.
     */
    status = fbe_api_get_logical_drive_object_id_by_location(port, enclosure, slot, object_handle_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nStatus: %d failed to get logical drive object id for port: %d enclosure: %d slot: %d\n",
                      status, port, enclosure, slot);
        return status;
    }

    /* Handle case where no object id was returned.
     */
    if (*object_handle_p == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_error("\nFailed to get logical drive object handle for port: %d enclosure: %d slot: %d\n",
                      port, enclosure, slot);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/******************************************
 * end fbe_cli_logical_drive_get_object_id()
 ******************************************/

/*!**************************************************************
 * fbe_cli_logical_drive_display_header_info()
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
 *  4/30/2009 - Created. rfoley
 *
 ****************************************************************/

static void fbe_cli_logical_drive_display_header_info(fbe_cli_ldo_cmd_type command)
{
    /* Depending on the command display some initial header info.
     */
    switch(command)
    {
        case FBE_CLI_LDO_CMD_TYPE_DISPLAY_VERBOSE_INFO:
            fbe_cli_printf("Logical Drive Information:\n\n");
            break;
        case FBE_CLI_LDO_CMD_TYPE_DISPLAY_TERSE_INFO:
            fbe_cli_logical_drive_display_table_header();
            break;
        default:
            /* Nothing to do for the default case.
             */
            break;
    };
    return;
}
/******************************************
 * end fbe_cli_logical_drive_display_header_info()
 ******************************************/

/*!**************************************************************
 * fbe_cli_cmd_logical_drive()
 ****************************************************************
 * @brief
 *   This function will allow viewing and changing attributes for the logical drive.
 *  
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  4/29/2009 - Created. rfoley
 *
 ****************************************************************/

void fbe_cli_cmd_logical_drive(int argc , char ** argv)
{
    fbe_u32_t port = 8;
    fbe_u32_t enclosure = 8;
    fbe_u32_t slot = 15;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t object_handle_p = FBE_OBJECT_ID_INVALID;
    fbe_cli_ldo_cmd_type command = FBE_CLI_LDO_CMD_TYPE_DISPLAY_TERSE_INFO;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_PHYSICAL;

    /* These are the default values needed to call
     * fbe_cli_execute_command_for_objects(), defaults to all ports,
     * enclosures and slots.
     */
    fbe_u32_t port_all = FBE_API_PHYSICAL_BUS_COUNT;
    fbe_u32_t enclosure_all = FBE_API_ENCLOSURES_PER_BUS;
    fbe_u32_t slot_all = FBE_API_ENCLOSURE_SLOTS;

    /* Context for the command.
     */
    fbe_u32_t context = 0;

    /* Determines if we should target this command at all logical drives. 
     * If the port, enclosure, slot are given, then we will use that single 
     * logical drive as the target. 
     */
    fbe_bool_t b_target_all = FBE_TRUE;

    fbe_cli_command_fn command_fn = NULL;

    /* By default if we are not given any arguments we do a 
     * terse display of all object. 
     * By default we also have -all enabled. 
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
            fbe_cli_printf("%s", LOGICAL_DRIVE_USAGE);
            return;
        }
        else if (strcmp(*argv, "-p") == 0)
        {
            /* Get Port argument.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-p, expected port, too few arguments \n");
                return;
            }
            port = (fbe_u32_t)strtoul(*argv, 0, 0);
            b_target_all = FBE_FALSE;
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
            b_target_all = FBE_FALSE;
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
            b_target_all = FBE_FALSE;
        }
        else if ((strcmp(*argv, "-trace_level") == 0) ||
                 (strcmp(*argv, "-t") == 0))
        {
            /* Get Trace Level argument.
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
            command = FBE_CLI_LDO_CMD_TYPE_SET_TRACE_LEVEL;
        }
        else if ((strcmp(*argv, "-set_state") == 0) ||
                 (strcmp(*argv, "-set") == 0))
        {
            /* Get lifecycle state argument.
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
            command = FBE_CLI_LDO_CMD_TYPE_SET_STATE;
        }
        else if (strcmp(*argv, "-all") == 0)
        {
            /* User wants to target all logical drives.
             */
            b_target_all = FBE_TRUE;
        }
        else if (strcmp(*argv, "-display_terse") == 0)
        {
            /* User wants terse/table display.
             */
            command = FBE_CLI_LDO_CMD_TYPE_DISPLAY_TERSE_INFO;
        }
        else if (strcmp(*argv, "-display_verbose") == 0)
        {
            /* User wants a verbose display.
             */
            command = FBE_CLI_LDO_CMD_TYPE_DISPLAY_VERBOSE_INFO;
        }

        argc--;
        argv++;

    }   /* end of while */

    if (b_target_all == FBE_FALSE)
    {
        fbe_const_class_info_t *class_info_p;

        /* Get the object handle for this target.
         */
        status = fbe_cli_logical_drive_get_object_id(&object_handle_p, port, enclosure, slot);

        if ((status != FBE_STATUS_OK) || (object_handle_p == FBE_OBJECT_ID_INVALID))
        {
            /* Just return the above function displayed an error.
             */
            return;
        }

        /* Make sure this is a valid logical drive.
         */
        status = fbe_cli_get_class_info(object_handle_p,package_id, &class_info_p);

        if (status != FBE_STATUS_OK)
        {
            /* Just return the above function displayed an error.
             */
            return;
        }

        /* Now that we have all the info, make sure this is a logical drive.
         */
        if (class_info_p->class_id != FBE_CLASS_ID_LOGICAL_DRIVE)
        {
            fbe_cli_error("This object id 0x%x is not a logical drive but a %s (0x%x).\n",
                          object_handle_p, class_info_p->class_name, class_info_p->class_id);
            return;
        }
    }
    else
    {
        /* The operation will be performed to all logical drives.
         */
    }

    /* Convert the command to a function.
     */
    command_fn = fbe_cli_logical_drive_get_command_function(command);

    if (command_fn == NULL)
    {
        fbe_cli_error("can't get command function for command %d status: %x\n",
                      command, status);
        return;
    }

    if (b_target_all)
    {
        fbe_cli_logical_drive_display_header_info(command);

        /* Execute the command against all logical drives.
         */
        status = fbe_cli_execute_command_for_objects(FBE_CLASS_ID_LOGICAL_DRIVE,
                                               package_id,
                                               port_all,
                                               enclosure_all,
                                               slot_all,
                                               command_fn,
                                               context /* no context */);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nlogical drive command %s to all drives failed status: 0x%x.\n",
                           fbe_cli_logical_drive_get_command_string(command), status);
            return;
        }
    }
    else
    {

        /* If we are not targeting all, then display the command we are issuing to 
         * this object. 
         */
        fbe_cli_printf("Logical Drive port: %d enclosure: %d slot: %d\n\n",
                       port, slot, enclosure );

        fbe_cli_logical_drive_display_header_info(command);

        /* Execute the command against this single object.
         */
        status = (command_fn)(object_handle_p, context, package_id);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nCommand %s failed to port: %d enclosure: %d slot: %d status: 0x%x.\n",
                           fbe_cli_logical_drive_get_command_string(command), port, enclosure, slot, status);
            return;
        }
    }
    return;
}      
/******************************************
 * end fbe_cli_cmd_logical_drive()
 ******************************************/

void fbe_cli_cmd_set_ica_stamp(int argc , char ** argv)
{
    fbe_u32_t slot = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;

    for(slot = 0; slot < 3; slot++){
        /* Get the object handle for this target. */
        status = fbe_cli_logical_drive_get_object_id(&object_id, 0, 0, slot);
        if ((status != FBE_STATUS_OK) || (object_id == FBE_OBJECT_ID_INVALID)) {
            fbe_cli_printf("Logical Drive slot: %d not found\n\n", slot);
            continue;
        }
        fbe_cli_printf("Logical Drive slot: %d found object_id 0x%X\n\n", slot, object_id);
        /* Set ICA stamp */
        status = fbe_api_logical_drive_interface_generate_ica_stamp(object_id, FBE_TRUE);
        if (status != FBE_STATUS_OK){
            fbe_cli_printf("Logical Drive slot: %d ICA stamp FAILED\n\n", slot);
        } else {
            fbe_cli_printf("Logical Drive slot: %d ICA stamp created\n\n", slot);
        }
    }
 
    return;
}      
/*************************
 * end file fbe_cli_src_logical_drive_cmds.c
 *************************/
