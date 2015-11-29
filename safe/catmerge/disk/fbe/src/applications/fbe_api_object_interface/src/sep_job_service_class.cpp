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
*      This file defines the methods of the SEP JOB SERVICE
*      INTERFACE class.
*
*  Notes:
*      The SEP JOB SERVICE class (sepJobService) is a derived
*      class of the SEP base class (bSEP).
*
*  History:
*      26-Aug-2011: initial version
*
*********************************************************************/

#ifndef T_SEP_JOB_SERVICE_CLASS_H
#include "sep_job_service_class.h"
#endif

/*********************************************************************
* sepJobService Class :: sepJobService() (Constructor)
*********************************************************************
*  Description:
*       Initializes class variables
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/

sepJobService::sepJobService() : bSEP()
{
    // Store Object number
    idnumber = (unsigned) SEP_JOB_SERVICE_INTERFACE;
    sepJobServiceCount = ++gSepJobServiceCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "SEP_JOB_SERVICE_INTERFACE");
    
    fbe_zero_memory(&update_pvd_sniff_verify_state,sizeof(update_pvd_sniff_verify_state));
    pvd_object_id = FBE_OBJECT_ID_INVALID;
    sniff_verify_state = FBE_FALSE;
    state = 0;

    if (Debug) {
        sprintf(temp, "sepJobService class constructor (%d) %s\n",
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP sepJobService Interface function calls>
    sepJobServiceInterfaceFuncs =
        "[function call]\n" \
        "[short name]               [FBE API SEP JOB SERVICE Interface]\n" \
        "-----------------------------------------------------------------\n" \
        " updatePvdSniffVerify    fbe_api_job_service_update_pvd_sniff_verify\n" \
        " enablePVDSniffVerify    fbe_api_job_service_enable_pvd_sniff_verify\n" \
        " -----------------------------------------------------------------\n";
};

/*********************************************************************
* sepJobService Class :: MyIdNumIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id number of object
*  Input: Parameters
*      none
*  Returns:
*      unsigned int - id number
*********************************************************************/

unsigned sepJobService::MyIdNumIs(void)
{
    return idnumber;
};

/*********************************************************************
* sepJobService Class :: MyIdNameIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id name of object
*  Input: Parameters
*      none
*  Returns:
*      char* - id name
*********************************************************************/

char * sepJobService::MyIdNameIs(void)
{
    return idname;
};

/*********************************************************************
* sepJobService Class :: dumpme()
*********************************************************************
*  Description:
*       Dumps id number and sep job service count
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/

void sepJobService::dumpme(void)
{
    strcpy (key, "sepJobService::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n",
            idnumber, sepJobServiceCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* sepJobService Class :: select()
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
*      2011-13-06 : initial version
*
*********************************************************************/

fbe_status_t sepJobService::select(int index, int argc, char *argv[])
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface");
    c = index;

    // List SEP JOB SERVICE Interface calls if help option and
    // less than 6 arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepJobServiceInterfaceFuncs );
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // update the provision drive sniff verify state
    if (strcmp (argv[index], "UPDATEPVDSNIFFVERIFY") == 0) {
        status = update_pvd_sniff_verify(argc, &argv[index]);

     } else if (strcmp (argv[index], "ENABLEPVDSNIFFVERIFY") == 0) {
        status = enable_pvd_sniff_verify(argc, &argv[index]);

      // can't find match on short name
    }else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n",
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepJobServiceInterfaceFuncs );
    }

    return status;
}

/*********************************************************************
* sepJobService Class :: update_pvd_sniff_verify()
*********************************************************************
*
*  Description:
*      Update the provision drive sniff verify state 
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepJobService::update_pvd_sniff_verify(
    int argc,
    char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [pvd object id] "\
        "[enable sniff verify :1 | disable sniff verify : 0]\n"\
        " For example: %s 4 1\n";


    // Check that all arguments have been entered
    status = call_preprocessing(
        "updatePvdSniffVerify",
        "sepJobService::update_pvd_sniff_verify",
        "fbe_api_job_service_update_pvd_sniff_verify",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    
    // extract sniff verify state
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
    update_pvd_sniff_verify_state.sniff_verify_state = 
        sniff_verify_state; 

    // Make api call:
    status = fbe_api_job_service_update_pvd_sniff_verify(
        &update_pvd_sniff_verify_state);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't update sniff verify state for "
                      "provision drive 0x%X",
                      pvd_object_id);
    } else {
        sprintf(temp, "Sniff verify state set to %d "
            "for Provision Drive Obj ID 0x%x [job number 0x%llX]",
            state,
            pvd_object_id, 
            (unsigned long long)update_pvd_sniff_verify_state.job_number);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}


/*********************************************************************
* sepJobService Class :: enable_pvd_sniff_verify()
*********************************************************************
*
*  Description:
*      enables the provision drive sniff verify state 
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepJobService :: enable_pvd_sniff_verify(
    int argc, 
    char ** argv){

        fbe_status_t job_status;
        fbe_job_service_error_type_t                                js_error_type;

    // Define specific usage string
    usage_format =
        " Usage: %s [pvd object id] "\
        " For example: %s 4 \n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "enablePvdSniffVerify",
        "sepJobService::enable_pvd_sniff_verify",
        "fbe_api_job_service_enable_pvd_sniff_verify",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    pvd_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    sprintf(params, " pvd_object_id 0x%x " , 
                    pvd_object_id);

    // Make api call:
    status = fbe_api_job_service_set_pvd_sniff_verify(
        pvd_object_id, &job_status, &js_error_type, FBE_TRUE);

   // Check job status 
    if(job_status != FBE_STATUS_OK)
    {
        sprintf(temp, "Configuring PVD to enable sniff verify failed for "
                      "provision drive 0x%X",
                      pvd_object_id);
        // Output results from call or description of error
        call_post_processing(job_status, temp, key, params);
        return job_status;
    }


    // Check status of call

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
            sprintf(temp, "Can't enable sniff verify state for "
                      "provision drive 0x%X",
                      pvd_object_id);
    }

    if (status == FBE_STATUS_TIMEOUT)
    {
            sprintf(temp, "Timed out enabling sniff verify for "
                      "provision drive 0x%X",
                      pvd_object_id);
    }else {
        sprintf(temp, "Sniff verify enabled "
            "for Provision Drive Obj ID 0x%x ",
            pvd_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}



/*********************************************************************
* sepJobService end of file
*********************************************************************/