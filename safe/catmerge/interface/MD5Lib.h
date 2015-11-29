/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/***************************************************************************
 *  FILENAME
 *      MD5Lib.h
 *  
 *  DESCRIPTION
 *      This header file lists the functions and types exported by MD5 library.
 *
 *  HISTORY
 *     14-April-2009     Created.   -Ashwin Tidke
 *
 ***************************************************************************/

#pragma once
/* Headers
 */
#include <windows.h>
//#include <winerror.h>

/* Constants
 */
#define MD5_DIGEST_SIZE 16
#define MD5_BASE64_ENCODED_DIGEST_SIZE 22

//#define ERROR_INVALID_ARGUMENT  ERROR_BAD_ARGUMENTS

#define ERROR_INVALID_FILENAME  2
#define ERROR_FILE_OPEN         3
#define ERROR_FILE_SEEK         4
#define ERROR_FILE_SIZE         6
#define ERROR_FILE_READ         7
#define ERROR_FILE_TELL         8
#define ERROR_MEMORY_ALLOC      9
#define ERROR_GET_PROC          10
#define ERROR_LOADING_DLL       11


/* Function Prototypes
 */
INT CalculateMD5Checksum(PCHAR      FileName,
                          ULONG     Offset,
                          ULONG     FileSize,
                          BOOL      Base64Encode,
                          USHORT    DigestSize,     
                          UCHAR     MD5Checksum[]);
