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
*      This file defines the methods of the SEP POWER SAVING
*      INTERFACE class.
*
*  Notes:
*      The SEP POWER SAVING class (sepPowerSaving) is a derived
*      class of the SEP base class (bSEP).
*
*  History:
*      13-Jun-2011: inital version
*
*********************************************************************/

#ifndef T_SEP_POWER_SAVING_CLASS_H
#include "sep_power_saving_class.h"
#endif

/*********************************************************************
* sepPowerSaving Class :: sepPowerSaving() (Constructor)
*********************************************************************
*  Description:
*       Initializes class variables
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/

sepPowerSaving::sepPowerSaving() : bSEP()
{
    // Store Object number
    idnumber = (unsigned) SEP_POWER_SAVING_INTERFACE;
    sepPowerSavingCount = ++gSepPowerSavingCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "SEP_POWER_SAVING_INTERFACE");

    Sep_Powersaving_Intializing_variable();

    if (Debug) {
        sprintf(temp, "sepPowerSaving class constructor (%d) %s\n",
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP sepPowerSaving Interface function calls>
    sepPowerSavingInterfaceFuncs =
        "[function call]\n" \
        "[short name]               [FBE API SEP POWER SAVING Interface]\n" \
        "-----------------------------------------------------------------\n" \
        " getSysPwrSaveInfo         fbe_api_get_system_power_save_info\n" \
        " enableSysPwrSave          fbe_api_enable_system_power_save\n" \
        " disableSysPwrSave         fbe_api_disable_system_power_save\n" \
        " -----------------------------------------------------------------\n" \
        " getObjPwrSaveInfo         fbe_api_get_object_power_save_info\n" \
        " enableObjPwrSave          fbe_api_enable_object_power_save\n" \
        " disableObjPwrSave         fbe_api_disable_object_power_save\n" \
        " setObjPwrSaveIdleTime     fbe_api_set_object_power_save_idle_time\n" \
        " -----------------------------------------------------------------\n" \
        " enableRgPwrSave           fbe_api_enable_raid_group_power_save\n" \
        " disableRgPwrSave          fbe_api_disable_raid_group_power_save\n" \
        " setRgPwrSaveIdleTime      fbe_api_set_raid_group_power_save_idle_time\n" \
        " -----------------------------------------------------------------\n" \
        " setPwrSavePeriodicdWakeup fbe_api_set_power_save_periodic_wakeup\n" \
        " -----------------------------------------------------------------\n";

    // Define common usage for SEP Power Saving commands
    usage_format =
        " Usage: %s [object id]\n" \
        " For example: %s 0x68";
};

/*********************************************************************
* sepPowerSaving Class :: MyIdNumIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id number of object
*  Input: Parameters
*      none
*  Returns:
*      unsigned int - id number
*********************************************************************/

unsigned sepPowerSaving::MyIdNumIs(void)
{
    return idnumber;
};

/*********************************************************************
* sepPowerSaving Class :: MyIdNameIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id name of object
*  Input: Parameters
*      none
*  Returns:
*      char* - id name
*********************************************************************/

char * sepPowerSaving::MyIdNameIs(void)
{
    return idname;
};

/*********************************************************************
* sepPowerSaving Class :: dumpme()
*********************************************************************
*  Description:
*       Dumps id number and sep power saving count
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/

void sepPowerSaving::dumpme(void)
{
    strcpy (key, "sepPowerSaving::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n",
            idnumber, sepPowerSavingCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* sepPowerSaving Class :: select()
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
*      2011-13-06 : inital version
*
*********************************************************************/

fbe_status_t sepPowerSaving::select(int index, int argc, char *argv[])
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface");
    c = index;

    // List SEP POWER SAVING Interface calls if help option and
    // less than 6 arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepPowerSavingInterfaceFuncs );
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get system power save info
    if (strcmp (argv[index], "GETSYSPWRSAVEINFO") == 0) {
        status = get_system_power_save_info(argc, &argv[index]);

        // enable system power save
    } else if (strcmp (argv[index], "ENABLESYSPWRSAVE") == 0) {
        status = enable_system_power_save(argc, &argv[index]);

        // disable system power save
    } else if (strcmp (argv[index], "DISABLESYSPWRSAVE") == 0) {
        status = disable_system_power_save(argc, &argv[index]);

        // get object power save info
    } else if (strcmp (argv[index], "GETOBJPWRSAVEINFO") == 0) {
        status = get_object_power_save_info(argc, &argv[index]);

        // enable object power save
    } else if (strcmp (argv[index], "ENABLEOBJPWRSAVE") == 0) {
        status = enable_object_power_save(argc, &argv[index]);
        // disable object power save
    } else if (strcmp (argv[index], "DISABLEOBJPWRSAVE") == 0) {
        status = disable_object_power_save(argc, &argv[index]);

        // set object power save idle time
    } else if (strcmp (argv[index], "SETOBJPWRSAVEIDLETIME") == 0) {
        status = set_object_power_save_idle_time(argc, &argv[index]);

        // enable raid group power save
    } else if (strcmp (argv[index], "ENABLERGPWRSAVE") == 0) {
        status = enable_raid_group_power_save(argc, &argv[index]);

        // disable raid group power save
    } else if (strcmp (argv[index], "DISABLERGPWRSAVE") == 0) {
        status = disable_raid_group_power_save(argc, &argv[index]);

        // set raid group power save idle time
    } else if (strcmp (argv[index], "SETRGPWRSAVEIDLETIME") == 0) {
        status = set_raid_group_power_save_idle_time(
            argc, 
            &argv[index]);

        // set power save periodic wakeup
    } else if (strcmp (argv[index], "SETPWRSAVEPERIODICDWAKEUP") == 0) {
        status = set_power_save_periodic_wakeup(argc, &argv[index]);

        // can't find match on short name
    }else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n",
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepPowerSavingInterfaceFuncs );
    }

    return status;
}

/*********************************************************************
* sepPowerSaving Class :: get_system_power_save_info()
*********************************************************************
*
*  Description:
*      Gets the system power information
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepPowerSaving::get_system_power_save_info(
    int argc,
    char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s\n"\
        " For example: %s\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getSysPwrSaveInfo",
        "sepPowerSaving::get_system_power_save_info",
        "fbe_api_get_system_power_save_info",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Make api call:
    status = fbe_api_get_system_power_save_info(&system_psi);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get system power save info");
    } else {
        edit_system_power_save_info(&system_psi);
    }

    params[0] = '\0';

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepPowerSaving Class :: enable_system_power_save()
*********************************************************************
*
*  Description:
*      Enable system power save
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepPowerSaving::enable_system_power_save(
    int argc ,
    char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s\n"\
        " For example: %s\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "enableSysPwrSave",
        "sepPowerSaving::enable_system_power_save",
        "fbe_api_enable_system_power_save",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Make api call:
    status = fbe_api_enable_system_power_save();

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't enable system power save");
    } else {
        sprintf(temp, "Enabled system power save");
    }

    params[0] = '\0';

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepPowerSaving Class :: disable_system_power_save()
*********************************************************************
*
*  Description:
*      Disable system power save
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepPowerSaving::disable_system_power_save(
    int argc ,
    char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s\n"\
        " For example: %s\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "disableSysPwrSave",
        "sepPowerSaving::disable_system_power_save",
        "fbe_api_disable_system_power_save",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Make api call:
    status = fbe_api_disable_system_power_save();

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't disable system power save");
    } else {
        sprintf(temp, "Disabled system power save");
    }

    params[0] = '\0';

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepPowerSaving Class :: get_object_power_save_info()
**********************************************************************
*
*  Description:
*      Get the power saving information about an object (LUN, RG or PVD object)
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepPowerSaving::get_object_power_save_info (
    int argc ,
    char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getObjPwrSave",
        "sepPowerSaving::get_object_power_save_info",
        "fbe_api_get_object_power_save_info",
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
    status = fbe_api_get_object_power_save_info(object_id, &object_psi);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get object power save info "\
                "for object id 0x%x", object_id);
    } else {
        edit_object_power_save_info(&object_psi);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepPowerSaving Class :: enable_raid_group_power_save()
*********************************************************************
*
*  Description:
*      Enable raid group power save
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepPowerSaving::enable_raid_group_power_save(
    int argc,
    char **argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "enableRgPwrSave",
        "sepPowerSaving::enable_raid_group_power_save",
        "fbe_api_enable_raid_group_power_save",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    rg_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " rg object id 0x%x (%s)", rg_object_id, *argv);

    // Make api call:
    status = fbe_api_enable_raid_group_power_save(rg_object_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't enable rg power save "\
                "for object id 0x%x", rg_object_id);
    }
    else {
        sprintf(temp, "Enabled raid group power save "\
                      "for object id 0x%x", rg_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepPowerSaving Class :: Disable_raid_group_power_save()
*********************************************************************
*
*  Description:
*      Disable raid group power save
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepPowerSaving::disable_raid_group_power_save(
    int argc,
    char **argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "disableRgPwrSave",
        "sepPowerSaving::disable_raid_group_power_save",
        "fbe_api_disable_raid_group_power_save",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    rg_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " rg object id 0x%x (%s)", rg_object_id, *argv);

    // Make api call:
    status = fbe_api_disable_raid_group_power_save(rg_object_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't disable rg power save "\
                "for object id 0x%x", rg_object_id);
    }
    else {
        sprintf(temp, "Disabled raid group power save "\
                      "for object id 0x%x", rg_object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepPowerSaving Class :: set_raid_group_power_save_idle_time()
*********************************************************************
*
*  Description:
*      Set raid group power save idle time
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepPowerSaving::set_raid_group_power_save_idle_time(
    int argc,
    char **argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [RG object id] [idle time in seconds]\n"\
        " For example: %s 4 1000";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setRgPwrSaveIdleTime",
        "sepPowerSaving::set_raid_group_power_save_idle_time",
        "fbe_api_set_raid_group_power_save_idle_time",
        usage_format,
        argc, 8);

     // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    // extract idle time
    argv++;
    idle_time_seconds = (fbe_u64_t)_strtoi64(*argv, 0, 0);

    sprintf(params, "object id 0x%x (%u) "\
            "object ps idle time seconds 0x%llx (%llu)",
            object_id, object_id,
            (unsigned long long)idle_time_seconds,
	    (unsigned long long)idle_time_seconds);

    // Make api call: 
    status = fbe_api_set_raid_group_power_save_idle_time(
        object_id, 
        idle_time_seconds);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to set rg power saving idle time: "\
                      "%llu for Object ID: 0x%x",
                      (unsigned long long)idle_time_seconds, object_id);
    }
    else {
        sprintf(temp, "Power Save Idle time of %llu secs set "\
                      "for RG object id 0x%x",
                      (unsigned long long)idle_time_seconds, object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepPowerSaving Class :: set_power_save_periodic_wakeup()
*********************************************************************
*
*  Description:
*      Sets the hibernation wakeup time for system
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepPowerSaving::set_power_save_periodic_wakeup(
    int argc,
    char **argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [minutes]\n"\
        " For example: %s 10";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setPwrSavePeriodicdWakeup",
        "sepPowerSaving::set_power_save_periodic_wakeup",
        "fbe_api_set_power_save_periodic_wakeup",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // extract minutes
    argv++;
    wakeup_minutes = (fbe_u64_t)_strtoi64(*argv, 0, 0);
    sprintf(params, "Power Save periodic wakeup minutes %llu (%s)",
            (unsigned long long)wakeup_minutes, *argv);

    // Make api call:
    status = fbe_api_set_power_save_periodic_wakeup(wakeup_minutes);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't set power save periodic wakeup");
    }
    else {
        sprintf(temp, "Power save periodic wakeup set to %llu minutes",
                (unsigned long long)wakeup_minutes);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepPowerSaving Class :: enable_object_power_save()
*********************************************************************
*
*  Description:
*      Enables power save for object except RAID and LUN objects.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepPowerSaving::enable_object_power_save(
    int argc,
    char **argv)
{
    // Define specific usage stirng
    usage_format =
        " Usage: %s [PVD/VD object id]\n" \
        " For example: %s 0x68";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "enableObjPwrSave",
        "sepPowerSaving::enable_object_power_save",
        "fbe_api_enable_object_power_save",
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
    status = fbe_api_enable_object_power_save(object_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't enable object power save "\
                      "for object id 0x%x", object_id);
    }
    else {
        sprintf(temp, "Enabled object power save "\
                      "for object id 0x%x", object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepPowerSaving Class :: disable_object_power_save()
*********************************************************************
*
*  Description:
*      Disables power save for object except RAID and LUN objects
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepPowerSaving::disable_object_power_save(
    int argc,
    char **argv)
{
    // Define specific usage stirng
    usage_format =
        " Usage: %s [PVD/VD object id]\n" \
        " For example: %s 0x68";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "disableObjPwrSave",
        "sepPowerSaving::disable_object_power_save",
        "fbe_api_disable_object_power_save",
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
    status = fbe_api_disable_object_power_save(object_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't disable object power save "\
                      "for object id 0x%x", object_id);
    }
    else {
        sprintf(temp, "Disabled object power save "\
                      "for object id 0x%x", object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepPowerSaving Class :: set_object_power_save_idle_time()
*********************************************************************
*
*  Description:
*      Sets the power saving idle time for VD and PVD objects
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepPowerSaving::set_object_power_save_idle_time(
    int argc,
    char **argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [PVD / VD object id] [idle time in seconds]\n"\
        " For example: %s 4 1000";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setObjPwrSaveIdleTime",
        "sepPowerSaving::set_object_power_save_idle_time",
        "fbe_api_set_object_power_save_idle_time",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    // extract idle time
    argv++;
    idle_time_seconds = (fbe_u64_t)_strtoi64(*argv, 0, 0);

    sprintf(params, "object id 0x%x (%u) "\
            "object ps idle time seconds 0x%llx (%llu)",
            object_id, object_id,
            (unsigned long long)idle_time_seconds,
	    (unsigned long long)idle_time_seconds);

    // Make api call: 
    status = fbe_api_set_object_power_save_idle_time(
        object_id, 
        idle_time_seconds);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to set the power saving idle time: "\
                      "%llu for Object ID: 0x%x",
                      (unsigned long long)idle_time_seconds, object_id);
    }
    else {
        sprintf(temp, "Set object power save idle time for"\
                      " object id 0x%x to %llu",
                      object_id, (unsigned long long)idle_time_seconds);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepPowerSaving Class :: edit_system_power_save_info()
                          (private method)
*********************************************************************
*
*  Description:
*      Format the system power save information to display
*
*  Input: Parameters
*      power_save_info  - system power save info structure

*  Returns:
*      void
*********************************************************************/

void sepPowerSaving::edit_system_power_save_info(
    fbe_system_power_saving_info_t *power_save_info)
{
    // system power save information
    sprintf(temp,                               "\n "
        "<Enabled>                              %d\n "
        "<Hibernation Wake Up Time In Minutes>  %llu\n ",
        power_save_info->enabled,
        (unsigned long long)power_save_info->hibernation_wake_up_time_in_minutes);
}

/*********************************************************************
* sepPowerSaving Class :: edit_object_power_save_info()
                          (private method)
*********************************************************************
*
*  Description:
*      Format the object power save information to display
*
*  Input: Parameters
*      power_save_info  - object power save info structure

*  Returns:
*      void
*********************************************************************/

void sepPowerSaving::edit_object_power_save_info(
    fbe_base_config_object_power_save_info_t *power_save_info)
{
    // object power save information
    sprintf(temp,                           "\n "
        "<Seconds Since Last IO>            0x%X\n "
        "<Power Saving Enabled>             %d\n "
        "<Configured Idle Time In Seconds>  %llu\n "
        "<Power Save State>                 0x%X\n "
        "<Hibernation Time>                 %llu\n "
        "<Flags>                            %llu\n ",
        power_save_info->seconds_since_last_io,
        power_save_info->power_saving_enabled,
        (unsigned long long)power_save_info->configured_idle_time_in_seconds,
        power_save_info->power_save_state,
        (unsigned long long)power_save_info->hibernation_time,
        (unsigned long long)power_save_info->flags);
}

/*********************************************************************
* sepPowerSaving::Sep_Provision_Intializing_variable (private method)
*********************************************************************/
void sepPowerSaving::Sep_Powersaving_Intializing_variable()
{
        object_id = FBE_OBJECT_ID_INVALID;
        wakeup_minutes = 0;
         idle_time_seconds = 0;
        params_temp[0] = '\0';
        fbe_zero_memory(&system_psi,sizeof(system_psi));
        fbe_zero_memory(&object_psi,sizeof(object_psi)); 

}

/*********************************************************************
* sepPowerSaving end of file
*********************************************************************/
