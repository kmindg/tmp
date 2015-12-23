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
*      This file defines the methods of the SEP PROVISION DRIVE INTERFACE class.
*
*  Notes:
*      The SEP Provision Drive class (sepProvisionDrive) is a derived class of 
*      the base class (bSEP).
*
*  History:
*      29-March-2011 : initial version
*
*********************************************************************/

#ifndef T_SEP_PROVISION_DRIVE_CLASS_H
#include "sep_provision_drive_class.h"
#endif

/*********************************************************************
* sepProvisionDrive class :: Constructor
*********************************************************************/

sepProvisionDrive::sepProvisionDrive() : bSEP()
{
    // Store Object number
    idnumber = (unsigned) SEP_PROVISION_DRIVE_INTERFACE;
    pvdrCount = ++gSepProvisionDriveCount;

    // Store Object name
    idname = new char [64];
    strcpy(idname, "SEP_PROVISION_DRIVE_INTERFACE");
    Sep_Provision_Intializing_variable();

    if (Debug) {
        sprintf(temp, 
            "sepProvisionDrive class constructor (%d) %s\n", 
            idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP Provision Drive Interface function calls>
    sepProvisionDriveInterfaceFuncs = 
        "[function call]\n" \
        "[short name] [FBE API SEP Provision Drive Interface]\n" \
        " -----------------------------------------------------------------------------\n" \
        " getObjbyDr               fbe_api_provision_drive_get_obj_id_by_location\n" \
        " getProvisionDrObjInfo    fbe_api_get_pvd_object_info\n" \
        " getPvdObjDrLocation      fbe_api_get_pvd_object_drive_location\n" \
        " getBackgroundPriorities  fbe_api_provision_drive_get_background_priorities\n" \
        " -----------------------------------------------------------------------------\n" \
        " initiateDiskZeroing      fbe_api_provision_drive_initiate_disk_zeroing\n" \
        " getDiskZeroChkpt         fbe_api_provision_drive_get_zero_checkpoint\n" \
        " getDiskZeroPct           fbe_api_provision_drive_get_zero_percentage\n" \
        " setDiskZeroChkpt         fbe_api_provision_drive_set_zero_checkpoint\n "\
        " -----------------------------------------------------------------------------\n" \
        " setSniffVerify           fbe_api_job_service_update_pvd_sniff_verify and fbe_api_common_wait_for_job\n" \
/*        " enableVerify             fbe_api_provision_drive_enable_verify\n" \
        " disableVerify            fbe_api_provision_drive_disable_verify\n" \ */
        " getVerifyReport          fbe_api_provision_drive_get_verify_report\n" \
        " clearVerifyReport        fbe_api_provision_drive_clear_verify_report\n" \
        " getDiskVerifyStatus      fbe_api_provision_drive_get_verify_status\n" \
        " -----------------------------------------------------------------------------\n" \
        " updateConfigType         fbe_api_provision_drive_update_config_type\n" \
        " getSpareDrivePool        fbe_api_provision_drive_get_spare_drive_pool\n" \
        " setConfigType            fbe_api_provision_drive_set_config_type\n" \
        " -----------------------------------------------------------------------------\n" \
        " getPvdPoolId             fbe_api_provision_drive_get_pool_id\n" \
        " setPvdPoolId             fbe_api_provision_drive_update_pool_id\n" \
        " setPvdEolState           fbe_api_provision_drive_set_eol_state\n" \
        " -----------------------------------------------------------------------------\n"\
        " setVerifyCheckpoint      fbe_api_provision_drive_set_verify_checkpoint\n" \
        " -----------------------------------------------------------------------------\n"\
        " copytoReplDisk           fbe_api_copy_to_replacement_disk\n"\
        " copytoProactiveSpare     fbe_api_provision_drive_user_copy\n"\
        " getCopyToCheckpoint      fbe_api_virtual_drive_get_checkpoint\n"\
        " -----------------------------------------------------------------------------\n"\
        " packagedClearEol         fbe_api_provision_drive_clear_eol_state\n"\
        " clearEol                 fbe_api_provision_drive_clear_eol_state\n"\
        " -----------------------------------------------------------------------------\n"\
        " mapLbaToChunk            fbe_api_provision_drive_map_lba_to_chunk\n"\
        " getPagedMetadata         fbe_api_provision_drive_get_paged_metadata\n"\
        " -----------------------------------------------------------------------------\n"
        " isSlf                    fbe_api_provision_drive_is_slf\n"\
        " clearDriveFaultFlag      fbe_api_provision_drive_clear_drive_fault\n"\
        " -----------------------------------------------------------------------------\n";

    // Define common usage for provision drive commands
    usage_format = 
        " Usage: %s [pvd object id]\n"
        " For example: %s 0x105";
};

/*********************************************************************
* sepProvisionDrive class : Accessor methods
*********************************************************************/

unsigned sepProvisionDrive::MyIdNumIs(void)
{
    return idnumber;
};

char * sepProvisionDrive::MyIdNameIs(void)
{
    return idname;
};

void sepProvisionDrive::dumpme(void)
{ 
    strcpy (key, "sepProvisionDrive::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, pvdrCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* sepProvisionDrive Class :: select()
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
*      29-March-2011 : initial version
*
*********************************************************************/

fbe_status_t sepProvisionDrive::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface"); 

    // List SEP Provision Drive calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepProvisionDriveInterfaceFuncs );

        // get object Id by location (port, enclosure, slot)
    } else if (strcmp (argv[index], "GETOBJBYDR") == 0) {
        status = get_object_id(argc, &argv[index]);

        // get PVD obj info
    } else if (strcmp (argv[index], "GETPROVISIONDROBJINFO") == 0) {
        status = get_pvd_object_info(argc, &argv[index]);

        // get drive location from PVD obj
    } else if (strcmp (argv[index], "GETPVDOBJDRLOCATION") == 0) {
        status = get_pvd_obj_dr_location(argc, &argv[index]);

        // get background priorities
    } else if (strcmp (argv[index], "GETBACKGROUNDPRIORITIES") == 0) {
        status = get_background_priorities(argc, &argv[index]);

        // initiate disk zero
    } else if (strcmp (argv[index], "INITIATEDISKZEROING") == 0) {
        status = initiate_disk_zeroing(argc, &argv[index]);

        // get disk zero checkpoint
    } else if (strcmp (argv[index], "GETDISKZEROCHKPT") == 0) {
        status = get_disk_zero_checkpoint(argc, &argv[index]);

        // get disk zero percent
    } else if (strcmp (argv[index], "GETDISKZEROPCT") == 0) {
        status = get_disk_zero_percentage(argc, &argv[index]);

        // enable or disable sniff verify
    } else if (strcmp (argv[index], "SETSNIFFVERIFY") == 0) {
        status = set_sniff_verify(argc, &argv[index]);

        // get sniff verify report
    } else if (strcmp (argv[index], "GETVERIFYREPORT") == 0){
        status = get_verify_report(argc, &argv[index]);

        // clear sniff verify report for disk
    } else if (strcmp (argv[index], "CLEARVERIFYREPORT") == 0) {
        status = clear_verify_report(argc, &argv[index]);

        // get disk verify status
    } else if (strcmp (argv[index], "GETDISKVERIFYSTATUS") == 0) {
        status = get_disk_verify_status(argc, &argv[index]);

        // update config type (configure/ unconfigure HotSpare )
    } else if (strcmp (argv[index], "UPDATECONFIGTYPE") == 0) {
        status = update_config_type(argc, &argv[index]);

        // get spare drive pool
    } else if (strcmp (argv[index], "GETSPAREDRIVEPOOL") == 0) {
        status = get_spare_drive_pool(argc, &argv[index]);

        // get pvd pool id
    } else if (strcmp (argv[index], "GETPVDPOOLID") == 0) {
        status = get_pvd_pool_id(argc, &argv[index]);
        
        // set pvd pool id
    } else if (strcmp (argv[index], "SETPVDPOOLID") == 0) {
        status = set_pvd_pool_id(argc, &argv[index]);

        // set end of life state for pvd
    } else if (strcmp (argv[index], "SETPVDEOLSTATE") == 0) {
        status = set_pvd_eol_state(argc, &argv[index]);

        // Sets the Verify Checkpoint
    } else if (strcmp (argv[index], "SETVERIFYCHECKPOINT") == 0) {
        status = set_verify_checkpoint(argc, &argv[index]);

    // Copy to replacement disk
    } else if (strcmp (argv[index], "COPYTOREPLDISK") == 0) {
        status = copy_to_replacement_disk(argc, &argv[index]);

    //Copy to proactive spare
    } else if (strcmp (argv[index], "COPYTOPROACTIVESPARE") == 0) {
        status = copy_to_proactive_spare(argc, &argv[index]);

       // Sets drive zero check point
    } else if (strcmp (argv[index], "SETDISKZEROCHKPT") == 0) {
        status = set_disk_zero_checkpoint(argc, &argv[index]);

    // Clear EOL on all PVD,pld and pdo
    } else if (strcmp (argv[index], "PACKAGEDCLEAREOL" ) == 0){
        status = packaged_clear_eol_state(argc, &argv[index]);

    // Clear EOL on PVD
    } else if (strcmp (argv[index], "CLEAREOL" ) == 0){
        status = clear_eol_state(argc, &argv[index]);

    // set config type 
    } else if (strcmp (argv[index], "SETCONFIGTYPE") == 0) {
        status = set_config_type(argc, &argv[index]);


    // Map lba to chunk
    } else if (strcmp (argv[index], "MAPLBATOCHUNK" ) == 0){
        status = map_lba_to_chunk(argc, &argv[index]);

    // Get paged metadata
    } else if (strcmp (argv[index], "GETPAGEDMETADATA" ) == 0){
        status = get_paged_metadata(argc, &argv[index]);

    // Get copyto checkpoint
    } else if (strcmp (argv[index], "GETCOPYTOCHECKPOINT" ) == 0){
        status = get_copyto_check_point(argc, &argv[index]);

    // set slf
    } else if (strcmp (argv[index], "ISSLF" ) == 0){
        status = is_slf(argc, &argv[index]);

    // clear drive fault bit
    } else if (strcmp (argv[index], "CLEARDRIVEFAULTFLAG" ) == 0){
        status = clear_drive_fault_bit(argc, &argv[index]);

    // can't find match on short name
    }  else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepProvisionDriveInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
*  sepProvisionDrive class :: get_object_id ()
*********************************************************************
 *
*  Description:
*      Get PVD object ID.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepProvisionDrive::get_object_id(int argc, char **argv) 
{
    // Assign default values
    pvd_object_id = FBE_OBJECT_ID_INVALID;

    // Define specific usage string  
    usage_format =
        " Usage: %s [disk number in B_E_D]\n"\
        " For example: %s 1_2_3";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getObjbyDr",
        "sepProvisionDrive::object_id_by_location,",
        "fbe_api_provision_drive_get_obj_id_by_location",  
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
    status = fbe_api_provision_drive_get_obj_id_by_location(
                port, 
                enclosure, 
                slot, 
                &pvd_object_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get object id for drive %d %d %d",
                port, enclosure, slot);

    }else if (status == FBE_STATUS_OK){ 
        sprintf(temp, "<pvd object id> 0x%X (%u)", 
                pvd_object_id, pvd_object_id);
    }     

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive class :: get_pvd_object_info () 
*********************************************************************
*
*  Description:
*      Get PVD object information.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepProvisionDrive::get_pvd_object_info (int argc ,
                                                     char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getProvisionDrObjInfo",
        "sepProvisionDrive::get_pvd_object_info",
        "fbe_api_get_pvd_object_info",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " pvd object id 0x%x (%s)", pvd_object_id, *argv);

    // Make api call: 
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get PVD Object Info");
    } else {
        edit_pvd_obj_info(&provision_drive_info); 
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive class :: get_pvd_obj_dr_location () 
*********************************************************************
 *
*  Description:
*      Get pvd drive location
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepProvisionDrive::get_pvd_obj_dr_location (int argc ,
                                                         char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getPvdObjDrLocation",
        "sepProvisionDrive::get_pvd_obj_dr_location",
        "fbe_api_get_pvd_object_drive_location",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " pvd object id 0x%x (%s)", pvd_object_id, *argv);

    // Make api call: 
    status = fbe_api_provision_drive_get_location(pvd_object_id,
             &port, &enclosure, &slot);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get Drive location");
    } else {
        sprintf(temp, "<DRIVE location> %d_%d_%d",
                port, enclosure, slot);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive class :: get_background_priorities () 
*********************************************************************
 *
*  Description:
*      Get background priorities for PVD
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepProvisionDrive::get_background_priorities (int argc ,
                                                           char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getBackgroundPriorities",
        "sepProvisionDrive::get_background_priorities",
        "fbe_api_provision_drive_get_background_priorities",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " pvd object id 0x%x (%s)", pvd_object_id, *argv);

    // Make api call: 
    status = fbe_api_provision_drive_get_background_priorities(
             pvd_object_id, &get_priorities);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get background priorities");
    } else {
        edit_background_priorities(&get_priorities);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive class :: initiate_disk_zeroing () 
*********************************************************************
 *
*  Description:
*      Initiate disk zeroing
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepProvisionDrive::initiate_disk_zeroing (int argc ,
                                                       char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "initiateDiskZeroing",
        "sepProvisionDrive::initiate_disk_zeroing",
        "fbe_api_provision_drive_initiate_disk_zeroing",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " pvd object id 0x%x (%s)", pvd_object_id, *argv);

    // Make api call: 
    status = fbe_api_provision_drive_initiate_disk_zeroing(pvd_object_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to start Disk zeroing");
    } else {
        sprintf(temp, "Initiated disk zeroing"); 
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive class :: get_disk_zero_checkpoint () 
*********************************************************************
*
*  Description:
*      Get disk zero checkpoint.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepProvisionDrive::get_disk_zero_checkpoint (int argc ,
                                                          char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getDiskZeroChkpt",
        "sepProvisionDrive::get_disk_zero_checkpoint",
        "fbe_api_provision_drive_get_zero_checkpoint",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " pvd object id 0x%x (%s)", pvd_object_id, *argv);

    // Make api call: 
    status = fbe_api_provision_drive_get_zero_checkpoint(
                    pvd_object_id, &disk_zero_checkpoint);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get Disk zero checkpoint");
    } else {
        edit_disk_zero_chkpt(disk_zero_checkpoint); 
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive class :: get_disk_zero_percentage () 
*********************************************************************
*
*  Description:
*      Get disk zero percentage.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepProvisionDrive::get_disk_zero_percentage (int argc ,
                                                          char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getDiskZeroPct",
        "sepProvisionDrive::get_disk_zero_percentage",
        "fbe_api_provision_drive_get_zero_percentage",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " pvd object id 0x%x (%s)", pvd_object_id, *argv);

    // Make api call: 
    status = fbe_api_provision_drive_get_zero_percentage(
                    pvd_object_id, &disk_zero_percent);

    // Check status of call
    if (status != FBE_STATUS_OK) {

        sprintf(temp, "Failed to get Disk zero checkpoint");

    } else {

        // format disk zero percent
        sprintf(temp,              "\n "
            "<PVD object id>       0x%X\n "
            "<Zero Percent>          %d\n ",
            pvd_object_id,
            disk_zero_percent);

    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepProvisionDrive class :: set_sniff_verify () 
*********************************************************************
*
*  Description:
*      Set sniff verify.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepProvisionDrive::set_sniff_verify (int argc , char ** argv)
{
    fbe_status_t                   job_status = FBE_STATUS_INVALID;
    fbe_job_service_error_type_t   job_error_type;

    // Define specific usage string
    usage_format =
        " Usage: %s [pvd object id] "\
        "[enable sniff verify : 1 | disable sniff verify : 0]\n"\
        " For example: %s 4 1\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setSniffVerify",
        "sepProvisionDrive::set_sniff_verify",
        "fbe_api_job_service_update_pvd_sniff_verify "\
        "and fbe_api_common_wait_for_job",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Get PVD object ID and convert it to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    // get sniff verify state
    argv++;
    state = (fbe_u32_t)strtoul(*argv, 0, 0);
    if (0 == state){
        sniff_verify_state = FBE_FALSE;
    }
    else{
        sniff_verify_state = FBE_TRUE;
    }

    sprintf(params, " pvd_object_id 0x%x sniff verify state (%d)" , 
                    pvd_object_id, state);

    update_pvd_sniff_verify_state.pvd_object_id = pvd_object_id;
    update_pvd_sniff_verify_state.sniff_verify_state = sniff_verify_state;

    // Make api call:
    status = fbe_api_job_service_update_pvd_sniff_verify(
        &update_pvd_sniff_verify_state);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Unable to update sniff verify state for "
                      "provision drive obj id 0x%X",
                      pvd_object_id);
    } else {
    // wait for the notification from the job service.
        status = fbe_api_common_wait_for_job(
            update_pvd_sniff_verify_state.job_number,
            FBE_JOB_SERVICE_WAIT_TIMEOUT,
            &job_error_type,
            &job_status,
            NULL);

        if ((status != FBE_STATUS_OK) || (job_status != FBE_STATUS_OK) || 
            (job_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR)) {

            sprintf(temp, "Unable to set sniff verify state for "
            "provision drive obj id 0x%X, status %d, job status %d, job error type %d",
            pvd_object_id, status, job_status, job_error_type);
            status = FBE_STATUS_GENERIC_FAILURE;

        } else {

            sprintf(temp, "Sniff verify state set to %d "
            "for Provision Drive Obj ID 0x%x ",
            state,
            pvd_object_id);

        }
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepProvisionDrive class :: get_verify_report () 
*********************************************************************
*
*  Description:
*      Get verify report.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepProvisionDrive::get_verify_report (int argc , char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getVerifyReport",
        "sepProvisionDrive::get_verify_report",
        "fbe_api_provision_drive_get_verify_report",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " pvd object id 0x%x (%s)", pvd_object_id, *argv);

    // Make api call: 
    status = fbe_api_provision_drive_get_verify_report(
             pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, &pvd_verify_report);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get sniff verify report");
    } else {
        edit_disk_verify_report(&pvd_verify_report);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive class :: clear_verify_report () 
*********************************************************************
*
*  Description:
*      Clear verify report.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepProvisionDrive::clear_verify_report (int argc ,
                                                     char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "clearVerifyReport",
        "sepProvisionDrive::clear_verify_report",
        "fbe_api_provision_drive_clear_verify_report",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " pvd object id 0x%x (%s)", pvd_object_id, *argv);

    // Make api call: 
    status = fbe_api_provision_drive_clear_verify_report(
             pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to clear sniff verify report");
    } else {
        sprintf(temp, "Cleared sniff verify report");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive class :: get_disk_verify_status ()
*********************************************************************
*
*  Description:
*      Get disk verify status.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepProvisionDrive::get_disk_verify_status (int argc ,
                                                        char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getDiskVerifyStatus",
        "sepProvisionDrive::get_disk_verify_status",
        "fbe_api_provision_drive_get_verify_status",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " pvd object id 0x%x (%s)", pvd_object_id, *argv);

    // Make api call: 
    status = fbe_api_provision_drive_get_verify_status(pvd_object_id,
             FBE_PACKET_FLAG_NO_ATTRIB, &disk_verify_status);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get Disk verify status");
    } else {
        edit_disk_verify_status(&disk_verify_status); 
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive class :: update_config_type () 
*********************************************************************
*
*  Description:
*      Update config type
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepProvisionDrive::update_config_type (int argc , char ** argv)
{
    usage_format = 
        " Usage: %s [config type] [pvd obj id]\n"
        " Config type: c - configures HotSpare\n"
        "              u - unconfigures HotSpare\n"
        " For example: %s c 0x100";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "updateConfigType",
        "sepProvisionDrive::update_config_type",
        "fbe_api_provision_drive_update_config_type",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // increment argv for config type
    argv++;

    // Update config type
    if (is_valid_config_type(argv)) {

        // Convert Object id to hex
        argv++;
        pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
        sprintf(params, " pvd object id 0x%x (%s)", pvd_object_id, *argv);

        update_pvd_config_type.pvd_object_id = pvd_object_id;
        update_pvd_config_type.config_type = config_type;

        // Make api call
        status = fbe_api_provision_drive_update_config_type(
                &update_pvd_config_type);

        // Check status of call
        if (status != FBE_STATUS_OK) {
            sprintf(temp, "Failed to update config type");
        } else {
            sprintf(temp, "Config Type updated to %s", edit_config_type(config_type));
        }
    }
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive class :: is_valid_config_type () (private method)
*********************************************************************
 *
*  Description:
*      check for valid config type. 
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
bool sepProvisionDrive::is_valid_config_type(char ** argv) {

    config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID;

    if(strcmp (*argv,"C") == 0) {
        config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE;

    } else if(strcmp (*argv, "U") == 0){
        config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED;

    } else if(strcmp (*argv, "F") == 0){
        config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_FAILED;

    } else if(strcmp (*argv, "R") == 0){
        config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID;
    } else {
        status = FBE_STATUS_GENERIC_FAILURE;
        sprintf(temp, "Invalid Config type %s.\n %s", *argv, usage_format);
        params[0] = '\0';
        return false;
    }
    
    return true;
}

/*********************************************************************
* sepProvisionDrive class :: get_spare_drive_pool () 
*********************************************************************/

fbe_status_t sepProvisionDrive::get_spare_drive_pool (int argc , char ** argv)
{
    // Define specific usage format
    usage_format = 
        " Usage: %s\n"
        " For example: %s";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getSpareDrivePool",
        "sepProvisionDrive::get_spare_drive_pool",
        "fbe_api_provision_drive_get_spare_drive_pool",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

     params[0] = '\0';
    char * data;

    // Make api call: 
    status = fbe_api_provision_drive_get_spare_drive_pool(&spare_drive_pool);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get spare drive pool");
        data = temp;
    } else {
        //display spare drive list
         data = edit_spare_drive_pool();
    }

    // Output results from call or description of error
    call_post_processing(status, data, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive Class :: get_pvd_pool_id()
*********************************************************************
*
*  Description:
*      Gets the pvd pool id
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepProvisionDrive::get_pvd_pool_id(int argc ,char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getPvdPoolId",
        "sepProvisionDrive::get_pvd_pool_id",
        "fbe_api_provision_drive_get_pool_id",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " pvd object id 0x%x (%s)", pvd_object_id, *argv);

    // Make api call: 
    status = fbe_api_provision_drive_get_pool_id(pvd_object_id, &pool_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get pool id for "
                      "pvd object id 0x%X",
                      pvd_object_id);
    } else {
        edit_pvd_pool_id(pool_id); 
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive Class :: set_pvd_pool_id()
*********************************************************************
*
*  Description:
*      Sets the pvd pool id
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepProvisionDrive::set_pvd_pool_id(int argc ,char ** argv)
{
    usage_format = 
        " Usage: %s [pvd object id] [pool id]\n"
        " For example: %s 3 100";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setPvdPoolId",
        "sepProvisionDrive::set_pvd_pool_id",
        "fbe_api_provision_drive_update_pool_id",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    update_pvd_pool_id.object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " pvd object id 0x%x (%s)", update_pvd_pool_id.object_id, *argv);

    // extract pool id
    argv++;
    update_pvd_pool_id.pool_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " pool id 0x%x (%s)", update_pvd_pool_id.pool_id, *argv);

    // Make api call: 
    status = fbe_api_provision_drive_update_pool_id(&update_pvd_pool_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to set pool id to 0x%x for "
                      "pvd object id 0x%X",
                      update_pvd_pool_id.pool_id, update_pvd_pool_id.object_id);
    } else {
        sprintf(temp, "Pool ID 0x%X has been set for PVD object ID 0x%X",
                      update_pvd_pool_id.pool_id, update_pvd_pool_id.object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive Class :: set_pvd_eol_state()
*********************************************************************
*
*  Description:
*      Sets the end of life state for pvd object
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepProvisionDrive::set_pvd_eol_state(int argc ,char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "setPvdEolState",
        "sepProvisionDrive::set_pvd_eol_state",
        "fbe_api_provision_drive_set_eol_state",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " pvd object id 0x%x (%s)", pvd_object_id, *argv);
    
    // Make api call: 
    status = fbe_api_provision_drive_set_eol_state(
        pvd_object_id, 
        FBE_PACKET_FLAG_NO_ATTRIB);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to set eol state for "
                      "pvd object id 0x%X",
                      pvd_object_id);
    } else {
        sprintf(temp, "EOL state has been set for PVD Object ID 0x%X",
                      pvd_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive class :: edit_disk_zero_chkpt (private method)
*********************************************************************
*
*  Description:
*      edit disk zero check point.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
void sepProvisionDrive::edit_disk_zero_chkpt(fbe_lba_t disk_zero_checkpoint) 
{
    // format disk zero checkpoint information
    sprintf(temp,              "\n "
        "<PVD object id>       0x%X\n "
        "<Zero Chkpt>          0x%llX\n ",
        pvd_object_id,
        (unsigned long long)disk_zero_checkpoint);
}

/*********************************************************************
* sepProvisionDrive class :: edit_disk_verify_report (private method)
*********************************************************************
*
*  Description:
*     Edit the disk verify report
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

void sepProvisionDrive::edit_disk_verify_report(
    fbe_provision_drive_verify_report_t * disk_verify_report) 
{
    // format disk verify status information
    sprintf(temp,                                 "\n "
        "<PVD object id>                          0x%X\n"
        "<Revision>                               %d\n"
        "<Pass Count>                             %d\n"
        "<Total Recoverable Read Errors>          %d\n"
        "<Total UnRecoverable Read Errors>        %d\n"
        "<Current Recoverable Read Errors>        %d\n"
        "<Current UnRecoverable Read Errors>      %d\n"
        "<Previous Recoverable Read Errors>       %d\n"
        "<Previous UnRecoverable Read Errors>     %d\n",
        pvd_object_id,
        disk_verify_report->revision,
        disk_verify_report->pass_count,
        disk_verify_report->totals.recov_read_count,
        disk_verify_report->totals.unrecov_read_count,
        disk_verify_report->current.recov_read_count,
        disk_verify_report->current.unrecov_read_count,
        disk_verify_report->previous.recov_read_count,
        disk_verify_report->previous.unrecov_read_count
        );
}

/*********************************************************************
* sepProvisionDrive class :: edit_disk_verify_status (private method)
*********************************************************************
*
*  Description:
*      Edit disk verify status
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
void sepProvisionDrive::edit_disk_verify_status(
     fbe_provision_drive_get_verify_status_t * disk_verify_status) 
{
    // format disk verify status information
    sprintf(temp,              "\n "
        "<PVD object id>         0x%X\n "
        "<Verify Enable>         %s\n "
        "<Verify Checkpoint>     0x%llX\n "
        "<Exported Capacity>     0x%llX\n "
        "<Percentage Completed>  %d\n ",
        pvd_object_id,
        (disk_verify_status->verify_enable)? "Enabled":"Disabled",
        (unsigned long long)disk_verify_status->verify_checkpoint,
        (unsigned long long)disk_verify_status->exported_capacity,
        disk_verify_status->precentage_completed
        );
}

/*********************************************************************
* sepProvisionDrive class :: edit_pvd_obj_info (private method)
*********************************************************************
*
*  Description:
*      Edit pvd object information
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
void sepProvisionDrive::edit_pvd_obj_info(
     fbe_api_provision_drive_info_t * pvd_info) 
{
    // format provision drive information
    sprintf(temp,              "\n"
        "<PVD object id>                        0x%X\n"
        "<Capacity>                             0x%llX\n"
        "<Config type>                          %d\n"
        "<Configured Physical Block Size>       %d\n"
        "<Max Drive Xfer Limit>                 0x%llX\n"
        "<Debug Flags>                          %d\n"
        "<End Of Life State>                    %s\n"
        "<Media Error LBA>                      0x%llX\n"
        "<Zero On Demand>                       %s\n"
        "<Paged Metadata LBA>                   0x%llX\n"
        "<Paged Metadata Capacity>              0x%llX\n"
        "<Port Number>                          %d\n"
        "<Enclosure Number>                     %d\n"
        "<Slot Number>                          %d\n"
        "<Drive Type>                           %d\n"
        "<Default Offset>                       0x%llX\n"
        "<Drive Fault State>                    %s\n",
        pvd_object_id,
        pvd_info->capacity,
        pvd_info->config_type,
        pvd_info->configured_physical_block_size,
        (unsigned long long)pvd_info->max_drive_xfer_limit,
        pvd_info->debug_flags,
        (pvd_info->end_of_life_state)? "True":"False",
        (unsigned long long)pvd_info->media_error_lba,
        (pvd_info->zero_on_demand)? "True":"False",
        (unsigned long long)pvd_info->paged_metadata_lba,
        (unsigned long long)pvd_info->paged_metadata_capacity,
        pvd_info->port_number,
        pvd_info->enclosure_number,
        pvd_info->slot_number,
        pvd_info->drive_type,
        pvd_info->default_offset,
        pvd_info->drive_fault_state ? "True":"False"
        );
}

/*********************************************************************
* sepProvisionDrive class :: edit_background_priorities (private method)
*********************************************************************
*
*  Description:
*      Edit background priorities
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
void sepProvisionDrive::edit_background_priorities(
     fbe_provision_drive_set_priorities_t *get_priorities) 
{
    // format pvd background priorities information
    sprintf(temp,              "\n "
        "<PVD object id>                0x%X\n "
        "<Zero Priority>                %d\n "
        "<Verify Priority>              %d\n "
        "<Verify Invalidate Priority>   %d\n",
        pvd_object_id,
        get_priorities->zero_priority,
        get_priorities->verify_priority,
        get_priorities->verify_invalidate_priority
        );
}

/*********************************************************************
* sepProvisionDrive class :: edit_spare_drive_pool (private method)
*********************************************************************
*
*  Description:
*      Edit the spare drive pool
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
char* sepProvisionDrive::edit_spare_drive_pool() 
{
    char *data = NULL;

    // format spare drive pool information
    sprintf(temp,              "\n"
        "<Number Of Spares>      %d\n",
        spare_drive_pool.number_of_spare);

    data = utilityClass::append_to(data, temp);

    fbe_u32_t i;
    for (i = 0; i < spare_drive_pool.number_of_spare; i++) {
        sprintf(temp, "<Spare Object %d>      0x%X\n",
                i, spare_drive_pool.spare_object_list[i]);
        data = utilityClass::append_to(data, temp);
    }
    return data;
}

/*********************************************************************
* sepProvisionDrive Class :: edit_unused_drive_as_spare_flag() 
                            (private method)
*********************************************************************
*
*  Description:
*      Format the pvd pool id to display
*
*  Input: Parameters
*      pool_id - pvd pool id

*  Returns:
*      void
*********************************************************************/

void sepProvisionDrive::edit_pvd_pool_id(fbe_u32_t pool_id)
{
    // format pvd pool id to display
    sprintf(temp,              "\n "
        "<PVD pool id>         0x%X\n ",
        pool_id);
}

/*********************************************************************
* sepProvisionDrive::Sep_Provision_Intializing_variable (private method)
*********************************************************************
*
*  Description:
*      Initialize the PVD variables.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
void sepProvisionDrive::Sep_Provision_Intializing_variable()
{
        disk_zero_checkpoint = 0; 
        disk_zero_percent = 0; 
        config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID; 
        sniff_verify_state = FBE_FALSE;
        state = 0; 
        port = 0;
        enclosure = 0;
        slot = 0;
        pool_id = 0; 
        in_checkpoint = 0;
        fbe_zero_memory(&phys_location,sizeof(phys_location));
        fbe_zero_memory(&pvd_verify_report,sizeof(pvd_verify_report)); 
        fbe_zero_memory(&disk_verify_status,sizeof(disk_verify_status));
        fbe_zero_memory(&provision_drive_info,sizeof(provision_drive_info)); 
        fbe_zero_memory(&get_priorities,sizeof(get_priorities)); 
        fbe_zero_memory(&update_pvd_config_type,sizeof(update_pvd_config_type));  
        fbe_zero_memory(&spare_drive_pool,sizeof(spare_drive_pool));  
        fbe_zero_memory(&update_pvd_sniff_verify_state,sizeof(update_pvd_sniff_verify_state));
        fbe_zero_memory(&update_pvd_pool_id,sizeof(update_pvd_pool_id));
}

/*********************************************************************
* sepProvisionDrive Class :: set_verify_checkpoint()
*********************************************************************
*
*  Description:
*      Sets the Verify Checkpoint
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepProvisionDrive::set_verify_checkpoint(int argc ,char ** argv)
{
    usage_format = 
        " Usage: %s [pvd object id] [Check point]\n"
        " For example: %s 3 100";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setVerifyCheckpoint",
        "sepProvisionDrive::set_verify_checkpoint",
        "fbe_api_provision_drive_set_verify_checkpoint",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " pvd object id 0x%x (%s)", pvd_object_id, *argv);

    // extract checkpoint
    argv++;
    in_checkpoint = (fbe_u64_t)_strtoui64(*argv, 0, 0);
    sprintf(params, " checkpoint 0x%llx (%s)", (unsigned long long)in_checkpoint, *argv);

    // Make api call: 
    status = fbe_api_provision_drive_set_verify_checkpoint(pvd_object_id,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           in_checkpoint);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to set VerifyCheckpoint to 0x%llx for "
                      "pvd object id 0x%X",
                      (unsigned long long)in_checkpoint, pvd_object_id);
    } else {
        sprintf(temp, "Verify checkpoint 0x%llx has been set for PVD object ID 0x%x",
                      (unsigned long long)in_checkpoint, pvd_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive Class :: copy_to_replacement_disk(
* int argc, char * * argv)
*********************************************************************
*
*  Description:
*      Copy to the replacement disk
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepProvisionDrive::copy_to_replacement_disk(
    int argc, char * * argv)
{
    usage_format = 
        " Usage: %s [src pvd object id] [dest pvd object id]\n"
        " For example: %s 0x104 0x105";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "copytoReplDisk",
        "sepProvisionDrive::copy_to_replacement_disk",
        "fbe_api_copy_to_replacement_disk",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert source Object id to hex
    argv++;
    src_obj_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    //  Convert destination Object id to hex
    argv++;
    dest_obj_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    sprintf(params, " src pvd object id 0x%x, dest pvd object id 0x%x ",
        src_obj_id, dest_obj_id);

    // Make api call:
    status = fbe_api_copy_to_replacement_disk(
        src_obj_id, dest_obj_id, &copy_status);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to copy to replacement disk with the copy"
            " status %s", edit_copy_status(copy_status));
    } else {
        sprintf(temp, "Copy to replacement disk successful with the"
            " copy status %s", edit_copy_status(copy_status));
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;

}

/*********************************************************************
* sepProvisionDrive Class :: copy_to_proactive_spare(
* int argc, char * * argv)
*********************************************************************
*
*  Description:
*      Copy to the proactive spare disk
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepProvisionDrive::copy_to_proactive_spare(
    int argc, char * * argv)
{
    usage_format = 
        " Usage: %s [src pvd object id] \n"
        " For example: %s 0x104 ";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "copytoProactiveSpare",
        "sepProvisionDrive::copy_to_proactive_spare",
        "fbe_api_provision_drive_user_copy",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert source Object id to hex
    argv++;
    src_obj_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    sprintf(params, " src pvd object id 0x%x", src_obj_id);
    
    // Make api call:
    status = fbe_api_provision_drive_user_copy(src_obj_id, &copy_status);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to copy to proactive spare disk with the"
            " copy status %s", edit_copy_status(copy_status));
    } else {
        sprintf(temp, "Copy to proactive spare disk successful with the"
            " copy status %s", edit_copy_status(copy_status));
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;

}
/*********************************************************************
* sepProvisionDrive Class :: edit_copy_status(
*  fbe_provision_drive_copy_to_status_t copy_status)
*********************************************************************
*
*  Description:
*      Edit the copy status
*
*  Input: Parameters
*      copy_status 
* 
*
*  Returns:
*      Status in the string form
*********************************************************************/
char* sepProvisionDrive :: edit_copy_status( fbe_provision_drive_copy_to_status_t copy_status)
{
    switch(copy_status){
        case FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR: 
           sprintf(cpy_status, "FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR");
            break;

        case FBE_PROVISION_DRIVE_COPY_TO_STATUS_INTER_OP_FAILURE:
            sprintf(cpy_status, "FBE_PROVISION_DRIVE_COPY_TO_STATUS_INTER_OP_FAILURE");
            break;

        case FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_UPSTREAM_RAID_GROUP:
            sprintf(cpy_status,  "FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_UPSTREAM_RAID_GROUP");
            break;

        case FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_INVALID_ID:
            sprintf(cpy_status, "FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_INVALID_ID");
            break;

        case FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_NOT_READY:
            sprintf(cpy_status, "FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_NOT_READY");
            break;

        case FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_REMOVED:
            sprintf(cpy_status, "FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_REMOVED");
            break;

        case FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_BUSY:
            sprintf (cpy_status, "FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_BUSY");
            break;

        case FBE_PROVISION_DRIVE_COPY_TO_STATUS_PERF_TIER_MISMATCH:
            sprintf(cpy_status, "FBE_PROVISION_DRIVE_COPY_TO_STATUS_PERF_TIER_MISMATCH");
            break;

        case FBE_PROVISION_DRIVE_COPY_TO_STATUS_BLOCK_SIZE_MISMATCH:
            sprintf(cpy_status, "FBE_PROVISION_DRIVE_COPY_TO_STATUS_BLOCK_SIZE_MISMATCH");
            break;

        case FBE_PROVISION_DRIVE_COPY_TO_STATUS_CAPACITY_MISMATCH:
            sprintf(cpy_status, " FBE_PROVISION_DRIVE_COPY_TO_STATUS_CAPACITY_MISMATCH");
            break;

        case FBE_PROVISION_DRIVE_COPY_TO_STATUS_SYS_DRIVE_MISMATCH:
            sprintf(cpy_status, "FBE_PROVISION_DRIVE_COPY_TO_STATUS_SYS_DRIVE_MISMATCH");
            break;

        default:
            sprintf(cpy_status, "FBE_PROVISION_DRIVE_COPY_TO_STATUS_UNKNOWN");
    }

    return cpy_status;
}

/*********************************************************************
* sepProvisionDrive Class ::  set_disk_zero_checkpoint (
*    int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Sets the disk zero Checkpoint
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepProvisionDrive::set_disk_zero_checkpoint (int argc ,
                                                          char ** argv)
{

    // Define specific usage string
    usage_format =
        " Usage: %s [pvd object id] [disk_zero_checkpoint]"\
        " For example: %s 0x100 1\n";
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "setDiskZeroChkpt",
        "sepProvisionDrive::set_disk_zero_checkpoint",
        "fbe_api_provision_drive_set_zero_checkpoint",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    //Get zero check point to set
    argv++;
    disk_zero_checkpoint = (fbe_lba_t)_strtoui64(*argv, 0, 0);

    sprintf(params, " pvd object id (0x%x), disk zero check point (%llu)",
        pvd_object_id, disk_zero_checkpoint);


    // Make api call: 
    status = fbe_api_provision_drive_set_zero_checkpoint(
                    pvd_object_id, disk_zero_checkpoint);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to set Disk zero checkpoint");
    } else {
        sprintf(temp, "Disk zero checkpoint set successfully");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive Class ::  packaged_clear_eol_state(
*    int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Clear the EOL (End Of Life) for PDO, LDO and PVD.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepProvisionDrive::packaged_clear_eol_state(int argc ,
    char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "packagedClearEol",
        "sepProvisionDrive::clear_eol_state",
        "fbe_api_provision_drive_clear_eol_state",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, "pvd object id 0x%x (%s)", pvd_object_id, *argv);

    // Get the location
    status = fbe_api_provision_drive_get_location(pvd_object_id,
        &port, &enclosure, &slot);
    if(status !=FBE_STATUS_OK){
        sprintf(temp, "Failed to get Drive location");
        call_post_processing(status, temp, key, params);
        return status;
    }

#if 0
    // Get the physical drive object id
    status = fbe_api_get_physical_drive_object_id_by_location(port,
        enclosure,
        slot, 
        &drive_object_id);
    if(status !=FBE_STATUS_OK){
        sprintf(temp, "Failed to get downstream physical drive "
            "object id of pvd 0x%x", pvd_object_id);
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Get the physical drive object id
    status = fbe_api_get_logical_drive_object_id_by_location(port,
        enclosure,  
        slot,
        &ldo_object_id);
    if(status !=FBE_STATUS_OK){
        sprintf(temp, "Failed to get downstream logical drive "
            "object id of pvd 0x%x", pvd_object_id);
        call_post_processing(status, temp, key, params);
        return status;
    }

    //send the EOL request to physical drive
    status = fbe_api_physical_drive_clear_eol(drive_object_id, 
        FBE_PACKET_FLAG_NO_ATTRIB);
    if(status != FBE_STATUS_OK){
        sprintf(temp, "Failed to clear EOL on downstream physical drive "
            "of pvd 0x%x", pvd_object_id);
        call_post_processing(status, temp, key, params);
        return status;
    }

    //send the EOL request to logical drive
    status = fbe_api_logical_drive_clear_pvd_eol(ldo_object_id);
    if(status != FBE_STATUS_OK){
        sprintf(temp, "Failed to clear EOL on downstream logical drive "
            "of pvd 0x%x", pvd_object_id);
        call_post_processing(status, temp, key, params);
        return status;
    }
#endif

    //Make the API call
    //send the EOL request to provisional drive
    status = fbe_api_provision_drive_clear_eol_state(pvd_object_id,
        FBE_PACKET_FLAG_NO_ATTRIB);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to clear eol on PVD 0x%x",pvd_object_id);
    } else {
        sprintf(temp, "EOL cleared successfully for pvd 0x%x", pvd_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;

}

/*********************************************************************
* sepProvisionDrive Class ::  clear_eol_state(
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

fbe_status_t sepProvisionDrive::clear_eol_state(int argc ,
    char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "ClearEol",
        "sepProvisionDrive::clear_eol_state_on_pvd",
        "fbe_api_provision_drive_clear_eol_state",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, "pvd object id 0x%x (%s)", pvd_object_id, *argv);

    //Make the API call
    //send the EOL request to provisional drive
    status = fbe_api_provision_drive_clear_eol_state(pvd_object_id,
        FBE_PACKET_FLAG_NO_ATTRIB);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to clear eol on PVD 0x%x",pvd_object_id);
    } else {
        sprintf(temp, "EOL cleared succesfully for pvd 0x%x", pvd_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive Class :: set_config_type()
*********************************************************************
*
*  Description:
*      Sets the config type
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepProvisionDrive::set_config_type (
    int argc , char ** argv)
{
    usage_format = 
        " Usage: %s [pvd obj id][config type: f|u|r|c] \n"
        " Config type: f - fails PVD\n"
        "              u - unconfigures spare\n"
        "              r - configure as a part of RG\n"
        "              c - configure as reserved spare"
        " For example: %s 0x100 c";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setConfigType",
        "sepProvisionDrive::set_config_type",
        "fbe_api_provision_drive_set_config_type",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    // increment argv for config type
    argv++;
    // Update config type
    if (is_valid_config_type(argv)) {
        sprintf(params, " pvd object id 0x%x (%s), config_type(%s))", 
            pvd_object_id, *argv, edit_config_type(config_type));

       update_pvd_config_type.pvd_object_id = pvd_object_id;
       update_pvd_config_type.config_type = config_type;

        // Make api call
       status = fbe_api_provision_drive_set_config_type(
                &update_pvd_config_type);

        // Check status of call
        if (status != FBE_STATUS_OK) {
            sprintf(temp, "Failed to set config type");
        } else {
            sprintf(temp, "Config Type set to %s", edit_config_type(config_type));
        }
    }
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive Class :: edit_config_type()
*********************************************************************
*
*  Description:
*      Edit config type
*
*  Input: Parameters
*      config_type
*
*  Returns:
*       config type in string format
*********************************************************************/

char* sepProvisionDrive:: edit_config_type(
    fbe_provision_drive_config_type_t c_type){

    switch(c_type){
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED:
            return "FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED";
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_FAILED:
            return "FBE_PROVISION_DRIVE_CONFIG_TYPE_FAILED";
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID:
            return "FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID";
        case  FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE:
            return " FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE";
        default:
            return "FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID";
    }

}

/*********************************************************************
* sepProvisionDrive Class :: map_lba_to_chunk (
*    int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Map a host lba on a pvd into a chunk index.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepProvisionDrive::map_lba_to_chunk(int argc ,
    char ** argv)
{
    // Define specific usage format
    usage_format = 
        " Usage: %s [pvd object id] [lba]\n"
        " For example: %s 0x105 0x1";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "mapLbaToChunk",
        "sepProvisionDrive::map_lba_to_chunk",
        "fbe_api_provision_drive_map_lba_to_chunk",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    pvd_object_id = (fbe_object_id_t) strtoul(*argv, 0, 0);

    argv++;

  if ((**argv == '0') && (*(*argv + 1) == 'x' || *(*argv+ 1) == 'X'))
    {
        status = string_to_hex64((fbe_u8_t* )*argv, &map_info.lba);
        sprintf(params, " pvd object id (0x%x), lba (0x%llx)",
        pvd_object_id,map_info.lba);
    }
    else{
        map_info.lba = (fbe_lba_t)_strtoui64(*argv, NULL, 10);
        sprintf(params, " pvd object id (0x%x), lba (%lld)",
        pvd_object_id,map_info.lba);
    }

     if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Error while convering string to hex value\n");
        call_post_processing(status, temp, key, params);
        return status;
     }

    sprintf(params,"pvd obj id (0x%x), lba (%llx)", pvd_object_id, map_info.lba);

    // Make api call: 
    status = fbe_api_provision_drive_map_lba_to_chunk(
        pvd_object_id, &map_info);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get spare drive pool");
    } else {
        sprintf(temp,"\n"
            " <chunk index>    0x%llx\n"
            " <metadata pba>   0x%llx",
            (unsigned long long)map_info.chunk_index,
            map_info.metadata_pba);

    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;

}

/*********************************************************************
* sepProvisionDrive Class :: get_paged_metadata(
*    int argc,
*    char ** argv)
*********************************************************************
*
*  Description:
*      Get the paged metadata of pvd.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepProvisionDrive::get_paged_metadata(
    int argc,
    char ** argv)
{
    char *data = NULL;
    // Define specific usage string  
    usage_format =
        " Usage: %s [pvd object id] [offset] [chunk count]\n"\
        " For example: %s  0x105 1 3\n";
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getPagedMetadata",
        "sepProvisionDrive::get_paged_metadata",
        "fbe_api_provision_drive_get_paged_metadata",
        usage_format,
        argc,9);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Extract raid group type
    argv++;
    pvd_object_id = (fbe_object_id_t)strtoul(*argv, 0, 0);

    argv++;
    chunk_offset = ( fbe_chunk_index_t )strtoul(*argv, 0, 0);

    argv++;
    status = string_to_hex64((fbe_u8_t*)*argv, &max_chunk_count);

    sprintf(params, " pvd object id (x%x), "
        "chunk_offset (0x%llx),"
        " chunk_count (0x%llx)",
        pvd_object_id,chunk_offset, max_chunk_count);

     if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Error while convering string to hex value\n");
        call_post_processing(status, temp, key, params);
        return status;
     }

    if (max_chunk_count > FBE_U32_MAX)
    {
         sprintf(temp, "Requested chunks count %llu is exceeding the "
         "maximum value %u", max_chunk_count, FBE_U32_MAX);
        call_post_processing(status, temp, key, params);
        return status;
    }

    paged_p = (fbe_provision_drive_paged_metadata_t*)fbe_api_allocate_memory(
        (fbe_u32_t)max_chunk_count * sizeof(fbe_provision_drive_paged_metadata_t));

    //Make the API call
    status = fbe_api_provision_drive_get_paged_metadata(pvd_object_id,
       chunk_offset, max_chunk_count, paged_p, FBE_FALSE /* Allow read from cache*/);
    if (status != FBE_STATUS_OK) 
    { 
        sprintf(temp,"Failed to get the paged metadata.");
        data = utilityClass::append_to (data, temp);
        fbe_api_free_memory(paged_p);
        // Output results from call or description of error
        call_post_processing(status, data, key, params);
        return status;
    }

     // For each chunk in this request, do display.
    sprintf(temp,  " \n<chunk offset>        0x%x", (unsigned int)chunk_offset);
    data = utilityClass::append_to (data, temp);
    for ( index = 0; index < max_chunk_count; index++)
    {
        current_paged_p = &paged_p[index];
        //get the data
        sprintf(temp,"\n"
            " <index>                    %d\n"
            " <valid bit>                %lld\n"
            " <need zero bit>            %lld\n"
            " <user zero bit>            %lld\n"
            " <consumed user data bit>   %lld\n"
            " <unused bit>               0x%llx\n",
            index, 
            current_paged_p->valid_bit,
            current_paged_p->need_zero_bit,
            current_paged_p->user_zero_bit,
            current_paged_p->consumed_user_data_bit,
            current_paged_p->unused_bit);
           data = utilityClass::append_to (data, temp);
    }

    //Free the memory
    fbe_api_free_memory(paged_p);

    //Output results from call or description of error
    call_post_processing(status, data, key, params);
    return status;
}

/*********************************************************************
* sepProvisionDrive Class ::  get_copyto_check_point(
*    int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Get the copy to checkpoint of PVD object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepProvisionDrive::get_copyto_check_point(int argc ,
    char ** argv)
{
    fbe_api_base_config_upstream_object_list_t upstream_object_list_p;
    fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;
    fbe_u32_t index = 0 ;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getCopytoCheckpoint",
        "sepProvisionDrive::get_copyto_check_point",
        "fbe_api_virtual_drive_get_info",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, "pvd object id 0x%x (%s)", pvd_object_id, *argv);
    // initialize the upstream object list. 
    upstream_object_list_p.number_of_upstream_objects = 0;
    for(index = 0; index < FBE_MAX_UPSTREAM_OBJECTS; index++)
    {
        upstream_object_list_p.upstream_object_list[index] = FBE_OBJECT_ID_INVALID;
    }

    //Make the API call
    status = fbe_api_base_config_get_upstream_object_list(pvd_object_id,
                                                          &upstream_object_list_p);
    for(index = 0 ; index < upstream_object_list_p.number_of_upstream_objects ; 
    index++)
    {
        fbe_api_get_object_class_id(upstream_object_list_p.upstream_object_list[index]
            ,&class_id, FBE_PACKAGE_ID_SEP_0); 
        if(class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
        {
            vd_object_id = upstream_object_list_p.upstream_object_list[index];
        }
    }
    if(vd_object_id == FBE_OBJECT_ID_INVALID)
    {
        sprintf(temp, "Failed to get copyto checkpoint for "\
              "pvd object id 0x%X",
              pvd_object_id);
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return(status);
    }

    // Make api call: 
    status = fbe_api_virtual_drive_get_info(
                vd_object_id, 
                &virtual_drive_info);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get copyto checkpoint for "\
                      "pvd object id 0x%X",
                      pvd_object_id);

    }else if (status == FBE_STATUS_OK){ 
        edit_get_copyto_checkpoint(&virtual_drive_info);
    }     
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}


/*********************************************************************
* sepProvisionDrive Class :: edit_get_copyto_checkpoint() 
                            (private method)
*********************************************************************
*
*  Description:
*      Display copy to checkpoint
*
*  Input: Parameters
*      virtual_drive_info_p - Structure pointer

*  Returns:
*      void
*********************************************************************/

void sepProvisionDrive::edit_get_copyto_checkpoint(
    fbe_api_virtual_drive_get_info_t *virtual_drive_info_p) 
{
    // unused drive as spare flag 
    sprintf(temp,                           "\n "\
        "<Copyto Checkpoint >      0x%llX\n ",
        virtual_drive_info_p->vd_checkpoint);
}

/*********************************************************************
* sepProvisionDrive Class :: set_slf()
*********************************************************************
*
*  Description:
*      Sets the single Loop failure.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepProvisionDrive::is_slf(  int argc , char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "isSlf",
        "sepProvisionDrive::is_slf",
        "fbe_api_provision_drive_is_slf",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Get PVD object ID and convert it to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    sprintf(params, "PVD object id (0x%x) ", pvd_object_id);

    // Make api call: 
    status = fbe_api_provision_drive_is_slf(pvd_object_id, &is_enable);

    // Check status of call
    if (status == FBE_STATUS_OK) {
        if(is_enable){
             sprintf(temp, "<SLF>        True");
        }
        else{
             sprintf(temp, "<SLF>        False");
        }
    } else {
        sprintf(temp, "Failed to get single loop failure status");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;

}

/*********************************************************************
* sepProvisionDrive Class :: clear_drive_fault_bit()
*********************************************************************
*
*  Description:
*      Clear drive fault bit
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepProvisionDrive::clear_drive_fault_bit(
    int argc , char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "clearDriveFaultFlag",
        "sepProvisionDrive::clear_drive_fault_bit",
        "fbe_api_provision_drive_clear_drive_fault",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " pvd object id 0x%x (%s)", pvd_object_id, *argv);

    // Make api call: 
    status = fbe_api_provision_drive_clear_drive_fault(pvd_object_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to clear drive fault flag.");
    } else {
        sprintf(temp, "Successfully cleared drive fault flag.");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;

}
/*********************************************************************
* sepProvisionDrive end of file
*********************************************************************/
