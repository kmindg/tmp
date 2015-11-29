/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               fbe_test_pubic_api.h
 ***************************************************************************
 *
 * DESCRIPTION: 
 *
 * FUNCTIONS: Prototypes for entrypoints available in the titan_fbe_test.dll
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

#ifndef FBE_TEST_PUBLIC_API_H
#define FBE_TEST_PUBLIC_API_H

# include "generic_types.h"
# include "csx_ext.h"

#if defined(TITAN_FBE_TEST)
#define TITANPUBLIC CSX_MOD_EXPORT 
#else
#define TITANPUBLIC CSX_MOD_IMPORT 
#endif

/*
 * used to identify SP to communicate  with
 */
typedef enum fbe_test_sp_type_e {
    fbe_test_SPA = 0,
    fbe_test_SPB = 1,
} fbe_test_sp_type_t;


/*
 * used to identify the hw configuration to be used during testing
 */
typedef enum fbe_test_sp_hw_config_e {
    hw_config_1enclosure_12drives,
    hw_config_2enclosures_24drives,
    hw_config_4enclosures_48drives,
} fbe_test_sp_hw_config_t;

/*
 * used to identify the logical configuration to be used during testing
 */
typedef enum fbe_test_logical_config_e {
    logical_config_1raidgroup_12drives,
    logical_config_4raidgroups_12drives,
    logical_config_4raidgroups_24drives,
    logical_config_8raidgroups_24drives,
    logical_config_16raidgroups_48drives,
} fbe_test_logical_config_t;


#ifdef __cplusplus
/*
 * all of these functions need standard C linkage.  
 */
extern "C" {
#endif

/*
 * \fn void fbe_test_environment_active()
 * \details
 *
 *   Returns TRUE if/when fbe_test_public_api will start and control
 *   the fbe_sp_test simulation processes
 *
 */
TITANPUBLIC BOOL fbe_test_environment_active();

/*
 * \fn void fbe_test_setup()
 * \param sp    - identifies the SP
 * \details
 *
 *   Setup routine launches the fbe_sp_sim process and establishes communication 
 *
 */
TITANPUBLIC void fbe_test_setup(fbe_test_sp_type_t type);

/*
 * \fn void fbe_test_cleanup()
 * \param sp    - identifies the SP
 * \details
 *
 *   cleanup routine shuts dow the fbe_sp_sim process
 *
 */
TITANPUBLIC void fbe_test_cleanup(fbe_test_sp_type_t type);


/*
 * \fn void fbe_test_initialize_hw_config()
 * \param sp       - identifies the SP
 * \param config   - identifies the hw config to configure
 * \details
 *
 *   configure the terminator with the specified hw config
 *
 */
TITANPUBLIC void fbe_test_initialize_hw_config(fbe_test_sp_type_t type,
                                                fbe_test_sp_hw_config_t config);

/*
 * \fn void fbe_test_initialize_logical_config()
 * \param config   - identifies the logical config to configure
 * \details
 *
 *   configure the SEP with the specified logical config
 *
 */
TITANPUBLIC void fbe_test_initialize_logical_config(fbe_test_logical_config_t config);

/*
 * \fn void fbe_test_create_lun()
 * \param rgId      - index of the raid group in which the LUN is created
 * \param lunId     - index the LUN to create
 * \param capacity  - the number of blocks capacity of the LUN
 * \details
 *
 *   configure LUN lunID in raid Group rgId with capacity
 *
 */
TITANPUBLIC void fbe_test_create_lun(UINT_32 rgId, UINT_32 lunId, UINT_64 capacity);

#ifdef __cplusplus
};
#endif

#endif // FBE_TEST_PUBLIC_API_H
