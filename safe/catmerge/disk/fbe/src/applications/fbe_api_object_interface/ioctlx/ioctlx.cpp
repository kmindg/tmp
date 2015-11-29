/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 20011
 * All Rights Reserved.
 * Licensed Material-Property of EMC Corporation.
 * This software is made available solely pursuant to the terms
 * of a EMC license agreement which governs its use.
 *********************************************************************/

/*********************************************************************
 *
 *  Description:
 *      This file instantiates a IOCTL interface class of type 
 *      package and invokes one of its methods.
 *
 *  Input
 *      Command Line Format:
 *         >cli  <(flags)-h|-d> <-package> <-interface> 
 *              <short name> <-parameter(1)> … <-parameter(n)>
 *
 *         >cli = fbe_ioctl_object_interface_cli.exe
 *
 *  Output
 *      - Trace of steps of execution if debug option (-d) is passed in
 *        to stdout and the log file.
 *      - Data returned from IOCTL execution, which is written to stdout
 *        as well as to the transaction file and log file.
 *
 *  Table of Contents:
 *      main() - Flow control.
 *
 *  Notes:
 *      For kernel environment only. 
 *
 *  History:
 *      12-May-2010 : inital version
 *
 *********************************************************************/

// Prevents loading fbe includes  

#ifndef T_IOCTL_ARG_CLASS_H
#include "ioctl_arg_class.h"
#endif
#include "fbe/fbe_emcutil_shell_include.h"

/*********************************************************************
 * main () - Flow control.
 *********************************************************************
 *
 *  Description
 *      This is the main routine, inits and calls methods.
 *
 *  Input: Parameters
 *      See Do_arguments.
 *
 *  Output: Console
 *      None, except what is generated via the called methods.
 *
 *  Program Flow:
 *      Step1: Open transaction and log files.
 *      Step2: Read and edit arguments; instantiate interface.
 *      Step3: Perform the function specified.
 *      Step4: Exit program; return with status.
 *
 *  History
 *      12-May-2010 : inital version
 *
 *********************************************************************/
int __cdecl main (int argc, char *argv[])
{
#include "fbe/fbe_emcutil_shell_maincode.h"
    // Step1: Open transaction and log files.
    iArguments * pArgs = new iArguments(argc, argv);

    // Step2: Read and edit arguments; instantiate interface.
    fbe_status_t status = pArgs->Do_Arguments(argc, argv);

    // Step3: Perform the function specified.
    if (status == FBE_STATUS_OK) {
            
        // Call method related to short name. 
        Object * pBase = pArgs->getBase();
        int index = pArgs->getIndex();
        status = pBase->select(index, argc, argv);
        unsigned help = pArgs->getHelp();
    }

    // Step4: Exit program; return with status.
    if (status == FBE_STATUS_OK){return(1);}
        
    return 0;
}
