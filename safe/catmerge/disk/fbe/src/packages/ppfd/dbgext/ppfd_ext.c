/***************************************************************************
 *  Copyright (C)  EMC Corporation 2009
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/***************************************************************************
 * ppfd_ext.c
 ***************************************************************************
 *
 * File Description:
 *   Debugging extensions for ppfd.
 *
 * Author:
 *
 * Revision History:
 *
*
 ***************************************************************************/
#include <stdio.h> // for sprintf

// Global Includes
#include "dbgext.h"

// Local Includes
#include "ppfdMisc.h"
#include "fbe/fbe_dbgext.h"

#ifdef	PPFD_DEBUG
#define dbgprintf csx_dbg_ext_print
#else
#define dbgprintf
#endif

const char EOS = '\000';
const char SP = '\040';
const char HT = '\t';

#define DISK_TYPE                  2 

static ULONG64 ppfdGlobal;

//  Description: 
//  Store the pointer to PPFD_GDS global data tracking structure declared in PPFD to a global in this  
//  debugger extension file (ppfdGlobal).  It will be used by some of the commands.
//
//  Arguments:
//  None
//
//  Return Value:
//  Return 0 on error.
//
BOOL ppfdReadInPPFDGlobal(void)
{
    BOOL rc = TRUE;
    
    FBE_GET_EXPRESSION("PPFD", PPFD_GDS, &ppfdGlobal)
            
    if (!ppfdGlobal )
    {
        csx_dbg_ext_print("Could not get address of PPFD_GDS\n");
        rc = FALSE;
    }
    else
    {
        dbgprintf("PPFD_GDS address 0x%llx\n", (unsigned long long)ppfdGlobal);
    }

    return(rc);
} // end of ppfdReadInPPFDGlobal()


//  Function: ppfdDisplayBeTypeInformation
//  Description: 
//      This function prints the backend 0 type that PPFD detected
//      PPFD detects this by determining what type of SLIC is inserted in slot 0 of the SP
//
//  Arguments:
//      none
//
//  Return Value:
//  NONE
//
void ppfdDisplayBeTypeInformation(void)
{
    ULONG beType = 0;

    static char* beTypeStr[] = {
        "UNSUPPORTED BACKEND TYPE",
        "FC BACKEND",
        "SAS or SATA BACKEND",
        "SATA BACKEND" };

    FBE_GET_FIELD_DATA("PPFD", ppfdGlobal, PPFD_GLOBAL_DATA, beType, sizeof(BACKEND_TYPE), &beType)
    csx_dbg_ext_print("Backend Type = %d, %s\n", beType, beTypeStr[beType] );

}


//  Function: ppfdDisplaySpId
//  Description: 
//      This function prints the ID of the SP that PPFD detected by calling the SPID driver
//      Either SPA or SPB.
//
//  Arguments:
//      none
//
//  Return Value:
//  NONE
//
void ppfdDisplaySpId(void)
{
    ULONG SpId = 0;

    static char* spTypeStr[] = {
        "SPA",
        "SPB" };
    
    FBE_GET_FIELD_DATA("PPFD", ppfdGlobal, PPFD_GLOBAL_DATA, SpId, sizeof(SP_ID), &SpId)

    csx_dbg_ext_print("PPFD detected SpId = %d, %s\n", SpId, spTypeStr[SpId] );
}


//
//  Description: 
//  Get pointer to PPFDDiskObject pointer from ppfd global data structure PPFDDiskObjectsMap
//
//  Arguments:
//  instance is used as index to read and print the contents from PPFDDiskObjectsMap array.
//
//  Return Value:
//  Return pointer of instance index of ppfdGlobal pointer to array variable.
//
ULONG64 ppfdGetPPFDDiskObjectPtr(ULONG instance)
{
    ULONG Size;
    ULONG64 PPFDDiskObjectPtr = 0;
    ULONG Offset = 0;
          
    FBE_GET_TYPE_SIZE("PPFD", PPFDDISKOBJ, &Size)
        
    FBE_GET_FIELD_OFFSET("PPFD", PPFD_GLOBAL_DATA, "PPFDDiskObjectsMap" , &Offset)
                    
    FBE_GET_EXPRESSION("PPFD", PPFD_GDS, &PPFDDiskObjectPtr)

    PPFDDiskObjectPtr += Offset + (instance * Size);

    return (PPFDDiskObjectPtr);
} // end of ppfdGetPPFDDiskObjectPtr()


//  Function:  ppfdDisplayDiskInformation(
//  Description: 
//      This function prints all disks information from PPFDDiskObjectsMap array.
//
//  Arguments:
//  indent to be used while printing infomation.
//
//  Return Value:
//  NONE
//
void ppfdDisplayDiskInformation( const char *indent)
{
    ULONG64 pPPFDDISKObjectPtr = 0;
    ULONG64 pDiskInformation = 0;
    ULONG Offset = 0;
    UCHAR PortNumber = 0;
    UCHAR PathId = 0;
    UCHAR TargetId = 0;
    UCHAR Lun = 0; 
    fbe_object_id_t DiskObjID = 0;
    UCHAR DiskLoop = 0;
    UCHAR DiskTypeLoop = 0;
    CHAR  DiskType[10] = {0};


    csx_dbg_ext_print("%sDiskNo\tPort\tPathID\tTargetID\tLun\tObjId\tType\n", indent);
    csx_dbg_ext_print("----------------------------------------------------------------\n");

    for (DiskTypeLoop = 0; DiskTypeLoop < DISK_TYPE; DiskTypeLoop++)
    {
        for (DiskLoop = 0; DiskLoop < TOTALDRIVES;DiskLoop++)
        {
            pPPFDDISKObjectPtr = ppfdGetPPFDDiskObjectPtr(DiskLoop);
            if(!pPPFDDISKObjectPtr)

            {                  
                  csx_dbg_ext_print("pPPFDDISKObjectPtr = NULL Error!\n");
                  break;
            }

            if(DiskTypeLoop == 0)
            {
                 //Read pnp disk information from structure
                 FBE_GET_FIELD_OFFSET("PPFD", PPFDDISKOBJ, "ScsiAddrPPFDpnpDisk", &Offset)

                 strcpy(DiskType, "PNP");
            }
            else
            {

                 //Read non pnp disk information from structure
                 FBE_GET_FIELD_OFFSET("PPFD", PPFDDISKOBJ, "ScsiAddrPPFDDisk", &Offset)

                 strcpy(DiskType, "non-PNP");
            }

            pDiskInformation = pPPFDDISKObjectPtr + Offset;

            FBE_GET_FIELD_DATA("PPFD", pDiskInformation, SCSI_ADDRESS, PortNumber, sizeof(UCHAR), &PortNumber)
            FBE_GET_FIELD_DATA("PPFD", pDiskInformation, SCSI_ADDRESS, PathId, sizeof(UCHAR), &PathId)
            FBE_GET_FIELD_DATA("PPFD", pDiskInformation, SCSI_ADDRESS, TargetId, sizeof(UCHAR), &TargetId)
            FBE_GET_FIELD_DATA("PPFD", pDiskInformation, SCSI_ADDRESS, Lun, sizeof(UCHAR), &Lun)
            FBE_GET_FIELD_DATA("PPFD", pPPFDDISKObjectPtr, PPFDDISKOBJ, DiskObjID, sizeof(fbe_object_id_t), &DiskObjID)

            //Print values read from structure on to the screen
            csx_dbg_ext_print("%s%d\t\t%d\t%d\t\t%d\t\t%d\t0x%x\t%s\n", indent, DiskLoop,PortNumber,PathId,TargetId, Lun, DiskObjID, DiskType);

        }
    }

}


//  Command: ppfddisk
//  Description: 
//      This command prints all disks information from PPFDDiskObjectsMap array.
//
//  Arguments:
//  None
//
//  Return Value:
//  None
//
#pragma data_seg ("EXT_HELP$4ppfddisk")
static char CSX_MAYBE_UNUSED usageMsg_ppfddisk[] =
"!ppfddisk\n"
"  Display ppfd disk information\n";
#pragma data_seg ()
CSX_DBG_EXT_DEFINE_CMD(ppfddisk, "ppfddisk")
{
    const char indent[] = {" "};
    globalCtrlC = FALSE;

    if(!ppfdReadInPPFDGlobal())
    {
        return;
    }

    ppfdDisplayDiskInformation(indent);

} // end of ppfddisk


//  Command: ppfdbetype
//  Description: 
//  This command prints the backend 0 type that PPFD detected
//  PPFD detects this by determining what type of SLIC is inserted in slot 0 of the SP
//
//  Arguments:
//      none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4ppfdbetype")
static char CSX_MAYBE_UNUSED usageMsg_ppfdbetype[] =
"!ppfdbetype\n"
"  Display the boot drive backend type detected by ppfd\n";
#pragma data_seg ()
CSX_DBG_EXT_DEFINE_CMD(ppfdbetype, "ppfdbetype")
{
    globalCtrlC = FALSE;

    if(!ppfdReadInPPFDGlobal())
    {
        return;
    }
    ppfdDisplayBeTypeInformation();

} // end of ppfdbetype


//  Command: ppfdspid
//  Description: 
//      This command prints the ID of the SP that PPFD detected by calling the SPID driver
//      Either SPA or SPB.
//
//  Arguments:
//      none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4ppfdspid")
static char CSX_MAYBE_UNUSED usageMsg_ppfdspid[] =
"!ppfdspid\n"
"  Display the SP ID that PPFD detects (gets it from SPID).\n" 
"  This is used for intializing crash dump support in the SASPMC driver\n"
"  and for detecting the I/O module type in slot 0 of the SP we are booting on";
#pragma data_seg ()
CSX_DBG_EXT_DEFINE_CMD(ppfdspid, "ppfdspid")
{
    globalCtrlC = FALSE;    
    if(!ppfdReadInPPFDGlobal())
    {
        return;
    }    
    ppfdDisplaySpId();

} // end of ppfdSpId


/* Customize this help text for your debugger extension.
 * It is used by CSX_DBG_EXT_DEFINE_CMD( <module>help ).  A custom name is used to
 * avoid overloading the normal "help" command in WinDbg.
 */
#pragma data_seg ("EXT_HELP$0header")
char ext_help_begin[] =
"!ppfdhelp [COMMAND[*]]\n"
"  !ppfdhelp = Display Help information for all PPFD Driver debugger extensions\n"
"  !ppfdhelp COMMAND = Display help information for specified COMMAND\n"
"  !ppfdhelp COMMAND* = Display help for all commands beginning with COMMAND\n"
"  !ppfdhelp -l = Display the list of available commands\n";
#pragma data_seg ("EXT_HELP$9trailer")
int ext_help_trailer = 0;
#pragma data_seg ()

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

//
//  Description: Extension to display PPFD help
//
//  Arguments:
//  None
//
//  Return Value:
//  None
//

CSX_DBG_EXT_DEFINE_CMD( ppfdhelp, "ppfdhelp")
{
  csx_dbg_ext_print("ppfdhelp is deprecated\n");
  csx_dbg_ext_print("ppfdhelp help is available via '!help' (WinDBG) or 'help extensions' (GDB)\n");
  return;
} /* CSX_DBG_EXT_DEFINE_CMD ( ppfdhelp ) */
