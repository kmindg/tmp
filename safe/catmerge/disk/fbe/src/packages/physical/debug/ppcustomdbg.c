/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-07
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/***************************************************************************
 * ppcustomdbg.c
 ***************************************************************************
 *
 * File Description:
 *
 *   Startup and entry code for the NtMirror Driver debugging extensions.
 *   This is basically boilerplate, with only a few driver-specific items.
 *
 * Author:
 *   Michael Hamel
 *
 * Revision History:
 *
 * MWH  05/15/02  Based on cmiscd debugging extensions.
 * IM   05/25/07  64bit port
 *
 ***************************************************************************/

#include <string.h>
#include "fbe/fbe_dbgext.h"
#include "csx_ext.h"
#include "fbe_trace.h"
#include "pp_dbgext.h"

#define READ_SIZE 1024

BOOL                            globalCtrlC;

const char EOS = '\000';
const char SP = '\040';
const char HT = '\t';

VOID
csx_dbg_ext_user_initialize_callback()
{
#if DBG
    csx_dbg_ext_print("ppdbg (Debug) Version (x64)\n");
#else
    csx_dbg_ext_print("ppdbg (Retail) Version (x64)\n");
#endif
    csx_dbg_ext_print("ppdbg help is available via '!help' (WinDBG) or 'help extensions' (GDB)\n");
    return;
}


/*
 * char *skipWhitespace (const char *const strPA)
 *
 * Return pointer to strPA without leading whitespace
 *
 * Helper function, taken from \catmerge\common
 */
char* skipWhitespace (const char *const strPA)
{
  char *strP = (char *)strPA;
  
  while (*strP != EOS)
    {
      if ((*strP != SP) && (*strP != HT))
	{
	  break;
	};
      strP++;
    } /* while */
  
  return (strP);
} /* skipWhitespace () */

/* Customize this help text for your debugger extension.
 * It is used by CSX_DBG_EXT_DEFINE_CMD( <module>help ).  A custom name is used to
 * avoid overloading the normal "help" command in WinDbg.
 */
#pragma data_seg ("EXT_HELP$0header")
char ext_help_begin[] =
"!pphelp [COMMAND[*]]\n"
"  !pphelp = Display Help information for all Physical Package debugger extensions\n"
"  !pphelp COMMAND = Display help information for specified COMMAND\n"
"  !pphelp COMMAND* = Display help for all commands beginning with COMMAND\n"
"  !pphelp -l = Display the list of available commands\n";
#pragma data_seg ("EXT_HELP$9trailer")
int ext_help_trailer = 0;
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(pphelp, "pphelp - get help for PPDBG commands")
{
  csx_dbg_ext_print("pphelp is deprecated\n");
  csx_dbg_ext_print("ppdbg help is available via '!help' (WinDBG) or 'help extensions' (GDB)\n");
  return;
} /* CSX_DBG_EXT_DEFINE_CMD ( pphelp ) */

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
//  05/15/02  MWH  Copied from cmiscd debugging extensions.
//  01/06/04  MWH  Added support for single-character alphabetic arguments.
//                 The lower-case form of the character will be returned.
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
			else if (*pBuffer == '`')
			{
				/* Skip ` in 64 bit representation.*/
				pBuffer++;
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

/*
 *
 *
 * DESCRIPTION:
 *
 * PARAMETERS
 *
 * GLOBALS
 *
 * CALLS
 *
 * RETURNS
 *
 * ERRORS
 *
 */
int
ppmRead_In64 (unsigned __int64 addrPA, const char *const symbolPA, void *destPA, unsigned int sizeA)
{
    ULONG64         _adr = addrPA;
    char           *_loc = destPA;
    int             _len = (int)sizeA;
    bool            _status = true;

    while (_len > READ_SIZE && _status)
    {
        if (CHECK_CONTROL_C() || globalCtrlC)
        {
            globalCtrlC = TRUE;		
            return (false);
        };

        // csx_dbg_ext_print ("ntmRead_In64: nth csx_dbg_ext_read_in_len next\n");

        if ((CSX_STATUS_SUCCESS == csx_dbg_ext_read_in_len(_adr, _loc, READ_SIZE)))
        {
            _adr += READ_SIZE;
            _loc += READ_SIZE;
            _len -= READ_SIZE;
        }
    }				/* while */

    if (!_status || ((_len > 0) && (CSX_STATUS_SUCCESS != csx_dbg_ext_read_in_len (_adr, _loc, _len))))
    {
        csx_dbg_ext_print ("Read %s @ 0x%llx failed.\n", symbolPA, addrPA);
        return (false);
    }

    return (true);

}				/* ntmRead_In64 () */

int
ppmRead_In32 (unsigned int addrPA, const char *const symbolPA, void *destPA, unsigned int sizeA)
{
    ULONG64         _adr = addrPA;
    char           *_loc = destPA;
    int             _len = (int)sizeA;
    bool            _status = true;

    while (_len > READ_SIZE && _status)
    {
        if (CHECK_CONTROL_C() || globalCtrlC)
        {
            globalCtrlC = TRUE;		
            return (false);
        };

        if ((CSX_STATUS_SUCCESS == csx_dbg_ext_read_in_len (_adr, _loc, READ_SIZE)))
        {
            _adr += READ_SIZE;
            _loc += READ_SIZE;
            _len -= READ_SIZE;
        }
    }				/* while */

    if (!_status || ((_len > 0) && (CSX_STATUS_SUCCESS != csx_dbg_ext_read_in_len (_adr, _loc, _len))))
    {
        csx_dbg_ext_print ("Read %s @ 0x%16x failed.\n", symbolPA, addrPA);
        return (false);
    }

    return (true);

}				/* ntmRead_In32 () */


/*!**************************************************************
 * pp_check_control_c()
 ****************************************************************
 * @brief
 *  Check to see if control-c has been hit.
 *
 * @param - None.             
 *
 * @return True if control-c was hit, False otherwise.
 *
 * @authorls
 *  3/4/2009 - Created. RPF
 *
 ****************************************************************/

int pp_debug_check_control_c(void)
{
    return(CHECK_CONTROL_C()  || globalCtrlC);
}
/******************************************
 * end pp_check_control_c()
 ******************************************/




/*
 * GetArgument(String, Count)
 *  skip to "Count" argument (1=first, 2=second ...)
 *  prefixes "0x" (hex), "0n" (windbg=decimal), or "0" octal.
 *  defaults to hex if hex digit or > 65536.
 */
INT_PTR GetArgument(const char * String, csx_u32_t Count)
{
    char Buffer[1024];
    char * pBuffer, *p, *rescan;
    INT_PTR i = 0;
    int neg = 0;

    if (strlen(String) < sizeof(Buffer))
    {
        strncpy(Buffer, String, strlen(String)+1);
        pBuffer = Buffer;
    }
    else
    {   
        csx_dbg_ext_print ("GetArgument(%s) failed.\n", String);
        return (-1);
    }

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

    if (*pBuffer == '-')
    {
        pBuffer++, neg++;
    }
    rescan = pBuffer;
    if (*pBuffer == '0' && (*(pBuffer+1) == 'x' || *(pBuffer+1) == 'X'))
    {
        pBuffer+=2;
    hexscan:
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
            else if (*pBuffer == '`') // DWORD`DWORD
            {
                pBuffer++;
            }
            else
                return(0);
        }
    }
    else if (*pBuffer == '0' && (*(pBuffer+1) == 'n'))
    {
        // "0n" is Microsoft windbg notation for decimal
        pBuffer+=2;
        while(*pBuffer >= '0' && *pBuffer <= '9')
            i = (i*10) + (*pBuffer++ - '0');
    }
    else if (*pBuffer == '0')
    {
        while(*pBuffer >= '0' && *pBuffer <= '7')
            i = (i*8) + (*pBuffer++ - '0');
        if ((*pBuffer >= 'a' && *pBuffer <= 'f') ||
            (*pBuffer >= 'A' && *pBuffer <= 'F'))
        {
            i = 0;
            pBuffer = rescan;
            goto hexscan;
        }
    }
    else
    {
        int hex = 0;
        // Default hex if >64K
        while(pBuffer && *pBuffer != '\0')
        {
            if (*pBuffer >= '0' && *pBuffer <= '9')
            {
                i = (i*16) + (*pBuffer++ - '0');
            }
            else if (*pBuffer >= 'a' && *pBuffer <= 'f')
            {
                i = (i*16) + ((*pBuffer++ - 'a') + 10);
                hex=1;
            }
            else if (*pBuffer >= 'A' && *pBuffer <= 'F')
            {
                i = (i*16) + ((*pBuffer++ - 'A') + 10);
                hex=1;
            }
            else if (*pBuffer == '`') // DWORD`DWORD
            {
                pBuffer++;
            }
            else
                return(0);
        }
        // smaller than 65536 with no hex digits interpreted in decimal.
        if (((ULONG)i < (ULONG)0x65536) && (hex == 0))
        {
            i = 0;
            pBuffer = rescan;
            while(*pBuffer >= '0' && *pBuffer <= '9')
                i = (i*10) + (*pBuffer++ - '0');
        }
    }
    if (neg)
        i = -i;
    return i;
}

