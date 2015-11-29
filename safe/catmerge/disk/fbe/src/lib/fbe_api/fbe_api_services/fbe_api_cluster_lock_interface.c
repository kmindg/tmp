/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************/
/** @file fbe_api_cluster_lock_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the cluster lock interfaces for external users.
 *
 *
 ***************************************************************************/

#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_cluster_lock_interface.h"

#if defined(UMODE_ENV) 
#include "DlsUserInterface.h"
static UDLS_LOCK_HANDLE            fbe_cluster_lock_handle = NULL;
static EMCUTIL_RDEVICE_REFERENCE   DLS_driver_handle    = EMCUTIL_DEVICE_REFERENCE_INVALID;
static DLS_LOCK_MODE               currentLockMode      = DlsLockModeNull;

static fbe_status_t fbe_api_cluster_lock_open(void);
static fbe_status_t fbe_api_cluster_lock_operation(DLS_LOCK_MODE lockMode, fbe_bool_t * peer_written);

#elif defined(SIMMODE_ENV)
/* TODO */
#else
/* hardware */
#endif

#if defined(UMODE_ENV) 
/*!***************************************************************
 * @fn fbe_api_cluster_lock_open()
 ****************************************************************
 * @brief
 *  This fbe api connects DLS driver and opens the lock.
 * Noted that the driver handle and the lock handle will be 
 * kept around for later operation.
 *
 *  This only works for user space.
 *
 * @return fbe_status_t
 *
 * @author
 *
 ****************************************************************/
static fbe_status_t fbe_api_cluster_lock_open(void)
{
    EMCUTIL_STATUS           Emcutil_status;
    fbe_status_t             status = FBE_STATUS_OK;
    csx_size_t               bytesReturned      = 0;
    csx_status_e			 csx_status;

    DLSDRIVER_OPEN_IN_BUFFER     openInBuffer;
    DLSDRIVER_OPEN_OUT_BUFFER    openOutBuffer;

    fbe_zero_memory(&(openInBuffer), sizeof(DLSDRIVER_OPEN_IN_BUFFER));
    fbe_zero_memory(&(openOutBuffer), sizeof(DLSDRIVER_OPEN_OUT_BUFFER));

    strcpy((char*) (openInBuffer.doibName.dlnDomain),"FBE");

    csx_status = csx_p_native_executable_path_get((void *)&openInBuffer.doibName.dlnMonicker, DlsLockMonickerMaxNameLen, &bytesReturned);
    if (csx_status == CSX_STATUS_BUFFER_OVERFLOW) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: name too long!\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
	/*to be on the safe side, force last char to NULL*/
	openInBuffer.doibName.dlnMonicker[DlsLockMonickerMaxNameLen - 1] = 0;

    /* open DLS device */
    Emcutil_status = EmcutilRemoteDeviceOpen(DLSDRIVER_USER_DEVICE_NAME,
                                             NULL,
                                             & DLS_driver_handle);

    if(EMCUTIL_FAILURE(Emcutil_status))
    {
        DLS_driver_handle    = EMCUTIL_DEVICE_REFERENCE_INVALID;
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: failed to open DLS device %s!\n", __FUNCTION__, DLSDRIVER_USER_DEVICE_NAME); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    openInBuffer.doibName.dlnVersion = DlsCurrentVersion;
    openInBuffer.doibVersion          = DlsCurrentVersion;
    Emcutil_status = EmcutilRemoteDeviceIoctl( DLS_driver_handle,
                                               (ULONG) IOCTL_DLSDRIVER_OPEN_LOCK,
                                               &openInBuffer,
                                               sizeof(DLSDRIVER_OPEN_IN_BUFFER),
                                               &openOutBuffer,
                                               sizeof(DLSDRIVER_OPEN_OUT_BUFFER),
                                               &bytesReturned);
    if(EMCUTIL_FAILURE(Emcutil_status))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: failed to open DLS lock (status 0x%x)!\n", __FUNCTION__, Emcutil_status); 
        fbe_cluster_lock_handle = NULL;
        /* close driver handle */
        Emcutil_status = EmcutilRemoteDeviceClose(DLS_driver_handle);
        if(EMCUTIL_FAILURE(Emcutil_status))
        {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: failed to close DLS drive (status 0x%x)!\n", __FUNCTION__, Emcutil_status); 
            fbe_cluster_lock_handle = NULL;
        }
        DLS_driver_handle    = EMCUTIL_DEVICE_REFERENCE_INVALID;

        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_cluster_lock_handle = openOutBuffer.doobLockHandle;
        currentLockMode         = DlsLockModeNull;
    }

    fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s: open DLS handle!\n", __FUNCTION__); 

    return(status);

}

/*!***************************************************************
 * @fn fbe_api_cluster_lock_operation()
 ****************************************************************
 * @brief
 *  This fbe api performs lock operation.  It checks for current
 * lock mode before sending requests down to drive to avoid 
 * unnecessary panic.
 *
 * This only works for user space.
 * 
 * @return fbe_status_t
 *
 * @author
 *
 ****************************************************************/
static fbe_status_t fbe_api_cluster_lock_operation(DLS_LOCK_MODE lockMode, fbe_bool_t * data_stale)
{
    fbe_status_t                 status;
    csx_size_t                   bytesReturned = 0;
    DLSDRIVER_CONVERT_IN_BUFFER  convertInBuffer;
    DLSDRIVER_CONVERT_OUT_BUFFER convertOutBuffer;
    EMCUTIL_STATUS               Emcutil_status;

    if (fbe_cluster_lock_handle == NULL)
    {
        status = fbe_api_cluster_lock_open();
        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: failed to create DLS lock!\n", __FUNCTION__); 
            return status;
        }
    }

    if (currentLockMode == lockMode)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s: lock already in state 0x%x!\n", __FUNCTION__, lockMode); 
        return FBE_STATUS_OK;
    }

    if ((currentLockMode == DlsLockModeRead)&& (lockMode == DlsLockModeWrite))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s: upgrade lock from reader to writer is not supported!\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(&(convertInBuffer), sizeof(DLSDRIVER_CONVERT_IN_BUFFER));
    fbe_zero_memory(&(convertOutBuffer), sizeof(DLSDRIVER_CONVERT_OUT_BUFFER));


    convertInBuffer.dcnibVersion    = DlsCurrentVersion;
    convertInBuffer.dcnibLockHandle = fbe_cluster_lock_handle;
    convertInBuffer.dcnibLockMode   = lockMode;  


    Emcutil_status = EmcutilRemoteDeviceIoctl( DLS_driver_handle,
                                            (ULONG) IOCTL_DLSDRIVER_CONVERT_LOCK,
                                            &convertInBuffer,
                                            sizeof(DLSDRIVER_CONVERT_IN_BUFFER),
                                            &convertOutBuffer,
                                            sizeof(DLSDRIVER_CONVERT_OUT_BUFFER),
                                            &bytesReturned);
    if(EMCUTIL_FAILURE(Emcutil_status))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: failed lock op 0x%x (status 0x%x)!\n", __FUNCTION__, lockMode, Emcutil_status); 
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        if (convertOutBuffer.dcnobPeerWritten) {
            *data_stale = FBE_TRUE;
        }
        else {
            *data_stale = FBE_FALSE;
        }
        currentLockMode = lockMode;
        status          = FBE_STATUS_OK;
    }

    return status;
}

#endif

/*!***************************************************************
 * @fn fbe_api_common_destroy_cluster_lock_user()
 ****************************************************************
 * @brief
 *  This fbe api closes DLS locks.
 *
 * This only works for user space.
 * 
 * @return fbe_status_t
 *
 * @author
 *
 ****************************************************************/
fbe_status_t fbe_api_common_destroy_cluster_lock_user (void)
{
#if defined(UMODE_ENV) 
    csx_size_t          bytesReturned      = 0;
    EMCUTIL_STATUS      Emcutil_status;


    DLSDRIVER_CLOSE_IN_BUFFER     closeInBuffer;
    DLSDRIVER_CLOSE_OUT_BUFFER    closeOutBuffer;

    fbe_zero_memory(&(closeInBuffer), sizeof(DLSDRIVER_CLOSE_IN_BUFFER));
    fbe_zero_memory(&(closeOutBuffer), sizeof(DLSDRIVER_CLOSE_OUT_BUFFER));

    if ((fbe_cluster_lock_handle != NULL) && (DLS_driver_handle != EMCUTIL_DEVICE_REFERENCE_INVALID))
    {
        closeInBuffer.dclibLockHandle       = fbe_cluster_lock_handle;
        closeInBuffer.dclibVersion          = DlsCurrentVersion;
        Emcutil_status = EmcutilRemoteDeviceIoctl(DLS_driver_handle,
                                                (ULONG) IOCTL_DLSDRIVER_CLOSE_LOCK,
                                                &closeInBuffer,
                                                sizeof(DLSDRIVER_CLOSE_IN_BUFFER),
                                                &closeOutBuffer,
                                                sizeof(DLSDRIVER_CLOSE_OUT_BUFFER),
                                                &bytesReturned);
        if(EMCUTIL_FAILURE(Emcutil_status))
        {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: failed to close lock (status 0x%x)!\n", __FUNCTION__, Emcutil_status); 
        }
        fbe_cluster_lock_handle = NULL;
        currentLockMode         = DlsLockModeNull;
    }

    if (DLS_driver_handle != EMCUTIL_DEVICE_REFERENCE_INVALID)
    {
        Emcutil_status = EmcutilRemoteDeviceClose(DLS_driver_handle);
        if(EMCUTIL_FAILURE(Emcutil_status))
        {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: failed to close DLS drive (status 0x%x)!\n", __FUNCTION__, Emcutil_status); 
            fbe_cluster_lock_handle = NULL;
        }
        DLS_driver_handle    = EMCUTIL_DEVICE_REFERENCE_INVALID;
    }

    fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s: close DLS handle!\n", __FUNCTION__); 

#endif
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_cluster_lock_release_lock()
 ****************************************************************
 * @brief
 *  This fbe api release DLS locks.
 *
 * This only works for user space.
 * 
 * @return fbe_status_t
 *
 * @author
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_cluster_lock_release_lock(fbe_bool_t * data_stale)
{
    fbe_status_t status = FBE_STATUS_OK;

#if defined(UMODE_ENV) 
    status = fbe_api_cluster_lock_operation(DlsLockModeNull, data_stale);
#endif
    return status;
}

/*!***************************************************************
 * @fn fbe_api_cluster_lock_shared_lock()
 ****************************************************************
 * @brief
 *  This fbe api aquires shared DLS locks.
 *
 * This only works for user space.
 * 
 * @return fbe_status_t
 *
 * @author
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_cluster_lock_shared_lock(fbe_bool_t * data_stale)
{
    fbe_status_t status = FBE_STATUS_OK;

#if defined(UMODE_ENV) 
    status = fbe_api_cluster_lock_operation(DlsLockModeRead, data_stale);
#endif
    return status;
}

/*!***************************************************************
 * @fn fbe_api_cluster_lock_exclusive_lock()
 ****************************************************************
 * @brief
 *  This fbe api acquires the exclusive DLS locks.
 *
 * This only works for user space.
 * 
 * @return fbe_status_t
 *
 * @author
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_cluster_lock_exclusive_lock(fbe_bool_t * data_stale)
{
    fbe_status_t status = FBE_STATUS_OK;

#if defined(UMODE_ENV) 
    status = fbe_api_cluster_lock_operation(DlsLockModeWrite, data_stale);
#endif
    return status;
}

