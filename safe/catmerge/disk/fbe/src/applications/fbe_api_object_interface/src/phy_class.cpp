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
 *      This file defines the methods of the physical class.
 *
 *  Notes:
 *      This is the base class for all of the Physical package
 *      interface classes. It is also a drived class of the
 *      Object class.
 *
 *  History:
 *      12-May-2010 : initial version
 *
 *********************************************************************/

#ifndef T_PHYCLASS_H
#include "phy_class.h"
#endif

/*********************************************************************
 *  bPhysical Class :: Constructors
 *********************************************************************
 *
 *  Description:
 *      This section contains the following methods:
 *          bPhysical(FLAG f) : Object(f) - Initial constructor call
 *          bPhysical() - constructor
 *
 *  Input: Parameters
 *      bPhysical(FLAG f) : Object(f) - first time call (overload).
 *
 *  Output: Console
 *      debug messages
 *
 * Returns:
 *      Nothing
 *
 *  History
 *      12-May-2010 : initial version
 *
 *********************************************************************/

bPhysical::bPhysical(FLAG f) : Object(f)
{
    idnumber = (unsigned) PHYSICAL_PACKAGE;
    phyCount = ++gPhysCount;

    fbe_zero_memory(&drive_info_asynch,sizeof(drive_info_asynch));

     if (Debug) {
        sprintf(temp, "bPhysical::bPhysical %s (%d)\n",
            "initial class constructor1", idnumber);
        vWriteFile(dkey, temp);
    }
};

bPhysical::bPhysical() //: Object()
{
    idnumber = (unsigned) PHYSICAL_PACKAGE;
    phyCount = ++gPhysCount;
    drive_info_asynch.completion_function = NULL;

    if (Debug) {
        sprintf(temp, "bPhysical::bPhysical %s (%d)\n",
            "class constructor2", idnumber);
        vWriteFile(dkey, temp);
     }
};

/*********************************************************************
 * phyDrive Class :: Accessor methods - simple functions
 *********************************************************************
 *
 *  Description:
 *      This section contains various accessor functions that return
 *      or display object class data or physical drive data stored in
 *      the the object. These methods are simple function calls with
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
 *      getProdRev()    - physical drive product revision number
 *      getSerialNum()  - physical drive serial number
 *      getObjId()      - physical drive object id
 *
 *  History
 *      12-May-2010 : initial version
 *
 *********************************************************************/

fbe_drive_vendor_id_t bPhysical::getVendorId(void)
{
    return (drive_vendor_id);
}

char * bPhysical::getProdRev(void)
{
    return (product_rev);
}

char * bPhysical::getSerialNum(void)
{
    return (product_serial_num);
}

fbe_u32_t bPhysical::getObjId(void)
{
    return (object_id);
}

unsigned bPhysical::MyIdNumIs(void)
{
    return idnumber;
}

void bPhysical::dumpme(void)
{
    strcpy (key, "bPhysical::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n",
        idnumber, phyCount);
    vWriteFile(key, temp);
}

/*********************************************************************
 * phyDrive Class :: HelpCmds()
 *********************************************************************
 *
 *  Description:
 *      This function packages up any passed in help lines with the
 *      list of Physical class interface help commands, which is
 *      sent to the Usage function.
 *
 *  Input: Parameters
 *      lines - help data sent from an interface method
 *
 *  Output: Console
 *      - Help with list of the physical class interfaces.
 *      - If called from a interface, it will include a list of the
 *        interface FBA API functions and assocaiated short name.
 *
 * Returns:
 *      nothing
 *
 *  History
 *      12-May-2010 : initial version
 *
 *********************************************************************/

void bPhysical::HelpCmds(char * lines)
{
    char * Interfaces =
        "[Physical Interface]\n" \
        " -PHYDRIVE  physical drive\n" \
        " -DISCOVERY Discovery\n" \
        " -LOGDRIVE  Logical Drive\n" \
        " -BOARD     Board\n" \
        " -ENCL      Enclosure\n" \
        "--------------------------\n";

    unsigned len = unsigned(strlen(lines) + strlen(Interfaces));
    char * temp = new char[len+1];
    
    strcpy (temp, (char *) Interfaces);
    strcat (temp, lines);
    
    Usage(temp);
    delete (temp);
    return;
}

/*********************************************************************
 * bPhysical end of file
 *********************************************************************/
