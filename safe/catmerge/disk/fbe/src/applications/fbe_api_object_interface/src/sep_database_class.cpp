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
*      This file defines the methods of the 
       SEP DATABASE INTERFACE class.
*
*  Notes:
*      The SEP Database class (sepDatabase) is a derived class of 
*      the base class (bSEP).
*
*  History:
*      21-July-2011 : initial version
*
*********************************************************************/

#ifndef T_SEP_DATABASE_CLASS_H
#include "sep_database_class.h"
#endif

/*********************************************************************
* sepDatabase class :: Constructor
*********************************************************************/

sepDatabase::sepDatabase() : bSEP()
{
    // Store Object number
    idnumber = (unsigned) SEP_DATABASE_INTERFACE;
    sepDatabaseCount = ++gSepDatabaseCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "SEP_DATABASE_INTERFACE");

    if (Debug) {
        sprintf(temp, 
            "sepDatabase class constructor (%d) %s\n", 
            idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP Database Interface function calls>
    sepDatabaseInterfaceFuncs = 
        "[function call]\n" \
        "[short name] [FBE API SEP Database Interface]\n" \
        " --------------------------------------------------------\n" \
        " getObjByLun       fbe_api_database_lookup_lun_by_number\n" \
        " getLunInfo        fbe_api_database_get_lun_info\n" \
        " getObjByRG        fbe_api_database_lookup_raid_group_by_number\n" \
        " getLunIdByObj     fbe_api_database_lookup_lun_by_object_id\n" \
        " getRgIdByObj      fbe_api_database_lookup_raid_group_by_object_id\n" \
        " --------------------------------------------------------\n" \
        " isSystemObject    fbe_api_database_is_system_object\n" \
        " setLoadBalance    fbe_api_database_set_load_balance\n"\
        " --------------------------------------------------------\n";

    // Define common usage for database commands
    usage_format = 
        " Usage: %s [object id]\n"
        " For example: %s 4\n";
};

/*********************************************************************
* sepDatabase class : Accessor methods
*********************************************************************/

unsigned sepDatabase::MyIdNumIs(void)
{
    return idnumber;
};

char * sepDatabase::MyIdNameIs(void)
{
    return idname;
};

void sepDatabase::dumpme(void)
{ 
    strcpy (key, "sepDatabase::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, sepDatabaseCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* sepDatabase Class :: select()
*********************************************************************
*
*  Description:
*      This function looks up the function short name to validate
*      it and then calls the function with the index passed back 
*      from the compare.
*            
*  Input: Parameters
*      index - index into arguments
*      argc  - argument count 
*      *argv - pointer to arguments 
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*
*  History
*      29-Mar-2011 : initial version
*
*********************************************************************/

fbe_status_t sepDatabase::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface"); 

    // List SEP Database calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepDatabaseInterfaceFuncs );

         // get LUN object id
    } else if (strcmp (argv[index], "GETOBJBYLUN") == 0) {
        status = get_lun_object_id(argc, &argv[index]);

        // get LUN ID from object
    }else if (strcmp (argv[index], "GETLUNIDBYOBJ") == 0) {
        status = get_lun_id_by_obj(argc, &argv[index]);

        // get lun info
    }else if (strcmp (argv[index], "GETLUNINFO") == 0) {
        status = get_lun_info(argc, &argv[index]);

        // get RG object id
    }else if (strcmp (argv[index], "GETOBJBYRG") == 0) {
        status = get_rg_object_id(argc, &argv[index]);

        // get RG ID from object
    }else if (strcmp (argv[index], "GETRGIDBYOBJ") == 0) {
        status = get_rg_id_by_obj(argc, &argv[index]);

        // check if object is system object
    } else if (strcmp (argv[index], "ISSYSTEMOBJECT") == 0) {
        status = is_system_object(argc, &argv[index]);

     // set load balancing
    } else if (strcmp (argv[index], "SETLOADBALANCE") == 0) {
        status = set_load_balance(argc, &argv[index]);
        // can't find match on short name
    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepDatabaseInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* sepDatabase class :: get_lun_object_id () 
*********************************************************************/

fbe_status_t sepDatabase::get_lun_object_id(int argc , char ** argv)
{
    // Assign default values
    lu_object_id = FBE_STATUS_GENERIC_FAILURE;

    // Define specific usage string  
    usage_format =
        " Usage: %s [Lun ID] \n"
        " For example: %s 1\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getObjByLun",
        "sepDatabase::get_lun_object_id",
        "fbe_api_database_lookup_lun_by_number",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert LUN ID to hex
    argv++;
    lun_number = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " lun number (%d)", lun_number);

    // Make api call: 
    status = fbe_api_database_lookup_lun_by_number(
        lun_number, &lu_object_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get object id for LUN %d", lun_number);
    } else {
        sprintf(temp, "<lu object id> 0x%X (%u)",
                lu_object_id,lu_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepDatabase class :: get_lun_id_by_obj () 
*********************************************************************/

fbe_status_t sepDatabase::get_lun_id_by_obj(int argc , char ** argv)
{
    // Assign default values
    lun_number = FBE_OBJECT_ID_INVALID;

    // Define specific usage string  
    usage_format =
        " Usage: %s [Lun Object ID] \n"
        " For example: %s 0x011\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getLunIdByObj",
        "sepDatabase::get_lun_id_by_obj",
        "fbe_api_database_lookup_lun_by_object_id",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert LUN ID to hex
    argv++;
    lu_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " lun Object ID 0x%x ",
            lu_object_id);

    // Make api call: 
    status = fbe_api_database_lookup_lun_by_object_id(
             lu_object_id, &lun_number);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get LUN ID for Object ID %d", lu_object_id);
    } else {
        sprintf(temp, "<LUN ID> %u (0x%X)",
                lun_number,lun_number);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepDatabase class :: get_lun_info () 
*********************************************************************/

fbe_status_t sepDatabase::get_lun_info(int argc , char ** argv)
{
    fbe_lun_number_t lun_number;
    char * data;

    // Define specific usage string  
    usage_format =
        " Usage: %s [Lun Object ID] \n"
        " For example: %s 0x011\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getLunInfo",
        "sepDatabase::get_lun_info",
        "fbe_api_database_get_lun_info",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    lu_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " lu object id 0x%x (%s)", lu_object_id, *argv);

    //  fbe_database_lun_info_t lun_info_l;
    lun_info.lun_object_id = lu_object_id;

    // get the lun number for display
    status = fbe_api_database_lookup_lun_by_object_id(lu_object_id,  &lun_number);
    if(status != FBE_STATUS_OK){
        sprintf(temp, "Can't lookup the  LUN 0x%X",
            lu_object_id);
         call_post_processing(status, temp, key, params); 
         return status;
    }

    // Make api call
    status = fbe_api_database_get_lun_info(&lun_info);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get information for LUN %d (0x%X)",
                lun_number, lu_object_id);
        data = temp;
    } else {
        // Make a call to get the drives associated with this LUN
        status = get_drives_by_rg (lun_info.raid_group_obj_id, &lu_drives);
    }

    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get information about drives for "\
                "LUN %d (0x%X)", lun_number, lu_object_id);
        data = temp;
    } else {
        data = edit_lun_info(&lun_info, &lu_drives);
    }

    // Output results from call or description of error
    call_post_processing(status, data, key, params); 

    return status;
}

/*********************************************************************
* sepDatabase class :: get_rg_object_id () 
*********************************************************************/

fbe_status_t sepDatabase::get_rg_object_id(int argc , char ** argv)
{
    // Assign default values
    rg_object_id = FBE_OBJECT_ID_INVALID;

    // Define specific usage string  
    usage_format =
        " Usage: %s [RG ID] \n"
        " For example: %s 1\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getObjByRG",
        "sepDatabase::get_rg_object_id",
        "fbe_api_database_lookup_raid_group_by_number",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert RG ID to hex
    argv++;
    raid_group_num = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " Raid Group number (%d)", raid_group_num);

    // Make api call: 
    status = fbe_api_database_lookup_raid_group_by_number(
                                raid_group_num, &rg_object_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get object id for Raid Group %d",
                raid_group_num);
    } else {
        sprintf(temp, "<RG object id> 0x%X (%u)",
                rg_object_id, rg_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepDatabase class :: get_rg_id_by_obj () 
*********************************************************************/

fbe_status_t sepDatabase::get_rg_id_by_obj(int argc , char ** argv)
{
    // Assign default values
    raid_group_num = FBE_RAID_ID_INVALID;

    // Define specific usage string  
    usage_format =
        " Usage: %s [RG OBJ ID] \n"
        " For example: %s 0x10a\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getRgIdByObj",
        "sepDatabase::get_rg_id_by_obj",
        "fbe_api_database_lookup_raid_group_by_object_id",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert RG ID to hex
    argv++;
    rg_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " Raid Group Object ID 0x%x ",
            rg_object_id);

    // Make api call: 
    status = fbe_api_database_lookup_raid_group_by_object_id(
             rg_object_id, &raid_group_num);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get RG ID for Object ID %d", rg_object_id);
    } else {
        sprintf(temp, "<Raid Group ID> %u(0x%X) ",
                raid_group_num, raid_group_num);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepDatabase Class :: is_system_object()
*********************************************************************
*
*  Description:
*      Checks if object is system object or not
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepDatabase::is_system_object(
    int argc, 
    char **argv) 
{
    // Assign default values
    object_id = FBE_OBJECT_ID_INVALID;
    b_found = FBE_FALSE;

    // Check that all arguments have been entered
    status = call_preprocessing(
        "isSystemObject",
        "sepDatabase::is_system_object",
        "fbe_api_database_is_system_object",  
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " object id 0x%x (%s)", object_id, *argv);
    
    // Make api call: 
    status = fbe_api_database_is_system_object(object_id, &b_found);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to check if object (id: 0x%X) "
                      "is system object",
                      object_id);

    }else if (status == FBE_STATUS_OK){ 
        edit_is_system_object(b_found);
    }     

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepDatabase class :: edit_lun_info (private method)
*********************************************************************/

char * sepDatabase::edit_lun_info(fbe_database_lun_info_t *lun_info, fbe_apix_rg_drive_list_t *lu_drives) 
{
    fbe_u32_t rg_width;
    fbe_u8_t index;
    
    char *data = NULL;
    char *mytemp = NULL;

    // format LUN information
    sprintf(temp,              "\n"
        "<Name>                          %s\n"
        "<Lun Characteristics>           %d \n"
        "<RAID Type>                     %s\n"
        "<Exported Capacity>             0x%llX\n"
        "<Offset>                        0x%llX\n"
        "<State>                         %d [%s]\n"
        "<WWN>                           ",
        (char*)lun_info->user_defined_name.name,
        lun_info->lun_characteristics,
        utilityClass::convert_rg_type_to_string(lun_info->raid_info.raid_type),
        lun_info->capacity,
        lun_info->offset,
        lun_info->lifecycle_state, utilityClass::lifecycle_state(lun_info->lifecycle_state));
        data = utilityClass::append_to (data, temp);
    for(index = 0; index < FBE_WWN_BYTES; index++)
    {
        sprintf(temp,"%02x",(fbe_u32_t)lun_info->world_wide_name.bytes[index]);
        data = utilityClass::append_to (data, temp);
        if(index+1 != FBE_WWN_BYTES){
            sprintf(temp,":");
            data = utilityClass::append_to (data, temp);
        }
    }

    sprintf(temp,
        "\n<Rotational Rate>               %d\n"
        "<RAID Group Obj ID>             0x%x\n",
        lun_info->rotational_rate,
        lun_info->raid_group_obj_id);

    data = utilityClass::append_to (data, temp);

    sprintf (temp, "<Drives associated with this LUN> ");

    data = utilityClass::append_to (data, temp);

    //calculating rg width for raid1_0
    if (lun_info->raid_info.raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        rg_width = lun_info->raid_info.width*2;
    }
    else
    {
        rg_width = lun_info->raid_info.width;
    }

    for(index = 0; index < rg_width; index++)
    {
        mytemp = new char[50];
        sprintf (mytemp, 
                "%d_%d_%d ",
                lu_drives->drive_list[index].port_num,
                lu_drives->drive_list[index].encl_num,
                lu_drives->drive_list[index].slot_num
                );

        data = utilityClass::append_to(data, mytemp);

        delete [] mytemp;
    }

    for(index = 0; index < rg_width; index++)
    {
        mytemp = new char[50];
        sprintf (mytemp, 
                "\n<Rebuild Chkpt for %d_%d_%d> 0x%llX", 
                lu_drives->drive_list[index].port_num,
                lu_drives->drive_list[index].encl_num,
                lu_drives->drive_list[index].slot_num,
                (unsigned long long)lun_info->raid_info.rebuild_checkpoint[index]
                );

        data = utilityClass::append_to(data, mytemp);

        delete [] mytemp;
    }

    return data;
}

/*********************************************************************
* sepDatabase Class :: edit_is_system_object() (private method)
*********************************************************************
*
*  Description:
*      Display if the object is system object or not
*
*  Input: Parameters
*      b_found - system object indicator

*  Returns:
*      void
*********************************************************************/

void sepDatabase::edit_is_system_object(fbe_bool_t b_found)
{
    // virtual drive spare information
    sprintf(temp,                       "\n "
        "<Is System Object>             %s\n ",
        b_found? "Yes" : "No");
}

/*********************************************************************
* sepDatabase::Sep_Database_Intializing_variable (private method)
*********************************************************************/
void sepDatabase::Sep_Database_Intializing_variable()
{
    object_id = FBE_OBJECT_ID_INVALID;
    b_found = FBE_FALSE;
    fbe_zero_memory(&lun_info,sizeof(lun_info));
    fbe_zero_memory(&lu_drives,sizeof(lu_drives));
    return;
}

/*********************************************************************
* sepDatabase class :: set_load_balance(int argc, char **argv)
*********************************************************************
*
*  Description:
*      Set the load balancing
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t  sepDatabase::set_load_balance(int argc, char **argv)
{

    // Define specific usage string  
    usage_format =
        " Usage: %s [load balancing state: 0|1] \n"
        " 0 -disable, 1 - enable \n"
        " For example: %s 1\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setLoadBalance",
        "sepDatabase::set_load_balance",
        "fbe_api_database_set_load_balance",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // get the load balancing state
    argv++;
    is_enable = (fbe_bool_t)strtoul(*argv, 0, 0);
    sprintf(params, "load balance state (%d) ",
            is_enable);

    // Make api call: 
    status = fbe_api_database_set_load_balance(is_enable);

    // Check status of call
    if (status == FBE_STATUS_OK) {
        if(is_enable){
             sprintf(temp, "Load balancing enabled successfully");
        }
        else{
             sprintf(temp, "Load balancing disabled successfully");
        }
    } else {
        sprintf(temp, "Failed to set load balancing");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;

}
/*********************************************************************
* sepDatabase end of file
*********************************************************************/
