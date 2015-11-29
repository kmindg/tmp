/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cmi_service_panic.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the cmi module for giving panic permission.
 * 
 *  When one SP or the other decides to panic this is decided on the
 *  active SP.  The active SP then needs an interlocked way
 *  to get permission for and save the SP that will be panicking.
 * 
 *  Once the peer does panic and connection is lost, this information
 *  is then cleared.
 *
 * @version
 *   9/28/2012:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_service.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe_cmi_private.h"
#include "fbe_cmi.h"

/*!*******************************************************************
 * @enum fbe_cmi_panic_statet
 *********************************************************************
 * @brief
 *
 *********************************************************************/
typedef enum fbe_cmi_panic_statee
{
    FBE_CMI_PANIC_STATE_INVALID = 0,
    FBE_CMI_PANIC_STATE_THIS_SP,
    FBE_CMI_PANIC_STATE_PEER_SP,
    FBE_CMI_PANIC_STATE_LAST,
}
fbe_cmi_panic_state_t;

static fbe_spinlock_t fbe_cmi_panic_spinlock;
static fbe_cmi_panic_state_t fbe_cmi_panic_state;

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

void fbe_cmi_panic_init(void)
{
    fbe_spinlock_init(&fbe_cmi_panic_spinlock);
    fbe_cmi_panic_state = FBE_CMI_PANIC_STATE_INVALID;
}   

void fbe_cmi_panic_destroy(void)
{
    fbe_spinlock_destroy(&fbe_cmi_panic_spinlock);
}   

fbe_status_t fbe_cmi_panic_get_permission(fbe_bool_t b_panic_this_sp)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cmi_panic_state_t desired_state;
    fbe_spinlock_lock(&fbe_cmi_panic_spinlock);
    if (b_panic_this_sp)
    {
        desired_state = FBE_CMI_PANIC_STATE_THIS_SP;
    }
    else
    {
        desired_state = FBE_CMI_PANIC_STATE_PEER_SP;
    }
    if (fbe_cmi_panic_state == FBE_CMI_PANIC_STATE_INVALID)
    {
        /* we have not decided to panic yet. Save the desired state and return success.
         */
        fbe_cmi_panic_state = desired_state;
        status = FBE_STATUS_OK;
    }
    else if (fbe_cmi_panic_state == desired_state)
    {
        /* We already granted this desired state, just return success.
         */
        status = FBE_STATUS_OK;
    }
    fbe_spinlock_unlock(&fbe_cmi_panic_spinlock);
    return status;
}
void fbe_cmi_panic_reset(void)
{
    fbe_spinlock_lock(&fbe_cmi_panic_spinlock);
    fbe_cmi_panic_state = FBE_CMI_PANIC_STATE_INVALID;
    fbe_spinlock_unlock(&fbe_cmi_panic_spinlock);
} 
/*************************
 * end file fbe_cmi_service_panic.c
 *************************/


