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
*      This file defines the methods of the SEP LUN INTERFACE class.
*
*  Notes:
*      The SEP LUN class (sepLun) is a derived class of
*      the base class (bSEP).
*
*  History:
*      07-March-2011 : initial version
*
*********************************************************************/

#ifndef T_SEP_LUN_CLASS_H
#include "sep_lun_class.h"
#endif

/*********************************************************************
* sepLun class :: Constructor
*********************************************************************/

sepLun::sepLun() : bSEP()
{
    // Store Object number
    idnumber = (unsigned) SEP_LUN_INTERFACE;
    sepLunCount = ++gSepLunCount;

    // Store Object name
    idname = new char [64];
    strcpy(idname, "SEP_LUN_INTERFACE");
    Sep_Lun_Intializing_variable();

    if (Debug) {
        sprintf(temp, "sepLun class constructor (%d) %s\n",
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP LUN Interface function calls>
    sepLunInterfaceFuncs =
        "[function call]\n" \
        "[short name]             [FBE API SEP LUN Interface]\n" \
        " ----------------------------------------------------------------\n" \
        " getZeroStat             fbe_api_lun_get_zero_status\n" \
        " ----------------------------------------------------------------\n" \
        " initVerify              fbe_api_lun_initiate_verify\n" \
        " getVerify               fbe_api_lun_get_verify_status\n" \
        " getBGVerify             fbe_api_lun_get_bgverify_status\n" \
        " getVerifyRep            fbe_api_lun_get_verify_report\n" \
        " clearVerifyRep          fbe_api_lun_clear_verify_report\n" \
        " initVerifyOnAllLuns     fbe_api_lun_initiate_verify_on_all_existing_luns\n"\
        " ----------------------------------------------------------------\n" \
        " enablePerfStats         fbe_api_lun_enable_peformance_stats\n" \
        " disablePerfStats        fbe_api_lun_disable_peformance_stats\n" \
        " getPerfStats            fbe_api_lun_get_peformance_stats\n" \
        " ----------------------------------------------------------------\n" \
        " unbind                  fbe_api_destroy_lun\n" \
        " ----------------------------------------------------------------\n"\
        " update                  fbe_api_update_lun_wwn\n" \
        " ----------------------------------------------------------------\n"\
        " calcImportedCap         fbe_api_lun_calculate_imported_capacity\n" \
        " calcExportedCap         fbe_api_lun_calculate_exported_capacity\n" \
        " calcMaxLunSize          fbe_api_lun_calculate_max_lun_size\n" \
        " ----------------------------------------------------------------\n"\
        " setPowerSavingPolicy    fbe_api_lun_set_power_saving_policy\n" \
        " getPowerSavingPolicy    fbe_api_lun_get_power_saving_policy\n" \
        " ----------------------------------------------------------------\n"\
        " getRebuildStatus        fbe_api_lun_get_rebuild_status\n" \
        " getLunInfo              fbe_api_lun_get_lun_info\n" \
        " ----------------------------------------------------------------\n"\
        " mapLba                  fbe_api_lun_map_lba\n";
        " ----------------------------------------------------------------\n";

    // Define common usage for SEP Lun commands
    usage_format =
        " Usage: %s [object id]\n"
        " For example: %s 4";
};

/*********************************************************************
* sepLun class : Accessor methods
*********************************************************************/

unsigned sepLun::MyIdNumIs(void)
{
    return idnumber;
};

char * sepLun::MyIdNameIs(void)
{
    return idname;
};

void sepLun::dumpme(void)
{
    strcpy (key, "sepLun::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n",
            idnumber, sepLunCount);
    vWriteFile(key, temp);
};

/*********************************************************************
* sepLun Class :: select()
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
*      07-March-2011 : initial version
*
*********************************************************************/

fbe_status_t sepLun::select(int index, int argc, char *argv[])
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface");

    // List SEP LUN Interface calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepLunInterfaceFuncs );

        // get LUN zero status
    } else if (strcmp (argv[index], "GETZEROSTAT") == 0) {
        status = get_lun_zero_status(argc, &argv[index]);

        // initiate lun verify
    } else if (strcmp (argv[index], "INITVERIFY") == 0) {
        status = initiate_lun_verify(argc, &argv[index]);

        // get lun verify status
    } else if (strcmp (argv[index], "GETVERIFY") == 0) {
        status = get_lun_verify_status(argc, &argv[index]);

        // get lun BGVerify status
    } else if (strcmp (argv[index], "GETBGVERIFY") == 0) {
        status = get_lun_bgverify_status(argc, &argv[index]);

        // get lun verify report
    } else if (strcmp (argv[index], "GETVERIFYREP") == 0) {
        status = get_lun_verify_report(argc, &argv[index]);

        // clear lun verify report
    } else if (strcmp (argv[index], "CLEARVERIFYREP") == 0) {
        status = clear_lun_verify_report(argc, &argv[index]);

        // enable performance statistics
    } else if (strcmp (argv[index], "ENABLEPERFSTATS") == 0) {
        status = enable_perf_stats(argc, &argv[index]);

        // disable performance statistics
    } else if (strcmp (argv[index], "DISABLEPERFSTATS") == 0) {
        status = disable_perf_stats(argc, &argv[index]);

        // Unbind LUN
    } else if (strcmp (argv[index], "UNBIND") == 0) {
        status = unbind_lun(argc, &argv[index]);

    // Get power saving policy
    } else if (strcmp (argv[index], "GETPOWERSAVINGPOLICY") == 0) {
        status = get_lun_power_saving_policy(argc, &argv[index]);

    // Set power saving policy
    } else if (strcmp (argv[index], "SETPOWERSAVINGPOLICY") == 0) {
        status = set_lun_power_saving_policy(argc, &argv[index]);

    // Update LUN
    } else if (strcmp (argv[index], "UPDATE") == 0) {
        status = update_lun(argc, &argv[index]);

    // Calculate imported capacity of the LUN
    } else if (strcmp (argv[index], "CALCIMPORTEDCAP") == 0) {
        status = calculate_lun_imported_capacity(argc, &argv[index]);

    // Calculate exported capacity of the LUN
    } else if (strcmp (argv[index], "CALCEXPORTEDCAP") == 0) {
        status = calculate_lun_exported_capacity(argc, &argv[index]);

    // Calculate max size of the LUN
    } else if (strcmp (argv[index], "CALCMAXLUNSIZE") == 0) {
        status = calculate_max_lun_size(argc, &argv[index]);

    // Get rebuild status for a LUN
    } else if (strcmp (argv[index], "GETREBUILDSTATUS") == 0) {
        status = get_lun_rebuild_status(argc, &argv[index]);

    // Get LUN info
    } else if (strcmp (argv[index], "GETLUNINFO") == 0) {
        status = get_lun_info(argc, &argv[index]);

    // Initiate verify on LUNs
    } else if (strcmp (argv[index], "INITVERIFYONALLLUNS") == 0) {
        status = initiate_verify_on_all_existing_luns(
        		argc, &argv[index]);

        // Map LBA of a LUN to a physical address
    } else if (strcmp (argv[index], "MAPLBA") == 0) {
        status = lun_map_lba(argc, &argv[index]);

    } else {
        // can't find match on short name
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n",
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepLunInterfaceFuncs );
    }

    return status;
}

/*!***************************************************************
*  sepLun class :: lun_map_lba()
****************************************************************
*  @brief:
*    Maps an lba of a lun to a physical address.
*
*  Input: Parameters
*    argc  - argument count
*    *argv - pointer to two arguments:
*        - LUN's object id,
*        - LBA.
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*
* @version
*  8/13/2012 - Created by Eugene Shubert
*
****************************************************************/
fbe_status_t sepLun::lun_map_lba(int argc , char ** argv)
{
	    usage_format =
	        " Usage: %s [lun object id] [lba id]\n"
	        " For example: %s 17 18";

	    // Check that all arguments have been entered
        status = call_preprocessing(
            "mapLba",
            "sepLun::lun_map_lba",
            "fbe_api_lun_map_lba",
            usage_format,
            argc, 8);

        // Return if error found or help option entered
        if (status != FBE_STATUS_OK) {
            return status;
        }

        // Convert Object id to hex
        argv++;
        lu_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
        sprintf(params, "lu object id 0x%x (%s)", lu_object_id, *argv);

        fbe_raid_group_map_info_t rg_map_info_p;
        argv++;
        rg_map_info_p.lba = (fbe_u64_t)_strtoui64(*argv, 0, 0);

        // Make api call:
        status = fbe_api_lun_map_lba(lu_object_id, &rg_map_info_p);

        // Check status of call
        if (status != FBE_STATUS_OK) {
            sprintf(temp, "Could not map LBA");
        } else {
            show_rg_map_info(&rg_map_info_p);
        }

        // Output results from call or description of error
        call_post_processing(status, temp, key, params);
        return status;
}

/*********************************************************************
* sepLun class :: show_rg_map_info ()
**********************************************************************
* @brief:
*     Outputs the content of RAID Group map.
*********************************************************************/
void sepLun::show_rg_map_info(fbe_raid_group_map_info_t *rg_map_info_p)
{
    // format drive information
    sprintf(temp,              "\n "
        "<LBA>          0x%llX\n "
        "<PBA>          0x%llX\n "
        /* Chunk in paged: */
        "<Chunk Index>  0x%llX\n "
        /* Logical Address of metadata chunk in the paged metadata: */
        "<Metadata LBA> 0x%llX\n "
        /* Index of data position in array: */
        "<Data pos>     0x%llX\n "
        /*!< Index of parity position in array: */
        "<Parity pos>   0x%llX\n "
    	/* Offset on raid group's downstream edge: */
        "<Offset>       0x%llX\n "
    	/* Used for R10 only, the top level RG's lba: */
        "<Original LBA> 0x%llX\n ",
        rg_map_info_p->lba,
        rg_map_info_p->pba,
        rg_map_info_p->chunk_index,
        rg_map_info_p->metadata_lba,
        (unsigned long long)rg_map_info_p->data_pos,
        (unsigned long long)rg_map_info_p->parity_pos,
        rg_map_info_p->offset,
        rg_map_info_p->original_lba );
}

/*********************************************************************
* sepLun class :: get_lun_zero_status ()
*********************************************************************/

fbe_status_t sepLun::get_lun_zero_status(int argc , char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getZeroStat",
        "sepLun::get_lun_zero_status",
        "fbe_api_lun_get_zero_status",
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

    // Make api call:
    status = fbe_api_lun_get_zero_status(
                lu_object_id, &lu_zero_status);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get LUN zero status");
    } else {
        edit_lu_zero_info(&lu_zero_status);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepLun class :: initiate_lun_verify ()
*********************************************************************/

fbe_status_t sepLun::initiate_lun_verify(int argc , char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [object id] [verify type: 1|2|3]\n"
        " Verify type: 1: user read/write  2: user read only  3: error verify\n"
        " For example: %s 4 1";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "initVerify",
        "sepLun::initiate_lun_verify",
        "fbe_api_lun_initiate_verify",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    lu_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    // Convert the lun verify type
    argv++;
    lu_verify_type = (enum fbe_lun_verify_type_e)strtoul(*argv, 0, 0);

    // Copy input parameters
    argv--;
    sprintf(params, " lu object id 0x%x (%s) , lun_verify_type (%d)",
            lu_object_id, *argv, lu_verify_type);

    // Make api call:
    status = fbe_api_lun_initiate_verify(lu_object_id,
                                         FBE_PACKET_FLAG_NO_ATTRIB, 
                                         lu_verify_type,
                                         FBE_TRUE, /* Verify the entire lun*/ 
                                         FBE_LUN_VERIFY_START_LBA_LUN_START,  
                                         FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN);   

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't initiate LUN verify");
    } else {
        sprintf(temp, "Initiated LUN verify on object id 0x%x",
                lu_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepLun class :: get_lun_verify_status ()
*********************************************************************/

fbe_status_t sepLun::get_lun_verify_status(int argc , char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getVerify",
        "sepLun::get_lun_verify_status",
        "fbe_api_lun_get_verify_status",
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

    // Make api call:
    status = fbe_api_lun_get_verify_status(lu_object_id,
             FBE_PACKET_FLAG_NO_ATTRIB, &lu_verify_status);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get LUN verify status");
    } else {
        edit_lu_verify_info(&lu_verify_status);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepLun class :: get_lun_bgverify_status ()
*********************************************************************/

fbe_status_t sepLun::get_lun_bgverify_status(int argc , char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [object id] [verify type: 1|2|3|4|5]\n"
        " Verify type: 1: user read/write  2: user read only  3: error verify \n"\
        " 4: incomplete write verify  5: system verify \n"
        " For example: %s 0x10f 1";
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getBGVerify",
        "sepLun::get_lun_bgverify_status",
        "fbe_api_lun_get_bgverify_status",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    lu_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    // Convert the lun verify type
    argv++;
    lu_verify_type = (enum fbe_lun_verify_type_e)strtoul(*argv, 0, 0);

    // Copy input parameters
    argv--;
    sprintf(params, " lu object id 0x%x (%s) , lun_verify_type (%d)",
            lu_object_id, *argv, lu_verify_type);

    // Make api call:
    status = fbe_api_lun_get_bgverify_status(
             lu_object_id, &lu_bgverify_status, lu_verify_type);

    // Check status of call
    if (status != FBE_STATUS_OK) {

        sprintf(temp, "Can't get LUN BGVerify status");

    } else {

        // format BGV status information
        sprintf(temp,              "\n "
            "<Background Verify Chkpt>         0x%llX\n "
            "<Background Verify Pct>           %d\n ",
            (unsigned long long)lu_bgverify_status.checkpoint,
            lu_bgverify_status.percentage_completed);

    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepLun class :: get_lun_verify_report ()
*********************************************************************/

fbe_status_t sepLun::get_lun_verify_report(int argc , char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getVerifyRep",
        "sepLun::get_lun_verify_report",
        "fbe_api_lun_get_verify_report",
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

    // Make api call:
    status = fbe_api_lun_get_verify_report(lu_object_id,
             FBE_PACKET_FLAG_NO_ATTRIB, &lu_verify_report);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get LUN verify report");
    } else {
        edit_lu_verify_report(&lu_verify_report);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepLun class :: clear_lun_verify_report()
*********************************************************************/

fbe_status_t sepLun::clear_lun_verify_report(int argc , char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "clearVerifyRep",
        "sepLun::clear_lun_verify_report",
        "fbe_api_lun_clear_verify_report",
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

    // Make api call:
    status = fbe_api_lun_clear_verify_report(lu_object_id,
                                             FBE_PACKET_FLAG_NO_ATTRIB);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't clear LUN verify report");
    } else {
        sprintf(temp, "Cleared LUN verify report for object id 0x%x",
                lu_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepLun class :: enable_perf_stats ()
*********************************************************************/

fbe_status_t sepLun::enable_perf_stats(int argc , char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "enablePerfStats",
        "sepLun::enable_perf_stats",
        "fbe_api_lun_enable_peformance_stats",
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

    // Make api call:
    status = fbe_api_lun_enable_peformance_stats(lu_object_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't enable performance stats for LUN"\
                "obj id 0x%X (%d)", lu_object_id, lu_object_id);
    } else {
        sprintf(temp, "Performance stats enabled for LUN obj id "\
                "0x%X (%d)", lu_object_id, lu_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepLun class :: disable_perf_stats ()
*********************************************************************/

fbe_status_t sepLun::disable_perf_stats(int argc , char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "disablePerfStats",
        "sepLun::disable_perf_stats",
        "fbe_api_lun_disable_peformance_stats",
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

    // Make api call:
    status = fbe_api_lun_disable_peformance_stats(lu_object_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't disable performance stats for LUN "\
                "obj id 0x%X (%d)", lu_object_id, lu_object_id);
    } else {
        sprintf(temp, "Performance stats disabled for LUN obj id "\
                "0x%X (%d)", lu_object_id, lu_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepLun class :: unbind_lun () 
*********************************************************************/

fbe_status_t sepLun::unbind_lun(int argc , char ** argv)
{
    // specific usage format
    usage_format =
        " Usage: %s [lun id]\n"
        " For example: %s 4";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "unbind",
        "sepLun::unbind_lun",
        "fbe_api_destroy_lun",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    lun_number = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " LUN number 0x%x (%s)", lun_number, *argv);

    lu_destroy.lun_number = lun_number;

    // Make api call:
    fbe_job_service_error_type_t job_error_type;
    status = fbe_api_destroy_lun(&lu_destroy,
             FBE_TRUE, LU_DESTROY_TIMEOUT, &job_error_type);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't unbind LUN 0x%x (%s)",
                lun_number, *argv);
    } else {
        sprintf(temp, "LUN 0x%x (%s) deleted successfully",
                lun_number, *argv);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepLun Class :: get_lun_power_saving_policy()
*********************************************************************
*
*  Description:
*      This function gets the power saving policy for a LUN.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*
*  History
*      05-July-2011 : initial version
*********************************************************************/

fbe_status_t sepLun::get_lun_power_saving_policy(int argc, char** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getPowerSavingPolicy",
        "sepLun::get_lun_power_saving_policy",
        "fbe_api_lun_get_power_saving_policy",
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

    // Make api call:
    status = fbe_api_lun_get_power_saving_policy(lu_object_id,&in_policy);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get the power saving policy"
            " for the LUN 0x%x", lu_object_id);
    } else {
        sprintf(temp,              "\n "
            " <lun delay before_hibernate_in_sec>   %lld\n "
            " <max_latency_allowed_in_100ns>        %llu\n",
            (long long)in_policy.lun_delay_before_hibernate_in_sec,
            (unsigned long long)in_policy.max_latency_allowed_in_100ns);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepLun Class :: set_lun_power_saving_policy()
*********************************************************************
*
*  Description:
*       This function sets the power saving policy for a LUN.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*
*  History
*      05-July-2011 : initial version
*********************************************************************/
fbe_status_t sepLun ::set_lun_power_saving_policy(int argc, char ** argv)
{

    // Usage format
    usage_format =
        " Usage: %s [object id] [lun delay before hibernate in sec]"\
        " [max latency allowed in 100 ns]\n"
        " For example: %s 0x10e 5 600000000";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setPowerSavingPolicy",
        "sepLun::set_lun_power_saving_policy",
        "fbe_api_lun_set_power_saving_policy",
        usage_format,
        argc, 9);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    lu_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    //Get power saving policy attributes
    argv++;
    in_policy.lun_delay_before_hibernate_in_sec = (fbe_u64_t)atol(*argv);
    argv++;
    in_policy.max_latency_allowed_in_100ns = ( fbe_time_t)atol(*argv);

    sprintf(params, 
        " lu object id 0x%x,"
        " delay before hibarnate in sec %llu,"
        " maximum latency allowed in 100ns %llu",
        lu_object_id,
        (unsigned long long)in_policy.lun_delay_before_hibernate_in_sec,
        (unsigned long long)in_policy.max_latency_allowed_in_100ns);

    // Make api call:
    status = fbe_api_lun_set_power_saving_policy(lu_object_id,&in_policy);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't set the power saving policy for LUN"
            " object id 0x%X", lu_object_id);
    } else {
        sprintf(temp, "Power saving policy successfully set for LUN"
            " object id 0x%X\n", lu_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepLun Class :: update_lun(int argc , char ** argv)
*********************************************************************
*
*  Description:
*      This function updates a LUN.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*
*  History
*      07-July-2011 : initial version
*********************************************************************/
fbe_status_t sepLun::update_lun(int argc, char ** argv)
{
    // Usage format
    usage_format =
        " Usage: %s [object id] [WWN]\n"
        " For example: %s 0x10e 0x12FA71";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "update",
        "sepLun::update_lun",
        "fbe_api_update_lun_wwn",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

       // Convert Object id to hex
    argv++;
    lu_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    argv++;
    fbe_copy_memory((void*)&lu_update_struct,*argv, sizeof(fbe_assigned_wwid_t));

    sprintf(params, " lu object id 0x%x, WWN %s",
            lu_object_id, (char*)lu_update_struct.bytes);

    // Make api call:
    fbe_job_service_error_type_t	job_error_type;
    status = fbe_api_update_lun_wwn(lu_object_id, &lu_update_struct, &job_error_type);
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Unable to update LUN 0x%x WWN. ", lu_object_id);
        return status;
    }else {
        sprintf(temp, "Successfully updated LUN 0x%x WWN. \n",lu_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepLUN Class :: calculate_lun_imported_capacity(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      This function calculates the imported capacity based on
*      the given exported capacity and lun align size.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*
*  History
*      07-July-2011 : initial version
*********************************************************************/
fbe_status_t sepLun ::calculate_lun_imported_capacity(int argc, char ** argv)
{

    // Usage format
    usage_format =
        " Usage: %s [exported size] [lun align size]\n"
        " For example: %s 0x9000 0xc000 ";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "calcImportedCap",
        "sepLun::calculate_lun_imported_capacity",
        "fbe_api_lun_calculate_imported_capacity",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    capacity_info.exported_capacity = (fbe_lba_t)_strtoui64(*argv, 0, 0);
    argv++;
    capacity_info.lun_align_size = (fbe_lba_t)_strtoui64(*argv, 0, 0);

    sprintf(params, " exported size 0x%llX",
            (unsigned long long)capacity_info.exported_capacity);

    // Make api call:
    status = fbe_api_lun_calculate_imported_capacity(&capacity_info);
    if(status != FBE_STATUS_OK) {
        sprintf(temp, "Can't calculate the imported capacity");
    } else {
        sprintf(temp,              "\n "
            "<Imported capacity>     0x%llX\n ",
            (unsigned long long)capacity_info.imported_capacity);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;

}

/*********************************************************************
* sepLUN Class :: calculate_lun_exported_capacity(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      This function calculates the exported capacity based on
*      the given imported capacity and lun align size.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*
*  History
*      29-Nov-2011 : initial version
*********************************************************************/
fbe_status_t sepLun ::calculate_lun_exported_capacity(
    int argc, char ** argv)
{

    // Usage format
    usage_format =
        " Usage: %s [imported size] [lun align size]\n"
        " For example: %s 0x90000 0xc000 ";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "calcExportedCap",
        "sepLun::calculate_lun_exported_capacity",
        "fbe_api_lun_calculate_exported_capacity",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    capacity_info.imported_capacity = (fbe_lba_t)_strtoui64(*argv, 0, 0);
    argv++;
    capacity_info.lun_align_size = (fbe_lba_t)_strtoui64(*argv, 0, 0);

    sprintf(params, " imported size 0x%llX lun align size 0x%llX",
            (unsigned long long)capacity_info.imported_capacity,
            (unsigned long long)capacity_info.lun_align_size);

    // Make api call:
    status = fbe_api_lun_calculate_exported_capacity(&capacity_info);
    if(status != FBE_STATUS_OK) {
        sprintf(temp, "Can't calculate the exported capacity");
    } else {
        sprintf(temp,              "\n "
            "<Exported capacity>     0x%llX\n ",
            (unsigned long long)capacity_info.exported_capacity);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepLUN Class :: calculate_max_lun_size(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      This function calculates the maximum size of the lun for 
*      the given raid group, capacity and number of luns.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*
*  History
*      29-Nov-2011 : initial version
*********************************************************************/
fbe_status_t sepLun ::calculate_max_lun_size(
    int argc, char ** argv)
{

    // Usage format
    usage_format =
        " Usage: %s [rg id] [capacity] [number of luns]\n"
        " For example: %s 1 0x2000 2 ";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "calcMaxLunSize",
        "sepLun::calculate_max_lun_size",
        "fbe_api_lun_calculate_max_lun_size",
        usage_format,
        argc, 9);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    argv++;
    fbe_u32_t rg_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    argv++;
    fbe_lba_t capacity = (fbe_lba_t)_strtoui64(*argv, 0, 0);
    argv++;
    fbe_u32_t no_of_luns = (fbe_u32_t)strtoul(*argv, 0, 0);

    sprintf(params, " rg id %d, capacity 0x%llX, number of luns %d",
            rg_id, (unsigned long long)capacity, no_of_luns);
    
    fbe_lba_t max_lun_size = 0;

    // Make api call:
    status = fbe_api_lun_calculate_max_lun_size(
        rg_id,  
        capacity,
        no_of_luns,
        &max_lun_size);
    if(status != FBE_STATUS_OK) {
        sprintf(temp, "Can't calculate the maximum lun size");
    } else {
        sprintf(temp,              "\n "
            "<Max lun size>     0x%llX\n ", (unsigned long long)max_lun_size);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepLun Class :: get_lun_rebuild_status()
*********************************************************************
*
*  Description:
*      This function gets the rebuild status (percentage and checkpoint)
*      for a given LUN.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*
*  History
*      19-September-2011 : initial version
*********************************************************************/

fbe_status_t sepLun::get_lun_rebuild_status(int argc, char** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getRebuildStatus",
        "sepLun::get_lun_rebuild_status",
        "fbe_api_lun_get_rebuild_status",
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

    // Make api call:
    status = fbe_api_lun_get_rebuild_status(lu_object_id,&lu_rebuild_status);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to obtain rebuild status "
                "for the LUN 0x%x", lu_object_id);
    } else {
        // format rebuild status information
        sprintf(temp,              "\n "
            "<LUN Rebuild Chkpt>         0x%llX\n "
            "<LUN Rebuild Pct>           %d\n ",
            (unsigned long long)lu_rebuild_status.checkpoint,
            lu_rebuild_status.percentage_completed);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepLun class :: get_lun_info ()
*********************************************************************/

fbe_status_t sepLun::get_lun_info(int argc , char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getLunInfo",
        "sepLun::get_lun_info",
        "fbe_api_lun_get_lun_info",
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

    // Make api call:
    status = fbe_api_lun_get_lun_info(
                lu_object_id, &lun_info);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get the LUN info");
    } else {
        edit_get_lun_info(&lun_info);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sepLun class ::initiate_verify_on_all_existing_luns (int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Initiate the verify on all existing LUNs.
*
*  Input: Parameters
*      verify type : 1 - user read/write
*                    2 - user read only
*                    3 - error verify
*
*  Returns:
*      fbe_status_t
*********************************************************************/

fbe_status_t sepLun::initiate_verify_on_all_existing_luns (
    int argc , char ** argv)
{
    fbe_u32_t system_bool;

    // Define specific usage string
    usage_format =
        " Usage: %s [verify type: 1|2|3|4|5] [system:0|1]\n"
        " Verify type: 1|USER_RW : user read/write  2|USER_RO : user read only "
        "3|ERROR : error verify\n"
        "4|INCOMPLETE_WRITE : incomplete write 5|SYSTEM : system "
        " For example: \n"
        "\tinitVerifyOnAllLuns 1 0 (R/W on public LUNs) \n"
        "\tinitVerifyOnAllLuns 1 1 (R/W on public LUNs and System LUNs) \n"
        "\tinitVerifyOnAllLuns 2 0 (R/O on public LUNs) \n"
        "\tinitVerifyOnAllLuns 2 1 (R/O on public LUNs and System LUNS) \n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "initVerifyOn AllLuns",
        "sepLun::initiate_verify_on_all_existing_luns ",
        "fbe_api_lun_initiate_verify_on_all_existing_luns ",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert the lun verify type
    argv++;
    status = get_lun_verify_type(argv);
    if (status != FBE_STATUS_OK) {
            sprintf(temp, "Can't initiate LUN verify on all user LUNs.Invalid lun verify type.");
            call_post_processing(status, temp, key, params);
            return status;
    }

    argv++;
    if(((utilityClass::is_string(*argv) == FBE_FALSE)&&(strtoul(*argv, 0, 
    0) == 1))||(strcmp(*argv, "SYS_ENABLE") == 0)) {
        system_bool = SYS_ENABLE;
    } else if(((utilityClass::is_string(*argv) == FBE_FALSE)&&(strtoul(*argv, 0, 
    0) == 0))||(strcmp(*argv, "SYS_DISABLE") == 0)){
        system_bool = SYS_DISABLE;
    }else{
            sprintf(temp, "Can't initiate LUN verify on all user LUNs.Invalid system type");
            call_post_processing(status, temp, key, params);
            return status;
    }
    
    // Copy input parameters
    sprintf(params, " verify_type (%d) system_type (%d)", lu_verify_type,system_bool);
    if (system_bool == 1 && (lu_verify_type < FBE_LUN_VERIFY_TYPE_ERROR || 
    lu_verify_type == FBE_LUN_VERIFY_TYPE_SYSTEM)) {
        
        // Make api call:
        status = fbe_api_lun_initiate_verify_on_all_user_and_system_luns(
            lu_verify_type);   
        // Check status of call
        if (status != FBE_STATUS_OK) {
            sprintf(temp, "Can't initiate LUN verify on all LUNs");
            call_post_processing(status, temp, key, params);
            return status;
        }
        
        sprintf(temp, "Initiate LUN verify on all System and User LUNs");
        
    }
    else if(system_bool == 0 )
    {

        // Make api call:
        status = fbe_api_lun_initiate_verify_on_all_existing_luns(
            lu_verify_type);   

        // Check status of call
        if (status != FBE_STATUS_OK) {
            sprintf(temp, "Can't initiate LUN verify on all LUNs");
        } else {
            sprintf(temp, "Initiate LUN verify on all LUNs");
        }
    }
    else 
    {
        sprintf(temp, "Can't initiate LUN verify on all LUNs.Invalid system type");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}
/*********************************************************************
* sepLun class :: edit_lu_zero_info (private method)
*********************************************************************/

void sepLun::edit_lu_zero_info(fbe_api_lun_get_zero_status_t *lu_zero_status)
{
    // format drive information
    sprintf(temp,              "\n "
        "<Zero Chkpt>         0x%llX\n "
        "<Zero Pct>           %d\n ",
        (unsigned long long)lu_zero_status->zero_checkpoint,
        lu_zero_status->zero_percentage);
}

/*********************************************************************
* sepLun class :: edit_lu_verify_info (private method)
*********************************************************************/

void sepLun::edit_lu_verify_info(fbe_lun_get_verify_status_t *lu_verify_status)
{
    // format drive information
    sprintf(temp,              "\n "
        "<Total Chunks>         %u\n "
        "<Marked Chunks>        %u\n ",
        lu_verify_status->total_chunks,
        lu_verify_status->marked_chunks);
}

/*********************************************************************
* sepLun class :: edit_lu_verify_report (private method)
*********************************************************************/

void sepLun::edit_lu_verify_report(fbe_lun_verify_report_t *lu_verify_report)
{
    // lun verify report information
    sprintf(temp,                                   "\n "
        "<Revision>                                 0x%X\n "
        "<Pass Count>                               0x%X\n "
        "<Mirror Objects Count>                     0x%X\n "
        "\n "
        "<Current Correctable Single Bit Crc>       0x%X\n "
        "<Current Uncorrectable Single Bit Crc>     0x%X\n "
        "<Current Correctable Multi Bit Crc>        0x%X\n "
        "<Current Uncorrectable Multi Bit Crc>      0x%X\n "
        "<Current Correctable Write Stamp>          0x%X\n "
        "<Current Uncorrectable Write Stamp         0x%X\n "
        "<Current Correctable Time Stamp>           0x%X\n "
        "<Current Uncorrectable Time Stamp>         0x%X\n "
        "<Current Correctable Shed Stamp>           0x%X\n "
        "<Current Uncorrectable Shed Stamp>         0x%X\n "
        "<Current Correctable Lba Stamp>            0x%X\n "
        "<Current Uncorrectable Lba Stamp>          0x%X\n "
        "<Current Correctable Coherency>            0x%X\n "
        "<Current Uncorrectable Coherency>          0x%X\n "
        "<Current Correctable Media>                0x%X\n "
        "<Current Uncorrectable Media>              0x%X\n "
        "<Current Correctable Soft Media>           0x%X\n "
        "\n "
        "<Previous Correctable Single Bit Crc>      0x%X\n "
        "<Previous Uncorrectable Single Bit Crc>    0x%X\n "
        "<Previous Correctable Multi Bit Crc>       0x%X\n "
        "<Previous Uncorrectable Multi Bit Crc>     0x%X\n "
        "<Previous Correctable Write Stamp>         0x%X\n "
        "<Previous Uncorrectable Write Stamp>       0x%X\n "
        "<Previous Correctable Time Stamp>          0x%X\n "
        "<Previous Uncorrectable Time Stamp>        0x%X\n "
        "<Previous Correctable Shed Stamp>          0x%X\n "
        "<Previous Uncorrectable Shed Stamp>        0x%X\n "
        "<Previous Correctable Lba Stamp>           0x%X\n "
        "<Previous Uncorrectable Lba Stamp>         0x%X\n "
        "<Previous Correctable Coherency>           0x%X\n "
        "<Previous Uncorrectable Coherency>         0x%X\n "
        "<Previous Correctable Media>               0x%X\n "
        "<Previous Uncorrectable Media>             0x%X\n "
        "<Previous Correctable Soft Media>          0x%X\n "
        "\n "
        "<Totals Correctable Single Bit Crc>        0x%X\n "
        "<Totals Uncorrectable Single Bit Crc>      0x%X\n "
        "<Totals Correctable Multi Bit Crc>         0x%X\n "
        "<Totals Uncorrectable Multi Bit Crc>       0x%X\n "
        "<Totals Correctable Write Stamp>           0x%X\n "
        "<Totals Uncorrectable Write Stamp>         0x%X\n "
        "<Totals Correctable Time Stamp>            0x%X\n "
        "<Totals Uncorrectable Time Stamp>          0x%X\n "
        "<Totals Correctable Shed Stamp>            0x%X\n "
        "<Totals Uncorrectable Shed Stamp>          0x%X\n "
        "<Totals Correctable Lba Stamp>             0x%X\n "
        "<Totals Uncorrectable Lba Stamp>           0x%X\n "
        "<Totals Correctable Coherency>             0x%X\n "
        "<Totals Uncorrectable Coherency>           0x%X\n "
        "<Totals Correctable Media>                 0x%X\n "
        "<Totals Uncorrectable Media>               0x%X\n "
        "<Totals Correctable Soft Media>            0x%X\n ",

        lu_verify_report->revision,
        lu_verify_report->pass_count,
        lu_verify_report->mirror_objects_count,

        lu_verify_report->current.correctable_single_bit_crc,
        lu_verify_report->current.uncorrectable_single_bit_crc,
        lu_verify_report->current.correctable_multi_bit_crc,
        lu_verify_report->current.uncorrectable_multi_bit_crc,
        lu_verify_report->current.correctable_write_stamp,
        lu_verify_report->current.uncorrectable_write_stamp,
        lu_verify_report->current.correctable_time_stamp,
        lu_verify_report->current.uncorrectable_time_stamp,
        lu_verify_report->current.correctable_shed_stamp,
        lu_verify_report->current.uncorrectable_shed_stamp,
        lu_verify_report->current.correctable_lba_stamp,
        lu_verify_report->current.uncorrectable_lba_stamp,
        lu_verify_report->current.correctable_coherency,
        lu_verify_report->current.uncorrectable_coherency,
        lu_verify_report->current.correctable_media,
        lu_verify_report->current.uncorrectable_media,
        lu_verify_report->current.correctable_soft_media,

        lu_verify_report->previous.correctable_single_bit_crc,
        lu_verify_report->previous.uncorrectable_single_bit_crc,
        lu_verify_report->previous.correctable_multi_bit_crc,
        lu_verify_report->previous.uncorrectable_multi_bit_crc,
        lu_verify_report->previous.correctable_write_stamp,
        lu_verify_report->previous.uncorrectable_write_stamp,
        lu_verify_report->previous.correctable_time_stamp,
        lu_verify_report->previous.uncorrectable_time_stamp,
        lu_verify_report->previous.correctable_shed_stamp,
        lu_verify_report->previous.uncorrectable_shed_stamp,
        lu_verify_report->previous.correctable_lba_stamp,
        lu_verify_report->previous.uncorrectable_lba_stamp,
        lu_verify_report->previous.correctable_coherency,
        lu_verify_report->previous.uncorrectable_coherency,
        lu_verify_report->previous.correctable_media,
        lu_verify_report->previous.uncorrectable_media,
        lu_verify_report->previous.correctable_soft_media,

        lu_verify_report->totals.correctable_single_bit_crc,
        lu_verify_report->totals.uncorrectable_single_bit_crc,
        lu_verify_report->totals.correctable_multi_bit_crc,
        lu_verify_report->totals.uncorrectable_multi_bit_crc,
        lu_verify_report->totals.correctable_write_stamp,
        lu_verify_report->totals.uncorrectable_write_stamp,
        lu_verify_report->totals.correctable_time_stamp,
        lu_verify_report->totals.uncorrectable_time_stamp,
        lu_verify_report->totals.correctable_shed_stamp,
        lu_verify_report->totals.uncorrectable_shed_stamp,
        lu_verify_report->totals.correctable_lba_stamp,
        lu_verify_report->totals.uncorrectable_lba_stamp,
        lu_verify_report->totals.correctable_coherency,
        lu_verify_report->totals.uncorrectable_coherency,
        lu_verify_report->totals.correctable_media,
        lu_verify_report->totals.uncorrectable_media,
        lu_verify_report->totals.correctable_soft_media );
}

/*********************************************************************
* sepLun class :: edit_get_lun_info (private method)
*********************************************************************/

void sepLun::edit_get_lun_info(
    fbe_api_lun_get_lun_info_t *lun_info)
{
    // format lun info 
    sprintf(temp,                   "\n "
        "<Peer Lifecycle State>     %s\n "
        "<Imported Capacity>        0x%llX\n "
        "<Offset>                   0x%llX\n ",
        utilityClass::lifecycle_state(
            lun_info->peer_lifecycle_state),
            (unsigned long long)lun_info->capacity,
            (unsigned long long)lun_info->offset);
}

/*********************************************************************
* sepLun::Sep_Provision_Intializing_variable (private method)
*********************************************************************/
void sepLun::Sep_Lun_Intializing_variable()
{
    fbe_zero_memory(&lu_zero_status,sizeof(lu_zero_status));
    fbe_zero_memory(&lu_verify_status,sizeof(lu_verify_status));
    fbe_zero_memory(&lu_verify_type,sizeof(lu_verify_type));
    fbe_zero_memory(&lu_verify_report,sizeof(lu_verify_report));
    fbe_zero_memory(&lu_destroy,sizeof(lu_destroy));
    fbe_zero_memory(&in_policy,sizeof(in_policy));
    fbe_zero_memory(&lu_update_struct,sizeof(lu_update_struct));
    fbe_zero_memory(&capacity_info,sizeof(capacity_info));
    fbe_zero_memory(&lu_info,sizeof(lu_info));
    fbe_zero_memory(&lu_rebuild_status,sizeof(lu_rebuild_status));
    fbe_zero_memory(&lu_bgverify_status,sizeof(lu_bgverify_status));
    fbe_zero_memory(&lun_info,sizeof(lun_info));
}

/*********************************************************************
* sepLun::get_lun_verify_type (private method)
*********************************************************************/
fbe_status_t sepLun::get_lun_verify_type(char **argv) 
{

    if(utilityClass::is_string(*argv) == FBE_TRUE)
    {
        if(strcmp(*argv, "USER_RW") == 0) {
            lu_verify_type = FBE_LUN_VERIFY_TYPE_USER_RW;
        } else if(strcmp(*argv, "USER_RO") == 0){
            lu_verify_type = FBE_LUN_VERIFY_TYPE_USER_RO;
        } else if(strcmp(*argv, "ERROR") == 0){
            lu_verify_type = FBE_LUN_VERIFY_TYPE_ERROR;
        } else if(strcmp(*argv, "INCOMPLETE_WRITE") == 0){
            lu_verify_type = FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE;
        } else if(strcmp(*argv, "SYSTEM") == 0){
            lu_verify_type = FBE_LUN_VERIFY_TYPE_SYSTEM;
        } else {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        return FBE_STATUS_OK;
    }

    lu_verify_type = (fbe_lun_verify_type_t)strtoul(*argv, 0, 0);

    return FBE_STATUS_OK;
}

/*********************************************************************
* sepLun end of file
*********************************************************************/
