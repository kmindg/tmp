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
 *      This file defines the methods of the SYS DISCOVERY INTERFACE class.
 *
 *  Notes:
 *      The system class (SysDiscovery) is a derived class of 
 *      the base class (bsys).
 *
 *  History:
 *      4-July-2011 : inital version
 *
 *********************************************************************/

#ifndef T_SYS_DISCOVERY_CLASS_H
#include "sys_discovery_class.h"
#endif

/*********************************************************************
* SysDiscovery::SysDiscovery() (Constructor)
*********************************************************************
*  Description:
*       Initializes class variables
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/

SysDiscovery::SysDiscovery() : bSYS()
{
    // Store Object number
    idnumber = (unsigned) SYS_DISCOVERY_INTERFACE;
    sysDiscoveyCount = ++gSysDiscoveryCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "SYS_DISCOVERY_INTERFACE");

    if (Debug) {
        sprintf(temp, 
            "SysDiscovery class constructor (%d) %s\n", 
            idnumber, idname);
        vWriteFile(dkey, temp);
    }

    Sys_Discovery_Intializing_variable();

    // <List of FBE API System Interface function calls>
    SysDiscoveryInterfaceFuncs = 
        "[function call]\n" \
        "[short name] [FBE API System Discovery Interface]\n" \
        " ------------------------------------------------------------\n" \
        " getObjLifecycleState  fbe_api_get_object_lifecycle_state\n" \
        " getObjDeathReason     fbe_api_get_object_death_reason\n" \
        " ------------------------------------------------------------\n" \
        " getTotalObj           fbe_api_get_total_objects\n" \
        " getObjClassId         fbe_api_get_object_class_id\n" \
        " ------------------------------------------------------------\n" \
        " discoverObjs          fbe_api_enumerate_objects\n" \
        " ------------------------------------------------------------\n" \
        " getPsmLunObjId        \n" \
        " getVaultLunObjId      \n" \
        " ------------------------------------------------------------\n" \
        " getPsmRgObjId        \n" \
        " getVaultRgObjId      \n" \
        " ------------------------------------------------------------\n";

    // Define common usage for system discovery commands
    usage_format =
        " Usage: %s [package id] [object id]\n"
        " For example: %s <PHY|SEP|ESP> 11";
};

 /*********************************************************************
* SysDiscovery Class :: MyIdNumIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id number of object
*  Input: Parameters
*      none
*  Returns:
*      unsigned int - id number
*********************************************************************/
unsigned SysDiscovery::MyIdNumIs(void)
{
    return idnumber;
};

/*********************************************************************
* SysDiscovery Class :: MyIdNameIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id name of object
*  Input: Parameters
*      none
*  Returns:
*      char* - id name
*********************************************************************/
char * SysDiscovery::MyIdNameIs(void)
{
    return idname;
};

/*********************************************************************
* SysDiscovery Class :: dumpme()
*********************************************************************
*  Description:
*       Dumps id number and sys discovery count
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/
void SysDiscovery::dumpme(void)
{ 
     strcpy (key, "SysDiscovery::dumpme");
     sprintf(temp, "Object id: (%d) count: (%d)\n", 
         idnumber, sysDiscoveyCount);
     vWriteFile(key, temp);
}

/*********************************************************************
 * SysDiscovery Class :: select()
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
 *      4-July-2011 : inital version
 *
 *********************************************************************/

fbe_status_t SysDiscovery::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    object_id = FBE_OBJECT_ID_INVALID;
    
    strcpy (key, "Select System Interface"); 
    c = index;

    // List System Discovery calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) SysDiscoveryInterfaceFuncs );
        return status;
    }
    
    // get object lifecycle state
    if (strcmp (argv[index], "GETOBJLIFECYCLESTATE") == 0) {
        status = get_object_lifecycle_state(argc, &argv[index]);

    // get object death reason
    }else if (strcmp (argv[index], "GETOBJDEATHREASON") == 0) {
        status = get_object_death_reason(argc, &argv[index]);

    // get total objects
    }else if (strcmp (argv[index], "GETTOTALOBJ") == 0) {
        status = get_total_objects(argc, &argv[index]);

    // get object class id
    }else if (strcmp (argv[index], "GETOBJCLASSID") == 0) {
        status = get_object_class_id(argc, &argv[index]);

    // display objects of a specific package
    }else if (strcmp (argv[index], "DISCOVEROBJS") == 0) {
        status = discover_objects_of_package(argc, &argv[index]);
    
    // get the psm lun object id
    }else if (strcmp (argv[index], "GETPSMLUNOBJID") == 0) {
        status = get_psm_lun_object_id(argc, &argv[index]);

    // get the vault lun object id
    }else if (strcmp (argv[index], "GETVAULTLUNOBJID") == 0) {
        status = get_vault_lun_object_id(argc, &argv[index]);
    
    // get the psm rg object id
    }else if (strcmp (argv[index], "GETPSMRGOBJID") == 0) {
        status = get_psm_rg_object_id(argc, &argv[index]);

    // get the vault rg object id
    }else if (strcmp (argv[index], "GETVAULTRGOBJID") == 0) {
        status = get_vault_rg_object_id(argc, &argv[index]);
    
    // can't find match on short name
    }else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
            BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) SysDiscoveryInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* SysDiscovery Class :: get_object_lifecycle_state()
*********************************************************************
*
*  Description:
*      Gets the object lifecycle state.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t SysDiscovery::get_object_lifecycle_state(int argc, char **argv) 
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getObjLifecycleState",
        "SysDiscovery::get_object_lifecycle_state",
        "fbe_api_get_object_lifecycle_state",  
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert package id name to hex.
    argv++;
    package_id = utilityClass::convert_string_to_package_id(*argv);
    if (package_id == FBE_PACKAGE_ID_INVALID) {
        sprintf(temp, " Invalid package %s. Valid packages are PHY, SEP, or ESP.\n",*argv);
        sprintf(params, " package id 0x%x (%s)", package_id, *argv);
        call_post_processing(status, temp, key, params);
        return status;
    }
    
    // Convert object id to hex.
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    sprintf(params, "  package id 0x%x (%s)  object id 0x%x (%s)",
        package_id, *(argv-1), object_id, *argv);
    
    // Make api call: 
    status = fbe_api_get_object_lifecycle_state(object_id,
                                                &lifecycle_state,
                                                package_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get lifecycle state for object id 0x%x",
                object_id);
    } else {
        edit_object_lifecycle_state(&lifecycle_state);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* SysDiscovery Class :: get_object_death_reason()
*********************************************************************
*
*  Description:
*      Gets the object death reason.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t SysDiscovery::get_object_death_reason(int argc, char **argv) 
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getObjDeathReason",
        "SysDiscovery::get_object_death_reason",
        "fbe_api_get_object_death_reason",  
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert package id name to hex.
    argv++;
    package_id = utilityClass::convert_string_to_package_id(*argv);
    if (package_id == FBE_PACKAGE_ID_INVALID) {
        sprintf(temp, " Invalid package %s. Valid packages are PHY, SEP, or ESP.\n",*argv);
        sprintf(params, " package id 0x%x (%s)", package_id, *argv);
        call_post_processing(status, temp, key, params);
        return status;
    }
    
    // Convert object id to hex.
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);

    sprintf(params, "  package id 0x%x (%s)  object id 0x%x (%s)", 
        package_id, *(argv-1), object_id, *argv);
    
    // Make api call: 
    status = fbe_api_get_object_death_reason(object_id,
                                             &death_reason,
                                             &death_reason_str,
                                             package_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get death reason for object id 0x%x",
                object_id);
    } else {
        sprintf(temp,"<Death reason>  0x%x(%s)",death_reason,
            death_reason_str);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* SysDiscovery Class :: get_total_objects()
*********************************************************************
*
*  Description:
*      Gets the total nuber of objects for the given package id.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t SysDiscovery::get_total_objects(int argc, char **argv) 
{
    // Define specific usage string  
    usage_format =
        " Usage: %s [package id]\n"
        " For example: %s <PHY|SEP|ESP> ";
        
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getTotalObj",
        "SysDiscovery::get_total_object",
        "fbe_api_get_total_objects",  
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert package id name to hex.
    argv++;
    package_id = utilityClass::convert_string_to_package_id(*argv);
    sprintf(params, " package id 0x%x (%s)", package_id, *argv);
    if (package_id == FBE_PACKAGE_ID_INVALID) {
        sprintf(temp, " Invalid package %s. Valid packages are PHY, SEP, or ESP.\n",*argv);
        call_post_processing(status, temp, key, params);
        return status;
    }
    
    // Make api call: 
    status = fbe_api_get_total_objects(&total_objects,
                                       package_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get total objects for package id 0x%x",
            package_id);
    } else {
        sprintf(temp, "<Total Objects>  %d ",total_objects);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* SysDiscovery Class :: get_object_class_id()
*********************************************************************
*
*  Description:
*      Gets the object class id.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t SysDiscovery::get_object_class_id(int argc, char **argv) 
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getObjClassId",
        "SysDiscovery::get_object_class_id",
        "fbe_api_get_object_class_id",  
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert package id name to hex.
    argv++;
    package_id = utilityClass::convert_string_to_package_id(*argv);
    if (package_id == FBE_PACKAGE_ID_INVALID) {
        sprintf(temp, " Invalid package %s. Valid packages are PHY, SEP, or ESP.\n",*argv);
        sprintf(params, " package id 0x%x (%s)", package_id, *argv);
        call_post_processing(status, temp, key, params);
        return status;
    }
    
    // Convert object id to hex.
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    
    sprintf(params, "  package id 0x%x (%s)  object id 0x%x (%s)", 
        package_id, *(argv-1), object_id, *argv);
            
    // Make api call: 
    status = fbe_api_get_object_class_id(object_id,
                                         &class_id,
                                         package_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get class id for object id 0x%x",
            object_id);
    } else {
        //
        status = fbe_get_class_by_id(class_id,&class_info_p);
        if (status != FBE_STATUS_OK) {
            sprintf(temp, "Can't get class name for class id 0x%x,object_id 0x%x",
            class_id,object_id);
            }
        sprintf(temp, "<Class id>  0x%x (%s)",class_id,class_info_p->class_name);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* SysDiscovery Class :: discover_objects_of_package()
*********************************************************************
*
*  Description:
*      Gets objects belonging to a package
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
          
fbe_status_t SysDiscovery::discover_objects_of_package(
    int argc, char **argv) 
{
    // Define specific usage string
    usage_format =
        " Usage: %s [package: phy|sep]\n"\
        " For example: %s sep\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "discoverObjs",
        "SysDiscovery::discover_objects_of_package",
        "fbe_api_enumerate_objects",  
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Convert package id name to hex.
    argv++;
    package_id = utilityClass::convert_string_to_package_id(*argv);
    if (package_id == FBE_PACKAGE_ID_INVALID) {
        sprintf(temp, " Invalid package %s. "
                      "Valid packages are PHY, SEP.",
                      *argv);
        sprintf(params, " package id 0x%x (%s)", package_id, *argv);
        call_post_processing(status, temp, key, params);
        return status;
    }
    sprintf(params, " package id 0x%x (%s)", package_id, *argv);
    
    // Make api call: 
    status = fbe_api_get_total_objects(&object_count, package_id);
    if (status != FBE_STATUS_OK) {
		sprintf(temp, " Failed to get the object count "
                      "for package %s.",
                      *argv);
        call_post_processing(status, temp, key, params);
        return status;
	}

    // Allocate memory for the objects.
    object_list_p = (fbe_object_id_t *) 
        fbe_api_allocate_memory(sizeof(fbe_object_id_t) * object_count);
    if (object_list_p == NULL)
    {
        sprintf(temp, "Unable to allocate memory for "
                      "object list for package %s.",
                      *argv);
        status = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Find the count of total objects.
    status = fbe_api_enumerate_objects(
        object_list_p, 
        object_count, 
        &total_objects, 
        package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_free_memory(object_list_p);
        sprintf(temp, "Failed to enumerate objects for package %s",
                      *argv);
        call_post_processing(status, temp, key, params);
        return status;
    } else {
        edit_objects_of_package(object_list_p, total_objects);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    // free allocated memory
    fbe_api_free_memory(object_list_p);

    return status;
}

/*********************************************************************
* SysDiscovery Class :: get_psm_lun_object_id()
*********************************************************************
*
*  Description:
*      Get the psm lun object id
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t SysDiscovery::get_psm_lun_object_id(int argc, char **argv) 
{
    // Define specific usage string  
    usage_format = " Usage: %s\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getPsmLunObjId",
        "SysDiscovery::get_psm_lun_object_id",
        "",  
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    params[0] = '\0';
    // Simply return the corresponding object id 
    // which are defined in fbe_api_private_space_layout.h
    sprintf(temp, "<Psm Lun Object Id>  0x%x", 
        FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM);

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* SysDiscovery Class :: get_vault_lun_object_id()
*********************************************************************
*
*  Description:
*      Get the vault lun object id
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t SysDiscovery::get_vault_lun_object_id(int argc, char **argv) 
{
    // Define specific usage string  
    usage_format = " Usage: %s\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getVaultLunObjId",
        "SysDiscovery::get_vault_lun_object_id",
        "",  
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    params[0] = '\0';
    // Simply return the corresponding object id 
    // which are defined in fbe_api_private_space_layout.h
    sprintf(temp, "<Vault Lun Object Id>  0x%x", 
        FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN);

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* SysDiscovery Class :: get_psm_rg_object_id()
*********************************************************************
*
*  Description:
*      Get the psm rg object id
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t SysDiscovery::get_psm_rg_object_id(int argc, char **argv) 
{
    // Define specific usage string  
    usage_format = " Usage: %s\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getPsmRgObjId",
        "SysDiscovery::get_psm_rg_object_id",
        "",  
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    params[0] = '\0';
    // Simply return the corresponding object id 
    // which are defined in fbe_api_private_space_layout.h
    sprintf(temp, "<Psm Rg Object Id>  0x%x", 
        FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR);

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* SysDiscovery Class :: get_vault_rg_object_id()
*********************************************************************
*
*  Description:
*      Get the vault rg object id
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t SysDiscovery::get_vault_rg_object_id(int argc, char **argv) 
{
    // Define specific usage string  
    usage_format = " Usage: %s\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getVaultRgObjId",
        "SysDiscovery::get_vault_rg_object_id",
        "",  
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    params[0] = '\0';
    // Simply return the corresponding object id 
    // which are defined in fbe_api_private_space_layout.h
    sprintf(temp, "<Vault Rg Object Id>  0x%x", 
        FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG);

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* SysDiscovery Class :: edit_object_lifecycle_state()
*********************************************************************
*
*  Description:
*      This function print lifecycle state.
*
*  Input: Parameters
*      lifecycle_state  - lifecycle state id.
*
*  Returns:
*      none
*********************************************************************/
void SysDiscovery::edit_object_lifecycle_state(
    fbe_lifecycle_state_t *lifecycle_state)
{
    const fbe_u8_t *pp_state_name;

    pp_state_name = fbe_api_state_to_string(*lifecycle_state);

    sprintf(temp, "<Lifescycle State>  0x%x (%s)",
        *lifecycle_state,pp_state_name);

    return;
}

/*********************************************************************
* SysDiscovery Class :: edit_objects_of_package()
                          (private method)
*********************************************************************
*
*  Description:
*      Format the object list to display
*
*  Input: Parameters
*      object_list_p  - object list 
       total_objects  - object count 
*  Returns:
*      void
*********************************************************************/

void SysDiscovery::edit_objects_of_package(
    fbe_object_id_t *object_list_p,
    fbe_u32_t total_objects)
{
    fbe_const_class_info_t *class_info_p;
    fbe_port_number_t port_number = FBE_PORT_NUMBER_INVALID;
    fbe_port_number_t enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_enclosure_slot_number_t slot_number = FBE_ENCLOSURE_NUMBER_INVALID;
    
    temp[0] = '\0';

    // Loop over all objects found in the table.
    for (object_index = 0; object_index < total_objects; object_index++)
    {
        fbe_object_id_t object_id = object_list_p[object_index];
        
        // get class id
        status = fbe_api_get_object_class_id(object_id, &class_id, package_id);
        if (status != FBE_STATUS_OK){
            continue;
        }

        // get class info
        status = get_class_info(class_id, &class_info_p);
        if (status != FBE_STATUS_OK){
            // failure info is already filled inside function, so 
            // just return from here
            return;
        }
        
        // get bus, enclosure, slot info only for physical package 
        // and pvd in sep package
        if (FBE_PACKAGE_ID_PHYSICAL == package_id ||
            class_id == FBE_CLASS_ID_PROVISION_DRIVE){

            status = get_bus_enclosure_slot_info(object_id, 
                                                 class_info_p->class_id,
                                                 &port_number,
                                                 &enclosure_number,
                                                 &slot_number,
                                                 package_id);
       
            if (status != FBE_STATUS_OK){
                // failure info is already filled inside function, so 
                // just return from here
                return;
            }
        }

        if (FBE_PACKAGE_ID_PHYSICAL == package_id){

            // display object info
            if (class_id > FBE_CLASS_ID_LOGICAL_DRIVE_FIRST &&
                class_id < FBE_CLASS_ID_LOGICAL_DRIVE_LAST){

                sprintf(params_temp, "\t<LD_%02d_%02d_%02d> 0x%X\n", 
                                     port_number,
                                     enclosure_number,
                                     slot_number,
                                     object_id);    
                strcat(temp, params_temp); 
            }
            else if (class_id > FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST &&
                     class_id < FBE_CLASS_ID_PHYSICAL_DRIVE_LAST){
        
                 sprintf(params_temp, "\t<PD_%02d_%02d_%02d> 0x%X\n", 
                             port_number,
                             enclosure_number,
                             slot_number,
                             object_id);    
                 strcat(temp, params_temp); 
            }
        }
        else if (FBE_PACKAGE_ID_SEP_0 == package_id){
            
            // display object info
            if (class_id == FBE_CLASS_ID_PROVISION_DRIVE){
        
                 sprintf(params_temp, "\t<PVD_%02d_%02d_%02d> 0x%X\n", 
                             port_number,
                             enclosure_number,
                             slot_number,
                             object_id);    
            }
            else if (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE){
        
                 sprintf(params_temp, "\t<VD> 0x%X\n", object_id);    
            }
            else if (class_id > FBE_CLASS_ID_RAID_FIRST &&
                class_id < FBE_CLASS_ID_RAID_LAST){
                
                fbe_u32_t rg_num;

                // get rg number
                status = fbe_api_database_lookup_raid_group_by_object_id(
                    object_id, 
                    &rg_num);
                if (status != FBE_STATUS_OK){
                    sprintf(temp, "Can't get RG ID for Object ID 0x%X", 
                                  object_id);
                    return;
                }
				
		  // skip mirror of r1_0
		  if (FBE_RAID_ID_INVALID != rg_num){
                	sprintf(params_temp, "\t<RG_%02d> 0x%X\n", rg_num, object_id);    
		  }
            }
            else if (class_id == FBE_CLASS_ID_LUN){
                
                fbe_u32_t lun_number;

                // get lun number
                status = fbe_api_database_lookup_lun_by_object_id(
                    object_id, 
                    &lun_number);
                if (status != FBE_STATUS_OK){
                    sprintf(temp, "Can't get LUN ID for Object ID 0x%X", 
                                  object_id);
                    return;
                }   
                
                sprintf(params_temp, "\t<LUN_%02d> 0x%X\n", 
                                    lun_number,
                                    object_id);    
            }
            else if (class_id == FBE_CLASS_ID_BVD_INTERFACE){
        
                 sprintf(params_temp, "\t<BVD> 0x%X\n", object_id);    
            }

            strcat(temp, params_temp); 

        }
    }


}

/*********************************************************************
* SysDiscovery Class :: get_class_info() (private method)
*********************************************************************
*
*  Description:
*      This function get class the information for object
*  Input: Parameters
*      class_id      - class id 
*      class_info_p  - class info structure
*  Returns:
*      fbe_status_t
*********************************************************************/

fbe_status_t SysDiscovery::get_class_info(
    fbe_class_id_t class_id,
    fbe_const_class_info_t **class_info_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    // Get the information on this class.
    status = fbe_get_class_by_id(class_id, class_info_p);
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't get object class info for Object ID 0x%X",
                      object_id);
        return status;
    }
    
    return status;
}

/*********************************************************************
* SysDiscovery Class :: get_bus_enclosure_slot_info() (private method)
*********************************************************************
*
*  Description:
*      This function gets bus, enclosure, slot info for object
*  Input: Parameters
*      object_id     - object id 
*      class_id      - class id
*      port_p        - port no output 
*      enc_p         - enclosure no output
*      slot_p        - slot no output
*      package_id    - package id
*  Returns:
*      fbe_status_t
*********************************************************************/

fbe_status_t SysDiscovery::get_bus_enclosure_slot_info(
    fbe_object_id_t object_id,
    fbe_class_id_t class_id,
    fbe_port_number_t *port_p,
    fbe_enclosure_number_t *enc_p,
    fbe_enclosure_slot_number_t *slot_p,
    fbe_package_id_t package_id)
{
    fbe_status_t status;

    *port_p = FBE_PORT_NUMBER_INVALID;
    *enc_p = FBE_ENCLOSURE_NUMBER_INVALID;
    *slot_p = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;

    if ( ((class_id >= FBE_CLASS_ID_PORT_FIRST) &&
          (class_id <= FBE_CLASS_ID_PORT_LAST)) ||
         ((class_id >= FBE_CLASS_ID_ENCLOSURE_FIRST) &&
          (class_id <= FBE_CLASS_ID_ENCLOSURE_LAST)) ||
         ((class_id >= FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) &&
          (class_id <= FBE_CLASS_ID_PHYSICAL_DRIVE_LAST)) ||
         ((class_id >= FBE_CLASS_ID_LOGICAL_DRIVE_FIRST) &&
          (class_id <= FBE_CLASS_ID_LOGICAL_DRIVE_LAST)) ) 
    {
        // We have a port.
        status  = fbe_api_get_object_port_number(object_id, port_p);
        if (status != FBE_STATUS_OK)
        {
            *port_p = FBE_PORT_NUMBER_INVALID;
        }
    }

    if ( ((class_id >= FBE_CLASS_ID_ENCLOSURE_FIRST) &&
          (class_id <= FBE_CLASS_ID_ENCLOSURE_LAST)) ||
         ((class_id >= FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) &&
          (class_id <= FBE_CLASS_ID_PHYSICAL_DRIVE_LAST)) ||
         ((class_id >= FBE_CLASS_ID_LOGICAL_DRIVE_FIRST) &&
          (class_id <= FBE_CLASS_ID_LOGICAL_DRIVE_LAST)) )
    {
        // We have an enclosure.
        status = fbe_api_get_object_enclosure_number(object_id, enc_p);
        if (status != FBE_STATUS_OK)
        {
            *enc_p = FBE_ENCLOSURE_NUMBER_INVALID;
        }
    }

    if ( ((class_id >= FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) &&
          (class_id <= FBE_CLASS_ID_PHYSICAL_DRIVE_LAST)) ||
         ((class_id >= FBE_CLASS_ID_LOGICAL_DRIVE_FIRST) &&
          (class_id <= FBE_CLASS_ID_LOGICAL_DRIVE_LAST)) )
    {
        // We have a slot.
        status = fbe_api_get_object_drive_number(object_id, slot_p);
        if (status != FBE_STATUS_OK)
        {
            *slot_p = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;
        }
    }
	if(class_id == FBE_CLASS_ID_PROVISION_DRIVE)
	{
	    status  = fbe_api_provision_drive_get_location(object_id, port_p, enc_p, slot_p);
        if (status != FBE_STATUS_OK)
        {
            *port_p = FBE_PORT_NUMBER_INVALID;
			*enc_p = FBE_ENCLOSURE_NUMBER_INVALID;
			*slot_p = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;
        }
		
	}
	if( (class_id == FBE_CLASS_ID_LUN) ||(class_id == FBE_CLASS_ID_RAID_GROUP) ||
		(class_id == FBE_CLASS_ID_RAID_FIRST) || (class_id == FBE_CLASS_ID_RAID_LAST) ||
		(class_id == FBE_CLASS_ID_BVD_INTERFACE) || (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE))
	{
	    *port_p = FBE_PORT_ENCL_SLOT_NA;
	    *enc_p =  FBE_PORT_ENCL_SLOT_NA;
		*slot_p = FBE_PORT_ENCL_SLOT_NA;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
* SysDiscovery::Sys_Discovery_Intializing_variable (private method)
*********************************************************************/
void SysDiscovery::Sys_Discovery_Intializing_variable()
{
    object_id = FBE_OBJECT_ID_INVALID;
    lifecycle_state =FBE_LIFECYCLE_STATE_INVALID;
    death_reason = 0;
    total_objects = 0;
    class_id = FBE_CLASS_ID_INVALID;
    object_list_p = NULL;
    found_objects_count = 0;      
    object_count = 0;
    object_index = 0;
    params_temp[0] ='\0';
}

/*********************************************************************
 * SysDiscovery end of file
 *********************************************************************/