#ifndef FBE_FIBRE_LCC_H
#define FBE_FIBRE_LCC_H

#include "fbe/fbe_winddk.h"
#include "fbe_base_lcc.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_transport.h"

/* MGMT entry control codes */
typedef enum fbe_fibre_lcc_control_code_e {
	FBE_FIBRE_LCC_CONTROL_CODE_INVALID = (FBE_BASE_LCC_CONTROL_CODE_LAST & FBE_CONTROL_CODE_CLASS_MASK) | (FBE_CLASS_ID_FIBRE_LCC << FBE_CONTROL_CODE_CLASS_SHIFT),

	FBE_FIBRE_LCC_CONTROL_CODE_UPDATE_ENCLOSURE_MAP,
	FBE_FIBRE_LCC_CONTROL_CODE_ENABLE_PRIMARY,
	FBE_FIBRE_LCC_CONTROL_CODE_READ,
	FBE_FIBRE_LCC_CONTROL_CODE_SEQ_DIPLEX_COMMAND,

	FBE_FIBRE_LCC_CONTROL_CODE_LAST
}fbe_fibre_lcc_control_code_t;

#endif /* FBE_FIBRE_LCC_H */
