#ifndef __mempeek_c_interface
#define __mempeek_c_interface
/*@LB************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****
 ****  Copyright (c) 2004-2009 EMC Corporation.
 ****  All rights reserved.
 ****
 ****  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.
 ****  ACCESS TO THIS SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT.
 ****  THIS CODE IS TO BE KEPT STRICTLY CONFIDENTIAL.
 ****
 ****************************************************************************
 ****************************************************************************
 *@LE************************************************************************/

/*@FB************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****
 **** FILE: mempeek.h
 ****
 **** DESCRIPTION:
 ****    Memory peeking device. Cross-address space memory peeking made easy!
 ****
 ****    Base defines and C Client interface
 ****
 **** NOTES:
 ****    See mempeek.hpp for usage patterns
 ****
 ****************************************************************************
 ****************************************************************************
 *@FE************************************************************************/

#include "csx_ext.h"
 
#ifdef __cplusplus
extern "C" {
#endif 

/*
 * This define used to denote pointers in foreign address space.
 * Just for easier readability
 * Example: FOREIGN PULONG pIndex; // ptr to ULONG var in another address space
 */
#ifndef FOREIGN
#define FOREIGN
#endif

#define MEMPEEK_GENERIC_DEVICE_NAME "MemPeekDevice"

typedef enum
{
    PEEK_LINEAR = 0x10,
    PEEK_STRLEN,
    PEEK_C_STRING,
    PEEK_C_STRING_L
} mempeek_ioctl;

    
typedef struct _mempeek_io_chunk_s
{
    void* from;
    csx_size_t num_bytes;
} mempeek_io_chunk_t;

#ifdef __cplusplus
}; //extern "C"
#endif

#endif //__mempeek_c_interface

