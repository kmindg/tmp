#ifndef FBE_SIMULATOR_H
#define FBE_SIMULATOR_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_port.h"
/* #include "fbe/fbe_lcc.h" */
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_shim_flare_interface.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_object.h"
/* #include "fbe/fbe_resume_prom_info.h" */

typedef struct simulator_map_object_e{
	fbe_lifecycle_state_t	    mgmt_state;
}simulator_map_object_t;

fbe_status_t fbe_simulator_init(void);
fbe_status_t fbe_simulator_destroy(void);

#endif /* FBE_SIMULATOR_H */
