#ifndef EMCUTIL_FILE_H_
#define EMCUTIL_FILE_H_

//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/*! @file  EmcUTIL_File.h */
//
// Contents:
//      The header file for the utility "file" APIs.

////////////////////////////////////

/*! @addtogroup emcutil_file
 *  @{
 */

#include "EmcUTIL.h"

////////////////////////////////////

/*
 * Explain here
 */

CSX_CDECLS_BEGIN

////////////////////////////////////

#if defined(UMODE_ENV) || defined(SIMMODE_ENV)

#ifdef EMCUTIL_CONFIG_BACKEND_WU
typedef WIN32_FIND_DATA EMCUTIL_FILE_FIND_DATA;
typedef HANDLE EMCUTIL_FILE_FIND_HANDLE;
#define findFileName(_data) (_data.cFileName)
#define findIsDir(_data) (_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
#else
typedef csx_p_file_find_info_t EMCUTIL_FILE_FIND_DATA;
typedef csx_p_file_find_handle_t EMCUTIL_FILE_FIND_HANDLE;
#define findFileName(_data) (_data.filename)
#define findIsDir(_data) (_data.file_query_info.file_type == CSX_P_FILE_TYPE_DIRECTORY)
#endif

CSX_STATIC_INLINE EMCUTIL_STATUS
EmcutilFindFirstFile(
    EMCUTIL_FILE_FIND_HANDLE *pFileFindHandleRv,
    csx_cstring_t pFileName,
    EMCUTIL_FILE_FIND_DATA *pFileFindData)
{
#ifdef EMCUTIL_CONFIG_BACKEND_WU
    *pFileFindHandleRv = FindFirstFile(pFileName, pFileFindData);
    if (*pFileFindHandleRv == INVALID_HANDLE_VALUE) {
        DWORD Error = GetLastError();
        CSX_ASSERT_H_DC(Error == ERROR_FILE_NOT_FOUND);
        return EMCUTIL_STATUS_NAME_NOT_FOUND;
    }
    return EMCUTIL_STATUS_OK;
#else
    csx_status_e status;
    csx_p_file_find_handle_t file_find_handle;
    csx_p_file_find_info_t file_find_info;
    status = csx_p_file_find_first(&file_find_handle, pFileName, CSX_P_FILE_FIND_FLAGS_NONE, &file_find_info);
    if (CSX_FAILURE(status)) {
        if (status == CSX_STATUS_END_OF_FILE) {
            return EMCUTIL_STATUS_NAME_NOT_FOUND;
        } else {
            return status;
        }
    } else {
        *pFileFindHandleRv = file_find_handle;
        return EMCUTIL_STATUS_OK;
    }
#endif
}

CSX_STATIC_INLINE EMCUTIL_STATUS
EmcutilFindNextFile(
    EMCUTIL_FILE_FIND_HANDLE FileFindHandle,
    EMCUTIL_FILE_FIND_DATA *pFileFindData)
{
#ifdef EMCUTIL_CONFIG_BACKEND_WU
    BOOL worked;
    worked = FindNextFile(FileFindHandle, pFileFindData);
    if (!worked) {
        DWORD Error = GetLastError();
        CSX_ASSERT_H_DC(Error == ERROR_NO_MORE_FILES);
        return EMCUTIL_STATUS_NO_MORE_ENTRIES;
    }
    return EMCUTIL_STATUS_OK;
#else
    csx_p_file_find_handle_t file_find_handle = (csx_p_file_find_handle_t) FileFindHandle;
    csx_p_file_find_info_t file_find_info;
    EMCUTIL_STATUS status;
    status = csx_p_file_find_next(file_find_handle, &file_find_info);
    if (CSX_FAILURE(status)) {
        if (status == CSX_STATUS_END_OF_FILE) {
            return EMCUTIL_STATUS_NO_MORE_ENTRIES;
        } else {
            return status;
        }
    } else {
        return EMCUTIL_STATUS_OK;
    }
#endif
}

CSX_STATIC_INLINE VOID
EmcutilFindClose(
    EMCUTIL_FILE_FIND_HANDLE FileFindHandle)
{
#ifdef EMCUTIL_CONFIG_BACKEND_WU
    FindClose(FileFindHandle);
#else
    csx_p_file_find_handle_t file_find_handle = (csx_p_file_find_handle_t) FileFindHandle;
    csx_p_file_find_close(file_find_handle);
#endif
}

#endif

////////////////////////////////////

CSX_CDECLS_END

//++
//.End file EmcUTIL_File.h
//--

#endif                                     /* EMCUTIL_FILE_H_ */
