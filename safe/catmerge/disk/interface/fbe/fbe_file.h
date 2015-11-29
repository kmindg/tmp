#ifndef FBE_FILE_H
#define FBE_FILE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_file.h
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the api for the fbe file access functions.
 *
 * HISTORY
 *   11/09/2007:  Ported to FBE From Flare. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "EmcPAL.h"

/*
**	This provides the file_io operations definitions.
*/
enum fbe_file_modes_e
{
    FBE_FILE_RDONLY	= 0x000,
    FBE_FILE_WRONLY	= 0x001,
    FBE_FILE_RDWR	= 0x002,
    FBE_FILE_NDELAY	= 0x004,
    FBE_FILE_APPEND	= 0x008,
    FBE_FILE_CREAT	= 0x100,
    FBE_FILE_TRUNC	= 0x200,
    FBE_FILE_UNBUF  = 0x400,
    FBE_FILE_NCACHE = 0x800,
//  Commented because exclusive file access is not implemented in CSX.
//  CSX has not implemented because its not being used.
//  FBE_FILE_EXCL   = 0x1000 
};

/* These are the access modes.
 */
enum fbe_file_access_modes_e
{
    FBE_WRITE_ACCESS = 2,
    FBE_READ_ACCESS  = 4
};

/* Define the file handle type.
 */
typedef csx_p_file_t fbe_file_handle_t;

#define FBE_FILE_SEEK_ERR 0xFFFFFFFF
#define FBE_FILE_INVALID_HANDLE (fbe_file_handle_t)-1
#define FBE_FILE_ERROR (int)-1

typedef csx_status_e fbe_file_status_t;

/****************************************
 * These are the access functions for
 * reading/writing/manipulating files.
 ****************************************/
extern fbe_file_handle_t fbe_file_open(const char *path, int open_flag, unsigned long protection_mode, fbe_file_status_t *err_status);
extern fbe_u64_t fbe_file_lseek (fbe_file_handle_t handle, fbe_u64_t offset, int whence);
extern int fbe_file_read (fbe_file_handle_t handle, void *buffer, unsigned int nbyte, fbe_file_status_t *err_status);
extern int fbe_file_write (fbe_file_handle_t handle, const void *buffer, unsigned int nbyte, fbe_file_status_t *err_status);
extern int fbe_file_fgets(fbe_file_handle_t handle, char *buffer, unsigned int nbytes_to_read);
extern int fbe_file_fgetc(fbe_file_handle_t handle);
extern int  fbe_file_close (fbe_file_handle_t handle);
#ifdef C4_INTEGRATED
extern int  fbe_file_sync_close (fbe_file_handle_t handle);
#endif /* C4_INTEGRATED - C4ARCH - file differences - C4WHY */
extern int  fbe_file_unlink (const char *path);
 
extern fbe_file_handle_t fbe_file_creat (const char *path,int mode);
extern int  fbe_file_rename (const char *old_path, const char *new_path);
extern int  fbe_file_access(const char *path, int amode);

extern long fbe_file_set_size(fbe_file_handle_t handle, long offset);
extern fbe_u64_t fbe_file_get_size(fbe_file_handle_t handle);

extern int fbe_file_delete(const char *path);
extern int fbe_file_find(const char *path);

#endif	// FBE_FILE_H
