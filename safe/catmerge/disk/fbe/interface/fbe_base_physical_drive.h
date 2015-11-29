#ifndef FBE_BASE_PHYSICAL_DRIVE_H
#define FBE_BASE_PHYSICAL_DRIVE_H

#include "fbe/fbe_physical_drive.h"
#include "fbe_base_object.h"
#include "fbe_block_transport.h"


/**************************************************************************
*  
*  *****************************************************************
*  *** 2012_03_30 SATA DRIVES ARE FAULTED IF PLACED IN THE ARRAY ***
*  *****************************************************************
*      SATA drive support was added as black content to R30
*      but SATA drives were never supported for performance reasons.
*      The code has not been removed in the event that we wish to
*      re-address the use of SATA drives at some point in the future.
*
***************************************************************************/
//#define BLOCKSIZE_512_SUPPORTED 1

typedef enum fbe_base_physical_drive_control_code_e {
	FBE_BASE_PHYSICAL_DRIVE_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE),

	FBE_BASE_PHYSICAL_DRIVE_CONTROL_CODE_LAST
}fbe_base_physical_drive_control_code_t;



#endif /* FBE_BASE_PHYSICAL_DRIVE_H */
