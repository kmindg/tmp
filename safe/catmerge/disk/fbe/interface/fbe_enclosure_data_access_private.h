#ifndef FBE_ENCL_DATA_ACCESS_PRIVATE_H
#define FBE_ENCL_DATA_ACCESS_PRIVATE_H

/***************************************************************************
 * Copyright (C) EMC Corporation  2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/***************************************************************************
 *  fbe_enclosure_data_access_private.h
 ***************************************************************************
 *
 * DESCRIPTION:
 *      This module is a header file for the library that provides 
 *      access functions (get/set) for Enclosure Object data.  This 
 *      header file is for components withing the Physical Package that 
 *      do not know anything about the various types of Enclosure objects.
 *
 *  FUNCTIONS:
 *
 *  LOCAL FUNCTIONS:
 *
 *  NOTES:
 *
 *  HISTORY:
 *      09-Oct-08   Joe Perry - Created
 *
 *
 **************************************************************************/

#include "csx_ext.h"
#include "fbe/fbe_types.h"


//***********************  Inline Functions ********************************
#if FALSE
static __forceinline void edal_validate_enclosureComponentBlock(void *enclosureComponentBlockPtr)
{
    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
}
#endif


//***********************  Function Prototypes *****************************

//***********************  Get Functions  **********************************
fbe_edal_status_t
fbe_edal_checkForWriteData(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                           fbe_bool_t *writeDataAvailable);

fbe_edal_status_t
fbe_edal_getSpecificComponent(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u32_t index,
                              void **encl_component);

fbe_edal_status_t
fbe_edal_incrementGenrationCount(fbe_edal_block_handle_t enclosureComponentBlockPtr);

//***********************  Set Functions  **********************************

//***********************  Print Functions  **********************************
void enclosure_access_log_info(const char * fmt, ...) __attribute__((format(__printf_func__,1,2)));
void enclosure_access_log_error(const char * fmt, ...) __attribute__((format(__printf_func__,1,2)));
void enclosure_access_log_debug(const char * fmt, ...) __attribute__((format(__printf_func__,1,2)));
void enclosure_access_log_warning(const char * fmt, ...) __attribute__((format(__printf_func__,1,2)));
void enclosure_access_log_critical_error(const char * fmt, ...) __attribute__((format(__printf_func__,1,2)));


#endif  // ifndef FBE_ENCL_DATA_ACCESS_PRIVATE_H
