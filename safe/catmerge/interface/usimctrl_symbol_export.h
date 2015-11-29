/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **************************************************************************/

/***************************************************************************
 *                                usimctrl_symbol_export.h
 ***************************************************************************
 *
 * DESCRIPTION: Macro definitions needs to export symbols into uSimCtrl
 *
 * HISTORY:
 *    02/26/2009   Igor Bobrovnikov  Initial version
 *    03/30/2009   Alex Alanov       Macros revised for automatic codegeneration
 *
 **************************************************************************/

#if !defined(_USIMCTRL_SYMBOL_EXPORT_H_)
#define _USIMCTRL_SYMBOL_EXPORT_H_

/***********************************
 *       INCLUDES                  *
 ***********************************/

#include "ipc_transport_base.h"

/***********************************
 *       TYPE DEFINITIONS          *
 ***********************************/

/* Generic function for processing calls from transport */
typedef ipc_transport_status_t (class_method_func_t)(ipc_transport_base_t * transport);

/***********************************
 *       FUNCTIONS                 *
 ***********************************/

void usimctrl_export_register_entry(const char * class_id, const char * method_id, class_method_func_t * method_ptr);

/***********************************
 *       MACRO DEFINITIONS         *
 ***********************************/

/** \def USIMCTRL_EXPORT_BEGIN(module)
 *  \param module - specifies the Python class id will be generated
 *  \details Functions will use module as a placeholder class where they will be placed
 */
#define USIMCTRL_EXPORT_BEGIN(module) const char * _usimctrl_export_module_##module = __FILE__; \
    void _usimctrl_export_moduletest_##module(void) { 

/** \def USIMCTRL_EXPORT_END()
 *  \details Specifies the Python class definition complete
 */
#define USIMCTRL_EXPORT_END()  return; }

/** \def USIMCTRL_EXPORT_ENUMERATION(enumeration_type)
 *  \param enumeration_type - enumeration type id
 *  \details Declares the enumeration type will be exported
 */
#define USIMCTRL_EXPORT_ENUMERATION(enumeration_type) volatile static const enumeration_type *  CSX_MAYBE_UNUSED _usimctrl_export_enum_##enumeration_type = (void *)0; 

/** \def USIMCTRL_EXPORT_STRUCTURE(structure_type)
 *  \param structure_type - structure type id
 *  \details Declares the structure type will be exported
 */
#define USIMCTRL_EXPORT_STRUCTURE(structure_type) volatile static const structure_type *  CSX_MAYBE_UNUSED _usimctrl_export_struct_##structure_type = (void *)0;

/** \def USIMCTRL_EXPORT_FUNCTION(function)
 *  \param function - function id
 *  \details Declares the function will be exported
 */
#define USIMCTRL_EXPORT_FUNCTION(function)     volatile static unsigned CSX_MAYBE_UNUSED _usimctrl_export_functdef_##function = __LINE__; \
    volatile static const void * CSX_MAYBE_UNUSED _usimctrl_export_function_##function = &function;

/** \def USIMCTRL_EXPORT_FUNCTION_DBA(function)
 *  \param function - function id
 *  \details Declares the DBA function will be exported
 */
#define USIMCTRL_EXPORT_FUNCTION_DBA(function) volatile static unsigned _usimctrl_export_functdef_##function = __LINE__; \
    volatile static const void * _usimctrl_export_function_##function = &function;

/** \def USIMCTRL_EXPORT_FUNCTION_DBA_MACRO(function)
 *  \param function - function id
 *  \details Declares the DBA macro stub function will be exported
 */
#define USIMCTRL_EXPORT_FUNCTION_DBA_MACRO(function) volatile static unsigned _usimctrl_export_functdef_##function = __LINE__; \
    volatile static const void * _usimctrl_export_function_EXPORTEDMACRO_##function = &EXPORTEDMACRO_##function;

/** \def USIMCTRL_EXPORT_DEFINE(define)
 *  \param define - define id
 *  \details Provides reguistration of function method_id in class_id
 */
#define USIMCTRL_EXPORT_DEFINE(def) volatile static __int64 CSX_MAYBE_UNUSED _usimctrl_export_define_##def = (__int64)(def);

/** \def USIMCTRL_EXPORT_REGISTRY_ENTRY(class_id, method_id)
 *  \param class_id - class id
 *  \param method_id - method id
 *  \details Provides reguistration of function method_id in class_id
 */
#define USIMCTRL_EXPORT_REGISTRY_ENTRY(class_id, method_id) usimctrl_export_register_entry(#class_id, #method_id, &class_id##__##method_id);

/** \def USIMCTRL_EXPORT_REGISTRY_ENTRY_MACRO(class_id, method_id)
 *  \param class_id - class id
 *  \param method_id - method id
 *  \details Provides reguistration of function method_id in class_id
 */
#define USIMCTRL_EXPORT_REGISTRY_ENTRY_MACRO(class_id, method_id) usimctrl_export_register_entry(#class_id, #method_id, &class_id##__EXPORTEDMACRO_##method_id);

/** \def USIMCTRL_EXPORT_REGISTER(module)
 *  \param medule - module id
 *  \details Provides runtime reguistration of module
 */
#define USIMCTRL_EXPORT_REGISTER(module) \
    { \
        extern const char * _usimctrl_export_module_##module; \
        void * dummy_##module = (void *)_usimctrl_export_module_##module ; \
    } \
    { \
        void usimctrl_export_register_##module(void); \
        usimctrl_export_register_##module(); \
    } \
    { \
        extern void _usimctrl_export_moduletest_##module(void); \
        _usimctrl_export_moduletest_##module(); \
    }

#endif /* !defined(_USIMCTRL_SYMBOL_EXPORT_H_) */
