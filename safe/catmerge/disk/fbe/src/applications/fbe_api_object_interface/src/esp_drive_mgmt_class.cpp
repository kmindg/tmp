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
*      This file defines the methods of the ESP DRIVE INTERFACE class.
*
*  Notes:
*      The ESP DRIVE MGMT class (espDriveMgmt) is a derived class of 
*      the base class (bESP).
*
*  History:
*      28-Jun-2011 : initial version
*
*********************************************************************/

#ifndef T_ESP_DRIVE_MGMT_CLASS_H
#include "esp_drive_mgmt_class.h"
#endif

/*********************************************************************
* espDriveMgmt class :: Constructor
*********************************************************************/

espDriveMgmt::espDriveMgmt() : bESP()
{
    // Store Object number
    idnumber = (unsigned) ESP_DRIVE_MGMT_INTERFACE;
    espDriveMgmtCount = ++gEspDriveMgmtCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "ESP_DRIVE_MGMT_INTERFACE");
    
    // Initialize member variables
    initialize();

    if (Debug) {
        sprintf(temp, "espDriveMgmt class constructor (%d) %s\n", 
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API ESP DRIVE MGMT Interface function calls>
    espDriveMgmtInterfaceFuncs = 
        "[function call]\n" \
        "[short name]       [FBE API ESP DRIVE MGMT Interface]\n" \
        " ------------------------------------------------------------\n" \
        " getDriveInfo          fbe_api_esp_drive_mgmt_get_drive_info\n" \
        " getDriveLog          fbe_api_esp_drive_mgmt_get_drive_log\n" \
        " getDloadDriveStatus   fbe_api_esp_drive_mgmt_get_download_drive_status - Obsolete FBE API\n" \
        " getDloadProcessStatus fbe_api_esp_drive_mgmt_get_download_process_status - Obsolete FBE API\n" \
        " abortDload            fbe_api_esp_drive_mgmt_abort_download - Obsolete FBE API\n" \
        " ------------------------------------------------------------\n"\
        " senddebugcontrol       fbe_api_esp_drive_mgmt_send_debug_control\n"\
        " ------------------------------------------------------------\n";
    
    // Define common usage for ESP DRIVE MGMT commands
    usage_format = 
        " Usage: %s\n"
        " For example: %s";
};

/*********************************************************************
* espDriveMgmt class : Accessor methods
*********************************************************************/

unsigned espDriveMgmt::MyIdNumIs(void)
{
    return idnumber;
};

char * espDriveMgmt::MyIdNameIs(void)
{
    return idname;
};

void espDriveMgmt::dumpme(void)
{ 
    strcpy (key, "espDriveMgmt::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, espDriveMgmtCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* espDriveMgmt Class :: select()
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
*      28-Jun-2011 : initial version
*
*********************************************************************/

fbe_status_t espDriveMgmt::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select ESP interface"); 

    // List ESP DRIVE MGMT Interface calls if help option and 
    // less than 6 arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) espDriveMgmtInterfaceFuncs );

        // get drive management info
    } else if (strcmp (argv[index], "GETDRIVEINFO") == 0) {
        status = get_drive_mgmt_info(argc, &argv[index]);
        
        // send drive log command
    } else if (strcmp (argv[index], "GETDRIVELOG") == 0) {
        status = get_drive_mgmt_drive_log(argc, &argv[index]);
   
        // send fuel gauge command
    } else if (strcmp (argv[index], "GETFUELGAUGE") == 0) {
        status = get_drive_mgmt_fuel_gauge(argc, &argv[index]);
        
        // get download drive status
    } else if (strcmp (argv[index], "GETDLOADDRIVESTATUS") == 0) {
        status = get_drive_mgmt_dload_drive_status(argc, &argv[index]);

        // get drive management download process info
    } else if (strcmp (argv[index], "GETDLOADPROCESSSTATUS") == 0) {
        status = get_drive_mgmt_dload_process_info(argc, &argv[index]);
        
        // drive management abort download 
    } else if (strcmp (argv[index], "ABORTDLOAD") == 0) {
        status = drive_mgmt_abort_dload(argc, &argv[index]);

        // drive management abort download 
    } else if (strcmp (argv[index], "SENDDEBUGCONTROL") == 0) {
        status = drive_mgmt_send_debug_control(argc, &argv[index]);

        // can't find match on short name
    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) espDriveMgmtInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* espDriveMgmt Class :: get_drive_mgmt_info()
*********************************************************************
*
*  Description:
*      Gets the drive management information
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t espDriveMgmt::get_drive_mgmt_info(int argc, char ** argv)
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [disk number in B_E_D]\n"\
        " For example: %s 1_2_3";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getDriveInfo",
        "espDriveMgmt::get_drive_mgmt_info",
        "fbe_api_esp_drive_mgmt_get_drive_info",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    sprintf(params, " %s ", *argv);

    //Convet disk name to b_e_d formate
    status = utilityClass::convert_diskname_to_bed(
            (fbe_u8_t*) argv[0], &phys_location, temp);

    if(status != FBE_STATUS_OK){
        // temp is being populated in convert_diskname_to_bed
        call_post_processing(status, temp, key, params);
        return status;
    }

    drive_info.location.bus = phys_location.bus;
    drive_info.location.enclosure = phys_location.enclosure;
    drive_info.location.slot = phys_location.slot;

    // Make api call
    status = fbe_api_esp_drive_mgmt_get_drive_info(
                &drive_info);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get drive mgmt info for "\
            "bus (%d) enclosure (%d) slot (%d)",
                drive_info.location.bus, drive_info.location.enclosure, 
                drive_info.location.slot);
    } else {
        edit_drive_mgmt_info(&drive_info); 
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espDriveMgmt Class :: get_drive_mgmt_drive_log()
*********************************************************************
*
*  Description:
*      Send drive log command to esp
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t espDriveMgmt::get_drive_mgmt_drive_log(
    int argc, 
    char ** argv)
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [disk number in B_E_D]\n"\
        " For example: %s 1_2_3";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getDriveLog",
        "espDriveMgmt::get_drive_mgmt_drive_log",
        "fbe_api_esp_drive_mgmt_get_drive_log",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    

    argv++;
    sprintf(params, " %s ", *argv);

    //Convet disk name to b_e_d formate
    status = utilityClass::convert_diskname_to_bed(
            (fbe_u8_t*) argv[0], &phys_location, temp);

    if(status != FBE_STATUS_OK){
        // temp is being populated in convert_diskname_to_bed
        call_post_processing(status, temp, key, params);
        return status;
    }

    location.bus = phys_location.bus;
    location.enclosure = phys_location.enclosure;
    location.slot = phys_location.slot;

    // Make api call: 
    status = fbe_api_esp_drive_mgmt_get_drive_log(&location);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to send the drivelog command to ESP");
    } else {
        sprintf(temp, "Sent the drivelog command to ESP");
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    
    return status;
}

/*********************************************************************
* espDriveMgmt Class :: get_drive_mgmt_fuel_gauge()
*********************************************************************
*
*  Description:
*      Send get fuel gauge command to esp
*
*  Input: Parameters
*       N/A
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t espDriveMgmt::get_drive_mgmt_fuel_gauge(
    int argc, 
    char ** argv)
{
    fbe_u32_t enable;
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getFuelGauge",
        "espDriveMgmt::get_drive_mgmt_fuel_gauge",
        "fbe_api_esp_drive_mgmt_get_fuel_gauge",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Make api call: 
    status = fbe_api_esp_drive_mgmt_get_fuel_gauge(&enable);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to send the fuel gauge command to ESP");
    } else {
        sprintf(temp, "Sent the fuel gauge command to ESP");
    }
    
    params[0] = '\0';
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    
    return status;
}


/*********************************************************************
* espDriveMgmt Class :: get_drive_mgmt_dload_drive_status()
*********************************************************************
*
*  Description:
*      Get download drive status
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t espDriveMgmt::get_drive_mgmt_dload_drive_status(
    int argc, 
    char ** argv)
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [disk number in B_E_D]\n"\
        " For example: %s 1_2_3";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getDloadDriveStatus",
        "espDriveMgmt::get_drive_mgmt_dload_drive_status",
        "fbe_api_esp_drive_mgmt_get_download_drive_status",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    

    argv++;
    sprintf(params, " %s ", *argv);

    status = utilityClass::convert_diskname_to_bed(
            (fbe_u8_t*) argv[0], &phys_location, temp);

    if(status != FBE_STATUS_OK){
        // temp is being populated in convert_diskname_to_bed
        call_post_processing(status, temp, key, params);
        return status;
    }

    // following is obsolete. 
#if 0
    dload_drive_status.bus = phys_location.bus;
    dload_drive_status.enclosure = phys_location.enclosure;
    dload_drive_status.slot = phys_location.slot;

    // Make api call: 
    status = fbe_api_esp_drive_mgmt_get_download_drive_status(
        &dload_drive_status);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get the download drive status");
    } else {
        edit_drive_mgmt_dload_drive_status(&dload_drive_status);
    }
#endif

    sprintf(temp, "Obsolete FBE API");
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    
    return status;
}


/*********************************************************************
* espDriveMgmt Class :: get_drive_mgmt_dload_process_info()
*********************************************************************
*
*  Description:
*      Gets the drive management download process information
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t espDriveMgmt::get_drive_mgmt_dload_process_info(
    int argc , 
    char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getDloadProcessStatus",
        "espDriveMgmt::get_drive_mgmt_dload_process_info",
        "fbe_api_esp_drive_mgmt_get_download_process_status",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // following call is obsolete. 
#if 0
    // Make api call: 
    status = fbe_api_esp_drive_mgmt_get_download_process_status(
                &download_status);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get drive mgmt download process info");
    } else {
        edit_drive_mgmt_dload_process_info(&download_status); 
    }
#endif
    
    sprintf(temp, "Obsolete FBE API");
        
    params[0] = '\0';

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* espDriveMgmt Class :: drive_mgmt_abort_dload()
*********************************************************************
*
*  Description:
*      Aborts the drive management download
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t espDriveMgmt::drive_mgmt_abort_dload(
    int argc , 
    char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "abortDload",
        "espDriveMgmt::drive_mgmt_abort_dload",
        "fbe_api_esp_drive_mgmt_abort_download",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // following call is obsolete. 
#if 0        
    // Make api call: 
    status = fbe_api_esp_drive_mgmt_abort_download();

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't abort the drive mgmt download");
    } else {
        sprintf(temp, "Aborted drive mgmt download"); 
    }
#endif
    
    sprintf(temp, "Obsolete FBE API");

    params[0] = '\0';

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}


/*********************************************************************
* espDriveMgmt Class :: edit_drive_mgmt_info() (private method)
*********************************************************************
*
*  Description:
*      Format the drive management information to display
*
*  Input: Parameters
*      drive_info  - drive information structure

*  Returns:
*      void
*********************************************************************/

void espDriveMgmt::edit_drive_mgmt_info(
    fbe_esp_drive_mgmt_drive_info_t *drive_info) 
{
    // drive information
    sprintf(temp,                           "\n "
        "<Processor Enclosure>              0x%X\n "
        "<Bus>                              0x%X\n "
        "<Enclosure>                        0x%X\n "
        "<Slot>                             0x%X\n "
        "<SP>                               0x%X\n "
        "<Port>                             0x%X\n "
        "\n "
        "<Inserted>                         %d\n "
        "<Faulted>                          %d\n "
        "<Loop A Valid>                     %d\n "
        "<Loop B Valid>                     %d\n "
        "<Bypass Enabled Loop A>            %d\n "
        "<Bypass Enabled Loop B>            %d\n "
        "<Bypass Requested Loop A>          %d\n "
        "<Bypass Requested Loop B>          %d\n "
        "<Drive Type>                       0x%X (%s)\n "
        "<Gross Capacity (blocks)>          %llu\n "
        "<Vendor Id>                        %s\n "
        "<Product Id>                       %s\n "
        "<TLA Part Number>                  %s\n "
        "<Product Rev>                      %s\n "
        "<Product Serial Num>               %s\n "
        "<State>                            0x%X (%s)\n "
        "<Last Log>                         %llu\n "
        "<Block Size>                       0x%X\n "
        "<Drive Qdepth>                     0x%X\n "
        "<Drive RPM>                        0x%X\n "
        "<Speed Capability>                 0x%X (%s)\n "
        "<Drive Description Buff>           %s\n "
        "<DG Part Number ASCII>             %s\n "
        "<Bridge HW Rev>                    %s\n "
        "<Spin Down Qualified>              %d\n "
        "<Spin Up Time In Minutes>          0x%X\n "
        "<Stand By Time In Minutes>         0x%X\n "
        "<Spin Up Count>                    0x%X\n ",

        drive_info->location.processorEnclosure,
        drive_info->location.bus,
        drive_info->location.enclosure,
        drive_info->location.slot,
        drive_info->location.sp,
        drive_info->location.port,
        
        drive_info->inserted,
        drive_info->faulted,
        drive_info->loop_a_valid,
        drive_info->loop_b_valid,
        drive_info->bypass_enabled_loop_a,
        drive_info->bypass_enabled_loop_b,
        drive_info->bypass_requested_loop_a,
        drive_info->bypass_requested_loop_b,

        drive_info->drive_type, 
        utilityClass::convert_drive_type_to_string(drive_info->drive_type),
        
        (unsigned long long)drive_info->gross_capacity,
        drive_info->vendor_id,
        drive_info->product_id,
        drive_info->tla_part_number,
        drive_info->product_rev,
        drive_info->product_serial_num,

        drive_info->state, 
        utilityClass::lifecycle_state(drive_info->state),
        
        (unsigned long long)drive_info->last_log,
        drive_info->block_size,
        drive_info->drive_qdepth,
        drive_info->drive_RPM,
        
        drive_info->speed_capability, 
        utilityClass::speed_capability(drive_info->speed_capability),
        
        drive_info->drive_description_buff,
        drive_info->dg_part_number_ascii,
        drive_info->bridge_hw_rev,
        drive_info->spin_down_qualified,
        drive_info->spin_up_time_in_minutes,
        drive_info->stand_by_time_in_minutes,
        drive_info->spin_up_count);
}


/*********************************************************************
* espDriveMgmt Class :: initialize()
*********************************************************************
*
*  Description:
*      Initializes the required member variables
*
*  Input: Parameters - None
*
*  Returns:
*       void
*********************************************************************/
void espDriveMgmt::initialize()
{
    // initialize the drive info structure
    fbe_zero_memory(&drive_info, sizeof(drive_info));

    // initialize location info
    fbe_zero_memory(&location,sizeof(location));

    // initialize download phys location
    fbe_zero_memory(&phys_location,sizeof(phys_location));

    // initialize download cru_on_off action
    fbe_zero_memory(&cru_on_off_action,sizeof(cru_on_off_action));

}

/*********************************************************************
* espDriveMgmt Class :: drive_mgmt_send_debug_control()
*********************************************************************
*
*  Description:
*      Send debug control.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espDriveMgmt::drive_mgmt_send_debug_control(
    int argc , 
    char ** argv)
{
    fbe_u32_t lastSlot;

    // Define specific usage string  
    usage_format =
        " Usage: %s [disk number in B_E_D] [debug action]\n"\
        " For example: %s 1_2_3 <CRU_ON | CRU_OFF>";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "senddebugcontrol",
        "espDriveMgmt::drive_mgmt_send_debug_control",
        "fbe_api_esp_drive_mgmt_send_debug_control",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    argv++;

    //Convet disk name to b_e_d formate
    status = utilityClass::convert_diskname_to_bed(
            (fbe_u8_t*) argv[0], &phys_location, temp);
           // Check status of call
    if (status != FBE_STATUS_OK) {
        call_post_processing(status, temp, key, params);
        return status;
    }

    argv++;
    status = get_drive_debug_action(argv);
       // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Invalid Drive Debug Action. Valid"
            " Actions are (CRU_ON | CRU_OFF)");
        sprintf(params," Drive Number:%s",*(argv-1));
        call_post_processing(status, temp, key, params);
        return status;
    }

    location.bus = phys_location.bus;
    location.enclosure = phys_location.enclosure;
    location.slot = phys_location.slot;

    sprintf(params," Drive Number:%s , Debug Action: %s",*(argv-1),*argv);

    // need the number of slots in this enclosure
    status = fbe_api_esp_encl_mgmt_get_drive_slot_count(&location, &lastSlot);
    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Unable to get lastslot information\n");
        call_post_processing(status, temp, key, params);
        return status;
    }

    //send comamnd to remove the drive
    status = fbe_api_esp_drive_mgmt_send_debug_control(location.bus, 
                                                       location.enclosure, 
                                                       location.slot, 
                                                       lastSlot, 
                                                       cru_on_off_action,
                                                       FBE_FALSE, FBE_FALSE);

        // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to send debug control to ESP");
    } else {
        sprintf(temp, "Successfully sent the debug control to ESP");
    }

    call_post_processing(status, temp, key, params);
    
    return status;
}

/*********************************************************************
* espDriveMgmt::get_drive_debug_action()
*********************************************************************
*
*  Description:
*      Gets the debug action.
*
*  Input: Parameters
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t espDriveMgmt::get_drive_debug_action(char **argv) 
{
    if(strcmp(*argv, "CRU_ON") == 0) {
        cru_on_off_action = FBE_DRIVE_ACTION_INSERT;
    } else if(strcmp(*argv, "CRU_OFF") == 0){
        cru_on_off_action = FBE_DRIVE_ACTION_REMOVE;
    } else {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
* espDriveMgmt end of file
*********************************************************************/
