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
 *      This file defines the methods of the PHYSICAL ENCLOSURE INTERFACE Class.
 *
 *  Notes:
 *      The Physical Enclosure Class (PhyEncl) is a derived class of 
 *      the base class (bPhysical).
 *
 *  History:
 *      18-July-2011 : inital version
 *
 *********************************************************************/

#ifndef T_PHY_ENCL_CLASS_H
#include "phy_encl_class.h"
#endif

/*********************************************************************
* PhyEncl::PhyEncl() (Constructor)
*********************************************************************
*  Description:
*       Initializes class variables
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/
PhyEncl::PhyEncl() : bPhysical()
{
    // Store Object number
    idnumber = (unsigned) PHY_ENCL_INTERFACE;
    PhyEnclCount = ++gPhysEnclCount;;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "PHY_ENCL_INTERFACE");

    //initializing variables
    phy_encl_initializing_variables();

    
    if (Debug) {
        sprintf(temp, 
            "PhyEncl class constructor (%d) %s\n", 
            idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API Physical Enclosure Interface function calls>
    PhyEnclInterfaceFuncs = 
        "[function call]\n" \
        "[short name] [FBE API Physical Enclosure Interface]\n" \
        " ------------------------------------------------------------\n" \
        " getEnclObjId          fbe_api_get_enclosure_object_id_by_location\n" \
        " getEnclType           fbe_api_enclosure_get_encl_type\n"\
        " getEnclDriveSlotInfo  fbe_api_enclosure_get_drive_slot_info\n"\
        " getEnclFaninfo        fbe_api_enclosure_get_fan_info\n"\
        " ------------------------------------------------------------\n" \
        " sendDriveDebugCtrl    fbe_api_enclosure_send_drive_debug_control\n" \
        " sendDrivePowerCtrl    fbe_api_enclosure_send_drive_power_control\n" \
        " sendExpanderCtrl      fbe_api_enclosure_send_expander_control\n" \
        " sendExpStringOutCtrl  fbe_api_enclosure_send_exp_string_out_control\n"\
        " sendExpTestModeCtrl   fbe_api_enclosure_send_exp_test_mode_control\n"\
        " ------------------------------------------------------------\n" \
        " getScsiErrorInfo      fbe_api_enclosure_get_scsi_error_info\n" \
        " ------------------------------------------------------------\n";

};

 /*********************************************************************
* PhyEncl Class :: MyIdNumIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id number of object
*  Input: Parameters
*      none
*  Returns:
*      unsigned int - id number
*********************************************************************/
unsigned PhyEncl::MyIdNumIs(void)
{
    return idnumber;
};

/*********************************************************************
* PhyEncl Class :: MyIdNameIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id name of object
*  Input: Parameters
*      none
*  Returns:
*      char* - id name
*********************************************************************/
char * PhyEncl::MyIdNameIs(void)
{
    return idname;
};

/*********************************************************************
* PhyEncl Class :: dumpme()
*********************************************************************
*  Description:
*       Dumps id number and PhyEnclcount
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/
void PhyEncl::dumpme(void)
{ 
     strcpy (key, "PhyEncl::dumpme");
     sprintf(temp, "Object id: (%d) count: (%d)\n", 
         idnumber, PhyEnclCount);
     vWriteFile(key, temp);
}

/*********************************************************************
 * PhyEncl Class :: select()
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
 *      18-July-2011 : inital version
 *
 *********************************************************************/
fbe_status_t PhyEncl::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    object_id = FBE_OBJECT_ID_INVALID;

    strcpy (key, "Select Physical Enclosure Interface"); 

    // List Physical Enclosure calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) PhyEnclInterfaceFuncs );
        return FBE_STATUS_GENERIC_FAILURE;
    }
    // get enclosure object id
    if (strcmp (argv[index], "GETENCLOBJID") == 0) {
        status = get_enclosure_object_id(argc, &argv[index]);

    // get enclosure information
     //}else if (strcmp (argv[index], "GETENCLINFO") == 0) {
        //status = get_enclosure_information(argc, &argv[index]);

    // get enclosure type
    }else if (strcmp (argv[index], "GETENCLTYPE") == 0) {
        status = get_enclosure_type(argc, &argv[index]);

        // get enclosure drive slot information
    }else if (strcmp (argv[index], "GETENCLDRIVESLOTINFO") == 0) {
        status = get_enclosure_drive_slot_information(argc, &argv[index]);

    // get enclosure fan information
    }else if (strcmp (argv[index], "GETENCLFANINFO") == 0) {
        status = get_enclosure_fan_information(argc, &argv[index]);

    // send drive debug control
    }else if (strcmp (argv[index], "SENDDRIVEDEBUGCTRL") == 0) {
        status = send_drive_debug_control(argc, &argv[index]);

    // send drive power control
    }else if (strcmp (argv[index], "SENDDRIVEPOWERCTRL") == 0) {
        status = send_drive_power_control(argc, &argv[index]);

    // send expander control
    }else if (strcmp (argv[index], "SENDEXPANDERCTRL") == 0) {
        status = send_expander_control(argc, &argv[index]);

    // get scsi error information
    }else if (strcmp (argv[index], "GETSCSIERRORINFO") == 0) {
        status = get_scsi_error_informatiom(argc, &argv[index]);

    // SEND EXP TEST MODE CONTROL
    }else if (strcmp (argv[index], "SENDEXPTESTMODECTRL") == 0) {
        status = send_exp_test_mode_control(argc, &argv[index]);

    // send exp string out control
    }else if (strcmp (argv[index], "SENDEXPSTRINGOUTCTRL") == 0) {
        status = send_exp_string_out_control(argc, &argv[index]);

    // can't find match on short name
    }else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
            BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) PhyEnclInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* PhyEncl Class :: get_enclosure_object_id()
*********************************************************************
*
*  Description:
*      Gets the enclosure object id by location.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyEncl::get_enclosure_object_id(int argc, char **argv) 
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [Port Number] [Enclosure Number]\n"
        " For example: %s 0 1";
        
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getEnclObjId",
        "PhyEncl::get_enclosure_object_id",
        "fbe_api_get_enclosure_object_id_by_location",  
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
   
    // Convert port number to hex.
    argv++;
    port_num = (fbe_u32_t)strtoul(*argv, 0, 0);

    // convert enclosure number to hex
    argv++;
    encl_num = (fbe_u32_t)strtoul(*argv, 0, 0);

    sprintf(params, " Port number:%d  Enclosure_number:%d", 
        port_num,encl_num);
            
    // Make api call: 
    status = fbe_api_get_enclosure_object_id_by_location(port_num,encl_num,&object_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get Object id for Enclosure:%d_%d",
            port_num,encl_num);
    } else {
        sprintf(temp,"<Object Id>: 0x%x(%d)",object_id,object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* PhyEncl Class :: get_enclosure_information()
*********************************************************************
*
*  Description:
*      Gets the enclosure information.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
/*
fbe_status_t PhyEncl::get_enclosure_information(int argc, char **argv) 
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [object id]\n"
        " For example: %s 104";
        
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getObjportnum",
        "PhyEncl::get_enclosure_information",
        "fbe_api_enclosure_get_info",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
   
    // Convert object id to hex.
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " object id 0x%x (%s)", object_id, *argv);
            
    // Make api call: 
    status = fbe_api_enclosure_get_info(object_id,&enclosure_info);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get enclosure information for object id 0x%x",
            object_id);
    } else {
        sprintf(temp,"<Enclosure information>:\n %s",enclosure_info.enclosureInfo);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    //release memory allocated to enclosure_info
    fbe_api_enclosure_release_info_memory(&enclosure_info);

    return status;
}
*/
/*********************************************************************
* PhyEncl Class :: send_drive_debug_control()
*********************************************************************
*
*  Description:
*      This function send DRIVE_DEBUG_CONTROL to enclosure
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyEncl::send_drive_debug_control(int argc, char **argv) 
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [Encl object id] [Drive Number] [Debug Action]\n"
        " For example: %s 2 4 <CRU_ON | CRU_OFF>";
        
    // Check that all arguments have been entered
    status = call_preprocessing(
        "sendDriveDebugCtrl",
        "PhyEncl::send_drive_debug_control",
        "fbe_api_enclosure_send_drive_debug_control",  
        usage_format,
        argc, 9);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
   
    // Convert object id to hex.
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    ++argv;
    // Convert drive number to hex.
    drive_num = (fbe_u32_t)strtoul(*argv, 0, 0);

    ++argv;
    //get drive debug action
    status = get_drive_debug_action(argv);
    
   // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Invalid Drive Debug Action. Valid"
            " Actions are (CRU_ON | CRU_OFF");
        sprintf(params," Encl Object Id 0x%x (%s)   Drive Number:%d",
            object_id, *(argv-2),drive_num);
        call_post_processing(status, temp, key, params);
        return status;
    }

    sprintf(params, " object id 0x%x (%s)  Drive Number:%d "
        " Debug Action:0x%x(%s)", 
        object_id, *(argv-2),drive_num,cru_on_off_action,*argv);

    status = get_drive_debug_info();
    if (status != FBE_STATUS_OK) {
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call: 
    status = fbe_api_enclosure_send_drive_debug_control(object_id,
                                                        &enclosureDriveDebugInfo);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Unable to send Drive Debug Control"
            " request (%s) for Encl Object Id 0x%x ,"
            " Drive Number:%d",*argv,object_id,drive_num);
    } else {
        sprintf(temp, " Successfully sent Drive Debug"
            " Control request (%s) for Encl Object id 0x%x ,"
            " Drive Number:%d",*argv,object_id,drive_num);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* PhyEncl Class :: send_drive_power_control()
*********************************************************************
*
*  Description:
*      This function send Drive Power Control.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyEncl::send_drive_power_control(int argc, char **argv) 
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [Encl Object Id] [Drive Number] [Power Action]\n"
        " For example: %s 2 4 <PWR_DOWN | PWR_UP | PWR_CYCLE>";
        
    // Check that all arguments have been entered
    status = call_preprocessing(
        "sendDrivePowerCtrl",
        "PhyEncl::send_drive_power_control",
        "fbe_api_enclosure_send_drive_power_control",
        usage_format,
        argc, 9);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
   
    // Convert object id to hex.
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    ++argv;
    // Convert drive number to hex.
    drive_num = (fbe_u32_t)strtoul(*argv, 0, 0);

    ++argv;
    //get drive power action
    status = get_drive_power_action(argv);
    
    // Check status of call
     if (status != FBE_STATUS_OK) {
         sprintf(temp, "Invalid Drive Power Action. Valid "
             "Actions are (PWR_DOWN | PWR_UP | PWR_CYCLE");
         sprintf(params," Encl Object Id 0x%x (%s)   Drive Number:%d",
             object_id, *(argv-2),drive_num);
        call_post_processing(status, temp, key, params);
         return status;
     }

    sprintf(params, " Encl Object Id 0x%x (%s)  Drive Number:%d "
        " Power Action:0x%x(%s)", 
        object_id, *(argv-2),drive_num,power_on_off_action,*argv);

    status = get_drive_power_info();
    if (status != FBE_STATUS_OK) {
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call: 
    status = fbe_api_enclosure_send_drive_power_control(object_id,
                                                        &drivePowerInfo);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Unable to send Drive Power Control"
            " request (%s) for Encl Object Id 0x%x ,"
            " Drive Number:%d",*argv,object_id,drive_num);
    } else {
        sprintf(temp, " Successfully sent Drive Power"
            " Control request (%s) for Encl Object Id 0x%x ,"
            " drive number:%d",*argv,object_id,drive_num);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* PhyEncl Class :: send_expander_control()
*********************************************************************
*
*  Description:
*      This function send expander control.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyEncl::send_expander_control(int argc, char **argv) 
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [Encl Object Id] [Expander Action]"
        " [Power Cycle Delay] [Power Cycle Duration]\n"
        " For example: %s 2 <COLD_RESET | SILENT_MODE> 0 0";
        
    // Check that all arguments have been entered
    status = call_preprocessing(
        "sendDriveExpanderCtrl",
        "PhyEncl::send_expander_control",
        "fbe_api_enclosure_send_expander_control",
        usage_format,
        argc, 10);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
   
    // Convert object id to hex.
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    ++argv;
    //get drive debug action
    status = get_expander_action(argv);
   // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Invalid Expander Action Valid"
            " Actions are (COLD_RESET | SILENT_MODE)");
        sprintf(params, " object id 0x%x (%s)", object_id, *(argv-1));
        call_post_processing(status, temp, key, params);
        return status;
    }

    ++argv;
    // Convert power Cycle Delay to hex.
    powerCycleDelay = (fbe_u32_t)strtoul(*argv, 0, 0);

        ++argv;
    // Convert power Cycle Duration to hex.
    powerCycleDuration = (fbe_u32_t)strtoul(*argv, 0, 0);

    sprintf(params, " object id 0x%x (%s)  Expander Action:0x%x(%s) "
        "Power Cycle Delay: %d sec  Power Cycle Duration: %d sec", 
        object_id, *(argv-3),expanderAction,*(argv-2),
        powerCycleDelay,powerCycleDuration);

    status = get_expander_info();
    if (status != FBE_STATUS_OK) {
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call: 
    status = fbe_api_enclosure_send_expander_control(object_id,&expanderInfo);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Unable to send Drive Power Control"
            " request (%s) for Encl Object Id 0x%x ,"
            "Power Cycle Delay:%d sec,Power Cycle Duration: %d sec",
            *(argv-2),object_id,powerCycleDelay,powerCycleDuration);
    } else {
        sprintf(temp, " Successfully sent Expander Control"
            " request (%s) for Encl Object Id 0x%x ,"
            "Power Cycle Delay:%d sec,Power Cycle Duration: %d sec",
            *(argv-2),object_id,powerCycleDelay,powerCycleDuration);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* PhyEncl Class :: get_scsi_error_informatiom()
*********************************************************************
*
*  Description:
*      This function gets scsi error information about the enclosure.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyEncl::get_scsi_error_informatiom(int argc, char **argv) 
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [Encl Object Id]\n"
        " For example: %s 4 ";
        
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getScsiErrorInfo",
        "PhyEncl::get_scsi_error_informatiom",
        "fbe_api_enclosure_get_scsi_error_info",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert object id to hex.
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    sprintf(params, " Encl Object Id 0x%x (%s)", object_id, *argv);

    // Make api call: 
    status = fbe_api_enclosure_get_scsi_error_info(object_id,
                                       &scsi_error_info);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get Scsi Error Information for Encl Object Id 0x%x",
            object_id);
    } else {
        edit_scsi_error_informatiom(&scsi_error_info);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* PhyEncl Class :: get_drive_debug_info()
*********************************************************************
*
*  Description:
*      Gets the drive debug information.
*
*  Input: Parameters
*      None
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyEncl::get_drive_debug_info() 
{
    //get drive count on enclosure
    status = get_drive_count();
        if (status != FBE_STATUS_OK) {
        return status;
    }

    //zeroing memory 
    fbe_zero_memory(&enclosureDriveDebugInfo, 
        sizeof(fbe_base_object_mgmt_drv_dbg_ctrl_t));

    enclosureDriveDebugInfo.driveCount = (fbe_u8_t)component_count;
    enclosureDriveDebugInfo.driveDebugAction[drive_num] = cru_on_off_action;

    return FBE_STATUS_OK;
}

/*********************************************************************
* PhyEncl Class :: get_drive_power_info()
*********************************************************************
*
*  Description:
*      Gets the drive power information.
*
*  Input: Parameters
*      None
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyEncl::get_drive_power_info() 
{
    //get drive count on enclosure
    status = get_drive_count();
        if (status != FBE_STATUS_OK) {
        return status;
    }
        
    //zeroing memory 
    fbe_zero_memory(&drivePowerInfo,
        sizeof(fbe_base_object_mgmt_drv_power_ctrl_t));
    
    drivePowerInfo.driveCount = (fbe_u8_t)component_count;
    drivePowerInfo.drivePowerAction[drive_num] = power_on_off_action;

    return FBE_STATUS_OK;
}

/*********************************************************************
* PhyEncl Class :: get_expander_info()
*********************************************************************
*
*  Description:
*      Gets the expander information.
*
*  Input: Parameters
*      None
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyEncl::get_expander_info() 
{
    //zeroing memory 
    fbe_zero_memory(&expanderInfo, sizeof(fbe_base_object_mgmt_exp_ctrl_t));

    expanderInfo.expanderAction= expanderAction;
    expanderInfo.powerCycleDelay = powerCycleDelay;
    expanderInfo.powerCycleDuration = powerCycleDuration;

    return FBE_STATUS_OK;
}

/*********************************************************************
* PhyEncl Class :: get_drive_count()
*********************************************************************
*
*  Description:
*      Gets the number of drive on enclosure.
*
*  Input: Parameters none
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyEncl::get_drive_count() 
{
    fbe_edal_status_t edal_status;

    // Make api call: 
    status = fbe_api_enclosure_get_info(object_id,&enclosure_info);
   // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get Enclosure Info for Encl Object Id:0x%x",
            object_id);
        return status;
    }

    //Enclosure Data contains some pointers that must be converted
    fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)&enclosure_info);
    
    //Get the specific component count for Drive
    edal_status = fbe_edal_getSpecificComponentCount((void*)&enclosure_info,
        FBE_ENCL_DRIVE_SLOT,
        &component_count);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        sprintf(temp, "Can't get Drive Count for Encl Object Id:0x%x",object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //drive number must be less than component count
    if(drive_num >= component_count)
    {
        sprintf(temp, "Invalid Drive Number!!!."
            " Drive Number must be less than %d",component_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*********************************************************************
* PhyEncl Class :: get_drive_debug_action()
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
fbe_status_t PhyEncl::get_drive_debug_action(char **argv) 
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
* PhyEncl Class :: get_drive_power_action()
*********************************************************************
*
*  Description:
*      Gets drive power action.
*
*  Input: Parameters
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyEncl::get_drive_power_action(char **argv) 
{
    if(strcmp(*argv, "PWR_DOWN") == 0) {
        power_on_off_action = FBE_DRIVE_POWER_ACTION_POWER_DOWN;
    } else if(strcmp(*argv, "PWR_UP") == 0){
        power_on_off_action = FBE_DRIVE_POWER_ACTION_POWER_UP;
    } else if(strcmp(*argv, "PWR_CYCLE") == 0){
        power_on_off_action = FBE_DRIVE_POWER_ACTION_POWER_CYCLE;
    } else {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
* PhyEncl Class :: get_expander_action()
*********************************************************************
*
*  Description:
*      Gets expander action.
*
*  Input: Parameters
*      **argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyEncl::get_expander_action(char **argv) 
{
    if(strcmp(*argv, "COLD_RESET") == 0) {
        expanderAction = FBE_EXPANDER_ACTION_COLD_RESET;
    } else if(strcmp(*argv, "SILENT_MODE") == 0){
        expanderAction = FBE_EXPANDER_ACTION_SILENT_MODE;
    } else {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
* PhyEncl Class :: edit_scsi_error_informatiom()
*********************************************************************
*
*  Description:
*      Displays Scsi error information.
*
*  Input: Parameters
*      scsi_error_info  - scsi error information
*
*  Returns:None
*
*********************************************************************/
void PhyEncl::edit_scsi_error_informatiom(fbe_enclosure_scsi_error_info_t *scsi_error_info) 
{
    sprintf(temp,                                   "\n "
            "<Scsi Error Code>                      0x%X\n "
            "<Scsi Status>                          0x%X\n "
            "<Payload Request Status>               0x%X\n "
            "<Sense Key>                            0x%X\n "
            "<Addl Sense Code>                      0x%X\n "
            "<Addl Sense Code Qualifier>            0x%X\n ",
            scsi_error_info->scsi_error_code,
            scsi_error_info->scsi_status,
            scsi_error_info->payload_request_status,
            scsi_error_info->sense_key,
            scsi_error_info->addl_sense_code,
            scsi_error_info->addl_sense_code_qualifier);

    return;
}

/*********************************************************************
* PhyEncl Class :: get_enclosure_type()
*********************************************************************
*
*  Description:
*      Gets the enclosure type.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyEncl::get_enclosure_type(int argc, char **argv) 
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [Encl Object Id]\n"
        " For example: %s 104";
        
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getEnclType",
        "PhyEncl::get_enclosure_type",
        "fbe_api_enclosure_get_encl_type",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
   
    // Convert object id to hex.
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, "Encl Object Id 0x%x (%s)", object_id, *argv);
            
    // Make api call: 
    status = fbe_api_enclosure_get_encl_type(object_id,&enclosuretype);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get enclosure type for object id 0x%x",
            object_id);
    } else {
        sprintf(temp,"\n"
            "<Enclosure Type>: 0x%x(%s)",
            enclosuretype,
            fbe_edal_print_Enclosuretype(enclosuretype));
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* PhyEncl Class :: get_enclosure_drive_slot_information()
*********************************************************************
*
*  Description:
*      Gets the enclosure drive slot information.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyEncl::get_enclosure_drive_slot_information(int argc, char **argv) 
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [Port Number] "\
        "[Enclosure Number] [Drive Number]\n"
        " For example: %s 1 2 3";
        
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getEnclDriveSlotInfo",
        "PhyEncl::get_enclosure_drive_slot_information",
        "fbe_api_enclosure_get_drive_slot_info",
        usage_format,
        argc, 9);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
   
    // Convert port number to hex.
    argv++;
    location.bus = (fbe_u8_t)strtoul(*argv, 0, 0);

    // Convert enclosure number to hex.
    argv++;
    location.enclosure = (fbe_u8_t)strtoul(*argv, 0, 0);

    // Convert drive number to hex.
    argv++;
    location.slot = (fbe_u8_t)strtoul(*argv, 0, 0);

    sprintf(params," Port Number:%d  Enclosure Number:%d  "
        "Drive Number:%d  ",
        location.bus,location.enclosure,location.slot);

    // Make api call: 
    status = fbe_api_get_enclosure_object_id_by_location(location.bus,
                                                         location.enclosure,
                                                         &object_id);
    if(status != FBE_STATUS_OK)
    {
        sprintf(temp, "Can't get enclosure Object Id by Location,"
            "Port :%d,Enclosure:%d,Slot:%d ",
            location.bus,location.enclosure,location.slot);
        call_post_processing(status, temp, key, params);
        return status;
    }

    //fill location information
    getDriveSlotInfo.location = location;

    // Make api call: 
    status = fbe_api_enclosure_get_drive_slot_info(object_id,
                                                   &getDriveSlotInfo);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get enclosure drive slot information"
            " for Encl object id 0x%x,Port:%d,Enclosure:%d,Slot:%d ",
            object_id,location.bus,location.enclosure,location.slot);
    } else {
        edit_enclosure_drive_slot_information(&getDriveSlotInfo);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* PhyEncl Class :: get_enclosure_fan_information()
*********************************************************************
*
*  Description:
*      This function gets enclosure Fan(Cooling) info.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyEncl::get_enclosure_fan_information(int argc, char **argv) 
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [Encl object id] [Slot Number]\n"
        " For example: %s 4 2";
        
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getEnclFanInfo",
        "PhyEncl::get_enclosure_fan_information",
        "fbe_api_enclosure_get_fan_info",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
   
    // Convert object id to hex.
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    // Convert Drive number to hex.
    argv++;
    drive_num = (fbe_u32_t)strtoul(*argv, 0, 0);

    sprintf(params, "Encl Object Id: 0x%x (%s)  Slot Number: %d", 
        object_id, *(argv-1),drive_num);

    //filling slot number on enclosure.
    fanInfo.slotNumOnEncl = drive_num;

    fanInfo.firmwareRev[0] = '\0';
    fanInfo.subenclProductId[0] = '\0';

    // Make api call: 
    status = fbe_api_enclosure_get_fan_info(object_id,&fanInfo);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get enclosure fan"\
            " information for enclosure object id 0x%x",
            object_id);
    } else {
        edit_enclosure_fan_information(&fanInfo);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* PhyEncl Class :: edit_enclosure_drive_slot_information()
*********************************************************************
*
*  Description:
*      Displays enclosure drive slot information.
*
*  Input: Parameters
*      getDriveSlotInfo  - enclosure drive slot information
*
*  Returns:None
*
*********************************************************************/
void PhyEncl::edit_enclosure_drive_slot_information(
                   fbe_enclosure_mgmt_get_drive_slot_info_t *getDriveSlotInfo) 
{
    sprintf(temp,                                   "\n "
            "<Bypassed>                           %s\n "
            "<Self Bypassed>                      %s\n ",
            getDriveSlotInfo->bypassed? "Yes" : "No",
            getDriveSlotInfo->selfBypassed? "Yes" : "No");

    return;
}

/*********************************************************************
* PhyEncl Class :: edit_enclosure_fan_information()
*********************************************************************
*
*  Description:
*      Displays enclosure Fan(Cooling) information.
*
*  Input: Parameters
*      fanInfo  - Fan(Cooling) information
*
*  Returns:None
*
*********************************************************************/
void PhyEncl::edit_enclosure_fan_information(fbe_cooling_info_t *fanInfo) 
{
    sprintf(temp,                                          "\n "
            "<Slot number>                                 %d\n "
            "<Env Interface Status>                        %s\n "
            "<In Processor Enclosure>                      %s\n "
            "<Inserted>                                    %s\n "
            "<Fan Faulted>                                 %s\n "
            "<Fan Degraded>                                %s\n "
            "<Downloadable>                                %s\n "
            "<Firmware Rev>                                %s\n "
            "<Sub Enclosure Product Id>                    %s\n "
            "<Unique Id>                                   0x%x\n "
            "<Multi Fan Module Fault>                      %s\n "
            "<Persistent Multi Fan Module Fault>           %s\n ",
            fanInfo->slotNumOnEncl,
            convert_env_interface_status_id_to_name(fanInfo->envInterfaceStatus),
            fanInfo->inProcessorEncl ? "Yes" : "No",
            convert_mgmt_status_id_to_name(fanInfo->inserted),
            convert_mgmt_status_id_to_name(fanInfo->fanFaulted),
            convert_mgmt_status_id_to_name(fanInfo->fanDegraded),
            fanInfo->bDownloadable ? "Yes" : "No",
            fanInfo->firmwareRev,
            fanInfo->subenclProductId,
            fanInfo->uniqueId,
            convert_mgmt_status_id_to_name(fanInfo->multiFanModuleFault),
            convert_mgmt_status_id_to_name(fanInfo->persistentMultiFanModuleFault));

    return;
}

/*********************************************************************
* PhyEncl Class :: convert_mgmt_status_id_to_name()
*********************************************************************
*
*  Description:
*      convert mgmt status id to name.
*
*  Input: Parameters
*     mgmt_status - mgmt status
*
*  Returns:
*      char* - pointer to status name string.
* 
*********************************************************************/
char* PhyEncl::convert_mgmt_status_id_to_name(fbe_mgmt_status_t mgmt_status) 
{
    char * mgmtstr;
        
    switch(mgmt_status)
    {
        case FBE_MGMT_STATUS_FALSE:
            mgmtstr = (char *)("Yes");
        break;
        case FBE_MGMT_STATUS_TRUE:
            mgmtstr = (char *)("No");
        break;
        case FBE_MGMT_STATUS_UNKNOWN:
            mgmtstr = (char *)("Unknown");
        break;
        default:
            mgmtstr = (char *)( "N.A. ");
        break;
    }
    return(mgmtstr);

}

/*********************************************************************
* PhyEncl Class :: convert_mgmt_status_id_to_name()
*********************************************************************
*
*  Description:
*      convert env interface status id to name.
*
*  Input: Parameters
*     envInterfaceStatus - env interface status
*
*  Returns:
*      char* - pointer to status name string.
* 
*********************************************************************/
char* PhyEncl::convert_env_interface_status_id_to_name(
                                fbe_env_inferface_status_t envInterfaceStatus) 
{
    char * envstr;
        
    switch(envInterfaceStatus)
    {
        case FBE_ENV_INTERFACE_STATUS_GOOD:
            envstr = (char *)("Good");
        break;
        case FBE_ENV_INTERFACE_STATUS_XACTION_FAIL:
            envstr = (char *)("Fail");
        break;
        case FBE_ENV_INTERFACE_STATUS_DATA_STALE:
            envstr = (char *)("Data Stale");
        break;
        default:
            envstr = (char *)( "N.A. ");
        break;
    }
    return(envstr);

}

/*********************************************************************
* PhyEncl Class :: phy_encl_initializing_variables()
*********************************************************************/
void PhyEncl :: phy_encl_initializing_variables()
{
        port_num = 0;
        encl_num = 0;
        drive_num = 0;
        component_count = 0;
        powerCycleDuration = 0;
        powerCycleDelay =0 ;
        cru_on_off_action = FBE_DRIVE_ACTION_NONE;
        power_on_off_action = FBE_DRIVE_POWER_ACTION_NONE;
        expanderAction = FBE_EXPANDER_ACTION_NONE;
        enclosuretype = 0;

        fbe_zero_memory(&enclosure_info, sizeof(enclosure_info));
        fbe_zero_memory(&enclosureDriveDebugInfo, sizeof(enclosureDriveDebugInfo));
        fbe_zero_memory(&drivePowerInfo, sizeof(drivePowerInfo));
        fbe_zero_memory(&expanderInfo, sizeof(expanderInfo));
        fbe_zero_memory(&scsi_error_info, sizeof(scsi_error_info));
        fbe_zero_memory(&location, sizeof(location));
        fbe_zero_memory(&getDriveSlotInfo, sizeof(getDriveSlotInfo));
        fbe_zero_memory(&fanInfo, sizeof(fanInfo));
        
}


/*********************************************************************
* PhyEncl Class :: send_exp_test_mode_control()
*********************************************************************
*
*  Description:
*      send the exp test mode control
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyEncl::send_exp_test_mode_control(int argc, char **argv) 
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [encl object id] [test mode control]\n"
        "Valid values for testmode control\n"
        "1:Status, 2:Disable, 3:Enable\n"
        " For example: %s 2 ";
        
    // Check that all arguments have been entered
    status = call_preprocessing(
        "sendExpTestModeCtrl",
        "PhyEncl::send_exp_test_mode_control",
        "fbe_api_enclosure_send_exp_test_mode_control ",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
   
    // Convert object id to hex.
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    ++argv;
    //get test mode control
     expanderInfoTest.cmd_buf.testModeInfo.testModeAction = (fbe_base_object_mgmt_exp_test_mode_action_t)strtoul(*argv, 0, 0);
    
    sprintf(params, "object id (0x%x),  test mode control(%d)",object_id, expanderInfoTest.cmd_buf.testModeInfo.testModeAction);

    
    // Make api call: 
    status = fbe_api_enclosure_send_exp_test_mode_control (
        object_id,&expanderInfoTest);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Unable to send expander test mode Control"
            " for Encl Object Id 0x%x ", object_id);
    } else {
        sprintf(temp, " Successfully sent Expander test mode Control"
            " for Encl Object Id 0x%x ", object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;

}

/*********************************************************************
* PhyEncl Class :: send_exp_string_out_control()
*********************************************************************
*
*  Description:
*      send the exp string out control
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyEncl::send_exp_string_out_control(int argc, char **argv) 
{
    fbe_u32_t       stringLen = 0;
    char       *stringtemp;
    
    // Define specific usage string  
    usage_format =
        " Usage: %s [encl object id] [command1]..[command n]\n"
        " For example: %s pinfo ";
        
    // Check that all arguments have been entered
    status = call_preprocessing(
        "sendExpStringOutCtrl",
        "PhyEncl::send_exp_string_out_controll",
        "fbe_api_enclosure_send_exp_string_out_control",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    fbe_zero_memory(&esesPageOp, sizeof(fbe_enclosure_mgmt_ctrl_op_t));
    stringOutPtr = esesPageOp.cmd_buf.stringOutInfo.stringOutCmd;
    
    // Convert object id to hex.
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    argv++;

    while(*argv != NULL){
       stringLen += (fbe_u32_t)strlen(*argv);
        if (stringLen > FBE_BASE_EXP_STRING_OUT_CMD_MAX_LENGTH)
        {
            sprintf(temp,"enclosure expStringOut, command too long\n");
            // Output results from call or description of error
            call_post_processing(status, temp, key, params);
            return status;
        }
        stringtemp = new char[strlen(*argv)];

        utilityClass::convert_upercase_to_lowercase(stringtemp,*argv);
        
        strcat((char*)stringOutPtr, stringtemp);
        stringLen++;
        argv++;

        if (*argv == NULL)
        {
            strcat((char*)stringOutPtr, "\n");
        }
        else
        {
            strcat((char*)stringOutPtr, " ");
        }

    }

    sprintf(params, "object id (0x%x), command(%s)",object_id, stringOutPtr);
    
    // Make api call: 
       status = fbe_api_enclosure_send_exp_string_out_control(object_id, &esesPageOp);
    
    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Unable to send expander string out Control"
            " for Encl Object Id 0x%x ", object_id);
    } else {
        sprintf(temp, " Successfully sent Expander string out Control"
            " for Encl Object Id 0x%x ", object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;

}


/*********************************************************************
 * PhyEncl end of file
 *********************************************************************/
