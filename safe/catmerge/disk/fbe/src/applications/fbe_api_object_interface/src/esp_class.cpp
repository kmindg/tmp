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
*      This file defines the methods of the ESP class.
*
*  Notes:
*      This is the base class for all of the ESP package 
*      interface classes. It is also a derived class of the
*      Object class.
*
*  History:
*      27-Jun-2011 : initial version
*
*********************************************************************/

#ifndef T_ESP_CLASS_H
#include "esp_class.h"
#endif

/*********************************************************************
*  bESP Class :: Constructors
*********************************************************************
*
*  Description:
*      This section contains the following methods:
*          bESP(FLAG f) : Object(f) - Initial constructor call
*          bESP() - constructor
*      
*  Input: Parameters
*      bESP(FLAG f) : Object(f) - first time call (overload).
*
*  Output: Console
*      debug messages
*
* Returns:
*      Nothing
*
*  History
*      27-Jun-2011 : initial version
*
*********************************************************************/

bESP::bESP(FLAG f) : Object(f)
{
    idnumber = (unsigned) ESP_PACKAGE;
    espCount = ++gEspCount;

    lu_object_id = FBE_OBJECT_ID_INVALID;
    status = FBE_STATUS_FAILED;
    passFail = FBE_STATUS_FAILED;

    if (Debug) {
        sprintf(temp, "bESP::bESP %s (%d)\n", 
            "initial class constructor1", idnumber);
        vWriteFile(dkey, temp);
    }
};

bESP::bESP() //: Object()
{
    idnumber = (unsigned) ESP_PACKAGE; 
    espCount = ++gEspCount;

    lu_object_id = FBE_OBJECT_ID_INVALID;
    status = FBE_STATUS_FAILED;
    passFail = FBE_STATUS_FAILED;

    if (Debug) {
        sprintf(temp, "bESP::bESP %s (%d)\n",
            "class constructor2", idnumber);
        vWriteFile(dkey, temp);
    }
};

/*********************************************************************
* ESP Class :: Accessor methods - simple functions 
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
*      27-Jun-2011 : initial version
*
*********************************************************************/

unsigned bESP::MyIdNumIs(void)
{
    return idnumber;
}

void bESP::dumpme(void)
{ 
    strcpy (key, "bESP::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, espCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* ESP Class :: HelpCmds()
*********************************************************************
*
*  Description:
*      This function packages up any passed in help lines with the 
*      list of ESP class interface help commands, which is 
*      sent to the Usage function.
*
*  Input: Parameters
*      lines - help data sent from an interface method 
*
*  Output: Console
*      - Help with list of the ESP class interfaces. 
*      - If called from a interface, it will include a list of the 
*        interface FBE API functions and associated short name.
*
* Returns:
*      nothing
*
*  History
*      27-Jun-2011 : initial version
*
*********************************************************************/

void bESP::HelpCmds(char * lines)
{
    char * Interfaces = 
        "[ESP Interface]\n" \
        " -DRIVEMGMT    DriveManagement\n" \
        " -SPSMGMT      Standby Power Supply Management\n"\
        " -EIR          Enclosure Information Reporting\n"\
        " -ENCLMGMT     Enclosure Management\n"\
        " -PSMGMT       Power Supply Management\n"\
        " -BOARDMGMT    Board Management\n"\
        " -COOLINGMGMT  Cooling Management\n"\
        " -MODULEMGMT   Module Management\n"\
        "--------------------------\n";

    unsigned len = unsigned(strlen(lines) + strlen(Interfaces));
    char * temp = new char[len+1];
    strcpy (temp, (char *) Interfaces);
    strcat (temp, lines); 
    Usage(temp);

    return;
}

/*********************************************************************
* bESP end of file
*********************************************************************/
