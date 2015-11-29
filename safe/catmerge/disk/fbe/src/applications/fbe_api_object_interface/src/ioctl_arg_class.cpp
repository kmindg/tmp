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
 *      This file defines methods of the argument type class.
 *
 *  Notes:
 *      These methods handle the processing to read and edit
 *      command line arguments.
 *
 *  History:
 *      12-May-2010 : initial version
 *
 *********************************************************************/

#ifndef T_IOCTL_ARG_CLASS_H
#include "ioctl_arg_class.h"
#endif

/*********************************************************************
 * iArguments Class :: Constructor 
 *********************************************************************
 *
 *  Description:
 *      Initializes class variables; passes arguments to fileUtilClass
 *      constructor where the debug flag is set after checking the 
 *      1st input argument. 
 *
 *      Next, the log and transaction files are open before any debug 
 *      messages are output.
 *
 *  Input: Parameters
 *      argc, *argv[] - command line arguments 
 *
 *  Output: Console
 *      Debug messages (trace purposes)
 *
 * Returns:
 *      Nothing
 *
 *  History
 *      12-May-2010 : initial version
 *
 *********************************************************************/

iArguments::iArguments(int &argc, char *argv[]) : fileUtilClass(argc, argv)
{ 
    idnumber = ARGUMENTS;
    argCount = ++gArgCount;

    // initialize working storage variables
    index = 0;
    pkg = IOCTL_PACKAGE;
    
    // find package (-IOC) in arguments to get index into list.
    for (int i=0; i<argc; i++, index++) {
        if (strncmp (argv[i], "-IOC", 4) == 0) {break;}
    }

    // Open log files 
    this->Open_Log_File();
    this->Open_Trans_File();
    
    // Output trace message
    if (Debug) {
        sprintf(temp, "iArguments class constructor (%d)\n", idnumber);
        vWriteFile(dkey, temp);
    }
}

/*********************************************************************
 *   iArguments Class :: Do_Arguments ()
 *********************************************************************
 *
 *  Description:
 *      This function processes the list of arguments passed to the
 *      program on the command line.
 *
 *  Input: Parameters
 *      Command line arguments:
 *       argv[0] Program Name
 *       argv[n] Debug/help Options (not required):
 *                 -D (set debug option on) or
 *                 -h or ? (display help menu)
 *       argv[n+1] -package (-IOC (top level))
 *       argv[n+2] -interface (Ioctl sub class))
 *       argv[n+3] short name used to call Ioctl function 
 *       argv[n+4] parameter(1) … parameter(n) call arguments 
 *
 *  Output: Console.
 *      - Error Messages.
 *      - Argument list if debug option is set.
 *
 *  Returns:
 *      status - FBE_STATUS_OK or FBE_STATUS_GENERIC_FAILURE
 *
 *  History:
 *       12-May-2010 : initial version.
 *
 *********************************************************************/

fbe_status_t iArguments::Do_Arguments (int &argc, char *argv[])
{
    
    // Set status to success.
    fbe_status_t status = FBE_STATUS_OK;
    strcpy (key, "Arguments");
 
    /* 
     * Test for # of command line arguments. If there are no 
     * arguments, then return.
     */
    if (argc < 2) {
        status = FBE_STATUS_GENERIC_FAILURE;
        strcpy (temp, "<ERROR!> No arguments entered\n");
        Usage(temp);
        return (status);
    
    }else if (!xIoctl) {
        status = FBE_STATUS_GENERIC_FAILURE;
        strcpy (temp, "<ERROR!> missing -ioc parameter\n");
        Usage(temp);
        List_Arguments(argc, argv);
        return (status);
    }
    
    /*
     * NEXT check for package subset. This option must follow package.
     */
    if ( (help && argc < index+2) || (Debug && argc < index+2) ) {            
        status = FBE_STATUS_GENERIC_FAILURE;
        bIoctl * pIoctl = new bIoctl();
        pIoctl->HelpCmds("");
        return (status);
    } 
    
    if (strcmp (argv[++index],"-DRAMCACHE") == 0) {
        pBase = new IoctlDramCache();
        index++;

    }else {
        status = FBE_STATUS_GENERIC_FAILURE;
        sprintf(temp, "<ERROR!> Argument [interface] %s %s\n",
            BAD_INPUT, argv[index]);
        bIoctl *  pIoctl = new bIoctl();
        pIoctl->HelpCmds(temp);
        List_Arguments(argc, argv);
        return (status);
    }
   
    return (status);
}

/*********************************************************************
 * iArguments Class :: Accessor methods
 *********************************************************************
 *
 *  Description:
 *      This section contains routines that operate on class data.
 *
 *  Input: Parameters
 *      none
 *
 *  Returns:
 *      getBase()    - returns interface object
 *      getIndex()   - index into argument list
 *      getPackage() - See typedef enum package_e in
 *      getPackageSubset() - See typedef enum package_subset_e in
 *      MyIdNumIs()  - returns the Object id.
 *
 *  Output: Console
 *      dumpme()    - general info about the object 
 *
 *  History
 *      07-March-2011 : initial version
 *
 *********************************************************************/

int iArguments::getIndex(void)
{
    return (index);
}

package_t iArguments::getPackage(void)
{
    return (pkg);
}

package_subset_t iArguments::getPackageSubset(void)
{
    return (pkgSubset);
}

Object * iArguments::getBase(void)
{
    return (pBase);
}

unsigned iArguments::MyIdNumIs(void)
{
    return idnumber;
}

void iArguments::dumpme(void) 
{   
    strcpy (key, "Arguments::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
        idnumber, argCount);
    vWriteFile(key, temp);
}

/*********************************************************************
 * iArguments Class :: List_Arguments ()
 *********************************************************************
 *
 *  Description:
 *      This function displays the command arguments.
 *
 *  Input: Parameters
 *      argc, *argv[] - command line arguments 
 *
 *  Output: Console
 *      List of arguments.
 *
 *  Returns:
 *      Nothing.
 *
 *  History
 *      07-March-2011 : initial version
 *
 *********************************************************************/

void iArguments::List_Arguments(int argc, char * argv[])
{
    sprintf (temp, "Command Line Arguments (%d)\n", argc); 
  
    for (int i=0; i < argc; i++) {
        char * hold_temp = new char[strlen(argv[i]) + 20];
        sprintf(hold_temp, " argv[%d] %s\n", i, argv[i]);
        if (strlen(hold_temp) > 5000){return;}
        strcat(temp, hold_temp);
        delete hold_temp;  
    }
    vWriteFile(dkey, temp);  
   
    return;
}

/*********************************************************************
 * iArguments end of file
 *********************************************************************/