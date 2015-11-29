#ifndef FBE_FIBRE_DRIVE_H
#define FBE_FIBRE_DRIVE_H

#include "fbe/fbe_winddk.h"
#include "fbe_base_lcc.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_transport.h"

#include "fbe_base_drive.h"

/* MGMT entry control codes */
typedef enum fbe_fibre_drive_control_code_e {
	FBE_FIBRE_DRIVE_CONTROL_CODE_INVALID = (FBE_PHYSICAL_DRIVE_CONTROL_CODE_LAST & FBE_CONTROL_CODE_CLASS_MASK) | (FBE_CLASS_ID_FIBRE_DRIVE << FBE_CONTROL_CODE_CLASS_SHIFT),

	FBE_FIBRE_DRIVE_CONTROL_CODE_LAST
}fbe_fibre_drive_control_code_t;

#endif /* FBE_FIBRE_DRIVE_H */
