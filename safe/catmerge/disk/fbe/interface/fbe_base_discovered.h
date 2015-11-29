#ifndef FBE_BASE_DISCOVERED_H
#define FBE_BASE_DISCOVERED_H

#include "fbe/fbe_object.h"
#include "fbe/fbe_class.h"
#include "fbe_discovery_transport.h"

/* Control codes */
typedef enum fbe_base_discovered_control_code_e {
	FBE_BASE_DISCOVERED_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_BASE_DISCOVERED),

	FBE_BASE_DISCOVERED_CONTROL_CODE_LAST
}fbe_base_discovered_control_code_t;




#endif /* FBE_BASE_DISCOVERED_H */