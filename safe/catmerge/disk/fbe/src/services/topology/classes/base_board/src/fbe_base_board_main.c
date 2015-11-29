#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_board.h"
#include "fbe_scheduler.h"
#include "fbe_diplex.h"

#include "fbe_base_discovered.h"
#include "base_board_private.h"
#include "base_board_pe_private.h"
#include "fbe_transport_memory.h"
#include "edal_base_enclosure_data.h"
#include "edal_processor_enclosure_data.h"

#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
extern fbe_status_t fbe_base_board_initHwErrMonCallback(fbe_base_board_t * base_board);
#endif  // !defined(UMODE_ENV) && !defined(SIMMODE_ENV)

/*
 * Initialization data for board components saved in EDAL. 
 * Please remember to update FBE_BASE_BOARD_PE_NUMBER_OF_COMPONENT_TYPES 
 * if more component type for processor enclosure is added.
 * The component size and component count will be initialized later.
 * Add the max_status_timeout (in seconds) if applicable for the component.
*/
fbe_base_board_pe_init_data_t   pe_init_data[FBE_BASE_BOARD_PE_NUMBER_OF_COMPONENT_TYPES] =
{
    /* Component type, component size, component count, max_status_timeout (in seconds) */
    {FBE_ENCL_PLATFORM, 0, 0, 0},
    {FBE_ENCL_MISC, 0, 0, 0},
    {FBE_ENCL_IO_MODULE, 0, 0, 90},
    {FBE_ENCL_MEZZANINE, 0, 0, 60},
    {FBE_ENCL_IO_PORT, 0, 0, 0},
    {FBE_ENCL_BEM, 0, 0, 90},    
    {FBE_ENCL_POWER_SUPPLY, 0, 0, 45},
    {FBE_ENCL_COOLING_COMPONENT, 0, 0, 45},
    {FBE_ENCL_MGMT_MODULE, 0, 0, 90},
    {FBE_ENCL_FLT_REG, 0, 0, 30},
    {FBE_ENCL_SLAVE_PORT, 0, 0, 30},
    {FBE_ENCL_SUITCASE, 0, 0, 60},
    {FBE_ENCL_BMC, 0, 0, 90},
    {FBE_ENCL_RESUME_PROM, 0, 0, 0},
    {FBE_ENCL_TEMPERATURE, 0, 0, 60},
    {FBE_ENCL_SPS, 0, 0, 9},
    {FBE_ENCL_BATTERY, 0, 0, 90},
    {FBE_ENCL_CACHE_CARD, 0, 0, 90},
    {FBE_ENCL_DIMM, 0, 0, 90},
    {FBE_ENCL_SSD, 0, 0, 1800},
};

/* Class methods forward declaration */
fbe_status_t base_board_load(void);
fbe_status_t base_board_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_base_board_class_methods = {FBE_CLASS_ID_BASE_BOARD,
                                                    base_board_load,
                                                    base_board_unload,
                                                    fbe_base_board_create_object,
                                                    fbe_base_board_destroy_object,
                                                    fbe_base_board_control_entry,
                                                    fbe_base_board_event_entry,
                                                    NULL,
                                                    fbe_base_board_monitor_entry};

/* Forward declaration */
static fbe_status_t fbe_base_board_create_edge(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t fbe_base_board_drain_command_queue(fbe_base_board_t *base_board);

fbe_status_t 
base_board_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_BASE_BOARD,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s: entry\n", __FUNCTION__);
    return fbe_base_board_monitor_load_verify();
}

fbe_status_t 
base_board_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_BASE_BOARD,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s: entry\n", __FUNCTION__);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_board_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_base_board_t * base_board;
    fbe_object_mgmt_attributes_t mgmt_attributes;
    fbe_status_t status;
    //    fbe_u32_t    ps;

    /* Call parent constructor */
    status = fbe_base_discovering_create_object(packet, object_handle);
    if(status != FBE_STATUS_OK){
        return status;
    }
    base_board = (fbe_base_board_t *)fbe_base_handle_to_pointer(*object_handle);

    /* Set class id */
    fbe_base_object_set_class_id((fbe_base_object_t *) base_board, FBE_CLASS_ID_BASE_BOARD);    

    fbe_base_object_trace((fbe_base_object_t*)base_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Set physical_object_level */
    fbe_base_object_set_physical_object_level((fbe_base_object_t *)base_board, FBE_PHYSICAL_OBJECT_LEVEL_BOARD);

    /* Set first io_port index and number of io_ports */
    fbe_base_board_set_number_of_io_ports(base_board, 0); /* It it up to specialized class to update this value */

    /* We may want to over-ride the defaults for our object attributes. */
    status = fbe_base_board_get_mgmt_attributes(&mgmt_attributes);
    if (status == FBE_STATUS_OK) {
        fbe_base_object_set_mgmt_attributes((fbe_base_object_t*)base_board, mgmt_attributes);
    }

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_board_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_base_board_t * base_board;
    fbe_status_t status;
    fbe_edal_block_handle_t   edal_block_handle = NULL;
    fbe_base_board_pe_buffer_handle_t  pe_buffer_handle = NULL;

    base_board = (fbe_base_board_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)base_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Check/Complete all outstanding commands */
    fbe_base_board_drain_command_queue(base_board);

    /* Get the pointer to the first EDAL data block. */
    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);

    if(edal_block_handle != NULL)
    {
        /* Release memory allocated for the chain of EDAL data blocks. */
        fbe_base_board_release_edal_block(edal_block_handle);
    }

    /* clean up */
    fbe_base_board_set_edal_block_handle(base_board, NULL);

    /* Get the pointer to the processor enclosure buffer. */
    fbe_base_board_get_pe_buffer_handle(base_board, &pe_buffer_handle);

    if(pe_buffer_handle != NULL)
    {
        /* Release the memory allocated to get the status of the processor enclosure components. */
        fbe_memory_ex_release(pe_buffer_handle);
    }

    /* clean up */
    fbe_base_board_set_pe_buffer_handle(base_board, NULL);

    fbe_spinlock_destroy(&base_board->board_command_queue.command_queue_lock); /* SAFEBUG - need destroy */
    fbe_spinlock_destroy(&base_board->enclGetLocalFupPermissionLock); 

    /* Step 1. Check parent edges */
    /* Step 2. Cleanup */
    /* Step 3. Call parent destructor */        

    status = fbe_base_discovering_destroy_object(object_handle); /* SAFEBUG - need to cleanup properly */

    return status;
}

fbe_status_t 
fbe_base_board_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_board_init(fbe_base_board_t * base_board)
{
    fbe_edal_block_handle_t   edal_block_handle = NULL;
    fbe_edal_status_t edal_status = FBE_EDAL_STATUS_OK;
    fbe_status_t      status = FBE_STATUS_OK;
    fbe_base_board_pe_buffer_handle_t pe_buffer_handle = NULL;
    fbe_u32_t                         pe_buffer_size = 0;
    fbe_u32_t                         sampleIndex,psIndex;
  
    fbe_base_object_trace((fbe_base_object_t*)base_board,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry.\n", __FUNCTION__);

    /* Init the component size for all the component types from EDAL. */
    status = fbe_base_board_init_component_size(&pe_init_data[0]);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s, init_component_size failed, status 0x%x.\n",
                __FUNCTION__, status); 

        return status;
    }
    
    /* Get the max required size for the buffer which 
     * will be used to get the SPECL status. 
     */
    fbe_base_board_pe_get_required_buffer_size(&pe_init_data[0], &pe_buffer_size);

    pe_buffer_handle = fbe_memory_ex_allocate(pe_buffer_size);

    if(pe_buffer_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s, native_allocate failed, pe_buffer_size %d.\n",
                __FUNCTION__, pe_buffer_size);    
        
        fbe_base_board_set_pe_buffer_handle(base_board, NULL);
        fbe_base_board_set_pe_buffer_size(base_board, 0);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_base_board_set_pe_buffer_handle(base_board, pe_buffer_handle);
    fbe_base_board_set_pe_buffer_size(base_board, pe_buffer_size); 

    /* Init component count for all the component types from SPECL. */
    status = fbe_base_board_init_component_count(base_board, &pe_init_data[0]);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s, init_component_count failed, status 0x%x.\n",
                __FUNCTION__, status); 

        return status;
    }

    /* Allocate and init EDAL data block. */
    edal_block_handle = fbe_base_board_alloc_and_init_edal_block();
    if(edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s, alloc_and_init_edal_block failed.\n",
                __FUNCTION__);    
        
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Set the EDAL block handle to the base board struct. */
    fbe_base_board_set_edal_block_handle(base_board, edal_block_handle);

    /* Format the EDAL block. */
    status = fbe_base_board_format_edal_block(base_board, &pe_init_data[0]);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_board_release_edal_block(edal_block_handle);
        fbe_base_board_set_edal_block_handle(base_board, NULL);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    /*
     * Initialize the command queue.  This is used to track the outstanding
     * write/set commands. 
     */
    fbe_queue_init(&base_board->board_command_queue.command_queue);
    fbe_spinlock_init(&base_board->board_command_queue.command_queue_lock);

    /* Save base_board->platformInfo to EDAL. */
    edal_status = fbe_edal_setBuffer(base_board->edal_block_handle,
            FBE_ENCL_PE_PLATFORM_INFO,  // attribute
            FBE_ENCL_PLATFORM,  // component type
            0,  //component index
            sizeof(SPID_PLATFORM_INFO),
            (fbe_u8_t *)&base_board->platformInfo);

    if(!EDAL_STAT_OK(edal_status))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Initialize EIR related info
     */
    base_board->sampleCount = 0;
    base_board->sampleIndex = 0;
    for (psIndex = 0; psIndex < FBE_ESP_PS_MAX_COUNT_PER_ENCL; psIndex++)
    {
	for (sampleIndex = 0; sampleIndex < FBE_BASE_BOARD_EIR_SAMPLE_COUNT; sampleIndex++)
	{
	    base_board->inputPowerSamples[psIndex][sampleIndex].inputPower = 0;
	    base_board->inputPowerSamples[psIndex][sampleIndex].status = FBE_ENERGY_STAT_UNINITIALIZED;
	}
    }

    /*
     * Initialize fup permission lock
     */
    fbe_spinlock_init(&base_board->enclGetLocalFupPermissionLock);
    fbe_zero_memory(&(base_board->enclosureFupPermission.location), sizeof(fbe_device_physical_location_t));
    base_board->enclosureFupPermission.permissionGrantTime = 0;
    base_board->enclosureFupPermission.permissionOccupied = FALSE;

    base_board->ssdPollCounter = 0;
 
#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
    /*
     * Setup interface with HwErrMon
     */
    fbe_base_board_initHwErrMonCallback(base_board);
#endif  // !defined(UMODE_ENV) && !defined(SIMMODE_ENV)

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 * @fn fbe_base_board_alloc_and_init_edal_block
 **************************************************************************
 *
 *  @brief
 *      This function allocates and initializes the EDAL block.
 *
 *  @return   fbe_edal_block_handle_t - pointer to allocated block.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    22-Feb-2010: PHE - Copied from fbe_sas_viper_allocate_encl_data_block() and 
 *                       make it more generic.
 *
 **************************************************************************/
fbe_edal_block_handle_t * fbe_base_board_alloc_and_init_edal_block(void)
{
    fbe_edal_block_handle_t edal_block_handle = NULL;

    edal_block_handle = (fbe_edal_block_handle_t)fbe_transport_allocate_buffer();
    
    if(edal_block_handle != NULL)
    {
        fbe_zero_memory(edal_block_handle, FBE_MEMORY_CHUNK_SIZE);

        fbe_edal_init_block(edal_block_handle, 
                                FBE_MEMORY_CHUNK_SIZE, 
                                FBE_ENCL_PE, 
                                FBE_BASE_BOARD_PE_NUMBER_OF_COMPONENT_TYPES);    
    }

    return (edal_block_handle);
}// End of fbe_base_board_alloc_and_init_edal_block

/*!*************************************************************************
 * @fn fbe_base_board_format_edal_block
 **************************************************************************
 *
 *  @brief
 *      This function formats the component type data in the EDAL data block.
 *      If more than one block is needed, a new block will be allocated.
 *
 *  @param base_board - pointer to the base board object.
 *  @param pe_init_data- pointer to the array which holds the init data
 *
 *  @return   fbe_status_t
 *             FBE_STATUS_OK - no error.
 *             otherwise - error encountered.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    22-Feb-2010: PHE - Copied from fbe_sas_viper_format_encl_data() and make it more generic.
 *
 **************************************************************************/
fbe_status_t
fbe_base_board_format_edal_block(fbe_base_board_t * base_board,
                                fbe_base_board_pe_init_data_t pe_init_data[])                               
{
    fbe_u32_t                   index;  // Used to loop through all the component types.
    fbe_edal_status_t           edal_status = FBE_EDAL_STATUS_OK;   
    fbe_edal_block_handle_t     edal_block_handle = NULL;
    fbe_edal_block_handle_t     local_block = NULL;
    
    edal_block_handle = base_board->edal_block_handle;
    // loop through all components and initialize data & allocate locations
    for(index = 0; index < FBE_BASE_BOARD_PE_NUMBER_OF_COMPONENT_TYPES; index++)
    {       
        /* Check whether the individual component can fit into one single block. */
        edal_status = fbe_edal_can_individual_component_fit_in_block(edal_block_handle, 
                                                               pe_init_data[index].component_type);     

        if(edal_status != FBE_EDAL_STATUS_OK)
        {
            // This is the design issue. We assume at least the individual component should fit into one block.
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s, individual component for %s can not fit into one block(size %d).\n",
                __FUNCTION__,
                enclosure_access_printComponentType(pe_init_data[index].component_type),
                FBE_MEMORY_CHUNK_SIZE); 
        }

        while(TRUE)
        {
            edal_status = fbe_edal_fit_component(edal_block_handle,
                                                    pe_init_data[index].component_type,
                                                    pe_init_data[index].component_count,
                                                    pe_init_data[index].component_size);  // individual size.

            
            if(edal_status == FBE_EDAL_STATUS_OK)
            {
                // fitting component succeeded. Break out the while loop. 
                break;          
            }
            else if(edal_status == FBE_EDAL_STATUS_INSUFFICIENT_RESOURCE) 
            {
                /* if it doesn't fit, allocate a new block,
                * appended to edal, and try again. 
                */
                local_block = (fbe_enclosure_component_block_t *)fbe_base_board_alloc_and_init_edal_block();
                fbe_edal_append_block(edal_block_handle, (fbe_enclosure_component_block_t *)local_block);
                continue;
                /* Continue with the while loop. */             
            }
            else
            {
                /* Error encountered. */
                fbe_base_object_trace((fbe_base_object_t *)base_board,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s, %s fbe_edal_fit_component failed.\n",
                                        __FUNCTION__,
                                        enclosure_access_printComponentType(pe_init_data[index].component_type)); 

                return FBE_STATUS_GENERIC_FAILURE;
            }
            
        }// End of while loop

    }// end of for loop

    if (edal_status == FBE_EDAL_STATUS_OK)
    {
        fbe_edal_printAllComponentBlockInfo((void *)edal_block_handle);
    }

    return FBE_STATUS_OK;

}   // end of fbe_edal_format_data_block


/*!*************************************************************************
* @fn fbe_base_board_release_edal_block            
***************************************************************************
*
* @brief
*       This function releases a chain of EDAL data blocks.
*
* @param      edal_block_handle - The pointer to the EDAL block.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*    22-Feb-2010: PHE - Copied from fbe_sas_viper_release_encl_data_block() and make it more generic.
*
***************************************************************************/
fbe_status_t 
fbe_base_board_release_edal_block(fbe_edal_block_handle_t edal_block_handle)
{
    fbe_enclosure_component_block_t *component_block = NULL;
    fbe_enclosure_component_block_t *dropped_block = NULL;

    component_block = (fbe_enclosure_component_block_t *)edal_block_handle;
    dropped_block = fbe_edal_drop_the_tail_block(component_block);

    while (dropped_block!=NULL)
    {
        /* release the memory */
        fbe_transport_release_buffer(dropped_block);
        /* drop the next one */
        dropped_block = fbe_edal_drop_the_tail_block(component_block);
    }

    if(component_block != NULL)
    {
        fbe_transport_release_buffer(component_block);
    }

    return (FBE_STATUS_OK);
}// End of fbe_base_board_release_data_block


/*!*************************************************************************
* @fn fbe_base_board_add_command_element            
***************************************************************************
*
* @brief
*       This adds a command packet to the board command queue.
*
* @param      base_board - Object context.
* @param      packet - command packet
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*    11-Feb-2011: bphilbin - Created
*
***************************************************************************/
fbe_status_t fbe_base_board_add_command_element(fbe_base_board_t *base_board, fbe_packet_t *packet)
{
    fbe_base_board_command_queue_element_t *element;
    element = fbe_memory_ex_allocate(sizeof(fbe_base_board_command_queue_element_t));
    fbe_zero_memory(element, sizeof(fbe_base_board_command_queue_element_t));
    element->packet = packet;
    fbe_spinlock_lock(&base_board->board_command_queue.command_queue_lock);
    fbe_base_object_trace((fbe_base_object_t *)base_board,
                FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s, add packet: %p, magic_num 0x%llx\n",
                __FUNCTION__,
                packet, (unsigned long long)packet->magic_number); 
    fbe_queue_push(&base_board->board_command_queue.command_queue, (fbe_queue_element_t *)element);
    fbe_spinlock_unlock(&base_board->board_command_queue.command_queue_lock);
    return FBE_STATUS_OK;
}// End of fbe_base_board_add_command_element


/*!*************************************************************************
* @fn fbe_base_board_check_command_queue            
***************************************************************************
*
* @brief
*       This function checks the command queue for any outstanding commands
*       and kicks off the process of checking that those commands completed
*       successfully.  
*
* @param      base_board - Object context.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*    11-Feb-2011: bphilbin - Created
*
***************************************************************************/
fbe_status_t fbe_base_board_check_command_queue(fbe_base_board_t *base_board)
{
    fbe_base_board_command_queue_element_t *element, *tmp_element;

    fbe_status_t status;

    fbe_spinlock_lock(&base_board->board_command_queue.command_queue_lock);
    element = (fbe_base_board_command_queue_element_t *) fbe_queue_front(&base_board->board_command_queue.command_queue);
    while(element != NULL)
    {
        fbe_spinlock_unlock(&base_board->board_command_queue.command_queue_lock);

        status = fbe_base_board_check_outstanding_command(base_board, element->packet);

        fbe_spinlock_lock(&base_board->board_command_queue.command_queue_lock);
        tmp_element = (fbe_base_board_command_queue_element_t *) fbe_queue_next(&base_board->board_command_queue.command_queue, (fbe_queue_element_t *)element);

        if(status != FBE_STATUS_PENDING)
        {
            fbe_queue_remove((fbe_queue_element_t *)element);
            fbe_memory_ex_release(element);
        }

        element = tmp_element;
    }
    fbe_spinlock_unlock(&base_board->board_command_queue.command_queue_lock);
    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_base_board_drain_command_queue            
***************************************************************************
*
* @brief
*       This function removes all elements from the command queue and
*       frees the memory allocated for the command elements.
*
* @param      base_board - Object context.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*    11-Feb-2011: bphilbin - Created
*
***************************************************************************/
fbe_status_t fbe_base_board_drain_command_queue(fbe_base_board_t *base_board)
{
    fbe_base_board_command_queue_element_t *element;

    fbe_spinlock_lock(&base_board->board_command_queue.command_queue_lock);
	while(!fbe_queue_is_empty(&base_board->board_command_queue.command_queue))
    {
		element = (fbe_base_board_command_queue_element_t *) fbe_queue_pop(&base_board->board_command_queue.command_queue);
		fbe_spinlock_unlock(&base_board->board_command_queue.command_queue_lock);
        fbe_memory_ex_release(element);
        fbe_spinlock_lock(&base_board->board_command_queue.command_queue_lock);
    }
    fbe_spinlock_unlock(&base_board->board_command_queue.command_queue_lock);
    return FBE_STATUS_OK;
}


/*!*************************************************************************
* @fn fbe_base_board_check_outstanding_command            
***************************************************************************
*
* @brief
*       This function checks that a command completed successfully by
*       verifying the data read in matches what should have been written
*       out.  It will complete the packet successfully if the read matches
*       the write.  This function assumes all sanity checking on the
*       control operation has been done in the usurper already.
*
* @param      base_board - Object context.
* @param      packet - command packet
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*    11-Feb-2011: bphilbin - Created
*
***************************************************************************/
fbe_status_t fbe_base_board_check_outstanding_command(fbe_base_board_t *base_board, fbe_packet_t *packet)
{
    fbe_payload_control_operation_opcode_t  control_code;
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;

    control_code = fbe_transport_get_control_code(packet);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
        
    switch(control_code) 
    {
    case FBE_BASE_BOARD_CONTROL_CODE_SET_IOM_FAULT_LED:
        // requires LED control code in Physical package
        //status = fbe_base_board_check_iom_fault_led_command(base_board, control_operation);
        break;
    case FBE_BASE_BOARD_CONTROL_CODE_SET_MGMT_FAULT_LED:
        // requires LED control code in Physical package
        break;
    case FBE_BASE_BOARD_CONTROL_CODE_SET_MGMT_VLAN_CONFIG_MODE:
        status = fbe_base_board_check_mgmt_vlan_mode_command(base_board, control_operation);
        break;
    case FBE_BASE_BOARD_CONTROL_CODE_SET_MGMT_PORT:
        status = fbe_base_board_check_mgmt_port_command(base_board, control_operation);
        break;
    case FBE_BASE_BOARD_CONTROL_CODE_SET_CLEAR_HOLD_IN_POST_AND_OR_RESET:
        status = fbe_base_board_check_hold_in_post_and_or_reset(base_board, control_operation);
        break;
    case FBE_BASE_BOARD_CONTROL_CODE_RESUME_WRITE:
        status = fbe_base_board_check_resume_prom_last_write_status(base_board, control_operation);
        break;
    case FBE_BASE_BOARD_CONTROL_CODE_RESUME_WRITE_ASYNC:
        status = fbe_base_board_check_resume_prom_last_write_status_async(base_board,control_operation);
        break;
    default:
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    fbe_base_object_trace((fbe_base_object_t *)base_board,
                FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s, Control Code:0x%x Status:0x%x\n",
                __FUNCTION__,
                control_code,
                status); 

    //If not pending, complete the packet and return
    if(status != FBE_STATUS_PENDING)
    {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        status = FBE_STATUS_OK;
    }

    return status;
}

fbe_status_t 
fbe_base_board_set_number_of_io_ports(fbe_base_board_t * base_board, fbe_u32_t number_of_io_ports)
{
    base_board->number_of_io_ports = number_of_io_ports;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_board_get_number_of_io_ports(fbe_base_board_t * base_board, fbe_u32_t * number_of_io_ports)
{
    *number_of_io_ports = base_board->number_of_io_ports;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_board_set_edal_block_handle(fbe_base_board_t * base_board, fbe_edal_block_handle_t edal_block_handle)
{
    base_board->edal_block_handle = edal_block_handle;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_board_get_edal_block_handle(fbe_base_board_t * base_board, fbe_edal_block_handle_t * edal_block_handle)
{
    *edal_block_handle = base_board->edal_block_handle;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_board_set_pe_buffer_handle(fbe_base_board_t * base_board, fbe_base_board_pe_buffer_handle_t pe_buffer_handle)
{
    base_board->pe_buffer_handle = pe_buffer_handle;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_board_get_pe_buffer_handle(fbe_base_board_t * base_board, fbe_base_board_pe_buffer_handle_t * pe_buffer_handle)
{
    *pe_buffer_handle = base_board->pe_buffer_handle;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_board_set_pe_buffer_size(fbe_base_board_t * base_board, fbe_u32_t pe_buffer_size)
{
    base_board->pe_buffer_size = pe_buffer_size;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_board_get_pe_buffer_size(fbe_base_board_t * base_board, fbe_u32_t * pe_buffer_size)
{
    *pe_buffer_size = base_board->pe_buffer_size;
    return FBE_STATUS_OK;
}



