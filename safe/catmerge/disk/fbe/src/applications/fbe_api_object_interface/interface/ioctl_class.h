/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 2011                    
 * All Rights Reserved.                                          
 * Licensed Material-Property of EMC Corporation.                
 * This software is made available solely pursuant to the terms  
 * of a EMC license agreement which governs its use.             
 *********************************************************************/

/**********************************************************************
 *
 *  Description:
 *    This file defines the Ioctl base class.
 *
 *  History:
 *      23-Feburary-2012 : initial version
 *
 *********************************************************************/

#ifndef T_IOCTL_CLASS_H
#define T_IOCTL_CLASS_H

// ioctl includes.
#ifndef IOCTL_PRIVATE_H
#include "ioctl_private.h" 
#endif

#ifndef T_IOCTL_UTIL_CLASS_H
#include "ioctl_util.h"
#endif

#ifndef T_OBJECT_H
#include "object.h"
#endif
#include "EmcUTIL_Device.h"

//using namespace ioctlUtil;

/*********************************************************************
 * bIoctl base class : Object Base class : fileUtilClass Base class
 *********************************************************************/

class bIoctl : public Object
{
    protected:

        // Every object has an id number
        unsigned idnumber;
        unsigned ioctlCount;

        // status indicator and flags
        fbe_status_t status, passFail;
        fbe_bool_t enabled;
            
        // Device attributes
        char mDeviceName[K10_DEVICE_ID_LEN];
        EMCUTIL_RDEVICE_REFERENCE mHandle;
        
        // poll Interval and Max elements
        int PollInterval;
        int PollCountMax;

        // Volume WWN and Id 
        K10_LU_ID wwn;
        ULONG volumeId;
		
    public:
        
		 // Device and Ioctl method variables
        char Devicename[K10_DEVICE_ID_LEN];
        char UpperDeviceName[K10_DEVICE_ID_LEN];
        char LowerDeviceName[K10_DEVICE_ID_LEN];
        char DeviceLinkName[K10_DEVICE_ID_LEN];
        char tempBuf[K10_DEVICE_ID_LEN];
		
        int  IoctlCode;   
        PVOID InBuffer, OutBuffer;
        ULONG InBuffSize, OutBuffSize, volume;
        
        // \interface\BasicVolumeDriver structure variables
		BasicVolumeEnumVolumesResp *EnumVolResponse; 
        BasicDriverAction DriverAction;
        BasicVolumeWwnChange WwnChange;
		BasicVolumeDriverCommitParams CommitParams;
		 
        // Constructor
        bIoctl();
        bIoctl(FLAG);
        void common_constructor_processing(void);

        //Destructor
        ~bIoctl() {
            if (IsOpened()){CloseDevice();}
        }

        // Accessor methods : object 
        virtual unsigned MyIdNumIs(void);
        virtual char * MyIdNameIs(void) {return 0;}
        virtual void dumpme(void);	
        
        // Accessor methods : store input parameters
        void SetWWN(K10_LU_ID Wwn); 
        void SetVolume(ULONG vol); 
        void SetPollInterval (int sec);
        void SetPollCountMax (int sec); 
        
         // Accessor methods : get input parameters
         int GetPollInterval(void);  
         int GetPollCountMax(void);  
        
        // Accessor methods : Ioctl 
        bool IsOpened(void);
        
        // Select and call Ioctl interface method 
        virtual fbe_status_t select(int i, int c, char *a[]){
             return FBE_STATUS_INVALID;
        }  
        
        // Common steps to send an Ioctl
        fbe_status_t doIoctl(int IoctlCode, PVOID InBuffer, 
            PVOID OutBuffer, ULONG InBuffSize, ULONG OutBuffSize,
            char *temp);

        // help 
        virtual void HelpCmds(char *);
		
        /**************************************************************
         * inline CreateDevice : Open device with given name.
         *************************************************************/
   
        inline bool CreateDevice(char* name, char* staus_info)
        {
            EMCUTIL_STATUS status;

            strncpy(mDeviceName, name, K10_DEVICE_ID_LEN);
            // Ensure that string is NULL terminated
            mDeviceName[K10_DEVICE_ID_LEN - 1] = '\0';

            if (GetPollInterval() == 0) {
		sprintf(staus_info, "Opening [%s].\n", mDeviceName);
            }

            status = EmcutilRemoteDeviceOpen(mDeviceName, NULL, &mHandle);

            if (!EMCUTIL_SUCCESS(status) ||
                (mHandle == EMCUTIL_DEVICE_REFERENCE_INVALID)) {
                sprintf(staus_info, "Can't connect to [%s]. Error: (0x%x)\n", 
                        mDeviceName, (int)status);
                return false;
            }
            return true;
        }

        /**************************************************************
         * inline CloseDevice : Close device 
         *************************************************************/
   
        inline bool CloseDevice(char* staus_info) {
            EMCUTIL_STATUS status;

            if (mHandle != EMCUTIL_DEVICE_REFERENCE_INVALID) {
                status = EmcutilRemoteDeviceClose(mHandle);

            if(EMCUTIL_FAILURE(status)){
                    sprintf(staus_info, 
                        "Close device. Error: (%s)\n", EmcutilStatusToString(status));
					return(false);
                }
            }
            mHandle = EMCUTIL_DEVICE_REFERENCE_INVALID;     
            return true;
        }

        /**************************************************************
         * inline CloseDevice : Close device no error handling
         *************************************************************/
        
        inline bool CloseDevice(void) {
            
            if (mHandle != EMCUTIL_DEVICE_REFERENCE_INVALID) {
                EMCUTIL_STATUS                  status;
                status = EmcutilRemoteDeviceClose(mHandle);
                if(EMCUTIL_FAILURE(status))
                {
                    return (false);
                }
            }
            mHandle = EMCUTIL_DEVICE_REFERENCE_INVALID;     
            return true;
        }

        /*************************************************************
         * inline SyncSendIoctl: Sends an IOCTL to an already
         * opened device. This is a synchronouse operation which will
         * not return until the lower driver completes its task.
         *************************************************************
         * @param ioctlCode :
         * 	The IOCTL code to send to the device.
         * @param pInputBuffer :
         *	Input buffer to the lower device. It can be NULL.
         * @param inputBufferSize :
         *  Size of the input buffer if we have passed it.
         * @param pOutputBuffer: 
         *  Output buffer in which we get output from the device. 
         *  It can be NULL if we dont expect any output.
         * @param outputBufferSize: 
         *  Size of output buffer if we have passed it.
         * @Return: True if the operation has completed successfully.
         *  False if the operation got failed. It display the error 
         *  which we getfrom device.
         *************************************************************/

        inline int SyncSendIoctl(
            IN ULONG ioctlCode,
            IN PVOID pInputBuffer,
            IN ULONG inputBufferSize,
            IN PVOID pOutputBuffer,
            IN ULONG outputBufferSize,
			char* staus_info)
        {
            csx_size_t ReturnedLength;
            EMCUTIL_STATUS status;
            //assert(mHandle != INVALID_HANDLE_VALUE );

            status = EmcutilRemoteDeviceIoctl(mHandle, ioctlCode,
                                              pInputBuffer, inputBufferSize,
                                              pOutputBuffer, outputBufferSize, 
                                              &ReturnedLength);
            if (!EMCUTIL_SUCCESS(status)) {

                if (ioctlCode == IOCTL_BVD_ENUM_VOLUMES 
                    && status == EMCUTIL_STATUS_MORE_DATA) {

                    // This is valid error code for IOCTL_BVD_ENUM_VOLUMES 
                    // while we are first enumerating number of volumes.
                
                } else {
                    sprintf(staus_info, "Send IOCTL. Error: (0x%x) %s",
                            (int)status, EmcutilStatusToString(status));
                }

                return(false);
            }

            return(true);
        }
};

#endif /* T_IOCTL_CLASS_H */
