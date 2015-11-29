/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_file_kernel.c
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
 *
 *  HISTORY:
 *    created by Kishore Chitrapu
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#ifdef ALAMOSA_WINDOWS_ENV
#include <ntifs.h>
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
#include "k10ntddk.h"
#include "EmcPAL_Irp.h"
#include <ntddk.h>
#include "ktrace.h"
#include "fbe/fbe_file.h"
//#include <ntstrsafe.h>
#include "EmcPAL_Memory.h"
#include "EmcPAL_DriverShell.h"

/******************************
 ** LOCAL FUNCTION PROTOTYPES *
 ******************************/

static void ReportLastSysErr(char *CallerMsg);

void fbe_file_set_sparse(fbe_file_handle_t handle)
{
#ifdef ALAMOSA_WINDOWS_ENV
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    EMCPAL_IRP_STATUS_BLOCK io_status = {0};

    status = ZwFsControlFile(handle, 0, 0, 0, &io_status, FSCTL_SET_SPARSE, 0, 0, 0, 0);
    if ( !EMCPAL_IS_SUCCESS(status) )
    {
        KvTrace("FSCTL_SET_SPARSE failed with status 0x%x", status);
    }
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
}

/*******************************************************************************************
 *                              fbe_file_open()
 *******************************************************************************************
 */
fbe_file_handle_t fbe_file_open(const char *name, int access_mode, unsigned long protection_mode, fbe_file_status_t *err_status)
                                                        //  protection_mode ignored
{
    fbe_file_handle_t handle = FBE_FILE_INVALID_HANDLE;
    csx_status_e status = CSX_STATUS_SUCCESS;
    fbe_char_t csx_access_mode[256];
    csx_module_context_t    csxModuleContext;

    csx_p_memset(csx_access_mode, '\0', sizeof(csx_access_mode));
    csx_p_strcat(csx_access_mode, "b");
    csxModuleContext = EmcpalClientGetCsxModuleContext(
                       EmcpalDriverGetCurrentClientObject());
    // Translate access_mode to NTaccess_mode
    if ((access_mode & 1) == 0)
    {
        csx_p_strcat(csx_access_mode, "r");
    }
    if (access_mode & FBE_FILE_WRONLY)
    {
        csx_p_strcat(csx_access_mode, "w");
    }
    if (access_mode & FBE_FILE_RDWR)
    {
        csx_p_strcat(csx_access_mode, "W");
    }

    if (access_mode & FBE_FILE_APPEND)
    {
        csx_p_strcat(csx_access_mode, "a");
    }
    // Translate access_mode to NTcreate_mode
    if (access_mode & FBE_FILE_TRUNC)
    {
        csx_p_strcat(csx_access_mode, "x");            
    }
    else if (access_mode & FBE_FILE_CREAT)
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
//        sharedAccess = 0;
//    }
//    else
//    {
//        sharedAccess = (FILE_SHARE_READ | FILE_SHARE_WRITE);
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

    if (access_mode & FBE_FILE_CREAT)
    {
        // We want to make the Terminator disk files (e.g. disk0_0_0) sparse.
        // These files are the only files currently created via this function call
        // so for now just set all created files to sparse files.

        csx_p_strcat(csx_access_mode, "s");            
    }

    status = csx_p_file_open(csxModuleContext, &handle, name, csx_access_mode);
    if (!CSX_SUCCESS(status))
    {
         KvTrace("fbe_file_open(): csx_p_file_open for %s = 0x%08x\n",
             name, status);
         handle = FBE_FILE_INVALID_HANDLE;
    }
    if(CSX_NOT_NULL(err_status))
    {
        *err_status = status;
    }

    return handle;
}

/*******************************************************************************************
 *                              fbe_file_read()
 *******************************************************************************************
 */
int fbe_file_read(fbe_file_handle_t handle,
                  void *rdbuf,
                  unsigned int nbytes_to_read,
                  fbe_file_status_t *err_status)
/*      handle                 File descriptor (handle)
 *      buffer             User data buffer.
 *      nbytes_to_read     Size (in bytes) of the read request.
 */
{
    csx_size_t bytesRead = 0;
    csx_status_e rdstatus = EMCPAL_STATUS_SUCCESS;

    rdstatus = csx_p_file_read(&handle, rdbuf, nbytes_to_read, &bytesRead);                           
    if (!CSX_SUCCESS(rdstatus))
    {
        KvTrace("fbe_file_read(): csx_p_file_read with length: %d"
            " failed with status: 0x%08x\n", nbytes_to_read, rdstatus);
        bytesRead = FBE_FILE_ERROR;
    }

    if(CSX_NOT_NULL(err_status))
    {
        *err_status = rdstatus;
    }
    return (int)bytesRead;
}

/*******************************************************************************************
 *                              fbe_file_write()
 *******************************************************************************************
 */
int fbe_file_write(IN fbe_file_handle_t handle,
                   IN const void *wrbuf,
                   IN unsigned int nbytes_to_write,
                   fbe_file_status_t *err_status)
/*      handle                  File descriptor (handle)
 *      buffer              User data buffer.
 *      nbytes_to_write     Size (in bytes) of the write request.
 */
{
    csx_size_t bytesWritten = 0;
    csx_status_e wrstatus = EMCPAL_STATUS_SUCCESS;

    wrstatus = csx_p_file_write (&handle, wrbuf, nbytes_to_write, &bytesWritten);
    if (!CSX_SUCCESS(wrstatus))
    {
        KvTrace("fbe_file_write(): csx_p_file_write with length: %d"
            " failed with status: 0x%08x\n", nbytes_to_write, wrstatus);
        bytesWritten = FBE_FILE_ERROR;
    }

    if(CSX_NOT_NULL(err_status))
    {
        *err_status = wrstatus;
    }
    return (int)bytesWritten;
}

/*******************************************************************************************
 *                              fbe_file_sync_close()
 *******************************************************************************************
 */
#ifdef C4_INTEGRATED
int fbe_file_sync_close(fbe_file_handle_t handle)
{
    int retVal = 0;
    csx_status_e status = CSX_STATUS_SUCCESS;

    csx_p_file_sync(&handle);
    status = csx_p_file_close(&handle);
    if (!CSX_SUCCESS(status))
    {
        KvTrace("fbe_file_sync_close(): csx_p_file_close failed with status: 0x%08x\n", status);
        retVal = FBE_FILE_ERROR;
    }

    return retVal;
}
#endif /* C4_INTEGRATED - C4ARCH - file differences - C4WHY */

/*******************************************************************************************
 *                              fbe_file_close()
 *******************************************************************************************
 */
int fbe_file_close(fbe_file_handle_t handle)
{
    int retVal = 0;
    csx_status_e status = EMCPAL_STATUS_SUCCESS;

    status = csx_p_file_close(&handle);
    if (!CSX_SUCCESS(status))
    {
        KvTrace("fbe_file_close(): csx_p_file_close failed with status: 0x%08x\n", status);
        retVal = FBE_FILE_ERROR;
    }

    return retVal;
}


fbe_file_handle_t fbe_file_creat(const char *path, int prot_mode)
{
#if 0 /* SAFEBUG - it seems that the callers of this are all supplying the access_mode they want as 2nd argument here */
    BOOL ReadMode = FALSE, WriteMode = FALSE;
    int access_mode = 0;

    access_mode |= (FBE_FILE_CREAT | FBE_FILE_TRUNC);
    if ((prot_mode & 0400) | (prot_mode & 0040) | (prot_mode & 0004))
    {
        ReadMode = TRUE;
    }
    if ((prot_mode & 0200) | (prot_mode & 0020) | (prot_mode & 0002))
    {
        WriteMode = TRUE;
    }
    if (ReadMode == TRUE)
    {
        if (WriteMode == TRUE)
        {
            access_mode |= FBE_FILE_RDWR;
        }
        else
        {
            access_mode |= FBE_FILE_RDONLY;
        }
    }
    else if (WriteMode == TRUE)
    {
        access_mode |= FBE_FILE_WRONLY;
    }
    else
    {
        KvTrace("Error in fbe_file_creat: no protection mode defined, defaulting to Read/Write \n");
        access_mode |= FBE_FILE_RDWR;
    }
    return fbe_file_open(path,access_mode, prot_mode, CSX_NULL);
#else
    return fbe_file_open(path, FBE_FILE_CREAT | FBE_FILE_TRUNC | prot_mode, 0, CSX_NULL);
#endif
}

fbe_u64_t fbe_file_get_size(fbe_file_handle_t handle)
{
    csx_status_e status = EMCPAL_STATUS_SUCCESS;
    csx_uoffset_t   seek_pos_rv=0;

    status = csx_p_file_seek(&handle, CSX_P_FILE_SEEK_TYPE_END, 0, &seek_pos_rv);    
    if (!CSX_SUCCESS(status))
    {
        KvTrace("fbe_file_get_size(): csx_p_file_seek failed with status: 0x%08x\n", status);
    }

    return seek_pos_rv;    
}

int fbe_file_delete(const char *path)
{
    csx_status_e status = CSX_STATUS_SUCCESS;
    status = csx_p_file_delete(path);

    if (!CSX_SUCCESS(status))
    {
        KvTrace("fbe_delete_file(): csx_p_file_delete failed with status: 0x%08x\n", status);
        status = FBE_FILE_ERROR; // Unix-style error code
    }
    return status;
}

int fbe_file_find(const char *path) {
    // not implemented
    return FBE_FILE_ERROR;
}

fbe_u64_t fbe_file_lseek(fbe_file_handle_t handle, fbe_u64_t offset, int whence)
{

    csx_status_e status = EMCPAL_STATUS_SUCCESS;
    csx_uoffset_t   seek_pos_rv=0;

    switch (whence)
    {
    case 0:
        // Offset from beginning of file
        status = csx_p_file_seek(&handle, CSX_P_FILE_SEEK_TYPE_SET, offset, &seek_pos_rv); 
        if (!CSX_SUCCESS(status))
        {
            KvTrace("fbe_file_lseek(): csx_p_file_seek failed with status: 0x%08x\n", status);
            return FBE_FILE_ERROR;
        }
        break;

    case 1:
        status = csx_p_file_seek(&handle, CSX_P_FILE_SEEK_TYPE_CUR, offset, &seek_pos_rv);
        if (!CSX_SUCCESS(status))
        {
            KvTrace("fbe_file_lseek(): csx_p_file_seek failed with status: 0x%08x\n", status);
            return FBE_FILE_ERROR;
        }
        break;

      case 2:
        
    default:
          // not supported
          return FBE_FILE_ERROR;
      }

    return seek_pos_rv;    
}

// assumes the file is read from the beginning which is the common usecase
int fbe_file_fgets(fbe_file_handle_t handle, char *buffer, unsigned int nbytes_to_read)
{
    csx_status_e status = EMCPAL_STATUS_SUCCESS;
    unsigned int nBytesToRead = 1;
    unsigned int cumBytesRead = 0;
    unsigned int maxLength = (nbytes_to_read - 1);
    csx_size_t   ActualBytesRead = 0;

    if (nbytes_to_read == 0)
    {
        return FBE_FILE_ERROR;
    }

    buffer[0] = 0;
    while(cumBytesRead < maxLength)
    {
        status = csx_p_file_read(&handle, &buffer[cumBytesRead], nBytesToRead, &ActualBytesRead);            
        if (!CSX_SUCCESS(status)) {
            if (cumBytesRead == 0)
            {
                // Error reading first byte
                KvTrace("fbe_file_fgets(): Error 0x%x reading file.\n", status);
                return FBE_FILE_ERROR;
            }
            else
            {
                // Read some data, return it.
                break;
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

    // NULL terminate the buffer
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
    char temp[2];
    int bytesRead = 0;

    EmcpalZeroVariable(temp);
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
