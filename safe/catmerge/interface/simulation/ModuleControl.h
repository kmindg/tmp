/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               ModuleControl.h
 ***************************************************************************
 *
 * DESCRIPTION:  Contain declarations used to introspect
 *               CLARiiON CSX Modules
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    11/14/2009  Martin Buckley Initial Version
 *
 **************************************************************************/

# include "generic_types.h"
# include "mut.h"

#define DISABLE_CPP_MEMORY_MANAGEMENT
# include "spid_types.h"
# include "BasicLib/ConfigDatabase.h"
# undef DISABLE_CPP_MEMORY_MANAGEMENT



typedef struct  mc_identity_s
{
    char    *name;
    char    *loadFlags;
}   mc_identity_t;

typedef struct mc_content_s
{
    BOOL    containsDriver;
    BOOL    containsMut;
} mc_content_t;

typedef struct mc_dependences_s
{
    char    **requiredModules;
    char    **requiredDevices;
    char    **exportedDevices;
} mc_dependencies_t;

typedef struct mc_driver_ep_s
{
    BOOL    (*DriverLoad)(DatabaseKey * rootKey, SP_ID spid, char* driverName);
    VOID    (*DriverUnload)();
} mc_driver_ep_t;

typedef struct mc_test_ep_s
{
    mut_testsuite_t *(*getTestSuite)();
} mc_test_ep_t;

typedef struct mc_details_s
{
    mc_identity_t       identity;
    mc_content_t        content;
    mc_dependencies_t   dependencies;
    mc_driver_ep_t      driver_routines;
    mc_test_ep_t        test_routines;
} mc_details_t, *p_mc_details_t;


typedef struct mc_public_details_s
{
    mc_identity_t       identity;
    mc_content_t        content;
    mc_dependencies_t   dependencies;
} mc_public_details_t, *p_mc_public_details_t;


#define MC_Identity(name, flags) {name, flags}
#define MC_Content(driver, mut)  {driver, mut}
#define MC_Dependencies(modules, devices, exports) {module, devices, exports}
#define MC_Driver(load, unload) {load, unload}
#define MC_Suite(suite)  {suite}

#define MC_MUT_CONTENT()    {FALSE, TRUE}
#define MC_DRIVER_CONTENT() {FALSE, TRUE}

#define MC_NO_DEPENDENCIES()    MC_Dependencies(NULL, NULL, NULL)
#define MC_NO_DRIVER()          MC_Driver{NULL, NULL}
#define MC_NO_SUITE()           MC_Suite(NULL)

#define MC_MUT_Identity(name)  {name, ""};

#define MC_SIMPLE_MUT_MODULE(Name, suiteRoutine)                        \
                                            {                           \
                                                MC_MUT_Identity(Name)   \
                                                MC_MUT_CONTENT()        \
                                                MC_NO_DEPENDENCIES()    \
                                                MC_NO_DRIVER()          \
                                                MC_Suite(suiteRoutine)  \
                                            };


#define MC_SIMPLE_DRIVER_MODULE(name, LoadFlags, load, unload)              \
                                            {                               \
                                                MC_Identy(name,LoadFlags)   \
                                                MC_DRIVER_CONTENT()         \
                                                MC_NO_DEPENDENCIES()        \
                                                MC_Driver(load, unload)     \
                                            };

