/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_xml_mem.c
 ***************************************************************************
 *
 *  Description
 *      Defines memory handler functions needed by the Expat library.
 *      This library is only needed in user mode and only if NEIT is not
 *      also linked.
 *
 *
 *  History:
 *      05/19/09    tayloj5 - Created
 *
 ***************************************************************************/

#include "csx_ext.h"
#include <stdlib.h>
#include <string.h>

/***************************************************
 * Define functions needed by the Expat library.
 * Since this is simulation and user mode,
 * we need to define these functions in terms of
 * available functions like malloc and free.
 ***************************************************/

void __stdcall XmlLogTrace(char *LogString, int arg1, int arg2)
{
    /* For user mode, this currently does nothing.
     */
    return;
}

void * __stdcall XmlMalloc( size_t size )
{
    /* For user mode redefine in terms of malloc.
     */
    return malloc(size);
}

void * __stdcall XmlCalloc( size_t num, size_t size )
{
    /* For user mode redefine in terms of XmlMalloc and memset.
     */
    void *ptr = XmlMalloc(size * num);
    memset( ptr, 0, (size * num));
    return ptr;
}
void __stdcall XmlFree( void *memblock )
{
    if (memblock == NULL)
    {
        return;
    }
    else
    {
        free(memblock);
    }
    return;
}
