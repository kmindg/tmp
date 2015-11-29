#ifndef FBE_FIBRE_PORT_H
#define FBE_FIBRE_PORT_H

#include "fbe/fbe_winddk.h"
#include "fbe_base_port.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_transport.h"
#include "fbe_diplex.h"

/* MGMT entry control codes */
typedef enum fbe_fibre_port_control_code_e {
	FBE_FIBRE_PORT_CONTROL_CODE_INVALID = (FBE_BASE_PORT_CONTROL_CODE_LAST & FBE_CONTROL_CODE_CLASS_MASK) | (FBE_CLASS_ID_FIBRE_PORT << FBE_CONTROL_CODE_CLASS_SHIFT),

	FBE_FIBRE_PORT_CONTROL_CODE_DIPLEX_COMMAND,
	FBE_FIBRE_PORT_CONTROL_CODE_GET_DIPLEX_LCC_INFO,

	FBE_FIBRE_PORT_CONTROL_CODE_LAST
}fbe_fibre_port_control_code_t;

/* FBE_FIBRE_PORT_CONTROL_CODE_DIPLEX_COMMAND */
typedef struct fbe_fibre_port_mgmt_diplex_command_s {
	fbe_diplex_lcc_info_t diplex_lcc_info; /* Result of resent poll command */
	fbe_diplex_command_t  diplex_command;
}fbe_fibre_port_mgmt_diplex_command_t;

/* FBE_FIBRE_PORT_CONTROL_CODE_GET_DIPLEX_LCC_INFO */
typedef struct fbe_fibre_port_mgmt_get_diplex_lcc_info_s {
	fbe_diplex_address_t	diplex_address;
	fbe_diplex_lcc_info_t	diplex_lcc_info; /* Result of resent poll command */
}fbe_fibre_port_mgmt_get_diplex_lcc_info_t;

#endif /* FBE_FIBRE_PORT_H */
