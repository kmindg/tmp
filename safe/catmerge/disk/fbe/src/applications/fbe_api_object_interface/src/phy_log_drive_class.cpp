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
 *      This file defines the methods of the Physical Logical Drive 
 *      class.
 *
 *  Notes:
 *      The Logical Drive class (PhylogDrive) is a derived class of
 *      the base class (bPhysical).
 *
 *  History:
 *      12-May-2010 : inital version
 *
 *********************************************************************/

#ifndef T_PHYLOGDRIVECLASS_H
#include "phy_log_drive_class.h"
#endif

/*********************************************************************
 * PhyDrive class :: Constructor
 *********************************************************************/

PhyLogDrive::PhyLogDrive() : bPhysical()
{
    // Store Object number
    idnumber = (unsigned) PHY_LOG_DRIVE_INTERFACE;
    pldrCount = ++gPhysLogDriveCount;
    Phy_Intializing_variable();
    // Store Object name
    idname = new char [64];
    strcpy(idname, "PHY_LOG_DRIVE_INTERFACE");

    if (Debug) {
        sprintf(temp,
            "PhyLogDrive class constructor (%d) %s\n",
            idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API logical Drive Interface function calls>
    phyLogDriveInterfaceFuncs =
        "[function call]\n"
        "[short name] [FBE API Logical Drive Interface]\n"
        " ------------------------------------------------------------\n"
        " getObjbyLDr fbe_api_get_logical_drive_object_id_by_location \n"
        " getObjbySer fbe_api_logical_drive_get_id_by_serial_number   \n"
        " ------------------------------------------------------------\n"
        " getDrAttr   fbe_api_logical_drive_get_attributes\n"
        " getDrBlkSz  fbe_api_logical_drive_get_drive_block_size\n"
        " setDrAttr   fbe_api_logical_drive_set_attributes\n"
        " valDrBlkSz  fbe_api_logical_drive_negotiate_and_validate_block_size\n"
        " ------------------------------------------------------------\n"
        " clearEol    fbe_api_logical_drive_clear_pvd_eol\n"
        " ------------------------------------------------------------\n";

    // Define common usage for physical drive commands
    usage_format =
        " Usage: %s [object id]\n"
        " For example: %s 4";
};

/*********************************************************************
 * PhyDrive class : Accessor methods
 *********************************************************************/

unsigned PhyLogDrive::MyIdNumIs(void)
{
    return idnumber;
};

char * PhyLogDrive::MyIdNameIs(void)
{
    return idname;
};

void PhyLogDrive::dumpme(void)
{
     strcpy (key, "PhyLogDrive::dumpme");
     sprintf(temp, "Object id: (%d) count: (%d)\n",
         idnumber, pldrCount);
     vWriteFile(key, temp);
}

/*********************************************************************
 * PhyLogDrive Class :: select()
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
 *      2010-05-12 : inital version
 *
 *********************************************************************/

fbe_status_t PhyLogDrive::select(int index, int argc, char *argv[])
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select Physical interface");

    // List Logical Drive calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) phyLogDriveInterfaceFuncs );

    // get object Id by location (port, enclosure, slot)
    }else if (strncmp (argv[index], "GETOBJBYLDR", 11) == 0) {
        status = get_object_id(argc, &argv[index]);

    // get_id_by_serial_number


    } else if (strncmp (argv[index], "GETOBJBYSER", 11) == 0) {
        status = get_id_by_serial_number(argc, &argv[index]);

    // get_attributes
    }else if (strncmp (argv[index], "GETDRATTR", 9) == 0) {
        status = get_attributes(argc, &argv[index]);

    // get_drive_block_size
    }else if (strncmp (argv[index], "GETDRBLKSZ", 10) == 0) {
        status = get_drive_block_size(argc, &argv[index]);

    // set_attributes
    }else if (strncmp (argv[index], "SETDRATTR", 9) == 0) {
        status = set_attributes(argc, &argv[index]);

    // negotiate_and_validate_block_size
    }else if (strncmp (argv[index], "VALDRBLKSZ", 10) == 0) {
        status = validate_block_size(argc, &argv[index]);

    // Clear EOL
    }else if (strcmp (argv[index], "CLEAREOL") == 0) {
        status = clear_eol(argc, &argv[index]);
        
    // can't find match on short name
    }else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
            BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) phyLogDriveInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
 *  PhyLogDrive class :: get_object_id ()
 *********************************************************************/

fbe_status_t PhyLogDrive::get_object_id(int argc, char **argv)
{
    // Assign default values
    object_id = FBE_OBJECT_ID_INVALID;

    // Define specific usage string
    usage_format =
        " Usage: %s [disk number in B_E_D]\n"\
        " For example: %s 1_2_3";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getObjbyLDr",
        "PhyLogDrive::object_id_by_location,",
        "fbe_api_get_logical_drive_object_id_by_location",
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
    status = fbe_api_get_logical_drive_object_id_by_location(
        port,
        enclosure,
        slot,
        &object_id);
    fbe_status_t passFail = status;

    // Check status of call
    if (object_id == (fbe_u32_t) FBE_OBJECT_ID_INVALID) {
        strcpy(temp, "FBE_OBJECT_ID_INVALID");
        passFail = FBE_STATUS_GENERIC_FAILURE;

    }else if (status != FBE_STATUS_OK) {
        strcpy(temp, "Can't get object id for drive ");

    }else if (status == FBE_STATUS_OK){
        sprintf(temp, "<object id> 0x%x (%u)",
            object_id, object_id);
    }

    // Output results from call or description of error
    call_post_processing(passFail, temp, key, params);
    return status;
}
/*********************************************************************
 * PhyLogDrive class :: get_attributes ()
 *********************************************************************/

fbe_status_t PhyLogDrive::get_attributes(int argc , char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getDrAttr",
        "PhyLogDrive::get_attributes",
        "fbe_api_logical_drive_get_attributes",
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
    status = fbe_api_logical_drive_get_attributes(
        object_id,
        &attributes);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get logical drive attributes");
    } else {
        edit_drive_info(&attributes);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyLogDrive class :: validate_block_size ()
 *********************************************************************/

fbe_status_t PhyLogDrive::validate_block_size(int argc, char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [object id] [Block size] [Optimal Block size]"\
        " [imported black size] [imported optimal block size]\n"\
        " For example: %s 0x17 520 1 520 1";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "valDrBlkSz",
        "PhyLogDrive::validate_block_size",
        "fbe_api_logical_drive_negotiate_and_validate_block_size",
        usage_format,
        argc, 11);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(cObjId, "%s", *argv++);

    // Convert block sizes to hex.
    block_size = (fbe_block_size_t)
        strtoul(*argv++, 0, 0);
    
    optimal_block_size = (fbe_block_size_t)
        strtoul(*argv++, 0, 0);
    
    imported_block_size = (fbe_block_size_t)
        strtoul(*argv++, 0, 0);
    
    imported_optimal_block_size = (fbe_block_size_t)
        strtoul(*argv, 0, 0);

    sprintf(params,
        " object id 0x%x (%s) "
        " block_size (%d)"
        " optimal_block_size (%d)"
        " imported_block_size (%d)"
        " imported_optimal_block_size (%d)",
            object_id, cObjId,
            block_size,
            optimal_block_size,
            imported_block_size,
            imported_optimal_block_size);

    // Make api call:
    status = fbe_api_logical_drive_negotiate_and_validate_block_size(
         object_id,
         block_size,
         optimal_block_size,
         imported_block_size,
         imported_optimal_block_size);

    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't validate block sizes");
    } else {
        sprintf(temp, "Successfully validated block size for logical drive obj ID 0x%X (%s)",
                object_id, cObjId);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyLogDrive class :: get_drive_block_size ()
 *********************************************************************/

fbe_status_t PhyLogDrive::get_drive_block_size(int argc, char ** argv)
{

    usage_format =
        " Usage: %s [object id] [imported capacity] [imported block size]\n"
        " For example: %s 4 0x16f23900 520";
    
    optimal_block_size = 0;
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getDrBlkSz",
        "PhyLogDrive::get_drive_block_size",
        "fbe_api_logical_drive_get_drive_block_size",
        usage_format,
        argc, 9);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(cObjId, "%s", *argv++);

    // Convert block size to hex.
    capacity.requested_block_size = (fbe_block_size_t)
        strtoul(*argv++, 0, 0);
  if (*argv){
    // Convert optimum_block_size to hex.
    optimal_block_size = (fbe_block_size_t)
        strtoul(*argv, 0, 0);
  }
    capacity.requested_optimum_block_size = (optimal_block_size)
        ? optimal_block_size : capacity.requested_block_size;

    sprintf(params,
        " object id 0x%x (%s)\n"
        " block_size (%d)"
        " optimal_block_size (%d)",
            object_id, cObjId,
            capacity.requested_block_size,
            capacity.requested_optimum_block_size);

    // Debug ... problem: call not returning - remove

    printf("%s \n", params);

    // Make api call:
     status = fbe_api_logical_drive_get_drive_block_size(
         object_id,
         &capacity,
         &block_status,
         &block_qualifier);
     printf("post call status %d\n", status);

    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't get drive block size info");
    } else {
        edit_drive_block_size_info(
            &capacity,
            block_status,
            block_qualifier);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyLogDrive class :: get_id_by_serial_number
 *********************************************************************/

fbe_status_t PhyLogDrive::get_id_by_serial_number(int argc , char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getObjbySer",
        "PhyLogDrive::get_id_by_serial_number",
        "fbe_api_logical_drive_get_id_by_serial_number",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Copy serial number and get length.
    argv++;
     if(strlen(*argv) > 5000)
    {
        sprintf(params, " Input string larger than temp buffer");
        passFail = FBE_STATUS_GENERIC_FAILURE;
        call_post_processing(passFail, temp, key, params);
        return status;
    }
    strcpy (temp, *argv);
    fbe_u32_t sn_length = (fbe_u32_t)strlen(temp);
    
    sprintf(params, " serial number %s length (%d)",
        (fbe_u8_t*)temp,
        sn_length);

    // Make api call:
    status = fbe_api_logical_drive_get_id_by_serial_number(
        (fbe_u8_t*)temp,
        sn_length,
        &object_id);
     fbe_status_t passFail = status;
   
    // Check status of call
    if (object_id == (fbe_u32_t) FBE_OBJECT_ID_INVALID) {
        strcpy(temp, "FBE_OBJECT_ID_INVALID");
        passFail = FBE_STATUS_GENERIC_FAILURE;
     
    }else if (status != FBE_STATUS_OK) {
        strcpy(temp, "Can't get object id by_serial_number");
    
    }else if (status == FBE_STATUS_OK){ 
        sprintf(temp, "<object id> 0x%x (%u)", 
            object_id, object_id);
    }     
    
    // Output results from call or description of error
    call_post_processing(passFail, temp, key, params);
    return status;
}

/*********************************************************************
 *  PhyLogDrive class :: set_attributes
 *********************************************************************/

fbe_status_t PhyLogDrive::set_attributes(int argc , char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [object id] [imported capacity] [imported block size]\n"
        " For example: %s 4 0x16f23900 520";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "setDrAttr",
        "PhyLogDrive::set_attributes ",
        "fbe_api_logical_drive_set_attributes",
        usage_format,
        argc, 9);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Convert Object id to hex
    argv++;
    object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
    sprintf(cObjId, "%s", *argv++);

    // Convert capacity to hex.
    if (strncmp (*argv, "*", 1) != 0) {
        attributes.imported_capacity = (fbe_lba_t)
             strtoul(*argv, 0, 0);
    }   
 
    // Convert block size to hex.
    argv++;
    if (strncmp (*argv, "*", 1) != 0) {
        attributes.imported_block_size = (fbe_block_size_t)
            strtoul(*argv, 0, 0);
    }

    sprintf(params,
        " object id 0x%x (%s)"
        " imported_capacity (%llu)"
        " optimal_imported_block_size (%d)",
            object_id, cObjId,
            (unsigned long long)attributes.imported_capacity,
            attributes.imported_block_size);

    // Make api call:
    attributes_p = &attributes;
    status = fbe_api_logical_drive_set_attributes(
        object_id,
        attributes_p);

    // Check status of call
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Can't set attributes");
    } else {
        sprintf(temp, "Successfully set attributes for logical drive obj ID 0x%X (%s)",
                object_id, cObjId);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
 * PhyLogDrive class :: edit_drive_info (private method)
 *********************************************************************/

void PhyLogDrive::edit_drive_info(fbe_logical_drive_attributes_t *d)
{
    sprintf(temp,                      "\n "
        "<capacity>              0x%llx\n "
        "<Imported block size>   %d\n "
        "<Identify info>         %s\n "
        "<optional_queued>       0x%x\n "
        "<low_queued>            0x%x\n "
        "<normal_queued>         0x%x\n "
        "<urgent_queued>         0x%x\n "
        "<No. of clients>        0x%x (%d)\n "
        "<Server Object id>      0x%x (%d)\n "
        "<outstanding_io_count>  0x%x (%d)\n "
        "<outstanding_io_max>    0x%x (%d)\n "
        "<block transport flags> 0x%x\n ",
            (unsigned long long)d->imported_capacity,
            d->imported_block_size,
            d->initial_identify.identify_info,
            d->server_info.b_optional_queued,
            d->server_info.b_low_queued,
            d->server_info.b_normal_queued,
            d->server_info.b_urgent_queued,
            d->server_info.number_of_clients,
            d->server_info.number_of_clients,
            d->server_info.server_object_id,
            d->server_info.server_object_id,
            d->server_info.outstanding_io_count,
            d->server_info.outstanding_io_count,
            d->server_info.outstanding_io_max,
            d->server_info.outstanding_io_max,
            d->server_info.attributes);
}

/*********************************************************************
 * PhyLogDrive class :: edit_drive_block_size_info (private method)
 *********************************************************************/

void PhyLogDrive::edit_drive_block_size_info(
    fbe_block_transport_negotiate_t *d,
    fbe_payload_block_operation_status_t block_status,
    fbe_payload_block_operation_qualifier_t block_qualifier)
{
    sprintf(temp,                       "\n "
        "<block status>                 0x%x\n "
        "<block qualifer>               0x%x\n "
        "<requested block size>         0x%x (%d)\n "
        "<requested_optimum_block_size> 0x%x (%d)\n "
        "<physical_block_size>          0x%x (%d)\n "
        "<physical_optimum_block_size>  0x%x (%d)\n ",
            block_status,
            block_qualifier,
            d->requested_block_size,
            d->requested_block_size,
            d->requested_optimum_block_size,
            d->requested_optimum_block_size,
            d->physical_block_size,
            d->physical_block_size,
            d->physical_optimum_block_size,
            d->physical_optimum_block_size);
}

/*********************************************************************
 * PhyLogDrive::Phy_Intializing_variable (private method)
 *********************************************************************/
void PhyLogDrive::Phy_Intializing_variable() 
{
    attributes.imported_block_size = 0;
    attributes.imported_capacity = 0;
    attributes.initial_identify.identify_info[0] = '\0';
    attributes.last_identify.identify_info[0] = '\0';
    attributes.server_info.b_optional_queued = FBE_FALSE;
    attributes.server_info.b_low_queued = FBE_FALSE; 
    attributes.server_info.b_normal_queued = FBE_FALSE;
    attributes.server_info.b_urgent_queued = FBE_FALSE;
    attributes.server_info.number_of_clients = 0;
    attributes.server_info.server_object_id = 0;
    attributes.server_info.outstanding_io_count = 0;
    attributes.server_info.outstanding_io_max = 0;
    attributes.server_info.tag_bitfield = 0;
    attributes.server_info.attributes = 0;
    capacity.block_count = 0;
    capacity.block_size = 0;
    capacity.optimum_block_alignment = 0;
    capacity.optimum_block_size = 0;
    capacity.physical_block_size = 0;
    capacity.physical_optimum_block_size = 0;
    capacity.requested_block_size = 0;
    capacity.requested_optimum_block_size = 0;
    phys_location.bus = 0;
    phys_location.enclosure = 0;
    phys_location.slot = 0;
    block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    block_size = 0;
    block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    enclosure = 0;
    imported_block_size = 0;
    imported_capacity = 0;
    imported_optimal_block_size = 0;
    optimal_block_size = 0;
    port = 0;
    slot = 0;
    return;
}

/*********************************************************************
*  PhyLogDrive Class ::  clear_eol(
*    int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Clear the EOL (End Of Life) for the specified logical disk.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t PhyLogDrive::clear_eol(int argc , char ** argv){

    // Check that all arguments have been entered
    status = call_preprocessing(
        "ClearEol",
        "PhyLogDrive::clear_eol",
        "fbe_api_logical_drive_clear_pvd_eol",
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

    // Make API call
    status = fbe_api_logical_drive_clear_pvd_eol(object_id);
    if(status != FBE_STATUS_OK){
        sprintf(temp, "Failed to clear EOL on logical drive "
            "with object id 0x%x", object_id);
    }
    else {
        sprintf(temp,"EOL cleared successfully on logical drive "
            "with object id 0x%x", object_id );
    }
    call_post_processing(status, temp, key, params);
    return status;

}
/*********************************************************************
 * PhyLogDrive end of file
 *********************************************************************/
