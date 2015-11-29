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
*      This file defines the methods of the SEP CMI INTERFACE class.
*
*  Notes:
*      The SEP CMI class (sepCmi) is a derived class of 
*      the base class (bSEP).
*
*  History:
*      22-June-2011 : inital version
*
*********************************************************************/
#ifndef T_SEP_CMI_CLASS_H
#include "sep_cmi_class.h"
#endif


/*********************************************************************
 * sepCmi class :: sepCmi() Constructor
 *********************************************************************
*  Description:
*      Initializes class variables
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/
sepCmi :: sepCmi():bSEP()
{
    // Store Object number
    idnumber = (unsigned)SEP_CMI_INTERFACE;
    sepCmiCount = ++gSepCmiCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "SEP_CMI_INTERFACE");

    //Initializing variable
    sep_cmi_initializing_variable();
    
    if (Debug) {
        sprintf(temp, 
            "sepCmi class constructor (%d) %s\n", 
            idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP CMI Interface function calls>
    sepCmiInterfaceFuncs = 
        "[function call]\n" \
        "[short name]   [FBE API SEP CMI Interface]\n" \
        " ------------------------------------------------------------\n" \
        " cmiGetInfo    fbe_api_cmi_service_get_info\n" \
        " ------------------------------------------------------------\n";

    // Define common usage for SEP CMI command
    usage_format = 
        " Usage: %s\n"
        " For example: %s";
};

/*********************************************************************
 * sepCmi class : MyIdNumIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id number of object
*  Input: Parameters
*      none
*  Returns:
*      unsigned int - id number
*********************************************************************/

unsigned sepCmi::MyIdNumIs(void)
{
    return idnumber;
};

/*********************************************************************
*  sepCmi class  :: MyIdNameIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id name of object
*  Input: Parameters
*      none
*  Returns:
*      char* - id name
*********************************************************************/
char * sepCmi::MyIdNameIs(void)
{
    return idname;
};

/*********************************************************************
* sepCmi class  :: dumpme()
*********************************************************************
*  Description:
*       Dumps id number and sep power saving count
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/

void sepCmi::dumpme(void)
{ 
    strcpy (key, "sepCmi::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
         idnumber, sepCmiCount );
         vWriteFile(key, temp);
}

/*********************************************************************
 * sepCmi Class :: select()
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
 *      22-Jun-2011: inital version
 *
 *********************************************************************/
fbe_status_t sepCmi::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface"); 
    c = index;

    // List SEP Cmi Interface calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepCmiInterfaceFuncs );
        return status;

    } 
    // get CMI status
    if (strcmp (argv[index], "CMIGETINFO") == 0) {
        status = cmi_get_info(argc, &argv[index]);

    // can't find match on short name
    }else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
            BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepCmiInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* sepCmi class :: cmi_get_info () 
*********************************************************************
*
*  Description:
*      Get the cmi  information of the SP
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t sepCmi::cmi_get_info(int argc , char ** argv)
{
    char sp_state[8]; 
    char sp_id[8];
    char peer_alive[6];

    // Check that all arguments have been entered
    status = call_preprocessing(
        "cmiGetInfo",
        "sepCmi::cmi_get_info",
        "fbe_api_cmi_service_get_info",
        usage_format,
        argc, 6);
 
    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    sprintf(params, " ");
    // Make api call: 
    status = fbe_api_cmi_service_get_info (&cmi_info);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get CMI information");
    } else {
        edit_cmi_info(&cmi_info, sp_state,sp_id,peer_alive);

        sprintf(temp,"\n"
               "<SP ID>         %s\n"
               "<SP State>      %s\n"
               "<Peer alive>    %s\n",
               sp_id,sp_state, peer_alive);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepCmi Class :: edit_cmi_info()(private method)
*********************************************************************
*
*  Description:
*      Format the cmi information to display
*
*  Input: Parameters
*      cmi_info  - cmi info structure
*      sp_state  - state of the SP 
*      sp_id      -  id of the SP  
*      peer alive   -..is peer alive or not.
*
*  Returns:
*      void
*********************************************************************/
void sepCmi :: edit_cmi_info(fbe_cmi_service_get_info_t  *cmi_info,
                             char* sp_state, char* sp_id,
                             char* peer_alive){

    if(cmi_info->peer_alive == FBE_TRUE){
        sprintf(peer_alive , "TRUE");
    } else {
        sprintf(peer_alive , "FALSE");
    }

    if(cmi_info->sp_id == FBE_CMI_SP_ID_A){
        sprintf(sp_id , "SPA");
    } else if (cmi_info->sp_id == FBE_CMI_SP_ID_B){
        sprintf(sp_id , "SPB");
    } else {
        sprintf(sp_id , "INVALID");
    }

    if(cmi_info->sp_state == FBE_CMI_STATE_ACTIVE){
        sprintf(sp_state, "ACTIVE");
    }else if(cmi_info->sp_state == FBE_CMI_STATE_PASSIVE){
        sprintf(sp_state , "PASSIVE");
    }else if(cmi_info->sp_state == FBE_CMI_STATE_BUSY){
        sprintf(sp_state , "BUSY");
    }else{
        sprintf(sp_state , "INVALID");
    }

}

/*********************************************************************
* sepCmi Class :: sep_cmi_initializing_variable()(private method)
*********************************************************************/
void sepCmi :: sep_cmi_initializing_variable()
{
    fbe_zero_memory(&cmi_info,sizeof(cmi_info));
}
