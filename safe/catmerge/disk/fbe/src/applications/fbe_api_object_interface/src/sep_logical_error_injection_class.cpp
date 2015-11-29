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
*      This file defines the methods of the SEP Logical error injection
*      INTERFACE class.
*
*  Notes:
*      The SEP Logical error injection class (sepLogicalErrorInjection)
*      is a derived class of the base class (bSEP).
*
*  History:
*      16-March-2012 : initial version
*
*********************************************************************/

#ifndef T_SEP_LOGICAL_ERROR_INJECTION_CLASS_H
#include "sep_logical_error_injection_class.h"
#endif

/*********************************************************************
* sepLogicalErrorInjection class :: Constructor
*********************************************************************/

sepLogicalErrorInjection:: sepLogicalErrorInjection() : bSEP()
{
    // Store Object number
    idnumber = (unsigned) SEP_LOGICAL_ERROR_INJECTION_INTERFACE;
    sepLogicalErrorInjectionCount = ++gSepLogicalErrorInjectionCount;

    // Store Object name
    idname = new char [64];
    strcpy(idname, "SEP_LOGICAL_ERROR_INJECTION_INTERFACE");

    if (Debug) {
        sprintf(temp, "sepLogicalErrorInjection class constructor "
        "(%d) %s\n", idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP LogicalErrorInjection Interface function calls>
    sepLogicalErrorInjectionInterfaceFuncs=
        "[function call]\n" \
        " [short name]       [FBE API SEP LEI Interface]\n" \
        " ----------------------------------------------------------------\n" \
        " CreateRecord       fbe_api_logical_error_injection_create_record\n" \
        " CreateObjRecord    fbe_api_logical_error_injection_create_object_record\n" \
        " ModifyRecord       fbe_api_logical_error_injection_modify_record\n" \
        " DeleteRecord       fbe_api_logical_error_injection_delete_record\n" \
        " ----------------------------------------------------------------\n" \
        " Enable             fbe_api_logical_error_injection_enable\n" \
        " Disable            fbe_api_logical_error_injection_disable\n" \
        " ----------------------------------------------------------------\n"\
        " EnableObject       fbe_api_logical_error_injection_enable_object\n" \
        " DisableObject      fbe_api_logical_error_injection_disable_object\n" \
        " EnableClass        fbe_api_logical_error_injection_enable_class\n" \
        " DisableClass       fbe_api_logical_error_injection_disable_class\n" \
        " DisableRecords     fbe_api_logical_error_injection_disable_records\n" \
        " ----------------------------------------------------------------\n"\
        " GetStats           fbe_api_logical_error_injection_get_stats\n" \
        " GetObjStats        fbe_api_logical_error_injection_get_object_stats\n" \
        " GetRecords         fbe_api_logical_error_injection\n" \
        " ----------------------------------------------------------------\n";

    // Define common usage for SEP Lun commands
        usage_format =
        " Usage: %s \n"
        " For example: %s ";
        

         sprintf(sub_usage_format2 ,
        " \nTo inject error to position 0 pos_bitmap is 1, for position 1 is 2,"
        " position 2 is 4, and position N is (1 << N).\n\n"\
        " d = default value \n"\
        " Valid values for error_injection_type : \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RAID_CRC  \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SINGLE_BIT_CRC\n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC\n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CORRUPT_CRC \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CORRUPT_DATA\n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP\n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP  \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SHED_STAMP \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_WRITE_STAMP\n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_TIME_STAMP \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_SHED_STAMP \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1NS\n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1S \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1R \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1D \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1COD \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1COP \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1POC \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_COH \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR\n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR\n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR\n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SILENT_DROP\n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALIDATED\n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_SEQ_NUM_MISMATCH \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN\n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_UP\n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_IO_UNEXPECTED \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INCOMPLETE_WRITE \n\n"\
        " Valid values for error mode:\n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_MODE_COUNT \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_MODE_RANDOM\n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_MODE_SKIP \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_MODE_SKIP_INSERT \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_MODE_TRANS \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_MODE_TRANS_RND \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_SAME_LBA \n"\
        " FBE_API_LOGICAL_ERROR_INJECTION_MODE_UNKNOWN \n");

}

/*********************************************************************
* sepLogicalErrorInjection class : Accessor methods
*********************************************************************/

unsigned sepLogicalErrorInjection::MyIdNumIs(void)
{
    return idnumber;
};

char * sepLogicalErrorInjection::MyIdNameIs(void)
{
    return idname;
};

void sepLogicalErrorInjection::dumpme(void)
{
    strcpy (key, "sepLogicalErrorInjection::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n",
            idnumber, sepLogicalErrorInjectionCount );
    vWriteFile(key, temp);
}

/*********************************************************************
* sepsepLogicalErrorInjection Class :: select()
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
*      16-March-2012 : initial version
*
*********************************************************************/

fbe_status_t sepLogicalErrorInjection::select(
    int index, int argc, char *argv[])
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface");

    // List SEP LogicalErrorInjection Interface calls if help option 
    // and less than 6 arguments were entered.
    if (help && argc <=  6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepLogicalErrorInjectionInterfaceFuncs);

        // HoldIO
    } else if (strcmp (argv[index], "CREATERECORD") == 0) {
        status = get_logical_error_injection_create_record(
            argc, &argv[index]);

    } else if (strcmp (argv[index], "CREATEOBJRECORD") == 0) {
        status = get_logical_error_injection_create_object_record(
            argc, &argv[index]);

    } else if (strcmp (argv[index], "MODIFYRECORD") == 0) {
        status = get_logical_error_injection_modify_record(
            argc, &argv[index]);

    } else if (strcmp (argv[index], "DELETERECORD") == 0) {
        status = get_logical_error_injection_delete_record(
            argc, &argv[index]);

    } else if (strcmp (argv[index], "DISABLERECORDS") == 0) {
        status = get_logical_error_injection_disable_records(
            argc, &argv[index]);

    } else if (strcmp (argv[index], "GETSTATS") == 0) {
        status = get_logical_error_injection_stats(argc, &argv[index]);

    } else if (strcmp (argv[index], "GETOBJSTATS") == 0) {
        status = get_logical_error_injection_object_stats(
            argc, &argv[index]);

    } else if (strcmp (argv[index], "GETRECORDS") == 0) {
        status = get_logical_error_injection_get_records(
            argc, &argv[index]);

    } else if (strcmp (argv[index], "ENABLE") == 0) {
        status = get_logical_error_injection_enable(argc, &argv[index]);

    } else if (strcmp (argv[index], "DISABLE") == 0) {
        status = get_logical_error_injection_disable(argc, &argv[index]);

    } else if (strcmp (argv[index], "ENABLEOBJECT") == 0) {
        status = get_logical_error_injection_enable_object(
            argc, &argv[index]);

    } else if (strcmp (argv[index], "DISABLEOBJECT") == 0) {
        status = get_logical_error_injection_disable_object(
            argc, &argv[index]);

    } else if (strcmp (argv[index], "ENABLECLASS") == 0) {
        status = get_logical_error_injection_enable_class(
            argc, &argv[index]);

    } else if (strcmp (argv[index], "DISABLECLASS") == 0) {
        status = get_logical_error_injection_disable_class(
            argc, &argv[index]);

    } else {
        // can't find match on short name
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n",
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepLogicalErrorInjectionInterfaceFuncs);
    }

    return status;
}

/*********************************************************************
* sepLogicalErrorInjection::get_logical_error_injection_create_record(
* int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Creates the record
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/

fbe_status_t sepLogicalErrorInjection :: 
    get_logical_error_injection_create_record(int argc , char ** argv)
{
    strcpy(sub_usage_format1,
        " Usage: %s [position bitmap] [lba] [blocks] [error_injection_type] \n"\
        "       [Optional:[width | d] [error_mode | d] [error count | d] \n"\
        "       [error limit | d] [skip count | d] [skip limit | d]\n "\
        "       [error adjusancy bitmask | d] [start bit of error | d] \n"\
        "       [number of bits of an error | d] [erroneous bits adjacent | d]\n"\
        "       [CRC detectable | d] [opcode | d]\n"\
        " For example: %s 0x1 0x0 0x1 FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_UP"\
        " 0x10 FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS 0x0 0x15 0x0 0x15 0x1 0x0 0x0 0x0 0x0 0 \n");

        strcat(sub_usage_format1, sub_usage_format2);
        usage_format = sub_usage_format1;
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "CreateRecord",
        "sepLogicalErrorInjection::get_logical_error_injection_create_record",
        "fbe_api_logical_error_injection_create_record",
        usage_format,
        argc, 22);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Get position bitmap
    argv++;
    record.pos_bitmap = (fbe_u16_t)strtoul(*argv, 0, 0);

    // To inject to position 0, then pos_bitmap is 1, position 1 is 2,
    // position 2 is 4, and position N is (1 << N).  
    // Check the position bitmap
    if(record.pos_bitmap  < 1){
        sprintf(temp,"Invalid position bitmap %d received\n", 
            record.pos_bitmap);
        params[0] = '\0';
        call_post_processing(status, temp, key, params);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //Get lba
    argv++;
    record.lba = (fbe_lba_t)_strtoui64(*argv, 0, 0);

    // Get blocks to extend the errors
    argv++;
    record.blocks = (fbe_block_count_t)_strtoui64(*argv, 0, 0);

    // Take error type
    argv++;
    error_injection_type = (fbe_u32_t)strtoul(*argv, 0, 0);

    // extract error type
    record.err_type= get_err_injection_type(argv);
    if (record.err_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID)
    {
        sprintf(temp,"Invalid error type %d received\n", 
            record.err_type);
        params[0] = '\0';
        call_post_processing(status, temp, key, params);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Copy input parameters
    sprintf(params_temp, "position bitmap (%x), "
        "lba (%llx), "
        "blocks (%llu), "
        "error_injection_type (%x) %s ,",
        record.pos_bitmap, 
        record.lba, 
        record.blocks, 
        record.err_type,
        edit_error_injection_type(record.err_type));

    // Get optional parameters
    status = get_optional_parameters(argv);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Make api call:
    status =  fbe_api_logical_error_injection_create_record(&record);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to create logical error injection record.");
    } else {
        sprintf(temp, "Logical error injection record created successfully.");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params_temp);

    return status;
}


/*********************************************************************
* sepLogicalErrorInjection::get_logical_error_injection_create_object_record(
* int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Creates the record for specific object
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/

fbe_status_t sepLogicalErrorInjection :: 
    get_logical_error_injection_create_object_record(int argc , char ** argv)
{
    strcpy(sub_usage_format1,
        " Usage: %s [object id] [position bitmap] [lba] [blocks] [error_injection_type] \n"\
        "       [Optional:[width | d] [error_mode | d] [error count | d] \n"\
        "       [error limit | d] [skip count | d] [skip limit | d]\n "\
        "       [error adjusancy bitmask | d] [start bit of error | d] \n"\
        "       [number of bits of an error | d] [erroneous bits adjacent | d]\n"\
        "       [CRC detectable | d] [opcode | d]\n"\
        " For example: %s 0x1 0x0 0x1 FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_UP"\
        " 0x10 FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS 0x0 0x15 0x0 0x15 0x1 0x0 0x0 0x0 0x0 0 0x100\n");

        strcat(sub_usage_format1, sub_usage_format2);
        usage_format = sub_usage_format1;
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "CreateObjRecord",
        "sepLogicalErrorInjection::get_logical_error_injection_create_object_record",
        "fbe_api_logical_error_injection_create_object_record",
        usage_format,
        argc, 22);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Get object id
    argv++;
    object_id = (fbe_class_id_t)strtoul(*argv, 0, 0);

    // Get position bitmap
    argv++;
    record.pos_bitmap = (fbe_u16_t)strtoul(*argv, 0, 0);

    // To inject to position 0, then pos_bitmap is 1, position 1 is 2,
    // position 2 is 4, and position N is (1 << N).  
    // Check the position bitmap
    if(record.pos_bitmap  < 1){
        sprintf(temp,"Invalid position bitmap %d received\n", 
            record.pos_bitmap);
        params[0] = '\0';
        call_post_processing(status, temp, key, params);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //Get lba
    argv++;
    record.lba = (fbe_lba_t)_strtoui64(*argv, 0, 0);

    // Get blocks to extend the errors
    argv++;
    record.blocks = (fbe_block_count_t)_strtoui64(*argv, 0, 0);

    // Take error type
    argv++;
    error_injection_type = (fbe_u32_t)strtoul(*argv, 0, 0);

    // extract error type
    record.err_type= get_err_injection_type(argv);
    if (record.err_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID)
    {
        sprintf(temp,"Invalid error type %d received\n", 
            record.err_type);
        params[0] = '\0';
        call_post_processing(status, temp, key, params);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Copy input parameters
    sprintf(params_temp,
        "object id (%x), "
        "position bitmap (%x), "
        "lba (%llx), "
        "blocks (%llu), "
        "error_injection_type (%x) %s ,",
        object_id,
        record.pos_bitmap, 
        record.lba, 
        record.blocks, 
        record.err_type,
        edit_error_injection_type(record.err_type));

    // Get optional parameters
    status = get_optional_parameters(argv);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Make api call:
    status =  fbe_api_logical_error_injection_create_object_record(&record, object_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to create logical error injection record.");
    } else {
        sprintf(temp, "Logical error injection record created successfully.");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params_temp);

    return status;
}


/*********************************************************************
* sepLogicalErrorInjection::get_error_injection_enable(
*     int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Releases the IO hold.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/
fbe_status_t sepLogicalErrorInjection :: 
    get_logical_error_injection_enable( int argc , char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "Enable",
        "sepLogicalErrorInjection::get_error_injection_enable",
        "fbe_api_logical_error_injection_enable",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

     params[0] = '\0';

    // Make api call:
    status =  fbe_api_logical_error_injection_enable();

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to enale error injection.");
    } else {
        sprintf(temp, "error injection enabled  successfully.");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;

}

/*********************************************************************
* sepLogicalErrorInjection::get_logical_error_injection_disable(
* int argc , char ** argv)
*********************************************************************
*
*  Description:
*      disables the logical error injection
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/
fbe_status_t sepLogicalErrorInjection :: 
    get_logical_error_injection_disable( int argc , char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "Disable",
        "sepLogicalErrorInjection::get_logical_error_injection_disable",
        "fbe_api_logical_error_injection_disable",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

     params[0] = '\0';

    // Make api call:
    status =  fbe_api_logical_error_injection_disable();

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to disable logical error injection.");
    } else {
        sprintf(temp, "Logical error injection disabled successfully.");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepLogicalErrorInjection::get_logical_error_injection_enable_object(
* int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Enable lei on object.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/
fbe_status_t sepLogicalErrorInjection :: 
    get_logical_error_injection_enable_object(int argc , char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [object id]\n"
        " For example: %s  0x52";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "EnableObject",
        "sepLogicalErrorInjection::get_logical_error_injection_enable_object",
        "fbe_api_logical_error_injection_enable_object",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // get class id
    argv++;
    object_id = (fbe_class_id_t)strtoul(*argv, 0, 0);

   // Copy input parameters
    sprintf(params, "Object id (0x%x) " , object_id);
   
    // Make api call:
    status = fbe_api_logical_error_injection_enable_object(
        object_id, FBE_PACKAGE_ID_SEP_0);

     // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to enable logical error injection on "
            "object 0x%x", object_id);
    } else {
        sprintf(temp, "Logical error injection enabled on object 0x%x "
            "successfully.", object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;

}

/*********************************************************************
* sepLogicalErrorInjection::get_logical_error_injection_enable_class(
* int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Enable logical Error Injection at the gicen class
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/
fbe_status_t sepLogicalErrorInjection :: 
    get_logical_error_injection_enable_class(int argc , char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [class id]\n"
        " For example: %s  0x52";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "EnableClass",
        "sepLogicalErrorInjection::get_logical_error_injection_enable_class",
        "fbe_api_logical_error_injection_enable_class",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // get class id
    argv++;
    class_id = (fbe_class_id_t)strtoul(*argv, 0, 0);

   // Copy input parameters
    sprintf(params, "Class id (0x%x) " , class_id);
   
    // Make api call:
    status = fbe_api_logical_error_injection_enable_class(
        class_id, FBE_PACKAGE_ID_SEP_0);

     // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to enable logical error injection on "
            "class 0x%x", class_id);
    } else {
        sprintf(temp, "Logical error injection enabled on class 0x%x "
            "successfully.", class_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;

}

/*********************************************************************
* sepLogicalErrorInjection::get_logical_error_injection_disable_object
* (int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Disable logical Error Injection at the gicen object
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/
fbe_status_t sepLogicalErrorInjection :: 
    get_logical_error_injection_disable_object (int argc , char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [object id]\n"
        " For example: %s  0x52";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "DisableObject",
        "sepLogicalErrorInjection::get_logical_error_injection_disable_object",
        "fbe_api_logical_error_injection_disable_object",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // get class id
    argv++;
    object_id = (fbe_class_id_t)strtoul(*argv, 0, 0);

   // Copy input parameters
    sprintf(params, "Object id (%x) " , object_id);
   
    // Make api call:
    status = fbe_api_logical_error_injection_disable_object(
        object_id, FBE_PACKAGE_ID_SEP_0);

     // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to disable logical error injection on "
            "object 0x%x", object_id);
    } else {
        sprintf(temp, "Logical error injection disabled on object 0x%x "
            "successfully.", object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;

}


/*********************************************************************
* sepLogicalErrorInjection::get_logical_error_injection_disable_class(
* int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Disable logical Error Injection at the gicen class
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/
fbe_status_t sepLogicalErrorInjection :: 
    get_logical_error_injection_disable_class (int argc , char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [class id]\n"
        " For example: %s  0x52";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "DisableClass",
        "sepLogicalErrorInjection::get_logical_error_injection_disable_class",
        "fbe_api_logical_error_injection_disable_class",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // get class id
    argv++;
    class_id = (fbe_class_id_t)strtoul(*argv, 0, 0);

   // Copy input parameters
    sprintf(params, "Class id (0x%x) " , class_id);
   
    // Make api call:
    status = fbe_api_logical_error_injection_disable_class(
        class_id, FBE_PACKAGE_ID_SEP_0);

     // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to disable logical error injection on "
            "class 0x%x", class_id);
    } else {
        sprintf(temp, "Logical error injection disabled on class 0x%x "
            "successfully.", class_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;

}

/*********************************************************************
* sepLogicalErrorInjection :: get_logical_error_injection_stats(
* int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Get stats
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/
fbe_status_t sepLogicalErrorInjection :: 
    get_logical_error_injection_stats(int argc , char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "GetStats",
        "sepLogicalErrorInjection::get_logical_error_injection_stats",
        "fbe_api_logical_error_injection_get_stats",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

     params[0] = '\0';

    // Make api call:
    status = fbe_api_logical_error_injection_get_stats(&stats);

        // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get stats.");
    } else {
        edit_logical_error_injection_stats (stats);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepLogicalErrorInjection :: get_logical_error_injection_object_stats
* (int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Get object stats
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/
fbe_status_t sepLogicalErrorInjection :: 
    get_logical_error_injection_object_stats(int argc , char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [object_id]\n"
        " For example: %s  0x10f";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "GetObjStats",
        "sepLogicalErrorInjection::get_logical_error_injection_object_stats",
        "fbe_api_logical_error_injection_get_object_stats",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // get object id
    argv++;
    object_id = (fbe_object_id_t)strtoul(*argv, 0, 0);

   // Copy input parameters
    sprintf(params, "Object id (%x) " , object_id);


    // Make api call:
    status =  fbe_api_logical_error_injection_get_object_stats(
        &obj_stats, object_id);

        // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get stats on object %x.",object_id);
    } else {
        sprintf(temp,"\n"
        "<num_read_media_errors_injected>      %llu\n"
        "<num_write_verify_blocks_remapped>    %llu\n"
        "<num_errors_injected>                 %llu\n"
        "<num_validations>                     %llu",
        obj_stats.num_read_media_errors_injected,
        obj_stats.num_write_verify_blocks_remapped,
        obj_stats.num_errors_injected,
        obj_stats.num_validations);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepLogicalErrorInjection :: get_logical_error_injection_modify_record
* (int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Modify the record
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/
fbe_status_t sepLogicalErrorInjection :: 
    get_logical_error_injection_modify_record(int argc , char ** argv)
{
    strcpy(sub_usage_format1,
        " Usage: %s [record_handle] [position bitmap]  [lba] [blocks] [error_injection_type] \n"\
        "        [Optional:[width | d] [error_mode | d] [error count | d] [error limit | d] \n"\
        "        [skip count | d] [skip limit | d] [error adjusancy bitmask | d] [start bit of error | d]\n"\
        "        [number of bits of an error | d] [erroneous bits adjacent | d] [CRC detectable | d] [opcode | d]] \n"\
        " For example: %s 0 0x1 0x0 0x1 FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC d d d d d d d d d d d d  \n\n");

    usage_format = strcat(sub_usage_format1, sub_usage_format2);
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "ModifyRecord",
        "sepLogicalErrorInjection::get_logical_error_injection_modify_record",
        "fbe_api_logical_error_injection_modify_record",
        usage_format,
        argc, 23);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

     // Get record handle
     argv++;
     modify_record.record_handle = (fbe_u64_t)_strtoui64(
         *argv, 0, 0);

     
    // Get position bitmap
    argv++;
   record.pos_bitmap = (fbe_u16_t)strtoul(
        *argv, 0, 0);
    
    // To inject to position 0, then pos_bitmap is 1, position 1 is 2,
    // position 2 is 4, and position N is (1 << N).  
    // Check the position bitmap
    if(record.pos_bitmap  < 1){
        sprintf(temp,"Invalid position bitmap %d received\n", 
            record.pos_bitmap);
        params[0] = '\0';
        call_post_processing(status, temp, key, params);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //Get lba
    argv++;
    record.lba = (fbe_lba_t)_strtoui64(*argv, 0, 0);

    // Get blocks to extend the errors
    argv++;
    record.blocks = (fbe_block_count_t)_strtoui64(
        *argv, 0, 0);

    // Take error type
    argv++;

    record.err_type= get_err_injection_type(argv);
    if (modify_record.modify_record.err_type == 
        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID)
    {
        sprintf(temp,"Invalid error type %d received\n", 
            modify_record.modify_record.err_type);
        params[0] = '\0';
        call_post_processing(status, temp, key, params);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    // Copy input parameters
    sprintf(params_temp,  "record handle  (%llx), "
        "position bitmap (%x), "
        "lba (%llx), "
        "blocks (%llu), "
        "error_injection_type (%x) %s",
        (unsigned long long)modify_record.record_handle,
        record.pos_bitmap, 
        record.lba, 
        record.blocks, 
        record.err_type,
        edit_error_injection_type(record.err_type));


    // Get optional parameters
    status = get_optional_parameters(argv);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    modify_record.modify_record = record;

    // Make api call:
    status =  fbe_api_logical_error_injection_modify_record(
        &modify_record);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to modify logical error injection record.");
    } else {
        sprintf(temp, "Logical error injection record modified successfully.");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;

}

/*********************************************************************
* sepLogicalErrorInjection::get_logical_error_injection_delete_record(
* int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Delete teh record
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/
fbe_status_t sepLogicalErrorInjection :: 
    get_logical_error_injection_delete_record(int argc , char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [record_handle]\n"
        " For example: %s  0\n"
        "[record_handle] is an index";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "DeleteRecord",
        "sepLogicalErrorInjection::get_logical_error_injection_delete_record",
        "fbe_api_logical_error_injection_delete_record",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // get record handle
    argv++;
    del_record.record_handle = (fbe_u64_t)_strtoui64(*argv, 0, 0);

   // Copy input parameters
    sprintf(params, "record handle (%llu)" , del_record.record_handle);


    // Make api call:
    status =  fbe_api_logical_error_injection_delete_record(&del_record);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to delete logical error injection record.");
    } else {
        sprintf(temp, "Logical error injection record deleted successfully.");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepLogicalErrorInjection::get_logical_error_injection_disable_records
* (int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Disable the records
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/
fbe_status_t sepLogicalErrorInjection :: 
    get_logical_error_injection_disable_records(int argc , char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [start_index] [num_to_clear]\n"
        " For example: %s  0 128";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "DisableRecords",
        "sepLogicalErrorInjection::get_logical_error_injection_disable_records",
        "fbe_api_logical_error_injection_disable_records",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    //Get start index
    argv++;
    start_index = (fbe_u32_t)strtoul(*argv, 0, 0);

    // Get number of records to disable
    argv++;
    num_to_clear = (fbe_u32_t)strtoul(*argv, 0, 0);

   // Copy input parameters
    sprintf(params, "start index (%d) , num to clear (%d)" , 
        start_index, num_to_clear);
   
    // Make api call:
    status = fbe_api_logical_error_injection_disable_records(
        start_index, num_to_clear);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to disable logical error injection records.");
    } else {
        sprintf(temp, "Logical error injection records disabled successfully.");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepLogicalErrorInjection::get_logical_error_injection_get_records(
* int argc , char ** argv)
*********************************************************************
*
*  Description:
*      Get the records
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/
fbe_status_t sepLogicalErrorInjection :: 
    get_logical_error_injection_get_records(int argc , char ** argv)
{
    int temp_record = 0;
    char *data = NULL;
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "GetRecords",
        "sepLogicalErrorInjection::get_logical_error_injection_get_records",
        "fbe_api_logical_error_injection_get_records",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

     params[0] = '\0';

    // Make api call:
    status =  fbe_api_logical_error_injection_get_records(&get_records);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get logical error injection records.");
    } else {

        sprintf(temp,"<Records>\n");

         data = utilityClass::append_to (data, temp);

        for (temp_record = 0; temp_record<get_records.num_records; temp_record++)
        {
            sprintf(temp,"\n"
                "<num_record>                   %d\n"
                " <position bitmap>             %x\n"
                " <width>                       %x\n"
                " <lba>                         %llx\n"
                " <blocks>                      %llu\n"
                " <error_injection_type>        %s\n"
                " <error_mode>                  %s\n"
                " <error count>                 %x\n"
                " <error limit>                 %x\n"
                " <skip count>                  %x\n"
                " <skip limit>                  %x\n"
                " <error adjusancy bitmask>     %x\n"
                " <start bit of error>          %x\n"
                " <number of bits of an error>  %x\n"
                " <erroneous bits adjacent>     %x\n"
                " <CRC detectable>              %x\n"
                " <opcode>                      %x\n",
                temp_record,
                get_records.records[temp_record].pos_bitmap,
                get_records.records[temp_record].width,
                get_records.records[temp_record].lba,
                get_records.records[temp_record].blocks,
                edit_error_injection_type(get_records.records[temp_record].err_type),
                edit_error_injection_mode(get_records.records[temp_record].err_mode),
                get_records.records[temp_record].err_count,
                get_records.records[temp_record].err_limit,
                get_records.records[temp_record].skip_count,
                get_records.records[temp_record].skip_limit,
                get_records.records[temp_record].err_adj,
                get_records.records[temp_record].start_bit,
                get_records.records[temp_record].num_bits,
                get_records.records[temp_record].bit_adj,
                get_records.records[temp_record].crc_det,
                get_records.records[temp_record].opcode);

                data = utilityClass::append_to (data, temp);

        }
        
    }

    // Output results from call or description of error
    call_post_processing(status, data, key, params);

    return status;
}


/*********************************************************************
* sepLogicalErrorInjection::get_err_injection_type(
*     fbe_u32_t error_injection_type)
*********************************************************************
*
*  Description:
*      Get the Error injection type.
*
*  Input: Parameters
*      error_injection_type -error injection type
*
*  Returns:
*      Error injection type
*********************************************************************/
fbe_api_logical_error_injection_type_t sepLogicalErrorInjection :: 
    get_err_injection_type(char** argv)
{

    if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RAID_CRC")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RAID_CRC;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SINGLE_BIT_CRC")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SINGLE_BIT_CRC;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CORRUPT_CRC")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CORRUPT_CRC;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CORRUPT_DATA")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CORRUPT_DATA;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SHED_STAMP")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SHED_STAMP;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_WRITE_STAMP")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_WRITE_STAMP;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_TIME_STAMP")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_TIME_STAMP;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_SHED_STAMP")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_SHED_STAMP;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1NS")==0){
        return  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1NS;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1S")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1S;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1R")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1R;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1D")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1D;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1COD")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1COD;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1COP")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1COP;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1POC")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1POC;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_COH")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_COH;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SILENT_DROP")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SILENT_DROP;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALIDATED")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALIDATED;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_SEQ_NUM_MISMATCH")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_SEQ_NUM_MISMATCH;
     }
     else if (strcmp(*argv,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_UP")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_UP;
    }
    else if (strcmp(*argv,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN;
    }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_IO_UNEXPECTED")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_IO_UNEXPECTED;
     }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INCOMPLETE_WRITE")==0){
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INCOMPLETE_WRITE;
     }
    else {
        return FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID;
    }
}


/*********************************************************************
* sepLogicalErrorInjection::edit_logical_error_injection_stats(
*     fbe_api_logical_error_injection_get_stats_t stats)
*********************************************************************
*
*  Description:
*      Edit the stats
*
*  Input: Parameters
*      fbe_api_logical_error_injection_get_stats_t - stats structure
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/
void sepLogicalErrorInjection :: edit_logical_error_injection_stats (
    fbe_api_logical_error_injection_get_stats_t stats)
{
    sprintf(temp, "  \n"
        "<b_enabled>       %s\n "
        "<num_objects>                    %d\n "
        "<num_records>                    %d\n "
        "<num_objects_enabled>            %d\n "
        "<num_errors_injected>            %llu\n "
        "<correctable_errors_detected>    %llu\n "
        "<uncorrectable_errors_detected>  %llu\n "
        "<num_validations>                %llu\n "
        "<num_failed_validations>         %llu\n ",
        get_true_or_false(stats.b_enabled),
        stats.num_objects,
        stats.num_records,
        stats.num_objects_enabled,
        stats.num_errors_injected,
        stats.correctable_errors_detected,
        stats.uncorrectable_errors_detected,
        stats.num_validations,
        stats.num_failed_validations);
}

/*********************************************************************
* sepLogicalErrorInjection::get_true_or_false(
*     fbe_bool_t b_enabled)
*********************************************************************
*
*  Description:
*      Edit the stats
*
*  Input: Parameters
*       b_enabled - service injection binary variable
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/
char* sepLogicalErrorInjection :: get_true_or_false(fbe_bool_t b_enabled)
{
    if (b_enabled == FBE_TRUE) {
        return "True";
    }
    else{
        return "False";
    }
}

/*********************************************************************
* sepLogicalErrorInjection::edit_error_injection_type(
*     fbe_api_logical_error_injection_type_t type)
*********************************************************************
*
*  Description:
*      Edit the logical error injection type
*
*  Input: Parameters
*       mode -logical error injection type
*
*  Returns:
*      char* - the type in string form
*********************************************************************/
char * sepLogicalErrorInjection:: edit_error_injection_type (fbe_api_logical_error_injection_type_t type)
{
    switch(type){
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RAID_CRC:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RAID_CRC");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SINGLE_BIT_CRC:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SINGLE_BIT_CRC");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CORRUPT_CRC:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CORRUPT_CRC");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CORRUPT_DATA:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CORRUPT_DATA");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SHED_STAMP:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SHED_STAMP");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_WRITE_STAMP:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_WRITE_STAMP");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_TIME_STAMP:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_TIME_STAMP");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_SHED_STAMP:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_SHED_STAMP");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1NS:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1NS");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1S:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1S");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1R:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1R");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1D:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1D");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1COD:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1COD");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1COP:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1COP");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1POC:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1POC");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_COH:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_COH");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SILENT_DROP:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SILENT_DROP");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALIDATED:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALIDATED");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_SEQ_NUM_MISMATCH:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_SEQ_NUM_MISMATCH");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_UP:
            sprintf(lei_type, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_UP");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN:
            sprintf(lei_type, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_IO_UNEXPECTED:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_IO_UNEXPECTED");
            break;
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INCOMPLETE_WRITE:
            sprintf(lei_type,"FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INCOMPLETE_WRITE");
            break;
        default:
            sprintf(lei_type, "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALIDATED");
    }
    return lei_type;
}

/*********************************************************************
* sepLogicalErrorInjection::edit_error_injection_mode(
*     fbe_api_logical_error_injection_mode_t mode)
*********************************************************************
*
*  Description:
*      Edit the logical error injection mode
*
*  Input: Parameters
*       mode -logical error injection mode
*
*  Returns:
*      char* - the mode in string form
*********************************************************************/
char* sepLogicalErrorInjection:: edit_error_injection_mode(fbe_api_logical_error_injection_mode_t mode){
    switch(mode){
        case FBE_API_LOGICAL_ERROR_INJECTION_MODE_COUNT:
            sprintf(lei_mode,"FBE_API_LOGICAL_ERROR_INJECTION_MODE_COUNT");
            break;

        case FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS:
            sprintf(lei_mode,"FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS");
            break;

        case FBE_API_LOGICAL_ERROR_INJECTION_MODE_RANDOM:
            sprintf(lei_mode,"FBE_API_LOGICAL_ERROR_INJECTION_MODE_RANDOM");
            break;

        case FBE_API_LOGICAL_ERROR_INJECTION_MODE_SKIP:
            sprintf(lei_mode,"FBE_API_LOGICAL_ERROR_INJECTION_MODE_SKIP");
            break;

        case FBE_API_LOGICAL_ERROR_INJECTION_MODE_SKIP_INSERT:
            sprintf(lei_mode,"FBE_API_LOGICAL_ERROR_INJECTION_MODE_SKIP_INSERT");
            break;

        case FBE_API_LOGICAL_ERROR_INJECTION_MODE_TRANS:
            sprintf(lei_mode,"FBE_API_LOGICAL_ERROR_INJECTION_MODE_TRANS");
            break;

        case FBE_API_LOGICAL_ERROR_INJECTION_MODE_TRANS_RND:
            sprintf(lei_mode,"FBE_API_LOGICAL_ERROR_INJECTION_MODE_TRANS_RND");
            break;

        case FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED:
            sprintf(lei_mode,"FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED");
            break;

        case FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_SAME_LBA:
            sprintf(lei_mode,"FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_SAME_LBA");
            break;

        default:
            sprintf(lei_mode,"FBE_API_LOGICAL_ERROR_INJECTION_MODE_INVALID");

    }
    return lei_mode;
}

/*********************************************************************
* sepLogicalErrorInjection::get_error_mode (fbe_u32_t  error_mode)
*********************************************************************
*
*  Description:
*      Get the error mode
*
*  Input: Parameters
*      *argv - pointer to arguments
*
*  Returns:
*      Error injection mode
*********************************************************************/

fbe_api_logical_error_injection_mode_t sepLogicalErrorInjection :: get_error_mode (char** argv){

    if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_MODE_COUNT")== 0){
        return FBE_API_LOGICAL_ERROR_INJECTION_MODE_COUNT;
    }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS")== 0){
        return FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS;
    }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_MODE_RANDOM")== 0){
        return FBE_API_LOGICAL_ERROR_INJECTION_MODE_RANDOM;
    }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_MODE_SKIP")== 0){
        return FBE_API_LOGICAL_ERROR_INJECTION_MODE_SKIP;
    }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_MODE_SKIP_INSERT")== 0){
        return FBE_API_LOGICAL_ERROR_INJECTION_MODE_SKIP_INSERT;
    }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_MODE_TRANS")== 0){
        return FBE_API_LOGICAL_ERROR_INJECTION_MODE_TRANS;
    }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_MODE_TRANS_RND")== 0){
        return FBE_API_LOGICAL_ERROR_INJECTION_MODE_TRANS_RND;
    }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED")== 0){
        return FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED;
    }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_SAME_LBA")== 0){
        return FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_SAME_LBA;
    }
    else if (strcmp(*argv, "FBE_API_LOGICAL_ERROR_INJECTION_MODE_UNKNOWN")== 0){
        return FBE_API_LOGICAL_ERROR_INJECTION_MODE_UNKNOWN;
    }
    else {
        return FBE_API_LOGICAL_ERROR_INJECTION_MODE_INVALID;
    }
    
}

/*********************************************************************
* sepLogicalErrorInjection::get_optional_parameters(char **argv)
*********************************************************************
*
*  Description:
*      Get optional parameters
*
*  Input: Parameters
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*********************************************************************/

fbe_status_t sepLogicalErrorInjection :: get_optional_parameters(char **argv)
{
    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    // take width
    if (strcmp(*argv, "D")) {
    record.width = (fbe_u16_t)strtoul(*argv, 0, 0);
    }
    else {
        record.width = 0x10;
    }
    sprintf(params,  "width (%x), ",  record.width);
    strcat(params_temp, params);

    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    //take error mode
    if (strcmp(*argv, "D")) {

        // extract error mode
        record.err_mode = get_error_mode(argv);
        if (record.err_mode== FBE_API_LOGICAL_ERROR_INJECTION_MODE_UNKNOWN)
        {
            sprintf(temp,"invalid error mode %d received\n", 
                record.err_mode);
            call_post_processing(status, temp, key, params);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else {
        record.err_mode = FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS;
    }
    sprintf(params,  "error_mode (%x) %s, ", record.err_mode, edit_error_injection_mode(record.err_mode));
    strcat(params_temp, params);

    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    //Get error count
    if (strcmp(*argv, "D")) {
        record.err_count= (fbe_u16_t)strtoul(*argv, 0, 0);
    }
    else {
        record.err_count = 0x0;
    }
    sprintf(params, "error count (%x), ", record.err_count);
    strcat(params_temp, params);


    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    //Get error limit
    if (strcmp(*argv, "D")) {
        record.err_limit = (fbe_u16_t)strtoul(*argv, 0, 0);
    }
    else {
        record.err_limit = 0x15;
    }
    sprintf(params,  "error limit (%x), ", record.err_limit);
    strcat(params_temp, params);


    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

     //Get skip count
     if (strcmp(*argv, "D")) {
        record.skip_count = (fbe_u16_t)strtoul(*argv, 0, 0);
    }
    else {
        record.skip_count = 0x0;
    }
    sprintf(params,  "skip count (%x), ",  record.skip_count);
    strcat(params_temp, params);


    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    //Get skip limit
    if (strcmp(*argv, "D")) {
        record.skip_limit = (fbe_u16_t)strtoul(*argv, 0, 0);
    }
    else {
        record.skip_limit = 0x15;
    }
    sprintf(params,  "skip limit (%x), ", record.skip_limit);
    strcat(params_temp, params);


    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    //Get error adjacency
    if (strcmp(*argv, "D")) {
        record.err_adj = (fbe_u16_t)strtoul(*argv, 0, 0);
    }
    else {
        record.err_adj = 0x1;
    }
    sprintf(params,  "error adjutancy  bitmask (%x), ", record.err_adj);
    strcat(params_temp, params);

    
    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    //Get start bit
    if (strcmp(*argv, "D")) {
         record.start_bit = (fbe_u16_t)strtoul(*argv, 0, 0);
    }
    else {
        record.start_bit = 0x0;
    }
    sprintf(params,  "start bit of error (%x), ",  record.start_bit);
    strcat(params_temp, params);


    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    //Get number of bits
    if (strcmp(*argv, "D")) {
        record.num_bits = (fbe_u16_t)strtoul(*argv, 0, 0);
    }
    else {
        record.num_bits = 0x0;
    }
    sprintf(params,  "number of bits of an error (%x), ", record.num_bits);
    strcat(params_temp, params);


    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    //Get bit adjacent
    if (strcmp(*argv, "D")) {
    record.bit_adj= (fbe_u16_t)strtoul(*argv, 0, 0);
    }
    else {
        record.bit_adj = 0x0;
    }
    sprintf(params, "erroneous bits adjacent (%x), ",  record.bit_adj );
    strcat(params_temp, params);

    
    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    //Get CRC
    if (strcmp(*argv, "D")) {
        record.crc_det = (fbe_u16_t)strtoul(*argv, 0, 0);
    }
    else {
        record.crc_det = 0x0;
    }
    sprintf(params, "CRC detectable (%x), ",  record.crc_det );
    strcat(params_temp, params);


    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    //Get opcode
    if (strcmp(*argv, "D")) {
        record.opcode = (fbe_u32_t)strtoul(*argv, 0, 0);
    }
    else {
        record.opcode = 0;
    }
    sprintf(params,  "opcode (%x)", record.opcode);
    strcat(params_temp, params);

    return FBE_STATUS_OK;
}
/*********************************************************************
* sepLogicalErrorInjection end of file
*********************************************************************/
