#ifndef FBE_SAS_DRIVE_H
#define FBE_SAS_DRIVE_H

#include "fbe/fbe_winddk.h"
#include "fbe_base_lcc.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_transport.h"

#include "fbe_base_drive.h"

/* MGMT entry control codes */
typedef enum fbe_sas_drive_control_code_e {
	FBE_SAS_DRIVE_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_SAS_DRIVE),

	FBE_SAS_DRIVE_CONTROL_CODE_LAST
}fbe_sas_drive_control_code_t;

#endif /* FBE_SAS_DRIVE_H */
