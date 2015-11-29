/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 2009                    
 * All Rights Reserved.                                          
 * Licensed Material-Property of EMC Corporation.                
 * This software is made available solely pursuant to the terms  
 * of a EMC license agreement which governs its use.             
 *********************************************************************/

/*********************************************************************
 *
 *  Description: 
 *      This file defines the methods of the SEP PANIC INTERFACE class.
 *
 *  Notes:
 *      The SEP PANICSP class (sepPanicSP) is a derived class of 
 *      the base class (bSEP).
 *
 *  History:
 *      2011-03-28 : inital version
 *
 *********************************************************************/

#ifndef T_SEP_PANIC_SP_CLASS_H
#include "sep_panic_sp_class.h"
#endif

/*********************************************************************
 * sepPanicSP class :: Constructor
 *********************************************************************
 *  Description:
*      Initializes class variables
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/
sepPanicSP::sepPanicSP() : bSEP()
{
    // Store Object number
    idnumber = (unsigned) SEP_PANIC_SP_INTERFACE;
    sepPanicSPCount = ++gSepPanicSPCount;

    // Store Object name
    idname = new char [64];
    strcpy(idname, "SEP_PANIC_SP_INTERFACE");

    if (Debug) {
        sprintf(temp, 
            "sepPanicSP class constructor (%d) %s\n", 
            idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP PANIC SP Interface function calls>
    sepPanicSPInterfaceFuncs = 
        "[function call]\n" \
        "[short name] [FBE API SEP PANIC SP Interface]\n" \
        " ------------------------------------------------------------\n" \
        " panicSP fbe_api_panic_sp\n" \
        " ------------------------------------------------------------\n";

    // Define common usage for SEP PanicSP command
    usage_format = 
        " Usage: %s\n"
        " For example: %s";
};

/*********************************************************************
 * epPanicSP class : MyIdNumIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id number of object
*  Input: Parameters
*      none
*  Returns:
*      unsigned int - id number
*********************************************************************/

unsigned sepPanicSP::MyIdNumIs(void)
{
    return idnumber;
};

/*********************************************************************
*  sepPanicSP class  :: MyIdNameIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id name of object
*  Input: Parameters
*      none
*  Returns:
*      char* - id name
*********************************************************************/
char * sepPanicSP::MyIdNameIs(void)
{
    return idname;
};

/*********************************************************************
* epPanicSP class  :: dumpme()
*********************************************************************
*  Description:
*       Dumps id number and sep power saving count
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/
void sepPanicSP::dumpme(void)
{ 
     strcpy (key, "sepPanicSP::dumpme");
     sprintf(temp, "Object id: (%d) count: (%d)\n", 
         idnumber, sepPanicSPCount);
     vWriteFile(key, temp);
}

/*********************************************************************
 * sepPanicSP Class :: select()
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
 *      2011-28-03 : inital version
 *
 *********************************************************************/

fbe_status_t sepPanicSP::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface"); 
    c = index;

    // List SEP PANICSP Interface calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepPanicSPInterfaceFuncs );

    // get PANICSP status
    } else if (strcmp (argv[index], "PANICSP") == 0) {
        status = panic_sp(argc, &argv[index]);

    // can't find match on short name
    }else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
            BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepPanicSPInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* sepPanicSP class :: panic_sp () 
*********************************************************************
*
*  Description:
*      Panic SP.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/
fbe_status_t sepPanicSP::panic_sp(int argc , char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "panicSP",
        "sepPanicSP::panic_sp",
        "fbe_api_panic_sp",
        usage_format,
        argc, 6);
    
    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;

    // Make api call: 
    status = fbe_api_panic_sp();

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't panic SP");
    } else {
        sprintf(temp, "Panicking SP.");
    }
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params); 
    return status;
}