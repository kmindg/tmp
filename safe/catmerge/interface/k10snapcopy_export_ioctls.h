//++
//
//  Module:
//
//      k10snapcopy_export_ioctls.h
//
//  Description:
//
//      This file contains the IOCTL and structure to handle conversions
//      from Phase 2 to Phase 3 customers who have existing SCLUNs that 
//      need to be put with their associated Source Luns in a gang to
//      support Gang Trespass.
//
//  Notes:
//
//      This is only to be used in Phase 3!
//
//  History:
//
//      23-May-01   PTM;    Created file.
//
//--

#if !defined (K10SNAPCOPY_EXPORT_IOCTLS_H)
#define K10SNAPCOPY_EXPORT_IOCTLS_H

#if !defined (K10_DEFS_H)
#include "k10defs.h"
#endif


#define IOCTL_SC_RETURN_GANG_ID         \
    EMCPAL_IOCTL_CTL_CODE( FILE_DEVICE_DISK, 0xAAAA, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS )


//++
//
//  Name:
//
//      SNAPCOPY_RETURN_GANG_ID, *PSNAPCOPY_RETURN_GANG_ID
//
//  Description:
//
//      Structure definition for input buffer used by IOCTL_SC_RETURN_GANG_ID.
//
//--

typedef struct _SNAPCOPY_RETURN_GANG_ID
{
    //
    //  World Wide Name of the Source Lun that this lun is associated with. 
    //
    //  If this IOCTL is sent to a Source Lun, then the WorldWideName will be
    //  its own WWN. If sent to a 
    //

    K10_WWID                                                    WorldWideName;


} SNAPCOPY_RETURN_GANG_ID, *PSNAPCOPY_RETURN_GANG_ID;
 

#endif /* !defined (K10SNAPCOPY_EXPORT_IOCTLS_H) */

