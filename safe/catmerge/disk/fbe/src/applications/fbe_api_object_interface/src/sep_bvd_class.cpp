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
 *      This file defines the methods of the SEP BVD INTERFACE class.
 *
 *  Notes:
 *      The SEP BVD class (sepBvd) is a derived class of
 *      the base class (bSEP).
 *
 *  History:
 *      27-June-2011 : initial version
 *
 *********************************************************************/

#ifndef T_SEP_BVD_CLASS_H
#include "sep_bvd_class.h"
#endif

/*********************************************************************
* sepBVD Class :: sepBVD() (Constructor)
*********************************************************************
*  Description:
*       Initializes class variables
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/

sepBVD::sepBVD() : bSEP()
{
    // Store Object number
    idnumber = (unsigned) SEP_BVD_INTERFACE;
    sepBvdCount = ++gSepBvdCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "SEP_BVD_INTERFACE");

    //Initializing variables
    void sep_bvd_initializing_variable();

    
    if (Debug) {
        sprintf(temp, "sepBVD class constructor (%d) %s\n",
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP sepBVD Interface function calls>
    sepBvdInterfaceFuncs=
        " [function call]\n" \
        " [short name]         [FBE API BVD Interface]\n"\
        " -------------------------------------------------------------------------\n"\
        " getObjID             fbe_api_bvd_interface_get_bvd_object_id\n"\
        " -------------------------------------------------------------------------\n"\
/*        " getIOStat            fbe_api_bvd_interface_get_io_statistics\n"\
        " -------------------------------------------------------------------------\n"\ */
        " getPerfStat          fbe_api_bvd_interface_get_peformance_statistics\n" \
        " enablePerfStat       fbe_api_bvd_interface_enable_peformance_statistics\n"\
        " disablePerfStat      fbe_api_bvd_interface_disable_peformance_statistics\n"\
        " clearPerfStat        fbe_api_bvd_interface_clear_peformance_statistics\n"\
        " -------------------------------------------------------------------------\n";
//        " getDownstreamAttr    fbe_api_bvd_interface_get_downstream_attr\n"\
//        " -------------------------------------------------------------------------\n";

    // Define common usage for SEP BVD commands
    usage_format =
        " Usage: %s\n"
        " For example: %s\n";

}

/*********************************************************************
* sepBVD Class :: MyIdNumIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id number of object
*  Input: Parameters
*      none
*  Returns:
*      unsigned int - id number
*********************************************************************/

unsigned sepBVD::MyIdNumIs(void)
{
    return idnumber;
};

/*********************************************************************
* sepBVD Class :: MyIdNameIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id name of object
*  Input: Parameters
*      none
*  Returns:
*      char* - id name
*********************************************************************/

char * sepBVD::MyIdNameIs(void)
{
    return idname;
};

/*********************************************************************
* sepBVD Class :: dumpme()
*********************************************************************
*  Description:
*       Dumps id number and sep BVD count
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/

void sepBVD::dumpme(void)
{
    strcpy (key, "sepBVD::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n",
       idnumber, sepBvdCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* sepBVD Class :: select(int index, int argc, char * argv [ ])
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
fbe_status_t sepBVD::select (int index, int argc, char * argv [ ])
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface");

    // Display all SEP BVD functions and return generic error when
    // the argument count is less than 6 and no help command executed.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepBvdInterfaceFuncs);
        return status;
    }

    if (strcmp(argv[2], "-A") == 0){
        strcpy(spid ,"A");
    }
    else if (strcmp(argv[2], "-B") == 0) {
        strcpy(spid ,"B");
    }
    
    // get performance statistics associated with BVD object
    if (strcmp (argv[index], "GETPERFSTAT") == 0) {
        status = get_bvd_performance_statistics(argc, &argv[index]);

    // enable performance statistics associated with BVD object
    } else if (strcmp (argv[index], "ENABLEPERFSTAT") == 0) {
        status = enable_bvd_performance_statistics(argc, &argv[index]);

    // disable performance statistics associated with BVD object
    } else if (strcmp (argv[index], "DISABLEPERFSTAT") == 0) {
        status = disable_bvd_performance_statistics(argc, &argv[index]);

    // clear performance statistics associated with BVD object
    } else if (strcmp (argv[index], "CLEARPERFSTAT") == 0) {
        status = clear_bvd_performance_statistics(argc, &argv[index]);

    // get BVD object id
    } else if (strcmp (argv[index], "GETOBJID") == 0) {
        status =  get_bvd_object_id(argc, &argv[index]);

    // get IO statistics associated with BVD object
    } else if (strcmp (argv[index], "GETIOSTAT") == 0) {
       // status =  get_bvd_io_statistics(argc, &argv[index]);

    // can't find match on short name
    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n",
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepBvdInterfaceFuncs );
    }

    return status;

}

/*********************************************************************
* sepBVD Class :: get_bvd_performance_statistics(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Gets the performace statistics for the BVD object.
*
*  Input: Parameters
*      argc  - argument count
*      **argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*
*  History
*      28-Jun-2011 : initial version
*********************************************************************/

fbe_status_t sepBVD::get_bvd_performance_statistics(
    int argc, char ** argv)
{

    char *data;

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getPerfStat",
        "sepBVD::get_bvd_performance_statistics",
        "fbe_api_bvd_interface_get_peformance_statistics",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Make api call:
    status = fbe_api_bvd_interface_get_peformance_statistics(
        &bvd_perf_stats);

    // Check status of call
     if (status == FBE_STATUS_OK)
    {
        data = edit_perf_stats_sp_result(&bvd_perf_stats);
    } else {
        sprintf(temp,"Could not get Performance statistics for SP%s.\n",spid);
        data = temp;
    }

    params[0] = '\0';

    // Output results from call or description of error
    call_post_processing(status, data, key, params);

    return status;

}

/*********************************************************************
* sepBVD Class::enable_bvd_performance_statistics(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Enables the performace statistics for the BVD object.
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
*      28-Jun-2011 : initial version
*********************************************************************/

fbe_status_t sepBVD::enable_bvd_performance_statistics(
    int argc, char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "enablePerfStat",
        "sepBVD::enable_bvd_performance_statistics",
        "fbe_api_bvd_interface_enable_peformance_statistics",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Make api call:
    status = fbe_api_bvd_interface_enable_peformance_statistics();

    if (status == FBE_STATUS_OK) {
        sprintf(temp,"Performance statistics on SP%s is enabled.",spid);

    } else {
        sprintf(temp,"Performance statistics on SP%s is not enabled.",spid);
    }

    params[0] = '\0';

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;

}

/*********************************************************************
* sepBVD Class :: disable_bvd_performance_statistics(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Disables the performace statistics for the BVD object.
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
*      30-Jun-2011 : initial version
*********************************************************************/

fbe_status_t sepBVD::disable_bvd_performance_statistics(
    int argc, char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "disablePerfStat",
        "sepBVD::disable_bvd_performance_statistics",
        "fbe_api_bvd_interface_disable_peformance_statistics",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Make api call:
    status = fbe_api_bvd_interface_disable_peformance_statistics();

    if (status == FBE_STATUS_OK) {
        sprintf(temp,"Performance statistics on SP%s is disabled.\n ",spid);
    } else {
        sprintf(temp,"Performance statistics on SP%s is not disabled.\n",spid);
    }

    params[0] = '\0';

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;

}

/*********************************************************************
* sepBVD Class :: clear_bvd_performance_statistics(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Clears the performace statistics for the BVD object.
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
*      02-Sep-2011 : initial version
*********************************************************************/

fbe_status_t sepBVD::clear_bvd_performance_statistics(
    int argc, char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "clearPerfStat",
        "sepBVD::clear_bvd_performance_statistics",
        "fbe_api_bvd_interface_clear_peformance_statistics",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Make api call:
    status = fbe_api_bvd_interface_clear_peformance_statistics();

    if (status == FBE_STATUS_OK) {
        sprintf(temp,"Cleared performance statistics on SP%s.\n ",spid);
    } else {
        sprintf(temp,"Unable to clear performance statistics on SP%s.\n",spid);
    }

    params[0] = '\0';

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;

}

/*********************************************************************
* sepBVD Class :: get_bvd_object_id(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Gets the BVD object ID.
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
*      30-Jun-2011 : initial version
*********************************************************************/

fbe_status_t sepBVD::get_bvd_object_id(int argc,  char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getObjId",
        "sepBVD::get_bvd_object_id",
        "fbe_api_bvd_interface_get_bvd_object_id",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Make api call:
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_id);

    // Check status of call

     if (status == FBE_STATUS_OK)
    {
        sprintf(temp, " <bvd_id>     %d\n", bvd_id);
    } else {
        sprintf(temp,"Failed to get BVD ID\n");
    }

    params[0] = '\0';

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;

}

/*********************************************************************
* sepBVD Class ::sep_bvd_initializing_variable(private method)
*********************************************************************/
void sepBVD :: sep_bvd_initializing_variable()
{
    bvd_id = 0;
    spid[0] = '\0';
    fbe_zero_memory(&bvd_perf_stats,sizeof(bvd_perf_stats));
}



/*********************************************************************
* sepBVD Class :: get_bvd_io_statistics(int argc,   char ** argv)
*********************************************************************
*
*  Description:
*      Gets the IO statistics for the BVD object.
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
*      30-Jun-2011 : initial version
*********************************************************************/
#if 0
fbe_status_t sepBVD::get_bvd_io_statistics(int argc,   char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getIOStat",
        "sepBVD::get_bvd_io_statistics",
        "fbe_api_bvd_interface_get_io_statistics",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Make api call:
    status = fbe_api_bvd_interface_get_io_statistics(&bvd_stat);

    // Check status of call

     if (status == FBE_STATUS_OK)
    {
        edit_io_stats_sp_result(&bvd_stat);
    } else {
        sprintf(temp,"Can't get stat from BVD object.\n ");
    }

    params[0] = '\0';

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}
#endif
// Uncomment it once it is determined to expose this API
///*********************************************************************
//* sepBVD Class :: get_bvd_downstream_attribute(int argc, char ** argv)
//*********************************************************************
//*
//*  Description:
//*      Gets the downstream attribute for the BVD object.
//*
//*  Input: Parameters
//*      argc  - argument count
//*      *argv - pointer to arguments
//*
//*  Returns:
//*      status - FBE_STATUS_OK or one of the other status codes.
//*               See fbe_types.h
//*
//*  History
//*      30-Jun-2011 : initial version
//*********************************************************************/
//fbe_status_t sepBVD::get_bvd_downstream_attribute(int argc, char ** argv)
//{
//    fbe_lun_number_t lun_number;
//    fbe_u32_t                           current_time = 0;
//    // Define specific usage string
//    usage_format =
//        " Usage: %s [Lun number]\n"\
//        " For example: %s 1\n";
//
//    // Check that all arguments have been entered
//    status = call_preprocessing(
//        "getDownstreamAttr",
//        "sepBVD::get_bvd_downstream_attribute",
//        "fbe_api_bvd_interface_get_downstream_attr",
//        usage_format,
//        argc, 7);
//
//    // Return if error found or help option entered
//    if (status != FBE_STATUS_OK) {
//        return status;
//    }
//
//    argv++;
//    lun_number = atoi(*argv);
//    sprintf(params, " Lun number  %d", lun_number);
//
//    //Get the BVD object id
//    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_id);
//    if (status != FBE_STATUS_OK)
//    {
//        sprintf(temp,"Failed to get BVD ID\n");
//    }
//    else
//    {
//        // Make api call:
//        status = fbe_api_bvd_interface_get_downstream_attr(
//            bvd_id, &downstream_attr_p,  lun_number);
//
//        if (status != FBE_STATUS_OK) {
//            sprintf(temp,"Failed to get BVD object's downstream attribute field.\n");
//        }
//        else {
//            sprintf(temp,"\n <Attribute>     %d\n",downstream_attr_p);
//        }
//    }
//
//    // Output results from call or description of error
//    call_post_processing(status, temp, key, params);
//    return status;
//}

/*********************************************************************
* sepBVD Class :: edit_perf_stats_sp_result()
                          (private method)
*********************************************************************
*
*  Description:
*      Format the BVD object information to display
*
*  Input: Parameters
*      bvd_perf_stat  - BVD performance statistics structure
*
*  Returns:
*      void
*
*  History
*      28-Jun-2011 : initial version
*********************************************************************/

char*  sepBVD::edit_perf_stats_sp_result(
    fbe_sep_shim_get_perf_stat_t* perf_stats_p)
{

    fbe_cpu_id_t        core_count = fbe_get_cpu_count();
    fbe_cpu_id_t        current_core;
    char *data = NULL;
    char *buffer = NULL;

    for (current_core = 0; current_core < core_count; current_core++) {

        buffer = new char[15];

        sprintf (buffer, "Core %d", current_core);

        sprintf (temp,                              "\n "
            "<%s Current IOs in progress>           0x%llX\n "
            "<%s Max concurrent IOs>                0x%llX\n "
            "<%s Total IOs since last stats reset>  0x%llX\n ",
            buffer, (unsigned long long)perf_stats_p->core_stat[current_core].current_ios_in_progress,
            buffer, (unsigned long long)perf_stats_p->core_stat[current_core].max_io_q_depth,
            buffer, (unsigned long long)perf_stats_p->core_stat[current_core].total_ios);

        data = utilityClass::append_to (data, temp);

        delete [] buffer;

    }

return data;

}

/*********************************************************************
* sepBVD Class :: edit_io_stats_sp_result(fbe_bvd_io_stat_t* bvd_stat)
                  (private method)
*********************************************************************
*
*  Description:
*      Format the BVD object information to display
*
*  Input: Parameters
*      bvd_stat  - BVD IO statistics structure
*
*  Returns:
*      void
*
*  History
*      30-Jun-2011 : initial version
*********************************************************************/
#if 0
void  sepBVD::edit_io_stats_sp_result( fbe_bvd_io_stat_t* bvd_stat)
{
    sprintf(temp,              "\n "
        "<current allocated packets>    %lld\n "
        "<Max packet pool>              %lld\n "
        "<Out of memory allocations>    %ul\n",
         bvd_stat->sep_shim_stat.current_allocated_packets,
         bvd_stat->sep_shim_stat.max_packet_pool,
         bvd_stat->sep_shim_stat.out_of_memory_allocations);

    return;
}
#endif
