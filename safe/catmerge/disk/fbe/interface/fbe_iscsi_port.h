#ifndef FBE_FC_ISCSI_H
#define FBE_FC_ISCSI_H

#include "fbe_base_port.h"

/*  control codes */
typedef enum fbe_iscsi_port_control_code_e {
	FBE_ISCSI_PORT_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_ISCSI_PORT),
	FBE_ISCSI_PORT_CONTROL_CODE_LAST
}fbe_iscsi_port_control_code_t;

#endif /* FBE_FC_ISCSI_H */
