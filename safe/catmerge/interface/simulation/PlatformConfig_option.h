/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               PlatformConfig_option.h
 ***************************************************************************
 *
 * DESCRIPTION: Implementation of the Platform_Config_option class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    02/06/2014  caridm Initial Version
 *
 **************************************************************************/

#ifndef PLATFORMCONFIG_OPTION_H_
#define PLATFORMCONFIG_OPTION_H_

#include "csx_ext.h"

#if defined(SHMEM_DLL_EXPORT)
#define SHMEMDLLPUBLIC CSX_MOD_EXPORT
#else
#define SHMEMDLLPUBLIC CSX_MOD_IMPORT
#endif

#define SUPPORTED_CCR_PLATFORMS "Pick the Platform to use (PROMETHEUS, NOVA, NOVAS1, HYPER1, HYPER2, HYPER3, or DEFIANT). default DEFIANT (windows), NOVA (linux)"
#ifdef C4_INTEGRATED
#define SUPPORTED_CCR_PLATFORM_DEFAULT "NOVA"
#else //C4_INTEGRATED
#define SUPPORTED_CCR_PLATFORM_DEFAULT "DEFIANT"
#endif //C4_INTEGRATED

#if defined(__cplusplus) || defined(c_plusplus)
#include "simulation/Program_Option.h"

class Platform_Config_option: public String_option {
public:
    SHMEMDLLPUBLIC Platform_Config_option();
    SHMEMDLLPUBLIC ~Platform_Config_option();
    SHMEMDLLPUBLIC void set(char *v);
    SHMEMDLLPUBLIC static Platform_Config_option *getCliSingleton();
};

extern "C" {
CSX_MOD_EXPORT csx_pchar_t GetPlatformConfigOptionString(void);
};

#else //defined(__cplusplus) || defined(c_plusplus)

CSX_MOD_IMPORT csx_pchar_t GetPlatformConfigOptionString(void);

#endif //defined(__cplusplus) || defined(c_plusplus)


#endif /* PLATFORMCONFIG_OPTION_H_ */
