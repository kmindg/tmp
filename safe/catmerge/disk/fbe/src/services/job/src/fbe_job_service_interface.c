/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_job_service_interface.c
 ***************************************************************************
 *
 * @brief
 *  This modules defines interface functions, registration etc. for the Job
 *  Service When new operation types are added, the consumer needs only to
 *  modify this file adding new operation types.
 *  
 * @version
 *  01/05/2010 - Created. Jesus Medina
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_service.h"
#include "fbe_job_service_private.h"
#include "fbe_job_service.h"
#include "fbe_spare.h"
#include "fbe_job_service_operations.h"


/*!-----------------------------------------------------------------------------
 * TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS:
 */

/*!-----------------------------------------------------------------------------
 * FORWARD DECLARATIONS:
 */

/*!------------------------------------------------------------------------------
 * Other registrations
 */

