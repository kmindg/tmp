/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_src_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions for fbe_cli.
 *
 * @ingroup fbe_cli
 *
 * HISTORY
 *   11-Mar-2009:  Dipak patel - created
 *
 ***************************************************************************/

#include "fbe_cli_private.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_transport_trace.h"
#include "fbe_base_object_trace.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_sps_info.h"
#include "fbe/fbe_api_enclosure_interface.h"


#include <windows.h>
#include <stdio.h>
#include <ctype.h>

#define NTDDK_H     // recover from GenIncludes BOOLEAN hint
#define FILE_IO_SEEK_ERR 0xFFFFFFFF
#define FILEIO_O_RDONLY 0x000
#define FILEIO_O_WRONLY 0x001
#define FILEIO_O_RDWR   0x002
#define FILEIO_O_NDELAY 0x004
#define FILEIO_O_APPEND 0x008
#define FILEIO_O_CREAT  0x100
#define FILEIO_O_TRUNC  0x200
#define FILEIO_O_UNBUF  0x400
#define FILEIO_O_EXCL   0x800

#define W_OK    2
#define R_OK    4

int  operating_mode;
fbe_bool_t cmd_not_executed;

/*!*************************************************************************
 * @fn fbe_cli_print_LifeCycleState
 *           (fbe_lifecycle_state_t state)
 **************************************************************************
 *
 *  @brief
 *      This function will get int Lifecycle_state and convert into text.
 *
 *  @param    fbe_lifecycle_state_t - state
 *
 *  @return     char * - string
 *
 *  NOTES:
 *
 *  HISTORY:
 *    26-Feb-2009: Dipak Patel- created.
 *
 **************************************************************************/
char * fbe_cli_print_LifeCycleState(fbe_lifecycle_state_t state)
{
    char * LifeCycType;
    switch (state)
    {
        case FBE_LIFECYCLE_STATE_SPECIALIZE:
            LifeCycType = (char *)("Specialize");
        break;
        case FBE_LIFECYCLE_STATE_ACTIVATE:
            LifeCycType = (char *)("Activate");
        break;
        case FBE_LIFECYCLE_STATE_READY:
            LifeCycType = (char *)("Ready");
        break;
        case FBE_LIFECYCLE_STATE_HIBERNATE:
            LifeCycType = (char *)("Hibernate");
        break;
        case FBE_LIFECYCLE_STATE_OFFLINE:
            LifeCycType = (char *)("Offline");
        break;
        case FBE_LIFECYCLE_STATE_FAIL:
            LifeCycType = (char *)("Fail");
        break;
        case FBE_LIFECYCLE_STATE_DESTROY:
            LifeCycType = (char *)("Destroy");
        break;
        case FBE_LIFECYCLE_STATE_NON_PENDING_MAX:
            LifeCycType = (char *)("Non Pending Max");
        break;
       case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
            LifeCycType = (char *)("Pending Activate");
        break;
        case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
            LifeCycType = (char *)("Pending Hibernate");
        break;
        case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
            LifeCycType = (char *)("Pending Offline");
        break;
        case FBE_LIFECYCLE_STATE_PENDING_FAIL:
            LifeCycType = (char *)("Pending Fail");
        break;
        case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
            LifeCycType = (char *)("Pending Destroy");
        break;
        case FBE_LIFECYCLE_STATE_PENDING_MAX:
            LifeCycType = (char *)("Pending Max");
        break;
        case FBE_LIFECYCLE_STATE_INVALID:
            LifeCycType = (char *)("Invalid");
        break;
        default:
            LifeCycType = (char *)( "Unknown ");
        break;
    }
    return (LifeCycType); 
} //End of file fbe_cli_print_LifeCycleState.c

/*!**************************************************************
 * fbe_cli_execute_command_for_objects_order_defined()
 ****************************************************************
 * @brief
 *  This function will iterate over some set of objects
 *  specified by an optional filter based on class, port, enclosure and slot.
 *  Results are ordered by object-id or physical order, defined by cmd_order parameter.
 *
 * @param filter_class_id - Class to iterate over.
 *                          If this is set to FBE_CLASS_ID_INVALID,
 *                          then it means to iterate over all classes.
 * 
 * @param package_id - package id of that object.
 *
 * @param port -  Selected port to iterate over, if set to FBE_API_PHYSICAL_BUS_COUNT
 *                then all ports are iterated over, otherwise when set to a value less
 *                than FBE_API_PHYSICAL_BUS_COUNT only that port is considered.
 *
 * @param enclosure - Selected enclosure to iterate over, if set to FBE_API_ENCLOSURES_PER_BUS
 *                    then all enclosures are iterated over, otherwise when set to a value less
 *                    than FBE_API_ENCLOSURES_PER_BUS only that enclosure is considered.
 *
 * @param slot - Selected slot to iterate over, if set to FBE_API_ENCLOSURE_SLOTS
 *               then all slots are iterated over, otherwise when set to a value less
 *               than FBE_API_ENCLOSURE_SLOTS only that slot is considered.
 * 
 * @param cmd_order - Desired order of command execution, by object-id or physical order. 
 *
 * @param command_function_p - The function to call for each object found.
 * 
 * @param context - This is the context that should be passed in to
 *                  the command_function_p.
 * 
 * @return fbe_status_t FBE_STATUS_OK if all commands issued to
 *                      the found objects were ok.
 *                      If any of the commands fail, then we will continue issuing
 *                      commands, but we will be returning the last failure status.
 *
 * @author
 *  5/9/2012 - Created. AV
 *
 ****************************************************************/
fbe_status_t fbe_cli_execute_command_for_objects_order_defined(fbe_class_id_t filter_class_id,
                                                               fbe_package_id_t package_id,
                                                               fbe_u32_t port,
                                                               fbe_u32_t enclosure,
                                                               fbe_u32_t slot,
															   fbe_cli_pdo_order_t cmd_order,
                                                               fbe_cli_command_fn command_function_p,
                                                               fbe_u32_t context)
{
    if (cmd_order == OBJECT_ORDER)
    {
		// Send commands sorted by object-id
        return fbe_cli_execute_command_for_objects(filter_class_id,
                                                   package_id,
                                                   port,
                                                   enclosure,
                                                   slot,
                                                   command_function_p,
                                                   context);
    }
    else if (cmd_order == PHYSICAL_ORDER)
    {
        // Send commands sorted by physical order (port-enclosure-slot)
        return fbe_cli_execute_command_for_objects_physical_order(filter_class_id,
                                                                  package_id,
                                                                  port,
                                                                  enclosure,
                                                                  slot,
                                                                  command_function_p,
                                                                  context);
    }
	else
    {
        // Command order not supported
        fbe_cli_error("Command not supported, cmd_order enum set to %d\n",cmd_order);
        return FBE_STATUS_GENERIC_FAILURE;
    }
}
/******************************************
 * end fbe_cli_execute_command_for_objects_order_defined()
 ******************************************/

/*!**************************************************************
 * fbe_cli_execute_command_for_objects()
 ****************************************************************
 * @brief
 *  This function will iterate over some set of objects
 *  specified by a filter class.
 *
 * @param filter_class_id - Class to iterate over.
 *                          If this is set to FBE_CLASS_ID_INVALID,
 *                          then it means to iterate over all classes.
 * 
 * @param package_id - package id of that object.
 * 
 * @param filter_port -  Selected port to iterate over, if set to FBE_API_PHYSICAL_BUS_COUNT
 *                       then all ports are iterated over, otherwise when set to a value less
 *                       than FBE_API_PHYSICAL_BUS_COUNT only that port is considered.
 *
 * @param filter_enclosure - Selected enclosure to iterate over, if set to FBE_API_ENCLOSURES_PER_BUS
 *                           then all enclosures are iterated over, otherwise when set to a value less
 *                           than FBE_API_ENCLOSURES_PER_BUS only that enclosure is considered.
 *
 * @param filter_slot - Selected slot to iterate over, if set to FBE_API_ENCLOSURE_SLOTS
 *                      then all slots are iterated over, otherwise when set to a value less
 *                      than FBE_API_ENCLOSURE_SLOTS only that slot is considered.
 *
 * @param command_function_p - The function to call for each object found.
 *
 * @param context - This is the context that should be passed in to
 *                  the command_function_p.
 * 
 * @return fbe_status_t FBE_STATUS_OK if all commands issued to
 *                      the found objects were ok.
 *                      If any of the commands fail, then we will continue issuing
 *                      commands, but we will be returning the last failure status.
 *
 * @author
 *  4/30/2009 - Created. RPF
 *  6/08/2012 - Modified to add port/enclosure/slot support.
 *
 ****************************************************************/
fbe_status_t fbe_cli_execute_command_for_objects(fbe_class_id_t filter_class_id,
                                                 fbe_package_id_t package_id,
                                                 fbe_u32_t filter_port,
                                                 fbe_u32_t filter_enclosure,
                                                 fbe_u32_t filter_drive_no,
                                                 fbe_cli_command_fn command_function_p,
                                                 fbe_u32_t context)

{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           object_index;
    fbe_u32_t           total_objects = 0;
    fbe_object_id_t    *object_list_p = NULL;
    fbe_class_id_t      class_id;
    fbe_port_number_t port = FBE_API_PHYSICAL_BUS_COUNT;
    fbe_enclosure_number_t enclosure = FBE_API_ENCLOSURES_PER_BUS;
    fbe_enclosure_slot_number_t drive_no = FBE_API_ENCLOSURE_SLOTS;
    fbe_status_t        error_status = FBE_STATUS_OK; /* Save last error we see.*/
    fbe_u32_t           found_objects_count = 0;      /* Number matching class filter. */
	fbe_u32_t			object_count = 0;
    fbe_topology_object_type_t  obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;

	status = fbe_api_get_total_objects(&object_count, package_id);
    if (status != FBE_STATUS_OK) {
		return status;
	}

    /* Allocate memory for the objects.
     */
    object_list_p = fbe_api_allocate_memory(sizeof(fbe_object_id_t) * object_count);

    if (object_list_p == NULL)
    {
        fbe_cli_error("Unable to allocate memory for object list %s \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Find the count of total objects.
     */
    status = fbe_api_enumerate_objects(object_list_p, object_count, &total_objects, package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_free_memory(object_list_p);
        return status;
    }

    /* Loop over all objects found in the table.
     */
    for (object_index = 0; object_index < total_objects; object_index++)
    {
        fbe_object_id_t object_id = object_list_p[object_index];

        status = fbe_api_get_object_class_id(object_id, &class_id, package_id);
        if(status == FBE_STATUS_OK)
        {
            status = fbe_api_get_object_type (object_id, &obj_type, package_id);
        }

        if (FBE_STATUS_OK == status)
        {
            switch(obj_type)
            {


                /* If the object is a drive or logical drive get the drive number and fall through*/
                case FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE:
                    if ((status = fbe_api_get_object_drive_number(object_id, &drive_no)) != FBE_STATUS_OK)
                    {
                        break;
                    }

                /* If the object is an enclosure get the enclosure number and fall through*/
                case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
                    if ((status = fbe_api_get_object_enclosure_number(object_id, &enclosure)) != FBE_STATUS_OK)
                    {
                        break;
                    }

                /* If the object is a port get the port number and fall through*/
                case FBE_TOPOLOGY_OBJECT_TYPE_PORT:
                    if ((status = fbe_api_get_object_port_number(object_id, &port)) != FBE_STATUS_OK)
                    {
                        break;
                    }

                /* If the object is any other type exit the switch */
                default:
                    break;
            }
        }

        /* Match the class id to the filter class id.
         * OR 
         * Assume a match if the filter id is set to invalid, which means to apply to 
         * every object. 
         */
        if ((status == FBE_STATUS_OK) && 
            ((class_id == filter_class_id) || (filter_class_id == FBE_CLASS_ID_INVALID)) &&
            ((port == filter_port) || (filter_port == FBE_API_PHYSICAL_BUS_COUNT)) &&
            ((enclosure == filter_enclosure) || (filter_enclosure == FBE_API_ENCLOSURES_PER_BUS)) &&
            ((drive_no == filter_drive_no) || (filter_drive_no == FBE_API_ENCLOSURE_SLOTS)))
        {
            /* Now that we found it, call our input function to apply some operation 
             * to this object.
             */
            status = (command_function_p)(object_id, context, package_id);

            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("%s error on  object 0x%x \n", __FUNCTION__, object_id);
                error_status = status;
            }

            found_objects_count++;

        } /* End if class id fetched OK and it is a match.*/
        else if (status != FBE_STATUS_OK)
        {
            /* Error fetching class id.
             */
            fbe_cli_error("can't get object class_id for id [%x], status: %x\n",
                          object_id, status);
        }
    }/* Loop over all objects found. */

    /* Display our totals.
     */
    fbe_cli_printf("Discovered %.4d objects in total.\n", total_objects);

    /* Only display this total if we have a filter.
     */
    if (filter_class_id != FBE_CLASS_ID_INVALID)
    {
        fbe_cli_printf("Discovered %.4d objects matching filter of class %d.\n", 
                       found_objects_count, filter_class_id);
    }
    /* Free up the memory we allocated for the object list.
     */
    fbe_api_free_memory(object_list_p);
    return error_status;
}
/******************************************
 * end fbe_cli_execute_command_for_objects()
 ******************************************/

/*!**************************************************************
 * fbe_cli_execute_command_for_objects_physical_order()
 ****************************************************************
 * @brief
 *  This function will iterate over some set of objects
 *  specified by an optional filter based on class, port, enclosure and slot.
 *  Results are ordered on port-enclosure-slot fashion.
 *
 * @param filter_class_id - Class to iterate over.
 *                          If this is set to FBE_CLASS_ID_INVALID,
 *                          then it means to iterate over all classes.
 * 
 * @param package_id - package id of that object.
 * 
 * @param filter_port - Selected port to iterate over, if set to FBE_API_PHYSICAL_BUS_COUNT
 *                      then all ports are iterated over, otherwise when set to a value less
 *                      than FBE_API_PHYSICAL_BUS_COUNT only that port is considered.
 *
 * @param filter_enclosure - Selected enclosure to iterate over, if set to FBE_API_ENCLOSURES_PER_BUS
 *                           then all enclosures are iterated over, otherwise when set to a value less
 *                           than FBE_API_ENCLOSURES_PER_BUS only that enclosure is considered.
 *
 * @param filter_slot - Selected slot to iterate over, if set to FBE_API_ENCLOSURE_SLOTS
 *                      then all slots are iterated over, otherwise when set to a value less
 *                      than FBE_API_ENCLOSURE_SLOTS only that slot is considered.
 *
 * @param command_function_p - The function to call for each object found.
 * 
 * @param context - This is the context that should be passed in to
 *                  the command_function_p.
 * 
 * @return fbe_status_t FBE_STATUS_OK if all commands issued to
 *                      the found objects were ok.
 *                      If any of the commands fail, then we will continue issuing
 *                      commands, but we will be returning the last failure status.
 *
 * @author
 *  5/1/2012 - Created. AV
 *
 ****************************************************************/

fbe_status_t fbe_cli_execute_command_for_objects_physical_order(fbe_class_id_t filter_class_id,
                                                                fbe_package_id_t package_id,
                                                                fbe_u32_t filter_port,
                                                                fbe_u32_t filter_enclosure,
                                                                fbe_u32_t filter_slot,
                                                                fbe_cli_command_fn command_function_p,
                                                                fbe_u32_t context)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_object_id_t     object_id;
	fbe_object_id_t     enclosure_object_id;
	fbe_u32_t           enclosure_slot_count;
    fbe_class_id_t      class_id;
    fbe_status_t        error_status = FBE_STATUS_OK; /* Save last error we see.*/
    fbe_u32_t           found_objects_count = 0;      /* Number matching class filter. */
	fbe_u32_t			port;
	fbe_u32_t			enclosure;
	fbe_u32_t			slot;

    for (port = 0; port < FBE_API_PHYSICAL_BUS_COUNT; port++)
	{
		/* Skip if port filter is enabled and this is not the filtered port 
		 * Note that a filter value exceeding the maximum capacity of ports is 
		 * considered as an enabled filter.
		 */
		if ((filter_port != FBE_API_PHYSICAL_BUS_COUNT) && (port != filter_port))
		{
			continue;
		}
		for (enclosure = 0; enclosure < FBE_API_ENCLOSURES_PER_BUS; enclosure++)
		{
           /* Skip if enclosure filter is enabled and this is not the filtered enclosure 
            * Note that a filter value exceeding the maximum capacity of enclosures is 
            * considered as an enabled filter.
            */
			if ((filter_enclosure != FBE_API_ENCLOSURES_PER_BUS) && (enclosure != filter_enclosure))
			{
				continue;
			}

			status = fbe_api_get_enclosure_object_id_by_location(port, enclosure, &enclosure_object_id); 				           

            if ((enclosure_object_id == FBE_OBJECT_ID_INVALID) || (status != FBE_STATUS_OK))
            {	
                /* Skip this enclosure since object could not be retrieved */
                continue;
            }

            /* Figure out how many slots this enclosure has */
            status = fbe_api_enclosure_get_slot_count(enclosure_object_id, &enclosure_slot_count);

            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("%s error retrieving slots from enclosure object 0x%x \n", __FUNCTION__, enclosure_object_id);
				
                /* Skip this enclosure since slot count could not be retrieved */
                continue;
            }
            
			
            /* If number of slots returns zero we do not know the actual number, use maximum to iterate over */
            if (enclosure_slot_count == 0)
			{
				enclosure_slot_count = FBE_API_ENCLOSURE_SLOTS;
			}

			for (slot = 0; slot < enclosure_slot_count; slot++)
			{
               /* Skip if slot filter is enabled and this is not the filtered slot
                * Note that a filter value exceeding the maximum capacity of slots is 
                * considered as an enabled filter.
                */				
				if ((filter_slot != FBE_API_ENCLOSURE_SLOTS) && (slot != filter_slot))
				{
					continue;
				}

				/* If we got here then this is a desired disk, fetch object-id and attempt to issue command */
				status = fbe_api_get_physical_drive_object_id_by_location(port, enclosure, slot, &object_id);

				/* Skip this disk if a failure occured while retrieving object id*/
				if(status != FBE_STATUS_OK)
				{
					continue;
				}

				/* Proceed with command if the object ID is valid and of the right class (if filtered) */
				if(object_id != FBE_OBJECT_ID_INVALID)
				{
					status = fbe_api_get_object_class_id(object_id, &class_id, package_id);

					if ((status == FBE_STATUS_OK) && 
					((class_id == filter_class_id) || (filter_class_id == FBE_CLASS_ID_INVALID)))
					{
						/* Now that we found it, call our input function to apply some operation 
						* to this object.
						*/
						status = (command_function_p)(object_id, context, package_id);
		
						if (status != FBE_STATUS_OK)
						{
							fbe_cli_error("%s error on  object 0x%x \n", __FUNCTION__, object_id);
							error_status = status;
						}

						found_objects_count++;
					}
			        else if (status != FBE_STATUS_OK)
					{
						/* Error fetching class id.
						*/
						fbe_cli_error("can't get object class_id for id [%x], status: %x\n",
							object_id, status);
					}
				} /* End if object_id != FBE_OBJECT_ID_INVALID */
			} /* End slot for loop */
		} /* End enclosure for loop*/
	} /* End port for loop*/

    /* Only display this total if we have a filter.
     */
    if (filter_class_id != FBE_CLASS_ID_INVALID)
    {
        fbe_cli_printf("Discovered %.4d objects matching filter of class %d.\n", 
                       found_objects_count, filter_class_id);
    }
    
    return error_status;
}
/******************************************
 * end fbe_cli_execute_command_for_objects_physical_order()
 ******************************************/

/*!**************************************************************
 * fbe_cli_set_trace_level()
 ****************************************************************
 * @brief
 *  This function allows us to set the trace level of a
 *  given object id.
 *
 * @param object_id - object id to change trace level of.
 * @param trace_level - level to set object's trace level to.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  4/30/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_cli_set_trace_level(fbe_u32_t object_id, fbe_u32_t trace_level, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_trace_level_control_t trace_control = {0};

    /* Cast the context to a trace level and simply issue the set trace level command.
     */
    trace_control.trace_type = FBE_TRACE_TYPE_OBJECT;
    trace_control.fbe_id = object_id;
    trace_control.trace_level = (fbe_trace_level_t) trace_level;
    status = fbe_api_trace_set_level(&trace_control, package_id);

    /* Handle the status.
     */
    if (status == FBE_STATUS_OK)
    {
        fbe_cli_printf("Successfully set trace level to %d ", trace_level);
        fbe_base_object_trace_level(trace_control.trace_level,
                                    fbe_cli_trace_func,
                                    NULL);
        fbe_cli_printf(" for object id 0x%x\n", object_id);
    }
    else
    {
        fbe_cli_printf("\n\nFailed to set trace level to %d ", trace_level);
        fbe_base_object_trace_level(trace_control.trace_level,
                                    fbe_cli_trace_func,
                                    NULL);
        fbe_cli_printf(" for object id 0x%x status: 0x%x\n", object_id, status);
    }
    return status;
}
/******************************************
 * end fbe_cli_set_trace_level()
 ******************************************/

/*!**************************************************************
 * fbe_cli_set_lifecycle_state()
 ****************************************************************
 * @brief
 *  This function allows us to set the state of a
 *  given object id.
 *
 * @param object_id - object id to change trace level of.
 * @param state - set object's state to.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  4/30/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_cli_set_lifecycle_state(fbe_u32_t object_id, fbe_u32_t state, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lifecycle_state_t lifecycle_state = (fbe_lifecycle_state_t)state;

    /* Try to change the state.
     */
    status = fbe_api_set_object_to_state(object_id, lifecycle_state, package_id);

    /* Handle the status.
     */
    if (status == FBE_STATUS_OK)
    {
        fbe_cli_printf("Successfully set state to %d ", state);
        fbe_base_object_trace_state(lifecycle_state,
                                    fbe_cli_trace_func,
                                    NULL);
        fbe_cli_printf(" for object id 0x%x\n", object_id);
    }
    else
    {
        fbe_cli_printf("\n\nFailed to set state to %d ", state);
        fbe_base_object_trace_state(lifecycle_state,
                                    fbe_cli_trace_func,
                                    NULL);
        fbe_cli_printf(" for object id 0x%x status: 0x%x\n", object_id, status);
    }
    return status;
}
/******************************************
 * end fbe_cli_set_lifecycle_state()
 ******************************************/

/*!**************************************************************
 * fbe_cli_set_write_uncorrectable()
 ****************************************************************
 * @brief
 *  This function allows us to set write_uncorrectable to a
 *  sata object id.
 *
 * @param object_id - object id to change trace level of.
 * @param lba - set lba.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  11/10/2011 - Ported from fbe_cli functionality. - Dipak Patel
 *
 ****************************************************************/
fbe_status_t fbe_cli_set_write_uncorrectable(fbe_u32_t object_id, fbe_lba_t lba)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    status = fbe_api_physical_drive_set_write_uncorrectable(object_id, lba, FBE_PACKET_FLAG_NO_ATTRIB);
  
    /* Handle the status.
     */
    if (status == FBE_STATUS_OK)
    {
        fbe_cli_printf("Successfully set write uncorrectable 0x%llx for object id 0x%x\n", (unsigned long long)lba, object_id);
    }
    else
    {
        fbe_cli_printf("\n\nFailed to set set write uncorrectable 0x%llx for object id 0x%x\n", (unsigned long long)lba, object_id);
    }
    return status;
}

/*!**************************************************************
 * fbe_cli_get_class_info()
 ****************************************************************
 * @brief
 *  Get the class id information for the input object.
 *
 * @param  object_id - The object id to send to.
 * @param  class_info_p - The class information ptr ptr to fill in.
 *
 * @return fbe_status_t.
 *
 * @author
 *  5/04/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_cli_get_class_info(fbe_u32_t object_id,
                                      fbe_package_id_t package_id,
                                    fbe_const_class_info_t **class_info_p)
{
    fbe_class_id_t class_id;
    fbe_status_t status;

    /* Get the class id for this object first.
     */
    status = fbe_api_get_object_class_id(object_id, &class_id, package_id);

    if (status == FBE_STATUS_OK)
    {
        /* Get the information on this class.
         */
        status = fbe_get_class_by_id(class_id, class_info_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("can't get object class info for id [%x], status: %x\n",
                          object_id, status);
            return status;
        }
    }
    else
    {
        fbe_cli_error("can't get object class_id for id [%x], status: %x\n",
                      object_id, status);
        return status;
    }
    return status;
}
/******************************************
 * end fbe_cli_get_class_info()
 ******************************************/

/*!**************************************************************
 * fbe_cli_get_bus_enclosure_slot_info()
 ****************************************************************
 * @brief
 *  Get the bus, enclosure, slot information for this object.
 *
 * @param  object_id - the object identifier.
 * @param  class_id - The class information.
 * @param  port_p - The port number to return.
 * @param  enc_p - The enclosure number to return.
 * @param  slot_p - The slot number to return.
 *
 * @return - fbe_status_t the status of the operation. 
 *
 * @author
 *  5/6/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_cli_get_bus_enclosure_slot_info(fbe_object_id_t object_id,
                                                 fbe_class_id_t class_id,
                                                 fbe_port_number_t *port_p,
                                                 fbe_enclosure_number_t *enc_p,
                                                 fbe_enclosure_slot_number_t *slot_p,
                                                 fbe_package_id_t package_id)
{
    fbe_status_t status;

    *port_p = FBE_PORT_NUMBER_INVALID;
    *enc_p = FBE_ENCLOSURE_NUMBER_INVALID;
    *slot_p = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;

    if ( ((class_id >= FBE_CLASS_ID_PORT_FIRST) &&
          (class_id <= FBE_CLASS_ID_PORT_LAST)) ||
         ((class_id >= FBE_CLASS_ID_ENCLOSURE_FIRST) &&
          (class_id <= FBE_CLASS_ID_ENCLOSURE_LAST)) ||
         ((class_id >= FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) &&
          (class_id <= FBE_CLASS_ID_PHYSICAL_DRIVE_LAST)) ||
         ((class_id >= FBE_CLASS_ID_LOGICAL_DRIVE_FIRST) &&
          (class_id <= FBE_CLASS_ID_LOGICAL_DRIVE_LAST)) )
    {
        /* We have a port.
         */
        status  = fbe_api_get_object_port_number(object_id, port_p);
        if (status != FBE_STATUS_OK)
        {
            *port_p = FBE_PORT_NUMBER_INVALID;
        }
    }

    if ( ((class_id >= FBE_CLASS_ID_ENCLOSURE_FIRST) &&
          (class_id <= FBE_CLASS_ID_ENCLOSURE_LAST)) ||
         ((class_id >= FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) &&
          (class_id <= FBE_CLASS_ID_PHYSICAL_DRIVE_LAST)) ||
         ((class_id >= FBE_CLASS_ID_LOGICAL_DRIVE_FIRST) &&
          (class_id <= FBE_CLASS_ID_LOGICAL_DRIVE_LAST)) )
    {
        /* We have an enclosure.
         */
        status = fbe_api_get_object_enclosure_number(object_id, enc_p);
        if (status != FBE_STATUS_OK)
        {
            *enc_p = FBE_ENCLOSURE_NUMBER_INVALID;
        }
    }

    if ( ((class_id >= FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) &&
          (class_id <= FBE_CLASS_ID_PHYSICAL_DRIVE_LAST)) ||
         ((class_id >= FBE_CLASS_ID_LOGICAL_DRIVE_FIRST) &&
          (class_id <= FBE_CLASS_ID_LOGICAL_DRIVE_LAST)) )
    {
        /* We have a slot.
         */ 
        status = fbe_api_get_object_drive_number(object_id, slot_p);
        if (status != FBE_STATUS_OK)
        {
            *slot_p = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;
        }
    }
	if(class_id == FBE_CLASS_ID_PROVISION_DRIVE)
	{
	    status  = fbe_api_provision_drive_get_location(object_id, port_p, enc_p, slot_p);
        if (status != FBE_STATUS_OK)
        {
            *port_p = FBE_PORT_NUMBER_INVALID;
			*enc_p = FBE_ENCLOSURE_NUMBER_INVALID;
			*slot_p = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;
        }
		
	}
	if( (class_id == FBE_CLASS_ID_LUN) ||(class_id == FBE_CLASS_ID_RAID_GROUP) ||
		(class_id == FBE_CLASS_ID_RAID_FIRST) || (class_id == FBE_CLASS_ID_RAID_LAST) ||
		(class_id == FBE_CLASS_ID_BVD_INTERFACE) || (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE))
	{
	    *port_p = FBE_PORT_ENCL_SLOT_NA;
	    *enc_p =  FBE_PORT_ENCL_SLOT_NA;
		*slot_p = FBE_PORT_ENCL_SLOT_NA;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_get_bus_enclosure_slot_info()
 ******************************************/
 
static int fileio_fgets(csx_p_file_t *fd, char *buffer, unsigned int nbytes_to_read)
/*      fd                 File descriptor (handle)      
 *      buffer             User data buffer.
 *      nbytes_to_read     Size (in bytes) of the read request.
 */
{
    csx_size_t   nbytesRead;
    unsigned int cumBytesRead = 0;
    csx_status_e status;
    unsigned int maxLength = (nbytes_to_read - 1);

    if (nbytes_to_read == 0)
        return -1;

    buffer[0] = 0;
    while(cumBytesRead < maxLength)
    {
        status = csx_p_file_read(fd, &buffer[cumBytesRead], 1, &nbytesRead);
        if (!CSX_SUCCESS(status))
        {
            if (CSX_STATUS_END_OF_FILE != status)
            {
                printf("System Error: csx_p_file_read: 0x%x\n", (int)status);
            }
            return -1;
        }
        if (nbytesRead == 0)
        {
            // End Of file is detected.
            if (cumBytesRead > 0)   // Pass the data this time.
                break;
            else
                return -1;  // return error for End Of File
        }
        // The new line in DOS has 0x0d followed by 0x0a for the
        // end of the line.
        if (buffer[cumBytesRead] == 0x0d)
            // No action
            ;
        else if (buffer[cumBytesRead] == 0x0a)
            break;
        else
            cumBytesRead++;
    }
    
    buffer[cumBytesRead] = 0;

    return cumBytesRead;    
}

#define fgets(a,b,c)    (fileio_fgets(c,a,b) < 0 ? NULL : (a))

static void prompt_split(char *line, int *argc, char ** argv)
{
    static fbe_u8_t * prompt_delimit = " ,\t";

    *argc = 0;
    *argv = strtok(line, prompt_delimit);
    while (*argv != NULL) {
        *argc = *argc + 1;
        argv++;
        *argv = strtok(NULL, prompt_delimit);
    }
}
void fbe_cli_x_run_script(int argc , char ** argv)
{
    char * input_file = *argv;   // path to file
    csx_p_file_t fbe_cli_script = NULL;        // file handle
    char current_line [1000];      // command buffer
    fbe_cli_cmd_t *cmd1 = NULL;
    char *argv1[CMD_MAX_ARGC] = {0};
    fbe_u32_t argc1 = 0;
    csx_status_e status;

    status = csx_p_file_open(CSX_MY_MODULE_CONTEXT(), &fbe_cli_script,
                             input_file, "r");

    // try to open the file, if it fails, print an error
    if( !CSX_SUCCESS(status) || fbe_cli_script == NULL )
    {
         fbe_cli_printf( "The file %s was not found\n", input_file );
         return;
    }
    // else, try to execute the file line by line
    else
    {
        fbe_cli_printf("FBE_CLI>");

	cmd_not_executed = TRUE;
        // clear out the buffer
        memset(current_line, '\0', sizeof(current_line));

        // file handle is open, process contents line by line
        while (fgets ((char*)current_line, sizeof(current_line), &fbe_cli_script) != (char *) NULL)
        {
            fbe_cli_printf(current_line);
            // call the script processing function
            prompt_split(current_line, &argc1, argv1);
            if (argc1 == 0) 
            {
                
                continue;
            }
            fbe_cli_cmd_lookup(argv1[0], &cmd1);
            if (cmd1 == NULL) 
            {
                fbe_cli_print_usage();
                continue;
            }
            if (cmd1->type == FBE_CLI_CMD_TYPE_QUIT) {
                break;
            }
            argc1--;
	    if (operating_mode == USER_MODE &&
                cmd1->access != FBE_CLI_USER_ACCESS)
            {
                /* Skip over this command,
                 * we will ignore these commands in user mode.
                 */
            }
	    else
	    {
                (*cmd1->func)(argc1, &argv1[1]);
		cmd_not_executed = FALSE;
	    }
		 /* Output error message */
            if (cmd_not_executed)
	    {
		printf("Command \"%s\" NOT found\n", argv1[0]);
            }
            
            // clear out the buffer
            memset(current_line, '\0', sizeof(current_line));
        }
    }

    status = csx_p_file_close(&fbe_cli_script);

    if (!CSX_SUCCESS(status))
    {
        printf("System Error: csx_p_file_close: 0x%x\n", (int)status);
    }
} // end fbe_cli_x_run_script

void fbe_cli_x_access(int argc , char ** argv)
{

/*********************************
 **    VARIABLE DECLARATIONS    **
 *********************************/

    fbe_s32_t opt_value = 0;       /* value for optional argument */
    fbe_bool_t valid_opt_value = FALSE;       /* valid optional argument value */

/*****************
 **    BEGIN    **
 *****************/


    /* debug <options> */
    if (argc < 1)
    {
        fbe_cli_printf("[switches]\n\
Switches:\n\
  -m [1 | 2]     Access mode: 1 = ENG mode, 2 = USER mode\n\n");
        return;
    }

    while(argc > 0)
    {
        if(strcmp(*argv, "-m") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                printf("Mode: %s\n",
                       operating_mode == ENG_MODE ? "Eng Mode" : "User Mode");
                return;
            }
            opt_value = (fbe_u32_t)strtoul(*argv, 0, 0);
            valid_opt_value = TRUE;
        }
        argc--;
        argv++;
    }
    /* validate option */
    if (valid_opt_value)
    {

        /* engineering-mode */
        if (opt_value == ENG_MODE)
        {
            operating_mode = ENG_MODE;
        }
        else if (opt_value == USER_MODE)
        {
            operating_mode = USER_MODE;
        }

        else
        {
            printf("%s: Illegal mode-value (1-eng, 2-user)\n", *argv);
            return;
        }
    }
    else
    {
        printf("Usage:\n\
   access -m [1 | 2] (1-eng, 2-user)\n\
   access -m  (displays current mode)\n");
    }
} /* fbe_cli_x_access */

fbe_u32_t fbe_atoh(fbe_char_t *s_p)
{
    fbe_u32_t   fvar;
    size_t      length;
    fbe_u32_t   result;
    fbe_u32_t   temp;

    /* First remove the leading hex value (if present)
     */
    if ((*s_p == '0') && 
        (*(s_p + 1) == 'x' || *(s_p + 1) == 'X'))
    {
        s_p += 2;
    }
         
    /* Now check the length.  If it's greater than 8, return the error code.
     */
    length = strlen(s_p);
    if ((length > 8) || (length == 0))
    {
        return FBE_U32_MAX;
    }

    /* Now, convert the string */
    result = 0;

    for (fvar = 0; fvar < length; fvar++)
    {
        /*  Shift the result to the next power of 16.  */
        result = result << 4;

        temp = s_p[fvar];

        if ((temp >= '0') && (temp <= '9'))
        {
            result += (temp - '0');
        }
        else if ((temp >= 'a') && (temp <= 'f'))
        {
            result += (0xA + (temp - 'a'));
        }
        else if ((temp >= 'A') && (temp <= 'F'))
        {
            result += (0xA + (temp - 'A'));
        }
        else
        {
            result = FBE_U32_MAX;
            break;
        }
    }                           /*  End of "for..." loop.  */

    /* That's it.  Return the result. */
    return (result);
}

/*!**************************************************************
 * fbe_atoh64()
 ****************************************************************
 * @brief
 *  This function gives u64 value for hexadecimal string.
 *
 * @param s_p - hexadecimal string.
 * @param result - u64 value
 *
 * @return fbe_status_t
 *
 * @author
 *  09/01/2010  Swati Fursule  - Created.
 *
 ****************************************************************/
fbe_status_t  fbe_atoh64(fbe_char_t * s_p,
                       fbe_u64_t * result)
{
    while(isspace(*s_p))
    {
        s_p++;
    }
    /*skip 0x */
    if(*s_p == '0' && (*(s_p+1) == 'x' || *(s_p+1) == 'X'))
    {
        s_p+=2;
    }
    if(isxdigit(*s_p))
    {
        fbe_u64_t r = 0;
        do
        {
            if(isdigit(*s_p))
            {
                r = 16*r + (fbe_u64_t)(*s_p - '0');
            }
            else
            {
                r = 16*r + (fbe_u64_t)((toupper(*s_p) - 55));
            }
        }
        while(isxdigit(*++s_p));
        /*skip trailing spaces*/
        while(isspace(*s_p))
        {
            s_p++;
        }
        /*check that we get into the end of line*/
        if(!(*s_p))
        {
            *result = r;
            return FBE_STATUS_OK;
        }
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*************************************************************************
 * @fn fbe_cli_lib_SP_ID_to_string
 **************************************************************************
 *
 *  @brief
 *      This function will get int SP_ID and convert into text.
 *
 *  @param    SP_ID - spid
 *
 *  @return     char * - string
 *
 *  NOTES:
 *
 *  HISTORY:
 *    25-Mar-2011: Rashmi Sawale- created.
 *
 **************************************************************************/
fbe_char_t * fbe_cli_lib_SP_ID_to_string(SP_ID spid)
{
     char * sp_id;
     switch (spid)
     {
         case SP_A:
             sp_id = (char *)("SP_A");
         break;
         case SP_B:
             sp_id = (char *)("SP_B");
	  break;
	  case SP_ID_MAX:
             sp_id = (char *)("MAX_SP");
         break;
	  case SP_NA:
             sp_id = (char *)("N/A");
         break;
	  case SP_INVALID:
             sp_id = (char *)("INVALID");
         break;
         default:
            sp_id = (char *)( "UNKNOWN");
         break;
    }
    return (sp_id);    
}

/*!*************************************************************************
 * @fn fbe_atoi
 **************************************************************************
 *
 *  @brief
 *      This function gets string and converts it to int.
 *
 *  @param    *pData - The input string
 *
 *  @return     fbe_u32_t - the integer
 *
 *
 *  HISTORY:
 *    14-Sep-2011: Shubhada Savdekar- created.
 *
 **************************************************************************/
fbe_u32_t fbe_atoi(fbe_u8_t * pData) {

    fbe_u32_t  result = 0;
    fbe_u32_t  sign = 1;

    /*Check for negative value*/
    if(*pData == '-'){
            sign =-1;
            pData++;
    }

    /*Conver to int*/
    while(*pData!='\0')
    {
        if((*pData >= '0') && (*pData <= '9')){
            result = (result * 10) + (*pData - '0');
            pData++;
        }
        else { 
            break;
        }
    }

     result *=sign;
    return result;

}

/*!*************************************************************************
 * @fn fbe_cli_print_EnclosureType
 *           (fbe_esp_encl_type_t encl_type)
 **************************************************************************
 *
 *  @brief
 *      This function will get int encl_type and convert into text.
 *
 *  @param    fbe_esp_encl_type_t - encl_type
 *
 *  @return     char * - string
 *
 *  NOTES:
 *
 *  HISTORY:
 *    09-Jan-2012: Zhenhua Dong- created.
 *
 **************************************************************************/
char * fbe_cli_print_EnclosureType(fbe_esp_encl_type_t encl_type)
{
    char * EnclType;
    switch (encl_type)
    {
        case FBE_ESP_ENCL_TYPE_INVALID:
            EnclType = (char *)("INVALID");
            break;
        case FBE_ESP_ENCL_TYPE_BUNKER:
            EnclType = (char *)("BUNKER");
            break;
        case FBE_ESP_ENCL_TYPE_CITADEL:
            EnclType = (char *)("CITADEL");
            break;
        case FBE_ESP_ENCL_TYPE_VIPER:
            EnclType = (char *)("VIPER");
            break;
        case FBE_ESP_ENCL_TYPE_DERRINGER:
            EnclType = (char *)("DERRINGER");
            break;
        case FBE_ESP_ENCL_TYPE_VOYAGER:
            EnclType = (char *)("VOYAGER");
            break;
        case FBE_ESP_ENCL_TYPE_FOGBOW:
            EnclType = (char *)("FOGBOW");
            break;
        case FBE_ESP_ENCL_TYPE_BROADSIDE:
            EnclType = (char *)("BROADSIDE");
            break;
        case FBE_ESP_ENCL_TYPE_RATCHET:
            EnclType = (char *)("RATCHET");
            break;
        case FBE_ESP_ENCL_TYPE_FALLBACK:
            EnclType = (char *)("FALLBACK");
            break;
        case FBE_ESP_ENCL_TYPE_BOXWOOD:
            EnclType = (char *)("BOXWOOD");
            break;
        case FBE_ESP_ENCL_TYPE_KNOT:
            EnclType = (char *)("KNOT");
            break;
        case FBE_ESP_ENCL_TYPE_PINECONE:
            EnclType = (char *)("PINECONE");
            break;
        case FBE_ESP_ENCL_TYPE_STEELJAW:
            EnclType = (char *)("STEELJAW");
            break;
        case FBE_ESP_ENCL_TYPE_RAMHORN:
            EnclType = (char *)("RAMHORN");
            break;
        case FBE_ESP_ENCL_TYPE_ANCHO:
            EnclType = (char *)("ANCHO");
            break;
        case FBE_ESP_ENCL_TYPE_VIKING:
            EnclType = (char *)("VIKING");
            break;
        case FBE_ESP_ENCL_TYPE_TABASCO:
            EnclType = (char *)("TABASCO");
            break;
        case FBE_ESP_ENCL_TYPE_CAYENNE:
            EnclType = (char *)("CAYENNE");
            break;
        case FBE_ESP_ENCL_TYPE_NAGA:
            EnclType = (char *)("NAGA");
            break;
        case FBE_ESP_ENCL_TYPE_PROTEUS:
            EnclType = (char *)("PROTEUS");
            break;
        case FBE_ESP_ENCL_TYPE_TELESTO:
            EnclType = (char *)("TELESTO");
            break;
        case FBE_ESP_ENCL_TYPE_CALYPSO:
            EnclType = (char *)("CALYPSO");
            break;
        case FBE_ESP_ENCL_TYPE_MIRANDA:
            EnclType = (char *)("MIRANDA");
            break;
        case FBE_ESP_ENCL_TYPE_RHEA:
            EnclType = (char *)("RHEA");
            break;
        default:
            EnclType = (char *)( "UNKWN");
            break;
    }
    return (EnclType);
} //End of fbe_cli_print_EnclosureType

/*!*************************************************************************
 * @fn fbe_cli_print_SpsCablingStatus
 *           (fbe_esp_encl_type_t encl_type)
 **************************************************************************
 *
 *  @brief
 *      This function will get int sps cabling status and convert into text.
 *
 *  @param    fbe_sps_cabling_status_t - sps_cabling_status
 *
 *  @return     char * - string
 *
 *  NOTES:
 *
 *  HISTORY:
 *    09-Jan-2012: Zhenhua Dong- created.
 *
 **************************************************************************/
char * fbe_cli_print_SpsCablingStatus(fbe_sps_cabling_status_t sps_cabling_status)
{
    char * cablingStatus;
    switch (sps_cabling_status)
    {
        case FBE_SPS_CABLING_VALID:
            cablingStatus = (char *)("VALID");
        break;
        case FBE_SPS_CABLING_INVALID_PE:
            cablingStatus = (char *)("INVALID_PE");
        break;
        case FBE_SPS_CABLING_INVALID_DAE0:
            cablingStatus = (char *)("INVALID_DAE0");
        break;
        case FBE_SPS_CABLING_INVALID_SERIAL:
            cablingStatus = (char *)("INVALID_SERIAL");
        break;
        case FBE_SPS_CABLING_INVALID_MULTI:
            cablingStatus = (char *)("INVALID_MULTI");
        break;
        default:
            cablingStatus = (char *)( "UNKNOWN");
        break;
    }
    return (cablingStatus);
} //End of fbe_cli_print_SpsCablingStatus

/*!*************************************************************************
 * @fn fbe_cli_print_SpsFaults
 *           (fbe_esp_encl_type_t encl_type)
 **************************************************************************
 *
 *  @brief
 *      This function will get int sps cabling status and convert into text.
 *
 *  @param    fbe_sps_cabling_status_t - sps_cabling_status
 *
 *  @return     char * - string
 *
 *  NOTES:
 *
 *  HISTORY:
 *    09-Jan-2012: Zhenhua Dong- created.
 *
 **************************************************************************/
char * fbe_cli_print_SpsFaults(fbe_sps_fault_info_t *spsFaultInfo)
{
    char * spsFaultString = (char *)("None");

    if (spsFaultInfo->spsModuleFault)
    {
        if (spsFaultInfo->spsChargerFailure)
        {
            spsFaultString = (char *)("ChargerFailure");
        }
        else if (spsFaultInfo->spsInternalFault)
        {
            spsFaultString = (char *)("InternalFault");
        }
    }
    else if (spsFaultInfo->spsBatteryFault)
    {
        if (spsFaultInfo->spsBatteryNotEngagedFault)
        {
            spsFaultString = (char *)("BatteryNotEngaged");
        }
        else if (spsFaultInfo->spsBatteryEOL)
        {
            spsFaultString = (char *)("BatteryEOL");
        }
    }

    return (spsFaultString);

} //End of fbe_cli_print_SpsFaults

/*!*************************************************************************
 * @fn fbe_cli_print_HwCacheStatus
 *           (fbe_common_cache_status_t hw_cache_status)
 **************************************************************************
 *
 *  @brief
 *      This function will get int sps cabling status and convert into text.
 *
 *  @param    fbe_sps_cabling_status_t - sps_cabling_status
 *
 *  @return     char * - string
 *
 *  NOTES:
 *
 *  HISTORY:
 *    09-Jan-2012: Zhenhua Dong- created.
 *
 **************************************************************************/
char * fbe_cli_print_HwCacheStatus(fbe_common_cache_status_t hw_cache_status)
{
    char * hwCacheStatus;
    switch (hw_cache_status)
    {
        case FBE_CACHE_STATUS_OK:
            hwCacheStatus = (char *)("OK");
            break;
        case FBE_CACHE_STATUS_DEGRADED:
            hwCacheStatus = (char *)("DEGRADED");
            break;
        case FBE_CACHE_STATUS_FAILED:
            hwCacheStatus = (char *)("FAILED");
            break;
        case FBE_CACHE_STATUS_FAILED_SHUTDOWN:
            hwCacheStatus = (char *)("SHUTDOWN");
            break;
        case FBE_CACHE_STATUS_FAILED_ENV_FLT:
            hwCacheStatus = (char *)("FAILED_EF");
            break;
        case FBE_CACHE_STATUS_FAILED_SHUTDOWN_ENV_FLT:
            hwCacheStatus = (char *)("SHUTDOWN_EF");
            break;
        case FBE_CACHE_STATUS_APPROACHING_OVER_TEMP:
            hwCacheStatus = (char *)("APPROACH_OT");
            break;
        case FBE_CACHE_STATUS_UNINITIALIZE:
            hwCacheStatus = (char *)("UNINIT");
            break;
        default:
            hwCacheStatus = (char *)( "UNKNOWN");
            break;
    }
    return (hwCacheStatus);
} //End of fbe_cli_print_HwCacheStatus
/*!**************************************************************
 * fbe_cli_convert_metadata_element_state_to_string()
 ****************************************************************
 * @brief
 *  This function converts object metadata element state on the SP to
 *  a text string.
 *
 * @param 
 *        fbe_metadata_element_state_t state
 * 
 * @return
 *        char *  A string for object metadata element state 
 * @author
 *  06/22/2012 - Created. Vera Wang
 ***************************************************************/
char * fbe_cli_convert_metadata_element_state_to_string(fbe_metadata_element_state_t state)
{
    switch(state)
    {
        case FBE_METADATA_ELEMENT_STATE_ACTIVE:
            return "FBE_METADATA_ELEMENT_STATE_ACTIVE";
        case FBE_METADATA_ELEMENT_STATE_PASSIVE:
            return "FBE_METADATA_ELEMENT_STATE_PASSIVE";
        default:
            return "FBE_METADATA_ELEMENT_STATE_INVALID";
    }
}
/******************************************
 * end fbe_cli_convert_metadata_element_state_to_string()
 ******************************************/
/*!**************************************************************
 * fbe_cli_display_metadata_element_state()
 ****************************************************************
 * @brief
 *  This function gets the metadata_element_state for the specific
 *  object and print it out.
 *
 * @param 
 *        fbe_object_id_t object_id
 * 
 * @return
 *        None. 
 * @author
 *  06/22/2012 - Created. Vera Wang
 ***************************************************************/
void fbe_cli_display_metadata_element_state(fbe_object_id_t object_id)
{
    fbe_metadata_info_t             sp_metadata_info;
    fbe_status_t                    status;  
    status = fbe_api_base_config_metadata_get_info(object_id, &sp_metadata_info);
    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("\nUnable to get metadata element state for this object.");
    }
    else{
        fbe_cli_printf("\n");
        fbe_cli_printf("Metadata element state on this SP: %s\n",
                       fbe_cli_convert_metadata_element_state_to_string(sp_metadata_info.metadata_element_state));
    }
    
}
/******************************************
 * end fbe_cli_display_metadata_element_state()
 ******************************************/

/******************************************
 * end fbe_cli_convert_metadata_element_state_to_string()
 ******************************************/
/*!**************************************************************
 * fbe_cli_display_sfp_media_type()
 ****************************************************************
 * @brief
 *  This function displays the SFP media type..
 *
 * @param 
 *        media_type
 * 
 * @return
 *        None. 
 * @author
 *  10/10/2012 - Created. Randall Porteus
 ***************************************************************/
void fbe_cli_display_sfp_media_type(fbe_port_sfp_media_type_t media_type)
{

    switch(media_type){
        case FBE_PORT_SFP_MEDIA_TYPE_COPPER:
            fbe_cli_printf("Copper\n");
            break;
        case FBE_PORT_SFP_MEDIA_TYPE_OPTICAL:
            fbe_cli_printf("Optical\n");
            break;
        case FBE_PORT_SFP_MEDIA_TYPE_NAS_COPPER:
            fbe_cli_printf("NAS Copper\n");
            break;
        case FBE_PORT_SFP_MEDIA_TYPE_UNKNOWN:
            fbe_cli_printf("Unknown\n");
            break;
        case FBE_PORT_SFP_MEDIA_TYPE_MINI_SAS_HD:
            fbe_cli_printf(" Mini SAS HD\n");
            break;
        default:
             fbe_cli_printf("Invalid\n");
   }
}
/******************************************
 * end fbe_cli_display_sfp_media_type()
 ******************************************/

/*!**************************************************************
 * fbe_cli_display_sfp_condition_type()
 ****************************************************************
 * @brief
 *  This function displays the SFP condition type..
 *
 * @param 
 *        condition_type
 * 
 * @return
 *        None. 
 * @author
 *  11/28/2013 - Created. zhoue1
 ***************************************************************/
void fbe_cli_display_sfp_condition_type(fbe_port_sfp_condition_type_t condition_type)
{
    switch(condition_type)
    {
        case FBE_PORT_SFP_CONDITION_GOOD:
            fbe_cli_printf("Good\n");
            break;
        case FBE_PORT_SFP_CONDITION_INSERTED:
            fbe_cli_printf("Inserted\n");
            break;
        case FBE_PORT_SFP_CONDITION_REMOVED:
            fbe_cli_printf("Removed\n");
            break;
        case FBE_PORT_SFP_CONDITION_FAULT:
            fbe_cli_printf("Fault\n");
            break;
        case FBE_PORT_SFP_CONDITION_WARNING:
            fbe_cli_printf("Warning\n");
            break;
        case FBE_PORT_SFP_CONDITION_INFO:
            fbe_cli_printf("Info\n");
            break;
        default:
             fbe_cli_printf("Invalid\n");
   }
}
/******************************************
 * end fbe_cli_display_sfp_condition_type()
 ******************************************/

/*!**************************************************************
 * fbe_cli_display_sfp_sub_condition_type()
 ****************************************************************
 * @brief
 *  This function displays the SFP sub condition type..
 *
 * @param 
 *        sub_condition_type
 * 
 * @return
 *        None. 
 * @author
 *  11/28/2013 - Created. zhoue1
 ***************************************************************/
void fbe_cli_display_sfp_sub_condition_type(fbe_port_sfp_sub_condition_type_t sub_condition_type)
{
    switch(sub_condition_type)
    {
        case FBE_PORT_SFP_SUBCONDITION_GOOD:
            fbe_cli_printf("Good\n");
            break;
        case FBE_PORT_SFP_SUBCONDITION_DEVICE_ERROR:
            fbe_cli_printf("Device error\n");
            break;
        case FBE_PORT_SFP_SUBCONDITION_CHECKSUM_PENDING:
            fbe_cli_printf("Checksum pending\n");
            break;
        case FBE_PORT_SFP_SUBCONDITION_UNSUPPORTED:
            fbe_cli_printf("Unsupported\n");
            break;
        default:
             fbe_cli_printf("None\n");
   }
}
/******************************************
 * end fbe_cli_display_sfp_sub_condition_type()
 ******************************************/


/*!*************************************************************************
* @void fbe_cli_get_speed_string(fbe_u32_t speed, char **stringBuf)
**************************************************************************
*
*  @brief
*  This function translate the speed from serial number to string.
*
*  @param    speed - the speed serial number.
*  @param    stringBuf - the speed string to print out.
*
*  @return   none
*
*  NOTES:
*
*  HISTORY:
*  23-Feb-2012 Eric Zhou - Created.
* **************************************************************************/
void fbe_cli_get_speed_string(fbe_u32_t speed, char **stringBuf)
{
    switch(speed)
    {
        case SPEED_ONE_GIGABIT:
            *stringBuf = "1G";
            break;
        case SPEED_TWO_GIGABIT:
            *stringBuf = "2G";
            break;
        case SPEED_FOUR_GIGABIT:
            *stringBuf = "4G";
            break;
        case SPEED_EIGHT_GIGABIT:
            *stringBuf = "8G";
            break;
        case SPEED_TEN_GIGABIT:
            *stringBuf = "10G";
            break;
        case SPEED_ONE_FIVE_GIGABIT:
            *stringBuf = "15G";
            break;
        case SPEED_THREE_GIGABIT:
            *stringBuf = "3G";
            break;
        case SPEED_SIX_GIGABIT:
            *stringBuf = "6G";
            break;
        case SPEED_TWELVE_GIGABIT:
            *stringBuf = "12G";
            break;
        case SPEED_TEN_MEGABIT:
            *stringBuf = "10M";
            break;
        case SPEED_100_MEGABIT:
            *stringBuf = "100M";
            break;
        default:
            *stringBuf = "UNKNOWN";
    }
}
/*!*************************************************************************
* @void fbe_cli_get_all_speeds_string(fbe_u32_t speed, char *stringBuf)
**************************************************************************
*
*  @brief
*  This function translate the speed bitmask into a concatenated list of speeds.
*
*  @param    speed - the speed serial number.
*  @param    stringBuf - the speed string to print out.
*
*  @return   none
*
*  NOTES:
*
*  HISTORY:
*  14-July-2015 Brion Philbin - Created.
* **************************************************************************/
void fbe_cli_get_all_speeds_string(fbe_u32_t speed, char *stringBuf)
{
    int	OneAlreadyPrinted = 0;

    if (speed  == SPEED_HARDWARE_DEFAULT) 
    {
        strncat(stringBuf, "Default", strlen("Default"));
    }
    else 
    {
        if (speed & SPEED_TEN_MEGABIT) {

                strncat(stringBuf, "10M", strlen("10M"));
                OneAlreadyPrinted++;
        }

        if (speed & SPEED_100_MEGABIT) {

                if (OneAlreadyPrinted) { strncat(stringBuf, ",", strlen(",")); }
                strncat(stringBuf, "100M", strlen("100M"));
                OneAlreadyPrinted++;
        }

        if (speed & SPEED_ONE_GIGABIT) {

                if (OneAlreadyPrinted) { strncat(stringBuf, ",", strlen(",")); }
                strncat(stringBuf, "1G", strlen("1G"));
                OneAlreadyPrinted++;
        }

        if (speed & SPEED_ONE_FIVE_GIGABIT) {

                if (OneAlreadyPrinted) { strncat(stringBuf, ",", strlen(",")); }
                strncat(stringBuf, "1.5G", strlen("1.5G"));
                OneAlreadyPrinted++;
        }

        if (speed & SPEED_TWO_GIGABIT) {

                if (OneAlreadyPrinted) { strncat(stringBuf, ",", strlen(",")); }
                strncat(stringBuf, "2G", strlen("2G"));
                OneAlreadyPrinted++;
        }

        if (speed & SPEED_THREE_GIGABIT) {

                if (OneAlreadyPrinted) { strncat(stringBuf, ",", strlen(",")); }
                strncat(stringBuf, "3G", strlen("3G"));
                OneAlreadyPrinted++;
        }

        if (speed & SPEED_FOUR_GIGABIT) {

                if (OneAlreadyPrinted) { strncat(stringBuf, ",", strlen(",")); }
                strncat(stringBuf, "4G", strlen("4G"));
                OneAlreadyPrinted++;
        }

        if (speed & SPEED_SIX_GIGABIT) {

                if (OneAlreadyPrinted) { strncat(stringBuf, ",", strlen(",")); }
                strncat(stringBuf, "6G", strlen("6G"));
                OneAlreadyPrinted++;
        }

        if (speed & SPEED_EIGHT_GIGABIT) {

                if (OneAlreadyPrinted) { strncat(stringBuf, ",", strlen(",")); }
                strncat(stringBuf, "8G", strlen("8G"));
                OneAlreadyPrinted++;
        }

        if (speed & SPEED_TEN_GIGABIT) {

                if (OneAlreadyPrinted) { strncat(stringBuf, ",", strlen(",")); }
                strncat(stringBuf, "10G", strlen("10G"));
                OneAlreadyPrinted++;
        }

        if (speed & SPEED_SIXTEEN_GIGABIT) {

                if (OneAlreadyPrinted) { strncat(stringBuf, ",", strlen(",")); }
                strncat(stringBuf, "16G", strlen("16G"));
                OneAlreadyPrinted++;
        }

        if (speed & SPEED_AUTO_SELECT) {

                if (OneAlreadyPrinted) { strncat(stringBuf, ",", strlen(",")); }
                strncat(stringBuf, "Auto", strlen("Auto"));
                OneAlreadyPrinted++;
        }

        if (!OneAlreadyPrinted) {
            strncat(stringBuf, "Unknown Speed", strlen("Unknown Speed"));
        }
    }
    return;
}

/*!**************************************************************
 * fbe_cli_sleep_at_prompt()
 ****************************************************************
 * @brief
 *  Sleep at the fbecli prompt for the input number of seconds.
 *  We expect a seconds value that is no more than half an hour.
 *
 * @param argc
 * @param argv          
 *
 * @return None
 *
 * @author
 *  1/31/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_cli_sleep_at_prompt(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_u32_t seconds;
    #define FBE_CLI_MAX_SLEEP_SECONDS 1800 /* This is arbitrary. */

    if (argc == 0)
    {
        fbe_cli_printf("Sleep: Requires a seconds argument\n");
        return;
    }

    seconds = (fbe_u32_t)strtoul(*argv, 0, 0);
    if (seconds > FBE_CLI_MAX_SLEEP_SECONDS)
    {
        fbe_cli_printf("Sleep: Seconds argument %d > Max sleep seconds %d\n",
                       seconds, FBE_CLI_MAX_SLEEP_SECONDS);
        return;
    }
    fbe_cli_printf("Delaying for %d seconds\n", seconds);
    fbe_api_sleep(seconds * FBE_TIME_MILLISECONDS_PER_SECOND);
}
/******************************************
 * end fbe_cli_sleep_at_prompt()
 ******************************************/

/*************************************************************************
 *                            @fn fbe_cli_decodeElapsedTime ()           *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_decodeElapsedTime - take elapsed time in seconds & return
 *   time in hours, minutes & seconds.
 *
 * @param :
 *   day      (INPUT) elapsedTimeInSecs value
 *   minute   (OUTPUT) decodedTime (hours, minutes, seconds) value
 *
 * @return:
 *   None
 *
 * @author
 *   23-Apr-2013 Created - Joe Perry
 *
 *************************************************************************/
void fbe_cli_decodeElapsedTime(fbe_u32_t elapsedTimeInSecs, fbe_system_time_t *decodedTime)
{

    decodedTime->hour = (elapsedTimeInSecs / 3600);
    elapsedTimeInSecs -= (decodedTime->hour * 3600);
    decodedTime->minute = (elapsedTimeInSecs / 60);
    elapsedTimeInSecs -= (decodedTime->minute * 60);
    decodedTime->second = elapsedTimeInSecs;

}   // end of fbe_cli_decodeElapsedTime

/*!*************************************************************************
 * @fn fbe_cli_convertStringToComponentType
 *           (char **componentString)
 **************************************************************************
 *
 *  @brief
 *      This function will convert Component Type strings to the
 *      correct enum value.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 *  NOTES:
 *
 *  HISTORY:
 *    13-Feb-2009: Joe Perry - created.
 *    13-May-2013: Chengkai  - move from fbe_cli_lib_enclosure_cmds.c to fbe_cli_lib_utils.c
 *
 **************************************************************************/
fbe_enclosure_component_types_t fbe_cli_convertStringToComponentType(char **componentString)
{
    fbe_enclosure_component_types_t componentType = FBE_ENCL_INVALID_COMPONENT_TYPE;

    if(strcmp(*componentString, "encl") == 0)
    {
        componentType = FBE_ENCL_ENCLOSURE;
    }
    else if(strcmp(*componentString, "lcc") == 0)
    {
        componentType = FBE_ENCL_LCC;
    }
    else if(strcmp(*componentString, "ps") == 0)
    {
        componentType = FBE_ENCL_POWER_SUPPLY;
    }
    else if(strcmp(*componentString, "cool") == 0)
    {
        componentType = FBE_ENCL_COOLING_COMPONENT;
    }
    else if(strcmp(*componentString, "drv") == 0)
    {
        componentType = FBE_ENCL_DRIVE_SLOT;
    }
    else if(strcmp(*componentString, "temp") == 0)
    {
        componentType = FBE_ENCL_TEMP_SENSOR;
    }
    else if(strcmp(*componentString, "temperature") == 0)
    {
        componentType = FBE_ENCL_TEMPERATURE;
    }
    else if(strcmp(*componentString, "conn") == 0)
    {
        componentType = FBE_ENCL_CONNECTOR;
    }
    else if(strcmp(*componentString, "exp") == 0)
    {
        componentType = FBE_ENCL_EXPANDER;
    }
    else if(strcmp(*componentString, "phy") == 0)
    {
        componentType = FBE_ENCL_EXPANDER_PHY;
    }
    else if(strcmp(*componentString, "disp") == 0)
    {
        componentType = FBE_ENCL_DISPLAY;
    }
    else if(strcmp(*componentString, "iomodule") == 0)  //Processor Enclosure component Type
    {
        componentType = FBE_ENCL_IO_MODULE;
    }
    else if(strcmp(*componentString, "ioport") == 0)
    {
        componentType = FBE_ENCL_IO_PORT;
    }
    else if(strcmp(*componentString, "bem") == 0)
    {
        componentType = FBE_ENCL_BEM;
    }
    else if(strcmp(*componentString, "mgmt") == 0)
    {
        componentType = FBE_ENCL_MGMT_MODULE;
    }
    else if(strcmp(*componentString, "mezzanine") == 0)
    {
        componentType = FBE_ENCL_MEZZANINE;
    }
    else if(strcmp(*componentString, "platform") == 0)
    {
        componentType = FBE_ENCL_PLATFORM;
    }
    else if(strcmp(*componentString, "suitcase") == 0)
    {
        componentType = FBE_ENCL_SUITCASE;
    }
    else if(strcmp(*componentString, "bmc") == 0)
    {
        componentType = FBE_ENCL_BMC;
    }
    else if(strcmp(*componentString, "misc") == 0)
    {
        componentType = FBE_ENCL_MISC;
    }
    else if(strcmp(*componentString, "fltreg") == 0)
    {
        componentType = FBE_ENCL_FLT_REG;
    }
    else if(strcmp(*componentString, "slaveport") == 0)
    {
        componentType = FBE_ENCL_SLAVE_PORT;
    }
    else if(strcmp(*componentString, "temperature") == 0)
    {
        componentType = FBE_ENCL_TEMPERATURE;
    }
    else if(strcmp(*componentString, "sps") == 0)
    {
        componentType = FBE_ENCL_SPS;
    }
    else if(strcmp(*componentString, "battery") == 0)
    {
        componentType = FBE_ENCL_BATTERY;
    }
    else if(strcmp(*componentString, "cachecard") == 0)
    {
        componentType = FBE_ENCL_CACHE_CARD;
    }
    else if(strcmp(*componentString, "dimm") == 0)
    {
        componentType = FBE_ENCL_DIMM;
    }
    else if(strcmp(*componentString, "ssd") == 0)
    {
        componentType = FBE_ENCL_SSD;
    }
    else if(strcmp(*componentString, "ssc") == 0)
    {
        componentType = FBE_ENCL_SSC;
    }

    return componentType;

}   // end of fbe_cli_convertStringToComponentType

/*!**************************************************************
* fbe_cli_create_read_uncorrectable
****************************************************************
* @brief
*  This function will create a read uncorrectable at
*  a given LBA for a specific SAS drive.
*
* @param object_id - object id to specify the SAS drive.
* @param lba - The LBA at which to create the read uncorrectable.
*
* @return fbe_status_t status of the operation.
*
* @author
*    08/21/2013: Dipak Patel - Ported from Joe B.'s code on R31 code base
****************************************************************/
fbe_status_t fbe_cli_create_read_uncorrectable(fbe_u32_t object_id, fbe_lba_t lba)
{
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_api_create_read_uncorrectable(object_id, lba, FBE_PACKET_FLAG_NO_ATTRIB);
    
    /* Handle the status.
     */

    if (status == FBE_STATUS_OK)
    {
        fbe_cli_printf("Successfully created read unc for object id 0x%x\n", object_id);
    }
    else
    {
        fbe_cli_printf("\n\nFailed to create read unc for object id 0x%x\n", object_id);
    }

    return status;
}


/*!**************************************************************
* fbe_cli_create_read_unc_range
****************************************************************
* @brief
*  This function will create several read uncorrectable over
*  a given range of LBA's for a specific SAS drive.
*
* @param object_id - object id to specify the SAS drive.
* @param start_lba - The starting LBA of the range over which
*                    to create the read uncorrectables.
* @param end_lba - The ending LBA of the range over which
*                  to create the read uncorrectables.
* @param inc_lba - The number of LBA's to increment between
*                  read uncorrectables.
*
* @return fbe_status_t status of the operation.
*
* @author
*    08/21/2013: Dipak Patel - Ported from Joe B.'s code on R31 code base*
****************************************************************/
fbe_status_t fbe_cli_create_read_unc_range(fbe_u32_t object_id, fbe_lba_t start_lba, fbe_lba_t end_lba, fbe_lba_t inc_lba)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t lba = 0;

    for (lba = start_lba; lba <= end_lba; lba += inc_lba)
    {
        status = fbe_api_create_read_uncorrectable(object_id, lba, FBE_PACKET_FLAG_NO_ATTRIB);
        /* Check the status.
        */
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\n\nFailed to create a range of read unc for object id 0x%x\n", object_id);
            return status;
        }
    }
    fbe_cli_printf("\n\nSucessfully created a range of uncorrectables for object id 0x%x\n", object_id);
    return status;
}

/*************************************************************************
 *                            @fn fbe_cli_strtoui64 ()
 *************************************************************************
 *
 * @brief
 *    Function to convert string to u64 integer.
 *    A wrapper to strtoui64 for easy use and error check.
 *
 * @param :
 *    str     string to convert
 *    value   pointer ot the integer
 *
 * @return:
 *    FBE_STATUS_OK:    success
 *    else:             str contains invalid character
 *
 * @author
 *    10/31/2013 - Created. Jamin Kang
 *************************************************************************/
fbe_status_t fbe_cli_strtoui64(const char *str, fbe_u64_t *value)
{
    char *end_ptr;
    fbe_u64_t temp;

    temp = _strtoui64(str, &end_ptr, 0);
    *value = temp;
    if (end_ptr == str || *end_ptr != '\0') {
        /* If find empty string or invalid character,
         * return failed
         */
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*************************************************************************
 *                            @fn fbe_cli_strtoui32 ()
 *************************************************************************
 *
 * @brief
 *    Function to convert string to u32 integer.
 *
 * @param :
 *    str     string to convert
 *    value   pointer ot the integer
 *
 * @return:
 *    FBE_STATUS_OK:    success
 *    else:             str contains invalid character
 *
 * @author
 *    10/31/2013 - Created. Jamin Kang
 *************************************************************************/
fbe_status_t fbe_cli_strtoui32(const char *str, fbe_u32_t *value)
{
    fbe_status_t status;
    fbe_u64_t v;

    status = fbe_cli_strtoui64(str, &v);
    *value = (fbe_u32_t)v;

    return status;
}


/*************************************************************************
 *                            @fn fbe_cli_strtoui16 ()
 *************************************************************************
 *
 * @brief
 *    Function to convert string to u16 integer.
 *
 * @param :
 *    str     string to convert
 *    value   pointer ot the integer
 *
 * @return:
 *    FBE_STATUS_OK:    success
 *    else:             str contains invalid character
 *
 * @author
 *    10/31/2013 - Created. Jamin Kang
 *************************************************************************/
fbe_status_t fbe_cli_strtoui16(const char *str, fbe_u16_t *value)
{
    fbe_status_t status;
    fbe_u64_t v;

    status = fbe_cli_strtoui64(str, &v);
    *value = (fbe_u16_t)v;

    return status;
}

/*************************************************************************
 *                            @fn fbe_cli_strtoui8 ()
 *************************************************************************
 *
 * @brief
 *    Function to convert string to u8 integer.
 *
 * @param :
 *    str     string to convert
 *    value   pointer ot the integer
 *
 * @return:
 *    FBE_STATUS_OK:    success
 *    else:             str contains invalid character
 *
 * @author
 *    10/31/2013 - Created. Jamin Kang
 *************************************************************************/
fbe_status_t fbe_cli_strtoui8(const char *str, fbe_u8_t *value)
{
    fbe_status_t status;
    fbe_u64_t v;

    status = fbe_cli_strtoui64(str, &v);
    *value = (fbe_u8_t)v;

    return status;
}

/*!***************************************************************************
 * fbe_cli_convert_drive_type_enum_to_string()
 ******************************************************************************
 * @brief
 *  Return a string for drive type.
 *
 * @param drive_type - Drive Type for the object.
 *
 * @return - char * string that represents drive type.
 *
 ****************************************************************/
char * fbe_cli_convert_drive_type_enum_to_string(fbe_drive_type_t drive_type)
{
    /* Drive Class is an enumerated type - print raw and interpreted value */
    switch(drive_type)
    {
        case FBE_DRIVE_TYPE_FIBRE:
            return("FIBRE");

         case FBE_DRIVE_TYPE_SAS:
            return("SAS");

         case FBE_DRIVE_TYPE_SATA:
            return("SATA");

         case FBE_DRIVE_TYPE_SAS_FLASH_HE:
            return ("SAS FLASH HE");

         case FBE_DRIVE_TYPE_SATA_FLASH_HE:
            return ("SATA FLASH HE");

        case FBE_DRIVE_TYPE_SAS_FLASH_ME:
            return ("SAS FLASH ME");

        case FBE_DRIVE_TYPE_SAS_FLASH_LE:
            return ("SAS FLASH LE");

        case FBE_DRIVE_TYPE_SAS_FLASH_RI:
            return ("SAS FLASH RI");

        case FBE_DRIVE_TYPE_SAS_NL:
            return ("SAS NL");

        default:
            break;
    }

    /* For default case */
    return("UNKNOWN");
}
/******************************************
 * end fbe_cli_convert_drive_type_enum_to_string()
 ******************************************/
/*************************
 * end file fbe_cli_src_utils.c
 *************************/
