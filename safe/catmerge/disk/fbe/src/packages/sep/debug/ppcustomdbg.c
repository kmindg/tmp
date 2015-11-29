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
#include "csx_ext.h"
#include "bool.h"

#include "dbgext.h"

#define READ_SIZE 1024

//
// Global variables
//
static EXT_API_VERSION          ApiVersion = { 3, 6, EXT_API_VERSION_NUMBER64, 0 };
       WINDBG_EXTENSION_APIS    ExtensionApis;
static USHORT                   SavedMajorVersion;
static USHORT                   SavedMinorVersion;

static CHAR                     TrueStr[]  = "TRUE";
static CHAR                     FalseStr[] = "FALSE";

BOOL                            globalCtrlC;

const char EOS = '\000';
const char SP = '\040';
const char HT = '\t';

LPEXT_API_VERSION ExtensionApiVersion( VOID );

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
 * It is used by DECLARE_API( <module>help ).  A custom name is used to
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

//
//  Description:
//      This function performs some required initialization for extension DLLs
//      and is a required function for a windbg extension.
//
//      The module-specific csx_dbg_ext_print is also useful.
//
//  Arguments:
//	NONE
//
//  Return Value:
//	NONE
//
VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    LPEXT_API_VERSION pApiVersion;

    pApiVersion       =  ExtensionApiVersion();
    ExtensionApis     = *lpExtensionApis;
    SavedMajorVersion =  MajorVersion;
    SavedMinorVersion =  MinorVersion;

#if DBG
    csx_dbg_ext_print("ppdbg (checked) Version %d.%d(x64)\n", 
        pApiVersion->MajorVersion, pApiVersion->MinorVersion);
#else
    csx_dbg_ext_print("ppdbg (free) Version %d.%d(x64)\n", 
        pApiVersion->MajorVersion, pApiVersion->MinorVersion);
#endif
    csx_dbg_ext_print("ppdbg help is available via !pphelp\n");
    return;
}


//
//  Function:
//	CheckVersion
//
//  Description:
//	This function gets called before each extension command is executed.
//	Its job is to complain if the version of NT being debugged doesn't
//	match the version of the extension DLL.  It is a required function
//      for a windbg extension.
//
//  Arguments:
//	NONE
//
//  Return Value:
//	NONE
//
VOID
CheckVersion(
    VOID
    )
{
    globalCtrlC = FALSE;
    return;
}

char *
moreHelp (char* nextPA)
{
  char *retP;

  retP = nextPA + strlen(nextPA);

  while (retP < (char*)&ext_help_trailer)
    {
      if (*retP != EOS)
	{
	  return (retP);
	};
      retP++;
    } /* while */
  
  return ((char*)NULL);
} /* moreHelp() */

//
//  Description:
//      This function returns the version of the extension DLL itself
//      and is a required function for a windbg extension.
//
//  Arguments:
//	NONE
//
//  Return Value:
//	Pointer to a version structure
//
LPEXT_API_VERSION
ExtensionApiVersion( VOID )
{
    return &ApiVersion;
}

DECLARE_API( pphelp )
{
  size_t argLen;		   /* model string length less '!' and '*' */
  char  *modelP  = skipWhitespace (args); /* argument w/o leading whitespace */
  char  *nextP   = ext_help_begin; /* beginning of help text" */
  bool   partial = false;          /* are we doing a partial match search? */


  if (modelP[0] == '!')		/* dump leading '!' in model string */
    {
      modelP++;
    };

  argLen = strlen (modelP);	/* how long is it? */

  /*
   * !help COMMAND* ??
   */
  if (modelP[argLen - 1] == '*') /* does it end in '*'? */
    {
      argLen--;			/* yes, omit '*' and do partial matches */

      while (true)
	{
	  if (CheckControlC() || globalCtrlC)
	    {
	      globalCtrlC = TRUE;		
	      return;
	    };
	  if (strncmp (modelP, &nextP[1], argLen) == 0)
	    {
	      csx_dbg_ext_print ("%s\n", nextP);
	    }; /* if modelP */
	  if ((nextP = moreHelp (nextP)) == (char*)NULL)
	    {
	      return;
	    };
	} /* while */
      return;
    }; /*  if (arg[argLen - 1] == '*') */

	/*
     * !help -l
     */
   if((modelP[argLen - 1] == 'l') && (modelP[argLen - 2] == '-')) /* check if the last two characters are -l*/
   {
	   int tab = 0;
	   char *targetP;
       char tokBuf[64];
       memset(tokBuf,0,sizeof(tokBuf));
	   while (true)
		{
			if (CheckControlC() || globalCtrlC)
			{
				globalCtrlC = TRUE;		
				return;
			};
			if(strncmp(nextP,"!",1) == 0)
			{
				strncpy (tokBuf, &nextP[1], (sizeof (tokBuf)-1));
				targetP = strtok (tokBuf," \n\t");
				csx_dbg_ext_print ("%-20s", targetP);
				tab++;
				if ((tab % 4) == 0)	/* Display four commands in a row */
				{
					csx_dbg_ext_print("\n");
				}
			}
			if ((nextP = moreHelp (nextP)) == (char*)NULL)
			{
				return;
			};
			
		}/* while */
	   return; 
   } /* if modelP[argLen - 1] == 'l' */



  /*
   * !help COMMAND ??
   */
  if (argLen != 0)		/* do exact matches */
    {
      char *targetP;
      char tokBuf[64];
      size_t tokLen;

      memset(tokBuf,0,sizeof(tokBuf));
      while (true)
	{
	  if (CheckControlC() || globalCtrlC)
	    {
	      globalCtrlC = TRUE;		
	      return;
	    };
	  strncpy (tokBuf, &nextP[1], (sizeof (tokBuf)-1));
	  targetP = strtok (tokBuf, "\t\n ");
	  if (targetP == (char*)NULL)
	    {
	      break;		/* odd, just show everything */
	    };

	  tokLen = strlen (tokBuf);
	  if (strncmp (modelP, tokBuf, tokLen) == 0)
	    {
	      csx_dbg_ext_print ("%s\n", nextP);
	      return;
	    }; /* if modelP */

	  if ((nextP = moreHelp (nextP)) == (char*)NULL)
	    {
	      return;
	    };
	} /* while */
    }; /* if strLen */

  /*
   * !help  ??
   */
  while (true)
    {
      if (CheckControlC() || globalCtrlC)
	{
	  globalCtrlC = TRUE;		
	  return;
	};
      csx_dbg_ext_print ("%s\n", nextP);
      if ((nextP = moreHelp (nextP)) == (char*)NULL)
	{
	  return;
	};
    } /* while */

  return;
} /* DECLARE_API ( pphelp ) */

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
csx_s64_t GetArgument64(const char * String, ULONG Count)
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
    ULONG           _bytes;
    ULONG64         _adr = addrPA;
    void           *_loc = destPA;
    int             _len = (int)sizeA;
    bool            _status = true;

    while (_len > READ_SIZE && _status)
    {
        if (CheckControlC () || globalCtrlC)
        {
            globalCtrlC = TRUE;		
            return (false);
        };

        // csx_dbg_ext_print ("ntmRead_In64: nth ReadMemory next\n");

        if ((_status = ReadMemory (_adr, _loc, READ_SIZE, &_bytes)))
        {
            (char *) _adr += READ_SIZE;
            (char *) _loc += READ_SIZE;
            _len -= READ_SIZE;
        }
    }				/* while */

    if (!_status || ((_len > 0) && (!ReadMemory (_adr, _loc, _len, &_bytes))))
    {
        csx_dbg_ext_print ("Read %s @ 0x%P failed.\n", symbolPA, addrPA);
        return (false);
    }

    return (true);

}				/* ntmRead_In64 () */

int
ppmRead_In32 (unsigned int addrPA, const char *const symbolPA, void *destPA, unsigned int sizeA)
{
    ULONG           _bytes;
    ULONG64         _adr = addrPA;
    void           *_loc = destPA;
    int             _len = (int)sizeA;
    bool            _status = true;

    while (_len > READ_SIZE && _status)
    {
        if (CheckControlC () || globalCtrlC)
        {
            globalCtrlC = TRUE;		
            return (false);
        };

        if ((_status = ReadMemory (_adr, _loc, READ_SIZE, &_bytes)))
        {
            (char *) _adr += READ_SIZE;
            (char *) _loc += READ_SIZE;
            _len -= READ_SIZE;
        }
    }				/* while */

    if (!_status || ((_len > 0) && (!ReadMemory (_adr, _loc, _len, &_bytes))))
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
 * @author
 *  3/4/2009 - Created. RPF
 *
 ****************************************************************/

int pp_debug_check_control_c(void)
{
    return(CheckControlC () || globalCtrlC);
}
/******************************************
 * end pp_check_control_c()
 ******************************************/



