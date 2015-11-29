/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               fbe_test_public_Call_Driver.h
 ***************************************************************************
 *
 * DESCRIPTION: 
 *
 * FUNCTIONS: Prototypes for entrypoint fbe_test_Call_Driver 
 *              available in the titan_fbe_test.dll
 *              This file is used in multiple feature branches and needs to 
 *              declare the same entrypoints if code from those branches are
 *              to execute properly together.
 *
 *
 * NOTES:
 *
 * HISTORY:
 *    07/13/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef FBE_TEST_CALL_DRIVER_H
#define FBE_TEST_CALL_DRIVER_H

# include "generic_types.h"
# include "k10ntddk.h"

#if defined(TITAN_FBE_TEST)
#define TITANPUBLIC CSX_MOD_EXPORT 
#else
#define TITANPUBLIC CSX_MOD_IMPORT 
#endif


#ifdef __cplusplus
extern "C" {
#endif
 


/*
 * \fn void fbe_test_Call_driver()
 * \param PIRP      - ptr to IRP to be forwarded into the FBE test environment
 * \details
 *
 *   Dispatch IRP to the FBE test environment
 *
 */
TITANPUBLIC EMCPAL_STATUS fbe_test_Call_Driver(PEMCPAL_IRP Irp);

#ifdef __cplusplus
};
#endif


#endif // FBE_TEST_CALL_DRIVER_H

