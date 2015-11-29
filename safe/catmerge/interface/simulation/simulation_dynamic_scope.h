/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               simulation_dynamic_scope.h
 ***************************************************************************
 *
 * DESCRIPTION:  Controls for importing/exporting symbols from/to 
 *               simulation dynamic link libraries
 *
 * NOTES:
 *               The build process for a dynamic link library needs to 
 *               define EXPORT_DYNAMIC_SYMBOLS.
 *
 *               When EXPORT_DYNAMIC_SYMBOLS is defined,  SYMBOL_SCOPE is 
 *               configured to functions to be exported, 
 *               otherwise configure functions to be imported.
 *               
 *
 * HISTORY:
 *    10/15/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef  _SIM_DYNAMIC_SCOPE_
#define _SIM_DYNAMIC_SCOPE_

#include "csx_ext.h"


/*
 * SYMBOL_SCOPE can be extern, CSX_MODE_IMPORT, CSX_MOD_EXPORT
 */
#if defined(SIMMODE_ENV)
// only export/import symbols when building TARGETMODE simulation
#if defined(EXPORT_SIMULATION_DRIVER_SYMBOLS)
#define SYMBOL_SCOPE CSX_MOD_EXPORT
#else
#define SYMBOL_SCOPE CSX_MOD_IMPORT
#endif
#else 
// kernel drivers should not import/export symbols
#define SYMBOL_SCOPE  extern
#endif

/*
 * Symbols defined in source files can not be MOD_IMPORT
 * This define for use when declaring functions in source files 
 */
#if defined(SIMMODE_ENV) && defined(EXPORT_SIMULATION_DRIVER_SYMBOLS)
#define SOURCE_SYMBOL_SCOPE  CSX_MOD_EXPORT
#else
#define SOURCE_SYMBOL_SCOPE
#endif

#endif // _SIM_DYNAMIC_SCOPE_
