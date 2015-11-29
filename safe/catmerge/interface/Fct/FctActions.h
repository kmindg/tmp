#ifndef __FctActions_h__
#define __FctActions_h__
 
//***************************************************************************
// Copyright (C) EMC Corporation 2012 - 2015
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      FctActions.h
//
// Contents:
//
// Revision History:
//--

#include "DiskSectorCount.h"

#define FCT_ACTION_SUSPEND_RESIZE_WEAR_FLAG    0x1
#define FCT_ACTION_SUSPEND_RESIZE_FLUSH_FLAG   0x2
#define FCT_ACTION_SUSPEND_RESIZE_ELEMENT_FLAG 0x4
#define FCT_ACTION_SUSPEND_ALL_RESIZE_FLAG     0x7

enum FCT_ACTION_CODE
{
    FCT_ACTION_MIN,  // zero
    FCT_ACTION_ENABLE,
    FCT_ACTION_DISABLE,
    FCT_ACTION_REVIVE,
    FCT_ACTION_CLEAR_DELETED_VOLUMES,
    FCT_SET_MEM_ALLOC_OK,
    FCT_CLEAR_MEM_ALLOC_OK,
    FCT_ACTION_FORCE_DIR_UPDATE_FAILURE,
    FCT_ACTION_ENABLE_WRITE_PIN_PAGE_FAILURE,
    FCT_ACTION_DISABLE_WRITE_PIN_PAGE_FAILURE,
    FCT_ACTION_DISABLE_IDLE_CLEANER,
    FCT_ACTION_ENABLE_IDLE_CLEANER,
    FCT_ACTION_DIR_LOAD_READ_ERROR,
    FCT_ACTION_DIR_LOAD_WRITE_ERROR,
    FCT_ACTION_DIR_LOAD_READ_HEADER_ERROR,
    FCT_ACTION_DIR_LOAD_WRITE_HEADER_ERROR,
    FCT_ACTION_ENABLE_DISABLE_FAST_FILL,
    FCT_ACTION_SET_OAM_BYPASS,
    FCT_ACTION_ENABLE_FORCE_DO_NOT_FLUSH,
    FCT_ACTION_DISABLE_FORCE_DO_NOT_FLUSH,
    FCT_ACTION_CHECK_PAGE_PROMOTED,
    FCT_ACTION_CHECK_PAGE_NOT_PROMOTED,
    FCT_ACTION_ENABLE_SHUTDOWN_TEST,
    FCT_ACTION_EXPAND,
    FCT_ACTION_SHRINK_FLUSHED,
    FCT_ACTION_SHRINK_READY,
    FCT_ACTION_FORCE_DIR_WRITE_FAILURE,
    FCT_ACTION_ENABLE_READ_CORRUPT_DATA,
    FCT_ACTION_DISABLE_READ_CORRUPT_DATA,
    FCT_ACTION_ENABLE_DISABLE_SUSPEND_DEVICE_RESIZE,
    FCT_ACTION_VALIDATE_UNUSED_DEVICE_ELEMENTS,
    FCT_ACTION_ENABLE_DISABLE_DYNAMIC_OVERPROVISIONING,
    FCT_ACTION_SET_DEVICE_WEAR_INFO,
    FCT_ACTION_ZERO_FILL_TEST,
    FCT_ACTION_ENABLE_UNMAP_RESPONSE_DELAY,
    FCT_ACTION_MAX
};


struct FctAction : public BasicDriverAction
{
    FCT_ACTION_CODE     Op;

    union
    {
        struct
        {
            UINT_8  deviceId;
        }
        dirErrorInsertion;

        struct
        {
            UINT_8 disabled;
        }
        FastFill;

        struct
        {
            UINT_32 bypassRate;
        }OAMBypass;

        struct
        {
            UINT_32 Volume;
            DiskSectorCount Lba;
        }TestLunAndBlock;

        struct
        {
            UINT_8 trackerType;
        }
        DirUpdateFailure;

        struct
        {
            bool tarnish;
            bool pendOnLock;
            bool peerContactLost;
        }
        ZeroFill;

        struct
        {
            UINT_32 originSPId;
            UINT_32 expansionStatus;
            UINT_16 devId;
        }
        ExpansionStatus;
		
        struct
        {
            UINT_32 originSPId;
            UINT_32 localSPRebooted;
            UINT_16 devId;
        }
        ShrinkStatus;

        struct
        {
            UINT_8 enableSuspend;
            UINT_8 suspendFlags;
            INT_16 deviceId;
            INT_32 elementIndex;
        }
        SuspendDeviceResize;

        struct
        {
            UINT_8 disable;
        }
        DynamicOverProvisioning;

        struct
        {
            UINT_8  deviceId;
            UINT_32 devEolPECount;
            UINT_32 devPECount;
            UINT_32 devPOH;
        }
        DeviceWearInfo;

        struct
        {
            UINT_8  devId;
            UINT_32 tmSeconds;
        }
        UnmapResponseDelayTime;
    }
    type;

};

#endif  // __FctActions_h__

