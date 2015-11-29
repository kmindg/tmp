/***************************************************************************

 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               BvdSimulationData.h
 ***************************************************************************
 *
 * DESCRIPTION: Declarations of convenience functions used to manage abstract
 *                DeviceObjects and IRPs for the BVD simulator
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    06/02/2009  Martin Buckley Initial Version
 *
 **************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

# include "generic_types.h"
 
void SimulationBreakpoint();

PVOID allocate_irp_for_simulation();
PVOID allocate_device_object_for_simulation();
PVOID allocate_driver_object_for_simulation();

void free_irp_for_simulation(PVOID ptr);
void free_device_object_for_simulation(PVOID ptr);
void free_driver_object_for_simulation(PVOID ptr);

#ifdef __cplusplus
}
#endif

