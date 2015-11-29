/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_file_user_main.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the user mode interface to files in Win32.
 *
 *  FUNCTIONS:
 *    fbe_file_open
 *    fbe_file_close
 *    fbe_file_read
 *    fbe_file_write
 *    fbe_file_creat
 *    fbe_file_lseek
 *    fbe_file_access
 *    fbe_file_rename
 *    fbe_file_fgets
 *    fbe_file_get_size
 *
 *  NOTES:
 *    The syntax is historic, based on original flare Unix simulation code.
 *
 *  HISTORY:
 *    09/05/97:  Created. RAC
 *    11/09/97:  Ported to FBE. RPF
 *    08/22/08:  Add file delete. VG
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include <windows.h>
#include <stdio.h>
#include <string.h>

#define NTDDK_H     // recover from GenIncludes BOOLEAN hint
#include "fbe/fbe_file.h"
#include "EmcPAL_Memory.h"

/******************************
 ** LOCAL FUNCTION PROTOTYPES *
 ******************************/

static void ReportSysErr(char *CallerMsg, int ErrorMsgID);

/*******************************************************************************************
 *                              fbe_file_open()
 *******************************************************************************************
 */
fbe_file_handle_t fbe_file_open(const char *name, int access_mode, unsigned long protection_mode, fbe_file_status_t *err_status)
                                                        //  protection_mode ignored
{
    csx_char_t        csx_access_mode[8];
    csx_status_e      status = CSX_STATUS_SUCCESS;
    fbe_u64_t         seek_status;
    fbe_file_handle_t handle = FBE_FILE_INVALID_HANDLE;
    char              tmpName[256];

#ifndef ALAMOSA_WINDOWS_ENV
    if (NULL != strchr(name, '\\'))
    {
        CSX_BREAK();
    }
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */

    if (strncmp(name, "\\Device\\", strlen("\\Device\\")) == 0)
    {
        sprintf(tmpName, "%s%s", "\\\\.\\pipe\\", &name[8]);
    }
    else
    {
        /* By default we use the current working directory.
         */
        sprintf(tmpName, "%s", name);
    }

    // Translate access_mode
    csx_p_memset(csx_access_mode, '\0', sizeof(csx_access_mode));
    csx_p_strcat(csx_access_mode, "b");

    if (access_mode & FBE_FILE_RDWR)
    {
        csx_p_strcat(csx_access_mode, "W");
    }
    else if (access_mode & FBE_FILE_WRONLY)
    {
        csx_p_strcat(csx_access_mode, "w");
    }
    else
    {
        csx_p_strcat(csx_access_mode, "r");
    }

    if (access_mode & FBE_FILE_TRUNC)
    {
        csx_p_strcat(csx_access_mode, "x");
    }
    else if (access_mode & FBE_FILE_APPEND)
    {
        csx_p_strcat(csx_access_mode, "a");
    }

    if (access_mode & FBE_FILE_CREAT)
    {
        csx_p_strcat(csx_access_mode, "c");
    }

    //
    // Note: The shared access is set as follows for the following reason.
    // LCC opens diskm_n, periodically to find if disk exists.
    // Flare opens and uses them for IO purposes. And these two operations
    // are done asynchronously. However, LCC does not write. So, there should
    // not be any issue here.
    // But nonvol simulation is different, since clkdrv and other parts of
    // flare actually write to the nonvol file. And for this reason, nonvol
    // implementation has a lock associated with it.
    // If such a case arises, disk file(s) will have to be locked.
    //
    //

//  Commented because exclusive file access is not implemented in CSX.
//  CSX has not implemented because its not being used.
//
//    if (access_mode & FBE_FILE_EXCL)
//    {
//        SharedAccess = 0;
//    }
//    else
//    {
//        SharedAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
//    }
    csx_p_strcat(csx_access_mode, "R");

    if (access_mode & FBE_FILE_UNBUF)
    {
        csx_p_strcat(csx_access_mode, "D");
    }
    
    if (access_mode & FBE_FILE_NCACHE)
    {   
        csx_p_strcat(csx_access_mode, "U");
    } 

    // Open file, return handle
    status = csx_p_file_open(CSX_MY_MODULE_CONTEXT(), &handle, name,
                             csx_access_mode);

    if (!CSX_SUCCESS(status))
    {
        printf("Error in fbe_file_open: open of %s failed: 0x%x\n",
               name, (int)status);
        handle = FBE_FILE_INVALID_HANDLE;
    }

    if (CSX_NOT_NULL(err_status))
    {
        *err_status = status;
    }

    // Check for errors
    if (handle != FBE_FILE_INVALID_HANDLE)
    {
        if (access_mode & FBE_FILE_APPEND)
        {
            if (seek_status = fbe_file_lseek(handle, 0, 2) == 0)
            {
                printf("Error in fbe_file_open: seek EOF failed\n");
            }
        }
    }
    return handle;
}

/*******************************************************************************************
 *                              fbe_file_creat()
 *******************************************************************************************
 */
fbe_file_handle_t fbe_file_creat(const char *path, int access_mode)
{
    return fbe_file_open(path, (FBE_FILE_CREAT | FBE_FILE_TRUNC | access_mode), 0, NULL); // returns either good handle or FBE_FILE_INVALID_HANDLE
}


/*******************************************************************************************
 *                              fbe_file_read()
 *******************************************************************************************
 */
int fbe_file_read(fbe_file_handle_t handle, void *buffer, unsigned int nbytes_to_read, fbe_file_status_t *err_status)
/*      handle                 File descriptor (handle)
 *      buffer             User data buffer.
 *      nbytes_to_read     Size (in bytes) of the read request.
 */
{
    csx_size_t   nbytesRead;
    csx_status_e csx_status;
    int          status;

    csx_status = csx_p_file_read(&handle, buffer, nbytes_to_read, &nbytesRead);

    if (!CSX_SUCCESS(csx_status))
    {
        ReportSysErr("ReadFile", (int)csx_status);
        status = FBE_FILE_ERROR; // Unix-style error code
    }
    else
    {
        status = (int)nbytesRead;
    }
    if (CSX_NOT_NULL(err_status))
    {
        *err_status = csx_status;
    }

    return status;
}


/*******************************************************************************************
 *                              fbe_file_fgets()
 *******************************************************************************************
 */
int fbe_file_fgets(fbe_file_handle_t handle, char *buffer, unsigned int nbytes_to_read)
/*      handle                 File descriptor (handle)
 *      buffer             User data buffer.
 *      nbytes_to_read     Size (in bytes) of the read request.
 */
{
    unsigned int cumBytesRead = 0;
    csx_size_t   nbytesRead;
    csx_status_e csx_status;
    unsigned int maxLength = (nbytes_to_read - 1);

    if (nbytes_to_read == 0)
    {
        return FBE_FILE_ERROR;
    }

    buffer[0] = 0;
    while(cumBytesRead < maxLength)
    {
        csx_status = csx_p_file_read(&handle, &buffer[cumBytesRead], 1,
                                     &nbytesRead);
        if (!CSX_SUCCESS(csx_status))
        {
            if (cumBytesRead == 0)
            {
                ReportSysErr("ReadFile", (int)csx_status);
                return FBE_FILE_ERROR;
            }
            else
            {
                break;
            }
        }
        if (nbytesRead == 0)
        {
            // End Of file is detected.
            if (cumBytesRead > 0)   // Pass the data this time.
            {
                break;
            }
            else
            {
                return FBE_FILE_ERROR;  // return error for End Of File
            }
        }
        // The new line in DOS has 0x0d followed by 0x0a for the
        // end of the line.
        if (buffer[cumBytesRead] == 0x0d)
        {
            // No action
            ;
        }
        else if (buffer[cumBytesRead] == 0x0a)
        {
            break;
        }
        else
        {
            cumBytesRead++;
        }
    }

    buffer[cumBytesRead] = 0;

    return cumBytesRead;
}

/*******************************************************************************************
 *                              fbe_file_fgetc()
 *******************************************************************************************
 */
int fbe_file_fgetc(fbe_file_handle_t handle)
/*      handle                  File descriptor (handle)
 */
{
    char temp[2]={0,0};
    int bytesRead = 0;

    do
    {
        bytesRead = fbe_file_fgets(handle, temp, 2);
    } while ( 0 == bytesRead );

    if ( FBE_FILE_ERROR == bytesRead )
    {
        return FBE_FILE_ERROR;
    }

    return temp[0];
}

/*******************************************************************************************
 *                              fbe_file_write()
 *******************************************************************************************
 */
int fbe_file_write(fbe_file_handle_t handle, const void *buffer, unsigned int nbytes_to_write, fbe_file_status_t *err_status)
/*      handle                  File descriptor (handle)
 *      buffer              User data buffer.
 *      nbytes_to_write     Size (in bytes) of the write request.
 */
{
    csx_size_t   nbytesWritten;
    csx_status_e csx_status;
    int          status;

    csx_status = csx_p_file_write(&handle, buffer, nbytes_to_write,
                                  &nbytesWritten);
    if (!CSX_SUCCESS(csx_status))
    {
        ReportSysErr("WriteFile", (int)csx_status);
        status = FBE_FILE_ERROR; // Unix-style error code
    }
    else
    {
        status = (int)nbytesWritten;
    }
    if (CSX_NOT_NULL(err_status)) {
        *err_status = csx_status;
    }

    return status;
}

/*******************************************************************************************
 *                              fbe_file_close()
 *******************************************************************************************
 */
int fbe_file_close(fbe_file_handle_t handle)
{
    csx_status_e csx_status;
    int          status;

    csx_status = csx_p_file_close(&handle);

    if (CSX_SUCCESS(csx_status))
    {
        status = 0;
    }
    else                    // use Unix-style return values
    {
        ReportSysErr("CloseFile", (int)csx_status);
        status = FBE_FILE_ERROR; // Unix-style error code
    }

    return status;
}


/*******************************************************************************************
 *                              fbe_file_sync_close()
 *******************************************************************************************
 */
#ifdef C4_INTEGRATED
int fbe_file_sync_close(fbe_file_handle_t handle)
{
    csx_p_file_sync(&handle);
    return fbe_file_close(handle);
}
#endif /* C4_INTEGRATED - C4ARCH - file differences - C4WHY */

/*******************************************************************************************
 *                              fbe_file_lseek()
 *******************************************************************************************
 *
 *      handle        File descriptor (handle)
 *      offset    Byte position in the object where the read is to begin.
 *      whence    is one of these three values specifying whether offset is
 *                an absolute or incremental address:
 *
 *                   val    Unix       NT           Description
 *                    0   SEEK_SET  FILE_BEGIN  Set handle to offset bytes.
 *                    1   SEEK_CUR  FILE_CURR   Set handle to (handle + offset) bytes.
 *                    2   SEEK_END  FILE_END    Set handle to (sizeof(*handle) + offset) bytes.
 *
 *      If fbe_file_lseek() fails, the object pointer is not changed.
 *
 *      NOTE:
 *      This NT allows for file sizes greater than (2**32 - 2) by use of the third argument.
 *      This implementation ignores this feature; thus that value is the upper bound on
 *      filesizes for files requiring fbe_file_lseek() operations.
 *
 *******************************************************************************************
 */
fbe_u64_t fbe_file_lseek(fbe_file_handle_t handle, fbe_u64_t offset, int whence)
{
    csx_uoffset_t          newOffset = FBE_FILE_ERROR;
    csx_p_file_seek_type_e seekType;
    csx_status_e           status;

    switch (whence)
    {
        case 0: seekType = CSX_P_FILE_SEEK_TYPE_SET; break;
        case 1: seekType = CSX_P_FILE_SEEK_TYPE_CUR; break;
        case 2: seekType = CSX_P_FILE_SEEK_TYPE_END; break;
        default:
            return FBE_FILE_ERROR;
    }

    status = csx_p_file_seek(&handle, seekType, offset, &newOffset);

    if (!CSX_SUCCESS(status))
    {
        printf("Error from csx_p_file_seek: %s\n",
               csx_p_cvt_csx_status_to_string(status));

        newOffset = FBE_FILE_ERROR;
    }

    return newOffset; // returns match Unix-style values
}

/*******************************************************************************************
 *                              fbe_file_rename()
 *******************************************************************************************
 */
int fbe_file_rename(const char *old_path, const char *new_path)
{
    csx_status_e csx_status;
    int           status;

    // try to move
    csx_status = csx_p_file_rename(old_path, new_path);

    if (CSX_SUCCESS(csx_status))
    {
        status = 0;
    }
    else                    // use Unix-style return values
    {
        ReportSysErr("Rename", (int)csx_status);
        status = FBE_FILE_ERROR; // Unix-style error code
    }

    return status;
}


/*******************************************************************************************
 *                              fbe_file_access()
 *******************************************************************************************
 */
int fbe_file_access(const char *path, int access_mode)
{
    int open_mode = FBE_FILE_APPEND;    // don't truncate
    fbe_file_handle_t handle = FBE_FILE_INVALID_HANDLE;
    int status = FBE_FILE_ERROR;

    if (access_mode & FBE_READ_ACCESS)
    {
        if (access_mode & FBE_WRITE_ACCESS)
        {
            open_mode |= FBE_FILE_RDWR;
        }
        else
        {
            open_mode |= FBE_FILE_RDONLY;
        }
    }
    else if (access_mode & FBE_WRITE_ACCESS)
    {
        open_mode |= FBE_FILE_WRONLY;
    }
    else
    {
        printf("Error in fbe_file_access: priv mask not understood, defaulting to FBE_FILE_RDWR\n");
        open_mode |= FBE_FILE_RDWR;
    }

    handle = fbe_file_open(path, open_mode, 0, NULL);

    if (handle != FBE_FILE_INVALID_HANDLE && handle != NULL)
    {
        status = fbe_file_close(handle);
    }
    if (status)     // fbe_file_close returns Unix-style
    {
        return FBE_FILE_ERROR;  // error
    }
    else                        // use Unix-style returns here as well
    {
        return 0;   // success
    }
}

/*******************************************************************************************
 *                            ReportSysErr()
 *******************************************************************************************
 */
void ReportSysErr(char *CallerMsg, int ErrorMsgID)
{
    // Display the message.
    printf("System Error: %s: %d\n", CallerMsg, ErrorMsgID);
    return;
}

/*******************************************************************************************
 *                              fbe_file_get_size()
 *******************************************************************************************
 *
 *******************************************************************************************
 */
fbe_u64_t fbe_file_get_size(fbe_file_handle_t handle)
{
    csx_p_file_query_info_t query_info;

    query_info.file_size = 0; /* in case call fails */

    (void) csx_p_file_query(&handle, &query_info);

    return query_info.file_size;
}

int fbe_file_delete(const char *path)
{
    csx_status_e csx_status;
    int          status = 0;

    csx_status = csx_p_file_delete(path);
    if (!CSX_SUCCESS(csx_status))
    {
        ReportSysErr("DeleteFile", (int)csx_status);
        status = FBE_FILE_ERROR; // Unix-style error code
    }
    return status;
}

int fbe_file_find(const char *path)
{
    int status;
    csx_status_e csx_status;
    csx_p_file_find_handle_t handle;
    csx_p_file_find_info_t FindFileData;

    csx_status = csx_p_file_find_first(&handle, path,
                                       CSX_P_FILE_FIND_FLAGS_NONE,
                                       &FindFileData);
    if (!CSX_SUCCESS(csx_status))
    {
        /* should not print out a system error if we did not file the file.  Just return the error and let the caller handle it.*/
        status = FBE_FILE_ERROR; // Unix-style error code
    }
    else
    {
        csx_p_file_find_close(handle);
        status = 0;
    }
    return status;
}
