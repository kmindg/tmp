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
 *      This file defines the methods of the physical Discovery Interface class.
 *
 *  Notes:
 *      The physical discovery interface class (PhyDiscovery) is a derived class of 
 *      the base class (bPhysical).
 *
 *  History:
 *      29-July-2011 : initial version
 *
 *********************************************************************/

#ifndef T_PHYDISCOVERYCLASS_H
#include "phy_discovery_class.h"
#endif

/*********************************************************************
* PhyDiscovery::PhyDiscovery() (Constructor)
*********************************************************************
*  Description:
*       Initializes class variables
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/

PhyDiscovery::PhyDiscovery() : bPhysical()
{
    // Store Object number
    idnumber = (unsigned) PHY_DISCOVERY_INTERFACE;
    phyDiscoveryCount = ++gPhysDiscoveryCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "PHY_DISCOVERY_INTERFACE");
    port_num = 0;
    encl_num = 0;
    drive_num = 0;
    lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    package_id = FBE_PACKAGE_ID_INVALID;
    params_temp[0] = '\0';

    if (Debug) {
        sprintf(temp, 
            "PhyDiscovery class constructor (%d) %s\n", 
            idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API Physical Drive Interface function calls>
    PhyDiscoveryInterfaceFuncs = 
        "[function call]\n" \
        "[short name] [FBE API Physical Discovery Interface]\n" \
        " ------------------------------------------------------------\n" \
        " getObjPortNum   fbe_api_get_object_port_number\n" \
        " getObjEnclNum   fbe_api_get_object_enclosure_number\n" \
        " getObjDriveNum  fbe_api_get_object_drive_number\n" \
        " ------------------------------------------------------------\n"
        " setObjToState   fbe_api_set_object_to_state\n" \
        " --------------------------------------------------------\n" ;

    // Define common usage for physical discovery commands
    usage_format = 
        " Usage: %s [object id]\n"
        " For example: %s 10";
};

 /*********************************************************************
* PhyDiscovery Class :: MyIdNumIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id number of object
*  Input: Parameters
*      none
*  Returns:
*      unsigned int - id number
*********************************************************************/
unsigned PhyDiscovery::MyIdNumIs(void)
{
    return idnumber;
};

/*********************************************************************
* PhyDiscovery Class :: MyIdNameIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id name of object
*  Input: Parameters
*      none
*  Returns:
*      char* - id name
*********************************************************************/
char * PhyDiscovery::MyIdNameIs(void)
{
    return idname;
};

/*********************************************************************
* PhyDiscovery Class :: dumpme()
*********************************************************************
*  Description:
*       Dumps id number and phy discovery count
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/
void PhyDiscovery::dumpme(void)
{ 
     strcpy (key, "PhyDiscovery::dumpme");
     sprintf(temp, "Object id: (%d) count: (%d)\n", 
         idnumber, phyDiscoveryCount);
     vWriteFile(key, temp);
}

/*********************************************************************
 * PhyDiscovery Class :: select()
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
 *      29-July-2011 : initial version
 *
 *********************************************************************/

fbe_status_t PhyDiscovery::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    
    strcpy (key, "Select Physical Interface");

    // List Physical Discovery calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) PhyDiscoveryInterfaceFuncs );
        return status;
    }
    
    // get object port number
    if (strcmp (argv[index], "GETOBJPORTNUM") == 0) {
        status = get_object_port_number(argc, &argv[index]);

    // get object enclosure number
     }else if (strcmp (argv[index], "GETOBJENCLNUM") == 0) {
        status = get_object_encl_number(argc, &argv[index]);

    // get object drive number
    }else if (strcmp (argv[index], "GETOBJDRIVENUM") == 0) {
        status = get_object_drive_number(argc, &argv[index]);
    
    // set object to specific state 
    } else if (strcmp (argv[index], "SETOBJTOSTATE") == 0) {
        status = set_object_to_state(argc, &argv[index]);

    // can't find match on short name
    }else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
            BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) PhyDiscoveryInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* PhyDiscovery Class :: get_object_port_number()
*********************************************************************
*
*  Description:
*      Gets the object port number.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t PhyDiscovery::get_object_port_number(int argc, char **argv) 
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getObjPortNum",
        "PhyDiscovery::get_object_port_number",
        "fbe_api_get_object_port_number",  
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
    status = fbe_api_get_object_port_number(object_id,&port_num);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get port number for object id 0x%x",
                object_id);
    } else {
        sprintf(temp,"<Port Number>: %d",port_num);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* PhyDiscovery Class :: get_object_encl_number()
*********************************************************************
*
*  Description:
*      Gets the object enclosure number.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t PhyDiscovery::get_object_encl_number(int argc, char **argv) 
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getObjEnclNum",
        "PhyDiscovery::get_object_encl_number",
        "fbe_api_get_object_enclosure_number",  
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
    status = fbe_api_get_object_enclosure_number(object_id,&encl_num);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get enclosure number for object id 0x%x",
                object_id);
    } else {
        sprintf(temp,"<Enclosure Number>: %d",encl_num);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* PhyDiscovery Class :: get_object_drive_number()
*********************************************************************
*
*  Description:
*      Gets the object drive number.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t PhyDiscovery::get_object_drive_number(int argc, char **argv) 
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getObjDriveNum",
        "PhyDiscovery::get_object_drive_number",
        "fbe_api_get_object_drive_number",  
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
    status = fbe_api_get_object_drive_number(object_id,&drive_num);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get drive number for object id 0x%x",
                object_id);
    } else {
        sprintf(temp,"<Drive Number>: %d",drive_num);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* PhyDiscovery Class :: set_object_to_state()
*********************************************************************
*
*  Description:
*      Set the object to specific state
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t PhyDiscovery::set_object_to_state(
    int argc, 
    char **argv) 
{
    // Assign default values
    object_id = FBE_OBJECT_ID_INVALID;
    
    // Define specific usage string  
    usage_format =
        " Usage: %s [object id] [lifecycle state] [package id] \n"
        "lifecycle state: \n" 
        "   specialize\n"       
        "   activate\n"           
        "   ready\n"                
        "   hibernate\n"            
        "   offline\n"            
        "   fail\n"                 
        "   destroy\n"             
        "   non_pending_max\n"
        "   pending_ready\n"
        "   pending_activate\n"
        "   pending_hibernate\n"
        "   pending_offline\n"
        "   pending_fail\n"
        "   pending_destroy\n"
        "   pending_max\n"
        "   not_exist\n"
        "\n"
        "package id: \n" 
        "   phy\n"       
        "   sep\n"                
        "   esp\n"            
        "\n" 
        " For example: %s 1 hibernate sep\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setObjToState",
        "PhyDiscovery::set_object_to_state",
        "fbe_api_set_object_to_state",  
        usage_format,
        argc, 9);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(params, " object id 0x%x (%s)", object_id, *argv);

    // Extract the lifecycle state
    argv++;
    lifecycle_state = utilityClass::convert_string_to_lifecycle_state(*argv);
    if (FBE_LIFECYCLE_STATE_INVALID == lifecycle_state){
        sprintf(temp, "The entered lifecycle state (%s) is invalid",
                      *argv);
        status = FBE_STATUS_GENERIC_FAILURE;
        params[0] = '\n';
        call_post_processing(status, temp, key, params);
        return status;

    }
    sprintf(params_temp, " lifecycle state 0x%x (%s) ", 
                    lifecycle_state, *argv);
    if(strlen(params_temp) > MAX_PARAMS_SIZE)
    {
        sprintf(temp, "<ERROR!> Params length is larger that buffer value");
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }
    strcat(params, params_temp);

    // Extract the package id
    argv++;
    package_id = utilityClass::convert_string_to_package_id(*argv);
    if (FBE_PACKAGE_ID_INVALID == package_id){
        sprintf(temp, "The entered package name (%s) is invalid",
                      *argv);
        status = FBE_STATUS_GENERIC_FAILURE;
        params[0] = '\n';
        call_post_processing(status, temp, key, params);
        return status;
    }
    sprintf(params_temp, " package id 0x%x (%s)", 
                    package_id, *argv);
    if(strlen(params_temp) > MAX_PARAMS_SIZE)
    {
        sprintf(temp, "<ERROR!> Params length is larger that buffer value");
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }
    strcat(params, params_temp);

    // Make api call: 
    status = fbe_api_set_object_to_state(
        object_id, 
        lifecycle_state,
        package_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to set the object id 0x%X from "
                      " package id 0x%x to state 0x%X",
                      object_id,  
                      package_id,
                      lifecycle_state);
                      

    }else if (status == FBE_STATUS_OK){ 
        sprintf(temp, "Set the object id 0x%X from "
                      " package id 0x%x to state 0x%X",
                      object_id,  
                      package_id,
                      lifecycle_state);
    }     

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 * PhyDiscovery end of file
 *********************************************************************/
