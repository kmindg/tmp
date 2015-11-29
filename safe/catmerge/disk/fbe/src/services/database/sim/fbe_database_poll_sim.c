/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
#include "fbe_database.h"
#include "fbe_database_private.h"
#include "fbe/fbe_time.h"

/*!**************************************************************
 * fbe_database_poll_complain()
 ****************************************************************
 * @brief
 *  If the number of full polls (FBE_DATABASE_MAX_POLL_RECORDS) 
 *  was exceeded within some specified time (FBE_DATABASE_MAX_FULL_POLL_TIMER_SECONDS), 
 *  Trace the problem for simulation
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
                   "%s: Polling is too frequent >%d polls in %d seconds\n", 
                   __FUNCTION__,
                   FBE_DATABASE_MAX_POLL_RECORDS,
                   FBE_DATABASE_MAX_FULL_POLL_TIMER_SECONDS);
}

/*!**************************************************************
 * @fn fbe_database_poll_is_recordkeeping_active
 ****************************************************************
 * @brief
 *  Determines whether to record statistics for full polls 
 * 
 * @param 
 * 
 * @return 
 *   fbe_bool_t - whether or not we should poll stats should be tracked
 *
 * @version
 *   03/15/2012:  Wayne Garrett - Created.
 *   2/13/2013:  Deanna Heng
 *
 ****************************************************************/
fbe_bool_t fbe_database_poll_is_recordkeeping_active(void)
{
    return FBE_DATABASE_DEFAULT_IS_ACTIVE;
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
    return fbe_database_get_extended_cache_enabled(); 
}
