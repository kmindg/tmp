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
*      This file defines the methods of the SYS class.
*
*  Notes:
*      This is the base class for all of the SYS package 
*      interface classes. It is also a derived class of the
*      Object class.
*
*  History:
*      05-April-2011 : initial version
*
*********************************************************************/

#ifndef T_SYS_CLASS_H
#include "sys_class.h"
#endif

/*********************************************************************
*  bSYS Class :: Constructors
*********************************************************************
*
*  Description:
*      This section contains the following methods:
*          bSYS(FLAG f) : Object(f) - Initial constructor call
*          bSYS() - constructor
*      
*  Input: Parameters
*      bSYS(FLAG f) : Object(f) - first time call (overload).
*
*  Output: Console
*      debug messages
*
* Returns:
*      Nothing
*
*  History
*      05-April-2011 : initial version
*
*********************************************************************/

bSYS::bSYS(FLAG f) : Object(f)
{
    idnumber = (unsigned) SYS_PACKAGE;
    sysCount = ++gSysCount;

    if (Debug) {
        sprintf(temp, "bSYS::bSYS %s (%d)\n", 
                "initial class constructor1", idnumber);
        vWriteFile(dkey, temp);
    }
};

bSYS::bSYS() //: Object()
{
    idnumber = (unsigned) SYS_PACKAGE; 
    sysCount = ++gSysCount;

    if (Debug) {
        sprintf(temp, "bSYS::bSYS %s (%d)\n",
                "class constructor2", idnumber);
        vWriteFile(dkey, temp);
    }
};

/*********************************************************************
* SYS Class :: Accessor methods - simple functions 
*********************************************************************
*
*  Description:
*      This section contains various accessor functions that return 
*      or display object class data stored in the object.
*      These methods are simple function calls with
*      no arguments.
*
*  Input: Parameters
*      None - for all functions 
*
*  Output: Console
*      dumpme()- displays info about the object
*
* Returns:
*      MyIdNumIs()     - Object id.
*
*  History
*      05-April-2011 : initial version
*
*********************************************************************/

unsigned bSYS::MyIdNumIs(void)
{
    return idnumber;
}

void bSYS::dumpme(void)
{ 
    strcpy (key, "bSYS::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, sysCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* SYS Class :: HelpCmds()
*********************************************************************
*
*  Description:
*      This function packages up any passed in help lines with the 
*      list of SEP class interface help commands, which is 
*      sent to the Usage function.
*
*  Input: Parameters
*      lines - help data sent from an interface method 
*
*  Output: Console
*      - Help with list of the SYS class interfaces. 
*      - If called from a interface, it will include a list of the 
*        interface FBA API functions and assocaiated short name.
*
* Returns:
*      nothing
*
*  History
*      05-April-2011 : initial version
*
*********************************************************************/

void bSYS::HelpCmds(char * lines)
{
    char * Interfaces = 
        "[SYS Interface]\n" \
        " -LOG         EventLog\n" \
        " -DISCOVERY   Discovery Interface\n" \
        "--------------------------\n";

    unsigned len = unsigned(strlen(lines) + strlen(Interfaces));
    char * temp = new char[len+1];
    strcpy (temp, (char *) Interfaces);
    strcat (temp, lines); 
    Usage(temp);

    return;
}

/*********************************************************************
* bSYS::Sys_Intializing_variable (private method)
*********************************************************************/
void bSYS::Sys_Intializing_variable()
{
    package_id = FBE_PACKAGE_ID_INVALID;
    status = FBE_STATUS_FAILED;
    passFail = FBE_STATUS_FAILED;
    c = 0;

    return;
}


/*********************************************************************
/*********************************************************************
* bSYS end of file
*********************************************************************/
