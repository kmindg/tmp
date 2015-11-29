/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-09
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file pp_drive_commands.c
 ***************************************************************************
 *
 * File Description:
 *
 *   Debugging extensions for PhysicalPackage driver covering drive commands.
 *
 * Author:
 *   Armando Vega
 *
 * Revision History:
 *
 * Armando Vega 06/13/12  Initial version.
 *
 ***************************************************************************/
#include "pp_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "../src/services/topology/fbe_topology_private.h"

#include "fbe_base_object_debug.h"
#include "fbe/fbe_class.h"
#include "fbe_logical_drive_debug.h"
#include "fbe_eses_enclosure_debug.h"
#include "fbe_sas_enclosure_debug.h"
#include "fbe_sas_pmc_port_debug.h"
#include "fbe_base_physical_drive_debug.h"
#include "fbe_topology_debug.h"
#include "fbe_ordered_object_list.h"

typedef struct list_item_s{
    struct list_item_s *prev;
    struct list_item_s *next;
    fbe_dbgext_ptr data_ptr;
}list_item_t;

/* Global variabled required to support callback functions.
 */
fbe_port_number_t filter_port = FBE_API_PHYSICAL_BUS_COUNT;
fbe_enclosure_number_t filter_enclosure = FBE_API_ENCLOSURES_PER_BUS;
fbe_slot_number_t filter_slot = FBE_API_ENCLOSURE_SLOTS;
fbe_ordered_handle_t drive_list_p = NULL;

/*!**************************************************************
 * @fn compare_drive_for_insert()
 ****************************************************************
 * @brief
 *  Callback function that compares a new drive with a drive from
 *  the list in order to insert in ascending order.
 *
 * @param new_drive_ptr - Pointer to new drive.
 *
 * @param drive_from_list_ptr - Pointer to drive from list.
 *
 * 
 * @return compare_status_t = result of comparison.
 *
 * HISTORY:
 *  06/25/12 Armando Vega - Initial Version.
 *
 ****************************************************************/
compare_status_t compare_drive_for_insert(fbe_dbgext_ptr new_drive_ptr,
                                          fbe_dbgext_ptr drive_from_list_ptr)
{
    
    fbe_port_number_t new_drive_port;
    fbe_enclosure_number_t new_drive_enclosure;
    fbe_slot_number_t      new_drive_slot;    
    fbe_port_number_t list_item_port;
    fbe_enclosure_number_t list_item_enclosure;
    fbe_slot_number_t      list_item_slot;

    /* Get data from new drive.
     */
    FBE_GET_FIELD_DATA(pp_ext_physical_package_name,
                       new_drive_ptr,
                       fbe_base_physical_drive_t,
                       port_number,
                       sizeof(fbe_port_number_t),
                       &new_drive_port);

    FBE_GET_FIELD_DATA(pp_ext_physical_package_name,
                       new_drive_ptr,
                       fbe_base_physical_drive_t,
                       enclosure_number,
                       sizeof(fbe_enclosure_number_t),
                       &new_drive_enclosure);

    FBE_GET_FIELD_DATA(pp_ext_physical_package_name,
                       new_drive_ptr,
                       fbe_base_physical_drive_t,
                       slot_number,
                       sizeof(fbe_slot_number_t),
                       &new_drive_slot);
 
    /* Get data from drive provided from list.
     */ 
    FBE_GET_FIELD_DATA(pp_ext_physical_package_name,
                       drive_from_list_ptr,
                       fbe_base_physical_drive_t,
                       port_number,
                       sizeof(fbe_port_number_t),
                       &list_item_port);

    FBE_GET_FIELD_DATA(pp_ext_physical_package_name,
                       drive_from_list_ptr,
                       fbe_base_physical_drive_t,
                       enclosure_number,
                       sizeof(fbe_enclosure_number_t),
                       &list_item_enclosure);

    FBE_GET_FIELD_DATA(pp_ext_physical_package_name,
                      drive_from_list_ptr,
                      fbe_base_physical_drive_t,
                      slot_number,
                      sizeof(fbe_slot_number_t),
                      &list_item_slot);    

    /* Make comparison 
     */
    if ((new_drive_port == list_item_port) && 
        (new_drive_enclosure == list_item_enclosure))
    {
        if (new_drive_slot == list_item_slot)
        {
            /* We should never get here since not two drives could have same
             * port-enclosure-slot location, declare error.
             */
            csx_dbg_ext_print("%s Attempted to insert a drive with same "
                       "port-enclosure-slot than drive on list\n", __FUNCTION__);

            csx_dbg_ext_print("%s Duplicate address is %d-%d-%d \n", 
                    __FUNCTION__, new_drive_port, new_drive_enclosure, new_drive_slot);

            return COMPARE_ERROR;  // The caller may want to use this to quit

        }
        return (new_drive_slot > list_item_slot) ? COMPARE_HIGHER_THAN: COMPARE_LESS_THAN;
    }
    else
    {
        return ((new_drive_port > list_item_port) ||
                ((new_drive_port == list_item_port) && 
                  new_drive_enclosure > list_item_enclosure)) ? COMPARE_HIGHER_THAN: COMPARE_LESS_THAN;
    }
}

/*!**************************************************************
 * @fn print_drive()
 ****************************************************************
 * @brief
 * Call back function that prints filtered drive information.
 *
 * @param drive_ptr = Drive pointer obtained from walk.
 *
 * HISTORY:
 *  06/27/12 Armando Vega - Initial Version.
 *
 ****************************************************************/
void print_drive(fbe_dbgext_ptr drive_ptr)
{
    if (drive_ptr != NULL)
    {
        fbe_base_physical_drive_debug_trace(pp_ext_physical_package_name, drive_ptr, fbe_debug_trace_func, NULL);
        csx_dbg_ext_print("\n");
    }
}

#pragma data_seg ("EXT_HELP$4pppdo")
static char CSX_MAYBE_UNUSED usageMsg_pppdo[] =
"!pppdo\n"
"  By default display data for all physical drives.\n"
"  [-p port] [-e encl] [-s slot] - Target drive sub-set filtered by any combination of port,\n"
"                                  enclosure and slot with the command. List sorted by\n"
"                                  port-encl-slot.\n"
"  The parameters -p, -e and -s can be used together to display a single drive.\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(pppdo, "pppdo")
{
    fbe_dbgext_ptr topology_object_table_ptr = 0;
	fbe_topology_object_status_t  object_status;
	fbe_dbgext_ptr control_handle_ptr = 0;
	ULONG topology_object_table_entry_size;
	int i = 0;
	ULONG64 max_toplogy_entries_p = 0;
	fbe_u32_t max_entries;
	//fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_dbgext_ptr addr;
    fbe_u32_t object_status_offset;
    fbe_u32_t control_handle_offset;
    fbe_u32_t class_id_offset;
    fbe_u32_t port_number_offset;
    fbe_u32_t enclosure_number_offset;
    fbe_u32_t slot_number_offset;
    fbe_u32_t ptr_size;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ordered_status_e ordered_status = FBE_OBJ_LIST_RET_CODE_OK;
    fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    PCHAR str = NULL;
    
    /* Initialize global variables (in case they changed).
     */
    filter_port = FBE_API_PHYSICAL_BUS_COUNT;
    filter_enclosure = FBE_API_ENCLOSURES_PER_BUS;
    filter_slot = FBE_API_ENCLOSURE_SLOTS;
    drive_list_p = NULL;

    str = strtok( (char*)args, " \t" );
    while (str != NULL)
    {
        if(strncmp(str, "-p", 2) == 0)
        {
            str = strtok (NULL, " \t");
            if (str != NULL)
            {
                filter_port = (fbe_u32_t)strtoul(str, 0, 0);
            }
            else
            {
                csx_dbg_ext_print("%s Need to specify port number.\n", __FUNCTION__);
                status = FBE_STATUS_INVALID;
                return;
            }
        }
        else if(strncmp(str, "-e", 2) == 0)
        {
            str = strtok (NULL, " \t");
            if (str != NULL)
            {
                filter_enclosure = (fbe_u32_t)strtoul(str, 0, 0);             
            }
            else
            {
                csx_dbg_ext_print("%s Need to specify enclosure number.\n", __FUNCTION__);
                status = FBE_STATUS_INVALID;
                return;
            }
        }
        else if(strncmp(str, "-s", 2) == 0)
        {
            str = strtok (NULL, " \t");
            if (str != NULL)
            {
                filter_slot = (fbe_slot_number_t)strtoul(str, 0, 0);             
            }
            else
            {
                csx_dbg_ext_print("%s Need to specify slot number.\n", __FUNCTION__);
                status = FBE_STATUS_INVALID;
                return;
            }
        }
        else
        {
            csx_dbg_ext_print("%s Invalid parameter.\n", __FUNCTION__);
            status = FBE_STATUS_INVALID;
            return;
        }

        str = strtok (NULL, " \t");

    } // End while
    

    status = fbe_debug_get_ptr_size(pp_ext_physical_package_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        csx_dbg_ext_print("%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        status = FBE_STATUS_INVALID;
        return ; 
    }

    /* Create drive list.
     */
    ordered_status = fbe_ordered_object_list_create(FBE_OBJ_ORDER_TYPE_ASCEND, &drive_list_p);

    if (ordered_status != FBE_OBJ_LIST_RET_CODE_OK)
    {
        csx_dbg_ext_print("%s unable to create drive list.\n", __FUNCTION__);
        status = FBE_STATUS_INVALID;
        return;
    }

	/* Get the topology table.
     */
	FBE_GET_EXPRESSION(pp_ext_physical_package_name, topology_object_table, &topology_object_table_ptr);
	FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
	csx_dbg_ext_print("\t topology_object_table_ptr %llx\n", (unsigned long long)topology_object_tbl_ptr);

    /* Get the size of the topology entry.
     */
	FBE_GET_TYPE_SIZE(pp_ext_physical_package_name, topology_object_table_entry_t, &topology_object_table_entry_size);
	csx_dbg_ext_print("\t topology_object_table_entry_size %d \n", topology_object_table_entry_size);

	FBE_GET_EXPRESSION(pp_ext_physical_package_name, max_supported_objects, &max_toplogy_entries_p);
	FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

	if(topology_object_tbl_ptr == 0){
        fbe_ordered_object_list_destroy(drive_list_p);
		return;
	}

        FBE_GET_FIELD_OFFSET(pp_ext_physical_package_name, topology_object_table_entry_t,
                             "object_status", &object_status_offset);
        FBE_GET_FIELD_OFFSET(pp_ext_physical_package_name, topology_object_table_entry_t,
                              "control_handle", &control_handle_offset);
        FBE_GET_FIELD_OFFSET(pp_ext_physical_package_name, fbe_base_object_t,
                             "class_id", &class_id_offset);
        FBE_GET_FIELD_OFFSET(pp_ext_physical_package_name, fbe_base_object_t,
                             "port_number", &port_number_offset);
        FBE_GET_FIELD_OFFSET(pp_ext_physical_package_name, fbe_base_object_t,
                             "enclosure_number", &enclosure_number_offset);
        FBE_GET_FIELD_OFFSET(pp_ext_physical_package_name, fbe_base_object_t,
                             "slot_number", &slot_number_offset);

	object_status = 0;

	for(i = 0; i < max_entries ; i++)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            fbe_ordered_object_list_destroy(drive_list_p);
            return;
        }
        /* Fetch the object's topology status.
         */
        addr = topology_object_tbl_ptr + object_status_offset;
        FBE_READ_MEMORY(addr, &object_status, sizeof(fbe_topology_object_status_t));

        /* If the object's topology status is ready, then get the
         * topology status. 
         */
	if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
        {
            fbe_class_id_t         class_id;
            fbe_port_number_t      drive_port;
            fbe_enclosure_number_t drive_enclosure;
            fbe_slot_number_t      drive_slot;

            addr = topology_object_tbl_ptr + control_handle_offset;
            FBE_READ_MEMORY(addr, &control_handle_ptr, sizeof(fbe_dbgext_ptr));

            addr = control_handle_ptr + class_id_offset;
            FBE_READ_MEMORY(addr, &class_id, sizeof(fbe_class_id_t));

            /* Only display physical drive objects.
             */
                switch (class_id)
                {                
                    case FBE_CLASS_ID_BASE_PHYSICAL_DRIVE:
                    case FBE_CLASS_ID_SAS_PHYSICAL_DRIVE:
                    case FBE_CLASS_ID_SATA_PHYSICAL_DRIVE:
                    case FBE_CLASS_ID_FIBRE_PHYSICAL_DRIVE:
                    case FBE_CLASS_ID_SAS_FLASH_DRIVE:
                    case FBE_CLASS_ID_SATA_PADDLECARD_DRIVE:
                    case FBE_CLASS_ID_SATA_FLASH_DRIVE:

                        addr = control_handle_ptr + port_number_offset;
                        FBE_READ_MEMORY(addr, &drive_port, sizeof(fbe_port_number_t));

                        addr = control_handle_ptr + enclosure_number_offset;
                        FBE_READ_MEMORY(addr, &drive_enclosure, sizeof(fbe_enclosure_number_t));

                        addr = control_handle_ptr + slot_number_offset;
                        FBE_READ_MEMORY(addr, &drive_slot, sizeof(fbe_u16_t));

                    /*      Insert drive only if it is not filtered out by its port-enclosure-slot.
                     */     
                        if (((filter_port == FBE_API_PHYSICAL_BUS_COUNT) || (filter_port == drive_port)) &&
                            ((filter_enclosure == FBE_API_ENCLOSURES_PER_BUS) || (filter_enclosure == drive_enclosure)) &&
                            ((filter_slot == FBE_API_ENCLOSURE_SLOTS) || (filter_slot == drive_slot)))
                        {
                      /*    Insert drive into list.
                       */                          
                               ordered_status = fbe_ordered_object_list_insert(drive_list_p,
                                                                            control_handle_ptr,
                                                                            compare_drive_for_insert);                       
                        }

                    break;
                }
		} /* End if object's topology status is ready. */
		topology_object_tbl_ptr += topology_object_table_entry_size;
	}

    /* print physical drives, using filters if applicable.
     */
    fbe_ordered_object_list_walk(drive_list_p,print_drive,FBE_FALSE);

    fbe_ordered_object_list_destroy(drive_list_p);

	return;
}
