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
*      This file defines the methods of the SEP RAID Group INTERFACE class.
*
*  Notes:
*      The SEP RAID Group class (sepRaidGroup) is a derived class of 
*      the base class (bSEP).
*
*  History:
*      31-Mar-2011 : initial version
*
*********************************************************************/

#ifndef T_SEP_RAID_GROUP_CLASS_H
#include "sep_raid_group_class.h"
#endif

#include "sep_bind_class.h"
/*********************************************************************
* sepRaidGroup class :: Constructor
*********************************************************************
*  Description:
*      Initializes class variables
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/
sepRaidGroup::sepRaidGroup() : bSEP()
{
    // Store Object number
    idnumber = (unsigned) SEP_RAID_GROUP_INTERFACE;
    sepRaidGroupCount = ++gSepRaidGroupCount;

    // Store Object name
    idname = new char [64];
    strcpy(idname, "SEP_RAID_GROUP_INTERFACE");
    Sep_Raid_Intializing_variable();

    if (Debug) {
        sprintf(temp, 
                "sepRaidGroup class constructor (%d) %s\n", 
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP RAID Group Interface function calls>
    sepRaidGroupInterfaceFuncs = 
        "[function call]\n" \
        "[short name] [FBE API SEP RAID Group Interface]\n" \
        " -----------------------------------------------------------------\n" \
        " createRg                   fbe_api_create_rg\n" \
        " destroyRG                  fbe_api_job_service_raid_group_destroy\n" \
        " getRaidGroupInfo           fbe_api_raid_group_get_info\n" \
        " getRGPwrSavingPolicy       fbe_api_raid_group_get_power_saving_policy\n" \
        " ------------------------------------------------------------------\n" \
        " initRgVerify               fbe_api_raid_group_initiate_verify\n" \
        " getRebuildStatus           fbe_api_raid_group_get_rebuild_status\n" \
        " getBgVerifyStatus          fbe_api_raid_group_get_bgverify_status\n" \
        " getBgOpSpeed               fbe_api_raid_group_get_background_operation_speed\n"\
        " setBgOpSpeed               fbe_api_raid_group_set_background_operation_speed\n"\
        " ------------------------------------------------------------------\n" \
        " getPhysicalLba             fbe_api_raid_group_get_physical_from_logical_lba\n" \
        " getPagedMetadata           fbe_api_raid_group_get_paged_metadatan\n"\
        " mapLba                     fbe_api_raid_group_map_lba\n" \
        " getRGState                 fbe_api_get_object_lifecycle_state\n"\
        " ------------------------------------------------------------------\n"\
        " setMirrorPreferedPosition  fbe_api_raid_group_set_mirror_prefered_postition\n"\
        " ------------------------------------------------------------------\n"\
        " getWriteLogInfo            fbe_api_raid_group_get_write_log_info\n"\
        " ------------------------------------------------------------\n";

    // Define common usage for SEP RAID Group commands
    usage_format = 
        " Usage: %s [rg object id]\n"
        " For example: %s 1";
};

/*********************************************************************
 * sepRaidGriup class : MyIdNumIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id number of object
*  Input: Parameters
*      none
*  Returns:
*      unsigned int - id number
*********************************************************************/
unsigned sepRaidGroup::MyIdNumIs(void)
{
    return idnumber;
};

/*********************************************************************
*  sepRaidGroup class  :: MyIdNameIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id name of object
*  Input: Parameters
*      none
*  Returns:
*      char* - id name
*********************************************************************/
char * sepRaidGroup::MyIdNameIs(void)
{
    return idname;
};

/*********************************************************************
* sepRaidGroup class  :: dumpme()
*********************************************************************
*  Description:
*       Dumps id number and sep power saving count
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/
void sepRaidGroup::dumpme(void)
{ 
    strcpy (key, "sepRaidGroup::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, sepRaidGroupCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* sepRaidGroup Class :: select()
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
*      31-March-2011 : initial version
*
*********************************************************************/

fbe_status_t sepRaidGroup::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface"); 
    c = index;

    // List SEP RAID Group Interface calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepRaidGroupInterfaceFuncs );
        return status;
    }

    // create RAID Group
    if (strcmp (argv[index], "CREATERG") == 0) {
        status = create_raid_group (argc, &argv[index]);

        // destroy RAID Group
    } else if (strcmp (argv[index], "DESTROYRG") == 0) {
        status = destroy_raid_group(argc, &argv[index]);

        // get RAID Group info
    } else if (strcmp (argv[index], "GETRAIDGROUPINFO") == 0) {
        status = get_raid_group_info(argc, &argv[index]);

        // get RAID Group power saving policy
    } else if (strcmp (argv[index], "GETRGPWRSAVINGPOLICY") == 0) {
        status = get_raid_group_power_saving_policy(argc, &argv[index]);

        // initiate raid group verify
    } else if (strcmp (argv[index], "INITRGVERIFY") == 0) {
        status = initiate_rg_verify(argc, &argv[index]);

        // get RAID group's rebuild status
    } else if (strcmp (argv[index], "GETREBUILDSTATUS") == 0) {
        status = get_rebuild_status(argc, &argv[index]);

        // get RAID group's background verify status
    } else if (strcmp (argv[index], "GETBGVERIFYSTATUS") == 0) {
        status = get_bgverify_status(argc, &argv[index]);

        // get physical lba from logical lba
    } else if (strcmp (argv[index], "GETPHYSICALLBA") == 0) {
        status = get_physical_lba(argc, &argv[index]);

    //get paged metadata
    } else if (strcmp (argv[index], "GETPAGEDMETADATA") == 0) {
        status = get_paged_metadata(argc, &argv[index]);

    //Map LBA
    } else if (strcmp (argv[index], "MAPLBA") == 0) {
        status = get_lba_mapping(argc, &argv[index]);

    //Map PBA
    } else if (strcmp (argv[index], "MAPPBA") == 0) {
        status = get_pba_mapping(argc, &argv[index]);

    //get raid group state 
    } else if (strcmp (argv[index], "GETRGSTATE") == 0) {
        status = get_raid_group_state(argc, &argv[index]);

    //get raid group state 
    } else if (strcmp (argv[index], "SETMIRRORPREFEREDPOSITION") == 0) {
        status = set_mirror_prefered_position(argc, &argv[index]);

    //get write log info
    } else if (strcmp (argv[index], "GETWRITELOGINFO") == 0) {
        status = get_write_log_info(argc, &argv[index]);

    //get Background operation speed
    } else if (strcmp (argv[index], "GETBGOPSPEED") == 0) {
        status = get_background_operation_speed(argc, &argv[index]);

    //set Background operation speed
    } else if (strcmp (argv[index], "SETBGOPSPEED") == 0) {
        status = set_background_operation_speed(argc, &argv[index]);

        // can't find match on short name
    }else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepRaidGroupInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* sepRaidGroup class :: destroy_raid_group () 
*********************************************************************
*
*  Description:
*      Destroy the raidgriup.
*            
*  Input: Parameters
*      argc  - argument count 
*      *argv - pointer to arguments 
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*
*********************************************************************/
fbe_status_t sepRaidGroup::destroy_raid_group(int argc , char ** argv)
{
    fbe_object_id_t  raid_group_object_id;
    fbe_job_service_error_type_t job_error_type;
    fbe_status_t job_status;

    // Define common usage for SEP RAID Group commands
    usage_format = 
        " Usage: %s [RG id]\n"
        " For example: %s 1";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "destroyRG",
        "sepRaidGroup::destroy_raid_group",
        "fbe_api_job_service_raid_group_destroy",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    raid_group_num = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " RG id  %u (%s)", raid_group_num, *argv);

    in_fbe_raid_group_destroy_req.raid_group_id = raid_group_num;
    in_fbe_raid_group_destroy_req.wait_destroy = FBE_FALSE;
    in_fbe_raid_group_destroy_req.destroy_timeout_msec = RG_WAIT_TIMEOUT;
    //Get the RG number
    status = fbe_api_database_lookup_raid_group_by_number( 
        raid_group_num, &raid_group_object_id);
    if(status != FBE_STATUS_OK){
        sprintf(temp, "Can't lookup the  RG %d",
            raid_group_num);
         call_post_processing(status, temp, key, params); 
         return status;
    }

    // Make api call: 
    status = fbe_api_job_service_raid_group_destroy(
        &in_fbe_raid_group_destroy_req);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to destroy RG %d (0x%X)",
                raid_group_num, raid_group_object_id);
    } else {

            // Wait to complete the job
            status = fbe_api_common_wait_for_job(
                in_fbe_raid_group_destroy_req.job_number, 
                RG_WAIT_TIMEOUT, &job_error_type, &job_status, NULL);
            if (status != FBE_STATUS_OK || job_status != FBE_STATUS_OK) 
            {
                sprintf(temp, "Wait for RG job failed. \n");

                // Output results from call or description of error
                call_post_processing(status, temp, key, params); 
                return status;
            }

            // Check for LUNs in RG
            if(job_error_type == FBE_JOB_SERVICE_ERROR_REQUEST_OBJECT_HAS_UPSTREAM_EDGES)
            {
                sprintf(temp,"Can't destroy RG %d(0x%X). it has upstream objects in it.\n",
                    in_fbe_raid_group_destroy_req.raid_group_id, 
                    in_fbe_raid_group_destroy_req.raid_group_id);

                // Output results from call or description of error
                call_post_processing(FBE_STATUS_GENERIC_FAILURE, temp, key, params); 
                return FBE_STATUS_GENERIC_FAILURE;
            }

            sprintf(temp, "Successfully destroyed RG %d (0x%X)",
            raid_group_num,raid_group_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params); 
    return status;
}

/*********************************************************************
* sepRaidGroup class :: get_raid_group_info () 
*********************************************************************
*
*  Description:
*      Get the raid group information.
*            
*  Input: Parameters
*      argc  - argument count 
*      *argv - pointer to arguments 
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*
*********************************************************************/
fbe_status_t sepRaidGroup::get_raid_group_info(int argc , char ** argv)
{
    fbe_raid_group_number_t  raid_group_id;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_u32_t index = 0;
    fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getRaidGroupInfo",
        "sepRaidGroup::get_raid_group_info",
        "fbe_api_raid_group_get_info",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    rg_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " Object id 0x%X (%s)", rg_object_id, *argv);
    fbe_api_get_object_class_id(rg_object_id,&class_id, FBE_PACKAGE_ID_SEP_0);
    if((class_id <= FBE_CLASS_ID_RAID_FIRST || class_id >= 
    FBE_CLASS_ID_RAID_LAST) && (class_id != FBE_CLASS_ID_VIRTUAL_DRIVE)){
        sprintf(temp, "Invalid Raid group object id 0x%x",
            rg_object_id);
        call_post_processing(status, temp, key, params); 
        return FBE_STATUS_FAILED;
    }
    else if(class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        status = fbe_api_base_config_get_upstream_object_list(
                rg_object_id, 
                &upstream_lun_object_list);

        rg_object_id = upstream_lun_object_list.upstream_object_list[0];

        if(status != FBE_STATUS_OK){
            sprintf(temp, "Failed to get the Raid group object list for "\
                "RG  (0x%X)",
                 rg_object_id);
            return status;
        }
    }
    // Make api call: 
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info[0],
             FBE_PACKET_FLAG_NO_ATTRIB);

    if(status != FBE_STATUS_OK){
        sprintf(temp, "Failed while getting the RGinfo for RG object id 0x%x",
        rg_object_id);
         call_post_processing(status, temp, key, params); 
        return status;
    }

    //For Raid R10 we have intermidiate mirror rg object
    if (raid_group_info[0].raid_type == FBE_RAID_GROUP_TYPE_RAID10){
        
        downstream_object_list.number_of_downstream_objects = 0;
        for(index = 0; index < FBE_MAX_DOWNSTREAM_OBJECTS; index++)
        {
            downstream_object_list.downstream_object_list[index] = FBE_OBJECT_ID_INVALID;
        }
        // Get Mirror rg associated with this RG
        status = fbe_api_base_config_get_downstream_object_list(
                 rg_object_id, &downstream_object_list);
        if (status != FBE_STATUS_OK)
        {
            sprintf(temp, "Failed while getting the RGinfo for RG object id 0x%x",rg_object_id);
            call_post_processing(status, temp, key, params); 
            return status;
        }

        for(index = 0; index < downstream_object_list.number_of_downstream_objects; index++)
        {
              status = fbe_api_raid_group_get_info(
                  downstream_object_list.downstream_object_list[index], 
                  &raid_group_info[index+1],//as raid_group_info[o] already filled 
                                         // by information of Raid10 object
                  FBE_PACKET_FLAG_NO_ATTRIB);
              if(status != FBE_STATUS_OK){
                 sprintf(temp, "Can't get the RGinfo 0x%x",
                    downstream_object_list.downstream_object_list[index]);
                 call_post_processing(status, temp, key, params); 
                 return status;
              }
        }
    }

    //Get the RG number
    status = fbe_api_database_lookup_raid_group_by_object_id(rg_object_id, &raid_group_id);
    if(status != FBE_STATUS_OK){
        sprintf(temp, "Can't lookup the  RG 0x%x",
            raid_group_id);
         call_post_processing(status, temp, key, params); 
         return status;
    }
    
    char * data;

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get information for RG %d (0x%X)",
                raid_group_id, rg_object_id);
        data = temp;
    } else {
        // Make a call to get the drives associated with this RG
        status = get_drives_by_rg (rg_object_id, &rg_drives);

    }
	
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get information about drives for "\
                "RG %d (0x%X)",
                raid_group_id, rg_object_id);
        data = temp;
    } else {
        
        // Get the upstream Lun object list 
        status = fbe_api_base_config_get_upstream_object_list(
            rg_object_id, 
            &upstream_object_list);
        if(status != FBE_STATUS_OK) {
            sprintf(temp, "Failed to get the LUN object list for "\
                "RG %d (0x%X)",
                raid_group_id, rg_object_id);
                return status;
        }

    if (upstream_object_list.number_of_upstream_objects > 0) {
            fbe_class_id_t class_id;
            status = fbe_api_get_object_class_id(
                upstream_object_list.upstream_object_list[0], 
                &class_id, 
                FBE_PACKAGE_ID_SEP_0);

            // Check class id for for special case of raid1_0
            if(class_id != FBE_CLASS_ID_LUN){
                // Get the upstream Lun object list for raid1_0
                status = fbe_api_base_config_get_upstream_object_list(
                    upstream_object_list.upstream_object_list[0], 
                    &upstream_lun_object_list);
                if(status != FBE_STATUS_OK){
                    sprintf(temp, "Failed to get the LUN object list for "\
                        "RG %d (0x%X)",
                        raid_group_id, rg_object_id);
                        return status;
                }
        }
        else{
            upstream_lun_object_list = upstream_object_list;
        }
        
        // Allocate memory for LUN list 
        raid_group_luns = 
            new fbe_u32_t[upstream_lun_object_list.number_of_upstream_objects];
        if (!raid_group_luns){
            sprintf(temp, "Failed to allocate memory for LUNs of "\
                    "RG %d (0x%X)",
                    raid_group_id, rg_object_id);
            return status;
        }
        
        lun_count = 0;

        for (fbe_u32_t lun_index= 0; 
            lun_index < upstream_lun_object_list.number_of_upstream_objects; 
            lun_index++){
            
            fbe_object_id_t lun_obj_id = 
                upstream_lun_object_list.upstream_object_list[lun_index];
            
            // get the LUN id from object ID 
            status = fbe_api_database_lookup_lun_by_object_id(
                lun_obj_id, 
                &raid_group_luns[lun_index]);
            
            // Check status of call
            if (status != FBE_STATUS_OK) {
                sprintf(temp, "Can't get LUN ID for Object ID %d", 
                    lun_obj_id);
                return status;
            }

            ++lun_count;
        }
 }			

        //edit_raid_group_info(&raid_group_info);
        data = edit_raid_group_info();
    }

    // Output results from call or description of error
    call_post_processing(status, data, key, params);

    return status;
}
/*********************************************************************
* sepRaidGroup class :: create_raid_group () 
*********************************************************************
*
*  Description:
*      Create a raidgroup.
*            
*  Input: Parameters
*      argc  - argument count 
*      *argv - pointer to arguments 
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*
*********************************************************************/


fbe_status_t sepRaidGroup::create_raid_group (int argc, char **argv)
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [type: ] [disk: [b_e_s]...] \n"\
        " [Optional: [rg id] [capacity: value|d] [ps idle time: value|d] \n"\
        " d = default value \n"\
        " For example: %s r5 0_0_5 0_0_6 0_0_7 9 5000 1800 \n";
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "createRg",
        "sepRaidGroup::create_raid_group",
        "fbe_api_create_rg",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Extract raid group type
    argv++;
    fbe_u8_t *rg_type = NULL;
    rg_type = (fbe_u8_t *) *argv;
    sprintf(params, " rg type (%s)", rg_type);

    // Check validity of Raid Type
    raid_type = check_valid_raid_type(rg_type);
    if(raid_type == FBE_RAID_GROUP_TYPE_UNKNOWN)
    {
        sprintf(temp, "Raid type %s given for "\
                      "RG creation is invalid. "\
                      "Valid raid types are r0,r1,r3,r5,r6,r1_0\n",
                      rg_type);

        call_post_processing(status, temp, key, params);
        return status;
    }

    // extract disk info
    argv++;
    sprintf(params_temp, " disks (");
    strcat(params, params_temp);
    
    for(drive_count = 0; 
        (!((*argv) == NULL)) && (!(strchr(*argv, '_') == NULL)); drive_count++){

        sprintf(params_temp, "%s ", *argv);
        if(strlen(params_temp) > MAX_PARAMS_SIZE)
        {
            sprintf(temp, "<ERROR!> Params length is larger that buffer value");
            status = FBE_STATUS_GENERIC_FAILURE;
            call_post_processing(status, temp, key, params);
            return status;
        }
        strcat(params, params_temp);

        status = utilityClass::convert_diskname_to_bed(
                (fbe_u8_t*) argv[0],
                &rg_request.disk_array[drive_count],
                temp
                );

        if(status != FBE_STATUS_OK){
            // temp is being populated in convert_diskname_to_bed
            call_post_processing(status, temp, key, params);
            return status;
        }

        argv++;
    }

    sprintf(params_temp, ") ");
    if(strlen(params_temp) > MAX_PARAMS_SIZE)
    {
        sprintf(temp, "<ERROR!> Params length is larger that buffer value");
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }
    strcat(params, params_temp);

    // Check validity of disk mentioned for given RG type
    status = check_disks_for_raid_type(drive_count, raid_type);
    if(status != FBE_STATUS_OK){

        if(status == FBE_STATUS_INSUFFICIENT_RESOURCES){
            sprintf(temp, "Drives provided for RG type %s "\
                          "are insufficient.\n", rg_type);
        }

        if(status == FBE_STATUS_INVALID){
            sprintf(temp, "Raid type %s given for RG creation is invalid."\
                    "Valid raid types are r0,r1,r3,r5,r6,hi5,r1_0,id",
                    rg_type);
        }

        call_post_processing(status, temp, key, params);
        return status;
    }

    // extract optional arguments
    //argv++;
    status = extract_optional_arguments(argv);
    if (FBE_STATUS_OK != status){
        // appropriate error messege is set in called function
        call_post_processing(status, temp, key, params);
        return status;
    }

    // If Raid group id is not given, set it to
    // current smallest possible number, else
    // validate the given raid group id
    status = assign_validate_raid_group_id(&raid_group_id, &rg_obj_id);
    if (FBE_STATUS_OK != status){
        // appropriate error messege is set in called function
        call_post_processing(status, temp, key, params);
        return status;
    }

    // create raid group request
    rg_request.b_bandwidth = FBE_TRUE;
    rg_request.capacity = capacity;
    rg_request.power_saving_idle_time_in_seconds = ps_idle_time;
    rg_request.max_raid_latency_time_is_sec = 120;
    rg_request.raid_type = raid_type;
    rg_request.raid_group_id = raid_group_id;
    //rg_request.explicit_removal = 0;
    rg_request.is_private = (fbe_job_service_tri_state_t) 0;
    rg_request.drive_count = drive_count;
    rg_request.user_private = FBE_FALSE;

    // Make API call to create raid group
    fbe_job_service_error_type_t job_error_type;
    status = fbe_api_create_rg(
            &rg_request, FBE_TRUE, RG_WAIT_TIMEOUT, &rg_obj_id, &job_error_type);
    if (status != FBE_STATUS_OK){
        sprintf(temp, "RG creation failed.");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepRaidGroup class :: check_disks_for_raid_type (private method)
*********************************************************************
*
*  Description:
*      Checks for minimum required disks for any raid group.
*            
*  Input: Parameters
*      drive_count 
*      raid_type
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*
*********************************************************************/


fbe_status_t sepRaidGroup::check_disks_for_raid_type(
    fbe_object_id_t drive_count, 
    fbe_raid_group_type_t raid_type)
{

    /*
    Raid 0    - 3 -16
    Raid 1    - 2
    Raid 1/0  - Even (multiple of 2)
    Raid 3    - 5 or 9
    Raid 5    - 3-16
    Raid 6    - 4,6,8,10,12,14,16
    Raid Disk - 1
    Raid HS   - 1
    */

    if (raid_type == FBE_RAID_GROUP_TYPE_RAID0){
        if ( (drive_count == RAID_INDIVIDUAL_DISK_WIDTH) ||
             ((drive_count > RAID_STRIPED_MIN_DISKS) && 
              (drive_count <= RAID_STRIPED_MAX_DISK)) ){
                  return FBE_STATUS_OK;
        }
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    // RAID-1 (mirrored disks) 
    else if (raid_type == FBE_RAID_GROUP_TYPE_RAID1){
        if(drive_count == RAID_MIRROR_MIN_DISKS){
            return FBE_STATUS_OK;
        }
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    // RAID-3
    else if (raid_type == FBE_RAID_GROUP_TYPE_RAID3){
        if((drive_count != RAID3_STRIPED_MIN_DISK) || 
            (drive_count != RAID3_STRIPED_MAX_DISK)){
            return FBE_STATUS_OK;
        }
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    // RAID-5
    else if (raid_type == FBE_RAID_GROUP_TYPE_RAID5){
        if((drive_count > RAID_STRIPED_MIN_DISKS) && 
            (drive_count <= RAID_STRIPED_MAX_DISK)){
            return FBE_STATUS_OK;
        }
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    // RAID-6
    else if (raid_type == FBE_RAID_GROUP_TYPE_RAID6){
        if((drive_count > RAID6_STRIPED_MIN_DISK) && 
            (drive_count <= RAID_STRIPED_MAX_DISK) && 
            (drive_count % 2 == 0)){
                return FBE_STATUS_OK;
        }
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    // RAID-1/0
    else if (raid_type == FBE_RAID_GROUP_TYPE_RAID10){
        if((drive_count > RAID10_MIR_STRIPED_MIN_DISK) && 
            (drive_count <= RAID_STRIPED_MAX_DISK) && 
            (drive_count % 2 == 0)){
                return FBE_STATUS_OK;
        }
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    // hot spare
    else if (raid_type == FBE_RAID_GROUP_TYPE_SPARE){
        if(drive_count == HOT_SPARE_NUM_DISK){
            return FBE_STATUS_OK;
        }
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    // unknown RAID type
    else{
        return FBE_STATUS_INVALID;
    }
}

/*********************************************************************
* sepRaidGroup class :: check_valid_raid_type (private method)
*********************************************************************
*
*  Description:
*      Check the valid raid type.
*            
*  Input: Parameters
*      argc  - argument count 
*      *argv - pointer to arguments 
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*
*********************************************************************/

fbe_raid_group_type_t sepRaidGroup::check_valid_raid_type(
                                    fbe_u8_t *rg_type)
{
    if (rg_type == NULL){
        return FBE_RAID_GROUP_TYPE_UNKNOWN;
    }
    
    // RAID-0
    if (strcmp((const char*)rg_type, "R0") == 0){
        return FBE_RAID_GROUP_TYPE_RAID0;
    }
    // RAID-0 
    if (strcmp((const char*)rg_type, "ID") == 0){
        return FBE_RAID_GROUP_TYPE_RAID0;
    }
    // RAID-1 (mirrored disks) 
    else if (strcmp((const char*)rg_type, "R1") == 0){
        return FBE_RAID_GROUP_TYPE_RAID1;
    }
    // RAID-3
    else if (strcmp((const char*)rg_type, "R3") == 0){
        return FBE_RAID_GROUP_TYPE_RAID3;
    }
    // RAID-5
    else if (strcmp((const char*)rg_type, "R5") == 0){
        return FBE_RAID_GROUP_TYPE_RAID5;
    }
    // RAID-6
    else if (strcmp((const char*)rg_type, "R6") == 0){
        return FBE_RAID_GROUP_TYPE_RAID6;
    }
    // RAID-1/0
    else if (strcmp((const char*)rg_type, "R1_0") == 0){
        return FBE_RAID_GROUP_TYPE_RAID10;
    }
    // RAID-Hotspare
    else if (strcmp((const char*)rg_type, "HS") == 0){
        return FBE_RAID_GROUP_TYPE_SPARE;
    }
    // unknown RAID type
    else{
        return FBE_RAID_GROUP_TYPE_UNKNOWN;
    }
}

/*********************************************************************
* sepRaidGroup class :: find_smallest_free_raid_group_number 
*                       (private method)
*********************************************************************
*
*  Description:
*      print raid group information.
*
*  Input: Parameters
*           raid_group_id - Raid group number
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepRaidGroup::find_smallest_free_raid_group_number(
                           fbe_raid_group_number_t *raid_group_id)
{
    fbe_u32_t i = 0;
    fbe_object_id_t object_id;

    if (raid_group_id == NULL){
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    for(i = 0; i <= MAX_RAID_GROUP_ID; i++){
        
        if( fbe_api_database_lookup_raid_group_by_number(
            i, &object_id) == FBE_STATUS_OK){
            continue;
        }
        else{
            *raid_group_id = i;
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_INSUFFICIENT_RESOURCES;
}

/*********************************************************************
* sepRaidGroup class :: extract_optional_arguments (private method)
*********************************************************************
*
*  Description:
*      This method will extract the optional arguments to 
*      createRg command and store them into class variables.
*            
*  Input: Parameters
*      argc  - argument count 
*      *argv - pointer to arguments 
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*
*********************************************************************/
fbe_status_t sepRaidGroup::extract_optional_arguments(char **argv)
{
    // check if optional arguments are entered 
    // if argv is d then default value is taken.

    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    // extract rg id
    if (strcmp(*argv, "D")){
        raid_group_id = (fbe_raid_group_number_t)strtoul(*argv, 0, 0);//atoi(*argv);
        sprintf(params_temp, " rg id (0x%x), ", raid_group_id);
        if(strlen(params_temp) > MAX_PARAMS_SIZE)
        {
            sprintf(temp, "<ERROR!> Params length is larger that buffer value");
            status = FBE_STATUS_GENERIC_FAILURE;
            call_post_processing(status, temp, key, params);
            return status;
        }
        strcat(params, params_temp);    
    }

    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    // extract capacity
    if (strcmp(*argv, "D")){
        // to do - - put convert_str_to_lba() in the base class
        sepBind sb;
        capacity = sb.convert_str_to_lba((fbe_u8_t *)*argv);

        sprintf(params_temp, " lun capacity (0x%llx), ", capacity);
        if(strlen(params_temp) > MAX_PARAMS_SIZE)
        {
            sprintf(temp, "<ERROR!> Params length is larger that buffer value");
            status = FBE_STATUS_GENERIC_FAILURE;
            call_post_processing(status, temp, key, params);
            return status;
        }
        strcat(params, params_temp);
    }

    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }
    
    // extract ps idle time
    if (strcmp(*argv, "D")){
        ps_idle_time = _strtoui64(*argv, NULL, 10);
        sprintf(params_temp, "ps idle time (0x%llx), ",
		(unsigned long long)ps_idle_time);
        if(strlen(params_temp) > MAX_PARAMS_SIZE)
        {
            sprintf(temp, "<ERROR!> Params length is larger that buffer value");
            status = FBE_STATUS_GENERIC_FAILURE;
            call_post_processing(status, temp, key, params);
            return status;
        }
        strcat(params, params_temp);
    }

    return FBE_STATUS_OK;
}

/*********************************************************************
* sepRaidGroup class :: assign_validate_raid_group_id (private method)
*********************************************************************
*
*  Description:
*      Validate the raid group id.
*            
*  Input: Parameters
*      argc  - argument count 
*      *argv - pointer to arguments 
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*
*********************************************************************/

fbe_status_t sepRaidGroup::assign_validate_raid_group_id (
    fbe_raid_group_number_t *raid_group_id,
    fbe_object_id_t *rg_obj_id) 
{

    fbe_status_t status = FBE_STATUS_OK;

    if(*raid_group_id == FBE_OBJECT_ID_INVALID){

        // Set the rg number to current smallest possible number
        status =  find_smallest_free_raid_group_number(&(*raid_group_id));

        if (status == FBE_STATUS_INSUFFICIENT_RESOURCES){
            sprintf(temp, "Maximum number of RG (%d) exists. "\
                          "RG creation is not allowed.\n",
                          MAX_RAID_GROUP_ID);
            return status;
        }

        sprintf(temp, "RG number is not mentioned. "\
                      "Creating RG having number %d.\n", *raid_group_id);

    } else {

        // Check if RG number is valid
        if(*raid_group_id > MAX_RAID_GROUP_ID){
            sprintf(temp, "RG number %d is not supported.\n",
                    *raid_group_id);
            status = FBE_STATUS_GENERIC_FAILURE;
            return status;
        }

        // Check if RG already exists
        status = fbe_api_database_lookup_raid_group_by_number(
                 *raid_group_id, rg_obj_id);

        if (status == FBE_STATUS_OK){
            sprintf(temp, "RG %d already exists.\n", *raid_group_id);
            status = FBE_STATUS_GENERIC_FAILURE;
        } else {
            sprintf (temp, "Creating RG %d.\n", *raid_group_id);
            status = FBE_STATUS_OK;
        }
    }

    return status;
}

/*********************************************************************
* sepRaidGroup Class :: get_raid_group_power_saving_policy()
*********************************************************************
*
*  Description:
*      Get raid group power saving policy
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepRaidGroup::get_raid_group_power_saving_policy(
    int argc,
    char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getRGPwrSavingPolicy",
        "sepRaidGroup::get_raid_group_power_saving_policy",
        "fbe_api_raid_group_get_power_saving_policy",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    raid_group_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " raid group id 0x%x (%s)", raid_group_id, *argv);

    // Make api call: 
    status = fbe_api_raid_group_get_power_saving_policy(
        raid_group_id,
        &rg_ps_policy);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get power saving policy for "
                      "raid group obj id 0x%X",
                      raid_group_id);
    } else {
        edit_raid_group_power_saving_policy(&rg_ps_policy);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    
    return status;
}

/*********************************************************************
* sepRaidGroup Class :: initiate_rg_verify()
*********************************************************************
*
*  Description:
*      Initiate raid group verify
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepRaidGroup::initiate_rg_verify(int argc , char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [object id] [verify type: 1|2|3]\n"
        " Verify type: 1: user read/write  2: user read only  3: error verify\n"
        " For example: %s 4 1";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "initRgVerify",
        "sepRaidGroup::initiate_rg_verify",
        "fbe_api_raid_group_initiate_verify",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    rg_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    // Convert the rg verify type
    argv++;
    lu_verify_type = (enum fbe_lun_verify_type_e)strtoul(*argv, 0, 0);

    // Copy input parameters
    argv--;
    sprintf(params, " rg object id 0x%x lun_verify_type (%d)\n",
            rg_object_id, lu_verify_type);

    // Make api call:
    status = fbe_api_raid_group_initiate_verify(
        rg_object_id,
        FBE_PACKET_FLAG_NO_ATTRIB, 
        lu_verify_type);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't initiate raid group verify for "
                      " rg object id 0x%X",
                      rg_object_id);
    } else {
        sprintf(temp, "Initiated raid group verify on rg object id 0x%x",
                rg_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepRaidGroup Class :: get_rebuild_status()
*********************************************************************
*
*  Description:
*      Gets the rebuild status (percentage and checkpoint ) 
*      for a given Raid Group
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepRaidGroup::get_rebuild_status(int argc , char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getRebuildStatus",
        "sepRaidGroup::get_rebuild_status",
        "fbe_api_raid_group_get_rebuild_status",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    rg_obj_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " rg object id 0x%x (%s)", rg_obj_id, *argv);

    // Make api call:
    status = fbe_api_raid_group_get_rebuild_status(
             rg_obj_id, &rebuild_status);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to obtain rebuild status for "
                      "rg object id 0x%x",
                      rg_obj_id);
    } else {
        // format rebuild status information
        sprintf(temp,              "\n "
            "<RG Rebuild Chkpt>         0x%llX\n "
            "<RG Rebuild Pct>           %d\n ",
            (unsigned long long)rebuild_status.checkpoint,
            rebuild_status.percentage_completed);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepRaidGroup Class :: get_bgverify_status()
*********************************************************************
*
*  Description:
*      Gets the Background Verify status (percentage and checkpoint ) 
*      for a given Raid Group
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepRaidGroup::get_bgverify_status(int argc , char ** argv)
{

    // Define specific usage string
    usage_format =
        " Usage: %s [object id] [verify type: 1|2|3|4|5]\n"
        " Verify type: 1: user read/write  2: user read only  3: error verify \n"\
        " 4: incomplete write verify  5: system verify \n"
        " For example: %s 0x10e 1";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getBgVerifyStatus",
        "sepRaidGroup::get_bgverify_status",
        "fbe_api_raid_group_get_bgverify_status",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    rg_obj_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    // Get verify type
    argv++;
    rg_verify_type = (enum fbe_lun_verify_type_e)strtoul(*argv, 0, 0);

    // Copy input parameters
    argv--;
    sprintf(params, " rg object id 0x%x (%s) , background verify type (%d)",
            rg_obj_id, *argv, rg_verify_type);

    // Make api call:
    status = fbe_api_raid_group_get_bgverify_status (
             rg_obj_id, &bg_verify_status, rg_verify_type);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to obtain background verify status for "
                      "rg object id 0x%x",
                      rg_obj_id);
    } else {
        // format BGV status information
        sprintf(temp,              "\n "
            "<Background Verify Chkpt>         0x%llX\n "
            "<Background Verify Pct>           %d\n ",
            (unsigned long long)bg_verify_status.checkpoint,
            bg_verify_status.percentage_completed);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepRaidGroup class :: get_physical_lba () 
*********************************************************************
*
*  Description:
*      Get the physical LBA from logical LBA
*            
*  Input: Parameters
*      argc  - argument count 
*      *argv - pointer to arguments 
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*
*********************************************************************/

fbe_status_t sepRaidGroup::get_physical_lba (int argc, char **argv)
{
    // Usage format
    usage_format =
        " Usage: %s [rg object id] [logical lba]\n"
        " For example: %s 0x154 0x2000 ";
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getPhysicalLba",
        "sepRaidGroup::get_physical_lba",
        "fbe_api_raid_group_get_physical_from_logical_lba",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Extract arguments
    argv++;
    rg_obj_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    
    argv++;
    status = string_to_hex64((fbe_u8_t* )*argv, &logical_lba);
     if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Error while convering string to hex value\n");
        return status;
     }

    sprintf(params, " rg object id %d, logical lba 0x%llx",
            rg_obj_id, (unsigned long long)logical_lba);

    // Make API call to create raid group
    status = fbe_api_raid_group_get_physical_from_logical_lba(
        rg_obj_id,
        logical_lba, 
        &physical_lba);
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't get physical lba");
    }
    else{
        sprintf(temp,       "\n "
        "<Physical Lba>     0x%llX\n ", (unsigned long long)physical_lba);    
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepRaidGroup class :: edit_raid_group_info (private method)
*********************************************************************
*
*  Description:
*      Edit the Raid group information
*            
*  Input: Parameters
*      argc  - argument count 
*      *argv - pointer to arguments 
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*
*********************************************************************/

char * sepRaidGroup::edit_raid_group_info() 
{

    fbe_u32_t rg_width;
    fbe_u32_t index = 0, loop_index = 0, drive_index = 0;
    
    char *data = NULL;
    char *mytemp = NULL;

    //calculating rg width for raid1_0
    if (raid_group_info[0].raid_type == FBE_RAID_GROUP_TYPE_RAID10){
        rg_width = raid_group_info[0].width*2;
    } else {
        rg_width = raid_group_info[0].width;
    }

    sprintf(temp,                       "\n"
            "<RG object ID>             0x%X\n"
            "<RG Type>                  %s\n"
            "<RG Width>                 %d\n"
            "<RG State>                 %s\n"
            "<Overall Capacity>         0x%llX\n"
            "<Element Size>             0x%X\n"
            "<Exported Block Size>      0x%X\n"
            "<Imported Block Size>      0x%X\n"
            "<Optimal Block Size>       0x%X\n"
            "<Stripe Count>             0x%llX\n"
            "<Sectors per stripe>       0x%llX\n"
            "<LUN Align Size>           0x%llX\n"
            "<RO verify checkpoint>     0x%llX\n"
            "<RW verify checkpoint>     0x%llX\n"
            "<Error verify checkpoint>  0x%llX\n"           
            "<Paged Metadata Start LBA> 0x%llX\n"           
            "<Paged Metadata Capacity>  0x%llX\n"           
            "<Journal log start PBA>    0x%llX\n"           
            "<Journal log Physical Capacity> 0x%llX\n"
            "<Metadata Element State>    %s\n"
            "<Metadata Verify Pass Count> 0x%X\n",
            rg_object_id,
            utilityClass::convert_rg_type_to_string(raid_group_info[0].raid_type),
            rg_width,
            raid_group_state(rg_object_id),
            raid_group_info[0].capacity,
            raid_group_info[0].element_size,
            raid_group_info[0].exported_block_size,
            raid_group_info[0].imported_block_size,
            raid_group_info[0].optimal_block_size,
            raid_group_info[0].stripe_count,
            raid_group_info[0].sectors_per_stripe,
            raid_group_info[0].lun_align_size,
            raid_group_info[0].ro_verify_checkpoint,
            raid_group_info[0].rw_verify_checkpoint,
            raid_group_info[0].error_verify_checkpoint,            
            raid_group_info[0].paged_metadata_start_lba,
            raid_group_info[0].paged_metadata_capacity,
            raid_group_info[0].write_log_start_pba,
            raid_group_info[0].write_log_physical_capacity,
            utilityClass::convert_object_state_to_string(raid_group_info[0].metadata_element_state),
            raid_group_info[0].paged_metadata_verify_pass_count
            );

    data = utilityClass::append_to (data, temp);

    sprintf (temp, "<Drives associated with this RG> ");

    data = utilityClass::append_to (data, temp);

    for(index = 0; index < rg_width; index++)
    {
        mytemp = new char[50];
        sprintf (mytemp, 
                "%d_%d_%d ",
                rg_drives.drive_list[index].port_num,
                rg_drives.drive_list[index].encl_num,
                rg_drives.drive_list[index].slot_num
                );

        data = utilityClass::append_to(data, mytemp);

        delete [] mytemp;
    }

    for(index = 0; index < rg_width; index++)
    {
        mytemp = new char[100];
        if (raid_group_info[0].raid_type == FBE_RAID_GROUP_TYPE_RAID10){
            if(index%2 == 0){
               loop_index++;
               drive_index = 0;
            }
            sprintf (mytemp, 
                "\n<Rebuild logging flag %d_%d_%d> %s"
                "\n<Rebuild Chkpt for %d_%d_%d> 0x%llX", 
                rg_drives.drive_list[index].port_num,
                rg_drives.drive_list[index].encl_num,
                rg_drives.drive_list[index].slot_num,
                raid_group_info[loop_index].b_rb_logging[drive_index]?"TRUE":"FALSE",
                rg_drives.drive_list[index].port_num,
                rg_drives.drive_list[index].encl_num,
                rg_drives.drive_list[index].slot_num,
                raid_group_info[loop_index].rebuild_checkpoint[drive_index]
                );
            drive_index++;
        }
        else{
            sprintf (mytemp, 
                "\n<Rebuild logging flag %d_%d_%d> %s"
                "\n<Rebuild Chkpt for %d_%d_%d> 0x%llX", 
                rg_drives.drive_list[index].port_num,
                rg_drives.drive_list[index].encl_num,
                rg_drives.drive_list[index].slot_num,
                raid_group_info[loop_index].b_rb_logging[index]?"TRUE":"FALSE",
                rg_drives.drive_list[index].port_num,
                rg_drives.drive_list[index].encl_num,
                rg_drives.drive_list[index].slot_num,
                raid_group_info[0].rebuild_checkpoint[index]
                );
        }
        data = utilityClass::append_to(data, mytemp);

        delete [] mytemp;
    }

    sprintf (temp, "\n<Lun(s) in RG> ");
    data = utilityClass::append_to (data, temp);
	
    if (raid_group_luns){

        for(index = 0; index < lun_count; index++){

            mytemp = new char[50];
            sprintf (mytemp, "%d ", raid_group_luns[index]);
            data = utilityClass::append_to(data, mytemp);
            
            delete [] mytemp;
        }
    }

    return data;
}

/*********************************************************************
* sepRaidGroup Class :: edit_raid_group_power_saving_policy() 
                            (private method)
*********************************************************************
*
*  Description:
*      Format the rg power saving policy information to display
*
*  Input: Parameters
*      rg_ps_policy - rg power saving policy structure

*  Returns:
*      void
*********************************************************************/

void sepRaidGroup::edit_raid_group_power_saving_policy(
            fbe_raid_group_get_power_saving_info_t *rg_ps_policy)
{
    // rg power saving policy information
    sprintf(temp,                       "\n "
        "<Max Raid Latency Time In Sec>     %llu\n "
        "<Power Saving Enabled>             %d\n "
        "<Idle Time In Sec>                 %llu\n ",
        
        (long long)rg_ps_policy->max_raid_latency_time_is_sec,
        rg_ps_policy->power_saving_enabled,
        (long long)rg_ps_policy->idle_time_in_sec);
}

/*********************************************************************
* sepRaidGroup::Sep_Raid_Intializing_variable (private method)
*********************************************************************
*
*  Description:
*      Initialize the class variables.
*            
*  Input: Parameters
*      argc  - argument count 
*      *argv - pointer to arguments 
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*
*********************************************************************/
void sepRaidGroup::Sep_Raid_Intializing_variable()
{
    fbe_u8_t index = 0;
    // initialize variables
    raid_group_id = FBE_OBJECT_ID_INVALID;
    ps_idle_time = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    capacity = FBE_LBA_INVALID;
    raid_group_luns = NULL;
    for(index=0;index < FBE_MAX_DOWNSTREAM_OBJECTS ; index++){
        fbe_zero_memory(&raid_group_info[index],sizeof(raid_group_info[index]));
    }
    destroy_rg.raid_group_id = 0;
    rg_ps_policy.idle_time_in_sec = 0;
    rg_ps_policy.max_raid_latency_time_is_sec = 0;
    rg_ps_policy.power_saving_enabled = 0;
    lu_verify_type = FBE_LUN_VERIFY_TYPE_NONE;
    rebuild_status.checkpoint = 0;
    rebuild_status.percentage_completed =0;
    bg_verify_status.checkpoint = 0;
    bg_verify_status.percentage_completed = 0;
    raid_type = FBE_RAID_GROUP_TYPE_UNKNOWN;
    drive_count = 0;
    status = FBE_STATUS_FAILED;
    rg_obj_id = 0;
    rg_request.raid_group_id = 0;
    rg_request.drive_count = 0;
    rg_request.power_saving_idle_time_in_seconds = 0;
    rg_request.max_raid_latency_time_is_sec = 0;
    rg_request.raid_type = FBE_RAID_GROUP_TYPE_UNKNOWN;
    //rg_request.explicit_removal = 0;
    rg_request.is_private = FBE_TRI_INVALID;
    rg_request.b_bandwidth = 0;
    rg_request.capacity = 0;
    rg_verify_type = FBE_LUN_VERIFY_TYPE_NONE;
    upstream_object_list.current_upstream_index = 0;
    upstream_object_list.number_of_upstream_objects = 0;
    upstream_object_list.upstream_object_list[0] = '\0';
    upstream_lun_object_list.current_upstream_index = 0;
    upstream_lun_object_list.number_of_upstream_objects = 0;
    upstream_lun_object_list.upstream_object_list[0] = '\0';
    lun_count = 0;
    paged_p = NULL;
    current_paged_p = NULL;
    return;
}

/*********************************************************************
* sepRaidGroup Class :: get_lba_mapping(
*    int argc,
*    char ** argv)
*********************************************************************
*
*  Description:
*      Maps the lba onto the raidgroup.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepRaidGroup::get_lba_mapping(
    int argc,
    char ** argv)
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [rg object id] [lba]\n"\
        " For example: %s  0x10e 0\n";
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "mapLba",
        "sepRaidGroup::get_lba_mapping",
        "fbe_api_raid_group_map_lba",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Extract raid group type
    argv++;
    rg_obj_id = (fbe_object_id_t)strtoul(*argv, 0, 0);

    argv++;

    // get the lba
    if ((**argv == '0') && (*(*argv + 1) == 'x' || *(*argv+ 1) == 'X'))
    {
        status = string_to_hex64((fbe_u8_t* )*argv, &rg_map_info.lba);
        sprintf(params, " rg object id (0x%x), lba (0x%llx)",
        rg_obj_id,rg_map_info.lba);

    }
    else{
        rg_map_info.lba = (fbe_lba_t)_strtoui64(*argv, NULL, 10);
        sprintf(params, " rg object id (0x%x), lba (%lld)",
        rg_obj_id,rg_map_info.lba);

    }

     if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Error while convering string to hex value\n");
        call_post_processing(status, temp, key, params);
        return status;
     }

    //make API call
    status = fbe_api_raid_group_map_lba( rg_obj_id, &rg_map_info);

    if(status == FBE_STATUS_OK){

        sprintf(temp,"\n"
            " <pba>          0x%llx\n"
            " <data pos>     %d\n"
            " <parity pos>   %d\n"
            " <chunk_index>  0x%llx\n"
            " <metadata lba> 0x%llx\n"
            " <original lba> 0x%llx\n"
            " <offset>       0x%llx\n",
            rg_map_info.pba,
            rg_map_info.data_pos,
            rg_map_info.parity_pos,
            rg_map_info.chunk_index,
            rg_map_info.metadata_lba,
            rg_map_info.original_lba,
            rg_map_info.offset);
    }
    else{
        sprintf(temp,"Failed to map the lba for RG object id 0x%x", rg_obj_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepRaidGroup Class :: get_pba_mapping(
*    int argc,
*    char ** argv)
*********************************************************************
*
*  Description:
*      Maps the pba of a disk to an lba.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepRaidGroup::get_pba_mapping(
    int argc,
    char ** argv)
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [rg object id] [pba]\n"\
        " For example: %s  0x10e 0\n";
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "mapPba",
        "sepRaidGroup::get_pba_mapping",
        "fbe_api_raid_group_map_lba",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Extract raid group type
    argv++;
    rg_obj_id = (fbe_object_id_t)strtoul(*argv, 0, 0);

    argv++;

    // get the lba
    if ((**argv == '0') && (*(*argv + 1) == 'x' || *(*argv+ 1) == 'X'))
    {
        status = string_to_hex64((fbe_u8_t* )*argv, &rg_map_info.pba);
        argv++;
        rg_map_info.position = (fbe_u32_t)strtoul(*argv, 0, 0);

        sprintf(params, " rg object id (0x%x), pba (0x%llx), position %d",
        rg_obj_id,(unsigned long long)rg_map_info.pba, rg_map_info.position);


    }
    else{
        rg_map_info.pba = (fbe_lba_t)_strtoui64(*argv, NULL, 10);
        argv++;
        rg_map_info.position = (fbe_u32_t)strtoul(*argv, 0, 0);

        sprintf(params, " rg object id (0x%x), pba (%lld) position %d",
        rg_obj_id,(long long)rg_map_info.pba, rg_map_info.position);
    }

     if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Error while convering string to hex value\n");
        call_post_processing(status, temp, key, params);
        return status;
     }

    //make API call
    status = fbe_api_raid_group_map_pba( rg_obj_id, &rg_map_info);

    if(status == FBE_STATUS_OK){

        sprintf(temp,"\n"
            " <pba>          0x%llx\n"
            " <data pos>     %d\n"
            " <parity pos>   %d\n"
            " <chunk_index>  0x%llx\n"
            " <metadata lba> 0x%llx\n"
            " <lba>          0x%llx\n"
            " <offset>       0x%llx\n",
            rg_map_info.pba,
            rg_map_info.data_pos,
            rg_map_info.parity_pos,
            rg_map_info.chunk_index,
            rg_map_info.metadata_lba,
            rg_map_info.lba,
            rg_map_info.offset);
    }
    else{
        sprintf(temp,"Failed to map the lba for RG object id 0x%x", rg_obj_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}


/*********************************************************************
* sepRaidGroup Class :: get_paged_metadata(
*    int argc,
*    char ** argv)
*********************************************************************
*
*  Description:
*      Get the paged metadata.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepRaidGroup::get_paged_metadata(
    int argc,
    char ** argv)
{
    char *data = NULL;
    // Define specific usage string  
    usage_format =
        " Usage: %s [rg object id] [offset] [chunk count]\n"\
        " For example: %s  0x10e 1 3\n";
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getPagedMetadata",
        "sepRaidGroup::get_paged_metadata",
        "fbe_api_raid_group_get_paged_metadata",
        usage_format,
        argc,9);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Extract raid group type
    argv++;
    rg_obj_id = (fbe_object_id_t)strtoul(*argv, 0, 0);

    argv++;
    chunk_offset = ( fbe_chunk_index_t )strtoul(*argv, 0, 0);

    argv++;
    status = string_to_hex64((fbe_u8_t*)*argv, &max_chunk_count);

    sprintf(params, " rg object id (x%x), "
        "chunk_offset (0x%llx),"
        " chunk_count (0x%llx)",
        rg_obj_id,chunk_offset, max_chunk_count);

     if (status != FBE_STATUS_OK)
    {
        sprintf(temp,"Error while convering string to hex value\n");
        call_post_processing(status, temp, key, params);
        return status;
     }

    if ( max_chunk_count > FBE_U32_MAX)
    {
         sprintf(temp, "Requested chunks count %llu is exceeding the "
         "maximum value %u", max_chunk_count, FBE_U32_MAX);
         call_post_processing(status, temp, key, params);
         return status;
    }

    paged_p = (fbe_raid_group_paged_metadata_t*)fbe_api_allocate_memory(
     (fbe_u32_t)max_chunk_count * sizeof(fbe_raid_group_paged_metadata_t));

    status = fbe_api_raid_group_get_paged_metadata(rg_obj_id,
            chunk_offset, max_chunk_count, paged_p, FBE_FALSE /* Allow data from cache*/);
    if (status != FBE_STATUS_OK) 
    { 
        sprintf(temp,"Failed to get the paged metadata for"
            " RG object id 0x%x", rg_obj_id);
        data = utilityClass::append_to (data, temp);
        fbe_api_free_memory(paged_p);
        // Output results from call or description of error
        call_post_processing(status, temp, key, params);
        return status;
    }

    // For each chunk in this request, do display.
    sprintf(temp,  " \n<chunk offset>        0x%x", (unsigned int)chunk_offset);
    data = utilityClass::append_to (data, temp);
    for ( index = 0; index < max_chunk_count; index++)
    {
        current_paged_p = &paged_p[index];
        //Get the data
             sprintf(temp,"\n"
        " <Index>                    %d\n"
        " <Valid bit>                %llx\n"
        " <Verify System>            %d\n"
        " <Verify Incomplete Write>  %d\n"
        " <Verify Error>             %d\n"
        " <User Read Only>           %d\n"
        " <User Read Write>          %d\n"
        " <Needs Rebuild>            %d\n",
        index,
        (unsigned long long)current_paged_p->valid_bit,
        ((current_paged_p->verify_bits & FBE_RAID_BITMAP_VERIFY_SYSTEM)>>4),
        ((current_paged_p->verify_bits & FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE)>>3),
        ((current_paged_p->verify_bits & FBE_RAID_BITMAP_VERIFY_ERROR)>>2),
        ((current_paged_p->verify_bits & FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY)>>1),
        (current_paged_p->verify_bits & FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE),
        current_paged_p->needs_rebuild_bits);
        data = utilityClass::append_to (data, temp);
     }

     //Free the memory
    fbe_api_free_memory(paged_p);

    // Output results from call or description of error
    call_post_processing(status, data, key, params);
    return status;
}

/*********************************************************************
* sepRaidGroup Class :: get_raid_group_state(
*    int argc,
*    char ** argv)
*********************************************************************
*
*  Description:
*      Get the raide group state.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepRaidGroup::get_raid_group_state(
    int argc,
    char ** argv)
{

    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_u32_t index = 0;
    fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;
    const fbe_char_t *state;

    // Define specific usage string  
    usage_format =
        " Usage: %s [rg object id] \n"\
        " For example: %s  0x10e\n";
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getRaidgroupState",
        "sepRaidGroup::get_raid_group_state",
        "fbe_api_get_object_lifecycle_state",
        usage_format,
        argc,7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Extract raid group type
    argv++;
    rg_obj_id = (fbe_object_id_t)strtoul(*argv, 0, 0);

    sprintf(params, " rg object id (0x%x)",
        rg_obj_id);

    fbe_api_get_object_class_id(rg_obj_id,&class_id, FBE_PACKAGE_ID_SEP_0);
    if((class_id <= FBE_CLASS_ID_RAID_FIRST || class_id >= 
    FBE_CLASS_ID_RAID_LAST) && (class_id != FBE_CLASS_ID_VIRTUAL_DRIVE)){
        sprintf(temp, "Invalid Raid group object id 0x%x",
            rg_obj_id);
        call_post_processing(status, temp, key, params); 
        return FBE_STATUS_FAILED;
    }
    else if(class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        status = fbe_api_base_config_get_upstream_object_list(
                rg_obj_id, 
                &upstream_lun_object_list);

        rg_obj_id = upstream_lun_object_list.upstream_object_list[0];

        if(status != FBE_STATUS_OK){
            sprintf(temp, "Failed to get the Raid group object list for "\
                "RG  (0x%X)",
                 rg_obj_id);
            return status;
        }
    }
    // Make api call: 
    status = fbe_api_raid_group_get_info(rg_obj_id, &raid_group_info[0],
             FBE_PACKET_FLAG_NO_ATTRIB);

    if(status != FBE_STATUS_OK){
        sprintf(temp, "Failed while getting the RGinfo for RG object id 0x%x",
        rg_obj_id);
         call_post_processing(status, temp, key, params); 
        return status;
    }
    //For Raid R10 we have intermidiate mirror rg object
    if(raid_group_info[0].raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
        status = fbe_api_base_config_get_upstream_object_list(
                rg_obj_id, 
                &upstream_lun_object_list);
        if(status != FBE_STATUS_OK){
            sprintf(temp, "Failed to get the Raid group object list for "\
                "RG  (0x%X)",
                 rg_obj_id);
            return status;
        }
        rg_obj_id = upstream_lun_object_list.upstream_object_list[0];
        status = fbe_api_raid_group_get_info(rg_obj_id, &raid_group_info[0],
             FBE_PACKET_FLAG_NO_ATTRIB);
    }
    //For Raid R10 we have intermidiate mirror rg object
    if (raid_group_info[0].raid_type == FBE_RAID_GROUP_TYPE_RAID10){
        
        downstream_object_list.number_of_downstream_objects = 0;
        for(index = 0; index < FBE_MAX_DOWNSTREAM_OBJECTS; index++)
        {
            downstream_object_list.downstream_object_list[index] = FBE_OBJECT_ID_INVALID;
        }
        // Get Mirror rg associated with this RG
        status = fbe_api_base_config_get_downstream_object_list(
                 rg_obj_id, &downstream_object_list);
        if (status != FBE_STATUS_OK)
        {
            sprintf(temp, "Failed while getting the RGinfo for RG object id 0x%x",rg_obj_id);
            call_post_processing(status, temp, key, params); 
            return status;
        }

        for(index = 0; index < downstream_object_list.number_of_downstream_objects; index++)
        {
              status = fbe_api_raid_group_get_info(downstream_object_list.downstream_object_list[index], 
              &raid_group_info[index+1],//as raid_group_info[o] already filled 
                                        // by information of Raid10 object
             FBE_PACKET_FLAG_NO_ATTRIB);
              if(status != FBE_STATUS_OK){
                sprintf(temp, "Can't get the RGinfo 0x%x",
                    downstream_object_list.downstream_object_list[index]);
                 call_post_processing(status, temp, key, params); 
                 return status;
              }
        }
    }

    state = raid_group_state(rg_obj_id);

    if(strcmp(state,"INVALID_STATUS") == 0){
        sprintf(temp,"Failed to get the paged metadata for RG "
        "object id 0x%x", rg_obj_id);
        status = FBE_STATUS_FAILED;
    }
    else{
        sprintf(temp,"\n"
            " <RG state>            %s\n",
            state);
        status = FBE_STATUS_OK;
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}


/*********************************************************************
* sepRaidGroup Class :: raid_group_state()
*********************************************************************
*
*  Description:
*      Get the Raid group state information.
*
*  Input: Parameters
*      object_id  - object id
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
const fbe_char_t* sepRaidGroup::raid_group_state(fbe_object_id_t object_id)
{
    fbe_lifecycle_state_t lifecycle_state;
    const fbe_char_t *p_string;
    fbe_u32_t rg_width;
    fbe_u8_t index = 0, loop_index = 0,drive_index=0;

    //calculating rg width for raid1_0
    if (raid_group_info[0].raid_type == FBE_RAID_GROUP_TYPE_RAID10){
        rg_width = raid_group_info[0].width*2;
    } else {
        rg_width = raid_group_info[0].width;
    }

    status = fbe_api_get_object_lifecycle_state(object_id,
                                                &lifecycle_state,
                                                FBE_PACKAGE_ID_SEP_0);
    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get lifecycle state for object id 0x%x",
                object_id);
        return("INVALID_STATUS");
    }

    if(lifecycle_state == FBE_LIFECYCLE_STATE_READY)
    {
        for(index = 0; index < rg_width; index++)
        {
            if (raid_group_info[0].raid_type == FBE_RAID_GROUP_TYPE_RAID10){
                if(index%2 == 0){
                   loop_index++;
                   drive_index = 0;
                }
                if (raid_group_info[loop_index].b_rb_logging[drive_index]){
                    return(p_string = "DEGRADED");
                }
                drive_index ++;
            }
            else{
                if(raid_group_info[loop_index].b_rb_logging[index]){
                    return(p_string = "DEGRADED");
                }
            }
        }
    }

    p_string = utilityClass::convert_state_to_string(lifecycle_state);
    
    return p_string;
}

/*********************************************************************
* sepRaidGroup Class :: set_mirror_prefered_position()
*********************************************************************
*
*  Description:
*      Set mirror preferd position.
*
*  Input: Parameters
*      argc  - argument count 
*      *argv - pointer to arguments 
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*********************************************************************/

fbe_status_t sepRaidGroup::set_mirror_prefered_position(
   int argc, char ** argv)
{

    // Define common usage for SEP RAID Group commands
    usage_format = 
        " Usage: %s [RG object id] [prefered position]\n"
        " For example: %s 0x10e 1";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setMirrorPreferedPosition",
        "sepRaidGroup::set_mirror_prefered_position",
        "fbe_api_raid_group_set_mirror_prefered_position",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    rg_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    // Get the preferd position
    argv++;
    prefered_position = (fbe_u32_t)strtoul(*argv, 0, 0);

    sprintf(params,"RG object ID  %d(0x%x), preferd position (%d)",
        rg_object_id,  rg_object_id, prefered_position);

    //Make API call
    status = fbe_api_raid_group_set_mirror_prefered_position(
       rg_object_id, prefered_position);

    if(status != FBE_STATUS_OK)
    {
        sprintf(temp,"Failed to set the mirror prefered position to %d "
            "for RG object %d(0x%x)", prefered_position, rg_object_id, rg_object_id);

    }else {
        sprintf(temp,"Successfully set the mirror prefered position to "
           "%d for RG object %d(0x%x)", 
           prefered_position, rg_object_id, rg_object_id);
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params); 
    return status;

}

/*********************************************************************
* sepRaidGroup Class :: get_write_log_info()
*********************************************************************
*
*  Description:
*      Get write log information.
*
*  Input: Parameters
*      argc  - argument count 
*      *argv - pointer to arguments 
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*********************************************************************/

fbe_status_t sepRaidGroup::get_write_log_info(int argc, char ** argv)
{
    char *data = NULL;
    fbe_u32_t         current_slot_id = FBE_PARITY_WRITE_LOG_INVALID_SLOT;
    fbe_parity_get_write_log_slot_t *slot_p;
    fbe_u8_t display_string[32];
    
    // Define common usage for SEP RAID Group commands
    usage_format = 
        " Usage: %s [RG object id]\n"
        " For example: %s 0x10e";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getWriteLogInfo",
        "sepRaidGroup::get_write_log_info",
        "fbe_api_raid_group_get_write_log_info",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    rg_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

   sprintf(params,"RG object ID  %d(0x%x)", rg_object_id, rg_object_id);

    //Make API call
    status = fbe_api_raid_group_get_write_log_info(rg_object_id, 
                                                                   &write_log_info, 
                                                   FBE_PACKET_FLAG_NO_ATTRIB);

    if(status != FBE_STATUS_OK)
    {
        sprintf(temp,"Failed to get the write log information  for RG object %d(0x%x)", 
            rg_object_id, rg_object_id);
        data = utilityClass::append_to (data, temp);

    }else {
        sprintf(temp,                   "\n"
                            "<Needs remap>             %s\n"
                            "<Quiesced>                %s\n"
                            "<Slot count>              0x%x\n"
                            "<Slot size>               0x%x\n"
                            "<Start lba>               0x%llx\n", 
                            (write_log_info.needs_remap)?"True":"False",
                            (write_log_info.quiesced)?"True":"False",
                            write_log_info.slot_count,
                            write_log_info.slot_size,
                            write_log_info.start_lba);

        data = utilityClass::append_to (data, temp);

        for (current_slot_id = 0; current_slot_id < write_log_info.slot_count; current_slot_id++)
        {
            slot_p = &write_log_info.slot_array[current_slot_id];
            switch (slot_p->state)
            {
                case FBE_PARITY_WRITE_LOG_SLOT_STATE_FREE:
                    strcpy((char*)display_string, "FREE");
                    break;

                case FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED:
                    strcpy((char*)display_string, "ALLOCATED");
                    break;

                case FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED_FOR_FLUSH:
                    strcpy((char*)display_string, "ALLOCATED_FOR_FLUSH");
                    break;

                case FBE_PARITY_WRITE_LOG_SLOT_STATE_FLUSHING:
                    strcpy((char*)display_string, "FLUSHING");
                    break;

                case FBE_PARITY_WRITE_LOG_SLOT_STATE_INVALID:
                default:
                    strcpy((char*)display_string, "INVALID");
                    break;
            }
           sprintf(temp,"<Slot id>              %d\n"  
                " <State>                  %s\n", 
                current_slot_id, display_string); 
           data = utilityClass::append_to (data, temp);

            switch (slot_p->invalidate_state)
            {
                case FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_SUCCESS:
                    strcpy((char*)display_string, "SUCCESS");
                    break;

                case FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_DEAD:
                    strcpy((char*)display_string, "DEAD");
                    break;

                default:
                    strcpy((char*)display_string, "INVALID");
                    break;
            }
            sprintf(temp, " <Invalidate state>       %s\n", display_string); 
            data = utilityClass::append_to (data, temp);
        }
    }
    
    // Output results from call or description of error
    call_post_processing(status, data, key, params); 
    return status;

}

/*********************************************************************
* sepRaidGroup Class :: get_background_operation_speed
*********************************************************************
*
*  Description:
*      Get background operation speed
*
*  Input: Parameters
*      argc  - argument count 
*      *argv - pointer to arguments 
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*********************************************************************/

fbe_status_t sepRaidGroup::get_background_operation_speed(int argc, char ** argv)
{

    // Define common usage for SEP RAID Group commands
    usage_format = 
        " Usage: %s \n"
        " For example: %s";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getBgOpSpeed",
        "sepRaidGroup::get_background_operation_speed",
        "fbe_api_raid_group_get_background_operation_speed",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    sprintf(params," ");

    //Make API call
    status = fbe_api_raid_group_get_background_operation_speed(&get_bg_op);
    
    if(status != FBE_STATUS_OK)
    {
        sprintf(temp,"Failed to get the background operation speed");

    }else {
        sprintf(temp,"\n"
            " <Rebuild speed>         %d\n"
            " <Verify speed>          %d",
            get_bg_op.rebuild_speed,
            get_bg_op.verify_speed);
    }

    call_post_processing(status, temp,key,params);
    return status;
}

/*********************************************************************
* sepRaidGroup Class :: set_background_operation_speed
*********************************************************************
*
*  Description:
*      Set background operation speed
*
*  Input: Parameters
*      argc  - argument count 
*      *argv - pointer to arguments 
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*********************************************************************/

fbe_status_t sepRaidGroup::set_background_operation_speed(int argc, char ** argv)
{

    // Define common usage for SEP RAID Group commands
    usage_format = 
        " Usage: %s [speed][Background operation code]\n"
        " For example: %s 10 FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY\n"
        " Valid values for background operation code:\n"
        " FBE_RAID_GROUP_BACKGROUND_OP_REBUILD\n"
        " FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setBgOpSpeed",
        "sepRaidGroup::set_background_operation_speed",
        "fbe_api_raid_group_set_background_operation_speed",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Get speed
    argv++;
    speed = (fbe_u32_t)strtoul(*argv, 0, 0);

    // Get background operation speed
    argv++;
    get_bg_op_code(argv);


    sprintf(params, "Spped (%d), Background operation speed  (%s)", speed , *argv) ;
    
    //Make API call
    status = fbe_api_raid_group_set_background_operation_speed(bg_op_code, speed);
    
    if(status != FBE_STATUS_OK)
    {
        sprintf(temp,"Failed to set the background operation speed to %d", speed);

    }else {
        sprintf(temp,"Successfully set the background operation speed to %d", speed);
    }
    call_post_processing(status, temp,key,params);
    return status;
}

/*********************************************************************
* sepRaidGroup Class :: get_bg_op_code
*********************************************************************
*
*  Description:
*      Get background operation code
*
*  Input: Parameters
*      *argv - pointer to arguments 
*    
*  Returns:
*      void
*********************************************************************/
void sepRaidGroup::get_bg_op_code(char**argv){

    if(strcmp(*argv, "FBE_RAID_GROUP_BACKGROUND_OP_REBUILD") == 0){
        bg_op_code = FBE_RAID_GROUP_BACKGROUND_OP_REBUILD;
    }
    else     if(strcmp(*argv, "FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY") == 0){
        bg_op_code = FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY;
    }
}
/*********************************************************************
* sepRaidGroup end of file
*********************************************************************/
