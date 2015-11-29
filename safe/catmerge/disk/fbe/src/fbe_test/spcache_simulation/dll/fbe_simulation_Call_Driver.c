/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               fbe_simulation_public_Call_Driver.c
 ***************************************************************************
 *
 * DESCRIPTION: 
 *
 * FUNCTIONS: Implementation of entrypoint fbe_simulation_Call_Driver 
 *              available in titan_fbe_test.dll
 *
 *
 * NOTES:
 *
 * HISTORY:
 *    07/13/2009  Martin Buckley Initial Version
 *
 **************************************************************************/

# include "simulation/fbe_simulation_Call_Driver.h"


/*
 * \fn void fbe_test_Call_driver()
 * \param PIRP      - ptr to IRP to be forwarded into the FBE test environment
 * \details
 *
 *   Dispatch IRP to the FBE test environment
 *
 */
TITANPUBLIC EMCPAL_STATUS fbe_test_Call_Driver(PEMCPAL_IRP Irp) {
    // TODO: link in fbe/src/packages/physical/kernel code Irp processing
    return EMCPAL_STATUS_SUCCESS;
}

