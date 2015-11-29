/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               BvdSimWindowsIo.h
 ***************************************************************************
 *
 * DESCRIPTION:  windows.h/winbase.h can not be included within the
 *            same source modes as Windows Kernel Include files.
 *            This module was created to hide user space File System
 *            IO declarations from the rest of the simulator environment
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    06/02/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
# include "generic_types.h"

#ifdef ALAMOSA_WINDOWS_ENV

typedef PVOID HANDLE;

extern HANDLE Windows_CreateFile(char *path);
extern BOOL   Windows_DeleteFile(char *path);
extern BOOL   Windows_SetFilePointer(HANDLE handle, INT_64 offset);
extern BOOL   Windows_WriteFile(HANDLE handle, void *buffer, UINT_32 noBytesToWrite, UINT_32 *noBytesWritten);
extern BOOL   Windows_ReadFile(HANDLE handle, void *buffer, UINT_32 noBytesToRead, UINT_32 *noBytesRead);
extern BOOL   Windows_CloseHandle(HANDLE handle);
extern BOOL   Windows_FileExists(char *path);

#endif /* ALAMOSA_WINDOWS_ENV - DEADCODE */
