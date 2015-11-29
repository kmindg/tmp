/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

#include "fbe_database.h"
#include "fbe_database_private.h"
#include "fbe/fbe_time.h"
#include "fbe_panic_sp.h"
#include "fbe/fbe_registry.h"


/*!**************************************************************
 * fbe_database_poll_complain()
 ****************************************************************
 * @brief
 *  If the number of full polls (FBE_DATABASE_MAX_POLL_RECORDS) 
 *  was exceeded within some specified time (FBE_DATABASE_MAX_FULL_POLL_TIMER_SECONDS), 
 *  Trace the problem for and panic for checked builds
 *
 * @param 
 *
 * @return 
 *
 * @author
 *  2/13/2013 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_database_poll_complain(void) 
{
    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Polling is too frequent %d polls in %d seconds\n", 
                   __FUNCTION__,
                   FBE_DATABASE_MAX_POLL_RECORDS,
                   FBE_DATABASE_MAX_FULL_POLL_TIMER_SECONDS);
#if DBG
    FBE_PANIC_SP(FBE_PANIC_SP_BASE, FBE_STATUS_OK);
#endif

}

/*!**************************************************************
 * @fn fbe_database_poll_is_recordkeeping_active
 ****************************************************************
 * @brief
 *  Determines whether to record statistics for full polls based
 *  on registry key (FBE_DATABASE_POLL_REG_KEY) value
 * 
 * @param 
 * 
 * @return fbe_bool_t - whether or not we should poll
 *
 * @version
 *   03/15/2012:  Wayne Garrett - Created.
 *   2/13/2013:  Deanna Heng
 *
 ****************************************************************/
fbe_bool_t fbe_database_poll_is_recordkeeping_active(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t key;
    fbe_bool_t defaultKey = FBE_DATABASE_DEFAULT_IS_ACTIVE;

    status = fbe_registry_read(NULL,
                               SEP_REG_PATH,
                               FBE_DATABASE_POLL_REG_KEY,  
                               &key,
                               sizeof(key),
                               FBE_REGISTRY_VALUE_DWORD,
                               &defaultKey,   // default 
                               0,                                     
                               FALSE);

    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Failed to read registry value for key %s\n", 
                       __FUNCTION__, FBE_DATABASE_POLL_REG_KEY);

        return defaultKey;
    }

    return (fbe_bool_t)key;
}

/*!**************************************************************
 * @fn fbe_database_is_vault_drive_tier_enabled
 ****************************************************************
 * @brief
 *  Determines whether to shoot drives based on vault drive tier
 *  compatibility. This is a requirement of extended cache
 * 
 * @param 
 * 
 * @return 
 *   fbe_bool_t 
 *
 * @version
 *   2/13/2014:  Deanna Heng
 *
 ****************************************************************/
fbe_bool_t fbe_database_is_vault_drive_tier_enabled(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t key;
    fbe_bool_t defaultKey = FBE_FALSE;

    status = fbe_registry_read(NULL,
                               K10DriverConfigRegPath,
                               FBE_DATABASE_EXTENDED_CACHE_KEY,  
                               &key,
                               sizeof(key),
                               FBE_REGISTRY_VALUE_DWORD,
                               &defaultKey,   // default 
                               0,                                     
                               FALSE);

    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Failed to read registry value for key %s\n", 
                       __FUNCTION__, FBE_DATABASE_EXTENDED_CACHE_KEY);

        return defaultKey;
    }

    return (fbe_bool_t)key;
}
