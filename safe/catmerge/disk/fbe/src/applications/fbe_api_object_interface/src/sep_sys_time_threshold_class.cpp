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
*      SEP System Time Threshold Interface class.
*
*  Notes:
*      The SEP System Time Threshold class (sepSysTimeThreshold) is
*      a derived class of the base class (bSEP) and with the purpose
*      to set system time parameters.
*
*  Created By : Eugene Shubert
*  History:
*      July 11, 2012 : Initial version
*
*********************************************************************/

#ifndef T_SEP_SYS_TIME_THRESHOLD_CLASS_H
#include "sep_sys_time_threshold_class.h"
#endif

/*********************************************************************
* sepSysTimeThreshold class :: Constructor
*********************************************************************
*  Description:
*      Initializes class variables
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/
sepSysTimeThreshold::sepSysTimeThreshold() : bSEP()
{
    // Set Object ID:
    idnumber = (unsigned) SEP_SYS_TIME_THRESHOLD_INTERFACE;
    // Set count of Objects:
    sepSysTimeThresholdCount = ++gSepBaseConfigCount;
    // Set Object name
    interfaceName = new char [IDNAME_LENGTH];
    strcpy(interfaceName, "SEP_SYS_TIME_THRESHOLD_INTERFACE");

    if (Debug) {
        sprintf(temp,
            "sepSysTimeThreshold class constructor (%d) %s\n",
            idnumber, interfaceName);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP Sys Time Threshold function calls>
    sepSysTimeThresholdFuncs =
        "[function call]\n" \
        "[short name] [FBE API SEP System Time Interface]\n" \
        " --------------------------------------------------------\n" \
        " setSysTimeThreshold fbe_api_set_system_time_threshold_info\n" \
        " getSysTimeThreshold fbe_api_get_system_time_threshold_info\n" \
        " --------------------------------------------------------\n";

    // Define common usage for the commands
    usage_format =
        " Usage: %s\n"
        " For example: %s\n";
};

/*********************************************************************
 * sepSysTimeThreshold class : MyIdNumIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id number of object
*  Input: Parameters
*      none
*  Returns:
*      unsigned int - id number
*********************************************************************/

unsigned sepSysTimeThreshold::MyIdNumIs(void)
{
    return idnumber;
}

/*********************************************************************
*  sepSysTimeThreshold class  :: MyIdNameIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id name of object
*  Input: Parameters
*      none
*  Returns:
*      char* - id name
*********************************************************************/
char * sepSysTimeThreshold::MyIdNameIs(void)
{
    return interfaceName;
}

/*********************************************************************
* sepSysTimeThreshold class  :: dumpme()
*********************************************************************
*  Description:
*       Dumps id number and sep power saving count
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/
void sepSysTimeThreshold::dumpme(void)
{
    strcpy (key, "sepSysTimeThreshold::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n",
            idnumber, sepSysTimeThresholdCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* sepSysTimeThreshold Class :: select()
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
*********************************************************************/

fbe_status_t sepSysTimeThreshold::select(int index, int argc, char *argv[])
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface");

    // List sep Base Config calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepSysTimeThresholdFuncs );

        // Set System Time Threshold (in minutes):
    }else if (strcmp (argv[index], "SETSYSTIMETHRESHOLD") == 0) {
        status = set_sys_time_threshold(argc, &argv[index]);

    // Get System Time Threshold(in minutes)
    }else if (strcmp (argv[index], "GETSYSTIMETHRESHOLD") == 0) {
        status = get_sys_time_threshold(argc, &argv[index]);

    // can't find match on short name
    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n",
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepSysTimeThresholdFuncs );
    }

    return status;
}

/*********************************************************************
* sepSysTimeThreshold class :: set_sys_time_threshold ()
**********************************************************************
*  Description:
*      Sets System Time Threshold.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - either FBE_STATUS_OK, or
*      any other status code from fbe_types.h.
*
*********************************************************************/
fbe_status_t
sepSysTimeThreshold::set_sys_time_threshold(int argc, char** argv)
{
    usage_format =
        " Usage: %s [time in minutes]\n"
        " For example: %s 5\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setSysTimeThreshold",
        "sepSysTimeThreshold::set_sys_time_threshold",
        "fbe_api_set_system_time_threshold_info",
        usage_format,
        argc, 7);

    // Return if arguments were wrong or user specified help option
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Set time structure:
    argv++;
    time_threshold_info.time_threshold_in_minutes =
            (fbe_u64_t)_strtoui64(*argv, 0, 0);

    sprintf(params, "System Time Threshold %s mins", *argv);

    char *data = temp;

    // Make API call:
    status = fbe_api_set_system_time_threshold_info(&time_threshold_info);

    // Check status of call
    if (status == FBE_STATUS_OK) {
        sprintf(data, "Successfully set System Time Threshold to %s mins.", 
            *argv);
    } else {
        sprintf(data, "Failed to set System Time Threshold to %s mins.",
            *argv);
    }

    // Output results from call or description of error
    call_post_processing(status, data, key, params);
    return status;
}

/*********************************************************************
* sepSysTimeThreshold class :: get_sys_time_threshold ()
**********************************************************************
*  Description:
*      Gets System Time Threshold.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - arguments
*
*  Returns:
*      status - either FBE_STATUS_OK, or
*      any other status code from fbe_types.h.
*
*********************************************************************/
fbe_status_t
sepSysTimeThreshold::get_sys_time_threshold(int argc, char** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getSysTimeThreshold",
        "sepSysTimeThreshold::get_sys_time_threshold",
        "fbe_api_get_system_time_threshold_info",
        usage_format,
        argc, 6);

    // Return if arguments were wrong or user specified help option
    if (status != FBE_STATUS_OK) {
        return status;
    }

    sprintf(params, " ");

    char *data = temp;

    // Make API call:
    status = fbe_api_get_system_time_threshold_info(&time_threshold_info);

    // Check status of call
    if (status == FBE_STATUS_OK) {
        sprintf(data, "<System Time Threshold> %llx minutes.",
            time_threshold_info.time_threshold_in_minutes);
    } else {
        sprintf(data, "Failed to get System Time Threshold.");
    }

    // Output results from call or description of error
    call_post_processing(status, data, key, params);
    return status;
}
