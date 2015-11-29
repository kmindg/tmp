#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe/fbe_winddk.h"
#include "fbe_transport_memory.h"
#include "fbe_ses.h"
#include "sas_bullet_enclosure_private.h"

fbe_status_t 
fbe_sas_bullet_enclosure_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_bullet_enclosure_t * sas_bullet_enclosure = NULL;

    sas_bullet_enclosure = (fbe_sas_bullet_enclosure_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                       "%s entry \n", __FUNCTION__);

    status = fbe_lifecycle_crank_object(&fbe_sas_bullet_enclosure_lifecycle_const, (fbe_base_object_t*)sas_bullet_enclosure, packet);

    return status;
}

fbe_status_t fbe_sas_bullet_enclosure_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(sas_bullet_enclosure));
}

/*--- local function prototypes --------------------------------------------------------*/

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t sas_bullet_enclosure_discovery_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_lifecycle_status_t sas_bullet_enclosure_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_lifecycle_status_t sas_bullet_enclosure_discovery_update_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

static fbe_lifecycle_status_t sas_bullet_enclosure_get_element_list_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_bullet_enclosure_get_element_list_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(sas_bullet_enclosure);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_NULL_COND(sas_bullet_enclosure);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(sas_bullet_enclosure) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        sas_bullet_enclosure,
        FBE_LIFECYCLE_NULL_FUNC,       /* online function */
        FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/
static fbe_lifecycle_const_cond_t sas_bullet_enclosure_self_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_OBJECT_LIFECYCLE_COND_SELF_INIT,
        sas_bullet_enclosure_self_init_cond_function)
};

static fbe_lifecycle_const_cond_t sas_bullet_enclosure_discovery_update_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE,
        sas_bullet_enclosure_discovery_update_cond_function)
};

static fbe_lifecycle_const_cond_t sas_bullet_enclosure_discovery_poll_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL,
        sas_bullet_enclosure_discovery_poll_cond_function)
};

static fbe_lifecycle_const_cond_t sas_bullet_enclosure_get_element_list_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_ELEMENT_LIST,
        sas_bullet_enclosure_get_element_list_cond_function)
};

FBE_LIFECYCLE_DEF_CONST_NULL_BASE_CONDITIONS(sas_bullet_enclosure);

/*--- constant rotaries ----------------------------------------------------------------*/
static fbe_lifecycle_const_rotary_cond_t sas_enclosure_specialize_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_bullet_enclosure_self_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t sas_enclosure_ready_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_bullet_enclosure_discovery_update_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_bullet_enclosure_discovery_poll_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_bullet_enclosure_get_element_list_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(sas_bullet_enclosure)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, sas_enclosure_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, sas_enclosure_ready_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(sas_bullet_enclosure);

/*--- global sas enclosure lifecycle constant data ----------------------------------------*/
fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_bullet_enclosure) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        sas_bullet_enclosure,
        FBE_CLASS_ID_SAS_BULLET_ENCLOSURE,          /* This class */
        FBE_LIFECYCLE_CONST_DATA(sas_enclosure))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/


/*--- Condition Functions --------------------------------------------------------------*/
static fbe_lifecycle_status_t 
sas_bullet_enclosure_discovery_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_sas_bullet_enclosure_t * sas_bullet_enclosure = (fbe_sas_bullet_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                       FBE_TRACE_LEVEL_INFO,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                       "%s entry\n", __FUNCTION__);

    /* We need to get element list from time to time
        We will also trigger this condition in the "topology_change" event handling
        once this event implemented.
    */
    status = fbe_lifecycle_set_cond(&fbe_sas_bullet_enclosure_lifecycle_const, 
                (fbe_base_object_t*)sas_bullet_enclosure, 
                FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_ELEMENT_LIST);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                            "%s, can't set ELEMENT_LIST condition, status: 0x%x.\n",
                            __FUNCTION__, status);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_bullet_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                           "%s can't clear current condition, status: 0x%X",
                           __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
sas_bullet_enclosure_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_sas_bullet_enclosure_t * sas_bullet_enclosure = (fbe_sas_bullet_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                       FBE_TRACE_LEVEL_INFO,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                       "%s entry\n", __FUNCTION__);

     
    fbe_sas_bullet_enclosure_init(sas_bullet_enclosure);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_bullet_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                           "%s can't clear current condition, status: 0x%X",
                           __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
sas_bullet_enclosure_discovery_update_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_sas_bullet_enclosure_t * sas_bullet_enclosure = (fbe_sas_bullet_enclosure_t*)p_object;
    fbe_discovery_edge_t * discovery_edge = NULL;
    fbe_u32_t i;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                       FBE_TRACE_LEVEL_INFO,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                       "%s entry\n", __FUNCTION__);

    /* Iterate throw discovery edges to figure out what drived we need to instantiate and 
        what edges we need to mark as gone
    */
    /* Iterate throw all slots */
    for(i = 0 ; i < FBE_SAS_BULLET_ENCLOSURE_NUMBER_OF_SLOTS; i ++){ 
        fbe_base_discovering_transport_server_lock((fbe_base_discovering_t *) sas_bullet_enclosure);
        discovery_edge = fbe_base_discovering_get_client_edge_by_server_index((fbe_base_discovering_t *) sas_bullet_enclosure, i);
        fbe_base_discovering_transport_server_unlock((fbe_base_discovering_t *) sas_bullet_enclosure);

        if(sas_bullet_enclosure->drive_info[i].sas_address != FBE_SAS_ADDRESS_INVALID){ /* we have a drive */
            if(discovery_edge == NULL){ /* Drive is not instantiated */
                status = fbe_base_discovering_instantiate_client((fbe_base_discovering_t *) sas_bullet_enclosure, 
                                                                FBE_CLASS_ID_BASE_PHYSICAL_DRIVE, 
                                                                i);
            } 
            
        } else { /* we have no drive */
                if(discovery_edge != NULL){ /* Drive is instantiated */
                /* Peter Puhov. The path state of the edge represents the surrogate state of the object.
                    We should use NOT_PRESENT bit instead.
                    fbe_base_discovering_set_path_state((fbe_base_discovering_t *) sas_bullet_enclosure, i, FBE_PATH_STATE_GONE);
                */
                    fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) sas_bullet_enclosure, i, FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT);
                }
        }       
    }/* for(i = 0 ; i < FBE_SAS_BULLET_ENCLOSURE_NUMBER_OF_SLOTS; i ++) */


    /* Enclosure instantiation */
    fbe_base_discovering_transport_server_lock((fbe_base_discovering_t *) sas_bullet_enclosure);
    discovery_edge = fbe_base_discovering_get_client_edge_by_server_index((fbe_base_discovering_t *) sas_bullet_enclosure,
                                                                            FBE_SAS_BULLET_ENCLOSURE_FIRST_EXPANSION_PORT_INDEX);
    fbe_base_discovering_transport_server_unlock((fbe_base_discovering_t *) sas_bullet_enclosure);

    if(sas_bullet_enclosure->expansion_port_info.sas_address != FBE_SAS_ADDRESS_INVALID){ /* we have an enclosure */
        if(discovery_edge == NULL){ /* Enclosure is not instantiated */
            status = fbe_base_discovering_instantiate_client((fbe_base_discovering_t *) sas_bullet_enclosure, 
                                                            FBE_CLASS_ID_SAS_ENCLOSURE, 
                                                            FBE_SAS_BULLET_ENCLOSURE_FIRST_EXPANSION_PORT_INDEX);
        } 

    } else { /* we have no enclosure */
        if(discovery_edge != NULL){ /* Enclosure is instantiated */
            /* Peter Puhov. The path state of the edge represents the surrogate state of the object.
                We should use NOT_PRESENT bit instead.
            fbe_base_discovering_set_path_state((fbe_base_discovering_t *) sas_bullet_enclosure, 
                                                FBE_SAS_BULLET_ENCLOSURE_FIRST_EXPANSION_PORT_INDEX, 
                                                FBE_PATH_STATE_GONE);
            */
            fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) sas_bullet_enclosure, 
                                                FBE_SAS_BULLET_ENCLOSURE_FIRST_EXPANSION_PORT_INDEX,
                                                FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT);
        }
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_bullet_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                           "%s can't clear current condition, status: 0x%X",
                           __FUNCTION__, status);        
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}


static fbe_lifecycle_status_t 
sas_bullet_enclosure_get_element_list_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_bullet_enclosure_t * sas_bullet_enclosure = (fbe_sas_bullet_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                       FBE_TRACE_LEVEL_INFO,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                       "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_bullet_enclosure_get_element_list_cond_completion, sas_bullet_enclosure);

    /* Call the executer function to do actual job */
    status = fbe_sas_bullet_enclosure_send_get_element_list_command(sas_bullet_enclosure, packet);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
sas_bullet_enclosure_get_element_list_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_bullet_enclosure_t * sas_bullet_enclosure = NULL;
    
    sas_bullet_enclosure = (fbe_sas_bullet_enclosure_t*)context;

    /* Check the packet status */

    /* I would assume that it is OK to clear CURRENT condition here.
        After all we are in condition completion context */

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_bullet_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                           "%s can't clear current condition, status: 0x%X",
                           __FUNCTION__, status);
        
    }

    return FBE_STATUS_OK;
}
