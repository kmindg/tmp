#ifndef FBE_CMS_ENVIRONMENT_H
#define FBE_CMS_ENVIRONMENT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cms_environment.h
 ***************************************************************************
 *
 * @brief
 *  This file contains environment related interfaces.
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"

/*functions*/
fbe_status_t fbe_cms_environment_init(void);
fbe_status_t fbe_cms_environment_destroy(void);


#endif /*FBE_CMS_ENVIRONMENT_H*/
