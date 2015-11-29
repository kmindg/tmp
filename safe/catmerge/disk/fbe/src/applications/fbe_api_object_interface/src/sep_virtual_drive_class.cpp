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
       SEP VIRTUAL DRIVE INTERFACE class.
*
*  Notes:
*      The SEP Virtual Drive class (sepVirtualDrive) is a derived class of 
*      the base class (bSEP).
*
*  History:
*      20-July-2011 : inital version
*
*********************************************************************/

#ifndef T_SEP_VIRTUAL_DRIVE_CLASS_H
#include "sep_virtual_drive_class.h"
#endif

/*********************************************************************
* sepVirtualDrive class :: Constructor
*********************************************************************/

sepVirtualDrive::sepVirtualDrive() : bSEP()
{
    // Store Object number
    idnumber = (unsigned) SEP_VIRTUAL_DRIVE_INTERFACE;
    vdrCount = ++gSepVirtualDriveCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "SEP_VIRTUAL_DRIVE_INTERFACE");

    //initialize the variables
    sep_virtual_drive_initializing_variable();

    if (Debug) {
        sprintf(temp, 
            "sepVirtualDrive class constructor (%d) %s\n", 
            idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP Virtual Drive Interface function calls>
    sepVirtualDriveInterfaceFuncs = 
        "[function call]\n" \
        "[short name] [FBE API SEP Virtual Drive Interface]\n" \
        " --------------------------------------------------------\n" \
        " getVdSpareInfo             fbe_api_virtual_drive_get_spare_info\n" \
        " getUnusedDriveAsSpareFlag  fbe_api_virtual_drive_get_unused_drive_as_spare_flag\n" \
        " setUnusedDriveAsSpareFlag  fbe_api_virtual_drive_set_unused_drive_as_spare_flag\n" \
        " updateSwapInTime           fbe_api_update_permanent_spare_swap_in_time \n"\
        " controlAutomaticHotSpare   fbe_api_control_automatic_hot_spare\n"\
        " --------------------------------------------------------\n";

    // Define common usage for virtual drive commands
    usage_format = 
        " Usage: %s [vd object id]\n"
        " For example: %s 4";
};

/*********************************************************************
* sepVirtualDrive class : Accessor methods
*********************************************************************/

unsigned sepVirtualDrive::MyIdNumIs(void)
{
    return idnumber;
};

char * sepVirtualDrive::MyIdNameIs(void)
{
    return idname;
};

void sepVirtualDrive::dumpme(void)
{ 
    strcpy (key, "sepVirtualDrive::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, vdrCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* sepVirtualDrive Class :: select()
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
*      29-Mar-2011 : inital version
*
*********************************************************************/

fbe_status_t sepVirtualDrive::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface"); 

    // List SEP Virtual Drive calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepVirtualDriveInterfaceFuncs );

        // get virtual drive spare info
    } else if (strcmp (argv[index], "GETVDSPAREINFO") == 0) {
        status = get_virtual_drive_spare_info(argc, &argv[index]);

        // get unused drive as spare flag
    } else if (strcmp (argv[index], "GETUNUSEDDRIVEASSPAREFLAG") == 0) {
        status = get_unused_drive_as_spare_flag(argc, &argv[index]);

        // get unused drive as spare flag
    } else if (strcmp (argv[index], "SETUNUSEDDRIVEASSPAREFLAG") == 0) {
        status = set_unused_drive_as_spare_flag(argc, &argv[index]);

    } else if (strcmp (argv[index], "UPDATESWAPINTIME") == 0) {
        status = update_permanent_spare_swap_in_time(argc, &argv[index]);

    } else if (strcmp (argv[index], "CONTROLAUTOMATICHOTSPARE") == 0) {
        status = set_control_automatic_hot_spare(argc, &argv[index]);

        // can't find match on short name
    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepVirtualDriveInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* sepVirtualDrive Class :: get_virtual_drive_spare_info()
*********************************************************************
*
*  Description:
*      Gets the virtual drive spare information for a given vd
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepVirtualDrive::get_virtual_drive_spare_info(
    int argc, 
    char **argv) 
{
    // Assign default values
    vd_object_id = FBE_OBJECT_ID_INVALID;

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getVdSpareInfo",
        "sepVirtualDrive::get_virtual_drive_spare_info",
        "fbe_api_virtual_drive_get_spare_info",  
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    vd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " vd object id 0x%x (%s)", vd_object_id, *argv);

    // Make api call: 
    status = fbe_api_virtual_drive_get_spare_info(
                vd_object_id, 
                &spare_drive_info);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get the spare drive info for "\
                      "vd object id 0x%X",
                      vd_object_id);

    }else if (status == FBE_STATUS_OK){ 
        edit_virtual_drive_spare_info(&spare_drive_info);
    }     

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepVirtualDrive Class :: get_unused_drive_as_spare_flag()
*********************************************************************
*
*  Description:
*     Get the unused_drive_as_spare flag of virtual drive object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepVirtualDrive::get_unused_drive_as_spare_flag(
    int argc, 
    char **argv) 
{
    // Assign default values
    vd_object_id = FBE_OBJECT_ID_INVALID;

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getUnusedDriveAsSpareFlag",
        "sepVirtualDrive::get_unused_drive_as_spare_flag",
        "fbe_api_virtual_drive_get_unused_drive_as_spare_flag",  
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    vd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " vd object id 0x%x (%s)", vd_object_id, *argv);

    // Make api call: 
    status = fbe_api_virtual_drive_get_unused_drive_as_spare_flag(
                vd_object_id, 
                &unsued_drive_as_spare_flag);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get unused drive as spare flag for "\
                      "vd object id 0x%X",
                      vd_object_id);

    }else if (status == FBE_STATUS_OK){ 
        edit_unused_drive_as_spare_flag(unsued_drive_as_spare_flag);
    }     

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepVirtualDrive Class :: set_unused_drive_as_spare_flag()
*********************************************************************
*
*  Description:
*     Set the unused_drive_as_spare flag of virtual drive object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepVirtualDrive::set_unused_drive_as_spare_flag(
    int argc, 
    char **argv) 
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [vd object id] [unused drive as spare flag: 0|1]\n"
        " For example: %s 4 1";

    // Assign default values
    vd_object_id = FBE_OBJECT_ID_INVALID;

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setUnusedDriveAsSpareFlag",
        "sepVirtualDrive::set_unused_drive_as_spare_flag",
        "fbe_api_virtual_drive_set_unused_drive_as_spare_flag",  
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    vd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " vd object id 0x%x (%s)", vd_object_id, *argv);

    // Extract unused drive as spare flag
    argv++;
    unsued_drive_as_spare_flag = (fbe_bool_t)strtoul(*argv, 0, 0);
    sprintf(params, " unused drive as spare flag 0x%x", 
        unsued_drive_as_spare_flag);

    // Make api call: 
    status = fbe_api_virtual_drive_set_unused_drive_as_spare_flag(
                vd_object_id, 
                unsued_drive_as_spare_flag);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to set unused drive as spare flag for "\
                      "vd object id 0x%X",
                      vd_object_id);

    }else if (status == FBE_STATUS_OK){ 
        sprintf(temp, "Set unused drive as spare flag to %d for "\
                      "vd object id 0x%X",
                      unsued_drive_as_spare_flag, vd_object_id);
    }     

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepVirtualDrive Class :: update_permanent_spare_swap_in_time(int argc, 
*    char **argv)
*********************************************************************
*
*  Description:
*     Updates the permanent spare swap in time.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t  sepVirtualDrive:: update_permanent_spare_swap_in_time(int argc, 
    char **argv)

{
    // Define specific usage string  
    usage_format =
        " Usage: %s [swap in time in seconds]\n"
        " For example: %s 1";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "updateSwapInTime",
        "sepVirtualDrive::update_permanent_spare_swap_in_time",
        "fbe_api_update_permanent_spare_swap_in_time",  
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Get spare swap in time
    argv++;
    swap_in_time_in_sec = (fbe_u64_t)_strtoui64(*argv,0,0);
	
    sprintf(params, "Swap in time %d ", (int)swap_in_time_in_sec);

    // Make api call: 
    status = fbe_api_update_permanent_spare_swap_in_time(swap_in_time_in_sec);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to update spare swap in time");

    }else if (status == FBE_STATUS_OK){ 
        sprintf(temp, "Updated the spare swap in time to %d seconds successfully.", (int)swap_in_time_in_sec);
    }     

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
	
}


/*********************************************************************
* sepVirtualDrive Class :: set_control_automatic_hot_spare(int argc, 
*    char **argv)
*********************************************************************
*
*  Description:
*     sets up the system to enable or disbale the hot spare feature.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t  sepVirtualDrive:: set_control_automatic_hot_spare(int argc, 
    char **argv)

{
    // Define specific usage string  
    usage_format =
        " Usage: %s [enable|disable]\n"
        " For example: %s enable";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "controlAutomaticHotSpare",
        "sepVirtualDrive::set_control_automatic_hot_spare",
        "fbe_api_control_automatic_hot_spare",  
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Get spare swap in time
    argv++;
   strcpy( enable_or_disable, *argv);

    if (strcmp(enable_or_disable, "ENABLE") == 0 ){
        enable = FBE_TRUE;
    }
    else if(strcmp(enable_or_disable, "DISABLE") == 0) {
        enable = FBE_FALSE;
    }
    else{
        sprintf(temp, "<ERROR!>Invalid option selected");
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }
    sprintf(params, "automatic hot spare enable (%s)",enable_or_disable);

    // Make api call: 
    status =fbe_api_control_automatic_hot_spare(enable);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to update the automatic hot spare status to %s", enable_or_disable);

    }else { 
        sprintf(temp, "Automatic hot spare state changed successfully to %s",enable_or_disable);
    }     

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;

}
/*********************************************************************
* sepVirtualDrive Class :: edit_virtual_drive_spare_info() 
                            (private method)
*********************************************************************
*
*  Description:
*      Format the virtual drive spare information to display
*
*  Input: Parameters
*      spare_drive_info - spare drive information structure

*  Returns:
*      void
*********************************************************************/

void sepVirtualDrive::edit_virtual_drive_spare_info(
    fbe_spare_drive_info_t *spare_drive_info) 
{
    // virtual drive spare information
    sprintf(temp,                       "\n "
        "<Port Number>                  0x%X\n "
        "<Enclosure Number>             0x%X\n "
        "<Slot Number>                  0x%X\n "
        "<Drive Type>                   0x%X (%s)\n "
        "<Configured Capacity>          %llu\n "
        "<Pool Id>                      0x%X\n "
        "<Configured Block Size>        0x%X (%s)\n ",
        
        spare_drive_info->port_number,
        spare_drive_info->enclosure_number,
        spare_drive_info->slot_number,

        spare_drive_info->drive_type,
        utilityClass::convert_drive_type_to_string(spare_drive_info->drive_type),
        
        (unsigned long long)spare_drive_info->configured_capacity,
        spare_drive_info->pool_id,

        spare_drive_info->configured_block_size, 
        utilityClass::convert_block_size_to_string(spare_drive_info->configured_block_size) );
}

/*********************************************************************
* sepVirtualDrive Class :: edit_unused_drive_as_spare_flag() 
                            (private method)
*********************************************************************
*
*  Description:
*      Format the unused drive as spare flag to display
*
*  Input: Parameters
*      unsued_drive_as_spare_flag - drive flag

*  Returns:
*      void
*********************************************************************/

void sepVirtualDrive::edit_unused_drive_as_spare_flag(
    fbe_bool_t unsued_drive_as_spare_flag) 
{
    // unused drive as spare flag 
    sprintf(temp,                           "\n "
        "<Unused Drive As Spare Flag >      %d\n ",
        unsued_drive_as_spare_flag);
}

/*********************************************************************
* sepVirtualDrive Class :: sep_virtual_drive_initializing_variable()
*    (private method)
*********************************************************************/
void sepVirtualDrive :: sep_virtual_drive_initializing_variable()
{
    unsued_drive_as_spare_flag = FBE_FALSE;
    copy_request_type =  FBE_SPARE_SWAP_INVALID_COMMAND;
    fbe_zero_memory(&spare_drive_info,sizeof(spare_drive_info));
}
/*********************************************************************
* sepVirtualDrive end of file
*********************************************************************/
