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
 *      This file instantiates a FBE API interface class of type 
 *      package and invokes one of its methods if not an IOCTL
 *      command. IOCTL commands are execute via calling the IOCTL
 *      executable and passing it the command line arguments.
 *
 *  Input
 *      FBE API Command Line Format:
 *         >cli <(flags)-h|-d> <(driver)-s|-k> <(SP)-a|-b> 
 *              <-package> <-interface> <short name> 
 *              <-parameter(1)> … <-parameter(n)>
 *
 *      IOCTL Command Line Format:         
 *         >cli <(flags)-h|-d> <-package> <-interface> 
 *               <short name> <-parameter(1)> … <-parameter(n)>
 *
 *         >cli = fbe_api_object_interface_cli.exe
 *
 *  Output
 *      - Trace of steps of execution if debug option (-d) is passed in
 *        to stdout and the log file.
 *      - Data returned from the FBE API or IOCTL execution, which is
 *        written to stdout as well as to the transaction file and log
 *        file.
 *
 *  Table of Contents:
 *      main() - Flow control.
 *
 *  Notes:
 *      For simulation (FBE API only) and kernel environments. 
 *
 *  History:
 *      12-May-2010 : inital version
 *
 *********************************************************************/

#ifndef T_ARGCLASS_H
#include "arg_class.h"
#endif

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
 *      Debug messages if Debug arument is input. Otherwise, just what is 
 *      generated via the called methods.
 *
 *  Program Flow:
 *      Step1: Open transaction and log files.
 *      Step2: Read and edit arguments; instantiate interface.
 *      Step3: If not an IOCTL input command (-IOC), perform the 
 *             respective FBE API function and then go to Step5.
 *      Step4: Perform the IOCTL function specified.
 *      Step5: Exit program; return with status.
 *
 *  History
 *      12-May-2010 : inital version
 *
 *********************************************************************/

int __cdecl main (int argc, char *argv[])
{
    #include "fbe/fbe_emcutil_shell_maincode.h"
    
    // Step1: Open transaction and log files.
    Arguments * pArgs = new Arguments(argc, argv);
    int Debug = pArgs->getDebug();

    // Step2: Read and edit arguments; instantiate interface.
    fbe_status_t status = pArgs->Do_Arguments(argc, argv);
    package_t pkg = pArgs->getPackage();

    // Step3: Perform the FBE API function specified.
    if (status == FBE_STATUS_OK && pkg != IOCTL_PACKAGE) {
            
        // Call Fbe method related to short name. 
        Object * pBase = pArgs->getBase();
        int index = pArgs->getIndex();
        status = pBase->select(index, argc, argv);
        unsigned help = pArgs->getHelp();
   
        // Close connection
        if (!help) {
            fbe_sim_transport_connection_target_t spId;
            spId = pArgs->getSP();
            pArgs->Close_connection(spId);
        }

    // Step4: Perform the IOCTL function specified.
    } else if (status == FBE_STATUS_OK) {
        
        // Dir list of fbe_ioctl* files 
        char cmd[20] = "dir fbe_ioctl*";
        if (Debug) {system (cmd);}

        // Execute IOCTL program
        int i = system (pArgs->getArgs());
        if (Debug) {printf ("The value returned was: %d.\n",i);}
        return(i);
    }

    // Step5: Exit program; return with status.
    if (status == FBE_STATUS_OK){return(1);}

    return 0;
}
