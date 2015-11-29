#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_enclosure.h"
#include "fbe_scheduler.h"
#include "fbe_diplex.h"

#include "base_enclosure_private.h"
#include "edal_base_enclosure_data.h"
#include "fbe_enclosure_data_access_private.h"


#define ELIMINATE_

/* Class methods forward declaration */
fbe_status_t base_enclosure_load(void);
fbe_status_t base_enclosure_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_base_enclosure_class_methods = {FBE_CLASS_ID_BASE_ENCLOSURE,
                                                    base_enclosure_load,
                                                    base_enclosure_unload,
                                                    fbe_base_enclosure_create_object,
                                                    fbe_base_enclosure_destroy_object,
                                                    fbe_base_enclosure_control_entry,
                                                    fbe_base_enclosure_event_entry,
                                                    NULL,
                                                    fbe_base_enclosure_monitor_entry};


/* Forward declaration */
static fbe_status_t base_enclosure_mgmt_get_enclosure_number(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet);
static fbe_status_t fbe_base_enclosure_get_data_change_info(fbe_base_enclosure_t * pBaseEncl,
                                fbe_enclosure_component_types_t componentType,
                                fbe_u32_t componentIndex,
                                fbe_notification_data_changed_info_t * pDataChangeInfo);

static void
fbe_base_enclosure_allocate_completion(fbe_memory_request_t * memory_request, 
                                  fbe_memory_completion_context_t context);

fbe_status_t 
base_enclosure_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_BASE_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    return fbe_base_enclosure_monitor_load_verify();
}

fbe_status_t 
base_enclosure_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_BASE_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_create_object()
 ****************************************************************
 * DESCRIPTION:
 *   This is the constructor function for this class.  
 *
 * PARAMETERS:
 *  object_handle - our object context
 *  packet - request packet
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  08/04/08 - add history. NCHIU
 *
 ****************************************************************/
fbe_status_t 
fbe_base_enclosure_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_base_enclosure_t * base_enclosure;
    fbe_status_t status;
    fbe_trace_level_t trace_level;

    fbe_topology_class_trace(FBE_CLASS_ID_BASE_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    /* Call parent constructor */
    status = fbe_base_discovering_create_object(packet, object_handle);
    if(status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id */
    base_enclosure = (fbe_base_enclosure_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_class_id((fbe_base_object_t *) base_enclosure, FBE_CLASS_ID_BASE_ENCLOSURE);    

    /* Initialize internal data structure */
    fbe_base_enclosure_init(base_enclosure);

    /* Set physical_object_level */
    fbe_base_object_set_physical_object_level((fbe_base_object_t *)base_enclosure, FBE_PHYSICAL_OBJECT_LEVEL_FIRST_ENCLOSURE);

    // Set trace level in EDAL
    trace_level = fbe_base_object_get_trace_level((fbe_base_object_t *)base_enclosure);
    fbe_edal_setTraceLevel(trace_level);

    return FBE_STATUS_OK;
}



/*!****************************************************************************
 * @fn      fbe_base_enclosure_destroy_object( fbe_object_handle_t object_handle)
 *
 * @brief
 *  This function destroys the object associated with the object_handle.
 *
 * @param object_handle - handle of object to destroy.
 *
 * @return
 * fbe_status_t - status returned from fbe_base_discovering_destroy_object().
 *
 * HISTORY
 *  05/07/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
fbe_status_t 
fbe_base_enclosure_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;

    fbe_topology_class_trace(FBE_CLASS_ID_BASE_ENCLOSURE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    /* Step 1. Check parent edges */
    /* Step 2. Cleanup */
    /* Step 3. Call parent destructor */
    status = fbe_base_discovering_destroy_object(object_handle);
    return status;
}




/*!****************************************************************************
 * @fn      fbe_base_enclosure_set_my_position()
 *
 * @brief
 *  This function 
 *
 * @param base_enclosure - pointer to object in which to set my_position.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY
 *  05/07/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
fbe_status_t 
fbe_base_enclosure_set_my_position(fbe_base_enclosure_t * base_enclosure, fbe_enclosure_number_t enclosure_number)
{
    base_enclosure->my_position = enclosure_number;
    return FBE_STATUS_OK;
}




/*!****************************************************************************
 * @fn      fbe_base_enclosure_set_logheader(fbe_base_enclosure_t * base_enclosure)
 *
 * @brief
 * This function inits the logheader to be specific to the passed
 * enclosure.  Subsequent log messages will contain this header string
 * to help identify which enclosure the message belongs too.
 *
 * @param base_enclosure - pointer to object from which to init log header.
 *
 * @return 
 * fbe_status_t FBE_STATUS_OK - no error.
 *
 * HISTORY
 *  05/07/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
fbe_status_t 
fbe_base_enclosure_set_logheader(fbe_base_enclosure_t * base_enclosure)
{
    if (base_enclosure->component_id)
    {
        csx_p_snprintf(base_enclosure->logHeader, FBE_BASE_ENCL_HEADER, "Enc:%d_%d.%d",
                        base_enclosure->port_number, base_enclosure->enclosure_number, base_enclosure->component_id);
    }
    else
    {
    csx_p_snprintf(base_enclosure->logHeader, FBE_BASE_ENCL_HEADER, "Enc:%d_%d",
                       base_enclosure->port_number, base_enclosure->enclosure_number);
    }
    return FBE_STATUS_OK;
}




/*!****************************************************************************
 * @fn      fbe_base_enclosure_get_logheader()
 *
 * @brief
 *  This function returns the logHeader from the base enclosure.
 *
 * @param base_enclosure - pointer to object from which to extract log header.
 *
 * @return logHeader pointer.
 *
 * HISTORY
 *  05/07/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
fbe_u8_t * 
fbe_base_enclosure_get_logheader(fbe_base_enclosure_t * base_enclosure)
{
    return (base_enclosure->logHeader);
}
 


/*!****************************************************************************
 * @fn      fbe_base_enclosure_get_enclosure_number()
 *
 * @brief
 *  This function returns the enclosure number from the base enclosure.
 *
 * @param base_enclosure - points to object from which to extract enclosure number.
 * @param enclosure_number - points to location where to write the enclosure number.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY
 *  05/07/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
fbe_status_t
fbe_base_enclosure_get_enclosure_number(fbe_base_enclosure_t * base_enclosure, 
                                        fbe_enclosure_number_t * enclosure_number)
{
    *enclosure_number = base_enclosure->enclosure_number;
    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_set_port_number()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function sets port_number.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  port_number - port_number
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  09/12/08 - Created. NCHIU
 *
 ****************************************************************/
fbe_status_t 
fbe_base_enclosure_set_port_number(fbe_base_enclosure_t * base_enclosure, 
                                   fbe_port_number_t port_number)
{
    base_enclosure->port_number = port_number;
    return FBE_STATUS_OK;
}


/*!****************************************************************************
 * @fn      fbe_base_enclosure_get_component_id(fbe_base_enclosure_t * base_enclosure)
 *
 * @brief
 *  This function returns the component_id from the base enclosure.
 *
 * @param base_enclosure - points to object from which to extract component id.
 *
 * @return
 *  component_id.
 *
 * HISTORY
 *  05/07/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
fbe_enclosure_component_id_t
fbe_base_enclosure_get_component_id(fbe_base_enclosure_t * base_enclosure)
{
    return (base_enclosure->component_id);
}


/****************************************************************
 * fbe_base_enclosure_get_port_number()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function returns port_number.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  port_number - port_number
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  09/12/08 - Created. NCHIU
 *
 ****************************************************************/
fbe_status_t
fbe_base_enclosure_get_port_number(fbe_base_enclosure_t * base_enclosure, 
                                   fbe_port_number_t * port_number)
{
    *port_number = base_enclosure->port_number;
    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_set_activeEdalValid()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function sets active_edal_valid.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  active_edal_valid - active_edal_valid
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  06-Jan-2010 - Created. Dipak Patel
 *
 ****************************************************************/
fbe_status_t 
fbe_base_enclosure_set_activeEdalValid(fbe_base_enclosure_t * base_enclosure, 
                                   fbe_bool_t active_edal_valid)
{
    base_enclosure->active_edal_valid = active_edal_valid;
    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_get_activeEdalValid()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function returns active_edal_valid.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  active_edal_valid - active_edal_valid
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  06-Jan-2010 - Created. Dipak Patel
 *
 ****************************************************************/
fbe_status_t
fbe_base_enclosure_get_activeEdalValid(fbe_base_enclosure_t * base_enclosure, 
                                   fbe_bool_t * active_edal_valid)
{
    *active_edal_valid = base_enclosure->active_edal_valid;
    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_set_server_address()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function sets server_address.  
 *  Note the server address is a union of different types.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  server_address - server_address
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  09/12/08 - Created. NCHIU
 *
 ****************************************************************/
fbe_status_t 
fbe_base_enclosure_set_server_address(fbe_base_enclosure_t * base_enclosure, 
                                      fbe_address_t server_address)
{
    base_enclosure->server_address = server_address;
    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_get_server_address()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function returns server_address.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  server_address - server_address
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  09/12/08 - Created. NCHIU
 *
 ****************************************************************/
fbe_status_t
fbe_base_enclosure_get_server_address(fbe_base_enclosure_t * base_enclosure, 
                                      fbe_address_t * server_address)
{
    *server_address = base_enclosure->server_address;
    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_set_default_ride_through_period()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function sets default_ride_through_period.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  default_ride_through_period - ride through time in seconds
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  08/04/08 - Created. NCHIU
 *
 ****************************************************************/
fbe_status_t 
fbe_base_enclosure_set_default_ride_through_period(fbe_base_enclosure_t * base_enclosure, 
                                                   fbe_u32_t default_ride_through_period)
{
    base_enclosure->default_ride_through_period = default_ride_through_period;
    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_get_default_ride_through_period()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function returns default_ride_through_period.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  default_ride_through_period - ride through time in seconds
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  08/04/08 - Created. NCHIU
 *
 ****************************************************************/
fbe_status_t
fbe_base_enclosure_get_default_ride_through_period(fbe_base_enclosure_t * base_enclosure, 
                                                   fbe_u32_t * default_ride_through_period)
{
    *default_ride_through_period = base_enclosure->default_ride_through_period;
    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_set_expected_ride_through_period()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function sets expected_ride_through_period.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  default_ride_through_period - ride through time in seconds
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  08/04/08 - Created. NCHIU
 *
 ****************************************************************/
fbe_status_t 
fbe_base_enclosure_set_expected_ride_through_period(fbe_base_enclosure_t * base_enclosure, 
                                                   fbe_u32_t expected_ride_through_period)
{
    base_enclosure->expected_ride_through_period = expected_ride_through_period;
    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_get_expected_ride_through_period()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function returns expected_ride_through_period.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  default_ride_through_period - ride through time in seconds
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  08/04/08 - Created. NCHIU
 *
 ****************************************************************/
fbe_status_t
fbe_base_enclosure_get_expected_ride_through_period(fbe_base_enclosure_t * base_enclosure, 
                                                   fbe_u32_t * expected_ride_through_period)
{
    *expected_ride_through_period = base_enclosure->expected_ride_through_period;
    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_get_time_ride_through_start()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function returns time_ride_through_start.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  time_ride_through_start - ride through start time 
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  08/12/08 - Created. NCHIU
 *
 ****************************************************************/
fbe_status_t
fbe_base_enclosure_get_time_ride_through_start(fbe_base_enclosure_t * base_enclosure, 
                                               fbe_time_t * time_ride_through_start)
{
    *time_ride_through_start = base_enclosure->time_ride_through_start;
    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_set_time_ride_through_start()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function sets time_ride_through_start.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  time_ride_through_start - ride through start time 
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  08/12/08 - Created. NCHIU
 *
 ****************************************************************/
fbe_status_t 
fbe_base_enclosure_set_time_ride_through_start(fbe_base_enclosure_t * base_enclosure, 
                                               fbe_time_t time_ride_through_start)
{
    base_enclosure->time_ride_through_start = time_ride_through_start;
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_base_enclosure_increment_ride_through_count()
 ****************************************************************
 * @brief
 *  This function increments the ride through count.
 *
 * @param  base_enclosure - our object context
 *
 * @return  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  08-Jan-2009 PHE - Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_base_enclosure_increment_ride_through_count(fbe_base_enclosure_t * base_enclosure)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               ride_through_count;

    // get rideThroughCount
    status = fbe_base_enclosure_get_ride_through_count(base_enclosure, &ride_through_count);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    // increment rideThroughCount & save
    ride_through_count ++;
    status = fbe_base_enclosure_set_ride_through_count(base_enclosure, ride_through_count);

    return status;
}

/*!***************************************************************
 * @fn fbe_base_enclosure_get_ride_through_count()
 ****************************************************************
 * @brief
 *  This function gets the ride through count.
 *
 * @param  base_enclosure - our object context.
 * @param  ride_through_count - The pointer to the ride through count.
 *
 * @return  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  08-Jan-2009 PHE - Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_base_enclosure_get_ride_through_count(fbe_base_enclosure_t * base_enclosure, fbe_u32_t * ride_through_count)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_enclosure_status_t  enclStatus;

    enclStatus = fbe_base_enclosure_edal_getU32(base_enclosure,
                                                FBE_ENCL_RESET_RIDE_THRU_COUNT,
                                                FBE_ENCL_ENCLOSURE,
                                                0,
                                                ride_through_count);
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_base_enclosure_set_ride_through_count()
 ****************************************************************
 * @brief
 *  This function sets the ride through count.
 *
 * @param  base_enclosure - our object context.
 * @param  ride_through_count - The ride through count to be set to
 *
 * @return  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  08-Jan-2009 PHE - Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_base_enclosure_set_ride_through_count(fbe_base_enclosure_t * base_enclosure, fbe_u32_t ride_through_count)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_enclosure_status_t  enclStatus;

    enclStatus = fbe_base_enclosure_edal_setU32(base_enclosure,
                                                FBE_ENCL_RESET_RIDE_THRU_COUNT,
                                                FBE_ENCL_ENCLOSURE,
                                                0,
                                                ride_through_count);
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!***************************************************************
 * @fn fbe_base_enclosure_increment_condition_retry_count()
 ****************************************************************
 * @brief
 *  This function increments the condition_retry_count.
 *
 * @param  base_enclosure - our object context
 *
 * @return  current retry count.
 *
 * HISTORY:
 *  27-Jul-2009 NC - Created.
 *
 ****************************************************************/
fbe_u32_t 
fbe_base_enclosure_increment_condition_retry_count(fbe_base_enclosure_t * base_enclosure)
{
    base_enclosure->condition_retry_count ++;

    return (base_enclosure->condition_retry_count);
}

/*!***************************************************************
 * @fn fbe_base_enclosure_clear_condition_retry_count()
 ****************************************************************
 * @brief
 *  This function sets the condition retry count to 0.
 *
 * @param  base_enclosure - our object context.
 *
 * @return  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  27-Jul-2009 NC - Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_base_enclosure_clear_condition_retry_count(fbe_base_enclosure_t * base_enclosure)
{
    base_enclosure->condition_retry_count = 0;

    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_set_component_block_ptr()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function sets enclosureComponentTypeBlock.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  encl_component_block - enclosureComponentTypeBlock pointer
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  08/29/08 - Created. NCHIU
 *
 ****************************************************************/
fbe_status_t fbe_base_enclosure_set_component_block_ptr(fbe_base_enclosure_t * base_enclosure, 
                                                        fbe_enclosure_component_block_t *encl_component_block)
{    
    base_enclosure->enclosureComponentTypeBlock = encl_component_block;
    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_set_component_block_backup_ptr()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function sets enclosureComponentTypeBlock_backup.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  encl_component_block - enclosureComponentTypeBlock pointer
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  06-Jan-2010 - Created. Dipak Patel
 *
 ****************************************************************/
fbe_status_t fbe_base_enclosure_set_component_block_backup_ptr(fbe_base_enclosure_t * base_enclosure, 
                                                        fbe_enclosure_component_block_t *encl_component_block)
{    
    base_enclosure->enclosureComponentTypeBlock_backup = encl_component_block;
    return FBE_STATUS_OK;
}
/****************************************************************
 * fbe_base_enclosure_get_component_block_ptr()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function gets enclosureComponentTypeBlock.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  encl_component_block - pointer to enclosureComponentTypeBlock pointer
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  08/29/08 - Created. NCHIU
 *
 ****************************************************************/
fbe_status_t fbe_base_enclosure_get_component_block_ptr(fbe_base_enclosure_t * base_enclosure, 
                                                        fbe_enclosure_component_block_t **encl_component_block)
{    
    *encl_component_block = base_enclosure->enclosureComponentTypeBlock;
    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_get_component_block_backup_ptr()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function gets enclosureComponentTypeBlock_backup.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  encl_component_block - pointer to enclosureComponentTypeBlock pointer
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  06-Jan-2010 - Created. Dipak Patel
 *
 ****************************************************************/
fbe_status_t fbe_base_enclosure_get_component_block_backup_ptr(fbe_base_enclosure_t * base_enclosure, 
                                                        fbe_enclosure_component_block_t **encl_component_block_backup)
{    
    *encl_component_block_backup = base_enclosure->enclosureComponentTypeBlock_backup;
    return FBE_STATUS_OK;
}
/****************************************************************
 * fbe_base_enclosure_set_lifecycle_set_cond_routine()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function sets lifecycle_set_condition_routine.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  lifecycle_set_cond_routine - fbe_base_enclosure_lifecycle_set_cond_function_t
 *
 * RETURNS:
 *  fbe_enclosure_status_t - FBE_ENCLOSUIRE_STATUS_OK - no error.
 *
 * HISTORY:
 *  04/24/09 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_enclosure_status_t fbe_base_enclosure_set_lifecycle_set_cond_routine(fbe_base_enclosure_t * base_enclosure, 
                                     fbe_base_enclosure_lifecycle_set_cond_function_t lifecycle_set_cond_routine)
{
    base_enclosure->lifecycle_set_condition_routine = lifecycle_set_cond_routine;
    return FBE_ENCLOSURE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_set_lifecycle_cond()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function calls lifecycle_set_condition_routine.
 *
 * PARAMETERS:
 *  p_class_const - fbe_lifecycle_const_t
 *  p_object - our object context
 *  new_state - lifecycle state to be set
 *
 * RETURNS:
 *  fbe_enclosure_status_t - FBE_ENCLOSURE_STATUS_OK - no error.
 *
 * HISTORY:
 *  04/24/09 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_enclosure_status_t fbe_base_enclosure_set_lifecycle_cond(fbe_lifecycle_const_t * p_class_const,
                                                   fbe_base_enclosure_t * base_enclosure,
                                                   fbe_lifecycle_state_t new_state)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;

    status = base_enclosure->lifecycle_set_condition_routine(
                                                        p_class_const, 
                                                        (fbe_base_object_t*)base_enclosure,
                                                        new_state);
    if(status != FBE_STATUS_OK)
    {
        encl_status = FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
    }
    return encl_status;
}

/****************************************************************
 * fbe_base_enclosure_handle_errors()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function sets component type, component index, fault symptom,
 *  and additional status for faulted enclosure. This functions also fails the 
 *  enclosure by setting life cycle condition.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  component - fbe_enclosure_component_types_t
 *  index - fbe_u32_t
 *  fault_symptom - fbe_u8_t  
 *  addl_status - fbe_u8_t 
 * RETURNS:
 *  fbe_enclosure_status_t - FBE_ENCLOSURE_STATUS_OK - no error.
 *
 * HISTORY:
 *  04/27/09 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_enclosure_status_t fbe_base_enclosure_handle_errors(fbe_base_enclosure_t * base_enclosure, 
                                                    fbe_u32_t component,
                                                    fbe_u32_t index,
                                                    fbe_u32_t  fault_symptom,
                                                    fbe_u32_t addl_status)
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_enclosure_set_enclosure_fault_info(base_enclosure,
                                                component,
                                                index,
                                                fault_symptom,
                                                addl_status);
    
    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                FBE_TRACE_LEVEL_INFO,
                fbe_base_enclosure_get_logheader(base_enclosure),
                "%s, Faulty Enclosure. Fault Symptom: %s\n", 
                __FUNCTION__,
                fbe_base_enclosure_getFaultSymptomString(fault_symptom));

    fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)base_enclosure, 
                                FBE_BASE_ENCLOSURE_DEATH_REASON_FAULTY_COMPONENT);

    status = base_enclosure->lifecycle_set_condition_routine(
                                &fbe_base_enclosure_lifecycle_const, 
                                (fbe_base_object_t*)base_enclosure,
                                FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
    if(status != FBE_STATUS_OK)
    {     
        return FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
    }
    return encl_status;
}

/****************************************************************
 * fbe_base_enclosure_get_enclosure_fault_info()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function gets fbe_enclosure_fault_t information.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  encl_fault_info - fbe_enclosure_fault_t
 *
 * RETURNS:
 *  fbe_enclosure_status_t - FBE_ENCLOSURE_STATUS_OK - no error.
 *
 * HISTORY:
 *  04/27/09 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_enclosure_status_t fbe_base_enclosure_get_enclosure_fault_info(fbe_base_enclosure_t * base_enclosure,
                                                    fbe_enclosure_fault_t * encl_fault_info)
{
    *encl_fault_info = base_enclosure->enclosure_faults;
    return FBE_ENCLOSURE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_set_enclosure_fault_info()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function sets fbe_enclosure_fault_t information
 * without failing the enclosure.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  component - fbe_enclosure_component_types_t
 *  index - fbe_u32_t
 *  fault_symptom - fbe_u8_t  
 *  addl_status - fbe_u8_t 
 *
 * RETURNS:
 *  fbe_enclosure_status_t - FBE_ENCLOSURE_STATUS_OK - no error.
 *
 * HISTORY:
 *  07/03/09 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_enclosure_status_t fbe_base_enclosure_set_enclosure_fault_info(fbe_base_enclosure_t * base_enclosure,
                                                    fbe_u32_t component,
                                                    fbe_u32_t index,
                                                    fbe_u32_t fault_symptom,
                                                    fbe_u32_t addl_status)
{
    base_enclosure->enclosure_faults.faultyComponentType = component;
    base_enclosure->enclosure_faults.faultSymptom = fault_symptom;
    base_enclosure->enclosure_faults.faultyComponentIndex = index;
    base_enclosure->enclosure_faults.additional_status = addl_status;
    return FBE_ENCLOSURE_STATUS_OK;
}

/*!**************************************************************
 * fbe_base_enclosure_translate_encl_stat_to_flt_symptom()
 ****************************************************************
 * DESCRIPTION:
 *  This function returns fault symptom based on enclsoure status
 *
 * PARAMETERS: 
 * encl_stat - enclosure status to be converted to fault symptom.
 *
 * RETURNS: 
 * fbe_u32_t - enclosure fault symptom
 *
 * HISTORY:
 *   05/06/2009:  Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_u32_t fbe_base_enclosure_translate_encl_stat_to_flt_symptom(fbe_enclosure_status_t encl_stat)
{
    fbe_u32_t encl_flt_sym;

    switch(encl_stat)
    {
    case FBE_ENCLOSURE_STATUS_OK:
    case FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_NONE;
        break;
    case FBE_ENCLOSURE_STATUS_ALLOCATED_MEMORY_INSUFFICIENT:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_ALLOCATED_MEMORY_INSUFFICIENT;
        break;
    case FBE_ENCLOSURE_STATUS_PROCESS_PAGE_FAILED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_PROCESS_PAGE_FAILED;
        break;
    case FBE_ENCLOSURE_STATUS_CMD_FAILED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_CMD_FAILED;
        break;
    case FBE_ENCLOSURE_STATUS_ENCL_FUNC_UNSUPPORTED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_ENCL_FUNC_UNSUPPORTED;
        break;
    case FBE_ENCLOSURE_STATUS_PARAMETER_INVALID:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_PARAMETER_INVALID;
        break;
    case FBE_ENCLOSURE_STATUS_HARDWARE_ERROR:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_HARDWARE_ERROR;
        break;
    case FBE_ENCLOSURE_STATUS_CDB_REQUEST_FAILED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_CDB_REQUEST_FAILED;
        break;
    case FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_LIFECYCLE_FAILED;
        break;
    case FBE_ENCLOSURE_STATUS_EDAL_FAILED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_EDAL_FAILED;
        break;
    case FBE_ENCLOSURE_STATUS_CONFIG_INVALID:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_CONFIG_INVALID;
        break;
    case FBE_ENCLOSURE_STATUS_MAPPING_FAILED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_MAPPING_FAILED;
        break;
    case FBE_ENCLOSURE_STATUS_COMP_UNSUPPORTED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_COMP_UNSUPPORTED;
        break;
    case FBE_ENCLOSURE_STATUS_ESES_PAGE_INVALID:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_ESES_PAGE_INVALID;
        break;
    case FBE_ENCLOSURE_STATUS_CTRL_CODE_UNSUPPORTED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_CTRL_CODE_UNSUPPORTED;
        break;
    case FBE_ENCLOSURE_STATUS_DATA_ILLEGAL:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_DATA_ILLEGAL;
        break;
    case FBE_ENCLOSURE_STATUS_BUSY:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_BUSY;
        break;
    case FBE_ENCLOSURE_STATUS_DATA_ACCESS_FAILED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_DATA_ACCESS_FAILED;
        break;
    case FBE_ENCLOSURE_STATUS_UNSUPPORTED_PAGE_HANDLED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_UNSUPPORTED_PAGE_HANDLED;
        break;
    case FBE_ENCLOSURE_STATUS_MODE_SELECT_NEEDED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_MODE_SELECT_NEEDED;
        break;
    case FBE_ENCLOSURE_STATUS_MEM_ALLOC_FAILED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_MEM_ALLOC_FAILED;
        break;
    case FBE_ENCLOSURE_STATUS_PACKET_FAILED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_PACKET_FAILED;
        break;
    case FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_EDAL_NOT_NEEDED;
        break;
    case FBE_ENCLOSURE_STATUS_TARGET_NOT_FOUND:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_TARGET_NOT_FOUND;
        break;
    case FBE_ENCLOSURE_STATUS_ELEM_GROUP_INVALID:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_ELEM_GROUP_INVALID;
        break;
    case FBE_ENCLOSURE_STATUS_PATH_ATTR_UNAVAIL:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_PATH_ATTR_UNAVAIL;
        break;
    case FBE_ENCLOSURE_STATUS_CHECKSUM_ERROR:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_CHECKSUM_ERROR;
        break;
    case FBE_ENCLOSURE_STATUS_ILLEGAL_REQUEST:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_ILLEGAL_REQUEST;
        break;
    case FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_BUILD_CDB_FAILED;
        break;
    case FBE_ENCLOSURE_STATUS_BUILD_PAGE_FAILED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_BUILD_PAGE_FAILED;
        break;
    case FBE_ENCLOSURE_STATUS_NULL_PTR:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_NULL_PTR;
        break;
    case FBE_ENCLOSURE_STATUS_INVALID_CANARY:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_INVALID_CANARY;
        break;
    case FBE_ENCLOSURE_STATUS_EDAL_ERROR:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_EDAL_ERROR;
        break;
    case FBE_ENCLOSURE_STATUS_COMPONENT_NOT_FOUND:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_COMPONENT_NOT_FOUND;
        break;
    case FBE_ENCLOSURE_STATUS_ATTRIBUTE_NOT_FOUND:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_ATTRIBUTE_NOT_FOUND;
        break;
    case FBE_ENCLOSURE_STATUS_COMP_TYPE_INDEX_INVALID:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_COMP_TYPE_INDEX_INVALID;
        break;
    case FBE_ENCLOSURE_STATUS_COMP_TYPE_UNSUPPORTED:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_COMP_TYPE_UNSUPPORTED;
        break;
    case FBE_ENCLOSURE_STATUS_UNSUPPORTED_ENCLOSURE:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_UNSUPPORTED_ENCLOSURE;
        break;
    case FBE_ENCLOSURE_STATUS_INSUFFICIENT_RESOURCE:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_INSUFFICIENT_RESOURCE;
        break;
    default:
    case FBE_ENCLOSURE_STATUS_INVALID:
        encl_flt_sym = FBE_ENCL_FLTSYMPT_INVALID;
        break;
    }
    return encl_flt_sym;
}



/*!****************************************************************************
 * @fn      fbe_base_enclosure_get_mgmt_basic_info(fbe_base_enclosure_t * base_enclosure,
 *                                    fbe_base_object_mgmt_get_enclosure_basic_info_t  *mgmt_basic_info)
 *
 * @brief
 * This function sets the return management basic info based on
 * default values except for the faultSymptom and the server_position. 
 * This should only be called when the enclosure object has not fully
 * initialized.  Such is the case when the enclosure is FAILED at the
 * ESES level and the leaf class has not been created yet, or if the
 * enclosure is not supported and is failed at the base object level.
 *
 * @param base_enclosure - our object context
 * @param mgmt_basic_info - Used to return the basic info.
 *
 * @return
 *  None
 *
 * HISTORY
 *  11/17/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
fbe_enclosure_status_t fbe_base_enclosure_get_mgmt_basic_info(fbe_base_enclosure_t * base_enclosure,
                                      fbe_base_object_mgmt_get_enclosure_basic_info_t  *mgmt_basic_info)
{
    mgmt_basic_info->enclSasAddress = FBE_SAS_ADDRESS_INVALID;
    mgmt_basic_info->enclChangeCount = FBE_ENCLOSURE_VALUE_INVALID;
    mgmt_basic_info->enclTimeSinceLastGoodStatus = 0;
    mgmt_basic_info->enclFaultSymptom = base_enclosure->enclosure_faults.faultSymptom;
    fbe_copy_memory((void *)&mgmt_basic_info->enclUniqueId, (const void *) "UNDETERMINED", sizeof("UNDETERMINED"));
    mgmt_basic_info->enclPosition = base_enclosure->enclosure_number;
    mgmt_basic_info->enclSideId = FBE_ENCLOSURE_VALUE_INVALID;
    mgmt_basic_info->enclRelatedExpanders = 0; 

    return FBE_ENCLOSURE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_get_scsi_error_info()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function gets fbe_enclosure_scsi_error_info_t information.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  scsi_error_info - pointer to fbe_enclosure_scsi_error_info_t 
 *
 * RETURNS:
 *  fbe_enclosure_status_t - FBE_ENCLOSURE_STATUS_OK - no error.
 *
 * HISTORY:
 *  05/13/09 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_enclosure_status_t fbe_base_enclosure_get_scsi_error_info(fbe_base_enclosure_t * base_enclosure,
                                                    fbe_enclosure_scsi_error_info_t  *scsi_error_info)
{
    *scsi_error_info = base_enclosure->scsi_error_info;
    return FBE_ENCLOSURE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_set_scsi_error_info()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function sets fbe_enclosure_scsi_error_info_t information.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  scsi_error_code - fbe_u32_t
 *  scsi_status - fbe_u32_t
 *  payload_request_status - fbe_u32_t
 *  sense_data - pointer to fbe_u8_t
 *
 * RETURNS:
 *  fbe_enclosure_status_t - FBE_ENCLOSURE_STATUS_OK - no error.
 *
 * HISTORY:
 *  05/13/09 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_enclosure_status_t fbe_base_enclosure_set_scsi_error_info(fbe_base_enclosure_t * base_enclosure,
                                                    fbe_u32_t scsi_error_code,
                                                    fbe_u32_t scsi_status,
                                                    fbe_u32_t payload_request_status,
                                                    fbe_u8_t * sense_data)
{
    base_enclosure->scsi_error_info.scsi_error_code = scsi_error_code;
    base_enclosure->scsi_error_info.payload_request_status = payload_request_status;
    base_enclosure->scsi_error_info.scsi_status = scsi_status;

    // sense_data is valid only for the check conditions
    if((scsi_status == FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION) ||
        (scsi_status == FBE_PAYLOAD_CDB_SCSI_STATUS_RESERVATION_CONFLICT))
    {
        if(sense_data)
        {
            base_enclosure->scsi_error_info.sense_key = sense_data[FBE_SCSI_SENSE_DATA_SENSE_KEY_OFFSET] & FBE_SCSI_SENSE_DATA_SENSE_KEY_MASK;
            base_enclosure->scsi_error_info.addl_sense_code = sense_data[FBE_SCSI_SENSE_DATA_ASC_OFFSET];
            base_enclosure->scsi_error_info.addl_sense_code_qualifier = sense_data[FBE_SCSI_SENSE_DATA_ASCQ_OFFSET];
        }
    }
    else
    {
        base_enclosure->scsi_error_info.sense_key = 0;
        base_enclosure->scsi_error_info.addl_sense_code = 0;
        base_enclosure->scsi_error_info.addl_sense_code_qualifier = 0;
    }
    return FBE_ENCLOSURE_STATUS_OK;
}



/*!****************************************************************************
 * @fn      fbe_enclosure_status_t fbe_base_enclosure_get_edal_locale(base_enclosure, edalLocalPtr)
 *
 * @brief
 * This function returns the edalLocale from the base enclosure
 * enclosureComponentTypeBlock.
 *
 * @param base_enclosure - points to object from which to extract edalLocale.
 *
 * @return
 *  fbe_enclosure_status_t FBE_ENCLOSURE_STATUS_OK - no error.
 *
 * HISTORY
 *  05/07/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
fbe_enclosure_status_t 
fbe_base_enclosure_get_edal_locale(fbe_base_enclosure_t * base_enclosure, fbe_u8_t *edalLocalPtr)
{
    if(base_enclosure == NULL || base_enclosure->enclosureComponentTypeBlock == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
   
    *edalLocalPtr = base_enclosure->enclosureComponentTypeBlock->edalLocale;

    return FBE_ENCLOSURE_STATUS_OK;
}



/****************************************************************
 * fbe_base_enclosure_init()
 ****************************************************************
 * DESCRIPTION:
 *   This is the init function for this class.  This function will
 * be called from the object constructor when base_enclosure or
 * its derived class object is created.  If some other objects morph
 * into base_enclosure by setting the class id, the init condition
 * will be set, which in turn call this function.  For instance, 
 * a base_discoverying can morph into a base_enclosure.
 *
 *  This condition function initialize default_ride_through_period.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  08/04/08 - Created. NCHIU
 *
 ****************************************************************/
fbe_status_t 
fbe_base_enclosure_init(fbe_base_enclosure_t * base_enclosure)
{
    base_enclosure->time_ride_through_start      = 0;
    base_enclosure->expected_ride_through_period = 0;
    base_enclosure->default_ride_through_period  = 0;
    base_enclosure->condition_retry_count        = 0;
    base_enclosure->port_number                  = FBE_BUS_ID_INVALID;
    base_enclosure->server_address.sas_address   = FBE_SAS_ADDRESS_INVALID;
    base_enclosure->enclosure_number             = FBE_ENCLOSURE_POSITION_INVALID;
    base_enclosure->my_position                  = FBE_ENCLOSURE_POSITION_INVALID;
    base_enclosure->enclosureComponentTypeBlock  = NULL;
    base_enclosure->enclosureComponentTypeBlock_backup = NULL;
    base_enclosure->active_edal_valid = FALSE;
    base_enclosure->enclosure_faults.faultSymptom = FBE_ENCL_FLTSYMPT_NONE;
    base_enclosure->lifecycle_set_condition_routine = 
        (fbe_base_enclosure_lifecycle_set_cond_function_t)fbe_lifecycle_set_cond;
    base_enclosure->number_of_drives_spinningup = 0;
    /*max_number_of_drives_spinningup will be updated when the leaf enclosure gets specialized. */ 
    base_enclosure->max_number_of_drives_spinningup = 0;
#if ENABLE_EDAL_CACHE_POINTER
    base_enclosure->enclosure_component_access_data.component = FBE_ENCL_INVALID_COMPONENT_TYPE;
    base_enclosure->enclosure_component_access_data.index = FBE_EDAL_INDEX_INVALID;
    base_enclosure->enclosure_component_access_data.specificCompPtr = NULL;
#endif
    
    /* Zeroing memory for logHeader and copy UNKNOWN as default */ 
    fbe_zero_memory(base_enclosure->logHeader, FBE_BASE_ENCL_HEADER);
    strncpy(base_enclosure->logHeader, "Enc:NA", FBE_BASE_ENCL_HEADER);
    
    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader(base_enclosure),
                                        "%s entry\n", __FUNCTION__);

    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader(base_enclosure),
                                        "%s entry\n", __FUNCTION__);

    return FBE_STATUS_OK;
}

/**************************************************************************
 *                         fbe_base_enclosure_access_specific_component
 **************************************************************************
 *
 *  DESCRIPTION:
 *      This function will retrieve pointer to a specific
 *      Component.
 *
 *  PARAMETERS:
 *      
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Aug-2008: NCHIU - Created
 *
 **************************************************************************/
fbe_status_t
fbe_base_enclosure_access_specific_component(fbe_base_enclosure_t *enclosurePtr,
                                             fbe_enclosure_component_types_t componentType,
                                             fbe_u8_t index,
                                             void **encl_component)
{
    fbe_status_t         status = FBE_STATUS_OK;
    fbe_edal_status_t    edal_status = FBE_EDAL_STATUS_OK;

    edal_status = fbe_edal_getSpecificComponent((fbe_edal_block_handle_t) enclosurePtr->enclosureComponentTypeBlock,
                                    componentType,
                                    index,
                                    encl_component);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        *encl_component = NULL;
        // Fail to find the number of components. 
        fbe_base_object_customizable_trace((fbe_base_object_t*)enclosurePtr,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(enclosurePtr),
                            "%s, failed to get specific component ptr, componentType: %s.\n", 
                            __FUNCTION__, enclosure_access_printComponentType(componentType));

        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_base_enclosure_access_specific_component


/**************************************************************************
 *                         fbe_base_enclosure_get_number_of_components
 **************************************************************************
 *
 *  DESCRIPTION:
 *      This function returns the number of components for the specified component type.
 *
 *  PARAMETERS:
 *      base_enclosure(Input) - the pointer to the base enclosure.
 *      component_type(Input) - the specified component type.
 *      number_of_components(Output) - the returned number of components.
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t    
 *
 *  NOTES:
 *      Need to revisit the code when the number_of_components is larger than 255.
 *
 *  HISTORY:
 *    05-Sep-2008: PHE - Created
 *
 **************************************************************************/
fbe_status_t
fbe_base_enclosure_get_number_of_components(fbe_base_enclosure_t * base_enclosure,
                                             fbe_enclosure_component_types_t component_type,
                                             fbe_u8_t * number_of_components)
{
    fbe_u32_t           component_count = 0;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_status_t   edal_status = FBE_EDAL_STATUS_OK;

    edal_status = fbe_edal_getSpecificComponentCount(
                            (fbe_edal_block_handle_t)base_enclosure->enclosureComponentTypeBlock, 
                             component_type,
                             &component_count);

    if(edal_status == FBE_EDAL_STATUS_OK)
    {
        *number_of_components = (fbe_u8_t)component_count;
        status = FBE_STATUS_OK;
    }
    else
    {
        *number_of_components = FBE_ENCLOSURE_VALUE_INVALID;
        
        // Fail to find the number of components. 
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, failed to find the number of components, comp type %d.\n", 
                            __FUNCTION__, component_type);
        
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_base_enclosure_get_number_of_components

/*!*************************************************************************
 * @fn fbe_base_enclosure_translate_edal_status(fbe_edal_status_t edal_status);
 *
 ***************************************************************************
 * @brief
 *       This routine translate edal_status into enclosure status
 *
 * @param   edal_status - edal status.
 *
 * @return  fbe_enclosure_status_t.
 *
 * HISTORY
 *   18-Nov-2008 NC - Created.
 ****************************************************************************/
fbe_enclosure_status_t fbe_base_enclosure_translate_edal_status(fbe_edal_status_t edal_status)
{
    fbe_enclosure_status_t encl_status;

    switch (edal_status)
    {
    case FBE_EDAL_STATUS_OK:
        encl_status = FBE_ENCLOSURE_STATUS_OK;
        break;
    case FBE_EDAL_STATUS_OK_VAL_CHANGED:
        encl_status = FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED;
        break;
    case FBE_EDAL_STATUS_NULL_BLK_PTR:
    case FBE_EDAL_STATUS_NULL_RETURN_PTR:
    case FBE_EDAL_STATUS_NULL_COMP_TYPE_DATA_PTR:
    case FBE_EDAL_STATUS_NULL_COMP_DATA_PTR:
        encl_status = FBE_ENCLOSURE_STATUS_NULL_PTR;
        break;
    case FBE_EDAL_STATUS_INVALID_BLK_CANARY:
    case FBE_EDAL_STATUS_INVALID_COMP_CANARY:
        encl_status = FBE_ENCLOSURE_STATUS_INVALID_CANARY;
        break;
    case FBE_EDAL_STATUS_ERROR:
        encl_status = FBE_ENCLOSURE_STATUS_EDAL_ERROR;
        break;
    case FBE_EDAL_STATUS_COMPONENT_NOT_FOUND:
        encl_status = FBE_ENCLOSURE_STATUS_COMPONENT_NOT_FOUND;
        break;
    case FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND:
        encl_status = FBE_ENCLOSURE_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    case FBE_EDAL_STATUS_COMP_TYPE_INDEX_INVALID:
        encl_status = FBE_ENCLOSURE_STATUS_COMP_TYPE_INDEX_INVALID;
        break;
    case FBE_EDAL_STATUS_COMP_TYPE_UNSUPPORTED:
        encl_status = FBE_ENCLOSURE_STATUS_COMP_TYPE_UNSUPPORTED;
        break;
    case FBE_EDAL_STATUS_UNSUPPORTED_ENCLOSURE:
        encl_status = FBE_ENCLOSURE_STATUS_UNSUPPORTED_ENCLOSURE;
        break;
    case FBE_EDAL_STATUS_INSUFFICIENT_RESOURCE:
        encl_status = FBE_ENCLOSURE_STATUS_INSUFFICIENT_RESOURCE;
        break;
    default:
         encl_status = FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        break;
    }

    return (encl_status);
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_setOverallStatus(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t setValue);
 *
 ***************************************************************************
 * @brief
 *       This routine sets the new value to the EDAL for the specified 
 * component type overall status.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   component      - The component type.
 * @param   setValue       - The new value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - set successful.
 *
 * HISTORY
 *   17-Dec-2008 NC - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_setOverallStatus(fbe_base_enclosure_t * base_enclosure,
                                         fbe_enclosure_component_types_t component,
                                         fbe_u32_t setValue)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    edalStatus = fbe_edal_setComponentOverallStatus(base_enclosure->enclosureComponentTypeBlock,
                                                    component,
                                                    setValue);
    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (edalStatus != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, unhandled stat %d, comp type %s.\n", 
                            __FUNCTION__, edalStatus,
                            enclosure_access_printComponentType(component));

        
    }

    return retStatus;
}


/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getOverallStatus(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t *returnValuePtr);
 *
 ***************************************************************************
 * @brief
 *       This routine retrieve the value to the EDAL from the specified 
 * component type overall status.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   component      - The component type.
 * @param   returnValuePtr - The pointer to the return status.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - set successful.
 *
 * HISTORY
 *   17-Dec-2008 NC - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getOverallStatus(fbe_base_enclosure_t * base_enclosure,
                                         fbe_enclosure_component_types_t component,
                                         fbe_u32_t *returnValuePtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    edalStatus = fbe_edal_getComponentOverallStatus(base_enclosure->enclosureComponentTypeBlock,
                                                    component,
                                                    returnValuePtr);
    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if(retStatus == FBE_ENCLOSURE_STATUS_COMPONENT_NOT_FOUND)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "base_encl_edal_getOverallStatus, COMPONENT_NOT_FOUND, comp type: %s.\n", 
                            enclosure_access_printComponentType(component));
    }
    else if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_WARNING,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "base_encl_edal_getOverallStatus, unhandled edalStat %d, comp type: %s.\n", 
                            edalStatus,
                            enclosure_access_printComponentType(component));
      
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getBool(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_bool_t *returnValuePtr);
 *
 ***************************************************************************
 * @brief
 *       This routine gets the bool value from the EDAL for the specified 
 * component type, attribute, and index
 *
 *       This routine assumes single thread context and provides no protection
 * against threads interaction.  It can be only used for requests generated
 * from monitor context, and the corresponding completion routine.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   returnValuePtr (out) - The pointer to the returned value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 * 
 * HISTORY
 *   17-Nov-2008 NC - Created.
 *   25-Jun-2009 Sachin D - Changed to optimize the EDAL calls
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getBool(fbe_base_enclosure_t * base_enclosure,
                                fbe_base_enclosure_attributes_t attribute,
                                fbe_enclosure_component_types_t component,
                                fbe_u32_t index,
                                fbe_bool_t *returnValuePtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;
#if ENABLE_EDAL_CACHE_POINTER
    fbe_enclosure_component_accessed_data_t *encl_comp_accessed_data_ptr;
#else
    fbe_edal_component_data_handle_t specificCompPtr;     
#endif

    if(base_enclosure == NULL || base_enclosure->enclosureComponentTypeBlock == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
#if ENABLE_EDAL_CACHE_POINTER
    encl_comp_accessed_data_ptr = &base_enclosure->enclosure_component_access_data;
   
    // Sanity check on previous data.
    if(encl_comp_accessed_data_ptr->component != component ||
       encl_comp_accessed_data_ptr->index != index ||
       encl_comp_accessed_data_ptr->specificCompPtr == NULL)
    {
        encl_comp_accessed_data_ptr->component = component;
        encl_comp_accessed_data_ptr->index = index;
        
        fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                                component,
                                                index,
                                                &encl_comp_accessed_data_ptr->specificCompPtr);
    }
    
    edalStatus = fbe_edal_getBool_direct(encl_comp_accessed_data_ptr->specificCompPtr,
                                         base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                                         attribute,
                                         component, 
                                         returnValuePtr);
#else
    fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                         component,
                                         index,
                                         &specificCompPtr);

    edalStatus = fbe_edal_getBool_direct(specificCompPtr,
                                         base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                                         attribute,
                                         component, 
                                         returnValuePtr);
#endif
    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "baseEncl getBool, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            retStatus, enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);        

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_setBool(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_bool_t setValue);
 *
 ***************************************************************************
 * @brief
 *       This routine sets the bool value to the EDAL for the specified 
 * component type, attribute, and index.
 *
 *       This routine assumes single thread context and provides no protection
 * against threads interaction.  It can be only used for requests generated
 * from monitor context, and the corresponding completion routine.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   setValue       - The new value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - set successful.
 *
 * HISTORY
 *   17-Nov-2008 NC - Created.
 *   25-Jun-2009 Sachin D - Changed to optimize the EDAL calls
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_setBool(fbe_base_enclosure_t * base_enclosure,
                                fbe_base_enclosure_attributes_t attribute,
                                fbe_enclosure_component_types_t component,
                                fbe_u32_t index,
                                fbe_bool_t setValue)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;
#if ENABLE_EDAL_CACHE_POINTER
    fbe_enclosure_component_accessed_data_t *encl_comp_accessed_data_ptr;
#else
    fbe_edal_component_data_handle_t specificCompPtr;     
#endif

    if(base_enclosure == NULL || base_enclosure->enclosureComponentTypeBlock == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
#if ENABLE_EDAL_CACHE_POINTER
    encl_comp_accessed_data_ptr = &base_enclosure->enclosure_component_access_data;
   
    // Sanity check on previous data.
    if(encl_comp_accessed_data_ptr->component != component ||
       encl_comp_accessed_data_ptr->index != index ||
       encl_comp_accessed_data_ptr->specificCompPtr == NULL)
    {
        encl_comp_accessed_data_ptr->component = component;
        encl_comp_accessed_data_ptr->index = index;
        
        fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                                component,
                                                index,
                                                &encl_comp_accessed_data_ptr->specificCompPtr);
    }
    
    edalStatus = fbe_edal_setBool_direct(encl_comp_accessed_data_ptr->specificCompPtr,
                          base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                          attribute,
                          component,
                          setValue);
#else
    fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                            component,
                                            index,
                                            &specificCompPtr);
    
    edalStatus = fbe_edal_setBool_direct(specificCompPtr,
                          base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                          attribute,
                          component,
                          setValue);
#endif
    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "baseEncl setBool, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);  

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
                
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getU8(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u8_t *returnValuePtr);
 *
 ***************************************************************************
 * @brief
 *       This routine gets the U8 value from the EDAL for the specified 
 * component type, attribute, and index
 *
 *       This routine assumes single thread context and provides no protection
 * against threads interaction.  It can be only used for requests generated
 * from monitor context, and the corresponding completion routine.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   returnValuePtr (out) - The pointer to the returned value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   17-Nov-2008 NC - Created.
 *   25-Jun-2009 Sachin D - Changed to optimize the EDAL calls
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getU8(fbe_base_enclosure_t * base_enclosure,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_u32_t index,
                              fbe_u8_t *returnValuePtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;
#if ENABLE_EDAL_CACHE_POINTER
    fbe_enclosure_component_accessed_data_t *encl_comp_accessed_data_ptr;
#else
    fbe_edal_component_data_handle_t specificCompPtr;     
#endif

    if(base_enclosure == NULL || base_enclosure->enclosureComponentTypeBlock == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
#if ENABLE_EDAL_CACHE_POINTER
    encl_comp_accessed_data_ptr = &base_enclosure->enclosure_component_access_data;
   
    // Sanity check on previous data.
    if(encl_comp_accessed_data_ptr->component != component ||
       encl_comp_accessed_data_ptr->index != index ||
       encl_comp_accessed_data_ptr->specificCompPtr == NULL)
    {
        encl_comp_accessed_data_ptr->component = component;
        encl_comp_accessed_data_ptr->index = index;
        
        fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                                component,
                                                index,
                                                &encl_comp_accessed_data_ptr->specificCompPtr);
    }
    
    edalStatus = fbe_edal_getU8_direct(encl_comp_accessed_data_ptr->specificCompPtr,
                                         base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                                         attribute,
                                         component, 
                                         returnValuePtr);
#else
    fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                            component,
                                            index,
                                            &specificCompPtr);
    
    edalStatus = fbe_edal_getU8_direct(specificCompPtr,
                                         base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                                         attribute,
                                         component, 
                                         returnValuePtr);
#endif

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "baseEncl getU8, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index); 

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                        FBE_ENCL_ENCLOSURE,
                                                        index,
                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                        0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_incrementStateChangeCount(fbe_base_enclosure_t * base_enclosure)
 *
 ***************************************************************************
 * @brief
 *       This routine increments the EDAL State Change Count
 *
 * @param   base_enclosure - The pointer to the enclosure.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   20-Apr-2009 Joe Perry - Created.
 *
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_incrementStateChangeCount(fbe_base_enclosure_t * base_enclosure)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    edalStatus = fbe_edal_incrementOverallStateChange(base_enclosure->enclosureComponentTypeBlock);
    if (edalStatus != FBE_EDAL_STATUS_OK)
    {
        retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
        fbe_base_object_trace((fbe_base_object_t*)base_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, unhandled status %d\n", 
                                __FUNCTION__, retStatus);
    }

    return retStatus;

}   // end of fbe_base_enclosure_edal_incrementStateChangeCount

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_incrementGenerationCount
 *
 ***************************************************************************
 * @brief
 *       This routine increments the EDAL generation Count
 *
 * @param   base_enclosure - The pointer to the enclosure.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   06-Jan-2010 Dipak Patel - Created.
 *
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_incrementGenerationCount(fbe_base_enclosure_t * base_enclosure)
{

   fbe_edal_status_t   edalStatus;
   fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    edalStatus = fbe_edal_incrementGenrationCount(base_enclosure->enclosureComponentTypeBlock);
    return retStatus; 
     
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_get_generationCount
 *
 ***************************************************************************
 * @brief
 *       This routine gets the EDAL generation Count
 *
 * @param   base_enclosure - The pointer to the enclosure.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   06-Jan-2010 Dipak Patel - Created.
 *
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_get_generationCount(fbe_base_enclosure_t * base_enclosure, fbe_u32_t * generation_count)
{

   fbe_edal_status_t   edalStatus;
   fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    edalStatus = fbe_edal_getGenerationCount(base_enclosure->enclosureComponentTypeBlock, generation_count);
    if (edalStatus != FBE_EDAL_STATUS_OK)
    {
        retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
        fbe_base_object_trace((fbe_base_object_t*)base_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, unhandled status %d\n", 
                                __FUNCTION__, retStatus);
    }
    return retStatus; 
     
}
/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_setU8(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u8_t setValue);
 *
 ***************************************************************************
 * @brief
 *       This routine sets the U8 value to the EDAL for the specified 
 * component type, attribute, and index.
 *
 *       This routine assumes single thread context and provides no protection
 * against threads interaction.  It can be only used for requests generated
 * from monitor context, and the corresponding completion routine.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   setValue       - The new value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - set successful.
 *
 * HISTORY
 *   17-Nov-2008 NC - Created.
 *   25-Jun-2009 Sachin D - Changed to optimize the EDAL calls
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_setU8(fbe_base_enclosure_t * base_enclosure,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_u32_t index,
                              fbe_u8_t setValue)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;
#if ENABLE_EDAL_CACHE_POINTER
    fbe_enclosure_component_accessed_data_t *encl_comp_accessed_data_ptr;
#else
    fbe_edal_component_data_handle_t specificCompPtr;     
#endif

    if(base_enclosure == NULL || base_enclosure->enclosureComponentTypeBlock == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
#if ENABLE_EDAL_CACHE_POINTER
    encl_comp_accessed_data_ptr = &base_enclosure->enclosure_component_access_data;
   
    // Sanity check on previous data.
    if(encl_comp_accessed_data_ptr->component != component ||
       encl_comp_accessed_data_ptr->index != index ||
       encl_comp_accessed_data_ptr->specificCompPtr == NULL)
    {
        encl_comp_accessed_data_ptr->component = component;
        encl_comp_accessed_data_ptr->index = index;
        
        fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                                component,
                                                index,
                                                &encl_comp_accessed_data_ptr->specificCompPtr);
    }
    
    edalStatus = fbe_edal_setU8_direct(encl_comp_accessed_data_ptr->specificCompPtr,
                          base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                          attribute,
                          component,
                          setValue);
#else
    fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                            component,
                                            index,
                                            &specificCompPtr);
    
    edalStatus = fbe_edal_setU8_direct(specificCompPtr,
                          base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                          attribute,
                          component,
                          setValue);
#endif

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "baseEncl setU8, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);  

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getU16(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u16_t *returnValuePtr);
 *
 ***************************************************************************
 * @brief
 *       This routine gets the U16 value from the EDAL for the specified 
 * component type, attribute, and index
 * 
 *       This routine assumes single thread context and provides no protection
 * against threads interaction.  It can be only used for requests generated
 * from monitor context, and the corresponding completion routine.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   returnValuePtr (out) - The pointer to the returned value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   17-Nov-2008 NC - Created.
 *   25-Jun-2009 Sachin D - Changed to optimize the EDAL calls
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getU16(fbe_base_enclosure_t * base_enclosure,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_enclosure_component_types_t component,
                               fbe_u32_t index,
                               fbe_u16_t *returnValuePtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;
#if ENABLE_EDAL_CACHE_POINTER
    fbe_enclosure_component_accessed_data_t *encl_comp_accessed_data_ptr;
#else
    fbe_edal_component_data_handle_t specificCompPtr;     
#endif

    if(base_enclosure == NULL || base_enclosure->enclosureComponentTypeBlock == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
#if ENABLE_EDAL_CACHE_POINTER
    encl_comp_accessed_data_ptr = &base_enclosure->enclosure_component_access_data;
   
    // Sanity check on previous data.
    if(encl_comp_accessed_data_ptr->component != component ||
       encl_comp_accessed_data_ptr->index != index ||
       encl_comp_accessed_data_ptr->specificCompPtr == NULL)
    {
        encl_comp_accessed_data_ptr->component = component;
        encl_comp_accessed_data_ptr->index = index;
        
        fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                                component,
                                                index,
                                                &encl_comp_accessed_data_ptr->specificCompPtr);
    }
    
    edalStatus = fbe_edal_getU16_direct(encl_comp_accessed_data_ptr->specificCompPtr,
                                         base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                                         attribute,
                                         component, 
                                         returnValuePtr);
#else
    fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                            component,
                                            index,
                                            &specificCompPtr);
    
    edalStatus = fbe_edal_getU16_direct(specificCompPtr,
                                         base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                                         attribute,
                                         component, 
                                         returnValuePtr);
#endif

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "baseEncl getU16, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index); 

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_setU16(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u16_t setValue);
 *
 ***************************************************************************
 * @brief
 *       This routine sets the U16 value to the EDAL for the specified 
 * component type, attribute, and index.
 *
 *       This routine assumes single thread context and provides no protection
 * against threads interaction.  It can be only used for requests generated
 * from monitor context, and the corresponding completion routine.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   setValue       - The new value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - set successful.
 *
 * HISTORY
 *   17-Nov-2008 NC - Created.
 *   25-Jun-2009 Sachin D - Changed to optimize the EDAL calls
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_setU16(fbe_base_enclosure_t * base_enclosure,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_enclosure_component_types_t component,
                               fbe_u32_t index,
                               fbe_u16_t setValue)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;
#if ENABLE_EDAL_CACHE_POINTER
    fbe_enclosure_component_accessed_data_t *encl_comp_accessed_data_ptr;
#else
    fbe_edal_component_data_handle_t specificCompPtr;     
#endif

    if(base_enclosure == NULL || base_enclosure->enclosureComponentTypeBlock == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
#if ENABLE_EDAL_CACHE_POINTER
    encl_comp_accessed_data_ptr = &base_enclosure->enclosure_component_access_data;
   
    // Sanity check on previous data.
    if(encl_comp_accessed_data_ptr->component != component ||
       encl_comp_accessed_data_ptr->index != index ||
       encl_comp_accessed_data_ptr->specificCompPtr == NULL)
    {
        encl_comp_accessed_data_ptr->component = component;
        encl_comp_accessed_data_ptr->index = index;
        
        fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                                component,
                                                index,
                                                &encl_comp_accessed_data_ptr->specificCompPtr);
    }
    
    edalStatus = fbe_edal_setU16_direct(encl_comp_accessed_data_ptr->specificCompPtr,
                          base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                          attribute,
                          component,
                          setValue);
#else
    fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                            component,
                                            index,
                                            &specificCompPtr);
    
    edalStatus = fbe_edal_setU16_direct(specificCompPtr,
                          base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                          attribute,
                          component,
                          setValue);
#endif

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "baseEncl setU16, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);  

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getU32(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u32_t *returnValuePtr);
 *
 ***************************************************************************
 * @brief
 *       This routine gets the U32 value from the EDAL for the specified 
 * component type, attribute, and index
 *
 *       This routine assumes single thread context and provides no protection
 * against threads interaction.  It can be only used for requests generated
 * from monitor context, and the corresponding completion routine.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   returnValuePtr (out) - The pointer to the returned value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   17-Nov-2008 NC - Created.
 *   25-Jun-2009 Sachin D - Changed to optimize the EDAL calls
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getU32(fbe_base_enclosure_t * base_enclosure,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_enclosure_component_types_t component,
                               fbe_u32_t index,
                               fbe_u32_t *returnValuePtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;
#if ENABLE_EDAL_CACHE_POINTER
    fbe_enclosure_component_accessed_data_t *encl_comp_accessed_data_ptr;
#else
    fbe_edal_component_data_handle_t specificCompPtr;     
#endif

    if(base_enclosure == NULL || base_enclosure->enclosureComponentTypeBlock == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
#if ENABLE_EDAL_CACHE_POINTER
    encl_comp_accessed_data_ptr = &base_enclosure->enclosure_component_access_data;
   
    // Sanity check on previous data.
    if(encl_comp_accessed_data_ptr->component != component ||
       encl_comp_accessed_data_ptr->index != index ||
       encl_comp_accessed_data_ptr->specificCompPtr == NULL)
    {
        encl_comp_accessed_data_ptr->component = component;
        encl_comp_accessed_data_ptr->index = index;
        
        fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                                component,
                                                index,
                                                &encl_comp_accessed_data_ptr->specificCompPtr);
    }
    
    edalStatus = fbe_edal_getU32_direct(encl_comp_accessed_data_ptr->specificCompPtr,
                                         base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                                         attribute,
                                         component, 
                                         returnValuePtr);
#else
    fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                            component,
                                            index,
                                            &specificCompPtr);
    
    edalStatus = fbe_edal_getU32_direct(specificCompPtr,
                                         base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                                         attribute,
                                         component, 
                                         returnValuePtr);
#endif

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "baseEncl getU32, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_setU32(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u32_t setValue);
 *
 ***************************************************************************
 * @brief
 *       This routine sets the U32 value to the EDAL for the specified 
 * component type, attribute, and index.
 *
 *       This routine assumes single thread context and provides no protection
 * against threads interaction.  It can be only used for requests generated
 * from monitor context, and the corresponding completion routine.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   setValue       - The new value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - set successful.
 *
 * HISTORY
 *   17-Nov-2008 NC - Created.
 *   25-Jun-2009 Sachin D - Changed to optimize the EDAL calls
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_setU32(fbe_base_enclosure_t * base_enclosure,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_enclosure_component_types_t component,
                               fbe_u32_t index,
                               fbe_u32_t setValue)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;
#if ENABLE_EDAL_CACHE_POINTER
    fbe_enclosure_component_accessed_data_t *encl_comp_accessed_data_ptr;
#else
    fbe_edal_component_data_handle_t specificCompPtr;     
#endif

    if(base_enclosure == NULL || base_enclosure->enclosureComponentTypeBlock == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
#if ENABLE_EDAL_CACHE_POINTER
    encl_comp_accessed_data_ptr = &base_enclosure->enclosure_component_access_data;
   
    // Sanity check on previous data.
    if(encl_comp_accessed_data_ptr->component != component ||
       encl_comp_accessed_data_ptr->index != index ||
       encl_comp_accessed_data_ptr->specificCompPtr == NULL)
    {
        encl_comp_accessed_data_ptr->component = component;
        encl_comp_accessed_data_ptr->index = index;
        
        fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                                component,
                                                index,
                                                &encl_comp_accessed_data_ptr->specificCompPtr);
    }
    
    edalStatus = fbe_edal_setU32_direct(encl_comp_accessed_data_ptr->specificCompPtr,
                          base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                          attribute,
                          component,
                          setValue);
#else
    fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                            component,
                                            index,
                                            &specificCompPtr);
    
    edalStatus = fbe_edal_setU32_direct(specificCompPtr,
                          base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                          attribute,
                          component,
                          setValue);
#endif

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "baseEncl setU32, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);  

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getU64(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u64_t *returnValuePtr);
 *
 ***************************************************************************
 * @brief
 *       This routine gets the U64 value from the EDAL for the specified 
 * component type, attribute, and index
 *
 *       This routine assumes single thread context and provides no protection
 * against threads interaction.  It can be only used for requests generated
 * from monitor context, and the corresponding completion routine.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   returnValuePtr (out) - The pointer to the returned value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   17-Nov-2008 NC - Created.
 *   25-Jun-2009 Sachin D - Changed to optimize the EDAL calls
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getU64(fbe_base_enclosure_t * base_enclosure,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_enclosure_component_types_t component,
                               fbe_u32_t index,
                               fbe_u64_t *returnValuePtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;
#if ENABLE_EDAL_CACHE_POINTER
    fbe_enclosure_component_accessed_data_t *encl_comp_accessed_data_ptr;
#else
    fbe_edal_component_data_handle_t specificCompPtr;     
#endif

    if(base_enclosure == NULL || base_enclosure->enclosureComponentTypeBlock == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
#if ENABLE_EDAL_CACHE_POINTER
    encl_comp_accessed_data_ptr = &base_enclosure->enclosure_component_access_data;
   
    // Sanity check on previous data.
    if(encl_comp_accessed_data_ptr->component != component ||
       encl_comp_accessed_data_ptr->index != index ||
       encl_comp_accessed_data_ptr->specificCompPtr == NULL)
    {
        encl_comp_accessed_data_ptr->component = component;
        encl_comp_accessed_data_ptr->index = index;
        
        fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                                component,
                                                index,
                                                &encl_comp_accessed_data_ptr->specificCompPtr);
    }
    
    edalStatus = fbe_edal_getU64_direct(encl_comp_accessed_data_ptr->specificCompPtr,
                                         base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                                         attribute,
                                         component, 
                                         returnValuePtr);

#else
    fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                            component,
                                            index,
                                            &specificCompPtr);
    
    edalStatus = fbe_edal_getU64_direct(specificCompPtr,
                                         base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                                         attribute,
                                         component, 
                                         returnValuePtr);
#endif

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "baseEncl getU64, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);  

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_setU64(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u64_t setValue);
 *
 ***************************************************************************
 * @brief
 *       This routine sets the U64 value to the EDAL for the specified 
 * component type, attribute, and index.
 *
 *       This routine assumes single thread context and provides no protection
 * against threads interaction.  It can be only used for requests generated
 * from monitor context, and the corresponding completion routine.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   setValue       - The new value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - set successful.
 *
 * HISTORY
 *   17-Nov-2008 NC - Created.
 *   25-Jun-2009 Sachin D - Changed to optimize the EDAL calls
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_setU64(fbe_base_enclosure_t * base_enclosure,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_enclosure_component_types_t component,
                               fbe_u32_t index,
                               fbe_u64_t setValue)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;
#if ENABLE_EDAL_CACHE_POINTER
    fbe_enclosure_component_accessed_data_t *encl_comp_accessed_data_ptr;
#else
    fbe_edal_component_data_handle_t specificCompPtr;     
#endif

    if(base_enclosure == NULL || base_enclosure->enclosureComponentTypeBlock == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
#if ENABLE_EDAL_CACHE_POINTER
    encl_comp_accessed_data_ptr = &base_enclosure->enclosure_component_access_data;
   
    // Sanity check on previous data.
    if(encl_comp_accessed_data_ptr->component != component ||
       encl_comp_accessed_data_ptr->index != index ||
       encl_comp_accessed_data_ptr->specificCompPtr == NULL)
    {
        encl_comp_accessed_data_ptr->component = component;
        encl_comp_accessed_data_ptr->index = index;
        
        fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                                component,
                                                index,
                                                &encl_comp_accessed_data_ptr->specificCompPtr);
    }
    
    edalStatus = fbe_edal_setU64_direct(encl_comp_accessed_data_ptr->specificCompPtr,
                          base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                          attribute,
                          component,
                          setValue);
#else
    fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                            component,
                                            index,
                                            &specificCompPtr);
    
    edalStatus = fbe_edal_setU64_direct(specificCompPtr,
                          base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                          attribute,
                          component,
                          setValue);
#endif

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "baseEncl setU64, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getStr(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                char *returnStringPtr);
 *
 ***************************************************************************
 * @brief
 *       This routine copies up to the length bytes of chars to the request location
 * from the EDAL for the specified component type, attribute, and index
 *
 *       This routine assumes single thread context and provides no protection
 * against threads interaction.  It can be only used for requests generated
 * from monitor context, and the corresponding completion routine.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   length         - The length of the string requested.
 * @param   returnStringPtr (out) - The pointer to the returned string location.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   17-Nov-2008 NC - Created.
 *   25-Jun-2009 Sachin D - Changed to optimize the EDAL calls
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getStr(fbe_base_enclosure_t * base_enclosure,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_enclosure_component_types_t component,
                               fbe_u32_t index,
                               fbe_u32_t length,
                               char *returnStringPtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;
#if ENABLE_EDAL_CACHE_POINTER
    fbe_enclosure_component_accessed_data_t *encl_comp_accessed_data_ptr;
#else
    fbe_edal_component_data_handle_t specificCompPtr;     
#endif

    if(base_enclosure == NULL || base_enclosure->enclosureComponentTypeBlock == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
#if ENABLE_EDAL_CACHE_POINTER
    encl_comp_accessed_data_ptr = &base_enclosure->enclosure_component_access_data;
   
    // Sanity check on previous data.
    if(encl_comp_accessed_data_ptr->component != component ||
       encl_comp_accessed_data_ptr->index != index ||
       encl_comp_accessed_data_ptr->specificCompPtr == NULL)
    {
        encl_comp_accessed_data_ptr->component = component;
        encl_comp_accessed_data_ptr->index = index;
        
        fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                                component,
                                                index,
                                                &encl_comp_accessed_data_ptr->specificCompPtr);
    }
    
    edalStatus = fbe_edal_getStr_direct(encl_comp_accessed_data_ptr->specificCompPtr,
                                         base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                                         attribute,
                                         component,
                                         length,
                                         returnStringPtr);
#else
    fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                            component,
                                            index,
                                            &specificCompPtr);
    
    edalStatus = fbe_edal_getStr_direct(specificCompPtr,
                                         base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                                         attribute,
                                         component,
                                         length,
                                         returnStringPtr);
#endif

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "baseEncl getStr, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);  

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_setStr(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u32_t length,
 *                                char *setString);
 *
 ***************************************************************************
 * @brief
 *       This routine copies string to the EDAL for the specified 
 * component type, attribute, and index.
 *
 *       This routine assumes single thread context and provides no protection
 * against threads interaction.  It can be only used for requests generated
 * from monitor context, and the corresponding completion routine.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   length         - The length of the string requested.
 * @param   setString      - The new string pointer.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - set successful.
 *
 * HISTORY
 *   17-Nov-2008 NC - Created.
 *   25-Jun-2009 Sachin D - Changed to optimize the EDAL calls
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_setStr(fbe_base_enclosure_t * base_enclosure,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_enclosure_component_types_t component,
                               fbe_u32_t index,
                               fbe_u32_t length,
                               char *setString)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;
#if ENABLE_EDAL_CACHE_POINTER
    fbe_enclosure_component_accessed_data_t *encl_comp_accessed_data_ptr;
#else
    fbe_edal_component_data_handle_t specificCompPtr;     
#endif

    if(base_enclosure == NULL || base_enclosure->enclosureComponentTypeBlock == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }

#if ENABLE_EDAL_CACHE_POINTER
    encl_comp_accessed_data_ptr = &base_enclosure->enclosure_component_access_data;
   
    // Sanity check on previous data.
    if(encl_comp_accessed_data_ptr->component != component ||
       encl_comp_accessed_data_ptr->index != index ||
       encl_comp_accessed_data_ptr->specificCompPtr == NULL)
    {
        encl_comp_accessed_data_ptr->component = component;
        encl_comp_accessed_data_ptr->index = index;
        
        fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                                component,
                                                index,
                                                &encl_comp_accessed_data_ptr->specificCompPtr);
    }
    
    edalStatus = fbe_edal_setStr_direct(encl_comp_accessed_data_ptr->specificCompPtr,
                          base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                          attribute,
                          component,
                          length,
                          setString);
#else
    fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(base_enclosure->enclosureComponentTypeBlock,
                                            component,
                                            index,
                                            &specificCompPtr);
    
    edalStatus = fbe_edal_setStr_direct(specificCompPtr,
                          base_enclosure->enclosureComponentTypeBlock->enclosureType,  
                          attribute,
                          component,
                          length,
                          setString);
#endif

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "baseEncl setStr, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);  

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_find_first_edal_match_Bool(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t start_index,
 *                                fbe_bool_t check_value);
 *
 ***************************************************************************
 * @brief
 *       This routine find the first matching bool value from the EDAL  
 * for the specified component type, attribute, and the starting index.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The starting index.
 * @param   check_value    - The check value.
 *
 * @return  the first matching index.
 *       FBE_ENCLOSURE_VALUE_INVALID - failed or not found.
 *
 * HISTORY
 *   17-Nov-2008 NC - Created.
 ****************************************************************************/
fbe_u32_t
fbe_base_enclosure_find_first_edal_match_Bool(fbe_base_enclosure_t * base_enclosure,
                                              fbe_base_enclosure_attributes_t attribute,
                                              fbe_enclosure_component_types_t component,
                                              fbe_u32_t start_index,
                                              fbe_bool_t check_value)
{
    fbe_u32_t                   component_count, i;
    fbe_bool_t                  edal_value;
    fbe_edal_status_t           edal_status;
    fbe_u32_t                   matching_index = FBE_ENCLOSURE_VALUE_INVALID;

    edal_status = 
        fbe_edal_getSpecificComponentCount(base_enclosure->enclosureComponentTypeBlock,
                                           component,
                                           &component_count);
    /* find the type */
    if (edal_status == FBE_EDAL_STATUS_OK)
    {
        /* let loop through the rest of the components to see if there's a match */
        for (i = start_index; i<component_count; i++)
        {
            edal_status = 
                fbe_edal_getBool(base_enclosure->enclosureComponentTypeBlock,
                                 attribute,
                                 component,
                                 i,
                                 &edal_value);
            if ((edal_status == FBE_EDAL_STATUS_OK) && (edal_value == check_value))
            {
                matching_index = i;  /* if we have found match */
                break; /* found */
            }
        }
    }

    return (matching_index);
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_find_first_edal_match_U8(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t start_index,
 *                                fbe_u8_t check_value);
 *
 ***************************************************************************
 * @brief
 *       This routine find the first matching U8 value from the EDAL  
 * for the specified component type, attribute, and the starting index.
 *
 * @param   base_enclosure - The pointer to the enclosure. 
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The starting index.
 * @param   check_value    - The check value.
 *
 * @return  the first matching index.
 *       FBE_ENCLOSURE_VALUE_INVALID - failed or not found.
 *
 * HISTORY
 *   17-Nov-2008 NC - Created.
 ****************************************************************************/
fbe_u32_t
fbe_base_enclosure_find_first_edal_match_U8(fbe_base_enclosure_t * base_enclosure,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_enclosure_component_types_t component,
                                            fbe_u32_t start_index,
                                            fbe_u8_t check_value)
{
    fbe_u32_t                   matching_index = FBE_ENCLOSURE_VALUE_INVALID;    
    fbe_enclosure_component_block_t * encl_component_block = NULL;
    fbe_status_t                status = FBE_STATUS_OK;   

    status = fbe_base_enclosure_get_component_block_ptr(base_enclosure,
                                                        &encl_component_block);

    if(status == FBE_STATUS_OK)
    {

        matching_index = fbe_edal_find_first_edal_match_U8(encl_component_block,
                                                           attribute,
                                                           component,
                                                           start_index,
                                                           check_value);

    }
    return (matching_index);
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_find_first_edal_match_U16(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t start_index,
 *                                fbe_u16_t check_value);
 *
 ***************************************************************************
 * @brief
 *       This routine find the first matching U16 value from the EDAL  
 * for the specified component type, attribute, and the starting index.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The starting index.
 * @param   check_value    - The check value.
 *
 * @return  the first matching index.
 *       FBE_ENCLOSURE_VALUE_INVALID - failed or not found.
 *
 * HISTORY
 *   17-Nov-2008 NC - Created.
 ****************************************************************************/
fbe_u32_t
fbe_base_enclosure_find_first_edal_match_U16(fbe_base_enclosure_t * base_enclosure,
                                             fbe_base_enclosure_attributes_t attribute,
                                             fbe_enclosure_component_types_t component,
                                             fbe_u32_t start_index,
                                             fbe_u16_t check_value)
{
    fbe_u32_t                   component_count, i, matching_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u16_t                   edal_value;
    fbe_edal_status_t           edal_status;

    edal_status = 
        fbe_edal_getSpecificComponentCount(base_enclosure->enclosureComponentTypeBlock,
                                           component,
                                           &component_count);
    /* find the type */
    if (edal_status == FBE_EDAL_STATUS_OK)
    {
        /* let loop through the rest of the components to see if there's a match */
        for (i = start_index; i<component_count; i++)
        {
            edal_status = 
                fbe_edal_getU16(base_enclosure->enclosureComponentTypeBlock,
                                 attribute,
                                 component,
                                 i,
                                 &edal_value);
            if ((edal_status == FBE_EDAL_STATUS_OK) && (edal_value == check_value))
            {
                matching_index = i;  /* if we have found match */
                break; /* found */
            }
        }
    }

    return (matching_index);
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_find_first_edal_match_U32(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t start_index,
 *                                fbe_u32_t check_value);
 *
 ***************************************************************************
 * @brief
 *       This routine find the first matching U32 value from the EDAL  
 * for the specified component type, attribute, and the starting index.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The starting index.
 * @param   check_value    - The check value.
 *
 * @return  the first matching index.
 *       FBE_ENCLOSURE_VALUE_INVALID - failed or not found.
 *
 * HISTORY
 *   17-Nov-2008 NC - Created.
 ****************************************************************************/
fbe_u32_t
fbe_base_enclosure_find_first_edal_match_U32(fbe_base_enclosure_t * base_enclosure,
                                             fbe_base_enclosure_attributes_t attribute,
                                             fbe_enclosure_component_types_t component,
                                             fbe_u32_t start_index,
                                             fbe_u32_t check_value)
{
    fbe_u32_t                   component_count, i, matching_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u32_t                   edal_value;
    fbe_edal_status_t           edal_status;

    edal_status = 
        fbe_edal_getSpecificComponentCount(base_enclosure->enclosureComponentTypeBlock,
                                           component,
                                           &component_count);
    /* find the type */
    if (edal_status == FBE_EDAL_STATUS_OK)
    {
        /* let loop through the rest of the components to see if there's a match */
        for (i = start_index; i<component_count; i++)
        {
            edal_status = 
                fbe_edal_getU32(base_enclosure->enclosureComponentTypeBlock,
                                 attribute,
                                 component,
                                 i,
                                 &edal_value);
            if ((edal_status == FBE_EDAL_STATUS_OK) && (edal_value == check_value))
            {
                matching_index = i;  /* if we have found match */
                break; /* found */
            }
        }
    }

    return (matching_index);
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_find_first_edal_match_U64(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t start_index,
 *                                fbe_u64_t check_value);
 *
 ***************************************************************************
 * @brief
 *       This routine find the first matching U64 value from the EDAL  
 * for the specified component type, attribute, and the starting index.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The starting index.
 * @param   check_value    - The check value.
 *
 * @return  the first matching index.
 *       FBE_ENCLOSURE_VALUE_INVALID - failed or not found.
 *
 * HISTORY
 *   17-Nov-2008 NC - Created.
 ****************************************************************************/
fbe_u32_t
fbe_base_enclosure_find_first_edal_match_U64(fbe_base_enclosure_t * base_enclosure,
                                             fbe_base_enclosure_attributes_t attribute,
                                             fbe_enclosure_component_types_t component,
                                             fbe_u32_t start_index,
                                             fbe_u64_t check_value)
{
    fbe_u32_t                   component_count, i, matching_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u64_t                   edal_value;
    fbe_edal_status_t           edal_status;

    edal_status = 
        fbe_edal_getSpecificComponentCount(base_enclosure->enclosureComponentTypeBlock,
                                           component,
                                           &component_count);
    /* find the type */
    if (edal_status == FBE_EDAL_STATUS_OK)
    {
        /* let loop through the rest of the components to see if there's a match */
        for (i = start_index; i<component_count; i++)
        {
            edal_status = 
                fbe_edal_getU64(base_enclosure->enclosureComponentTypeBlock,
                                 attribute,
                                 component,
                                 i,
                                 &edal_value);
            if ((edal_status == FBE_EDAL_STATUS_OK) && (edal_value == check_value))
            {
                matching_index = i;  /* if we have found match */
                break; /* found */
            }
        }
    }

    return (matching_index);
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getOverallStateChangeCount(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_u32_t *returnValuePtr);
 *
 ***************************************************************************
 * @brief
 *       This routine gets the OverallStateChange value from the EDAL 
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   returnValuePtr (out) - The pointer to the returned value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   15-Jan-2009     Joe Perry - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getOverallStateChangeCount(fbe_base_enclosure_t * base_enclosure,
                                              fbe_u32_t *returnValuePtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    edalStatus = fbe_edal_getOverallStateChangeCount(base_enclosure->enclosureComponentTypeBlock,
                                                     returnValuePtr);
    if (edalStatus != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                             "%s, unhandled status %d\n", 
                            __FUNCTION__, edalStatus);
       
        retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    }

    return retStatus;
}
/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getEnclosureSide(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_u8_t *returnValuePtr);
 *
 ***************************************************************************
 * @brief
 *       This routine gets the enclosure Side value from the EDAL 
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   returnValuePtr (out) - The pointer to the returned value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   15-Jan-2009     Joe Perry - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getEnclosureSide(fbe_base_enclosure_t * base_enclosure,
                                         fbe_u8_t *returnValuePtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    edalStatus = fbe_edal_getEnclosureSide(base_enclosure->enclosureComponentTypeBlock,
                                           returnValuePtr);
    if (edalStatus != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                              "%s, unhandled status %d\n", 
                            __FUNCTION__, edalStatus);

        
        retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getEnclosureType(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_u8_t *returnValuePtr);
 *
 ***************************************************************************
 * @brief
 *       This routine gets the enclosure Side value from the EDAL 
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   fbe_enclosure_types_t (out) - The pointer to the returned value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   17-Jan-2011: PHE - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getEnclosureType(fbe_base_enclosure_t * base_enclosure,
                                         fbe_enclosure_types_t *returnValuePtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    edalStatus = fbe_edal_getEnclosureType(base_enclosure->enclosureComponentTypeBlock,
                                           returnValuePtr);
    if (edalStatus != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                              "%s, unhandled status %d\n", 
                            __FUNCTION__, edalStatus);

        
        retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    }

    return retStatus;
}


/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_printUpdatedComponent(fbe_base_enclosure_t * base_enclosure,
 *                                                   fbe_enclosure_component_types_t component,
 *                                                   fbe_u32_t index)
 *
 ***************************************************************************
 * @brief
 *       This routine will perform the following checks:
 *           -check if trace level is appropriate
 *           -check if a state change occurred in this component
 *           -check that this is not the first status reported
 *       If these conditions are met, the components data will be traced.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   component      - The component type.
 * @param   index          - index to a specific component
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   24-Feb-2009     Joe Perry - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_printUpdatedComponent(fbe_base_enclosure_t * base_enclosure,
                                              fbe_enclosure_component_types_t component,
                                              fbe_u32_t index)
{
    fbe_bool_t              stateChange = FALSE;
    fbe_u32_t               overallStateChangeCount = 0;
    fbe_enclosure_status_t  encl_status;
    fbe_trace_level_t       trace_level;

    /*
     * Check enclosure object's trace level
     */
    trace_level = fbe_base_object_get_trace_level((fbe_base_object_t *)base_enclosure);
    if (trace_level < FBE_TRACE_LEVEL_DEBUG_LOW)
    {
        return FBE_ENCLOSURE_STATUS_OK;
    }

    /*
     * Check for state change & trace if detected
     */
    encl_status = fbe_base_enclosure_edal_getBool_thread_safe(base_enclosure,
                                                            FBE_ENCL_COMP_STATE_CHANGE,
                                                            component,
                                                            index,
                                                            &stateChange);
    if ((encl_status == FBE_ENCLOSURE_STATUS_OK) && stateChange)
    {
        /*
         * Do not trace on the very first state change (don't want all components tracing)
         */
        encl_status = 
            fbe_base_enclosure_edal_getOverallStateChangeCount(base_enclosure,
                                                               &overallStateChangeCount);
        if ((encl_status == FBE_ENCLOSURE_STATUS_OK) && 
            (overallStateChangeCount != 0))
        {
            fbe_edal_printSpecificComponentData(base_enclosure->enclosureComponentTypeBlock,
                                                component,
                                                index,
                                                FALSE,
                                                fbe_edal_trace_func);         // trace just HW status
        }
    }

    return FBE_ENCLOSURE_STATUS_OK;

}   // end of fbe_base_enclosure_edal_printUpdatedComponent

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getBool_thread_safe(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_bool_t *returnValuePtr);
 *
 ***************************************************************************
 * @brief
 *       This routine gets the bool value from the EDAL for the specified 
 * component type, attribute, and index
 *       This routine should be used from usurper context, so it won't 
 * interact with requests from within the object itself.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   returnValuePtr (out) - The pointer to the returned value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 * 
 * HISTORY
 *   27-Jul-2008 NC - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getBool_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_enclosure_component_types_t component,
                                            fbe_u32_t index,
                                            fbe_bool_t *returnValuePtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    if(base_enclosure == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
    edalStatus = fbe_edal_getBool(base_enclosure->enclosureComponentTypeBlock,
                                attribute,
                                component, 
                                index,
                                returnValuePtr);

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            __FUNCTION__, retStatus, enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);     

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_setBool_thread_safe(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_bool_t setValue);
 *
 ***************************************************************************
 * @brief
 *       This routine sets the bool value to the EDAL for the specified 
 * component type, attribute, and index.
 *       This routine should be used from usurper context, so it won't 
 * interact with requests from within the object itself.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   setValue       - The new value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - set successful.
 *
 * HISTORY
 *   27-Jul-2008 NC - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_setBool_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_enclosure_component_types_t component,
                                            fbe_u32_t index,
                                            fbe_bool_t setValue)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    if(base_enclosure == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
    edalStatus = fbe_edal_setBool(base_enclosure->enclosureComponentTypeBlock,  
                          attribute,
                          component,
                          index,
                          setValue);

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            __FUNCTION__, retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);  

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getU8_thread_safe(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u8_t *returnValuePtr);
 *
 ***************************************************************************
 * @brief
 *       This routine gets the U8 value from the EDAL for the specified 
 * component type, attribute, and index
 *       This routine should be used from usurper context, so it won't 
 * interact with requests from within the object itself.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   returnValuePtr (out) - The pointer to the returned value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   27-Jul-2008 NC - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getU8_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                          fbe_base_enclosure_attributes_t attribute,
                                          fbe_enclosure_component_types_t component,
                                          fbe_u32_t index,
                                          fbe_u8_t *returnValuePtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    if(base_enclosure == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }

    edalStatus = fbe_edal_getU8(base_enclosure->enclosureComponentTypeBlock,  
                                attribute,
                                component, 
                                index,
                                returnValuePtr);

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            __FUNCTION__, retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index); 

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_setU8_thread_safe(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u8_t setValue);
 *
 ***************************************************************************
 * @brief
 *       This routine sets the U8 value to the EDAL for the specified 
 * component type, attribute, and index.
 *       This routine should be used from usurper context, so it won't 
 * interact with requests from within the object itself.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   setValue       - The new value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - set successful.
 *
 * HISTORY
 *   27-Jul-2008 NC - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_setU8_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                          fbe_base_enclosure_attributes_t attribute,
                                          fbe_enclosure_component_types_t component,
                                          fbe_u32_t index,
                                          fbe_u8_t setValue)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    if(base_enclosure == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }

    edalStatus = fbe_edal_setU8(base_enclosure->enclosureComponentTypeBlock,  
                                attribute,
                                component,
                                index,
                                setValue);
    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            __FUNCTION__, retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);  

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getU16_thread_safe(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u16_t *returnValuePtr);
 *
 ***************************************************************************
 * @brief
 *       This routine gets the U16 value from the EDAL for the specified 
 * component type, attribute, and index
 *       This routine should be used from usurper context, so it won't 
 * interact with requests from within the object itself.
 * 
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   returnValuePtr (out) - The pointer to the returned value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   27-Jul-2008 NC - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getU16_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                           fbe_base_enclosure_attributes_t attribute,
                                           fbe_enclosure_component_types_t component,
                                           fbe_u32_t index,
                                           fbe_u16_t *returnValuePtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    if(base_enclosure == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
    
    edalStatus = fbe_edal_getU16(base_enclosure->enclosureComponentTypeBlock,  
                                attribute,
                                component, 
                                index,
                                returnValuePtr);

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            __FUNCTION__, retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index); 

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_setU16_thread_safe(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u16_t setValue);
 *
 ***************************************************************************
 * @brief
 *       This routine sets the U16 value to the EDAL for the specified 
 * component type, attribute, and index.
 *       This routine should be used from usurper context, so it won't 
 * interact with requests from within the object itself.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   setValue       - The new value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - set successful.
 *
 * HISTORY
 *   27-Jul-2008 NC - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_setU16_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                           fbe_base_enclosure_attributes_t attribute,
                                           fbe_enclosure_component_types_t component,
                                           fbe_u32_t index,
                                           fbe_u16_t setValue)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    if(base_enclosure == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }

    edalStatus = fbe_edal_setU16(base_enclosure->enclosureComponentTypeBlock,  
                          attribute,
                          component,
                          index,
                          setValue);

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            __FUNCTION__, retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);  

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getU32_thread_safe(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u32_t *returnValuePtr);
 *
 ***************************************************************************
 * @brief
 *       This routine gets the U32 value from the EDAL for the specified 
 * component type, attribute, and index
 *       This routine should be used from usurper context, so it won't 
 * interact with requests from within the object itself.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   returnValuePtr (out) - The pointer to the returned value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   27-Jul-2008 NC - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getU32_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                           fbe_base_enclosure_attributes_t attribute,
                                           fbe_enclosure_component_types_t component,
                                           fbe_u32_t index,
                                           fbe_u32_t *returnValuePtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    if(base_enclosure == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }

    edalStatus = fbe_edal_getU32(base_enclosure->enclosureComponentTypeBlock,  
                                attribute,
                                component, 
                                index,
                                returnValuePtr);

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            __FUNCTION__, retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_setU32_thread_safe(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u32_t setValue);
 *
 ***************************************************************************
 * @brief
 *       This routine sets the U32 value to the EDAL for the specified 
 * component type, attribute, and index.
 *       This routine should be used from usurper context, so it won't 
 * interact with requests from within the object itself.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   setValue       - The new value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - set successful.
 *
 * HISTORY
 *   27-Jul-2008 NC - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_setU32_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                           fbe_base_enclosure_attributes_t attribute,
                                           fbe_enclosure_component_types_t component,
                                           fbe_u32_t index,
                                           fbe_u32_t setValue)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    if(base_enclosure == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
    
    edalStatus = fbe_edal_setU32(base_enclosure->enclosureComponentTypeBlock,  
                          attribute,
                          component,
                          index,
                          setValue);

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            __FUNCTION__, retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);  

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getU64_thread_safe(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u64_t *returnValuePtr);
 *
 ***************************************************************************
 * @brief
 *       This routine gets the U64 value from the EDAL for the specified 
 * component type, attribute, and index
 *       This routine should be used from usurper context, so it won't 
 * interact with requests from within the object itself.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   returnValuePtr (out) - The pointer to the returned value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   27-Jul-2008 NC - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getU64_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                           fbe_base_enclosure_attributes_t attribute,
                                           fbe_enclosure_component_types_t component,
                                           fbe_u32_t index,
                                           fbe_u64_t *returnValuePtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    if(base_enclosure == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }

    edalStatus = fbe_edal_getU64(base_enclosure->enclosureComponentTypeBlock,  
                                attribute,
                                component, 
                                index,
                                returnValuePtr);

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            __FUNCTION__, retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);  

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_setU64_thread_safe(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u64_t setValue);
 *
 ***************************************************************************
 * @brief
 *       This routine sets the U64 value to the EDAL for the specified 
 * component type, attribute, and index.
 *       This routine should be used from usurper context, so it won't 
 * interact with requests from within the object itself.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   setValue       - The new value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - set successful.
 *
 * HISTORY
 *   27-Jul-2008 NC - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_setU64_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                           fbe_base_enclosure_attributes_t attribute,
                                           fbe_enclosure_component_types_t component,
                                           fbe_u32_t index,
                                           fbe_u64_t setValue)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    if(base_enclosure == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }
    
    edalStatus = fbe_edal_setU64(base_enclosure->enclosureComponentTypeBlock,  
                          attribute,
                          component,
                          index,
                          setValue);

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            __FUNCTION__, retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);  

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getStr_thread_safe(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                char *returnStringPtr);
 *
 ***************************************************************************
 * @brief
 *       This routine copies up to the length bytes of chars to the request location
 * from the EDAL for the specified component type, attribute, and index
 *       This routine should be used from usurper context, so it won't 
 * interact with requests from within the object itself.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   length         - The length of the string requested.
 * @param   returnStringPtr (out) - The pointer to the returned string location.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   27-Jul-2008 NC - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getStr_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                           fbe_base_enclosure_attributes_t attribute,
                                           fbe_enclosure_component_types_t component,
                                           fbe_u32_t index,
                                           fbe_u32_t length,
                                           char *returnStringPtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    if(base_enclosure == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }

    edalStatus = fbe_edal_getStr(base_enclosure->enclosureComponentTypeBlock,  
                                attribute,
                                component,
                                index,
                                length,
                                returnStringPtr);

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            __FUNCTION__, retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);  

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_setStr_thread_safe(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_enclosure_component_types_t component,
 *                                fbe_u32_t index,
 *                                fbe_u32_t length,
 *                                char *setString);
 *
 ***************************************************************************
 * @brief
 *       This routine copies string to the EDAL for the specified 
 * component type, attribute, and index.
 *       This routine should be used from usurper context, so it won't 
 * interact with requests from within the object itself.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   component      - The component type.
 * @param   index          - The component index.
 * @param   length         - The length of the string requested.
 * @param   setString      - The new string pointer.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - set successful.
 *
 * HISTORY
 *   27-Jul-2008 NC - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_setStr_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                           fbe_base_enclosure_attributes_t attribute,
                                           fbe_enclosure_component_types_t component,
                                           fbe_u32_t index,
                                           fbe_u32_t length,
                                           char *setString)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    if(base_enclosure == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }

    edalStatus = fbe_edal_setStr(base_enclosure->enclosureComponentTypeBlock,  
                          attribute,
                          component,
                          index,
                          length,
                          setString);

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, unhandled stat %d, comp type %s, attr %s, idx %d.\n", 
                            __FUNCTION__, retStatus,
                            enclosure_access_printComponentType(component),
                            enclosure_access_printAttribute(attribute),
                            index);  

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;
}
/*!*************************************************************************
 *  @fn fbe_base_enclosure_get_packet_payload_control_data( )
 **************************************************************************
 *  @brief
 *      This function processes the payload ,control operation,return bufferptr. 
 *      
 *  @param    baseEncltPtr - The pointer to the enclosure.
 *  @param    fbe_packet_t - pointer to control code packet.
 *  @param    use_current_operation -bool
 *  @param    bufferptr -The pointer to the buffer
 *  @param    user_request_length -The sizeof the buffer 
 *
 * 
 *  @return   fbe_status_t
 *       FBE_STATUS_OK - no error.
 *       Otherwise, error found.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    13-Sept-2009:  -Nayana  Created
 *
 **************************************************************************/
fbe_status_t
fbe_base_enclosure_get_packet_payload_control_data( fbe_packet_t * packetPtr,
                                               fbe_base_enclosure_t *baseEncltPtr,
                                               BOOL use_current_operation,
                                               fbe_payload_control_buffer_t  *bufferPtr,
                                               fbe_u32_t  user_request_length)
{
    fbe_payload_ex_t    *payload = NULL;
    fbe_payload_control_buffer_length_t         bufferLength = 0;
    fbe_payload_control_operation_t             *control_operation = NULL;
    fbe_status_t     status = FBE_STATUS_OK;
   
    if(packetPtr == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)baseEncltPtr,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(baseEncltPtr),
                            "%s, Null packetPtr or baseEnclPtr.\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packetPtr);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)baseEncltPtr,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(baseEncltPtr),
                            "%s, get_payload failed.\n", __FUNCTION__); 

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if(use_current_operation == TRUE)
    {
        control_operation = fbe_payload_ex_get_control_operation(payload);    
    }
    else
    {
        control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    }

    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)baseEncltPtr,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(baseEncltPtr),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate the bufferPtr */
    if(bufferPtr == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)baseEncltPtr,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader(baseEncltPtr),
                                "%s, return bufferPtr is NULL\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Get Control data from packet payload and validate buffer pointer & size
     */
    status = fbe_payload_control_get_buffer(control_operation, bufferPtr); 
    if ((status != FBE_STATUS_OK) ||(bufferPtr == NULL))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)baseEncltPtr,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(baseEncltPtr),
                            "%s, invalid control buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_payload_control_get_buffer_length(control_operation, &bufferLength); 
    if ((status != FBE_STATUS_OK) || (bufferLength != user_request_length))
    {
         fbe_base_object_customizable_trace((fbe_base_object_t*)baseEncltPtr,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(baseEncltPtr),
                            "%s, %X len invalid\n", __FUNCTION__, bufferLength);

        return FBE_STATUS_GENERIC_FAILURE;
    }

   
    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_enclosure_set_packet_payload_status( )
 **************************************************************************
 *  @brief
 *      This function setup return status for the packet and control operation. 
 *      
 *  @param    baseEncltPtr - The pointer to the enclosure.
 *  @param    encl_stat - enclosure status.
 *
 * 
 *  @return   fbe_status_t
 *       FBE_STATUS_OK - no error.
 *       Otherwise, error found.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    09-Oct-2009:  -Naizhong  Created
 *
 **************************************************************************/
fbe_status_t
fbe_base_enclosure_set_packet_payload_status( fbe_packet_t * packetPtr,
                                             fbe_enclosure_status_t encl_stat)
{
    fbe_payload_ex_t    *payload = NULL;
    fbe_payload_control_operation_t             *control_operation = NULL;
    fbe_status_t     status;

    if(packetPtr == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packetPtr);
    if(payload == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);    
    if(control_operation == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (encl_stat)
    {
    case FBE_ENCLOSURE_STATUS_OK:
        status = FBE_STATUS_OK;
        break;
    case FBE_ENCLOSURE_STATUS_BUSY:
        status = FBE_STATUS_BUSY;
        break;
    default:
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    /* control operation status OK */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    /* packet status */
    fbe_transport_set_status(packetPtr, status, 0);

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_enclosure_copy_edal_to_backup( )
 **************************************************************************
 *  @brief
 *      This function copy active EDAL to backup EDAL. 
 *      
 *  @param    base_enclosure - The pointer to the enclosure.
 *
 * 
 *  @return   fbe_status_t
 *       FBE_EDAL_STATUS_OK - no error.
 *       Otherwise, error found.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    06-Jan-2010:  -Dipak Patel  Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_base_enclosure_copy_edal_to_backup(fbe_base_enclosure_t * base_enclosure)
{
    fbe_edal_status_t edal_status = FBE_EDAL_STATUS_OK;       
    
    edal_status = fbe_edal_copy_backup_data(base_enclosure->enclosureComponentTypeBlock,
                                                                         base_enclosure->enclosureComponentTypeBlock_backup);
    return edal_status;
}

/*!**************************************************************************
 * @fn fbe_base_enclosure_record_control_opcode(
 *      fbe_object_handle_t object_handle, 
 *      fbe_payload_control_operation_opcode_t control_code)
 ***************************************************************************
 *
 * @brief
 *       Function to set the control_code attributes.
 * 
 * @param  base_enclosure - our object context
 * @param  control_operation_opcode - The control operation opcode
 *
 * @return
 *       fbe_status_t.
 *
 * NOTES
 *
 * HISTORY
 *   08-Sep-2010 Prasenjeet - Created.
 ***************************************************************************/
 fbe_status_t
fbe_base_enclosure_record_control_opcode(fbe_base_enclosure_t * base_enclosure, 
                                                                                        fbe_payload_control_operation_opcode_t control_code)
{
    base_enclosure->control_code = control_code;
   return FBE_STATUS_OK;        
}


/*!**************************************************************************
 * @fn fbe_base_enclosure_record_discovery_transport_opcode(
 *       fbe_base_object_t * base_object,
 *       fbe_payload_discovery_opcode_t discovery_opcode)
 ***************************************************************************
 *
 * @brief
 *       Function to set the discovery_opcode attributes.
 * 
 * @param  base_enclosure - our object context
 * @param  discovery_opcode - The discovery opcode
 *
 * @return
 *       fbe_status_t.
 *
 * NOTES
 *
 * HISTORY
 *   08-Sep-2010 Prasenjeet - Created.
 ***************************************************************************/
 fbe_status_t
fbe_base_enclosure_record_discovery_transport_opcode(fbe_base_enclosure_t * base_enclosure,
                                                                                                                fbe_payload_discovery_opcode_t discovery_opcode)
{
    base_enclosure->discovery_opcode = discovery_opcode;
    return FBE_STATUS_OK;       
}



/*!*************************************************************************
 * @fn fbe_base_enclosure_allocate_buffer()
 **************************************************************************
 *
 *  @brief:
 *      This function will allocate a buffer that will be used by an
 *      enclosure object.
 *
 *  @param    void
 *
 *  @return   Pointer to the newly allocated buffer.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    23-Jul-2008: Joe Perry - Created
 *    05-Mar-2010: NCHIU - moved to base_enclosure from eses
 *
 **************************************************************************/
void *
fbe_base_enclosure_allocate_buffer(void)
{
    fbe_memory_request_t memory_request;
    fbe_semaphore_t sem;

    fbe_semaphore_init(&sem, 0, 1);

    memory_request.request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_BUFFER;
    memory_request.number_of_objects = 1;
    memory_request.ptr = NULL;
    memory_request.completion_function = fbe_base_enclosure_allocate_completion;
    memory_request.completion_context = &sem;
    fbe_memory_allocate(&memory_request);

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    return memory_request.ptr;

}   // end of old fbe_eses_allocate_buffer

/*!*************************************************************************
 *  @fn fbe_base_enclosure_allocate_completion(
 *                   fbe_memory_request_t * memory_request, 
 *                   fbe_memory_completion_context_t context)
 **************************************************************************
 *
 *  @brief
 *      This function will cleanup (release semaphore) after an
 *      enclosure object's buffer allocation has completed.
 *
 *  @param    memory_request
 *  @param    context
 *
 *  @return   void
 *
 *  NOTES:
 *
 *  HISTORY:
 *    23-Jul-2008: Joe Perry - Created
 *    05-Mar-2010: NCHIU - moved to base_enclosure from eses
 *
 **************************************************************************/
static void
fbe_base_enclosure_allocate_completion(fbe_memory_request_t * memory_request, 
                                  fbe_memory_completion_context_t context)
{

    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);

}   // end of old fbe_eses_allocate_completion

/*!*************************************************************************
 * @fn fbe_sas_bunker_allocate_encl_data_block
 **************************************************************************
 *
 *  @brief
 *      This function allocates and initializes the enclosure data block.
 *
 *  @return   fbe_enclosure_component_block_t - pointer to allocated block.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    08-Jun-2009: Dipak Patel - Created
 *    08-Jun-2009: Dipak Patel - Created from fbe_sas_magnum_allocate_encl_data_block.
 *    05-Mar-2010: NCHIU - moved to base_enclosure
 *
 **************************************************************************/
fbe_enclosure_component_block_t
*fbe_base_enclosure_allocate_encl_data_block(fbe_enclosure_types_t enclosureType,
                                             fbe_u8_t componentId,
                                             fbe_u8_t maxNumberComponentTypes)
{
    fbe_enclosure_component_block_t   *base_enclosure_component_block;
    base_enclosure_component_block = fbe_base_enclosure_allocate_buffer();
    if (base_enclosure_component_block != NULL)
    {
        fbe_zero_memory(base_enclosure_component_block, FBE_MEMORY_CHUNK_SIZE);

        base_enclosure_component_block->enclosureType = enclosureType;
        base_enclosure_component_block->edalLocale = componentId;
        base_enclosure_component_block->enclosureBlockCanary = EDAL_BLOCK_CANARY_VALUE;
        base_enclosure_component_block->numberComponentTypes = 0;
        base_enclosure_component_block->maxNumberComponentTypes = maxNumberComponentTypes;
        base_enclosure_component_block->pNextBlock = NULL;
        /* beside this block header, we need to save FBE_SAS_BUNKER_ENCLOSURE_MAX_COMPONENT_TYPE number
        * of space for component descriptors
        */
        base_enclosure_component_block->blockSize = FBE_MEMORY_CHUNK_SIZE;
        base_enclosure_component_block->availableDataSpace  = 
            base_enclosure_component_block->blockSize - 
//            0x50 -  // TESTING: waste some space to force using 2 blocks
            (sizeof(fbe_enclosure_component_t) * base_enclosure_component_block->maxNumberComponentTypes) -
            (sizeof(fbe_enclosure_component_block_t));
    }

    return (base_enclosure_component_block);
}

/*!*************************************************************************
 *  @fn fbe_base_enclosure_process_state_change
 **************************************************************************
 *  @brief
 *      This function will determine whether a StateChange has occurred
 *      in any component of an Enclosure Object.
 *      If yes, send the notification.
 *      This function goes through the EDAL block chain. 
 *
 *  @param pBaseEncl - pointer to fbe_base_enclosure_t
 *
 *  @return fbe_status_t - status of process State Change operation
 *
 *  @note
 *
 *  @version
 *    29-Sep-2008: Joe Perry - Created
 *    17-Mar-2010: PHE - Modified for the component type which can be in multiple blocks.
 *    07-Jul-2010: PHE - Modified to notify the state change.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_enclosure_process_state_change(fbe_base_enclosure_t * pBaseEncl)
{
    fbe_enclosure_component_types_t componentType;
    fbe_u32_t                       componentIndex = 0;
    fbe_u32_t                       componentCount = 0;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_edal_status_t               edalStatus = FBE_EDAL_STATUS_OK;
    fbe_bool_t                      individualCompStateChange = FALSE;
    fbe_bool_t                      compTypeStateChange = FALSE;
    fbe_bool_t                      anyStateChange = FALSE;
    fbe_enclosure_component_block_t * enclComponentBlockPtr = NULL;
    fbe_edal_block_handle_t         edalBlockHandle = NULL;
    fbe_notification_data_changed_info_t dataChangeInfo = {0};
    fbe_u32_t                       stateChangeCount = 0;

    fbe_base_object_trace((fbe_base_object_t *)pBaseEncl,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry.\n", __FUNCTION__);

    fbe_base_enclosure_get_component_block_ptr(pBaseEncl, &enclComponentBlockPtr);

    edalBlockHandle = (fbe_edal_block_handle_t *)enclComponentBlockPtr;

    for(componentType = 0; componentType < FBE_ENCL_MAX_COMPONENT_TYPE; componentType++)
    {
        edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                            componentType,
                                                            &componentCount);

        if (edalStatus != FBE_EDAL_STATUS_OK)
        {
            return FBE_STATUS_FAILED;
        }  

        // check if any of this component type changed
        compTypeStateChange = FALSE;

        for (componentIndex = 0; componentIndex < componentCount; componentIndex++)
        {
            individualCompStateChange = FALSE;

            edalStatus = fbe_edal_getBool(edalBlockHandle,
                                        FBE_ENCL_COMP_STATE_CHANGE,
                                        componentType,
                                        componentIndex,
                                        &individualCompStateChange);

            // if state change, record it & clear flag
            if ((edalStatus == FBE_EDAL_STATUS_OK) && individualCompStateChange)
            {
                anyStateChange = TRUE;
                compTypeStateChange = TRUE;
                edalStatus = fbe_edal_setBool(edalBlockHandle,
                                            FBE_ENCL_COMP_STATE_CHANGE,
                                            componentType,
                                            componentIndex,
                                            FALSE);

                // send notification. 
                status = fbe_base_enclosure_get_data_change_info(pBaseEncl,
                                                         componentType, 
                                                         componentIndex,
                                                         &dataChangeInfo);
                if(status != FBE_STATUS_GENERIC_FAILURE)
                {
                    fbe_base_enclosure_send_data_change_notification(pBaseEncl, &dataChangeInfo); 
                }
            } 
        } // End of - for (componentIndex = 0; componentIndex < componentCount; componentIndex++)

        // if state change to this component type, increment state change count for the component type
        if (compTypeStateChange)
        {
            fbe_edal_incrementComponentOverallStateChange(edalBlockHandle, componentType);
        }

    } //End of -for(componentType = 0; componentType < FBE_ENCL_MAX_COMPONENT_TYPE; componentType++)

    if (anyStateChange)
    {
        fbe_edal_incrementOverallStateChange(edalBlockHandle);

        edalStatus = fbe_edal_getOverallStateChangeCount(edalBlockHandle, &stateChangeCount);

        if (edalStatus != FBE_EDAL_STATUS_OK) 
        {
             fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEncl,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(pBaseEncl),
                            "%s error checking for Overall Enclosure state change, status: 0x%X",
                             __FUNCTION__, edalStatus);
           
        }
        else
        {
             fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEncl,
                            FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                            fbe_base_enclosure_get_logheader(pBaseEncl),
                            "%s State Change Count %d\n", 
                            __FUNCTION__, stateChangeCount);
        }
    }

    return FBE_STATUS_OK;

}   // end of fbe_base_enclosure_process_state_change

/*!*************************************************************************
* @fn fbe_base_enclosure_send_data_change_notification(fbe_base_enclosure_t *pBaseEncl, 
*                                         fbe_notification_data_changed_info_t * pDataChangeInfo)
***************************************************************************
*
* @brief
*       This function send an event notification for the specified device
*       from the Enclosure Object.
*
* @param      pBaseEncl - The pointer to the fbe_base_enclosure_t.
* @param      pDataChangeInfo - 
*
* @return     void.
*
* @note
*
* @version
*   27-Apr-2010     Joe Perry - Created.
*   07-Jul-2010 PHE - Changed the function arguments.
*
***************************************************************************/
void fbe_base_enclosure_send_data_change_notification(fbe_base_enclosure_t *pBaseEncl, 
                                                      fbe_notification_data_changed_info_t * pDataChangeInfo)
{
    fbe_notification_info_t     notification;
    fbe_object_id_t             objectId;
    
    fbe_base_object_get_object_id((fbe_base_object_t *)pBaseEncl, &objectId);

    fbe_set_memory(&notification, 0, sizeof(fbe_notification_info_t));
    notification.notification_type = FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED;
        notification.class_id = fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *) pBaseEncl));    
    fbe_topology_class_id_to_object_type(notification.class_id, &notification.object_type);
    notification.notification_data.data_change_info = *pDataChangeInfo;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEncl,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        fbe_base_enclosure_get_logheader(pBaseEncl),
                                        "Sending object data change notification, dataType 0x%X, slot %d, deviceMask 0x%llX.\n", 
                                        pDataChangeInfo->data_type,
                                        pDataChangeInfo->phys_location.slot,
                                        pDataChangeInfo->device_mask);

    fbe_notification_send(objectId, notification);

    return;

}   // end of fbe_base_enclosure_send_data_change_notification



/*!*************************************************************************
 *   @fn fbe_base_encloosure_get_data_change_info(fbe_base_board_t * pBaseBoard,,
 *                             fbe_enclosure_component_types_t componentType,
 *                             fbe_u32_t componentIndex,
 *                             fbe_notification_data_changed_info_t * pDataChangeInfo)     
 **************************************************************************
 *  @brief
 *      This function will get the necessary data change info 
 *      for state change notification.
 *
 *  @param pBaseEncl - The pointer to the fbe_base_enclosure_t.
 *  @param componentType - The EDAL component type.
 *  @param componentIndex - The EDAL component index.
 *  @param pDataChangeInfo - The pointer to the data change info for data change notification.
 *
 *  @return fbe_status_t 
 *              FBE_STATUS_OK - successful, 
 *              otherwise - failed or no need to send data change notification for the component.
 *
 *  @version
 *    07-Jul-2010: PHE - Created.
 *    01-Feb-2012: Rui Chang  - Add code for processing Temp Sensor data change.
 *
 **************************************************************************/
static fbe_status_t
fbe_base_enclosure_get_data_change_info(fbe_base_enclosure_t * pBaseEncl,
                                fbe_enclosure_component_types_t componentType,
                                fbe_u32_t componentIndex,
                                fbe_notification_data_changed_info_t * pDataChangeInfo) 
{
    fbe_u32_t              port = 0;
    fbe_u32_t              enclosure = 0;
    fbe_u8_t               slot = 0;
    fbe_status_t           status = FBE_STATUS_OK;
    fbe_edal_status_t      edalStatus = FBE_EDAL_STATUS_OK;
    fbe_enclosure_component_block_t * enclComponentBlockPtr = NULL;
    fbe_edal_block_handle_t           edalBlockHandle = NULL;
    fbe_u8_t               subType = FBE_ENCL_COOLING_SUBTYPE_INVALID;
    fbe_bool_t             isEntireConnector = FBE_FALSE;
    fbe_u8_t               compLocation = 0;
    fbe_u8_t               sideId = 0;
    fbe_u64_t      lccDeviceType = FBE_ENCL_TYPE_INVALID;
    fbe_u8_t               containerIndex = 0;
    fbe_u32_t              firstCompIndex = 0;

     
    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEncl,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        fbe_base_enclosure_get_logheader(pBaseEncl),
                        "%s entry, compType %s, compIndex %d.\n", 
                        __FUNCTION__,
                        enclosure_access_printComponentType(componentType),
                        componentIndex);

    fbe_set_memory(pDataChangeInfo, 0xFF, sizeof(fbe_notification_data_changed_info_t));

    /* Setting physical location to zero. So it will avoid failure while comparing physical location
       during resume prom operation at ESP */
    fbe_zero_memory(&(pDataChangeInfo->phys_location),sizeof(fbe_device_physical_location_t));

    fbe_base_enclosure_get_port_number(pBaseEncl, &port);
    fbe_base_enclosure_get_enclosure_number(pBaseEncl, &enclosure);

    pDataChangeInfo->phys_location.bus = port;
    pDataChangeInfo->phys_location.enclosure = enclosure;
    pDataChangeInfo->phys_location.componentId = fbe_base_enclosure_get_component_id(pBaseEncl);
    pDataChangeInfo->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    fbe_base_enclosure_get_component_block_ptr(pBaseEncl, &enclComponentBlockPtr);
    edalBlockHandle = (fbe_edal_block_handle_t *)enclComponentBlockPtr;

    switch(componentType)
    {
        case FBE_ENCL_POWER_SUPPLY:
            edalStatus = fbe_edal_getU8(edalBlockHandle, 
                                FBE_ENCL_COMP_SIDE_ID,  //Attribute
                                componentType,  // component type
                                componentIndex,  // comonent index
                                &sideId);  // return value

            if(!EDAL_STAT_OK(edalStatus))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEncl,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_enclosure_get_logheader(pBaseEncl),
                                        "Failed to get sideId for PS, index %d, %s.\n", 
                                        componentIndex,
                                        __FUNCTION__);

                pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
                status = FBE_STATUS_GENERIC_FAILURE;
            }   
            else
            {
                pDataChangeInfo->phys_location.slot = sideId;
                pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_PS;
            }
            break;

        case FBE_ENCL_COOLING_COMPONENT:
            edalStatus = fbe_edal_getU8(edalBlockHandle, 
                                FBE_ENCL_COMP_SIDE_ID,  //Attribute
                                componentType,  // component type
                                componentIndex,  // comonent index
                                &sideId);  // return value

            if(!EDAL_STAT_OK(edalStatus))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEncl,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_enclosure_get_logheader(pBaseEncl),
                                        "Failed to get sideId for FAN, index %d, %s.\n", 
                                        componentIndex,
                                        __FUNCTION__);

                pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                edalStatus = fbe_edal_getU8(edalBlockHandle, 
                                    FBE_ENCL_COMP_SUBTYPE,  //Attribute
                                    componentType,  // component type
                                    componentIndex,  // comonent index
                                    &subType);  // return value
    
                if(!EDAL_STAT_OK(edalStatus)) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEncl,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_enclosure_get_logheader(pBaseEncl),
                                        "Failed to get subType for FAN, index %d, %s.\n", 
                                        componentIndex,
                                        __FUNCTION__);

                    pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
                    status = FBE_STATUS_GENERIC_FAILURE;
                }
                else if(subType == FBE_ENCL_COOLING_SUBTYPE_PS_INTERNAL) 
                {
                    pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_PS;
                    pDataChangeInfo->phys_location.slot = sideId;
                }
                else if(subType == FBE_ENCL_COOLING_SUBTYPE_STANDALONE) 
                {
                    pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_FAN;
                    pDataChangeInfo->phys_location.slot = sideId;
                }
                else if(subType == FBE_ENCL_COOLING_SUBTYPE_LCC_INTERNAL) 
                {
                    pDataChangeInfo->phys_location.sp = sideId;
                    pDataChangeInfo->phys_location.slot = 0;
                    pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_BACK_END_MODULE;
                }
                else if(subType == FBE_ENCL_COOLING_SUBTYPE_CHASSIS)
                {
                    edalStatus = fbe_edal_getU8(edalBlockHandle, 
                                    FBE_ENCL_COMP_CONTAINER_INDEX,  //Attribute
                                    componentType,  // component type
                                    componentIndex,  // comonent index
                                    &containerIndex);  // return value
                    if(!EDAL_STAT_OK(edalStatus)) 
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEncl,
                                            FBE_TRACE_LEVEL_INFO,
                                            fbe_base_enclosure_get_logheader(pBaseEncl),
                                            "Failed to get containerIndex for FAN, index %d, %s.\n", 
                                            componentIndex,
                                            __FUNCTION__);
    
                        pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
                        status = FBE_STATUS_GENERIC_FAILURE;
                    }
                    else if(containerIndex == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED) 
                    {
                        /* No need to notify the change. This is the overall element. */
                        pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
                        status = FBE_STATUS_GENERIC_FAILURE;
                    }
                    else
                    {


                        pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_FAN;
                        /* The first one is overall element. Then offset by first Chassis component*/ 
                        firstCompIndex = fbe_edal_find_first_edal_match_U8(edalBlockHandle, 
                                            FBE_ENCL_COMP_SUBTYPE,  //Attribute
                                            componentType,  // component type
                                            0,  // Starting index
                                            FBE_ENCL_COOLING_SUBTYPE_CHASSIS);
                        pDataChangeInfo->phys_location.slot = (componentIndex - 1)-firstCompIndex; 
                    }
                }
            }
            break;

        case FBE_ENCL_LCC:
            edalStatus = fbe_edal_getU8(edalBlockHandle, 
                                FBE_ENCL_COMP_LOCATION,  //Attribute
                                componentType,  // component type
                                componentIndex,  // comonent index
                                &compLocation);  // return value

            if(!EDAL_STAT_OK(edalStatus))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEncl,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_enclosure_get_logheader(pBaseEncl),
                                        "Failed to get location for LCC, index %d, %s.\n", 
                                        componentIndex,
                                        __FUNCTION__);

                pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                if(port == 0 && enclosure == 0) 
                {
                    lccDeviceType = fbe_base_enclosure_get_lcc_device_type(pBaseEncl);
                     
                    if((lccDeviceType == FBE_DEVICE_TYPE_BACK_END_MODULE) ||
                       (lccDeviceType == FBE_DEVICE_TYPE_SP))  
                    {
                        pDataChangeInfo->phys_location.sp = compLocation;
                        pDataChangeInfo->phys_location.slot = 0;
                        pDataChangeInfo->device_mask = lccDeviceType;
                    }
                    else
                    {
                        pDataChangeInfo->phys_location.slot = compLocation;
                        pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_LCC;
                    }
                }
                else
                {
                    pDataChangeInfo->phys_location.slot = compLocation;
                    pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_LCC;
                }
            }
            break;
    
        case FBE_ENCL_ENCLOSURE:
            pDataChangeInfo->phys_location.slot = 0;
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_ENCLOSURE;
            break;

        case FBE_ENCL_DISPLAY:
            pDataChangeInfo->phys_location.slot = 0;
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_DISPLAY;
            break;

        case FBE_ENCL_DRIVE_SLOT:
            /* Update the slot number for the drive. */
            edalStatus = fbe_edal_getU8(edalBlockHandle, 
                                FBE_ENCL_DRIVE_SLOT_NUMBER,  //Attribute
                                componentType,  // component type
                                componentIndex,  // comonent index
                                &slot);  // return value

            if(!EDAL_STAT_OK(edalStatus)) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEncl,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_enclosure_get_logheader(pBaseEncl),
                                        "Failed to get slot num for Drive, index %d, %s.\n", 
                                        componentIndex,
                                        __FUNCTION__);

                pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                pDataChangeInfo->phys_location.slot = slot;
                pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_DRIVE;
            }
            break;

        case FBE_ENCL_CONNECTOR:
            edalStatus = fbe_edal_getBool(edalBlockHandle, 
                                FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR,  //Attribute
                                componentType,  // component type
                                componentIndex,  // comonent index
                                &isEntireConnector);  // return value

            if(!EDAL_STAT_OK(edalStatus))
            {
                /* ESP only cares about the entire connector. */
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEncl,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_enclosure_get_logheader(pBaseEncl),
                                        "Failed to get isEntireConnector for Connector, index %d, %s.\n", 
                                        componentIndex,
                                        __FUNCTION__);
                
                pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            else if(!isEntireConnector )
            {
                pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                edalStatus = fbe_edal_getU8(edalBlockHandle, 
                                FBE_ENCL_COMP_LOCATION,  //Attribute
                                componentType,  // component type
                                componentIndex,  // comonent index
                                &compLocation);  // return value

                if(!EDAL_STAT_OK(edalStatus))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEncl,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_enclosure_get_logheader(pBaseEncl),
                                        "Failed to get location for Connector, index %d, %s.\n", 
                                        componentIndex,
                                        __FUNCTION__);

                    pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
                    status = FBE_STATUS_GENERIC_FAILURE;
                }
                else
                {
                    pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_CONNECTOR;
                    pDataChangeInfo->phys_location.slot = compLocation;
                }
            }
            break;

        case FBE_ENCL_TEMP_SENSOR:
            /* Currently, for temp sensor, we only cares about the chassis temp sensor
             * Because we are monitoring chassis OverTemp warning and failure in ESP
             */

            edalStatus = fbe_edal_getU8(edalBlockHandle, 
                            FBE_ENCL_COMP_SUBTYPE,  //Attribute
                            componentType,  // component type
                            componentIndex,  // comonent index
                            &subType);  // return value

            if(!EDAL_STAT_OK(edalStatus))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEncl,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader(pBaseEncl),
                                    "Failed to get subtype for Temp Sensor %d, %s.\n", 
                                    componentIndex,
                                    __FUNCTION__);

                pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
            /* We only cares about chassis overall temp sensor */
            if (subType == FBE_ENCL_TEMP_SENSOR_SUBTYPE_CHASSIS)
            {
                pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_ENCLOSURE;
                pDataChangeInfo->phys_location.slot = 0;
                break;
            }
            else
            {
                pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
            break;

        case FBE_ENCL_SPS:
            /* ESES gives us side 31 for SPS. 
             * ESP currently takes 0 for local SPS while processing the SPS status change. 
             * So we just return 0 as the slot here. 
             */
            pDataChangeInfo->phys_location.slot = 0;
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_SPS;
            break;

        case FBE_ENCL_SSC:
            pDataChangeInfo->phys_location.slot = 0;
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_SSC;
            break;

        default:
            pDataChangeInfo->device_mask = FBE_DEVICE_TYPE_INVALID;
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}// End of function fbe_base_enclosure_get_data_change_info

       
       
/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getTempSensorBool_thread_safe(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_u32_t index,
 *                                fbe_bool_t *returnValuePtr);
 *
 ***************************************************************************
 * @brief
 *       This routine gets the Boolean value from the EDAL for the specified 
 * component type, attribute, and index
 *       This routine should be used from usurper context, so it won't 
 * interact with requests from within the object itself.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   index          - The component index.
 * @param   returnValuePtr (out) - The pointer to the returned value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   15-Aug-2013    Joe Perry - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getTempSensorBool_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                                      fbe_base_enclosure_attributes_t attribute,
                                                      fbe_u32_t index,
                                                      fbe_bool_t *returnValuePtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    if(base_enclosure == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }

    edalStatus = fbe_edal_getTempSensorBool(base_enclosure->enclosureComponentTypeBlock,  
                                            attribute,
                                            index,
                                            FBE_EDAL_INDICATOR_NOT_APPLICABLE,
                                            0,
                                            returnValuePtr);

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, unhandled stat %d, attr %s, idx %d.\n", 
                            __FUNCTION__, retStatus,
                            enclosure_access_printAttribute(attribute),
                            index); 

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;

}   // end of fbe_base_enclosure_edal_getTempSensorBool_thread_safe
        
/*!*************************************************************************
 * @fn fbe_base_enclosure_edal_getTempSensorU16_thread_safe(fbe_base_enclosure_t * base_enclosure,
 *                                fbe_base_enclosure_attributes_t attribute,
 *                                fbe_u32_t index,
 *                                fbe_u16_t *returnValuePtr);
 *
 ***************************************************************************
 * @brief
 *       This routine gets the U16 value from the EDAL for the specified 
 * component type, attribute, and index
 *       This routine should be used from usurper context, so it won't 
 * interact with requests from within the object itself.
 *
 * @param   base_enclosure - The pointer to the enclosure.
 * @param   attribute      - The component attrbute.
 * @param   index          - The component index.
 * @param   returnValuePtr (out) - The pointer to the returned value.
 *
 * @return  fbe_enclosure_status_t.
 *       FBE_ENCLOSURE_STATUS_OK - returnValuePtr contains valid data.
 *       otherwise - returnValuePtr is not valid.
 *
 * HISTORY
 *   15-Aug-2013    Joe Perry - Created.
 ****************************************************************************/
fbe_enclosure_status_t
fbe_base_enclosure_edal_getTempSensorU16_thread_safe(fbe_base_enclosure_t * base_enclosure,
                                                     fbe_base_enclosure_attributes_t attribute,
                                                     fbe_u32_t index,
                                                     fbe_u16_t *returnValuePtr)
{
    fbe_edal_status_t   edalStatus;
    fbe_enclosure_status_t retStatus = FBE_ENCLOSURE_STATUS_OK;

    if(base_enclosure == NULL)
    {
        return FBE_ENCLOSURE_STATUS_NULL_PTR;
    }

    edalStatus = fbe_edal_getTempSensorU16(base_enclosure->enclosureComponentTypeBlock,  
                                           attribute,
                                           index,
                                           FBE_EDAL_INDICATOR_NOT_APPLICABLE,
                                           0,
                                           returnValuePtr);

    retStatus = fbe_base_enclosure_translate_edal_status(edalStatus);
    if (!ENCL_STAT_OK(retStatus))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, unhandled stat %d, attr %s, idx %d.\n", 
                            __FUNCTION__, retStatus,
                            enclosure_access_printAttribute(attribute),
                            index); 

        if (retStatus == FBE_ENCLOSURE_STATUS_INVALID_CANARY)
        {
          fbe_base_enclosure_handle_errors(base_enclosure, 
                                                    FBE_ENCL_ENCLOSURE,
                                                    index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(retStatus),
                                                    0);
        }               
    }

    return retStatus;

}   // end of fbe_base_enclosure_edal_getTempSensorU16_thread_safe

