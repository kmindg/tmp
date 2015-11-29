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

#ifndef T_ARGCLASS_H
#include "arg_class.h"
#endif

/*********************************************************************
 * Arguments Class :: Constructor 
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

Arguments::Arguments(int &argc, char *argv[]) : fileUtilClass(argc, argv)
{ 
    idnumber = ARGUMENTS;
    argCount = ++gArgCount;

    // initialize working storage variables
    driverType = INVALID_DRIVER;
    index      = 0;
    spId       = FBE_SIM_INVALID_SERVER; 

    // Open log files 
    this->Open_Log_File();
    this->Open_Trans_File();
    
    // Output trace message
    if (Debug) {
        sprintf(temp, "Arguments class constructor (%d)\n", idnumber);
        vWriteFile(dkey, temp);
    }
}

/*********************************************************************
 *   Arguments Class :: Do_Arguments ()
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
 *       argv[n+1] -s or -k (s:simulation, k:kernel mode)
 *       argv[n+2] -a or -b (SPA or SPB) 
 *       argv[n+3] -package (FBE API class (top level))
 *       argv[n+4] -interface (FBE API sub class))
 *       argv[n+5] short name (name of function that calls the FBE API)
 *       argv[n+6] parameter(1) … parameter(n) FBE API call arguments 
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

fbe_status_t Arguments::Do_Arguments (int &argc, char *argv[])
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
    }

    /* 
     * First check for debug (-d) OR help (-h) options ...
     * Set index to point to 1st argument. These options must
     * come first.
     */
    if (strcmp (argv[++index],"-D") == 0) {
        Debug= 1;
        index++;
        
        // List input arguments.
        List_Arguments(argc, argv);

        // Ioctl: Check that at least 5 arguments were entered.
        if (xIoctl) {
            if (argc < 5) {
                status = FBE_STATUS_GENERIC_FAILURE;
                sprintf(temp, 
                    "<ERROR!> Missing arguments: %s\n", 
                    "need at least 5. Rerun with help.");
                Usage(temp);
                return (status);
            }
        
        // Fbe: Check that at least 7 arguments were entered.
        } else {
            if (argc < 7) {
                status = FBE_STATUS_GENERIC_FAILURE;
                sprintf(temp, 
                    "<ERROR!> Missing arguments: %s\n", 
                    "need at least 7. Rerun with help.");
                Usage(temp);
                return (status);
            }
        }
       
    /*
     * If help, display usage if arguments entered is less than 5
     */
    } 
    else if (strcmp (argv[index],"-H") == 0 ||
        strcmp (argv[index],"?") == 0){
            help= 1;
            index++;

        if (!xIoctl) {
           if (argc < 5) {
                 status = FBE_STATUS_GENERIC_FAILURE;
                 sprintf (temp, 
                    "%s\n", " Rerun with a <package> name to get a "
                    "list of interfaces. See examples below: \n "
                    "(Fbe) -h -s -a -phy \n (Ioctl) -h -ioc.\n" 
                    "--------------------------\n "
                    "Then rerun again with a <interface> name to get "
                    "a list of short names (function calls). See "
                    "examples below:\n (Fbe) -h -s -a -phy -phydrive "
                    "\n (Ioctl) -h -ioc -dramCache\n"
                    "--------------------------");
                Usage(temp);
                List_Arguments(argc, argv);
                return (status);
            }
        }

    /*
     * Help and debug options not entered. Now check that at least 6 
     * arguments were entered for Fbe command.
     */
    } else if (!xIoctl) {
        if (argc < 6) {
            status = FBE_STATUS_GENERIC_FAILURE;
            sprintf(temp, 
                "<ERROR!> %s\n", "Missing arguments: "
                "need at least 6. Rerun with help.\n"
                "--------------------------");
            Usage(temp);
            List_Arguments(argc, argv);
            return (status);
        }
    } else {
        if (argc < 4) {
            status = FBE_STATUS_GENERIC_FAILURE;
            sprintf(temp, 
                "<ERROR!> %s\n", "Missing arguments: "
                "need at least 4. Rerun with help.\n"
                "--------------------------");
            Usage(temp);
            List_Arguments(argc, argv);
            return (status);
        }
    }

    /*
     * NEXT check for driver type. This option must follow debug 
     * option.
     */
    if (strcmp (argv[index],"-K") == 0) {
        driverType = KERNEL;
        index++;

    }else if (strcmp (argv[index],"-S") == 0) {
        driverType = SIMULATOR;
        index++;

    } else if (!xIoctl) {
        status = FBE_STATUS_GENERIC_FAILURE;
        sprintf(temp, "<ERROR!> Argument [driver] %s [%s]\n", 
            BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        if (help) {index++;}
    }

    /*
     * NEXT check for SP type. This option must follow driver type.
     */
    if (strcmp (argv[index],"-A") == 0) {
        spId = FBE_SIM_SP_A;
        index++;

    }else if (strcmp (argv[index],"-B") == 0) {
        spId = FBE_SIM_SP_B;
        index++;

    } else if (!xIoctl) {
        status = FBE_STATUS_GENERIC_FAILURE;
        sprintf(temp, 
            "<ERROR!> Argument [SP] %s [%s]\n", 
            BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        if (help) {index++;}
    }

    /*
      * NEXT check for package. This option must follow driver and 
      * SP type parameters if they are entered. 
      */
    if (strcmp (argv[index],"-IOC") == 0) {
        // Get out. Ioctl commands are handled by ioctl.exe, 
        // which is called from the driver.    
        pkg = IOCTL_PACKAGE;
        Close_Files();
        return (status);
        
    }else if (strcmp (argv[index],"-PHY") == 0) {
        pkg = PHYSICAL_PACKAGE;
        index++;

    }else if (strcmp (argv[index],"-ESP") == 0) {
        pkg = ESP_PACKAGE;
        index++;

    }else if (strcmp (argv[index],"-NEIT") == 0) {
        pkg = NEIT_PACKAGE;
        index++;

    }else if (strcmp (argv[index],"-SEP") == 0) {
        pkg = SEP_PACKAGE;
        index++;

    }else if (strcmp (argv[index],"-SYS") == 0) {
        pkg = SYS_PACKAGE;
        index++;

    } else {
        status = FBE_STATUS_GENERIC_FAILURE;
        sprintf(temp, 
            "<ERROR!> Argument [package] %s [%s]\n", 
            BAD_INPUT, argv[index]);
        Usage(temp);
        return (status);
    }

    /*
     * Next implement library mode.
     */
     pInit = new initFbeApiCLASS();
     
    /*
     * Next check if not help. Exit if errors detected.
     */
    if (!help && (status != FBE_STATUS_OK) ) {
        Usage("");
        return (status);
    }

    /*
     * Next initialize for kernel or simulator, depending upon 
     * driver type.
     */
     if (!help) {
        switch (driverType) 
        {
        case KERNEL :
            status = pInit->Init_Kernel(spId);
            break;
        
        case SIMULATOR :
            status = pInit->Init_Simulator(spId);
            break;
        }
        if (status != FBE_STATUS_OK) {
            sprintf(temp, 
                    "%s API init failed - status: %d\n", 
                    __FUNCTION__, status);
            vWriteFile(ukey, temp);
            return (status);
        }
    }

    /*
     * NEXT check for package subset. This option must follow package.
     */
    switch (pkg)
    {
        case PHYSICAL_PACKAGE:

            if (help && argc < 6) {
                status = FBE_STATUS_GENERIC_FAILURE;
                bPhysical * pPhy = new bPhysical();
                pPhy->HelpCmds("");
                return (status);
            } 
            
            if (strcmp (argv[index],"-PHYDRIVE") == 0) {
                pBase = new PhyDrive();
                index++;

            }else if (strcmp (argv[index],"-DISCOVERY") == 0) {
                pBase = new PhyDiscovery();
                index++;

            }else if (strcmp (argv[index],"-BOARD") == 0) {
                pBase = new PhyBoard();
                index++;

            }else if (strcmp (argv[index],"-ENCL") == 0) {
                 pBase = new PhyEncl();
                index++;
    
            }else {
                status = FBE_STATUS_GENERIC_FAILURE;
                sprintf(temp, 
                    "<ERROR!> Argument [interface] %s %s\n", 
                    BAD_INPUT, argv[index]);
                bPhysical * pPhy = new bPhysical();
                pPhy->HelpCmds(temp);
                return (status);
            }
            break;

        case ESP_PACKAGE:
            if (help && argc < 6) {
                status = FBE_STATUS_GENERIC_FAILURE;
                bESP * pEsp = new bESP();
                pEsp->HelpCmds("");
                return (status);
            } 
            
            if (strcmp (argv[index],"-DRIVEMGMT") == 0) {
                pBase = new espDriveMgmt();
                index++;

            }else  if (strcmp (argv[index],"-SPSMGMT") == 0) {
                pBase = new espSpsMgmt();
                index++;

            }else  if (strcmp (argv[index],"-EIR") == 0) {
                pBase = new espEir();
                index++;

            }else  if (strcmp (argv[index],"-ENCLMGMT") == 0) {
                pBase = new espEnclMgmt();
                index++;

            }else  if (strcmp (argv[index],"-PSMGMT") == 0) {
                pBase = new espPsMgmt();
                index++;

            }else  if (strcmp (argv[index],"-BOARDMGMT") == 0) {
                pBase = new espBoardMgmt();
                index++;

            }else  if (strcmp (argv[index],"-COOLINGMGMT") == 0) {
                pBase = new espCoolingMgmt();
                index++;

            }else  if (strcmp (argv[index],"-MODULEMGMT") == 0) {
                pBase = new espModuleMgmt();
                index++;

            } else {
                status = FBE_STATUS_GENERIC_FAILURE;
                sprintf(temp, 
                    "<ERROR!> Argument [interface] %s %s\n", 
                    BAD_INPUT, argv[index]);
                bESP * pEsp = new bESP();
                pEsp->HelpCmds(temp);
                return (status);
            }

            break;

        case NEIT_PACKAGE:
            break;

        case SEP_PACKAGE:
            if (help && argc < 6) {
                status = FBE_STATUS_GENERIC_FAILURE;
                bSEP * pSep = new bSEP();
                pSep->HelpCmds("");
                return (status);
            }

            if (strcmp (argv[index],"-PVD") == 0) {
                pBase = new sepProvisionDrive();
                index++;

            }else if (strcmp (argv[index],"-LUN") == 0) {
                pBase = new sepLun();
                index++;

            }else if (strcmp (argv[index],"-RG") == 0) {
                pBase = new sepRaidGroup();
                index++;

            }else if (strcmp (argv[index],"-PANICSP") == 0) {
                 pBase = new sepPanicSP();
                index++;

            }else if (strcmp (argv[index],"-BIND") == 0) {
                pBase = new sepBind();
                index++;

            }else if (strcmp (argv[index],"-POWERSAVING") == 0) {
                pBase = new sepPowerSaving();
                index++;

            }else if (strcmp (argv[index],"-CMI") == 0){
                pBase = new sepCmi();
                index++;

            }else if (strcmp (argv[index],"-BVD") == 0){
                pBase = new sepBVD();
                index++;

            }else if (strcmp (argv[index],"-VD") == 0){
                pBase = new sepVirtualDrive();
                index++;    
            
            }else if (strcmp (argv[index],"-DATABASE") == 0){
                pBase = new sepDatabase();
                index++;    

            }else if (strcmp (argv[index],"-METADATA") == 0){
                pBase = new sepMetadata();
                index++;    

            }else if (strcmp (argv[index],"-JOBSERVICE") == 0){
                pBase = new sepJobService();
                index++;   

            }else if (strcmp (argv[index],"-SCHEDULER") == 0){
                pBase = new sepScheduler();
                index++;    

            }else if (strcmp (argv[index],"-BASECONFIG") == 0){
                pBase = new sepBaseConfig();
                index++;

            }else if (strcmp (argv[index],"-LEI") == 0){
                pBase = new sepLogicalErrorInjection();
                index++;

            }else if (strcmp (argv[index],"-SYSBGSERVICE") == 0){
                pBase = new sepSystemBgService();
                index++;

            }else if (strcmp (argv[index],"-SYSTIMETHRESHOLD") == 0){
                pBase = new sepSysTimeThreshold();
                index++;

            }else {
                status = FBE_STATUS_GENERIC_FAILURE;
                sprintf(temp, 
                    "<ERROR!> Argument [interface] %s %s\n", 
                    BAD_INPUT, argv[index]);
                bSEP * pSep = new bSEP();
                pSep->HelpCmds(temp);
                return (status);
            }

            break;

        case SYS_PACKAGE:
            if (help && argc < 6) {
                status = FBE_STATUS_GENERIC_FAILURE;
                bSYS * pSys = new bSYS();
                pSys->HelpCmds("");
                return (status);
            } 
            
            if (strcmp (argv[index],"-LOG") == 0) {
                pBase = new sysEventLog();
                index++;

            }else if (strcmp (argv[index],"-DISCOVERY") == 0) {
                pBase = new SysDiscovery();
                index++;

            }else {
                status = FBE_STATUS_GENERIC_FAILURE;
                sprintf(temp, 
                    "<ERROR!> Argument [interface] %s %s\n", 
                    BAD_INPUT, argv[index]);
                bSYS * pSys = new bSYS();
                pSys->HelpCmds(temp);
                return (status);
            }
            
            break;

        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            sprintf(temp, 
                "<ERROR!> Internal program %s %d\n", 
                "value in pkg not found", pkg);
            vWriteFile(ukey, temp);
            return status;
    }

    /*
     * (Trace) display the object id and class name.
     */
    if (Debug && status == FBE_STATUS_OK) {
        sprintf(temp, 
            "Created object of type [%s]\n",
            pBase->MyIdNameIs());
        vWriteFile(dkey, temp);
        pBase->dumpme();
    }

    //set Base class help flag
    pBase->setHelp(help);
    //set Base class debug flag
    pBase->setDebug(Debug);

    return (status);
}

/*********************************************************************
 *   Arguments Class :: Close_connection ()
 *********************************************************************/
 
void Arguments::Close_connection(fbe_sim_transport_connection_target_t spId)
{
    pInit->fbe_cli_destroy_fbe_api(spId);

}

/*********************************************************************
 * Arguments Class :: Accessor methods
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
 *      getDriver()  - 1 (k) or 2 (s)
 *      getSP()      - 1 (A) or 2 (B)
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

int Arguments::getIndex(void)
{
    return (index);
}

driver_t Arguments::getDriver(void)
{
    return (driverType);
}

fbe_sim_transport_connection_target_t Arguments::getSP(void)
{
    return (spId);
}

package_t Arguments::getPackage(void)
{
    return (pkg);
}

package_subset_t Arguments::getPackageSubset(void)
{
    return (pkgSubset);
}

Object * Arguments::getBase(void)
{
    return (pBase);
}

unsigned Arguments::MyIdNumIs(void)
{
    return idnumber;
}

void Arguments::dumpme(void) 
{   
    strcpy (key, "Arguments::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", idnumber, argCount);
    vWriteFile(key, temp);
}

/*********************************************************************
 * Arguments Class :: List_Arguments ()
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

void Arguments::List_Arguments(int argc, char * argv[])
{
    sprintf (temp, "Command Line Arguments (%d)\n", argc); 
  
    for (int i=0; i < argc; i++) {
        char * hold_temp = new char[strlen(argv[i]) + 20];
        sprintf(hold_temp, " argv[%d] %s\n", i, argv[i]);
        if(strlen(hold_temp) > 5000) {return;}
        strcat(temp, hold_temp);
        delete hold_temp;  
    }
    
    vWriteFile(dkey, temp);  
    return;
}

/*********************************************************************
 * Arguments end of file
 *********************************************************************/
