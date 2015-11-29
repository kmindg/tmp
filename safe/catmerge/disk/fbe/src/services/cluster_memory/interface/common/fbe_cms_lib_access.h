#ifndef FBE_CMS_LIB_ACCESS_H
#define FBE_CMS_LIB_ACCESS_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cms_lib_access.h
 ***************************************************************************
 *
 * @brief
 *  This file contains function calls to be called by the library to use the service.
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_service.h"
#include "fbe_cms_private.h"

fbe_status_t fbe_cms_get_ptr(void);

#endif /* end FBE_CMS_LIB_ACCESS_H */
