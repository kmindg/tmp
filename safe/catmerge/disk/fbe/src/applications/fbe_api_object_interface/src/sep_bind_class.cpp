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
*      This file defines the methods of the SEP BIND INTERFACE class.
*
*  Notes:
*      The SEP BIND class (sepBind) is a derived class of
*      the base class (bSEP).
*
*  History:
*      07-March-2011 : initial version
*
*********************************************************************/

#ifndef T_SEP_BIND_CLASS_H
#include "sep_bind_class.h"
#endif

/*********************************************************************
* sepBind class :: Constructor
*********************************************************************/

sepBind::sepBind() : bSEP()
{
    // Store Object number
    idnumber = (unsigned) SEP_BIND_INTERFACE;
    sepBindCount = ++gSepBindCount;

    // Store Object name
    idname = new char [64];
    strcpy(idname, "SEP_BIND_INTERFACE");

    // assign default values
    sep_bind_initializing_variables();

    if (Debug) {
        sprintf(temp, "sepBind class constructor (%d) %s\n",
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP BIND Interface function calls>
    sepBindInterfaceFuncs =
        "[function call]\n" \
        "[short name] [FBE API SEP BIND Interface]\n" \
        " --------------------------------------------------------\n" \
        " bindRg    fbe_api_create_lun\n" \
        " --------------------------------------------------------\n";
};

/*********************************************************************
* sepBind class : Accessor methods
*********************************************************************/

unsigned sepBind::MyIdNumIs(void)
{
    return idnumber;
};

char * sepBind::MyIdNameIs(void)
{
    return idname;
};

void sepBind::dumpme(void)
{
    strcpy (key, "sepBind::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n",
            idnumber, sepBindCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* sepBind Class :: select()
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

fbe_status_t sepBind::select(int index, int argc, char *argv[])
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface");
    c = index;

    // List SEP BIND Interface calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepBindInterfaceFuncs );

        // bind raid group
    } else if (strcmp (argv[index], "BINDRG") == 0) {
        status = bind_rg(argc, &argv[index]);

        // can't find match on short name
    }else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n",
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepBindInterfaceFuncs );
    }

    return status;
}

/*********************************************************************
* sepBind class :: bind_rg ()
*********************************************************************/

fbe_status_t sepBind::bind_rg(int argc , char ** argv)
{
    // Define specific usage string
    usage_format =
        " Usage: %s [rg id] [Optional: [capacity: value|d] \n"
        " [size unit: GB|MB|BL|d] [ndb: 1|0|d] \n"
        " [address offset: value|d] [lun number: value|d] \n"
        " [placement: value|d] [wwn: value|d] [count: value|d]] \n"
        " d = default value \n"
        " For example: %s 4 5000 MB d d d d d d\n";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "bindRg",
        "sepBind::bind_rg",
        "fbe_api_create_lun",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Extract raid group id hex
    argv++;
    raid_group_num = (fbe_u32_t)atoi(*argv);
    sprintf(params, " rg num (0x%x)", raid_group_num);

    char *data = NULL;
    // bind the LUN with the given parameters
    status = _bind(argv);
    data = utilityClass::append_to (data, temp);

    if (status != FBE_STATUS_OK){
        sprintf(temp, " Failed to bind LUN with the given parameters.\n");
        data = utilityClass::append_to (data, temp);
    }

    // Output results from call or description of error
    call_post_processing(status, data, key, params);

    return status;
}

/*********************************************************************
* sepBind class :: validate_lun_request (private method)
*********************************************************************/

fbe_status_t sepBind::validate_lun_request(
    fbe_api_lun_create_t *lun_create_req)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Make sure that LUN number is in range */
    if (lun_create_req->lun_number > MAX_LUN_ID) {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*********************************************************************
* sepBind class :: find_smallest_free_lun_number (private method)
*********************************************************************/

fbe_status_t sepBind::find_smallest_free_lun_number(
    fbe_lun_number_t* lun_number_p)
{
    fbe_u32_t i = 0;
    fbe_object_id_t object_id;

    if (lun_number_p == NULL){
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    for(i = 0; i <= MAX_LUN_ID;i++){
        if(fbe_api_database_lookup_lun_by_number(i, &object_id) ==
            FBE_STATUS_OK){
            continue;
        }
        else{
            *lun_number_p = i;
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_INSUFFICIENT_RESOURCES;
}

/*********************************************************************
* sepBind class :: convert_str_to_lba (public method)
*********************************************************************/

fbe_lba_t sepBind::convert_str_to_lba(fbe_u8_t *str)
{
    fbe_lba_t lba_val = 0;

    if (strlen((const char*)str) == 0)
        return FBE_LBA_INVALID;

    while(*str != '\0')
    {
        if (!isdigit(*str))
        {
            return FBE_LBA_INVALID;
        }
        lba_val *= 10;
        lba_val += *str - '0';
        str++;
    }

    return lba_val;
}

/*********************************************************************
* sepBind class :: convert_size_unit_to_lba (private method)
*********************************************************************/

fbe_lba_t sepBind::convert_size_unit_to_lba(
    fbe_lba_t unit_size, fbe_u8_t *str)
{
    fbe_lba_t calculated_size = 0;
    if (unit_size <= 0){
          return calculated_size;
    }
    if ((strcmp((const char*)str, "MB") == 0)){
        calculated_size =( unit_size * FBE_MEGABYTE) / 520;
    }
    else if ((strcmp((const char*)str, "GB") == 0)){
        calculated_size =( unit_size * FBE_GIGABYTE) / 520;
    }
    else if((strcmp((const char*)str, "BL") == 0)){
        //Nothing to do
        calculated_size = unit_size;
    }

    return calculated_size;
}

/*********************************************************************
* sepBind class :: validate_atoi_args (private method)
*********************************************************************/

fbe_bool_t sepBind::validate_atoi_args(const TEXT * arg_value)
{
    fbe_bool_t arg_is_valid = TRUE;
    fbe_u32_t loop_index;

    if(arg_value == NULL){
        return FBE_FALSE;
    }

    for (loop_index = 0; loop_index < strlen(arg_value); loop_index++){
        if (!isxdigit(arg_value[loop_index])){
            arg_is_valid = FALSE;
        }
    }

    return arg_is_valid;
}

/*********************************************************************
* sepBind class :: checks_for_similar_wwn_number (private method)
*********************************************************************/

fbe_bool_t sepBind::checks_for_similar_wwn_number(
    fbe_assigned_wwid_t wwn)
{
    fbe_status_t    status;
    fbe_u32_t       i;
    fbe_u32_t       total_luns_in_system;
    fbe_object_id_t *object_ids;
    lun_details_s   lun_details;
    fbe_bool_t      wwn_flag = FBE_FALSE;

    // get the number of LUNs in the system
    status = fbe_api_enumerate_class(
        FBE_CLASS_ID_LUN,
        FBE_PACKAGE_ID_SEP_0,
        &object_ids,
        &total_luns_in_system);
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Failed to enumerate classes");
        return FBE_FALSE;
    }

    // skip the first LUN for now as it is private/sysem LUN
    for (i = 1; i < total_luns_in_system; i++){

        // get the LUN details based on object ID of LUN
        status = get_lun_details(object_ids[i], &lun_details);

        if (status != FBE_STATUS_OK){
            // error in getting the details skip to next object
            continue;
        }

        if(!strncmp(
            (const char*) wwn.bytes,
            (const char*)lun_details.lun_info.world_wide_name.bytes,
            16)){
            wwn_flag = FBE_TRUE;
            return wwn_flag;
        }
    }

    fbe_api_free_memory(object_ids);

    return wwn_flag;
}

/*********************************************************************
* sepBind class :: get_lun_details (private method)
*********************************************************************/

fbe_status_t sepBind::get_lun_details(
                                      fbe_object_id_t lun_id,
                                      lun_details_t* lun_details)
{
    fbe_status_t status;

    status = fbe_api_database_lookup_lun_by_object_id(
                                  lun_id, &lun_details->lun_number);
    if (status == FBE_STATUS_GENERIC_FAILURE){
        // The LUN does not exist
        return status;
    }

    status = fbe_api_get_object_lifecycle_state(
        lun_id,
        &lun_details->lifecycle_state,
        FBE_PACKAGE_ID_SEP_0);

    lun_details->p_lifecycle_state_str = utilityClass::lifecycle_state(
        lun_details->lifecycle_state);

    lun_details->lun_info.lun_object_id = lun_id;

    status = fbe_api_database_get_lun_info(
        &lun_details->lun_info);
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        // not able to retrieve LUN details
        return status;
    }

    status = fbe_api_lun_get_zero_status(
        lun_id,
        &lun_details->fbe_api_lun_get_zero_status);
    if (status == FBE_STATUS_GENERIC_FAILURE){
        // not able to retrieve LUN details anyways
        return status;
    }

    lun_details->p_rg_str = utilityClass::convert_rg_type_to_string(
        lun_details->lun_info.raid_info.raid_type);

    return status;
}

/*********************************************************************
* sepBind class :: extract_optional_arguments (private method)

* This method will extract the optional arguments to bind command and
* store them into class variables.
*********************************************************************/

fbe_status_t sepBind::extract_optional_arguments(char **argv)
{
    // check if optional arguments are entered
    // if argv is d then default value is taken.

    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    // extract lun capacity
    if (strcmp(*argv, "D")){
        lun_capacity = convert_str_to_lba((fbe_u8_t *)*argv);
    }

    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    // extract lun size unit
    if (strcmp(*argv, "D")){

        if(!strcmp(*argv,"GB") || !strcmp(*argv,"Gb")||
            !strcmp(*argv,"gb")|| !strcmp(*argv,"gB")){

                lun_capacity = convert_size_unit_to_lba(
                    lun_capacity,
                    (fbe_u8_t*)"GB");
        }
        else if( !strcmp(*argv,"MB") || !strcmp(*argv,"Mb")||
            !strcmp(*argv,"mb")|| !strcmp(*argv,"mB")) {
            lun_capacity = convert_size_unit_to_lba(
                lun_capacity, (fbe_u8_t*)"MB");
        }
        else if(!strcmp(*argv,"BL") || !strcmp(*argv,"bL") ||
            !strcmp(*argv,"Bl") || !strcmp(*argv,"bl")){
            lun_capacity = convert_size_unit_to_lba(
                lun_capacity, (fbe_u8_t*)"BL");
        }
        else{
            sprintf(temp, "Unknown size_unit specified.\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (lun_capacity <= 0){
            sprintf(temp, "LUN capacity argument is expected.\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        sprintf(params_temp, " lun capacity (0x%llx), ",
		(unsigned long long)lun_capacity);
        if(strlen(params_temp) > MAX_PARAMS_SIZE)
        {
            sprintf(temp, "<ERROR!> Params length is larger that buffer value");
            status = FBE_STATUS_GENERIC_FAILURE;
            call_post_processing(status, temp, key, params);
            return status;
        }
        strcat(params, params_temp);
    }

    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    // extract ndb value
    if (strcmp(*argv, "D")){
        is_ndb_req = atoi(*argv);
        sprintf(params_temp, "ndb (0x%x), ", (int) is_ndb_req);
        if(strlen(params_temp) > MAX_PARAMS_SIZE)
        {
            sprintf(temp, "<ERROR!> Params length is larger that buffer value");
            status = FBE_STATUS_GENERIC_FAILURE;
            call_post_processing(status, temp, key, params);
            return status;
        }
        strcat(params, params_temp);
    }

    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    // extract address offset
    if (strcmp(*argv, "D")){
        addr_offset = convert_str_to_lba((fbe_u8_t*)*argv);
        sprintf(params_temp, "address offset (0x%llx), ",
		(unsigned long long)addr_offset);
        if(strlen(params_temp) > MAX_PARAMS_SIZE)
        {
            sprintf(temp, "<ERROR!> Params length is larger that buffer value");
            status = FBE_STATUS_GENERIC_FAILURE;
            call_post_processing(status, temp, key, params);
            return status;
        }
        strcat(params, params_temp);
    }

    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    // extract lun number
    if (strcmp(*argv, "D")){
        lun_no = atoi(*argv);
        sprintf(params_temp, "lun number (0x%x), ", lun_no);
        if(strlen(params_temp) > MAX_PARAMS_SIZE)
        {
            sprintf(temp, "<ERROR!> Params length is larger that buffer value");
            status = FBE_STATUS_GENERIC_FAILURE;
            call_post_processing(status, temp, key, params);
            return status;
        }
        strcat(params, params_temp);
    }

    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    // extract placement
    if (strcmp(*argv, "D")){
        block_transport_placement =
            (fbe_block_transport_placement_t) atoi(*argv);
        sprintf(params_temp,
            "placement (0x%x), ", block_transport_placement);
        if(strlen(params_temp) > MAX_PARAMS_SIZE)
        {
            sprintf(temp, "<ERROR!> Params length is larger that buffer value");
            status = FBE_STATUS_GENERIC_FAILURE;
            call_post_processing(status, temp, key, params);
            return status;
        }
        strcat(params, params_temp);
    }

    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    // extract wwn
    if (strcmp(*argv, "D")){
        strncpy((char *)wwn.bytes, *argv, 16);
        if((strlen(*argv) != 16) ||
            (!validate_atoi_args((const TEXT *) wwn.bytes))){
            sprintf(temp,
                "Specified wwn number %s is invalid.\n", *argv);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if(checks_for_similar_wwn_number(wwn))
        {
            sprintf(temp,
                "Specified wwn number %s is already assigned.\n",
                *argv);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        strcat(params, "wwn (");
        for (int i = 0; i < 16; i++){
            sprintf(params_temp, "%c", wwn.bytes[i]);
        }
        if(strlen(params_temp) > MAX_PARAMS_SIZE)
        {
            sprintf(temp, "<ERROR!> Params length is larger that buffer value");
            status = FBE_STATUS_GENERIC_FAILURE;
            call_post_processing(status, temp, key, params);
            return status;
        }
        strcat(params, params_temp);
        strcat(params, "), ");
    }

    argv++;
    if (!(*argv)){
        return FBE_STATUS_OK;
    }

    // extract count
    if (strcmp(*argv, "D")){
        cnt = atoi(*argv);
        sprintf(params_temp, "count (0x%x)", cnt);
        if(strlen(params_temp) > MAX_PARAMS_SIZE)
        {
            sprintf(temp, "<ERROR!> Params length is larger that buffer value");
            status = FBE_STATUS_GENERIC_FAILURE;
            call_post_processing(status, temp, key, params);
            return status;
        }
        strcat(params, params_temp);
    }

    return FBE_STATUS_OK;
}

/*********************************************************************
* sepBind class :: _bind (private method)

* This method will perform the bind command.
* It calls the extracts the arguments, validates if the given RG exists
* finds the LUN number (if not given). Basically it populates the 
* data structure "lun_create_req" which is then used by fbe_api_create_lun
* to bind the lun.
*********************************************************************/

fbe_status_t sepBind::_bind(char **argv)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_job_service_error_type_t job_error_type;
    // extract optional arguments
    argv++;
    status = extract_optional_arguments(argv);
    if (status != FBE_STATUS_OK){
        // appropriate error messege is set in called function
        return status;
    }

    // check if rg number exists on the system
    status = fbe_api_database_lookup_raid_group_by_number(
                                raid_group_num, &rg_object_id);

    if (status != FBE_STATUS_OK){
        sprintf(temp, "RG %d does not exist.\n", raid_group_num);
        return status;
    }

    // find the smallest free lun number
    if(lun_no == FBE_OBJECT_ID_INVALID){

        status = find_smallest_free_lun_number(&lun_no);
        if (status == FBE_STATUS_INSUFFICIENT_RESOURCES){
            sprintf(temp,
                "Maximum number of LUN (%d) exists. "\
                "LUN creation is not allowed. %d\n",
                MAX_LUN_ID, status);
            return status;
        }
    }

    // get raid group info
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info_p,
             FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK){
        sprintf(temp,
            "Unable to obtain the information for RG %d.\n",
            raid_group_num);
        return status;
    }

    // Set LUN capacity to RG capacity
    // when user does not specify the LUN capacity
    if (lun_capacity == FBE_LBA_INVALID){
        // set LUN capacity to maximum LUN size this RG can handle
        lun_capacity = raid_group_info_p.capacity - lun_cap_offset - 1;
    }

    // assign values to lun creation request
    lun_create_req.raid_group_id = raid_group_num;
    lun_create_req.capacity = lun_capacity;
    lun_create_req.raid_type = raid_group_info_p.raid_type;
    lun_create_req.ndb_b = is_ndb_req;
    lun_create_req.addroffset = addr_offset;
    lun_create_req.lun_number = lun_no;
    lun_create_req.placement = block_transport_placement;
    lun_create_req.world_wide_name = wwn;

    for(index = 0 ; index < cnt ; index ++)
    {
        status = validate_lun_request(&lun_create_req);
        if (status != FBE_STATUS_OK){
            sprintf(temp,
                "Error in command line parameters "\
                "for LUN creation request.\n");
            return status;
        }

        status = fbe_api_create_lun(
            &lun_create_req, FBE_TRUE, LU_READY_TIMEOUT, &lu_id, &job_error_type);
        if (status == FBE_STATUS_OK){
            sprintf(temp,
                "Created LU :%d with object ID:0x%X\n",
                lun_create_req.lun_number ,lu_id);
        }
        else if (status == FBE_STATUS_TIMEOUT){
            sprintf(temp, "Timed out waiting for LU to become ready.\n");
            return status;
        }
        else{
            sprintf(temp,
                "LU creation failed: error code: %d\n", status);
            return status;
        }

        status = find_smallest_free_lun_number(&lun_no);
        if (status == FBE_STATUS_INSUFFICIENT_RESOURCES){
            sprintf(temp, "Maximum number of LUN (%d) exists.\
                          LUN creation is not allowed. %d\n",
                          MAX_LUN_ID, status);
            return status;
        }
        lun_create_req.lun_number = lun_no;
    }

    return status;
}


/*********************************************************************
* sepBind class ::sep_bind_initializing_variables (private method)
*********************************************************************/
void sepBind :: sep_bind_initializing_variables()
{

    fbe_zero_memory(&lun_create_req, sizeof(lun_create_req));
    fbe_zero_memory(&wwn,sizeof(wwn));
    fbe_zero_memory(&raid_group_info_p,sizeof(raid_group_info_p));

    raid_group_num = FBE_OBJECT_ID_INVALID;
    rg_object_id = FBE_OBJECT_ID_INVALID;

    lun_capacity = FBE_LBA_INVALID;
    is_ndb_req = 0;
    addr_offset = FBE_LBA_INVALID;
    lun_no = FBE_OBJECT_ID_INVALID;

    is_create_rg_b = FBE_FALSE;

    lun_cap_offset = FBE_LUN_CHUNK_SIZE;
    lu_id = FBE_OBJECT_ID_INVALID;
    index = 0;
    cnt = FBE_CLI_BIND_DEFAULT_COUNT;
    block_transport_placement = FBE_BLOCK_TRANSPORT_BEST_FIT;

}


/*********************************************************************
* sepBind end of file
*********************************************************************/
