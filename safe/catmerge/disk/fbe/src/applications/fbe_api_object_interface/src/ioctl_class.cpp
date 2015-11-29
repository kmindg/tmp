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
 *      This file defines the methods of the Ioctl base class.
 *
 *  Notes:
 *      This is the base class for all Ioctl type classes. It is also
 *      a drived class of the Object class.
 *
 *  History:
 *      23-Feburary-2012 : initial version
 *
 *********************************************************************/

#ifndef T_IOCTL_CLASS_H
#include "ioctl_class.h"
#endif

/*********************************************************************
 *  bIoctl Class :: Constructors
 *********************************************************************
 *
 *  Description:
 *      This section contains the following methods:
 *          bIoctl(FLAG f) : Object(f) - Initial constructor call
 *          bIoctl() - constructor
 *
 *  Input: Parameters
 *      bIoctl(FLAG f) : Object(f) - first time call (overload).
 *
 *  Output: Console
 *      debug messages
 *
 * Returns:
 *      Nothing
 *
 *  History
 *      23-Feburary-2012 : initial version
 *
 *********************************************************************/

bIoctl::bIoctl(FLAG f) : Object(f)
{
    idnumber = (unsigned) IOCTL_PACKAGE;
    ioctlCount = ++gIoctlCount;
    common_constructor_processing();

    if (Debug) {
        sprintf(temp, "bIoctl::bIoctl %s (%d)\n",
            "initial class constructor1", idnumber);
        vWriteFile(dkey, temp);
    }
};

bIoctl::bIoctl() 
{
    idnumber = (unsigned) IOCTL_PACKAGE;
    ioctlCount = ++gIoctlCount;
    common_constructor_processing();

    if (Debug) {
        sprintf(temp, "bIoctl::bIoctl %s (%d)\n",
            "class constructor2", idnumber);
        vWriteFile(dkey, temp);
     }
};

/*********************************************************************
 * bIoctl Class :: common_constructor_processing 
 *********************************************************************/

void bIoctl::common_constructor_processing(void)
{
    mHandle = EMCUTIL_DEVICE_REFERENCE_INVALID;
    PollInterval = 0;
    PollCountMax = 0;
    volume       = 0;
}

/*********************************************************************
 * bIoctl Class :: Accessor methods - simple functions
 *********************************************************************
 *
 *  Description:
 *      This section contains various accessor functions that set, 
 *      return or display class data. 
 *
 *  Input: Parameters
 *      SetWWN()      - (WWN of the targetted volume)
 *      SetVolume()   - (ID of the targetted volume)
 *      SetPollInterval()
 *      SetPollCountMax()    
 *
 *  Output: Console
 *      dumpme()- displays info about the object
 *
 * Returns:
 *      MyIdNumIs()  - Object Id.
 *      IsOpened()   - invalid handle value (Checks if device is opened)
 *      GetPollInterval()
 *      GetPollCountMax()
 *      
 *  History
 *      12-May-2010 : initial version
 *
 *********************************************************************/

void bIoctl::dumpme(void)
{
    strcpy (key, "bIoctl::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
        idnumber, ioctlCount);
    vWriteFile(key, temp);
}

int bIoctl::GetPollInterval(void)  
{
    return PollInterval;
}

int bIoctl::GetPollCountMax(void)  
{
    return PollCountMax;
}

bool bIoctl::IsOpened(void)
{
    return (mHandle != EMCUTIL_DEVICE_REFERENCE_INVALID);
};

unsigned bIoctl::MyIdNumIs(void)
{
    return idnumber;
}

void bIoctl::SetWWN(K10_LU_ID Wwn) 
{ 
    wwn = Wwn;
}
     
void bIoctl::SetVolume(ULONG vol)
{ 
    volumeId = vol; 
}

void bIoctl::SetPollInterval (int sec) 
{
    PollInterval = sec;
}

void bIoctl::SetPollCountMax (int sec) 
{
    PollCountMax = sec;
}

/*********************************************************************
* bIoctl class :: doIoctl - Common steps to send an Ioctl
*********************************************************************/

fbe_status_t bIoctl::doIoctl(int IoctlCode, PVOID InBuffer, 
     PVOID OutBuffer, ULONG InBuffSize, ULONG OutBuffSize,
     char *temp)
{
    char status_info [200];
    status = FBE_STATUS_OK;
    
    // Make device  
    sprintf(Devicename, DRAMCACHE_DEVICE_NAME_CHAR);
    
    if (!CreateDevice(Devicename, temp)){
        status = FBE_STATUS_GENERIC_FAILURE;    
        
        if(EnumVolResponse){delete EnumVolResponse;}  
        _flushall();
       
        if (Debug) {vWriteFile(dkey, temp);}
        
        return status;
    }
    if (Debug) {vWriteFile(dkey, temp);}

    // Send Ioctl - see inline function ioctl_class.h
    if (SyncSendIoctl(
        IoctlCode, 
        InBuffer, 
        InBuffSize, 
        OutBuffer, 
        OutBuffSize, 
        status_info))
    {
        // Success
        sprintf(status_info, "Volume created successfully\n");
        if (Debug) {vWriteFile(dkey, status_info);}
   
    } else {
         // Failure 
         status = FBE_STATUS_GENERIC_FAILURE; 
         if (Debug) {vWriteFile(dkey, status_info);}
    }
    
    // Append ioctl status 
    strcat (temp, status_info);
    status_info[0] = '\0'; 

    // Close device
    if (EnumVolResponse){delete EnumVolResponse;}
    if (IsOpened()){CloseDevice(status_info);}     
    strcat (temp, status_info);

    //Reset the Volume number to default.
    SetVolume(INVALID_VOLUME);

    return status;
}

/**********************************************************************
 * bIoctl Class :: HelpCmds()
 *********************************************************************
 *
 *  Description:
 *      This function packages up any passed in help lines with the
 *      list of bIoctl class interface help commands, which is
 *      sent to the Usage function.
 *
 *  Input: Parameters
 *      lines - help data sent from an interface method
 *
 *  Output: Console
 *      - Help with list of the bIoctl class interfaces.
 *      - If called from a interface, it will include a list of the
 *        functions and assocaiated short name.
 *
 * Returns:
 *      nothing
 *
 *  History
 *      12-May-2010 : initial version
 *
 *********************************************************************/

void bIoctl::HelpCmds(char * lines)
{
    char * Interfaces =
        "[Ioctl Interface]\n" \
        " -DRAMCACHE Send Ioctls to DRAMCache driver\n" \
        "--------------------------\n";

    unsigned len = unsigned(strlen(lines) + strlen(Interfaces));
    char * temp = new char[len+1];
    strcpy (temp, (char *) Interfaces);
    strcat (temp, lines);
    Usage(temp);

    return;
}

/*********************************************************************
 * Ioctl_Class  end of file
 *********************************************************************/
