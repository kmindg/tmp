/*******************************************************************
 * Copyright (C) EMC Corporation 2000 - 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/

/*******************************************************************
 *
 *  @brief
 *    This file contains functions for the Sector tracing facility.
 *    
 *      
 *  @author
 *
 *  01-14-2010  Omer Miranda - Original version.
 *
 *******************************************************************/

/*************************
 * INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_sector_trace_interface.h"
#include "fbe_sector_trace_private.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_library_interface.h"
#include "fbe_base.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_database.h"
#include "fbe/fbe_registry.h"


/****************************************
 * MACROS
 ****************************************/

/***********************
 * STRUCTURE DEFINITIONS
 ***********************/
typedef struct fbe_sector_trace_service_s 
{
    fbe_base_service_t      base_service;
    fbe_sector_trace_info_t sector_trace_info;
} fbe_sector_trace_service_t;

/***********************
 * FORWARD REFERENCES
 ***********************/
fbe_status_t fbe_sector_trace_control_entry(fbe_packet_t * packet);
static void fbe_sector_trace_set_stop_system_on_error(fbe_bool_t stop_on_error);

static void fbe_sector_trace_set_stop_on_error_flags(fbe_sector_trace_error_flags_t sector_err_flag,fbe_u32_t additional_flag, fbe_bool_t persist);
static fbe_status_t fbe_sector_trace_read_ssoef_flag_from_registry(fbe_sector_trace_error_flags_t* flag_p ); 
static fbe_status_t fbe_sector_trace_persist_ssoef_flag_to_registry(fbe_sector_trace_error_flags_t flag) ;

static fbe_status_t fbe_sector_trace_read_error_flags_from_registry(fbe_sector_trace_error_flags_t* flag_p ); 
static fbe_status_t fbe_sector_trace_persist_error_flags_to_registry(fbe_sector_trace_error_flags_t flag) ;

static fbe_status_t fbe_sector_trace_read_error_level_from_registry(fbe_sector_trace_error_level_t* flag_p ); 
static fbe_status_t fbe_sector_trace_persist_error_level_to_registry(fbe_sector_trace_error_level_t flag) ;

static fbe_status_t fbe_sector_trace_read_addtional_event_from_registry(fbe_u32_t* flag_p );

/*!*******************************************************************
 * @def FBE_SECTOR_TRACE_STOP_ON_ERROR_FLAGS
 *********************************************************************
 * @brief Name of registry key where we keep the stop_on_error_flag flags
 *
 *********************************************************************/
#define FBE_SECTOR_TRACE_STOP_ON_ERROR_FLAGS "fbe_sector_trace_stop_on_error_flags"

/*!*******************************************************************
 * @def FBE_SECTOR_TRACE_STOP_ON_ADDITIONAL_EVENT
 *********************************************************************
 * @brief Name of registry key where we keep the fbe_sector_trace_stop_on_additional_event
 *
 *********************************************************************/
#define FBE_SECTOR_TRACE_STOP_ON_ADDITIONAL_EVENT "fbe_sector_trace_stop_on_additional_event"


/*!*******************************************************************
 * @def FBE_SECTOR_TRACE_TRACE_LEVEL_FLAGS
 *********************************************************************
 * @brief Name of registry key where we keep the fbe_sector_trace_trace_level_flags  flags
 *
 *********************************************************************/
#define FBE_SECTOR_TRACE_TRACE_LEVEL_FLAGS "fbe_sector_trace_trace_level_flags"

/*!*******************************************************************
 * @def FBE_SECTOR_TRACE_TRACE_MASK
 *********************************************************************
 * @brief Name of registry key where we keep the fbe_sector_trace_trace_mask flags
 *
 *********************************************************************/
#define FBE_SECTOR_TRACE_TRACE_MASK "fbe_sector_trace_trace_mask"



/***********************
 * GLOBALS
 ***********************/
fbe_service_methods_t               fbe_sector_trace_service_methods = {FBE_SERVICE_ID_SECTOR_TRACE, fbe_sector_trace_control_entry};
static fbe_sector_trace_service_t   fbe_sector_trace_service;
fbe_u32_t stop_on_additional_raid_event;

/* This is copied from fbe_trace_panic.c
 */
fbe_status_t fbe_sector_trace_stop_system(void)
{
#if !(defined(UMODE_ENV) || defined(SIMMODE_ENV)) 
    extern fbe_status_t fbe_database_cmi_send_mailbomb(void);

    /* Send mailbomb to peer.
     */
    if (fbe_database_cmi_send_mailbomb() == FBE_STATUS_OK) {
        fbe_base_service_trace(&fbe_sector_trace_service.base_service,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Sent mailbomb to peer\n", 
                               __FUNCTION__);
    }
#endif

    fbe_base_service_trace(&fbe_sector_trace_service.base_service,
                           FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s Stopping SP\n", 
                           __FUNCTION__);
#if !(defined(UMODE_ENV) || defined(SIMMODE_ENV)) 
	CSX_BREAK(); 
	/* blue screen at this point. Suppress Prefast warning 311 which is a 
	 * suggestion to not use KeBugCheckEx() which is proper here.
	 */
#pragma warning(disable: 4068)
#pragma prefast(disable: 311)
/*! @todo need to add a proper panic code here.
 */
	EmcpalBugCheck(0, 0, 0, /* panic code */ __LINE__, 0);
#pragma prefast(default: 311)
#pragma warning(default: 4068)
#else
    fbe_DbgBreakPoint();
#endif
    return FBE_STATUS_OK;
}

/*!***************************************************************
 *          fbe_sector_trace_get_info()
 ****************************************************************
 *
 * @brief   Returns the pointer to the sector trace information
  
 * @param   None.
 *      
 * @return  Pointer to fbe_sector_trace_info_t   
 *
 ****************************************************************/
static __forceinline fbe_sector_trace_info_t *fbe_sector_trace_get_info(void)
{
    return(&fbe_sector_trace_service.sector_trace_info);
}
/* end of fbe_sector_trace_get_info() */

/************************************************************ 
 *  fbe_sector_trace_lock
 *   Obtain the spinlock protecting the group of trace entries
 *   related to one sector.
 ************************************************************/
void fbe_sector_trace_lock(void)
{
    fbe_sector_trace_info_t *sector_trace_info_p = fbe_sector_trace_get_info();

    fbe_spinlock_lock(&sector_trace_info_p->sector_trace_lock);
    return;
}
/* end fbe_sector_trace_lock */

/************************************************************ 
 *  fbe_sector_trace_unlock
 *   Release the spinlock protecting a group of trace entries.
 ************************************************************/
void fbe_sector_trace_unlock(void)
{
    fbe_sector_trace_info_t *sector_trace_info_p = fbe_sector_trace_get_info();

    fbe_spinlock_unlock(&sector_trace_info_p->sector_trace_lock);
    return;
}
/* end fbe_sector_trace_unlock */

/************************************************************ 
 *  fbe_sector_trace_get_strings_lock()
 *   Obtain the spinlock protecting the location strings
 ************************************************************/
static void fbe_sector_trace_get_strings_lock(void)
{
    fbe_sector_trace_info_t *sector_trace_info_p = fbe_sector_trace_get_info();

    fbe_spinlock_lock(&sector_trace_info_p->location_strings_lock);
    return;
}
/* end fbe_sector_trace_lock */

/************************************************************ 
 *  fbe_sector_trace_release_strings_lock()
 *   Release the spinlock protecting the location strings.
 ************************************************************/
void fbe_sector_trace_release_strings_lock(void)
{
    fbe_sector_trace_info_t *sector_trace_info_p = fbe_sector_trace_get_info();

    fbe_spinlock_unlock(&sector_trace_info_p->location_strings_lock);
    return;
}
/* end fbe_sector_trace_release_strings_lock() */

/************************************************************ 
 *  fbe_sector_trace_can_trace
 *  Determine if we are allowed to trace taking into consideration
 *  that every quanta we are only allowed to fill the buffer once.
 ************************************************************/
fbe_bool_t fbe_sector_trace_can_trace(fbe_u32_t entries)
{
    fbe_sector_trace_info_t *sector_trace_info_p = fbe_sector_trace_get_info();
    fbe_u32_t ms;

    /* Determine if this number of entries can be added now.
     */
    fbe_sector_trace_lock();
    ms = fbe_get_elapsed_milliseconds(sector_trace_info_p->last_trace_time);
    if (ms > FBE_SECTOR_TRACE_FILL_BUFFER_QUANTA_MS) {
        /* Reset the start time and give a fresh set of entries to trace in this quanta.
         */
        sector_trace_info_p->num_entries_traced = 0;
        sector_trace_info_p->last_trace_time = fbe_get_time();
    }

    if (sector_trace_info_p->num_entries_traced < FBE_SECTOR_TRACE_ENTRIES) {
        /* Give permission for these entries.
         * We intentionally allow this to go over the limit since we want to use 
         * the entire trace buffer in this quanta. 
         */
        sector_trace_info_p->num_entries_traced += entries;
        fbe_sector_trace_unlock();
        return FBE_TRUE;
    }
    /* Not allowed to trace these entries, but track what we dropped.
     */
    sector_trace_info_p->total_entries_dropped += entries;
    fbe_sector_trace_unlock();
    return FBE_FALSE;
}
/* end fbe_sector_trace_can_trace */

/****************************************************************
 *  fbe_sector_trace_get_error_level
 ****************************************************************
 * @brief
 *      Returns the current value of the sector tracing level.
 *  
 * @param
 *      None.
 *      
 * @return
 *      fbe_sector_trace_err_level - Trace Error level to set 
 *      for error trace buffer.
 *
 * @author
 *  01/14/2010 :Omer Miranda - Created.
 *
 ****************************************************************/
static fbe_sector_trace_error_level_t fbe_sector_trace_get_error_level(void)
{
    fbe_sector_trace_info_t *sector_trace_info_p = fbe_sector_trace_get_info();

    return(sector_trace_info_p->sector_trace_err_level);
} /* fbe_sector_trace_get_error_level */

/*!***************************************************************************
 *          fbe_sector_trace_get_default_error_level()
 *****************************************************************************
 *
 * @brief   Returns the default sector trace error level.
 *  
 * @param   None
 *      
 * @return  default sector trace error level - Any sector trace requests with
 *          and error level equal or less than this value will be traced
 *
 *****************************************************************************/
static fbe_sector_trace_error_level_t fbe_sector_trace_get_default_error_level(void)
{
    return(FBE_SECTOR_TRACE_DEFAULT_ERROR_LEVEL);
}
/* end of fbe_sector_trace_get_default_error_level() */

/*!***************************************************************************
 *          fbe_sector_trace_get_default_error_flags()
 *****************************************************************************
 *
 * @brief   Returns the default sector trace error flags.
 *  
 * @param   None
 *      
 * @return  default sector trace error flags - Each flag bit represents a 
 *          particular error type.  Only those error types that are set in
 *          sector trace error flags will result in the trace entry being
 *          saved to the appropriate sector trace buffer.
 *
 *****************************************************************************/
static fbe_sector_trace_error_flags_t fbe_sector_trace_get_default_error_flags(void)
{
    /*! Simply return the default error flags which currently is a constant
     *
     *  @todo May need to `load' the default trace flags from a different location
     */
    return(FBE_SECTOR_TRACE_DEFAULT_ERROR_FLAGS);
}
/* end of fbe_sector_trace_get_default_error_flags() */

static fbe_status_t fbe_sector_trace_read_error_level_from_registry(fbe_sector_trace_error_level_t * flag_p)
{
    fbe_status_t status;
    fbe_sector_trace_error_level_t flags;
    fbe_u32_t default_input_value = fbe_sector_trace_get_default_error_level();
    *flag_p = default_input_value;
    status = fbe_registry_read(NULL,
                               SEP_REG_PATH,
                               FBE_SECTOR_TRACE_TRACE_LEVEL_FLAGS,
                               &flags,
                               sizeof(fbe_u32_t),
                               FBE_REGISTRY_VALUE_DWORD,
                               &default_input_value,
                               sizeof(fbe_u32_t),
                               FALSE);
    if (status != FBE_STATUS_OK)
    {        
	    fbe_base_service_trace(&fbe_sector_trace_service.base_service,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s failed to read %s from registry 0x%x\n", __FUNCTION__, FBE_SECTOR_TRACE_TRACE_MASK, flags);
        return FBE_STATUS_GENERIC_FAILURE;        
    }
	*flag_p = flags;
    return status;
}

static fbe_status_t fbe_sector_trace_persist_error_level_to_registry(fbe_sector_trace_error_level_t flag )
{
    fbe_status_t status;

    status = fbe_registry_write(NULL,
                                SEP_REG_PATH,
                                FBE_SECTOR_TRACE_TRACE_LEVEL_FLAGS,
                                FBE_REGISTRY_VALUE_DWORD,
                                &flag, 
                                sizeof(fbe_u32_t));

    if (status != FBE_STATUS_OK)
    {
    
	    fbe_base_service_trace(&fbe_sector_trace_service.base_service,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s failed writing %s to 0x%x\n", __FUNCTION__,FBE_SECTOR_TRACE_TRACE_LEVEL_FLAGS,flag );
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    return status;
}


/****************************************************************
 *  fbe_sector_trace_set_error_level
 ****************************************************************
 * @brief  
 *       Overrides the current sector tracing level value if the
 *       passed value is sane.  Used to allow FCLI to over-ride 
 *       tracing level.
 *  
 * @param sector_err_level - Trace level to set for error trace buffer.
 *      
 *
 * @return
 *       None.
 *
 * @author
 *  01/14/2010 :Omer Miranda - Created.
 *
 ****************************************************************/
static void fbe_sector_trace_set_error_level(fbe_sector_trace_error_level_t sector_err_level,fbe_bool_t persist)
{
    fbe_sector_trace_info_t *sector_trace_info_p = fbe_sector_trace_get_info();

    if (sector_err_level <= FBE_SECTOR_TRACE_ERROR_LEVEL_MAX)
    {
        sector_trace_info_p->sector_trace_err_level = sector_err_level;
    }
    else
    {
        sector_trace_info_p->sector_trace_err_level = fbe_sector_trace_get_default_error_level();
    }
    if( persist == FBE_TRUE ){
		fbe_sector_trace_persist_error_level_to_registry(sector_err_level);
    }

    return;

} /* fbe_sector_trace_set_error_level */

/****************************************************************
 *  fbe_sector_trace_get_error_flags
 ****************************************************************
 * @brief
 *      Returns the current value of the sector tracing error type.
 *  
 * @param
 *      None.
 *      
 * @return
 *      fbe_sector_trace_error_flags_t - Trace Error type to set 
 *      for error trace buffer.
 *
 * @author
 *  01/14/2010 :Omer Miranda - Created.
 *
 ****************************************************************/
static fbe_sector_trace_error_flags_t fbe_sector_trace_get_error_flags(void)
{
    fbe_sector_trace_info_t *sector_trace_info_p = fbe_sector_trace_get_info();

    return(sector_trace_info_p->sector_trace_err_flag);
} /* fbe_sector_trace_get_error_flags */

/****************************************************************
 *  fbe_sector_trace_get_stop_on_error_flags
 ****************************************************************
 * @brief
 *      Returns the current value of stop on error flags.
 *  
 * @param
 *      None.
 *      
 * @return
 *      fbe_sector_trace_error_flags_t - Trace Error type to set 
 *      for error trace buffer.
 *
 * @author
 *  03/01/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_sector_trace_error_flags_t fbe_sector_trace_get_stop_on_error_flags(void)
{
    fbe_sector_trace_info_t *sector_trace_info_p = fbe_sector_trace_get_info();

    return(sector_trace_info_p->sector_trace_stop_on_error_flag);
} /* fbe_sector_trace_get_stop_on_error_flags */

/****************************************************************
 *  fbe_sector_trace_get_stop_on_error_enabled()
 ****************************************************************
 * @brief
 *      Returns the current value of stop on error enabled.
 *  
 * @param
 *      None.
 *      
 * @return
 *      bool
 *
 * @author
 *  03/01/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_bool_t fbe_sector_trace_get_stop_on_error_enabled(void)
{
    fbe_sector_trace_info_t *sector_trace_info_p = fbe_sector_trace_get_info();

    return(sector_trace_info_p->sector_trace_stop_on_error);
} /* fbe_sector_trace_get_stop_on_error_enabled */

static fbe_status_t fbe_sector_trace_read_error_flags_from_registry(fbe_sector_trace_error_flags_t* flag_p )
{
    fbe_status_t status;
    fbe_sector_trace_error_flags_t flags;
    fbe_u32_t default_input_value = fbe_sector_trace_get_default_error_flags();
    *flag_p = default_input_value;
    status = fbe_registry_read(NULL,
                               SEP_REG_PATH,
                               FBE_SECTOR_TRACE_TRACE_MASK,
                               &flags,
                               sizeof(fbe_u32_t),
                               FBE_REGISTRY_VALUE_DWORD,
                               &default_input_value,
                               sizeof(fbe_u32_t),
                               FALSE);
    if (status != FBE_STATUS_OK)
    {        
	    fbe_base_service_trace(&fbe_sector_trace_service.base_service,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s failed to read %s from registry 0x%x\n", __FUNCTION__,FBE_SECTOR_TRACE_TRACE_MASK, flags);
        return FBE_STATUS_GENERIC_FAILURE;        
    }
	*flag_p = flags;
    return status;
}

static fbe_status_t fbe_sector_trace_persist_error_flags_to_registry(fbe_sector_trace_error_flags_t flag )
{
    fbe_status_t status;

    status = fbe_registry_write(NULL,
                                SEP_REG_PATH,
                                FBE_SECTOR_TRACE_TRACE_MASK,
                                FBE_REGISTRY_VALUE_DWORD,
                                &flag, 
                                sizeof(fbe_u32_t));

    if (status != FBE_STATUS_OK)
    {
    
	    fbe_base_service_trace(&fbe_sector_trace_service.base_service,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s failed writing %s to 0x%x\n", __FUNCTION__,FBE_SECTOR_TRACE_TRACE_MASK,flag );
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    return status;
}


/****************************************************************
 *  fbe_sector_trace_set_error_flags
 ****************************************************************
 * @brief
 *      Overrides the current sector tracing error type value if the
 *      passed value is sane.  Used to allow FCLI to over-ride 
 *      tracing error type.
 *  
 * @param
 *      sector_err_flag - Trace error type to set for error trace buffer.
 *      
 *
 * @return
 *      None.
 *
 * @author
 *  01/14/2010 :Omer Miranda - Created.
 *
 ****************************************************************/
static void fbe_sector_trace_set_error_flags(fbe_sector_trace_error_flags_t sector_err_flag,fbe_bool_t persist)
{
    fbe_sector_trace_info_t *sector_trace_info_p = fbe_sector_trace_get_info();

    fbe_bool_t new_trace_location;

    /* To simplify the tracing checks, the global `xor_trace_location' 
     * is used to determine whether the function and line information
     * are traced or not.
     */
    new_trace_location = (sector_err_flag & FBE_SECTOR_TRACE_ERROR_FLAG_LOC) ? 1 : 0;
    sector_err_flag &= ~FBE_SECTOR_TRACE_ERROR_FLAG_LOC;
    if (sector_err_flag )
    {
        /* Notify user if value of location flag is changing.
         */
        if ( new_trace_location != sector_trace_info_p->sector_trace_location )
        {
            fbe_KvTraceRing(FBE_TRACE_RING_XOR_START, 
                            "sector_trace: %s - Location tracing flag changed from %d to %d \n",
                            __FUNCTION__, sector_trace_info_p->sector_trace_location, new_trace_location);
        }
        sector_trace_info_p->sector_trace_location = new_trace_location;
        sector_trace_info_p->sector_trace_err_flag = sector_err_flag;
    }
    else
    {
        sector_trace_info_p->sector_trace_err_flag = fbe_sector_trace_get_default_error_flags();
    }
    if( persist == FBE_TRUE ){
        fbe_sector_trace_persist_error_flags_to_registry(sector_err_flag);
    }

    return;
} /* fbe_sector_trace_set_error_flags */

/*!***************************************************************************
 *          fbe_sector_trace_increment_invocation_count()
 *****************************************************************************
 *
 * @brief   Increments the count of the number of times the sector tracing
 *          service was called (whether the sector was traced or not).
 *  
 * @param   error_level - The error level for the error reported
 * @param   error_flags - The error flags for the error reported
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  06/29/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_sector_trace_increment_invocation_count(void)
{
    fbe_sector_trace_info_t *sector_trace_info_p = fbe_sector_trace_get_info(); 

    /* Simply increment the total invacation counter.
     */
    if (sector_trace_info_p->total_invocations < FBE_SECTOR_TRACE_MAX_COUNT)
    {
        sector_trace_info_p->total_invocations++;
    }

    /* Always return the executions status
     */
    return(FBE_STATUS_OK);
}
/* fbe_sector_trace_increment_invocation_count() */

/*!***************************************************************************
 *          fbe_sector_trace_flags_to_index()
 *****************************************************************************
 *
 * @brief   Convert the sector trace error flags to an index into the error
 *          counters array.
 *  
 * @param   error_flags - The error flags for the error reported
 *
 * @return  index - Index of error counters
 *
 *****************************************************************************/
static __forceinline fbe_u32_t fbe_sector_trace_flags_to_index(fbe_sector_trace_error_flags_t error_flags)
{
    fbe_u32_t   index = 0;
    fbe_u32_t   bit_position;

    for (bit_position = 0; bit_position < (sizeof(error_flags) * 8); bit_position++)
    {
        if ((error_flags & (1 << bit_position)) != 0)
        {
            index = bit_position;
            break;
        }
    }

    return(index);
}
/* end of fbe_sector_trace_flags_to_index()*/

/*!***************************************************************************
 *          fbe_sector_trace_increment_error_counts()
 *****************************************************************************
 *
 * @brief   Increments the associated counters for the sector error types and
 *          level reported.
 *  
 * @param   error_level - The error level for the error reported
 * @param   error_flags - The error flags for the error reported
 * @param   b_initial_error_trace - This is the point where the error is reported
 * @param   b_sector_trace_start - This is the start of a sector dump
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  06/29/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_sector_trace_increment_error_counts(fbe_sector_trace_error_level_t error_level,
                                                            fbe_sector_trace_error_flags_t error_flags,
                                                            fbe_bool_t b_initial_error_trace,
                                                            fbe_bool_t b_sector_trace_start)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_sector_trace_info_t    *sector_trace_info_p = fbe_sector_trace_get_info();

    /* If this is a sector trace start increment the sectors dumped counter.
     */
    if (b_sector_trace_start == FBE_TRUE)
    {
        if (sector_trace_info_p->total_sectors_traced < FBE_SECTOR_TRACE_MAX_COUNT)
        {
            sector_trace_info_p->total_sectors_traced++;
        }
    }

    /*! @note Only on the `initial' error trace do we want to:
     *          o Increment the error count
     *          o Increment the correct error level counter
     *          o Increment the correct error type counter
     */
    if (b_initial_error_trace == FBE_TRUE)
    {
        /* Increment the count of errors reported.
         */
        if (sector_trace_info_p->total_errors_traced < FBE_SECTOR_TRACE_MAX_COUNT)
        {
            sector_trace_info_p->total_errors_traced++;
        }

        /* Increment the error level
         */
        if (error_level <= FBE_SECTOR_TRACE_ERROR_LEVEL_MAX)
        {
            if (sector_trace_info_p->error_level_counters[error_level] < FBE_SECTOR_TRACE_MAX_COUNT)
            {
                sector_trace_info_p->error_level_counters[error_level]++;
            }
        }
        else
        {
            /* Trace the error and return failure
             */
            fbe_KvTrace("sector_trace: %s - Invalid level %d \n",
                        __FUNCTION__, error_level);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /*! Increment the associated flags (error type)
         *
         *  @note  Currently it is assumed that only (1) flag bit will be set
         */
        if ((error_flags != 0)                                         &&
            ((error_flags & ~(FBE_SECTOR_TRACE_ERROR_FLAG_MASK)) == 0)    )
        {
            fbe_u32_t   error_flag_index;

            error_flag_index = fbe_sector_trace_flags_to_index(error_flags);
            if (error_flag_index >= FBE_SECTOR_TRACE_ERROR_FLAG_COUNT)
            { 
                fbe_KvTrace("sector_trace: %s - Invalid flag index %d flag: 0x%x \n", __FUNCTION__, error_flag_index, error_flags);
            }
            else if (sector_trace_info_p->error_type_counters[error_flag_index] < FBE_SECTOR_TRACE_MAX_COUNT)
            {
                sector_trace_info_p->error_type_counters[error_flag_index]++;
            }
        }
        else
        {
            /* Trace the error and return failure
             */
            fbe_KvTrace("sector_trace: %s - Invalid flags 0x%x \n",
                        __FUNCTION__, error_flags);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

    } /* end if this is the initial error trace */

    /* Always return the executions status
     */
    return(status);
}
/* end of fbe_sector_trace_increment_error_counts() */


/*!***************************************************************************
 *          fbe_sector_trace_is_initialized()
 *****************************************************************************
 * 
 * @brief   This method simply determines of the sector trace object has been
 *          initialized or not.
 *  
 * @param   None
 *      
 * @return  bool - FBE_TRUE - The sector trace service has been initialized
 *                 FBE_FALSE - The sector trace service has not been initialized
 *
 * @author
 *  06/18/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_bool_t fbe_sector_trace_is_initialized(void)
{
    /* Just return the value of the global*/
    return(fbe_sector_trace_service.base_service.initialized);
}
/* end of fbe_sector_trace_is_initialized() */

/****************************************************************
 *  fbe_sector_trace_set_external_build()
 ****************************************************************
 * @brief
 *      set retail build
 *  
 * @param   b_is_external_build - FBE_TRUE - Is retail build 
 *      
 *
 * @return
 *      None.
 *
 * @author
 *  2/03/2014 mferson - Created.
 *
 ****************************************************************/
static void fbe_sector_trace_set_external_build(fbe_bool_t b_is_external_build)
{

    fbe_sector_trace_service.sector_trace_info.b_is_external_build = b_is_external_build;

    return;
} /* fbe_sector_trace_set_external_build */

/*****************************************************************************
 *          fbe_sector_trace_init()
 ***************************************************************************** 
 * 
 * @brief   Initialize the fbe sector tracing facility.
 *  
 * @param   packet  - Pointer to packet with init request
 *      
 * @return  status - Status of the initialization request
 *
 * @author
 *  01/15/2010 :Omer Miranda - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_sector_trace_init(fbe_packet_t * packet)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_sector_trace_info_t    *sector_trace_info_p = fbe_sector_trace_get_info();
    fbe_sector_trace_error_flags_t stop_on_error_flags = FBE_SECTOR_TRACE_ERROR_FLAG_NONE;
    fbe_sector_trace_error_flags_t error_flags = FBE_SECTOR_TRACE_DEFAULT_ERROR_FLAGS;
    fbe_sector_trace_error_level_t trace_error_level = FBE_SECTOR_TRACE_DEFAULT_ERROR_LEVEL;

    /* Initialize the base service
     */
    fbe_base_service_set_service_id(&fbe_sector_trace_service.base_service, 
                                    FBE_SERVICE_ID_SECTOR_TRACE);
    fbe_base_service_set_trace_level(&fbe_sector_trace_service.base_service, 
                                     fbe_trace_get_default_trace_level());
    fbe_base_service_init(&fbe_sector_trace_service.base_service);
    

    /* Now initialize our private structure
     */
    fbe_zero_memory((void *)sector_trace_info_p, sizeof(*sector_trace_info_p));

    /* The defaults */
    fbe_sector_trace_set_stop_system_on_error(FBE_FALSE);

    fbe_sector_trace_read_ssoef_flag_from_registry(&stop_on_error_flags);
    fbe_KvTrace("%s: stop_on_error_flags =0x%x \n",
                __FUNCTION__, stop_on_error_flags);
	fbe_sector_trace_read_addtional_event_from_registry(&stop_on_additional_raid_event);
    fbe_KvTrace("%s: stop_on_additional_raid_event =0x%x \n",
                __FUNCTION__, stop_on_additional_raid_event);
	
    fbe_sector_trace_set_stop_on_error_flags((fbe_sector_trace_error_flags_t)stop_on_error_flags,0,FBE_FALSE);
    if( stop_on_error_flags != FBE_SECTOR_TRACE_ERROR_FLAG_NONE ){
        fbe_sector_trace_set_stop_system_on_error(FBE_TRUE);	
    }
	
    fbe_sector_trace_set_external_build(FBE_FALSE);

/*********************************************************************

    // Here I need to get the trace_flag. Right now setting to default

***********************************************************************/
    fbe_sector_trace_read_error_flags_from_registry(&error_flags);
    fbe_KvTrace("%s: error_flags =0x%x \n",
                __FUNCTION__, error_flags);
	
    fbe_sector_trace_set_error_flags(error_flags,FBE_FALSE);

/*********************************************************************

    // Here I need to get the trace_level. Right now setting to default

***********************************************************************/
    fbe_sector_trace_read_error_level_from_registry(&trace_error_level);
    fbe_KvTrace("%s: trace_error_level =0x%x \n",
                __FUNCTION__, trace_error_level);

    fbe_sector_trace_set_error_level(trace_error_level,FBE_FALSE);

    /* Default is to not trace the location (function and line
     * number).
     */
    sector_trace_info_p->sector_trace_location = FBE_FALSE;

    /* Init the spin lock that protects the group of trace
     * entries for one sector.
     */
    fbe_spinlock_init(&sector_trace_info_p->sector_trace_lock);

    sector_trace_info_p->last_trace_time = 0;
    sector_trace_info_p->num_entries_traced = 0;
    sector_trace_info_p->total_entries_dropped = 0;

    /* Init the spinlock that protects the location string buffers
     */
    fbe_spinlock_init(&sector_trace_info_p->location_strings_lock);

    /* Need to initialize fbe ktrace infrastructure as required
     */
    if (fbe_ktrace_is_initialized() == FBE_FALSE)
    {
        fbe_ktrace_initialize();
    }

    /* Now complete the control packet */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0); 
    fbe_transport_complete_packet(packet);

    /* Always return the execution status
     */
    return(status);
} 
/* end of fbe_sector_trace_init */

/*!***************************************************************************
 *          fbe_sector_trace_destroy()
 *****************************************************************************
 *
 * @brief   Destroy the sector trace module.
 *
 * @param   packet - Pointer to packet with destroy               
 *
 * @return fbe_status_t   
 *
 * @author
 *  3/22/2010 - Created. Omer Miranda
 *
 *****************************************************************************/
static fbe_status_t fbe_sector_trace_destroy(fbe_packet_t * packet)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_sector_trace_info_t    *sector_trace_info_p = fbe_sector_trace_get_info();

    /* First destroy the spinlocks.
     */
    fbe_spinlock_destroy(&sector_trace_info_p->sector_trace_lock);
    fbe_spinlock_destroy(&sector_trace_info_p->location_strings_lock);

    /* Now destroy the base service
     */
    fbe_base_service_destroy(&fbe_sector_trace_service.base_service);

    /* Must destroy the fbe ktrace also */
    fbe_ktrace_destroy();

    /* Now complete the packet
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_sector_trace_destroy() */

/*!***************************************************************************
 *          fbe_sector_trace_get_function_string()
 *****************************************************************************
 *
 * @brief   Take the lock for the function and line strings then return the
 *          function string address that is empty.
 *
 * @param   None
 *
 * @return  fbe_char_t * - Pointer to empty string
 *                
 * @author
 *  07/01/2010  Ron Proulx  - Created   
 *
 *****************************************************************************/
fbe_char_t *fbe_sector_trace_get_function_string(void)
{
    fbe_sector_trace_info_t *sector_trace_info_p = fbe_sector_trace_get_info();

    /* First get the lock
     */
    fbe_sector_trace_get_strings_lock();

    /* Create empty string
     */
    sector_trace_info_p->function_string[0] = '\0';

    /* Return the address of the string
     */
    return(&sector_trace_info_p->function_string[0]);
}
/* end of fbe_sector_trace_get_function_string()*/

/*!***************************************************************************
 *          fbe_sector_trace_get_line_string()
 *****************************************************************************
 *
 * @brief   Return the line string address that is empty.
 *
 * @param   None
 *
 * @return  fbe_char_t * - Pointer to empty string
 *                
 * @author
 *  07/01/2010  Ron Proulx  - Created   
 *
 *****************************************************************************/
fbe_char_t *fbe_sector_trace_get_line_string(void)
{
    fbe_sector_trace_info_t *sector_trace_info_p = fbe_sector_trace_get_info();

    /* Create empty string
     */
    sector_trace_info_p->line_string[0] = '\0';

    /* Return the address of the string
     */
    return(&sector_trace_info_p->line_string[0]);
}
/* end of fbe_sector_trace_get_line_string()*/

/****************************************************************
 *  fbe_sector_trace_need_to_stop
 ****************************************************************
 * @brief
 *      Determines if the supplied trace error_type need to cause 
 *  system to stop.
 *
 * @param   trace_err_flags - Trace error flagd for this request
 *
 * @return
 *      fbe_bool_t 
 *      FBE_TRUE  - should stop.
 *                
 * @author
 *  11/10/2011 :NChiu - Created.
 *
 ****************************************************************/
static fbe_bool_t fbe_sector_trace_need_to_stop(fbe_sector_trace_error_flags_t trace_err_flags)
{
    /* if we have debug flags set, treat it as enabled*/
    if ((trace_err_flags &  fbe_sector_trace_service.sector_trace_info.sector_trace_stop_on_error_flag) &&
        (fbe_sector_trace_service.sector_trace_info.sector_trace_stop_on_error == FBE_TRUE))
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
/* fbe_sector_trace_need_to_stop */

/****************************************************************
 *  fbe_sector_trace_is_enabled
 ****************************************************************
 * @brief
 *      Determines if the supplied trace error_type and  
 *      error_level will result in the trace entry being  
 *      generated or not.  This is used to save resources 
 *      so that the resources are not allocated if there will 
 *      be no need due to the tracing not being enabled.
 *
 * @param   trace_err_level - Trace error level for this request
 * @param   trace_err_flags - Trace error flagd for this request
 *
 * @return
 *      fbe_bool_t 
 *      FBE_TRUE  - The entry will be logged in the Xor trace buffer.
 *      FBE_FALSE - The trace entry will not be logged
 *                
 * @author
 *  01/15/2010 :Omer Miranda - Created.
 *
 ****************************************************************/

fbe_bool_t fbe_sector_trace_is_enabled(fbe_sector_trace_error_level_t trace_err_level,
                                       fbe_sector_trace_error_flags_t trace_err_flags)
{
    /* Both the error flag and level must
     * be enabled to save the trace entry.
     */
    if ((trace_err_level <= fbe_sector_trace_service.sector_trace_info.sector_trace_err_level) &&
        (trace_err_flags &  fbe_sector_trace_service.sector_trace_info.sector_trace_err_flag)     )
    {
        return(FBE_TRUE);
    }
    /* if we have stop on error flags set, treat it as enabled*/
    if (trace_err_flags &  fbe_sector_trace_service.sector_trace_info.sector_trace_stop_on_error_flag) 
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
/* fbe_sector_trace_is_enabled */


/*!***************************************************************************
 *          fbe_sector_trace_is_location_enabled()
 *****************************************************************************
 *
 * @brief   Determines if the location information should be added to the trace
 *          entry or not.
 *  
 * @param   None
 *
 * @return  bool - FBE_TRUE - Location information should be traced
 *                 FBE_FALSE - Do not trace the location information
 *
 * @author
 *  06/18/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static __forceinline fbe_bool_t fbe_sector_trace_is_location_enabled(void)
{
    if (fbe_sector_trace_service.sector_trace_info.sector_trace_location == FBE_TRUE)
    {
        return(FBE_TRUE);
    }

    return(FBE_FALSE);
}
/* end of fbe_sector_trace_get_line_string() */ 

/*!***************************************************************************
 *          fbe_sector_trace_entry()
 *****************************************************************************
 * @brief
 *      Saves the requested data in the sector trace buffer.
 *  
 * @param   error_level - The error level for this trace request      
 * @param   error_flag - The error type for this trace request 
 * @param   trace_location - Determines if the location should be traced or not
 * @param   function_p - Pointer to invoking function string                    
 * @param   line - The line number of functions that invoked us
 * @param   b_initial_error_trace - This is the point where the error is reported
 * @param   sector_trace_start - This is the start of a sector dump
 * @param   fmt_p - Pointer to format string for this request.
 * @param   function_string_p - Function buffer to optionally trace location
 * @param   line_string_p - Line buffer to optionally trace location
 * @param   (..) - Additional parameters                                       
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  07/06/2010  Ron Proulx  - Created
 * 
 *****************************************************************************/
fbe_status_t fbe_sector_trace_entry(fbe_sector_trace_error_level_t error_level,
                                    fbe_sector_trace_error_flags_t error_flag,
                                    fbe_bool_t trace_location,
                                    const fbe_char_t *function_p,
                                    const fbe_u32_t line,
                                    fbe_bool_t b_initial_error_trace,
                                    fbe_bool_t sector_trace_start,
                                    const fbe_char_t *fmt_p,
                                    ...)
{
    fbe_status_t                        status = FBE_STATUS_OK;

    /* If we need to initialize the sector trace object do it now.
     */
    if (fbe_sector_trace_is_initialized() == FBE_FALSE)
    {
        /*! @note This will not have a corresponding call to destroy.  Therefore
         *        this is an error.
         */
        fbe_KvTrace("sector_trace: %s invoked from %s line: %d without being initialized. \n",
                __FUNCTION__, function_p, line);
        return(FBE_STATUS_NOT_INITIALIZED);
    }

    /* Increment total invocations
     */
    fbe_sector_trace_increment_invocation_count();

    /* Determine if the event should be traced or not.
     */
    if (fbe_sector_trace_is_enabled(error_level, error_flag))
    {
        static fbe_trace_ring_t ring = FBE_TRACE_RING_XOR_START;
        fbe_char_t              prefix_fmt[FBE_SECTOR_TRACE_MAX_FMT_STRING_WITH_PREFIX] = "%s%s";
        va_list                 arg_list_p;

        /* Increment the associated counters
         */
        status = fbe_sector_trace_increment_error_counts(error_level, error_flag, b_initial_error_trace, sector_trace_start);
        if (status != FBE_STATUS_OK)
        {
            fbe_sector_trace_release_strings_lock();
            return(status);
        }

        /* If the passed format string is too long return an error
         */
        if (strlen(fmt_p) > (FBE_SECTOR_TRACE_MAX_FMT_STRING_WITH_PREFIX - 5))
        {
            fbe_KvTrace("sector_trace: %s - Format string %s is too long \n",
                        __FUNCTION__, fmt_p);
            fbe_sector_trace_release_strings_lock();
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /* Generate the format string including the prefix.
         */
        strncat(prefix_fmt, fmt_p, (FBE_SECTOR_TRACE_MAX_FMT_STRING_WITH_PREFIX - 1));
        prefix_fmt[(FBE_SECTOR_TRACE_MAX_FMT_STRING_WITH_PREFIX - 1)] = '\0';

        /* Change ring buffer to prevent wrapping of Xor
         * start buffer.
         */
        if ( (ring == FBE_TRACE_RING_XOR_START) &&
             (fbe_KtraceEntriesRemaining(FBE_TRACE_RING_XOR_START) <= 0) )
        {
            ring = FBE_TRACE_RING_XOR;
        }

        /* Generate the argument list form the variable arguments
         */
        va_start(arg_list_p, fmt_p);
        
        /* Currently we limit the location tracing to events that are
         * info and higher priority!!
         */
        if ((trace_location                         || 
             fbe_sector_trace_is_location_enabled()    )       &&
            (error_level <= FBE_SECTOR_TRACE_ERROR_LEVEL_INFO)     )
        {
            fbe_KvTraceLocation(ring, (fbe_char_t *)function_p, line,
                                FBE_SECTOR_TRACE_FUNCTION_STR_LEN, FBE_SECTOR_TRACE_LINE_STR_LEN, 
                                prefix_fmt, arg_list_p);
        }
        else
        {
            /* Default is to not trace location thus use empty strings.
             */
            fbe_KvTraceRingArgList(ring, prefix_fmt, arg_list_p);
        }            

         /* Close argument list
          */
         va_end(arg_list_p);

    }

    /* Release the location strings lock
     */
    fbe_sector_trace_release_strings_lock();

    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_sector_trace_entry() */

/*!***************************************************************************
 *          fbe_sector_trace_check_stop_on_error()
 ***************************************************************************** 
 * 
 * @brief   If the passed error type is set in the stop on error flags then
 *          stop the SP.
 *
 * @param   error_flag - The error flag that was just traced.
 *
 * @return  status - The status of the operation. 
 *
 * @author
 *
 *****************************************************************************/
fbe_status_t fbe_sector_trace_check_stop_on_error(fbe_sector_trace_error_flags_t error_flag)
{
    if (fbe_sector_trace_need_to_stop(error_flag))
    {
        fbe_sector_trace_stop_system();
    }
    return FBE_STATUS_OK;
}
/********************************************** 
 * end fbe_sector_trace_check_stop_on_error()
 *********************************************/

/*!***************************************************************************
 *          fbe_sector_trace_validate_control_request()
 ***************************************************************************** 
 * 
 * @brief   This function gets the buffer pointer out of the packet and validates
 *          it. It returns error status if any of the   pointer validations fail
 *          or the buffer size doesn't match the specifed buffer length.
 *
 * @param   in_packet_p       - Pointer to the packet.
 * @param   in_buffer_length  - Expected length of the buffer.
 * @param   out_buffer_pp     - Pointer to the buffer pointer. 
 *
 * @return status - The status of the operation. 
 *
 * @author
 *
 *****************************************************************************/
static fbe_status_t fbe_sector_trace_validate_control_request(fbe_packet_t *in_packet_p,
                                        fbe_payload_control_buffer_length_t in_buffer_length,
                                        fbe_payload_control_buffer_t *out_buffer_pp)
{
    fbe_payload_ex_t                  *payload_p;
    fbe_payload_control_operation_t    *control_operation_p;  
    fbe_payload_control_buffer_length_t buffer_length;


    // Get the control op buffer data pointer from the packet payload.
    payload_p = fbe_transport_get_payload_ex(in_packet_p);
    if (payload_p == NULL)
    {
        fbe_base_service_trace(&fbe_sector_trace_service.base_service,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        fbe_base_service_trace(&fbe_sector_trace_service.base_service,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s control operation is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation_p, out_buffer_pp);
    if (*out_buffer_pp == NULL)
    {
        // The buffer pointer is NULL!
        fbe_base_service_trace(&fbe_sector_trace_service.base_service,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer pointer is NULL\n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    };

    fbe_payload_control_get_buffer_length(control_operation_p, &buffer_length);
    if (buffer_length != in_buffer_length)
    {
        // The buffer length doesn't match!
        fbe_base_service_trace(&fbe_sector_trace_service.base_service,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer length mismatch\n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }



    return FBE_STATUS_OK;

}
/* end of fbe_sector_trace_validate_control_request() */

/*!***************************************************************************
 *          fbe_sector_trace_control_get_info()
 ***************************************************************************** 
 * 
 * @brief   The method returns the current trace configuration for the sector
 *          trace facility.
 *
 * @param   packet_p       - Pointer to the packet.
 *
 * @return  status - The status of the operation. 
 *
 * @author
 *  06/28/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_sector_trace_control_get_info(fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_sector_trace_get_info_t        *get_info_p = NULL;
    
    /* Get a pointer to the debug flags to populate
     */
    status = fbe_sector_trace_validate_control_request(packet_p, 
                                              sizeof(*get_info_p),
                                              (fbe_payload_control_buffer_t)&get_info_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);    
        return(status);
    }

    /* Now populate the payload
     */
    get_info_p->b_sector_trace_initialized = fbe_sector_trace_is_initialized();
    get_info_p->default_sector_trace_level = fbe_sector_trace_get_default_error_level();
    get_info_p->current_sector_trace_level = fbe_sector_trace_get_error_level();
    get_info_p->default_sector_trace_flags = fbe_sector_trace_get_default_error_flags();
    get_info_p->current_sector_trace_flags = fbe_sector_trace_get_error_flags();
    get_info_p->stop_on_error_flags = fbe_sector_trace_get_stop_on_error_flags();
    get_info_p->b_stop_on_error_flags_enabled = fbe_sector_trace_get_stop_on_error_enabled();

    /* Always return the execution status
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/* end of fbe_sector_trace_control_get_info() */

/*!***************************************************************************
 *          fbe_sector_trace_control_set_level()
 ***************************************************************************** 
 * 
 * @brief   The method sets the sector trace level (i.e. any error with an
 *          equal or lower error level will be traced) for the sector trace
 *          service.
 *
 * @param   packet_p       - Pointer to the packet.
 *
 * @return  status - The status of the operation. 
 *
 * @author
 *  06/28/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_sector_trace_control_set_level(fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_sector_trace_set_level_t       *set_level_p = NULL;
    
    /* Get a pointer to the debug flags to populate
     */
    status = fbe_sector_trace_validate_control_request(packet_p, 
                                              sizeof(*set_level_p),
                                              (fbe_payload_control_buffer_t)&set_level_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);    
        return(status);
    }

    /* Now set the trace level from the payload
     */
    fbe_sector_trace_set_error_level(set_level_p->new_sector_trace_level,set_level_p->persist);

    /* Always return the execution status
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/* end of fbe_sector_trace_control_set_level() */
 
/*!***************************************************************************
 *          fbe_sector_trace_control_set_flags()
 ***************************************************************************** 
 * 
 * @brief   The method return the current trace configuration for the sector
 *          trace facility.
 *
 * @param   packet_p       - Pointer to the packet.
 *
 * @return  status - The status of the operation. 
 *
 * @author
 *  06/28/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_sector_trace_control_set_flags(fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_sector_trace_set_flags_t       *set_flags_p = NULL;
    
    /* Get a pointer to the debug flags to populate
     */
    status = fbe_sector_trace_validate_control_request(packet_p, 
                                              sizeof(*set_flags_p),
                                              (fbe_payload_control_buffer_t)&set_flags_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);    
        return(status);
    }

    /* Now set the trace flags from the payload
     */
    fbe_sector_trace_set_error_flags(set_flags_p->new_sector_trace_flags,set_flags_p->persist);

    /* Always return the execution status
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/* end of fbe_sector_trace_control_set_flags() */

/****************************************************************
 *  fbe_sector_trace_set_stop_system_on_error
 ****************************************************************
 * @brief
 *      set actions on error trace  
 *  
 * @param
 *      stop_on_error - Trace error type to set for error trace buffer.
 *      
 *
 * @return
 *      None.
 *
 * @author
 *  11/09/2011 :nchiu - Created.
 *
 ****************************************************************/
static void fbe_sector_trace_set_stop_system_on_error(fbe_bool_t stop_on_error)
{

    fbe_sector_trace_service.sector_trace_info.sector_trace_stop_on_error = stop_on_error;

    return;
} /* fbe_sector_trace_set_stop_system_on_error */

/*!**************************************************************
 * fbe_sector_trace_read_addtional_event_from_registry()
 ****************************************************************
 * @brief
 *  Read this  flag from the registry.
 *
 * @param flag - Flag to read in registry.
 *
 * @return None.
 *
 * @author
 *  09/02/2015 - Created. Huibing Xiao
 *
 ****************************************************************/

static fbe_status_t fbe_sector_trace_read_addtional_event_from_registry(fbe_u32_t* flag_p )
{
    fbe_status_t status;
    fbe_u32_t flags;
    fbe_u32_t default_input_value = 0;
    *flag_p = 0;
    status = fbe_registry_read(NULL,
                               SEP_REG_PATH,
                               FBE_SECTOR_TRACE_STOP_ON_ADDITIONAL_EVENT,
                               &flags,
                               sizeof(fbe_u32_t),
                               FBE_REGISTRY_VALUE_DWORD,
                               &default_input_value,
                               sizeof(fbe_u32_t),
                               FALSE);
    if (status != FBE_STATUS_OK)
    {        
	    fbe_base_service_trace(&fbe_sector_trace_service.base_service,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s failed to read %s from registry 0x%x\n", __FUNCTION__,FBE_SECTOR_TRACE_STOP_ON_ADDITIONAL_EVENT, flags);
        return FBE_STATUS_GENERIC_FAILURE;        
    }
	*flag_p = flags;
    return status;
}
/******************************************
 * end fbe_sector_trace_read_ssoef_flag_from_registry()
 ******************************************/

/*!**************************************************************
 * fbe_sector_trace_persist_addtional_event_to_registry()
 ****************************************************************
 * @brief
 *  Write this  flag to the registry.
 *
 * @param flag - Flag to set in registry.
 *
 * @return None.
 *
 * @author
 *  09/02/2015 - Created. Huibing Xiao
 *
 ****************************************************************/
static fbe_status_t fbe_sector_trace_persist_addtional_event_to_registry(fbe_u32_t additional_event)
{
    fbe_status_t status;

    status = fbe_registry_write(NULL,
                                SEP_REG_PATH,
                                FBE_SECTOR_TRACE_STOP_ON_ADDITIONAL_EVENT,
                                FBE_REGISTRY_VALUE_DWORD,
                                &additional_event, 
                                sizeof(fbe_u32_t));

    if (status != FBE_STATUS_OK)
    {
    
	    fbe_base_service_trace(&fbe_sector_trace_service.base_service,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s failed writing %s to 0x%x\n", __FUNCTION__,FBE_SECTOR_TRACE_STOP_ON_ADDITIONAL_EVENT,additional_event );
        return FBE_STATUS_GENERIC_FAILURE;        
    }

    return status;
}
/******************************************
 * end fbe_sector_trace_persist_ssoef_flag_to_registry()
 ******************************************/

/*!**************************************************************
 * fbe_sector_trace_read_ssoef_flag_from_registry()
 ****************************************************************
 * @brief
 *  Read this  flag from the registry.
 *
 * @param flag - Flag to read in registry.
 *
 * @return None.
 *
 * @author
 *  09/02/2015 - Created. Huibing Xiao
 *
 ****************************************************************/

static fbe_status_t fbe_sector_trace_read_ssoef_flag_from_registry(fbe_sector_trace_error_flags_t* flag_p )
{
    fbe_status_t status;
    fbe_sector_trace_error_flags_t flags;
    fbe_u32_t default_input_value = FBE_SECTOR_TRACE_ERROR_FLAG_NONE;
    *flag_p = FBE_SECTOR_TRACE_ERROR_FLAG_NONE;
    status = fbe_registry_read(NULL,
                               SEP_REG_PATH,
                               FBE_SECTOR_TRACE_STOP_ON_ERROR_FLAGS,
                               &flags,
                               sizeof(fbe_u32_t),
                               FBE_REGISTRY_VALUE_DWORD,
                               &default_input_value,
                               sizeof(fbe_u32_t),
                               FALSE);
    if (status != FBE_STATUS_OK)
    {        
	    fbe_base_service_trace(&fbe_sector_trace_service.base_service,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s failed to read %s from registry 0x%x\n", __FUNCTION__,FBE_SECTOR_TRACE_STOP_ON_ERROR_FLAGS, flags);
        return FBE_STATUS_GENERIC_FAILURE;        
    }
	*flag_p = flags;
    return status;
}
/******************************************
 * end fbe_sector_trace_read_ssoef_flag_from_registry()
 ******************************************/

/*!**************************************************************
 * fbe_sector_trace_persist_ssoef_flag_to_registry()
 ****************************************************************
 * @brief
 *  Write this  flag to the registry.
 *
 * @param flag - Flag to set in registry.
 *
 * @return None.
 *
 * @author
 *  09/02/2015 - Created. Huibing Xiao
 *
 ****************************************************************/

static fbe_status_t fbe_sector_trace_persist_ssoef_flag_to_registry(fbe_sector_trace_error_flags_t flag)
{
    fbe_status_t status;

    status = fbe_registry_write(NULL,
                                SEP_REG_PATH,
                                FBE_SECTOR_TRACE_STOP_ON_ERROR_FLAGS,
                                FBE_REGISTRY_VALUE_DWORD,
                                &flag, 
                                sizeof(fbe_u32_t));

    if (status != FBE_STATUS_OK)
    {
    
	    fbe_base_service_trace(&fbe_sector_trace_service.base_service,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s failed writing %s to 0x%x\n", __FUNCTION__,FBE_SECTOR_TRACE_STOP_ON_ERROR_FLAGS,flag );
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    return status;
}
/******************************************
 * end fbe_sector_trace_persist_ssoef_flag_to_registry()
 ******************************************/

/****************************************************************
 *  fbe_sector_trace_set_trace_debug_flags
 ****************************************************************
 * @brief
 *      Overrides the current sector tracing error type value if the
 *      passed value is sane.  
 *  
 * @param
 *      sector_err_flag - Trace error type to set for error trace buffer.
 *      
 *
 * @return
 *      None.
 *
 * @author
 *  11/09/2011 :nchiu - Created.
 * 09/02/2015:xiaoh2 modified to add persistence
 *
 ****************************************************************/
static void fbe_sector_trace_set_stop_on_error_flags(fbe_sector_trace_error_flags_t sector_err_flag,fbe_u32_t additional_event,fbe_bool_t persist)
{

    fbe_sector_trace_service.sector_trace_info.sector_trace_stop_on_error_flag = sector_err_flag;
    stop_on_additional_raid_event = additional_event;
    if( persist == FBE_TRUE ){
        fbe_sector_trace_persist_ssoef_flag_to_registry(sector_err_flag);
		fbe_sector_trace_persist_addtional_event_to_registry(additional_event);
    }
    return;
} /* fbe_sector_trace_set_trace_debug_flags */

/****************************************************************
 *  fbe_sector_trace_set_encryption_mode
 ****************************************************************
 * @brief
 *      set encryption mode
 *  
 * @param
 *      encryption_mode
 *      
 *
 * @return
 *      None.
 *
 * @author
 *  2/03/2014 mferson - Created.
 *
 ****************************************************************/
static void fbe_sector_trace_set_encryption_mode(fbe_system_encryption_mode_t encryption_mode)
{

    fbe_sector_trace_service.sector_trace_info.encryption_mode = encryption_mode;

    return;
} /* fbe_sector_trace_set_encryption_mode */


/****************************************************************
 *  fbe_sector_trace_get_external_build()
 ****************************************************************
 * @brief
 *      get retail build
 *  
 * @param   None
 *      
 *
 * @return  bool
 *
 * @author
 *  2/03/2014 mferson - Created.
 *
 ****************************************************************/
static fbe_bool_t fbe_sector_trace_get_external_build(void)
{
    return fbe_sector_trace_service.sector_trace_info.b_is_external_build;
} /* fbe_sector_trace_get_external_build */

/*!***************************************************************************
 *          fbe_sector_trace_control_set_stop_on_error_flags()
 ***************************************************************************** 
 * 
 * @brief   The method sets debug flags for the sector
 *          trace facility.
 *
 * @param   packet_p       - Pointer to the packet.
 *
 * @return  status - The status of the operation. 
 *
 * @author
 *  11/09/2011  nchiu  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_sector_trace_control_set_stop_on_error_flags(fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_sector_trace_set_stop_on_error_flags_t *set_flags_p = NULL;
    
    /* Get a pointer to the debug flags to populate
     */
    status = fbe_sector_trace_validate_control_request(packet_p, 
                                              sizeof(*set_flags_p),
                                              (fbe_payload_control_buffer_t)&set_flags_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);    
        return(status);
    }

    /* Now set the trace flags from the payload
     */
    fbe_sector_trace_set_stop_on_error_flags(set_flags_p->new_sector_trace_stop_on_error_flags,set_flags_p->additional_event,set_flags_p->persist);

    /* Always return the execution status
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/* end of fbe_sector_trace_control_set_stop_on_error_flags() */

/*!***************************************************************************
 *          fbe_sector_trace_control_set_stop_on_error()
 ***************************************************************************** 
 * 
 * @brief   The method sets action for the sector
 *          trace facility.
 *
 * @param   packet_p       - Pointer to the packet.
 *
 * @return  status - The status of the operation. 
 *
 * @author
 *  11/09/2011  nchiu  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_sector_trace_control_set_stop_on_error(fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_sector_trace_stop_system_on_error_t *set_flags_p = NULL;
    
    /* Get a pointer to the debug flags to populate
     */
    status = fbe_sector_trace_validate_control_request(packet_p, 
                                              sizeof(*set_flags_p),
                                              (fbe_payload_control_buffer_t)&set_flags_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);    
        return(status);
    }

    /* Now set the trace flags from the payload
     */
    fbe_sector_trace_set_stop_system_on_error(set_flags_p->b_stop_system);

    /* Always return the execution status
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/* end of fbe_sector_trace_control_set_stop_on_error() */

/*!***************************************************************************
 *          fbe_sector_trace_reset_counters()
 ***************************************************************************** 
 * 
 * @brief   The method resets the sector tracing counters to 0
 *
 * @param   packet_p       - Pointer to the packet.
 *
 * @return  None
 *
 *****************************************************************************/
static void fbe_sector_trace_reset_counters(void)
{
    fbe_sector_trace_info_t    *sector_trace_info_p = fbe_sector_trace_get_info();
    fbe_u32_t                   index;

    /* Simply zero the associated counters
     */
    sector_trace_info_p->total_invocations = 0;
    sector_trace_info_p->total_errors_traced = 0;
    sector_trace_info_p->total_sectors_traced = 0;
    for (index = 0; index < FBE_SECTOR_TRACE_ERROR_LEVEL_COUNT; index++)
    {
        sector_trace_info_p->error_level_counters[index] = 0;
    }
    for (index = 0; index < FBE_SECTOR_TRACE_ERROR_FLAG_COUNT; index++)
    {
        sector_trace_info_p->error_type_counters[index] = 0;
    }

    return;
}
/* end of fbe_sector_trace_reset_counters() */

/*!***************************************************************************
 *          fbe_sector_trace_control_reset_counters()
 ***************************************************************************** 
 * 
 * @brief   The method resets the sector tracing counters to 0
 *
 * @param   packet_p       - Pointer to the packet.
 *
 * @return  status - The status of the operation. 
 *
 * @author
 *  06/30/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_sector_trace_control_reset_counters(fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_sector_trace_reset_counters_t  *reset_counters_p = NULL;
    
    /* Get a pointer to the debug flags to populate
     */
    status = fbe_sector_trace_validate_control_request(packet_p, 
                                              sizeof(*reset_counters_p),
                                              (fbe_payload_control_buffer_t)&reset_counters_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);    
        return(status);
    }

    /* Invoke methods to reset the counters
     */
    fbe_sector_trace_reset_counters();

    /* Always return the execution status
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/* end of fbe_sector_trace_control_reset_counters() */

/*!***************************************************************************
 *          fbe_sector_trace_get_counters()
 ***************************************************************************** 
 * 
 * @brief   The method returns the current trace counters for the sector
 *          trace facility.
 *
 * @param   get_counters_p - Pointer to destination counters to populate
 *
 * @return  status - The status of the operation. 
 *
 *****************************************************************************/
static fbe_status_t fbe_sector_trace_get_counters(fbe_sector_trace_get_counters_t *get_counters_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_sector_trace_info_t    *sector_trace_info_p = fbe_sector_trace_get_info();
    fbe_u32_t                   index;

    /* Populate the requested buffer
     */
    get_counters_p->total_invocations = sector_trace_info_p->total_invocations;
    get_counters_p->total_errors_traced = sector_trace_info_p->total_errors_traced;
    get_counters_p->total_sectors_traced = sector_trace_info_p->total_sectors_traced;

    /* Populate the error level counters
     */
    for (index = 0; index < FBE_SECTOR_TRACE_ERROR_LEVEL_COUNT; index++)
    {
        get_counters_p->error_level_counters[index] = sector_trace_info_p->error_level_counters[index];
    }

    /* Populate the error type counters
     */
    for (index = 0; index < FBE_SECTOR_TRACE_ERROR_FLAG_COUNT; index++)
    {
        get_counters_p->error_type_counters[index] = sector_trace_info_p->error_type_counters[index];
    } 
    return(status);
}
/* end of fbe_sector_trace_get_counters() */

/*!***************************************************************************
 *          fbe_sector_trace_control_get_counters()
 ***************************************************************************** 
 * 
 * @brief   The method returns the current trace counters for the sector
 *          trace facility.
 *
 * @param   packet_p       - Pointer to the packet.
 *
 * @return  status - The status of the operation. 
 *
 * @author
 *  06/30/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_sector_trace_control_get_counters(fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_sector_trace_get_counters_t    *get_counters_p = NULL;
    
    /* Get a pointer to the debug flags to populate
     */
    status = fbe_sector_trace_validate_control_request(packet_p, 
                                              sizeof(*get_counters_p),
                                              (fbe_payload_control_buffer_t)&get_counters_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);    
        return(status);
    }

    /* Now populate the payload
     */
    status = fbe_sector_trace_get_counters(get_counters_p);

    /* Always return the execution status
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/* end of fbe_sector_trace_control_get_counters() */


/*!***************************************************************************
 *          fbe_sector_trace_control_set_encryption_mode()
 ***************************************************************************** 
 * 
 * @brief   The method sets action for the sector
 *          trace facility.
 *
 * @param   packet_p       - Pointer to the packet.
 *
 * @return  status - The status of the operation. 
 *
 * @author
 *  2/03/2014  mferson  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_sector_trace_control_set_encryption_mode(fbe_packet_t *packet_p)
{
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_sector_trace_encryption_mode_t * set_mode_p = NULL;
    
    /* Get a pointer to the debug flags to populate
     */
    status = fbe_sector_trace_validate_control_request(packet_p, 
                                              sizeof(*set_mode_p),
                                              (fbe_payload_control_buffer_t)&set_mode_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);    
        return(status);
    }

    /* Now set the trace flags from the payload
     */
    fbe_sector_trace_set_encryption_mode(set_mode_p->encryption_mode);
    fbe_sector_trace_set_external_build(set_mode_p->b_is_external_build);

    /* Always return the execution status
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/* end of fbe_sector_trace_control_set_stop_on_error() */

/*!***************************************************************************
 *          fbe_sector_trace_is_external_build()
 ***************************************************************************** 
 * 
 * @brief   Simply returns if this is a retail build or not.
 *
 * @param   None
 *
 * @return  status - The status of the operation. 
 *
 * @author
 *  07/01/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_bool_t fbe_sector_trace_is_external_build(void)
{
    return (fbe_sector_trace_get_external_build());
}
/* end fbe_sector_trace_is_external_build() */

/*!***************************************************************************
 *          fbe_sector_trace_control_entry()
 *****************************************************************************
 * 
 * @brief   The sector trace service control entry
 *  
 * @param   packet  - Pointer to fbe packet with control request      
 *
 * @return  status  - Typically FBE_STATUS_OK
 *
 * @author
 *  06/28/2010  Ron Proulx  - Created
 * 
 *****************************************************************************/
fbe_status_t fbe_sector_trace_control_entry(fbe_packet_t * packet)
{    
    fbe_payload_control_operation_opcode_t control_code;
    fbe_status_t    status = FBE_STATUS_OK;

    control_code = fbe_transport_get_control_code(packet);
    if (control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        status = fbe_sector_trace_init(packet);
        return(status);
    }

    if (fbe_base_service_is_initialized(&fbe_sector_trace_service.base_service) == FALSE) {
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return(FBE_STATUS_NOT_INITIALIZED);
    }

    switch(control_code) {
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = fbe_sector_trace_destroy(packet);
            break;
        case FBE_SECTOR_TRACE_CONTROL_CODE_GET_INFO:
            status = fbe_sector_trace_control_get_info(packet);
            break;
        case FBE_SECTOR_TRACE_CONTROL_CODE_SET_LEVEL:
            status = fbe_sector_trace_control_set_level(packet);
            break;
        case FBE_SECTOR_TRACE_CONTROL_CODE_SET_FLAGS:
            status = fbe_sector_trace_control_set_flags(packet);
            break;
        case FBE_SECTOR_TRACE_CONTROL_CODE_RESET_COUNTERS:
            status = fbe_sector_trace_control_reset_counters(packet);
            break;
        case FBE_SECTOR_TRACE_CONTROL_CODE_GET_COUNTERS:
            status = fbe_sector_trace_control_get_counters(packet);
            break;
        case FBE_SECTOR_TRACE_CONTROL_CODE_SET_STOP_ON_ERROR_FLAGS:
            status = fbe_sector_trace_control_set_stop_on_error_flags(packet);
            break;
        case FBE_SECTOR_TRACE_CONTROL_CODE_STOP_SYSTEM_ON_ERROR:
            status = fbe_sector_trace_control_set_stop_on_error(packet);
            break;
        case FBE_SECTOR_TRACE_CONTROL_CODE_SET_ENCRYPTION_MODE:
            status = fbe_sector_trace_control_set_encryption_mode(packet);
            break;
        default:
            fbe_base_service_trace(&fbe_sector_trace_service.base_service,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s, unknown trace control code: %X\n", __FUNCTION__, control_code);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return status;
}
/* end of fbe_sector_trace_control_entry() */
/*!**************************************************************
 * fbe_sector_trace_can_trace_data()
 ****************************************************************
 * @brief
 *  Determine if we are allowed to trace data or not.
 *
 * @param None      
 *
 * @return FBE_TRUE - Yes we are allowed to trace data.
 *        FBE_FALSE - No we are not allowed to trace data. 
 *
 * @author
 *  3/14/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_sector_trace_can_trace_data(void)
{
    /* Do not trace data if encryption is enabled.
     */
    if (fbe_sector_trace_service.sector_trace_info.encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED){
        return FBE_FALSE;
    }
    return FBE_TRUE;
}
/******************************************
 * end fbe_sector_trace_can_trace_data()
 ******************************************/
/***************************************************************************
 * END fbe_sector_trace.c
 ***************************************************************************/
