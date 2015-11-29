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
 *      This file defines the methods of the Ioctl DramCache class.
 *
 *  Notes:
 *      The Ioctl DramCache class (IoctlDramcache) is a derived class
 *      of the base class (bIoctl).
 *
 *  History:
 *      12-May-2010 : inital version
 *
 *********************************************************************/

#ifndef T_IOCTL_DRAMCACHE_CLASS_H
#include "ioctl_dramCache_class.h"
#endif

/*********************************************************************
 * IoctlDramCache class :: Constructor
 *********************************************************************/

IoctlDramCache::IoctlDramCache() : bIoctl()
{
    // Store Object number
    idnumber = (unsigned) IOCTL_DRAMCACHE_INTERFACE;
    idcCount = ++gIoctlDramCacheCount;

    // Store Object name
    idname = new char [64];
    strcpy(idname, "IOCTL_DRAMCACHE_INTERFACE");

   //Intializing variables passed to Ioctl methods
    EnumVolResponse = NULL;
    InBuffer        = NULL;
    OutBuffer       = NULL;
    InBuffSize      = 0;
    OutBuffSize     = 0;
    volume          = 0;

    if (Debug) {
        sprintf(temp,"%s (%d) %s\n",
            "IoctlDramCache class constructor",
            idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of Ioctl DramCache function calls>
    IoctlDramCacheFuncs = 
        "[function call]\n" \
        "[short name] [Ioctl DramCache Fucntions]\n" \
        " ------------------------------------------------------------\n" \
        " createVol   IoctlDramCache::createVol\n"  \
        " destroyVol  IoctlDramCache::destroyVol\n" \
        " ------------------------------------------------------------\n";

    // Define common usage for Ioctl DramCache commands
    usage_format = 
        " Usage: %s [object id]\n"
        " For example: %s 4";
};

/*********************************************************************
 * IoctlDramCache class : Accessor methods
 *********************************************************************/

unsigned IoctlDramCache::MyIdNumIs(void)
{
    return idnumber;
};

char * IoctlDramCache::MyIdNameIs(void)
{
    return idname;
};

void IoctlDramCache::dumpme(void)
{ 
     strcpy (key, "IoctlDramCache::dumpme");
     sprintf(temp, "Object id: (%d) count: (%d)\n", 
         idnumber, idcCount);
     vWriteFile(key, temp);
}

/*********************************************************************
 * IoctlDramCache Class :: select()
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
 *      *argv - pointer to argument 
 *    
 *  Returns:
 *      status - FBE_STATUS_OK or one of the other status codes.
 *               See fbe_types.h 
 *
 *  History
 *      12-May-2010 : inital version
 *
 *********************************************************************/

fbe_status_t IoctlDramCache::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select Ioctl interface");
    
    // List calls if help option and less than 6 arguments entered.
    if ( (help && argc <= 4) || (Debug && argc <= 4) ) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) IoctlDramCacheFuncs  );
    
    // create volume 
    }else if (strncmp (argv[index], "CREATEVOL", 9) == 0) {
        status = createVol(argc, &argv[index]);
   
    // destroy volume
    } else if (strncmp (argv[index], "DESTROYVOL", 10) == 0) {
        status = destroyVol(argc, &argv[index]);
       
   
    // can't find match on short name
    }else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
            BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) IoctlDramCacheFuncs  );
    } 

    return status;
}

/*********************************************************************
 *  IoctlDramCache :: createVol ()
 *********************************************************************/

fbe_status_t IoctlDramCache::createVol(int argc, char **argv) 
{
    // Assign default values  
    int IoctlCode = IOCTL_BVD_CREATE_VOLUME;
    passFail = FBE_STATUS_OK;
    iUtilityClass * pUtil = new iUtilityClass();
    
    // Define specific usage string  
    usage_format =
        " Usage: %s [volume Id] [wwn] \n"\
        " For example: %s 1 xxxxxxxxxxxxxxxx:xxxxxxxxxxxxxxxx";
        
    // Check that all arguments have been entered
    status = call_preprocessing(
        "createVol",
        "IoctlDramCache::createVol,",
        "SyncSendIoctl",  
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
   
    // Get volume number 
    argv++; 
    int rtn = sscanf(*argv, "%ld", (long*)&volume);  
    sprintf(params, "%s ", *argv);
    sprintf(temp, "volume Id [%s] (%ld)\n", *argv, (long)volume);
    if (Debug) {vWriteFile(dkey, temp);}

    // Get wwn 
    argv++;
    status = pUtil->EditWWn(&Wwn, argv, temp);
    strcat (params, *argv);
    if (Debug) {vWriteFile(dkey, temp);}
    
    // Check valid volume number
    if (rtn == 0){
        passFail = FBE_STATUS_INVALID;
        sprintf(temp, "Invalid volume Id (%s)\n", *argv);
        call_post_processing(passFail, temp, key, params);
    }
    
    // Check valid wwwn 
    if (status != FBE_STATUS_OK ){
        passFail = FBE_STATUS_INVALID;
        call_post_processing(passFail, temp, key, params);
    }

    // Return if wwn or volume number is invalid
    if ( passFail != FBE_STATUS_OK ){
        return status;
    }
     
    // Creating upper device name.
    strcpy(UpperDeviceName,DRAMCACHE_UPPER_DEVICE_NAME);
    _itoa((int)volume,tempBuf,10);
    strncat(UpperDeviceName,tempBuf,strlen(tempBuf) + 1);

    // Creating lower device name.
    LowerDeviceName[0] = 0;
    strcpy(LowerDeviceName,DRAMCACHE_LOWER_DEVICE_NAME);
    strncat(LowerDeviceName,tempBuf,strlen(tempBuf) + 1);

    // Creating device link name.
    DeviceLinkName[0]  = 0;
    strcpy(DeviceLinkName,DRAMCACHE_DEVICE_LINK_NAME);
    strncat(DeviceLinkName,tempBuf,strlen(tempBuf) + 1);

    // Copy to VolParams
    SetWWN(Wwn);
    VolParams.LunWwn = wwn;
    strncpy(VolParams.UpperDeviceName,UpperDeviceName,K10_DEVICE_ID_LEN);
    strncpy(VolParams.LowerDeviceName,LowerDeviceName,K10_DEVICE_ID_LEN);
    strncpy(VolParams.DeviceLinkName,DeviceLinkName,K10_DEVICE_ID_LEN);

    // Ensure that the strings are NULL terminated
    VolParams.UpperDeviceName[K10_DEVICE_ID_LEN - 1] = '\0'; 
    VolParams.LowerDeviceName[K10_DEVICE_ID_LEN - 1] = '\0'; 
    VolParams.DeviceLinkName[K10_DEVICE_ID_LEN - 1] = '\0'; 
    
    if (Debug) {
        sprintf(temp, "%s\n", VolParams.UpperDeviceName);
        vWriteFile(dkey, temp);
        sprintf(temp, "%s\n", VolParams.LowerDeviceName);
        vWriteFile(dkey, temp);
        sprintf(temp, "%s\n", VolParams.DeviceLinkName);
        vWriteFile(dkey, temp);
    }

    // Setup Ioctl parameters
    InBuffer = &VolParams;
    OutBuffer = NULL;
    InBuffSize = sizeof(CacheVolumeParams);
    OutBuffSize = 0;  

    // perform steps REQUIRED to SEND Ioctl 
    status = doIoctl(IoctlCode, 
        InBuffer, 
        OutBuffer, 
        InBuffSize, 
        OutBuffSize, 
        temp);
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    
    return status;

}

/*********************************************************************
 * IoctlDramCache class :: destroyVol () 
 *********************************************************************/

fbe_status_t IoctlDramCache::destroyVol(int argc , char ** argv)
{      
    // Assign default values
    int IoctlCode = IOCTL_BVD_DESTROY_VOLUME;
    iUtilityClass * pUtil = new iUtilityClass();

    // Define specific usage string  
    usage_format =
        " Usage: %s [wwn] \n"\
        " For example: %s xxxxxxxxxxxxxxxx:xxxxxxxxxxxxxxxx";
        
    // Check that all arguments have been entered
    status = call_preprocessing(
        "destroyVol",
        "IoctlDramCache::destroyVol",
        "SyncSendIoctl",  
        usage_format,
        argc, 5);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Get wwn 
    argv++;
    status = pUtil->EditWWn(&Wwn, argv, temp); 
    if (Debug) {vWriteFile(dkey, temp);}

    // Return if wwn is invalid
    if (status != FBE_STATUS_OK){
        call_post_processing(status, temp, key, *argv);
        return status;
    } 
    
    // Setup Ioctl parameters
    SetWWN(Wwn);
    InBuffer    = &wwn;
    OutBuffer   = NULL;
    InBuffSize  = sizeof(K10_WWID);
    OutBuffSize = 0;  
    
    // Perform steps REQUIRED to SEND Ioctl 
    status = doIoctl(IoctlCode, 
        InBuffer, 
        OutBuffer, 
        InBuffSize, 
        OutBuffSize, 
        temp);
    
    // Output results from call or description of error
    call_post_processing(status, temp, key, *argv);
    
    return status;
}

/*********************************************************************
 * IOCTL_DRAMCACHE_CLASS end of file
 *********************************************************************/
