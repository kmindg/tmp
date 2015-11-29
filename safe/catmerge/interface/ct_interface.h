#ifndef CT_INTERFACE_H
#define CT_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2005-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * FILE NAME: 
 *      ct_interface.h
 *
 * DESCRIPTION
 *      This header file is created to define the structures and functions
 *      for the APIs in the conversion interface library.
 * 
 * STRUCTURES:
 *      CT_REGION_DESCRIPTOR_TYPE
 *      CT_STATUS_TYPE
 *      CT_MESSAGE_DESTINATION_TYPE
 *      CT_SECTOR_SIZE_TYPE
 *
 * FUNCTIONS:
 *      CT_Register()
 *      CT_SetParameter()
 *      CT_GetParameter()
 *      CT_DeleteParameters()
 *      CT_DumpParameters()
 *      CT_Message()
 *      CT_ReadRegion()
 *      CT_WriteRegion()
 *      CT_CopyRegion()
 *      CT_ZeroRegion()
 *      CT_GetIRFile()
 *      CT_PutIRFile()
 *      CT_DeleteIRFile()
 *      CT_FlushKtraceToIR()
 *      CT_GetStatusString()
 *      CT_Exit()
 *	CT_DM_GetIOModuleStatusThisBlade()
 *	CT_DM_GetIOModuleSummary()
 *	CT_DM_GetSpId()
 *	CT_DM_GetHwType()	
 *	CT_DM_GetPeerHwType()	
 *	CT_DM_GetHwName()
 * 
 ***************************************************************************/

#include <stdio.h>
#include "user_generics.h"
#include "specl_types.h"
#include "spid_types.h"


// The maximum length of a conversion tool name
#define CT_MAX_CT_NAME 32

// The maximum length of a CT parameter name
#define CT_MAX_PARAMETER_NAME_LENGTH 64

// The maximum length of a CT parameter
#define CT_MAX_PARAMETER_LENGTH 4096

// The maximum length of a CT parameter pattern
#define CT_MAX_PARAMETER_PATTERN_LENGTH 512

// The maximum length of the message for CT_Message
#define CT_MAX_MSG_LENGTH 1024

// The maximum length of the file name
#define CT_MAX_FILE_NAME 512

//  Define the sector sizes that are supported for IO operations to disk regions
typedef enum _CT_SECTOR_SIZE_TYPE
{
    CT_SECTOR_SIZE_512,
    CT_SECTOR_SIZE_520
}CT_SECTOR_SIZE_TYPE;

//  Describe the destination for messages created by CT_Message interface.
//  The bit mask identifies one or more message destinations.
typedef BITS_32 CT_MESSAGE_DESTINATION_TYPE;
#define CT_MESSAGE_TO_UI        0x1
#define CT_MESSAGE_TO_KTRACE    0x2
#define CT_MESSAGE_TO_ALL       (CT_MESSAGE_TO_UI | CT_MESSAGE_TO_KTRACE)

//  Check if CT_MESSAGE_DESTINATION is invalid, used in argument check in ct_lib.c
//  (~CT_MESSAGE_TO_ALL & X) prevents the invalid case that one or more 
//  bits(bit2-bit7) are 1 such as 0x10000000, 0x01000011
//  (X==0) prevents the invalid case such as 0x00000000
#define CT_MESSAGE_DESTINATION_IS_VALID(X) !((~CT_MESSAGE_TO_ALL & X)||(X==0))

//  CT error start value
#define CT_STATUS_FIRST_ERROR_VALUE   0x1000

//  This type defines the return status from CT interface functions.
//  It also makes a distinction between internal errors and errors caused 
//  by hard IO failures.
typedef enum  _CT_STATUS_TYPE
{
    // OK status
    CT_STATUS_OK = 0x0,

    // Unused CT status error code for code compatibility
    CT_STATUS_UNUSED = CT_STATUS_FIRST_ERROR_VALUE,

    // Internal error
    CT_STATUS_INTERNAL_ERROR = CT_STATUS_FIRST_ERROR_VALUE + 0x1,

    // Parameter not found
    CT_STATUS_PARAMETER_NOT_FOUND = CT_STATUS_FIRST_ERROR_VALUE + 0x2,

    // CT interface version does not match the server version
    CT_STATUS_VERSION_ERROR = CT_STATUS_FIRST_ERROR_VALUE + 0x3,

    // Cannot access the device
    CT_STATUS_DEVICE_INACCESSABLE = CT_STATUS_FIRST_ERROR_VALUE + 0x4,

    // Unrecoverable IO error with unknown read or write error
    CT_STATUS_IO_ERROR = CT_STATUS_FIRST_ERROR_VALUE + 0x5,

    // IO error in reading from a disk region
    CT_STATUS_IO_READ_ERROR = CT_STATUS_FIRST_ERROR_VALUE + 0x6,

    // IO error in writing to a disk region
    CT_STATUS_IO_WRITE_ERROR = CT_STATUS_FIRST_ERROR_VALUE + 0x7,

    // File not found in
    CT_STATUS_FILE_NOT_FOUND = CT_STATUS_FIRST_ERROR_VALUE + 0x8,

    // Parameters missing or incorrect
    CT_STATUS_ARGUMENT_ERROR = CT_STATUS_FIRST_ERROR_VALUE + 0x9,

    // No enough buffer space
    CT_STATUS_INSUFFICIENT_MEMORY = CT_STATUS_FIRST_ERROR_VALUE + 0xA,

    // Unable to connect to the server
    CT_STATUS_UNABLE_TO_CONNECT = CT_STATUS_FIRST_ERROR_VALUE + 0xB,

    // Tool not registered 
    CT_STATUS_NOT_CONNECTED = CT_STATUS_FIRST_ERROR_VALUE + 0xC,

    // Already registered, no need to register again
    CT_STATUS_ALREADY_CONNECTED = CT_STATUS_FIRST_ERROR_VALUE + 0xD,

    // Other error
    CT_STATUS_OTHER_ERROR = CT_STATUS_FIRST_ERROR_VALUE + 0xE,

    // IRFS unavailable
    CT_STATUS_IRFS_UNAVAILABLE = CT_STATUS_FIRST_ERROR_VALUE + 0xF

} CT_STATUS_TYPE;

//  This structure specifies a region of space on a backend disk. It is used 
//  in functions that read or write to backend disks. Note: an enclosure 
//  number is not currently part of this structure because FLARE identifies 
//  disks based on consecutive disk number on each bus and we are only
//  using the first enclosure for now anyway. 
//  Also, the Bus number should always be zero.
typedef struct _CT_REGION_DESCRIPTOR_TYPE
{
    UINT_16E                crd_Bus;
    UINT_16E                crd_Disk;
    UINT_32E                crd_RegionOffset;      // aka Disk LBA
    UINT_32E                crd_RegionLength;
    CT_SECTOR_SIZE_TYPE     crd_SectorSize;
} CT_REGION_DESCRIPTOR_TYPE;

/*************************  F U N C T I O N S  ***************************/

/***************************************************************************
 * FUNCTIONS:
 *      CT_Register()
 *
 * DESCRIPTION:
 *      This function creates the named pipe to wait for utility frontend
 *      to connect and identifies the tool name.
 *
 * ARGUMENTS:
 *      Name    -   the name of the conversion tool
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_Register(
                    const CHAR*   Name
                    );

/***************************************************************************
 * FUNCTIONS:
 *      CT_SetParameter()
 *
 * DESCRIPTION:
 *      This function sets the value of a named parameter.
 *
 * ARGUMENTS:
 *      ParamName   -   name of the parameter being set
 *      ParamPtr    -   a pointer of the data to be stored under the ParamName
 *      Length      -   length in bytes of the data to be stored (Max 4K bytes)
 *      Persistence -   if TRUE this parameter is flushed to persistent storage 
 *                      before this call returns
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_SetParameter(
                        const CHAR*       ParamName,
                        const PVOID       ParamPtr,
                        const UINT_32E    Length,
                        const BOOL        Persistence
                        );


/***************************************************************************
 * FUNCTIONS:
 *      CT_GetParameter()
 *
 * DESCRIPTION:
 *      This function gets the value of a named parameter.
 *
 * ARGUMENTS:
 *      ParamName   -   name of the parameter to get
 *      ParamPtr    -   a pointer of the data to be stored under the ParamName
 *      LengthPtr   -   a pointer to the length in bytes of the buffer pointed to 
 *                      by ParamPtr
 *      Persistence -   if TRUE this parameter is flushed to persistent storage 
 *                      before this call returns
 *
 * RETURN:
 *      CT_STATUS
 *
 * NOTE:
 *      LengthPtr   -   returns the updated length of the parameter data once the
 *                      the function returns value OK or INSUFFICIENT_MEMORY.
 *      ParamPtr    -   returns the data only when the function returns value OK.
 *      Persistence -   returns the persistence charactor once the function returns
 *                      value OK or INSUFFICIENT_MEMORY.
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_GetParameter(
                        const CHAR*     ParamName,
                        PVOID           ParamPtr,
                        UINT_32E        *LengthPtr,
                        BOOL            *PersistencePtr
                        );


/***************************************************************************
 * FUNCTIONS:
 *      CT_DeleteParameters()
 *
 * DESCRIPTION:
 *      This function deletes all parameters matching the given pattern.
 *
 * ARGUMENTS:
 *      Pattern    -    a Perl style regular expression indicating the parameters 
 *                      to delete
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_DeleteParameters(
                        const CHAR*   Pattern
                        );


/***************************************************************************
 * FUNCTIONS:
 *      CT_DumpParameters()
 *
 * DESCRIPTION:
 *      This function dumps parameter names and values to ktrace for debugging.
 *
 * ARGUMENTS:
 *      Pattern             -   this pattern specifies which parameters to dump. 
 *                              If it is NULL, then all parameters will be dumped.
 *      MessageDestination  -   this argument indicates whether the parameters are 
 *                              written to ktrace and or the user. Currently, 
 *                              only CT_MESSAGE_TO_KTRACE is supported.
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_DumpParameters(
                    const CHAR*                           Pattern,
                    const CT_MESSAGE_DESTINATION_TYPE     MessageDestination
                    );


/***************************************************************************
 * FUNCTIONS:
 *      CT_Message()
 *
 * DESCRIPTION:
 *      This function displays messages to the ktrace and the user, 
 *      prefixing them with the Name given in CT_Register.
 *
 * ARGUMENTS:
 *      String              -   the string will be displayed
 *      MessageDestination  -   a bit mask specifying the message destination.  
 *                              This field consists of one or more of the values 
 *                              from CT_MESSAGE_DESTINATION_TYPE.
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_Message(
                const CHAR*                           String,
                const CT_MESSAGE_DESTINATION_TYPE     MessageDestination
                );


/***************************************************************************
 * FUNCTIONS:
 *      CT_ReadRegion()
 *
 * DESCRIPTION:
 *      This function copies data from a region on disk to a file in the RAM disk
 *
 * ARGUMENTS:
 *      DestinationFilename -   the relative pathname to a file in the RAM Disk 
 *                              where the disk region data will be written
 *      SourceRegion        -   the disk region to be read
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_ReadRegion(
                const CHAR*                           DestinationFilename,
                const CT_REGION_DESCRIPTOR_TYPE       SourceRegion
                );


/***************************************************************************
 * FUNCTIONS:
 *      CT_WriteRegion()
 *
 * DESCRIPTION:
 *      This function copies data from a file in the RAM disk to a region on disk
 *
 * ARGUMENTS:
 *      DestinationRegion   -   the disk region to be written
 *      SourceFilename      -   the relative pathname to a file in the RAM Disk 
 *                              where the disk region data will be written
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_WriteRegion(
                const CT_REGION_DESCRIPTOR_TYPE       DestinationRegion,
                const CHAR*                           SourceFilename
                );


/***************************************************************************
 * FUNCTIONS:
 *      CT_CopyRegion()
 *
 * DESCRIPTION:
 *      This function copies a region on disk to another region on the same or 
 *      a different disk
 *
 * ARGUMENTS:
 *      DestinationRegion   -   the region on disk to be copied to
 *      SourceRegion        -   the region on disk to be copied from
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_CopyRegion(
            const CT_REGION_DESCRIPTOR_TYPE       DestinationRegion, 
            const CT_REGION_DESCRIPTOR_TYPE       SourceRegion
            );

/***************************************************************************
 * FUNCTIONS:
 *      CT_ZeroRegion()
 *
 * DESCRIPTION:
 *      This function zero a disk region
 *
 * ARGUMENTS:
 *      Region   -   the region to be zeroed
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_ZeroRegion(
            const CT_REGION_DESCRIPTOR_TYPE       Region
            );


/***************************************************************************
 * FUNCTIONS:
 *      CT_GetIRFile()
 *
 * DESCRIPTION:
 *      This function gets all data from an Image Repository file and writes 
 *      it to a RAM disk file
 *
 * ARGUMENTS:
 *      Destination         -   the relative path name of the RAM Disk file 
 *                              that will be the destination for the data
 *      SourceIRFilename    -   the source file name in the Image Repository
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_GetIRFile(
                    const CHAR*       Destination,
                    const CHAR*       SourceIRFilename
                    );


/***************************************************************************
 * FUNCTIONS:
 *      CT_PutIRFile()
 *
 * DESCRIPTION:
 *      This function reads all data from a RAM disk and puts it into an Image 
 *      Repository file
 *
 * ARGUMENTS:
 *      DestinationIRFilename   -   the name of the destination file in the 
 *                                  Image Repository
 *      Source                  -   the relative path name of the RAM Disk file 
 *                                  that will be the source for the data
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_PutIRFile(
                const CHAR*       DestinationIRFilename,
                const CHAR*       Source
                );


/***************************************************************************
 * FUNCTIONS:
 *      CT_DeleteIRFile()
 *
 * DESCRIPTION:
 *      This function deletes a file from the Image Repository
 *
 * ARGUMENTS:
 *      IRFilename  -   the name of the destination file in the Image Repository
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_DeleteIRFile(
                    const CHAR*   IRFilename
                    );


/***************************************************************************
 * FUNCTIONS:
 *      CT_FlushKtraceToIR()
 *
 * DESCRIPTION:
 *      This function flushes the in-memory ktrace buffer to the Image Repository
 *
 * ARGUMENTS:
 *      None
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_FlushKtraceToIR(void);



/***************************************************************************
 * FUNCTIONS:
 *      CT_GetStatusString()
 *
 * DESCRIPTION:
 *      It returns a readable string of the passed error code
 *
 * ARGUMENTS:
 *      Status   -   the status value in question
 *
 * RETURN:
 *      Status string
 *
 ***************************************************************************/
CHAR* CT_GetStatusString(
            const CT_STATUS_TYPE    Status
            );


/***************************************************************************
 * FUNCTIONS:
 *      CT_Exit()
 *
 * DESCRIPTION:
 *      This function reports exit status and then closes the server connection
 *
 * ARGUMENTS:
 *      Value   -   the exit status to report
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_Exit(
            const UINT_32E    Value
            );




/************************ DUAL-MODE INTERFACE FUNCTIONS ********************
 *
 * This series of functions provides access into other drivers in the stack
 * (SPECL, SPID, etc.) to provide centralized access to functions that these
 * drivers provide.
 *
 * Each of these functions has a Simulation Parameter associated with it.
 * If the required drive isn't available (such as in the simulation environment)
 * the required data will be retrieved from the parameter, if it exists.
 * If the parameter doesn't exist, an error will be returned.
 *
 ***************************************************************************/


/***************************************************************************
 * FUNCTIONS:
 *      CT_DM_GetIOModuleStatusThisBlade()
 *
 * DESCRIPTION:
 *      This function gets the current IO Module Status.
 *
 * ARGUMENTS:
 *      IoStatusThisBladePtr   -    This structure will contain the current IO
 *      Module status as returned by speclGetIOModuleStatus as it applies to the
 *      current SP.
 *
 * DRIVER CALLED:
 *	SPECL
 *
 * SIMULATION PARAMETER:
 *	CTDMIoStatusThisBlade_ParameterString   "CT_DM_IoStatusThisBlade"
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_DM_GetIOModuleStatusThisBlade (
		SPECL_IO_STATUS_PER_BLADE *IoStatusThisBladePtr );



/***************************************************************************
 * FUNCTIONS:
 *      CT_DM_GetIOModuleSummary()
 *
 * DESCRIPTION:
 *      This function gets the current IO Module Status for both SPs.
 *
 * ARGUMENTS:
 *      IoSummaryPtr   -    This structure will contain the IO Module
 *      summary status as returned by speclGetIOModuleStatus.
 *
 * DRIVER CALLED:
 *	SPECL
 *
 * SIMULATION PARAMETER:
 *	CTDMIoSummary_ParameterString		"CT_DM_IoSummary"
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_DM_GetIOModuleSummary (
		SPECL_IO_SUMMARY *IoSummaryPtr );


/***************************************************************************
 * FUNCTIONS:
 *      CT_DM_GetSpId()
 *
 * DESCRIPTION:
 *      This function gets the SPID information.
 *
 * ARGUMENTS:
 *      SpIdPtr   -    This structure will contain a SP_ID value as returned
 *      by spidGetSpId.
 *
 * DRIVER CALLED:
 *	SPID
 *
 * SIMULATION PARAMETER:
 *	CTDMSpId_ParameterString		"CT_DM_SpId"
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_DM_GetSpId (
		SP_ID *SpIdPtr );

/***************************************************************************
 * FUNCTIONS:
 *      CT_DM_GetHwType()
 *
 * DESCRIPTION:
 *      This function gets the Hardware Type information for this SP.
 *
 * ARGUMENTS:
 *      HwTypePtr   -    This structure will contain a SPID_HW_TYPE value as
 *      returned by spidGetHwTypeEx.
 *
 * DRIVER CALLED:
 *	SPID
 *
 * SIMULATION PARAMETER:
 *	CTDMHwType_ParameterString		"CT_DM_HwType"
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_DM_GetHwType (
		SPID_HW_TYPE *HwTypePtr );


/***************************************************************************
 * FUNCTIONS:
 *      CT_DM_GetPeerHwType()
 *
 * DESCRIPTION:
 *      This function gets the Hardware Type information for the peer SP.
 *
 * ARGUMENTS:
 *      HwTypePtr   -    This structure will contain a SPID_HW_TYPE value as
 *      returned by speclGetMiscStatus.
 *
 * DRIVER CALLED:
 *	SPECL
 *
 * SIMULATION PARAMETER:
 *	CTDMHwType_ParameterString		"CT_DM_PeerHwType"
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_DM_GetPeerHwType (
		SPID_HW_TYPE *HwTypePtr );


/***************************************************************************
 * FUNCTIONS:
 *      CT_DM_GetHwName()
 *
 * DESCRIPTION:
 *      This function gets the Hardware Name information.
 *
 * ARGUMENTS:
 *      HwNamePtr     -  A buffer for storing the HwName value string as
 *      returned by spidGetHwName.
 *      HwNameLength  -  The size of the HwNamePtr buffer. 
 *
 * DRIVER CALLED:
 *	SPID
 *
 * SIMULATION PARAMETER:
 *	CTDMGetHwName_ParameterString		"CT_DM_HwName"
 *
 * RETURN:
 *      CT_STATUS
 *
 ***************************************************************************/
CT_STATUS_TYPE CT_DM_GetHwName (
        CHAR *HwNamePtr,
        UINT_32E HwNameLength );

#endif
