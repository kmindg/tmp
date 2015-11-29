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
 *      This file defines the methods of the physical drive class.
 *
 *  Notes:
 *      The physical drive class (PhyDrive) is a derived class of 
 *      the base class (bPhysical).
 *
 *  History:
 *      12-May-2010 : inital version
 *
 *********************************************************************/

#ifndef T_PHYDRIVECLASS_H
#include "phy_drive_class.h"
#endif

/*********************************************************************
 * PhyDrive class :: Constructor
 *********************************************************************/

PhyDrive::PhyDrive() : bPhysical()
{
    // Store Object number
    idnumber = (unsigned) PHY_DRIVE_INTERFACE;
    pdrCount = ++gPhysDriveCount;;

    // Store Object name
    idname = new char [64];
    strcpy(idname, "PHY_DRIVE_INTERFACE");

    Phy_Drive_Intializing_variable();

    if (Debug) {
        sprintf(temp, 
            "PhyDrive class constructor (%d) %s\n", 
            idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API Physical Drive Interface function calls>
    phyDriveInterfaceFuncs = 
        "[function call]\n" \
        "[short name] [FBE API Physical Drive Interface]\n" \
        " ------------------------------------------------------------\n" \
        " getObjbyDr  fbe_api_get_physical_drive_object_id_by_location\n" \
        " ------------------------------------------------------------\n" \
        " getCacheDr  fbe_api_physical_drive_get_cached_drive_info\n" \
        " getDrInfo   fbe_api_physical_drive_get_drive_information\n" \
        " getDrInfoAs fbe_api_physical_drive_get_drive_information_asynch\n" \
        " recDiagInfo fbe_api_physical_drive_receive_diag_info\n" \
        /*" getDskErrlg fbe_api_physical_drive_get_disk_error_log\n" \*/
        " isWrCachEnb fbe_api_physical_drive_is_wr_cache_enabled\n" \
        " ------------------------------------------------------------\n" \
        " resetDrive  fbe_api_physical_drive_reset\n" \
        " resetDrSlot fbe_api_physical_drive_reset_slot\n" \
        " powerCycle  fbe_api_physical_drive_power_cycle\n" \
        " failDrive   fbe_api_physical_drive_fail_drive\n" \
        " setDrDthRe  fbe_api_physical_drive_set_drive_death_reason\n" \
        " setWrUcorr  fbe_api_physical_drive_set_write_uncorrectable\n" \
        " ------------------------------------------------------------\n" \
        " getErrCnts  fbe_api_physical_drive_get_error_counts\n" \
        " clrErrCnts  fbe_api_physical_drive_clear_error_counts\n" \
        " ------------------------------------------------------------\n" \
        " getIOTime   fbe_api_physical_drive_get_default_io_timeout\n" \
        " setIOTime   fbe_api_physical_drive_set_default_io_timeout\n" \
        " ------------------------------------------------------------\n" \
        " getObQdepth fbe_api_physical_drive_get_object_queue_depth\n" \
        " setObQdepth fbe_api_physical_drive_set_object_queue_depth\n" \
        " getDrQdepth fbe_api_physical_drive_get_drive_queue_depth\n" \
        " ------------------------------------------------------------\n" \
        " clearEol    fbe_api_physical_drive_clear_eol\n"\
        " ------------------------------------------------------------\n" \
/*        " FwDnldstart fbe_api_physical_drive_fw_download_start_asynch\n" \
        " FwDnldasync fbe_api_physical_drive_fw_download_asynch\n" \
        " FwDnldstop  fbe_api_physical_drive_fw_download_stop_asynch\n" \
        " ------------------------------------------------------------\n" */;

    // Define common usage for physical drive commands
    usage_format = 
        " Usage: %s [object id]\n"
        " For example: %s 4";
};

/*********************************************************************
 * PhyDrive class : Accessor methods
 *********************************************************************/

unsigned PhyDrive::MyIdNumIs(void)
{
    return idnumber;
};

char * PhyDrive::MyIdNameIs(void)
{
    return idname;
};

void PhyDrive::dumpme(void)
{ 
     strcpy (key, "PhyDrive::dumpme");
     sprintf(temp, "Object id: (%d) count: (%d)\n", 
         idnumber, pdrCount);
     vWriteFile(key, temp);
}

/*********************************************************************
 * PhyDrive Class :: select()
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
 *      12-May-2010 : inital version
 *
 *********************************************************************/

fbe_status_t PhyDrive::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select Physical interface");

    // List Physical Drive calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) phyDriveInterfaceFuncs );
        //printf("PhyDrive::select exit\n");

    // get object Id by location (port, enclosure, slot)
    }else if (strncmp (argv[index], "GETOBJBYDR", 10) == 0) {
        status = get_object_id(argc, &argv[index]);
   
    // get cached drive information
    } else if (strncmp (argv[index], "GETCACHEDR", 10) == 0) {
        status = get_cached_drive_info(argc, &argv[index]);
       
    // get drive information async
    }else if (strncmp (argv[index], "GETDRINFOAS", 11) == 0) {
        status = get_drive_info_asynch(argc, &argv[index]);

    // get drive information
    }else if (strncmp (argv[index], "GETDRINFO", 9) == 0) {
        status = get_drive_info(argc, &argv[index]);

    // receive diag info
    }else if (strncmp (argv[index], "RECDIAGINFO", 11) == 0) {
        status = receive_diag_info(argc, &argv[index]);

    // get disk error log 
    }else if (strncmp (argv[index], "GETDSKERRLG", 11) == 0) {
        //status = get_disk_error_log(argc, &argv[index]);
        
    // is write cahched enabled
    }else if (strncmp (argv[index], "ISWRCACHENB", 11) == 0) {
        status = is_wr_cache_enabled(argc, &argv[index]);

    // reset drive
    }else if (strncmp (argv[index], "RESETDRIVE", 10) == 0) {
        status = reset_drive(argc, &argv[index]);
    
    // reset drive slot
    }else if (strncmp (argv[index], "RESETDRSLOT", 11) == 0) {
        status = drive_reset_slot(argc, &argv[index]);

    // power_cycle
    }else if (strncmp (argv[index], "POWERCYCLE", 10) == 0) {
        status = power_cycle_drive(argc, &argv[index]);
        
    // fail_drive
    }else if (strncmp (argv[index], "FAILDRIVE", 9) == 0) {
        status = fail_drive(argc, &argv[index]);
        
    // set_write_uncorrectable
    }else if (strncmp (argv[index], "SETWRUCORR", 10) == 0) {
        status = set_write_uncorrectable(argc, &argv[index]);
        
    // get error counts
    }else if (strncmp (argv[index], "GETERRCNTS", 10) == 0) {
        status = get_error_counts(argc, &argv[index]);

    // clear error counts
    }else if (strncmp (argv[index], "CLRERRCNTS", 10) == 0) {
        status = clear_error_counts(argc, &argv[index]);

    // get_default_io_timeout
    }else if (strncmp (argv[index], "GETIOTIME", 10) == 0) {
        status = get_default_io_timeout(argc, &argv[index]);
    
    // set_default_io_timeout
    }else if (strncmp (argv[index], "SETIOTIME", 10) == 0) {
        status = set_default_io_timeout(argc, &argv[index]);

    // get_object_queue_depth
    }else if (strncmp (argv[index], "GETOBQDEPTH", 10) == 0) {
        status = get_object_queue_depth(argc, &argv[index]);
    
    // set_object_queue_depth
    }else if (strncmp (argv[index], "SETOBQDEPTH", 10) == 0) {
        status = set_object_queue_depth(argc, &argv[index]);
        
    // get_drive_queue_depth
    }else if (strncmp (argv[index], "GETDRQDEPTH", 10) == 0) {
        status = get_drive_queue_depth(argc, &argv[index]);

    // fw_download_start_asynch
    }else if (strncmp (argv[index], "FWDNLDSTART", 11) == 0) {
        //status = fw_download_start_asynch(argc, &argv[index]);
    
    // fw_download_asynch
    }else if (strncmp (argv[index], "FWDNLDASYNC", 11) == 0) {
        //status = fw_download_asynch(argc, &argv[index]);
        
    // fw_download_stop
    }else if (strncmp (argv[index], "FWDNLDSTOP", 10) == 0) {
        //status = fw_download_stop(argc, &argv[index]);
        
    // can't find match on short name
    }else if (strncmp (argv[index], "SETDRDTHRE", 10) == 0) {
        status = set_drive_death_reason(argc, &argv[index]);
        
    // Clear EOL
    }else if (strcmp (argv[index], "CLEAREOL") == 0) {
        status = clear_eol(argc, &argv[index]);

    }else if (strcmp (argv[index], "PASSTHRU") == 0) {
        status = send_pass_thru(argc, index, &argv[0]);

    // can't find match on short name
    }else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
            BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) phyDriveInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
 *  PhyDrive class :: get_object_id ()
 *********************************************************************/

fbe_status_t PhyDrive::get_object_id(int argc, char **argv) 
{
    // Assign default values
    object_id = FBE_OBJECT_ID_INVALID;
    
    // Define specific usage string  
    usage_format =
        " Usage: %s [disk number in B_E_D]\n"\
        " For example: %s 1_2_3";
        
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getObjbyDr",
        "PhyDrive::object_id_by_location,",
        "fbe_api_get_physical_drive_object_id_by_location",  
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

    port = phys_location.bus;
    enclosure = phys_location.enclosure;
    slot = phys_location.slot;

    // Make api call: 
    status = fbe_api_get_physical_drive_object_id_by_location(
        port, 
        enclosure, 
        slot, 
        &object_id);
    fbe_status_t passFail = status;
   
    // Check status of call
    if (object_id == (fbe_u32_t) FBE_OBJECT_ID_INVALID) {
        strcpy(temp, "FBE_OBJECT_ID_INVALID");
        passFail = FBE_STATUS_GENERIC_FAILURE;
     
    }else if (status != FBE_STATUS_OK) {
        strcpy(temp, "Can't get object id for drive ");
    
    }else if (status == FBE_STATUS_OK){ 
        sprintf(temp, "<object id> 0x%x (%u)", 
            object_id, object_id);
    }     
    
    // Output results from call or description of error
    call_post_processing(passFail, temp, key, params);
    return status;
}

/*********************************************************************
 * PhyDrive class :: get_cached_drive_info () 
 *********************************************************************/

fbe_status_t PhyDrive::get_cached_drive_info(int argc , char ** argv)
{      
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getCacheDr",
        "PhyDrive::get_cached_drive_info",
        "fbe_api_physical_drive_get_cached_drive_info", 
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
    status = fbe_api_physical_drive_get_cached_drive_info(
        object_id, 
        &drive_info, 
        FBE_PACKET_FLAG_NO_ATTRIB);
        
    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get cached drive info");
    } else {
        edit_drive_info(&drive_info); 
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}
/*********************************************************************
 *  PhyDrive class :: get_drive_info
 *********************************************************************/

fbe_status_t PhyDrive::get_drive_info(int argc, char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getDrInfo",
        "PhyDrive::PhyDrive::get_drive_info",
        "fbe_api_physical_drive_get_drive_information", 
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
    status = fbe_api_physical_drive_get_drive_information(
         object_id, 
         &drive_info, 
         FBE_PACKET_FLAG_NO_ATTRIB);
        
    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't get drive info");
    } else {
        edit_drive_info(&drive_info); 
    }
    
    // Output results from call or description of error    
    call_post_processing(status, temp, key, params);
    return status;
}
/*********************************************************************
 *  PhyDrive class :: get_drive_info_asynch
 *********************************************************************/
 
fbe_status_t PhyDrive::get_drive_info_asynch(int argc, char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getDrInfoAs",
        "PhyDrive::get_drive_info_asynch",
        "fbe_api_physical_drive_get_drive_information_asynch", 
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
    status = fbe_api_physical_drive_get_drive_information_asynch(
        object_id, 
        &drive_info_asynch);

    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't get asynch drive info");
    } else {
        edit_asynch_drive_info(&drive_info_asynch); 
    }
    
    // Output results from call or description of error    
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyDrive class :: receive_diag_info
 *********************************************************************/

fbe_status_t PhyDrive::receive_diag_info(int argc , char ** argv) 
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "recDiagInfo",
        "PhyDrive::receive_diag_info",
        "fbe_api_physical_drive_receive_diag_info", 
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

    //Force page 82.
    diag_info.page_code = 0x82;

    // Make api call: 
    status = fbe_api_physical_drive_receive_diag_info(
        object_id, 
        &diag_info);

    // Check status of call
    if (status != FBE_STATUS_OK)
    {
        sprintf(temp, "Can't get diagnostic drive info");
  
    } else {
        if (diag_info.page_code != 0x82)
        {
            sprintf(temp, "Not page 0x82 : 0x%x",
                diag_info.page_code);
        } else {
            edit_diag_info(&(diag_info.page_info.pg82), 
                diag_info.page_code);
        }
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyDrive class :: get_disk_error_log
 *********************************************************************/

fbe_status_t PhyDrive::get_disk_error_log(int argc , char ** argv) 
{
    fbe_u8_t  buffer[DC_TRANSFER_SIZE];
    fbe_lba_t block;
    fbe_u32_t blocks_to_get = 0;
    fbe_u32_t bytes = DC_TRANSFER_SIZE;
    char * hold_temp = new char[2000];

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getDskErrLog",
        "PhyDrive::get_disk_error_log",
        "fbe_api_physical_drive_get_disk_error_log", 
        usage_format,
        argc, 7);
    
    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        delete hold_temp;
        return status;
    }
    
    // Convert Object id to blocks_to_get to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " object id 0x%x (%s)", object_id, *argv);

    argv++;
    blocks_to_get = (fbe_u32_t)strtoul(*argv, 0, 0);
    
    argv--;
    sprintf(params, " %s 0x%x (%s)\n %s 0x%x", 
        "object id", object_id, *argv, 
        "blocks to get", blocks_to_get);

    for ( fbe_u16_t ix = 0; ix < blocks_to_get; ix++)
    {
        // File limit reached: Put out message and exit loop.
        if (ix > 4095) 
        {
            strcpy(key, "File Limit reached (4096 blocks)");
            sprintf(temp, "<ERROR!> %s (%d)\n %s",
                "Can't get block", ix,
                "PhyDrive::get_disk_error_log");
            vWriteFile(key, temp);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }

        // zero buffer 
        fbe_zero_memory( buffer, sizeof(buffer));
        block = (fbe_lba_t)ix;
               
        // Make api call: 
        status = fbe_api_physical_drive_get_disk_error_log(
            object_id, 
            block, 
            buffer);

        // Check status of call
        if (status != FBE_STATUS_OK){
            sprintf(hold_temp, "Can't get block 0x%llX",
		    (unsigned long long)block);
            blocks_to_get = 0;
        } else {
            edit_buffer(hold_temp, buffer, bytes, block); 
        }
        
        // Output results from call or description of error
        call_post_processing(status, hold_temp, key, params);
        key[0] = params[0] = '\0';
    }
    
    // release memory
    delete hold_temp;
    
    return status;
}

/*********************************************************************
 *  PhyDrive class :: is_wr_cache_enabled
 *********************************************************************/

fbe_status_t PhyDrive::is_wr_cache_enabled(int argc , char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "isWrCachEnb",
        "PhyDrive::is_wr_cache_enabled",
        "fbe_api_physical_drive_is_wr_cache_enabled", 
        usage_format,
        argc, 7);
    
    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " object id 0x%x (%u)", object_id, object_id);

    // Trace function. Set enabled to passed in value.
    if (Debug && argc > 8) {
        argv++;
        enabled = object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
        sprintf(temp, "%s\n %s 0x%x (%d) %s\n",
            "PhyDrive::is_wr_cache_enabled",
            "Set [enabled] to", enabled, enabled,
            "before making FBE API call. Should fail.");
        vWriteFile(dkey, temp);  
    }

    // Make api call: 
    status = fbe_api_physical_drive_is_wr_cache_enabled(
        object_id, 
        &enabled, 
        FBE_PACKET_FLAG_NO_ATTRIB);

    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't get Is Write Cache Enabled");
    } else {
        sprintf(temp, "<Is Write Cache Enabled> 0x%x (%d)", 
            enabled, enabled); 
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyDrive class :: reset_drive (Send RESET to the drive.)
 *********************************************************************/

fbe_status_t PhyDrive::reset_drive(int argc , char ** argv) 
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "resetDrive",
        "PhyDrive::reset_drive",
        "fbe_api_physical_drive_reset", 
        usage_format,
        argc, 7);
    
    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " object id 0x%x (%s)\n", object_id, *argv);

    // Make api call: 
    status = fbe_api_physical_drive_reset(
        object_id, 
        FBE_PACKET_FLAG_NO_ATTRIB);

    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't send RESET to drive");
    } else {
        sprintf(temp, "<status> 0x%x", status);
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *   PhyDrive class :: drive_reset_slot()
 *********************************************************************
 * @brief
 *  This function sends a RESET PHY command to the PDO.
 *
 * @param object_id - the object to display.
 * @param context - none.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  11/07/2010 - Created. 
 *********************************************************************/

fbe_status_t PhyDrive::drive_reset_slot(int argc , char ** argv) 
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "resetDrSlot",
        "PhyDrive::drive_reset_slot",
        "fbe_api_physical_drive_reset_slot",
        usage_format,
        argc, 7);
    
    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0); 
    sprintf(params, " object id 0x%x (%s)\n", object_id, *argv);

    // Make api call: 
    status = fbe_api_physical_drive_reset_slot(object_id);

    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't send drive slot RESET");
    } else {
        sprintf(temp, "<status> 0x%x", status);  
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyDrive class :: power_cycle drive 
 *********************************************************************/

fbe_status_t PhyDrive::power_cycle_drive(int argc , char ** argv) 
{
    // Define usage
    usage_format =
        "Usage: %s [object id] [duration: 0|1]\n" 
        " For example: %s 4 1";
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "PowerCycle",
        "PhyDrive::power_cycle_drive",
        "fbe_api_physical_drive_power_cycle", 
        usage_format,
        argc, 8);
    
    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert Object id and duration to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    
    argv++;
    duration  = (fbe_u32_t)strtoul(*argv, 0, 0);
    
    argv--;
    sprintf(params, "object id 0x%x (%s) duration (%d)\n", 
        object_id, *argv, duration);

    // Make api call: 
    status = fbe_api_physical_drive_power_cycle(
        object_id, 
        duration);

    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't send power cycle to drive");
    } else {
        sprintf(temp, "<status> 0x%x", status);
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}
  
/*********************************************************************
 *  PhyDrive class :: fail_drive 
 *********************************************************************/

fbe_status_t PhyDrive::fail_drive(int argc , char ** argv) 
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "failDrive",
        "PhyDrive::fail_drive",
        "fbe_api_physical_drive_fail_drive", 
        usage_format,
        argc, 7);
    
    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " object id 0x%x (%s)\n", object_id, *argv);

    // Make api call: 
    reason =  FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_CAPACITY;
    status = fbe_api_physical_drive_fail_drive(
        object_id, 
        reason);

    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't send failDrive to drive");
    } else {
        sprintf(temp, "<status> 0x%x", status);
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyDrive class :: set_drive_death_reason  
 *********************************************************************/

fbe_status_t PhyDrive::set_drive_death_reason(int argc , char ** argv) 
{
    // Define usage
    usage_format =
        "Usage: %s [object id] [death reason]\n" 
        " For example: %s 4 FBE_DRIVE_PHYSICALLY_REMOVED";
   // Check that all arguments have been entered
    status = call_preprocessing(
        "setDrDthRe",
        "PhyDrive::set_drive_death_reason",
        "fbe_api_physical_drive_set_drive_death_reason",
        usage_format,
        argc, 8);
    
    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert Object id and reason to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0); 

    argv++;
    death_reason = (enum _FBE_DRIVE_DEAD_REASON)strtoul(*argv, 0, 0);
    
    argv--;
    sprintf(params, " object id 0x%x (%s) death_reason (%d)\n", 
        object_id, *argv, death_reason);

    // Make api call: 
    status = fbe_api_physical_drive_set_drive_death_reason(
        object_id,
        death_reason,
        FBE_PACKET_FLAG_NO_ATTRIB);

    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't set death reason");
    } else {
        sprintf(temp, "<status> 0x%x", status);  
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyDrive class :: set_write_uncorrectable 
 *********************************************************************/

fbe_status_t PhyDrive::set_write_uncorrectable(int argc , char ** argv) 
{
    // define usage 
    usage_format = 
        "Usage: %s [object id] [lba]\n" 
        " For example: %s 4 0x11080\n";
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "setWrUcorr",
        "PhyDrive::set_write_uncorrectable",
        "fbe_api_physical_drive_set_write_uncorrectable",
        usage_format,
        argc, 8);
    
    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert Object id and lba to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0); 
    
    argv++;
    lba  = (fbe_lba_t)_strtoui64(*argv, 0, 0);
    if (strncmp (*argv, "-1", 2) == 0) {lba = 0x11080;}
        
    argv--;
    sprintf(params, " object id 0x%x (%s) lba 0x%llX\n", 
            object_id, *argv, (unsigned long long)lba);

    // Make api call: 
    status = fbe_api_physical_drive_set_write_uncorrectable(
        object_id, 
        lba, 
        FBE_PACKET_FLAG_NO_ATTRIB );

    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't send failDrive to drive");
    } else {
        sprintf(temp, "<status> 0x%x", status);
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyDrive class :: get_error_counts  
 *********************************************************************/

fbe_status_t PhyDrive::get_error_counts(int argc , char ** argv) 
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getErrCnts",
        "PhyDrive::get_error_counts",
        "fbe_api_physical_drive_get_error_counts",
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
    status = fbe_api_physical_drive_get_error_counts(
        object_id, 
        &error_stats, 
        FBE_PACKET_FLAG_NO_ATTRIB);

    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't get error counts");
    } else {
        edit_error_counts(&error_stats);
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyDrive class :: clear_error_counts  
 *********************************************************************/

fbe_status_t PhyDrive::clear_error_counts(int argc , char ** argv) 
{
   // Check that all arguments have been entered
    status = call_preprocessing(
        "clrErrCnts",
        "PhyDrive::clear_error_counts",
        "fbe_api_physical_drive_get_error_counts",
        usage_format,
        argc, 7);
    
    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0); 
    sprintf(params, " object id 0x%x (%u)", object_id, object_id);

    // Make api call: 
    status = fbe_api_physical_drive_clear_error_counts(
        object_id, 
        FBE_PACKET_FLAG_NO_ATTRIB);

    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't clear error counts");
    } else {
        sprintf(temp, "Cleared error counts for Obj ID 0x%x (%u)",
                object_id, object_id);
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyDrive class :: get_default_io_timeout
 *********************************************************************/

fbe_status_t PhyDrive::get_default_io_timeout(int argc, char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getIoTime",
        "PhyDrive::get_default_io_timeout",
        "fbe_api_physical_drive_get_default_io_timeout",
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
    status = fbe_api_physical_drive_get_default_io_timeout(
        object_id, 
        &drive_timeout, 
        FBE_PACKET_FLAG_NO_ATTRIB);
     
    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't get default IO timeout");
    } else {
        sprintf(temp, "<default timeout> 0x%llX (%llu)", 
                (unsigned long long)drive_timeout,
		(unsigned long long)drive_timeout);  
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyDrive class :: set_default_io_timeout
 *********************************************************************/

fbe_status_t PhyDrive::set_default_io_timeout(int argc, char ** argv)
{
    // Define specific usage string
    usage_format = " Usage: %s [object id] [drive timeout]\n"\
                   " For example: %s 0x4 600";

   // Check that all arguments have been entered
    status = call_preprocessing(
        "setIoTime",
        "PhyDrive::set_default_io_timeout",
        "fbe_api_physical_drive_set_default_io_timeout",
        usage_format,
        argc, 8);
    
    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0); 
    
    // Convert drive timeout to hex    
    argv++;
    drive_timeout = (fbe_u32_t)strtoul(*argv, 0, 0);
    
    argv--;
    sprintf(params, " object id 0x%x (%s) timeout 0x%llx",
            object_id, *argv, (unsigned long long)drive_timeout);

    // Make api call: 
    status = fbe_api_physical_drive_set_default_io_timeout(
        object_id, 
        drive_timeout, 
        FBE_PACKET_FLAG_NO_ATTRIB);
     
    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't get default IO timeout");
    } else {
        sprintf(temp, "Successfully set drive timeout to 0x%llX (%llu) for"\
                " physical drive obj ID 0x%X (%u)",
                (unsigned long long)drive_timeout,
		(unsigned long long)drive_timeout, object_id, object_id);
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}


/*********************************************************************
 *  PhyDrive class :: get_object_queue_depth
 *********************************************************************/

fbe_status_t PhyDrive::get_object_queue_depth(int argc, char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getObQdepth",
        "PhyDrive::get_object_queue_depth",
        "fbe_api_physical_drive_get_object_queue_depth",
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
    status = fbe_api_physical_drive_get_object_queue_depth(
        object_id, 
        &qdepth, 
        FBE_PACKET_FLAG_NO_ATTRIB);

    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't get object queue depth");
    } else {
        sprintf(temp, "<queue depth> 0x%x (%d)", qdepth, qdepth);  
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyDrive class :: set_object_queue_depth
 *********************************************************************/

fbe_status_t PhyDrive::set_object_queue_depth(int argc, char ** argv)
{
    // Define specific usage string
    usage_format = " Usage: %s [object id] [qdepth]\n"\
                   " For example: %s 0x4 5";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setObQdepth",
        "PhyDrive::set_object_queue_depth",
        "fbe_api_physical_drive_set_object_queue_depth",
        usage_format,
        argc, 8);
    
    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0); 
   
    // Convert qdepth to hex
    argv++;
    qdepth = (fbe_u32_t)strtoul(*argv, 0, 0);

    argv--;
    sprintf(params, " object id 0x%x (%s) qdepth 0x%x",
            object_id, *argv, qdepth);

    // Make api call: 
    status = fbe_api_physical_drive_set_object_queue_depth(
        object_id, 
        qdepth, 
        FBE_PACKET_FLAG_NO_ATTRIB);

    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't set object queue depth");
    } else {
        sprintf(temp, "Successfully set queue depth to 0x%X (%d) for "\
                "physical drive obj id 0x%X (%s)",
                 qdepth, qdepth, object_id, *argv);
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyDrive class :: get_drive_queue_depth
 *********************************************************************/

fbe_status_t PhyDrive::get_drive_queue_depth(int argc , char ** argv) 
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getDrQdepth",
        "PhyDrive::get_drive_queue_depth",
        "fbe_api_physical_drive_get_drive_queue_depth",
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
    status = fbe_api_physical_drive_get_object_queue_depth(
        object_id, 
        &depth, 
        FBE_PACKET_FLAG_NO_ATTRIB);

    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't get drive queue depth");
    } else {
        sprintf(temp, "<drive queue depth> 0x%x (%d)", depth, depth);  
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyDrive class :: fw_download_start_asynch 
 *********************************************************************/

fbe_status_t PhyDrive::fw_download_start_asynch(int argc , char ** argv) 
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "FwDnldstart",
        "PhyDrive::fw_download_start_asynch",
        "fbe_api_physical_drive_fw_download_start_asynch",
        usage_format,
        argc, 8);
    
    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0); 
    
    // Set force download option to FBE_FALSE or FBE_TRUE
    argv++;
    force_download = (strncmp (*argv, "0", 1) == 0) ? FBE_FALSE : FBE_TRUE;
    
    argv--;
    sprintf(params, " object id 0x%x (%s) force download 0x%x\n", 
        object_id, *argv, force_download);

    // Memory Allocation  
    /*start_context = (fbe_physical_drive_fwdl_start_asynch_t *)
        fbe_memory_native_allocate(
            sizeof(fbe_physical_drive_fwdl_start_asynch_t));*/

    // Check to see if Memory Allocation failed 
    if(start_context == NULL) 
    {
        sprintf(temp, "Memory Allocation failed for ...\n %s\n",
            " fbe_physical_drive_fwdl_start_asynch_t");

    // Make api call: 
    } else {
        //start_context->caller_instance = (void*)drive_mgmt;
        start_context->drive_object_id = object_id;
        start_context->request.force_download = force_download;
        start_context->response.proceed = FBE_FALSE;  //init 
        
        //fbe_physical_drive_fwdl_start_asynch_t *context = NULL;
        //start_context->completion_function = 
            //fwdl_start_async_completion(context);

        status = fbe_api_physical_drive_fw_download_start_asynch(
            start_context);

        // Check status of call
        if (status != FBE_STATUS_OK){
            sprintf(temp, "Can't start fw download");
        } else {
           sprintf(temp, "<status> 0x%x", status);   
        }
        //fbe_memory_native_release(start_context);
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyDrive class :: fw_download_stop_asynch 
 *********************************************************************/

fbe_status_t PhyDrive::fw_download_stop (int argc , char ** argv) 
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "FwDnldstop",
        "PhyDrive::fw_download_stop_asynch",
        "fbe_api_physical_drive_fw_download_stop_asynch",
        usage_format,
        argc, 8);
    
    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0); 
    sprintf(params, " object id 0x%x (%s)\n", object_id, *argv);

    // Memory Allocation  
    /*stop_context = (fbe_physical_drive_fwdl_stop_asynch_t *)
        fbe_memory_native_allocate(
            sizeof(fbe_physical_drive_fwdl_stop_asynch_t));*/

    // Check to see if Memory Allocation failed 
    if(stop_context == NULL) 
    {
        sprintf(temp, "Memory Allocation failed for ...\n %s\n",
            " fbe_physical_drive_fwdl_stop_asynch_t");

    // Make api call: 
    } else {
        //start_context->caller_instance = (void*)drive_mgmt;
        start_context->drive_object_id = object_id;
        
        //fbe_physical_drive_fwdl_stop_asynch_t *context = NULL;
        //start_context->completion_function = 
            //fwdl_stop_async_completion(context);

        status = fbe_api_physical_drive_fw_download_stop_asynch(
            stop_context);

        // Check status of call
        if (status != FBE_STATUS_OK){
            sprintf(temp, "Can't stop fw download");
        } else {
           sprintf(temp, "<status> 0x%x", status);   
        }
        //fbe_memory_native_release(stop_context);
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyDrive class :: fw_download_asynch 
 *********************************************************************/

fbe_status_t PhyDrive::fw_download_asynch(int argc , char ** argv) 
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "FwDnldasync",
        "PhyDrive::fw_download_asynch",
        "fbe_api_physical_drive_fw_download_asynch",
        usage_format,
        argc, 8);
    
    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0); 
    sprintf(params, " object id 0x%x (%s)\n", object_id, *argv);

    // Memory Allocation  
    //dl_context = fbe_memory_native_allocate(sizeof(
        //fbe_physical_drive_fw_info_asynch_t))

    // Check to see if Memory Allocation failed 
    if(stop_context == NULL) 
    {
        sprintf(temp, "Memory Allocation failed for ...\n %s\n",
            " fbe_physical_drive_fw_info_asynch_t");

    // Make api call: 
    } else {
        //start_context->caller_instance = (void*)drive_mgmt;
        start_context->drive_object_id = object_id;

        //fbe_physical_drive_fw_info_asynch_t *context = NULL;
        //start_context->completion_function = 
            //fwdl_fw_download_async_completion(context);

        //status = fbe_api_physical_drive_fw_download_asynch(
        //    dl_context);

        // Check status of call
        if (status != FBE_STATUS_OK){
            sprintf(temp, "Can't fw download");
        } else {
           sprintf(temp, "<status> 0x%x", status);   
        }
        //fbe_memory_native_release(dl_context);
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 * PhyDrive class :: edit_drive_info (private method)
 *********************************************************************/

void PhyDrive::edit_drive_info(fbe_physical_drive_information_t *d) 
{
    // format drive information
    sprintf(temp, "\n "
	        "<vendor>         %s\n <product>        %s\n "
        "<rev length>     %d\n <serial no>      %s\n "
        "<tla part no>    %s\n <dg_part_number> %s\n "
        "<description>    %s\n <bridge_hw_rev>  %s\n "
        "<speed>          %d\n <drive_vendor>   %d\n "
        "<drive_type>     %d\n <block_count>    0x%llX\n "
        "<block_size>     %d\n <queue depth>    0x%x\n "
        "spindown_qual>   %d\n <price class>    0x%x\n "
        "<rev>            %s\n <capacity>       0x%llX\n "
        "<product rev>    %s\n <drive_RPM>      %d\n ", 
            d->vendor_id,              
            d->product_id,
            d->product_rev_len,        
            d->product_serial_num, 
            d->tla_part_number,        
            d->dg_part_number_ascii,
            d->drive_description_buff, 
            d->bridge_hw_rev,
            d->speed_capability,       
            d->drive_vendor_id,
            d->drive_type,
            (unsigned long long)d->block_count, 
            d->block_size,
            d->drive_qdepth,
            d->spin_down_qualified,
            d->drive_price_class,
            d->product_rev,
            d->gross_capacity,
            d->product_rev,
            d->drive_RPM);
}

/*********************************************************************
 * PhyDrive class :: edit_asynch_drive_info (private method)
 *********************************************************************/

void PhyDrive::edit_asynch_drive_info(
    fbe_physical_drive_information_asynch_t *d)
{
    sprintf(temp,                              "\n "
        "<vendor>        %s\n <product>       %s\n "
        "<rev length>    %d\n <serial no>     %s\n ",
        /*
        "<tla part no>   %s\n <dg_part_number>%s\n "
        "<description>   %s\n <bridge_hw_rev> %s\n ",
        "<speed>         %d\n <drive_vendor>  %d\n "
        "<drive_type>    %d\n <block_count>   0x%llX\n "
        "<block_size>    %d\n <queue depth>   0x%x\n "
        "spindown_qual>  %d\n <price class>   0x%x\n "
        "<rev>           %s\n <capacity>      0x%I64X\n "
        "<product rev>   %s\n <drive_RPM>     %d\n ", 
        */
            (d->get_drive_info).vendor_id,              
            (d->get_drive_info).product_id,
            (d->get_drive_info).product_rev_len,        
            (d->get_drive_info).product_serial_num);
            /*
            (d->get_drive_info).tla_part_number,        
            (d->get_drive_info).dg_part_number_ascii,
            (d->get_drive_info).drive_description_buff, 
            (d->get_drive_info).bridge_hw_rev); 
            (d->get_drive_info).speed_capability,       
            (d->get_drive_info).drive_vendor_id,
            (d->get_drive_info).drive_type,
            (unsigned long long)(d->get_drive_info).block_count, 
            (d->get_drive_info).block_size,
            (d->get_drive_info).drive_qdepth,
            (d->get_drive_info).spin_down_qualified,
            (d->get_drive_info).drive_price_class,
            (d->get_drive_info).product_rev,
            (d->get_drive_info).gross_capacity,
            (d->get_drive_info).product_rev),
            (d->get_drive_info).disk_RPM);
            */
}

/*********************************************************************
 * PhyDrive class :: edit_diag_info (private method)
 *********************************************************************/

void PhyDrive::edit_diag_info(
   fbe_physical_drive_receive_diag_pg82_t *d, fbe_u8_t page_code) 
{
    sprintf(temp,                                                      "\n "
        "<page code>               0x%x\n <Initiate Port>           0x%x\n "
        "<Port A: Invalid Dword>   0x%x\n <Port A: Loss of Sync>    0x%x\n "
        "<Port A: PHY Reset>       0x%x\n <Port A: Invalid CRC>     0x%x\n "
        "<Port A: Disparity Error> 0x%x\n <Port B: Invalid Dword>   0x%x\n "
        "<Port B: Loss of Sync>    0x%x\n <Port B: PHY Reset>       0x%x\n "   
        "<Port B: Invalid CRC>     0x%x\n <Port B: Disparity Error> 0x%x\n ",
            page_code,
            d->initiate_port,
            d->port[0].invalid_dword_count,
            d->port[0].loss_sync_count,
            d->port[0].phy_reset_count,
            d->port[0].invalid_crc_count,
            d->port[0].disparity_error_count,
            d->port[1].invalid_dword_count,
            d->port[1].loss_sync_count,
            d->port[1].phy_reset_count,
            d->port[1].invalid_crc_count,
            d->port[1].disparity_error_count);
}

/*********************************************************************
 * PhyDrive class :: edit_error_counts (private method)
 *********************************************************************/

void PhyDrive::edit_error_counts(
    fbe_physical_drive_mgmt_get_error_counts_t *e) 
{
    sprintf(temp,                 "\n "
        "<reset count>         0x%x\n "
        "<remap block count>   0x%x\n "
        "<error count>         0x%x\n "
        "<IO Count>            0x%08llx\n "
        "<Cummulative Err>     0x%llX\n "
        "<Recovered Error>     0x%llX\n "
        "<Media Error>         0x%llX\n "
        "<Hardware Error>      0x%llX\n "
        "<Link Error>          0x%llX\n "
        "<Data Error>          0x%llX\n "
        "<Reset Error>         0x%llX\n "
        "<Cummulative Ratio>   %d\n "
        "<Recovered Err Ratio> %d\n "
        "<Media Error Ratio>   %d\n "
        "<Hardware Err Ratio>  %d\n "
        "<Link Error Ratio>    %d\n "
        "<Data Error Ratio>    %d\n "
        "<Reset Ratio>         %d\n "
        "<Power Cycle Ratio>   %d\n ",
            e->physical_drive_error_counts.reset_count,
            e->physical_drive_error_counts.remap_block_count,
            e->physical_drive_error_counts.error_count,
            (unsigned long long)e->drive_error.io_counter,
            (unsigned long long)e->drive_error.io_error_tag,
            (unsigned long long)e->drive_error.recovered_error_tag,  
            (unsigned long long)e->drive_error.media_error_tag,
            (unsigned long long)e->drive_error.hardware_error_tag,
            (unsigned long long)e->drive_error.link_error_tag,
            (unsigned long long)e->drive_error.data_error_tag,
            (unsigned long long)e->drive_error.reset_error_tag,
            e->io_error_ratio,
            e->recovered_error_ratio,
            e->media_error_ratio,
            e->hardware_error_ratio,
            e->link_error_ratio,
            e->data_error_ratio,
            e->reset_error_ratio,
            e->power_cycle_error_ratio);
}

/*********************************************************************
 * PhyDrive class :: edit_buffer (private method)
 *********************************************************************/

void PhyDrive::edit_buffer(
    char *temp, fbe_u8_t *buffer, fbe_u32_t bytes, fbe_lba_t block) 
{
    char edit[8];

    sprintf(temp, "Data block: 0x%llx", (unsigned long long)block);
    
    for (fbe_u32_t ix = 0; ix < bytes; ix++) {
        sprintf(edit, "%.2x ", buffer[ix]);
        strcat(temp, edit);
    }
}

/*********************************************************************
* PhyDrive::Phy_Drive_Intializing_variable (private method)
*********************************************************************/
void PhyDrive::Phy_Drive_Intializing_variable()
{
    enclosure = 0;
    port = 0;
    slot = 0;
    fbe_zero_memory(&drive_information,sizeof(drive_information));
    fbe_zero_memory(&phys_location,sizeof(phys_location));
}

/*********************************************************************
*  PhyDrive::  clear_eol(
*    int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Clear the EOL (End Of Life) for the specified disk.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyDrive :: clear_eol(int argc, char**argv){

    // Check that all arguments have been entered
    status = call_preprocessing(
        "ClearEol",
        "PhyDrive::clear_eol",
        "fbe_api_physical_drive_clear_eol",
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

    //make API call
        status = fbe_api_physical_drive_clear_eol(object_id, 
            FBE_PACKET_FLAG_NO_ATTRIB);
    if(status != FBE_STATUS_OK){
        sprintf(temp, "Failed to clear EOL on physical drive with "
            "object id 0x%x", object_id);
    }
    else {
        sprintf(temp,"EOL cleared successfully for physical drive "
            "with object id 0x%x", object_id );
    }

        call_post_processing(status, temp, key, params);
        return status;
}



fbe_status_t PhyDrive::get_object_id_from_bes(char* b_e_s)
{    
    fbe_u32_t  scanned = 0;

    sprintf(params, " %s ", b_e_s);

    scanned = sscanf((const char*)b_e_s,
              "%d_%d_%d", &(port), &(enclosure), &(slot));

    if (scanned < 3){
        printf("Invalid disk number %s\n", b_e_s);
        return FBE_STATUS_INVALID;
    }



    if(status != FBE_STATUS_OK){
        printf("ERROR: Conversion of b_e_s failed for: %s\n",b_e_s);
        return status;
    }

    // Make api call: 
    status = fbe_api_get_physical_drive_object_id_by_location(
        port, 
        enclosure, 
        slot, 
        &object_id);
    fbe_status_t passFail = status;

    // Check status of call
    if (object_id == (fbe_u32_t) FBE_OBJECT_ID_INVALID) {
        strcpy(temp, "FBE_OBJECT_ID_INVALID");
        passFail = FBE_STATUS_GENERIC_FAILURE;

    }else if (status != FBE_STATUS_OK) {
        strcpy(temp, "Can't get object id for drive ");

    }else if (status == FBE_STATUS_OK){ 
        sprintf(temp, "<object id> 0x%x (%u)", 
            object_id, object_id);
    }     

    return passFail;
}




/*********************************************************************
*  PhyDrive::  send_pass_thru(
*    int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Special Engineering method for sending pass thru cmds.  Use
*      with CARE!   Please contact Wayne Garrett for support.
*
*********************************************************************/
fbe_status_t PhyDrive::send_pass_thru(int argc, int index, char ** argv)
{
#if DBG
    fbe_physical_drive_send_pass_thru_t *mode_sense_op = NULL;    

    // Setup default args
    fbe_char_t *arg_cdb_file = "PASS_THRU_CDB.BIN";        
    fbe_char_t *arg_data_in_file = "PASS_THRU_DATA_IN.BIN";
    fbe_bool_t arg_data_out = FBE_FALSE;    
    fbe_char_t *arg_data_out_file = "PASS_THRU_DATA_OUT.BIN";
    fbe_u16_t arg_buff_len = 2048;  

    // Define specific usage string for PASSTHRU
    usage_format =
        " Usage: %s [B_E_D] [Options]\n"\
        "   Options: \n"\
        "      --cdb_file [file]        : Input cdb binary file.  Defaults to ./PASS_THRU_CDB.BIN\n"\
        "      --data_in_file [file]    : Binary file to copy the drive's response to.   Defaults to ./PASS_THRU_DATA_IN.BIN\n"\
        "      --data_out               : Data will be xfered to drive.  If not provided then assumes a data_in cmd\n"\
        "      --data_out_file [file]   : Binary file containing data to send.  defaults to ./PASS_THRU_DATA_OUT.BIN\n"\
        "      --buff_len [nbytes]      : Increase data_in buff len if too small.  defaults 1024.\n"\
        " For example: %s 1_2_3";


    // Check that all arguments have been entered
    status = call_preprocessing(
        "PassThru",
        "PhyDrive::send_pass_thru",
        "fbe_api_physical_drive_send_pass_thru",
        usage_format,
        argc, 7);


    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // *** Handle b_e_s argument
    index++;
    // first verify its a b_e_s format, else assume it's an object id.
    if (strstr(argv[index], "_") != NULL)
    {
        status = get_object_id_from_bes(argv[index]);
        printf("object id 0x%x (%s)\n", object_id, argv[index]);
    
        if (status != FBE_STATUS_OK)
        {
            sprintf(temp, "failed to parse b_e_s: %s.\n", argv[index]);
            status = FBE_STATUS_GENERIC_FAILURE;
            call_post_processing(status, temp, key, params);
            return status;
        }
    }
    else  // assume object ID
    {
        object_id = (fbe_u32_t)strtoul(argv[index], 0, 0);
        printf("object id 0x%x (%s)", object_id, argv[index]);
    }


    // *** Parse remaining cmd specific args
    index++;
    while (index < argc)
    {
        if (strncmp (argv[index], "--CDB_FILE", 20) == 0) {
            index++;
            if ( index >= argc)
            {
                sprintf(temp, "missing arg: %s\n", argv[index-1]);
                status = FBE_STATUS_GENERIC_FAILURE;
                call_post_processing(status, temp, key, params);
                return status;
            }
            arg_cdb_file  = argv[index];
        }
        else if (strncmp (argv[index], "--DATA_IN_FILE", 20) == 0) {
            index++;
            if ( index >= argc)
            {
                sprintf(temp, "missing arg: %s\n", argv[index-1]);
                status = FBE_STATUS_GENERIC_FAILURE;
                call_post_processing(status, temp, key, params);
                return status;
            }
            arg_data_in_file  = argv[index];
        }

        else if (strncmp (argv[index], "--DATA_OUT", 20) == 0) {
            arg_data_out = FBE_TRUE;
        }
        else if (strncmp (argv[index], "--DATA_OUT_FILE", 20) == 0) {
            index++;
            if ( index >= argc)
            {
                sprintf(temp, "missing arg: %s\n", argv[index-1]);
                status = FBE_STATUS_GENERIC_FAILURE;
                call_post_processing(status, temp, key, params);
                return status;
            }
            arg_data_out = FBE_TRUE;
            arg_data_out_file  = argv[index];
        }
        else if (strncmp (argv[index], "--BUFF_LEN", 10) == 0)
        {
            index++;
            if ( index >= argc)
            {
                sprintf(temp, "missing arg: %s\n", argv[index-1]);
                status = FBE_STATUS_GENERIC_FAILURE;
                call_post_processing(status, temp, key, params);
                return status;
            }
            arg_buff_len = (fbe_u16_t)strtoul(argv[index], 0, 0);
        }
        else
        {
            sprintf(temp, "invalid arg: %s\n", argv[index]);
            status = FBE_STATUS_GENERIC_FAILURE;
            call_post_processing(status, temp, key, params);
            return status;            
        }
        index++;
    }


    // *** allocate pass thru cmd
    mode_sense_op = (fbe_physical_drive_send_pass_thru_t*) fbe_api_allocate_memory(sizeof(fbe_physical_drive_send_pass_thru_t));
    if (NULL == mode_sense_op)
    {
        sprintf(temp, "%s failed pass thru alloc.\n",__FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }
    fbe_zero_memory(mode_sense_op, sizeof(fbe_physical_drive_send_pass_thru_t));


    // *** allocate data in/out buffer        
    mode_sense_op->response_buf = (fbe_u8_t*)fbe_api_allocate_memory(arg_buff_len);
    if (NULL == mode_sense_op->response_buf)
    {
        sprintf(temp, "%s failed recv buff alloc.\n",__FUNCTION__);  
        status = FBE_STATUS_GENERIC_FAILURE;      
        call_post_processing(status, temp, key, params);
        fbe_api_free_memory(mode_sense_op);
        return status;
    }


    // *** open cdb file and init cdb structure
    fbe_file_handle_t fp = fbe_file_open(arg_cdb_file, FBE_FILE_RDONLY, 0, NULL);
    if(fp == FBE_FILE_INVALID_HANDLE)
    {
       sprintf(temp,"%s:Failed to open pass thru file %s\n", 
                      __FUNCTION__, arg_cdb_file);
       status = FBE_STATUS_GENERIC_FAILURE;
       call_post_processing(status, temp, key, params);
       fbe_api_free_memory(mode_sense_op->response_buf);
       fbe_api_free_memory(mode_sense_op);
       return status;	 
    }

    fbe_u32_t cdb_len = (fbe_u32_t)fbe_file_get_size(fp);
    int bytes_read = fbe_file_read(fp, (char*)mode_sense_op->cmd.cdb, cdb_len, NULL);
    fbe_file_close(fp);

    if (cdb_len > 16)
    {
       sprintf(temp,"CDB file len: %d > 16.   Not supported.\n", cdb_len);
       status = FBE_STATUS_GENERIC_FAILURE;
       call_post_processing(status, temp, key, params);
       fbe_api_free_memory(mode_sense_op->response_buf);
       fbe_api_free_memory(mode_sense_op);       
       return status;
    }
    if (bytes_read != cdb_len)
    {
        sprintf(temp,"%s: Bytes read (%d) != cdb len (%d).\n", __FUNCTION__, bytes_read, cdb_len);
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        fbe_api_free_memory(mode_sense_op->response_buf);
        fbe_api_free_memory(mode_sense_op);        
        return status;
    }
    if (cdb_len !=6 && cdb_len != 10 && cdb_len != 12 && cdb_len != 16)
    {
        printf("CDB file len: %d doesn't matched expected 6 ,10 ,12 or 16. Continuing anyway...\n", cdb_len);
    }

    mode_sense_op->cmd.cdb_length = cdb_len;


    printf("CDB: \n");
    int i=0;
    while (i < cdb_len)
    {
        printf("%02x ", mode_sense_op->cmd.cdb[i]);
        i++;

        if (i%16 == 0)
        {
            printf("\n");
        }
    }
    printf("\n");


    mode_sense_op->cmd.transfer_count = arg_buff_len;    
    mode_sense_op->cmd.payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    if (arg_data_out){
        mode_sense_op->cmd.payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;
    }
    else{
        mode_sense_op->cmd.payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;
    }
    mode_sense_op->status = FBE_STATUS_PENDING;


    // *** if DATA OUT then open data_out and init buffer
    if (arg_data_out)
    {
        /* open file */
        fbe_file_handle_t fp = fbe_file_open(arg_data_out_file, FBE_FILE_RDONLY, 0, NULL);
        if(fp == FBE_FILE_INVALID_HANDLE)
        {
           sprintf(temp,"%s:Failed to open pass thru file %s\n", 
                          __FUNCTION__, arg_data_out_file);
           status = FBE_STATUS_GENERIC_FAILURE;
           call_post_processing(status, temp, key, params);
           fbe_api_free_memory(mode_sense_op->response_buf);
           fbe_api_free_memory(mode_sense_op);
           return status;	 
        }

        fbe_u32_t buff_len = (fbe_u32_t)fbe_file_get_size(fp);

        if (buff_len > arg_buff_len)
        {
            /* need to free and reallocate to increase buffer.  */

            fbe_api_free_memory(mode_sense_op->response_buf);

            mode_sense_op->response_buf = (fbe_u8_t*)fbe_api_allocate_memory(buff_len);
            if (NULL == mode_sense_op->response_buf)
            {
                sprintf(temp, "%s failed recv buff alloc2.\n",__FUNCTION__);  
                status = FBE_STATUS_GENERIC_FAILURE;      
                call_post_processing(status, temp, key, params);
                fbe_api_free_memory(mode_sense_op);
                fbe_file_close(fp);
                return status;
            }            
        }

        mode_sense_op->cmd.transfer_count = buff_len;

        fbe_file_read(fp, (char*)mode_sense_op->response_buf, buff_len, NULL); 
        fbe_file_close(fp);

        printf("DATA_OUT: \n");
        int i=0;
        while (i < buff_len)
        {
            printf("%02x ", mode_sense_op->response_buf[i]);
            i++;

            if (i%16 == 0)
            {
                printf("\n");
            }
        }
        printf("\n");
    }


    // *** Send pass-thru cmd
    status = fbe_api_physical_drive_send_pass_thru(object_id, mode_sense_op);
    if (status != FBE_STATUS_OK || 
        mode_sense_op->cmd.port_request_status != FBE_PORT_REQUEST_STATUS_SUCCESS || 
        mode_sense_op->cmd.payload_cdb_scsi_status != FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD) 
    {
        printf("pass_thru cmd failed. return stat:%d, port stat:%d, scsi stat:0x%x\n", 
                status, mode_sense_op->cmd.port_request_status, mode_sense_op->cmd.payload_cdb_scsi_status);

        if (mode_sense_op->cmd.payload_cdb_scsi_status == FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION)
        {
            char *cc_file = "CHECK_CONDIITON.BIN";
            printf("Check Condition occurred.  Writing it to ./%s\n", cc_file);


            /* create data_in file and write drive's response to it */
            fbe_file_handle_t fp_cc = fbe_file_creat(cc_file, FBE_FILE_RDWR);
            if(fp_cc == FBE_FILE_INVALID_HANDLE)
            {
                sprintf(temp,"%s:Failed to create data in file %s\n",
                        __FUNCTION__, cc_file);
            }
            else
            {
                /* write out drive response */
                fbe_u32_t bytes_sent = fbe_file_write(fp_cc, mode_sense_op->cmd.sense_info_buffer, sizeof(mode_sense_op->cmd.sense_info_buffer), NULL);

                if (bytes_sent != sizeof(mode_sense_op->cmd.sense_info_buffer))
                {
                    // Error, we didn't transfer what we expected.
                    printf("error writing sense data.  bytes_written = %d, bytes_expected = %d. \n",
                           bytes_sent, (int)sizeof(mode_sense_op->cmd.sense_info_buffer));
                }

                /* cleanup */
                fbe_file_close(fp_cc);
            }
        }
        else
        {
            printf("Search traces for errors reported from drive\n");
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }  
    else 
    {
        sprintf(temp,"Pass Thru successfully for physical drive "
            "with object id 0x%x", object_id );
    }
        
    printf("DATA_IN: len=%d\n",mode_sense_op->cmd.transfer_count);
    i=0;
    while (i < mode_sense_op->cmd.transfer_count)
    {
        printf("%02x ", mode_sense_op->response_buf[i]);
        i++;

        if (i%16 == 0)
        {
            printf("\n");
        }
    }
    printf("\n");


    /* create data_in file and write drive's response to it */
    fbe_file_handle_t fp_data_in = fbe_file_creat(arg_data_in_file, FBE_FILE_RDWR);
    if(fp_data_in == FBE_FILE_INVALID_HANDLE)
    {
       sprintf(temp,"%s:Failed to create data in file %s\n", 
                      __FUNCTION__, arg_data_in_file);
       status = FBE_STATUS_GENERIC_FAILURE;
       call_post_processing(status, temp, key, params);
       fbe_api_free_memory(mode_sense_op->response_buf);
       fbe_api_free_memory(mode_sense_op);
       return status;	 
    }
    
    /* write out drive response */
    fbe_u32_t bytes_sent = fbe_file_write(fp_data_in, mode_sense_op->response_buf, mode_sense_op->cmd.transfer_count, NULL);

    if (bytes_sent != mode_sense_op->cmd.transfer_count)
    {
        // Error, we didn't transfer what we expected.
        sprintf(temp,"drive object id 0x%x unexpected transfer. bytes_written = 0x%x, bytes_expected = 0x%x. \n",
                      object_id, bytes_sent, mode_sense_op->cmd.transfer_count);

        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params); 
        fbe_api_free_memory(mode_sense_op->response_buf);
        fbe_api_free_memory(mode_sense_op);
        fbe_file_close(fp_data_in);
        return status;
    }

    /* cleanup */
    fbe_file_close(fp_data_in);



    call_post_processing(status, temp, key, params);

    fbe_api_free_memory(mode_sense_op->response_buf);
    fbe_api_free_memory(mode_sense_op);

    return status;

#else
    sprintf(temp,"Not Supported");
    call_post_processing(FBE_STATUS_GENERIC_FAILURE, temp, key, params);
    return FBE_STATUS_GENERIC_FAILURE;
#endif
}

/*********************************************************************
 * PhyDrive end of file
 *********************************************************************/
