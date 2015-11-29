/***************************************************************************
 *  Copyright (C)  EMC Corporation 2009
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/***************************************************************************
 * ppfdcustomdbg.c
 ***************************************************************************
 *
 * File Description:
 *
 *   Startup and entry code for the PPFD Driver debugging extensions.
 *   This is basically boilerplate, with only a few driver-specific items.
 *
 * Author:
 *
 * Revision History:
 *
 ***************************************************************************/

#include <string.h>
#include "fbe/fbe_dbgext.h"
#include "csx_ext.h"
#include "bool.h"

BOOL                            globalCtrlC;

VOID
csx_dbg_ext_user_initialize_callback()
{
#if DBG
    csx_dbg_ext_print("pppdf (Debug) Version %s %s (x64)\n", __DATE__, __TIME__);
#else
    csx_dbg_ext_print("pppdf (Retail) Version %s %s (x64)\n",  __DATE__, __TIME__);
#endif
    csx_dbg_ext_print("pppdf help is available via '!help' (WinDBG) or 'help extensions' (GDB)\n");
    return;
}

      
//
//  Description:
//      This function parses a command-line and returns a single argument.
//      Function prototype defined in dbgext.h.
//
//      Hex arguments are expected to begin with "0x". Alpha characters will
//      otherwise cause the argument scan to stop UNLESS it is a single-
//      character argument.  
//      
//      Examples: "a" will return 0x61, 
//                "1234" or "1234a567" will return 0x4D2 (i.e. 1234),
//                "0x12345" will return 0x12345, but
//                "a12345" will return 0x00.
//
//  Arguments:
//  String - Pointer to command-line arguments
//  Count - Argument desired (1 for the first arg, 2 for the second arg, etc)
//
//  Return Value:
//	Integer value of the argument
//
//  History:
//  03/20/09   Copied from ntmirror debugging extensions.
//
csx_s64_t GetArgument64(const char * String, csx_u32_t Count)
{
    char Buffer[1024];
    char * pBuffer, *p;
    csx_s64_t i = 0;
	int neg = 0;

    strncpy(Buffer, String, (sizeof(Buffer)-1));
    // pBuffer = strtok(Buffer, " ");
    pBuffer = Buffer;
 strtok:
    p = pBuffer;
    while(*p && *p != ' ' && *p != '\t') p++;
    if (*p) *p++ = '\0';
    while(*p && (*p == ' ' || *p == '\t')) p++;
    
    if (Count > 1)
    {
        pBuffer = p; // strtok(NULL, " ");
        Count--;
        goto strtok;
    }

    // return atoi(pBuffer);
    if (*pBuffer == '-')
        pBuffer++, neg++;
	if (*pBuffer == '0' && (*(pBuffer+1) == 'x' || *(pBuffer+1) == 'X'))
	{
        // hexidecimal
		pBuffer++;
		pBuffer++;
		while(pBuffer && *pBuffer != '\0')
		{
			if (*pBuffer >= '0' && *pBuffer <= '9')
			{
				i = (i*16) + (*pBuffer++ - '0');
			}
			else if (*pBuffer >= 'a' && *pBuffer <= 'f')
			{
				i = (i*16) + ((*pBuffer++ - 'a') + 10);
			}
			else if (*pBuffer >= 'A' && *pBuffer <= 'F')
			{
				i = (i*16) + ((*pBuffer++ - 'A') + 10);
			}
			else
				return(0);
		}
	}
    else if ((*pBuffer >= 'a' && *pBuffer <= 'z') && 
             ((*(pBuffer+1) == ' ') || (*(pBuffer+1) == '\0')))
    {
        // This is a single-character argument, so stop scanning.
        i = *pBuffer;
        neg = 0;
    }
    else if ((*pBuffer >= 'A' && *pBuffer <= 'Z') &&
             ((*(pBuffer+1) == ' ') || (*(pBuffer+1) == '\0')))
    {
        // This is a single-character argument, so stop scanning.

        // convert to lower case before returning
        i = 'a' + (*pBuffer - 'A');
        neg = 0;
    }
	else if (*pBuffer == '0')
	{
        // octal
		while(*pBuffer >= '0' && *pBuffer <= '7')
			i = (i*8) + (*pBuffer++ - '0');
	}
	else
	{
        // decimal
		while(*pBuffer >= '0' && *pBuffer <= '9')
			i = (i*10) + (*pBuffer++ - '0');
	}
	if (neg)
		i = -i;
  	return i;
}
