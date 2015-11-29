#ifndef FBE_FC_PORT_H
#define FBE_FC_PORT_H

#include "fbe_base_port.h"

/*  control codes */
typedef enum fbe_fc_port_control_code_e {
	FBE_FC_PORT_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_FC_PORT),
	FBE_FC_PORT_CONTROL_CODE_LAST
}fbe_fc_port_control_code_t;

#endif /* FBE_FC_PORT_H */
