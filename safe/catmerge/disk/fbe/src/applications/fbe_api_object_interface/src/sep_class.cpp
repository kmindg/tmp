/*********************************************************************
* Copyright (C) EMC Corporation, 1989 - 2011
* All Rights Reserved.
* Licensed Material-Property of EMC Corporation.
* This software is made available solely pursuant to the terms
* of a EMC license agreement which governs its use.
*********************************************************************/

/*********************************************************************
*
*  Description:
*      This file defines the methods of the SEP class.
*
*  Notes:
*      This is the base class for all of the SEP package
*      interface classes. It is also a derived class of the
*      Object class.
*
*  History:
*      07-March-2011 : initial version
*
*********************************************************************/

#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif

/*********************************************************************
*  bSEP Class :: Constructors
*********************************************************************
*
*  Description:
*      This section contains the following methods:
*          bSEP(FLAG f) : Object(f) - Initial constructor call
*          bSEP() - constructor
*
*  Input: Parameters
*      bSEP(FLAG f) : Object(f) - first time call (overload).
*
*  Output: Console
*      debug messages
*
* Returns:
*      Nothing
*
*  History
*      07-March-2011 : initial version
*
*********************************************************************/

bSEP::bSEP(FLAG f) : Object(f)
{
    idnumber = (unsigned) SEP_PACKAGE;
    sepCount = ++gSepCount;
    Sep_Intializing_variable();

    if (Debug) {
        sprintf(temp, "bSEP::bSEP %s (%d)\n",
            "initial class constructor1", idnumber);
        vWriteFile(dkey, temp);
    }
};

bSEP::bSEP() //: Object()
{
    idnumber = (unsigned) SEP_PACKAGE;
    sepCount = ++gSepCount;
    Sep_Intializing_variable();

    if (Debug) {
        sprintf(temp, "bSEP::bSEP %s (%d)\n",
            "class constructor2", idnumber);
        vWriteFile(dkey, temp);
    }
};

/*********************************************************************
* bSEP::Sep_Intializing_variable (private method)
*********************************************************************/
void bSEP::Sep_Intializing_variable()
{
    
    lu_object_id = 0;
    pvd_object_id = 0;
    rg_object_id = 0;
    vd_object_id = 0;
    lun_number = 0;
    raid_group_num = 0;
    status = FBE_STATUS_FAILED;
    passFail = FBE_STATUS_FAILED;
    enabled = FBE_FALSE;
    c = 0;
    return;
}

/*********************************************************************
* bSEP Class :: Accessor methods - simple functions
*********************************************************************
*
*  Description:
*      This section contains various accessor functions that return
*      or display object class data stored in the object.
*      These methods are simple function calls with
*      no arguments.
*
*  Input: Parameters
*      None - for all functions
*
*  Output: Console
*      dumpme()- displays info about the object
*
* Returns:
*      MyIdNumIs()     - Object id.
*
*  History
*      07-March-2011 : initial version
*
*********************************************************************/

unsigned bSEP::MyIdNumIs(void)
{
    return idnumber;
}

void bSEP::dumpme(void)
{
    strcpy (key, "bSEP::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n",
            idnumber, sepCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* bSEP Class :: HelpCmds()
*********************************************************************
*
*  Description:
*      This function packages up any passed in help lines with the
*      list of SEP class interface help commands, which is
*      sent to the Usage function.
*
*  Input: Parameters
*      lines - help data sent from an interface method
*
*  Output: Console
*      - Help with list of the SEP class interfaces.
*      - If called from a interface, it will include a list of the
*        interface FBA API functions and assocaiated short name.
*
* Returns:
*      nothing
*
*  History
*      07-March-2011 : initial version
*
*********************************************************************/

void bSEP::HelpCmds(char * lines)
{
    char * Interfaces =
        "[SEP Interface]\n" \
        " -BIND             Bind LUN\n" \
        " -DATABASE         Database\n" \
        " -LUN              LUN\n" \
        " -PANICSP          PanicSP\n" \
        " -POWERSAVING      Power Saving\n" \
        " -PVD              Provision Drive\n" \
        " -RG               RAID Group\n" \
        " -BVD              Base Volume Drive\n" \
        " -CMI              CMI\n" \
        " -VD               Virtual Drive\n" \
        " -JOBSERVICE       Job Service\n" \
        " -SCHEDULER        Scheduler debug hook\n"\
        " -BASECONFIG       Base Config\n"\
        " -LEI              Logical error injection\n"
        " -SYSBGSERVICE     System Bg Service\n"\
        " -METADATA         Metadata\n"\
        " -SYSTIMETHRESHOLD System Time Threshold\n"\
        "--------------------------\n";

    unsigned len = unsigned(strlen(lines) + strlen(Interfaces));
    char * temp = new char[len+1];
    strcpy (temp, (char *) Interfaces);
    strcat (temp, lines);
    Usage(temp);

    return;
}

/*********************************************************************
* bSEP class :: get_drives_by_rg (private method)
*********************************************************************/
fbe_status_t bSEP::get_drives_by_rg(fbe_object_id_t rg_object_id ,
                                    fbe_apix_rg_drive_list_t *rg_drives)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       index, pvd_obj_indx, disk_index, vd_index, loop_count;
    fbe_api_base_config_downstream_object_list_t downstream_vd_object_list;
    fbe_api_base_config_downstream_object_list_t downstream_pvd_object_list;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_api_base_config_downstream_object_list_t object_list;
    fbe_class_id_t                               class_id = FBE_CLASS_ID_INVALID;
    fbe_bool_t                                   is_system_rg = FBE_FALSE;
    fbe_api_virtual_drive_get_configuration_t    configuration_mode;

    vd_index = 0;
    loop_count = 0;

    // initialize the downstream object list.
    downstream_vd_object_list.number_of_downstream_objects = 0;
    for(index = 0; index < FBE_MAX_DOWNSTREAM_OBJECTS; index++)
    {
        downstream_vd_object_list.downstream_object_list[index] = FBE_OBJECT_ID_INVALID;
    }

    downstream_pvd_object_list.number_of_downstream_objects = 0;
    for(index = 0; index < FBE_MAX_DOWNSTREAM_OBJECTS; index++)
    {
        downstream_pvd_object_list.downstream_object_list[index] = FBE_OBJECT_ID_INVALID;
    }

    downstream_object_list.number_of_downstream_objects = 0;
    for(index = 0; index < FBE_MAX_DOWNSTREAM_OBJECTS; index++)
    {
        downstream_object_list.downstream_object_list[index] = FBE_OBJECT_ID_INVALID;
    }

    object_list.number_of_downstream_objects = 0;
    for(index = 0; index < FBE_MAX_DOWNSTREAM_OBJECTS; index++)
    {
        object_list.downstream_object_list[index] = FBE_OBJECT_ID_INVALID;
    }

    // Get Virtual Drives associated with this RG
    status = fbe_api_base_config_get_downstream_object_list(
             rg_object_id, &downstream_object_list);
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        return status;
    }

    /* 1) check whether it is a system rg or not
      * 2) deal with it accordingly as for system raid group vd object is 
      * absent
    */
    fbe_api_database_is_system_object(rg_object_id, &is_system_rg); 
    if(is_system_rg)
    {
        for(index = 0; index < downstream_object_list.number_of_downstream_objects ;index ++)
        {
            downstream_pvd_object_list.downstream_object_list[index] = downstream_object_list.downstream_object_list[index];
            downstream_pvd_object_list.number_of_downstream_objects = downstream_object_list.number_of_downstream_objects;
        }

        for(pvd_obj_indx = 0;
            pvd_obj_indx < downstream_pvd_object_list.number_of_downstream_objects;
            pvd_obj_indx++)
        {
            /* every PVD will have port, enclosure and slot which will correspond to drives in RG */
            /* Get the location details of the drives. */
            /* Using index variable in array subscript below will have the drive stored in consecutive
             * array location otherwise drive in position 0(may be other position as well)
             * will be overwritten every time.
             */

            // Get pvd object
            status  = fbe_api_provision_drive_get_location(
                        downstream_pvd_object_list.downstream_object_list[pvd_obj_indx],
                        &rg_drives->drive_list[pvd_obj_indx].port_num,
                        &rg_drives->drive_list[pvd_obj_indx].encl_num,
                        &rg_drives->drive_list[pvd_obj_indx].slot_num);
            if (status != FBE_STATUS_OK)
            {
                return status;
            }
        }
    }
    else
    {
        // Find out if given obj ID is for RAID group or Vd  
        status = fbe_api_get_object_class_id(downstream_object_list.downstream_object_list[0], &class_id, FBE_PACKAGE_ID_SEP_0);
        //if status is generic failure then return that back 
        if (status == FBE_STATUS_GENERIC_FAILURE)
        {
            return status;
        }
        //We do this for Raid type R1_0 as we have intermidiate raid object 
        // between raid10 object and vd object.
        if(class_id != FBE_CLASS_ID_VIRTUAL_DRIVE)
        {
            for(index = 0; index < downstream_object_list.number_of_downstream_objects ;index ++)
            {
                status = fbe_api_base_config_get_downstream_object_list(downstream_object_list.downstream_object_list[index], &object_list);
                if (status == FBE_STATUS_GENERIC_FAILURE)
                {
                    return status;
                }

                for(loop_count = 0; loop_count< object_list.number_of_downstream_objects ;loop_count++)
                {
                    downstream_vd_object_list.downstream_object_list[vd_index] = object_list.downstream_object_list[loop_count];
                    vd_index++;
                }
            }
            downstream_vd_object_list.number_of_downstream_objects = vd_index;
        }
        else
        {
            for(index = 0; index < downstream_object_list.number_of_downstream_objects ;index ++)
            {
                downstream_vd_object_list.downstream_object_list[index] = downstream_object_list.downstream_object_list[index];
            }
            downstream_vd_object_list.number_of_downstream_objects = downstream_object_list.number_of_downstream_objects;
        }
        
        //set disk index to zero
        disk_index = 0;
        // For every Virtual Drive find the corresponding PVD 
        for (index = 0; index < downstream_vd_object_list.number_of_downstream_objects; index++)
        {
            status = fbe_api_base_config_get_downstream_object_list(
                     downstream_vd_object_list.downstream_object_list[index],
                     &downstream_pvd_object_list);
            if (status == FBE_STATUS_GENERIC_FAILURE)
            {
                return status;
            }

            status = fbe_api_virtual_drive_get_configuration(
                     downstream_vd_object_list.downstream_object_list[index], 
                     &configuration_mode);
            if (status == FBE_STATUS_GENERIC_FAILURE)
            {
                return status;
            }

            // Question - each VD had one PVD , so why loop around ?
            // Ans - Copy or Spare may be undergoing
            for(pvd_obj_indx = 0;
                pvd_obj_indx < downstream_pvd_object_list.number_of_downstream_objects;
                pvd_obj_indx++)
            {
                /* every PVD will have port, enclosure and slot which will correspond to drives in RG */
                /* Get the location details of the drives. */
                /* Using index variable in array subscript below will have the drive stored in consecutive
                 * array location otherwise drive in position 0(may be other position as well)
                 * will be overwritten every time.
                 */

                if ((configuration_mode.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE && pvd_obj_indx == SECOND_EDGE_INDEX) ||
                    (configuration_mode.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE && pvd_obj_indx == FIRST_EDGE_INDEX))
                {
                    continue;
                }

                // Get pvd object
                status  = fbe_api_provision_drive_get_location(
                            downstream_pvd_object_list.downstream_object_list[pvd_obj_indx],
                            &rg_drives->drive_list[disk_index].port_num,
                            &rg_drives->drive_list[disk_index].encl_num,
                            &rg_drives->drive_list[disk_index].slot_num);
                if (status != FBE_STATUS_OK)
                {
                    return status;
                }
                disk_index++;
            }
        }
    }

    return status;
}

/*********************************************************************
* bSEP end of file
*********************************************************************/
