#ifndef FBE_CPD_SHIM_PRIVATE_H
#define FBE_CPD_SHIM_PRIVATE_H

typedef enum cpd_shim_port_state_e {
	CPD_SHIM_PORT_STATE_INVALID,
	CPD_SHIM_PORT_STATE_UNINITIALIZED,
	CPD_SHIM_PORT_STATE_INITIALIZED,

	CPD_SHIM_PORT_STATE_LAST
}cpd_shim_port_state_t;

#endif /*FBE_CPD_SHIM_PRIVATE_H */