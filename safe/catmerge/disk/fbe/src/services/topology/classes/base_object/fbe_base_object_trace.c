/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_object_trace.c
 ****************************************************************************
 *
 * @brief
 *  This file contains the trace functions for Base Object.
 *
 * @author
 *   02/13/2009: - Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "base_object_private.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!***************************************************************************
 * fbe_base_object_trace_object_id()
 ******************************************************************************
 * @brief
 *  Display's the trace of the base object related to object id.
 *
 * @param  object_id - Object id for the object
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK 
 *
 * @author
 *  02/13/2009 - Created. Nishit Singh
 *
 ****************************************************************/
fbe_status_t
fbe_base_object_trace_object_id(fbe_object_id_t object_id,
                                fbe_trace_func_t trace_func,
                                fbe_trace_context_t trace_context)
{
    trace_func(trace_context, "%d (0x%x)", object_id, object_id);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_object_trace_object_id()
 ******************************************/

/*!***************************************************************************
 * fbe_base_object_trace_class_id()
 ******************************************************************************
 * @brief
 *  Display's the trace of the base object related to class id
 *
 * @param  class_id - Class id for the object
  *@param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK 
 *
 * @author
 *  02/13/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t
fbe_base_object_trace_class_id(fbe_class_id_t class_id, 
                               fbe_trace_func_t trace_func, 
                               fbe_trace_context_t trace_context)
{
    switch(class_id)
    {
        case FBE_CLASS_ID_BASE_OBJECT:
            trace_func(trace_context, "BASE_OBJECT");
        break;
        case FBE_CLASS_ID_BASE_DISCOVERED:
            trace_func(trace_context, "BASE_DISCOVERED");
        break;
        case FBE_CLASS_ID_BASE_DISCOVERING:
            trace_func(trace_context, "BASE_DISCOVERING");
        break;
        case FBE_CLASS_ID_BASE_BOARD:
            trace_func(trace_context, "BASE_BOARD");
        break;
        case FBE_CLASS_ID_FLEET_BOARD:
            trace_func(trace_context, "FLEET_BOARD");
        break;
        case FBE_CLASS_ID_MAGNUM_BOARD:
            trace_func(trace_context, "MAGNUM_BOARD");
        break;
        case FBE_CLASS_ID_ARMADA_BOARD:
            trace_func(trace_context, "ARMADA_BOARD");
        break;
        case FBE_CLASS_ID_BASE_PORT:
            trace_func(trace_context, "BASE_PORT");
        break;
        case FBE_CLASS_ID_SAS_PORT:
            trace_func(trace_context, "SAS_PORT");
        break;
        case FBE_CLASS_ID_FC_PORT:
            trace_func(trace_context, "FC_PORT");
        break;
        case FBE_CLASS_ID_SAS_PMC_PORT:
            trace_func(trace_context, "SAS_PMC_PORT");
        break;
        case FBE_CLASS_ID_FC_PMC_PORT:
            trace_func(trace_context, "FC_PMC_PORT");
        break;
        case FBE_CLASS_ID_ISCSI_PORT:
            trace_func(trace_context, "ISCSI_PORT");
            break;
        case FBE_CLASS_ID_LOGICAL_DRIVE:
            trace_func(trace_context, "LOGICAL_DRIVE");
        break;
        case FBE_CLASS_ID_BASE_PHYSICAL_DRIVE:
            trace_func(trace_context, "BASE_PHYSICAL_DRIVE");
        break;
        case FBE_CLASS_ID_SAS_PHYSICAL_DRIVE:
            trace_func(trace_context, "SAS_PHYSICAL_DRIVE");
        break;
        case FBE_CLASS_ID_SAS_FLASH_DRIVE:
            trace_func(trace_context, "SAS_FLASH_DRIVE");
        break;
        case FBE_CLASS_ID_SATA_PADDLECARD_DRIVE:
            trace_func(trace_context, "SATA_PADDLECARD_DRIVE");
        break;
        case FBE_CLASS_ID_SATA_PHYSICAL_DRIVE:
            trace_func(trace_context, "SATA_PHYSICAL_DRIVE");
        break;
        case FBE_CLASS_ID_SATA_FLASH_DRIVE:
            trace_func(trace_context, "SATA_FLASH_DRIVE");
        break;
	    case FBE_CLASS_ID_SAS_VIKING_DRVSXP_LCC:
            trace_func(trace_context, "SAS_VIKING_DRVSXP_LCC");
        break;
        case FBE_CLASS_ID_SAS_NAGA_DRVSXP_LCC:
            trace_func(trace_context, "SAS_NAGA_DRVSXP_LCC");
        break;	
        case FBE_CLASS_ID_SAS_VOYAGER_EE_LCC:
            trace_func(trace_context, "SAS_VOYAGER_EE_LCC");
        break;	
        case FBE_CLASS_ID_SAS_CAYENNE_DRVSXP_LCC:
            trace_func(trace_context, "SAS_CAYENNE_DRVSXP_LCC");
        break;	
        case FBE_CLASS_ID_BASE_ENCLOSURE:
            trace_func(trace_context, "BASE_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_ENCLOSURE:
            trace_func(trace_context, "SAS_ENCLOSURE");
        break;
        case FBE_CLASS_ID_ESES_ENCLOSURE:
            trace_func(trace_context, "ESES_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_BULLET_ENCLOSURE:
            trace_func(trace_context, "SAS_BULLET_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_VIPER_ENCLOSURE:
            trace_func(trace_context, "SAS_VIPER_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE:
            trace_func(trace_context, "SAS_MAGNUM_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_CITADEL_ENCLOSURE:
            trace_func(trace_context, "SAS_CITADEL_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_BUNKER_ENCLOSURE:
            trace_func(trace_context, "SAS_BUNKER_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_DERRINGER_ENCLOSURE:
            trace_func(trace_context, "SAS_DERRINGER_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_TABASCO_ENCLOSURE:
            trace_func(trace_context, "SAS_TABASCO_ENCLOSURE");
        break;
	    case FBE_CLASS_ID_SAS_VOYAGER_ICM_ENCLOSURE:
            trace_func(trace_context, "SAS_VOYAGER_ICM_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_VIKING_IOSXP_ENCLOSURE:
            trace_func(trace_context, "SAS_VIKING_IOSXP_ENCLOSURE");
        break;
    	case FBE_CLASS_ID_SAS_CAYENNE_IOSXP_ENCLOSURE:
            trace_func(trace_context, "SAS_CAYENNE_IOSXP_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_NAGA_IOSXP_ENCLOSURE:
            trace_func(trace_context, "SAS_NAGA_IOSXP_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_FALLBACK_ENCLOSURE:
            trace_func(trace_context, "SAS_FALLBACK_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_BOXWOOD_ENCLOSURE:
            trace_func(trace_context, "SAS_BOXWOOD_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_KNOT_ENCLOSURE:
            trace_func(trace_context, "SAS_KNOT_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_ANCHO_ENCLOSURE:
            trace_func(trace_context, "SAS_ANCHO_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_PINECONE_ENCLOSURE:
            trace_func(trace_context, "SAS_PINECONE_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_STEELJAW_ENCLOSURE:
            trace_func(trace_context, "SAS_STEELJAW_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_RAMHORN_ENCLOSURE:
            trace_func(trace_context, "SAS_RAMHORN_ENCLOSURE");
        break;
        case FBE_CLASS_ID_SAS_CALYPSO_ENCLOSURE:
            trace_func(trace_context, "SAS_CALYPSO_ENCLOSURE");
            break;
        case FBE_CLASS_ID_SAS_RHEA_ENCLOSURE:
            trace_func(trace_context, "SAS_RHEA_ENCLOSURE");
            break;
        case FBE_CLASS_ID_SAS_MIRANDA_ENCLOSURE:
            trace_func(trace_context, "SAS_MIRANDA_ENCLOSURE");
            break;
        case FBE_CLASS_ID_MIRROR:
            trace_func(trace_context, "MIRROR");
        break;
        case FBE_CLASS_ID_STRIPER:
            trace_func(trace_context, "STRIPER");
        break;
        case FBE_CLASS_ID_PARITY:
            trace_func(trace_context, "PARITY");
        break;
        case FBE_CLASS_ID_VIRTUAL_DRIVE:
            trace_func(trace_context, "VIRTUAL_DRIVE");
        break;
        case FBE_CLASS_ID_PROVISION_DRIVE:
            trace_func(trace_context, "PROVISIONED_DRIVE");
        break;
        case FBE_CLASS_ID_LUN:
            trace_func(trace_context, "LUN");
        break;
        case FBE_CLASS_ID_BVD_INTERFACE:
            trace_func(trace_context, "BVD_INTERFACE");
        break;
        case FBE_CLASS_ID_EXTENT_POOL:
            trace_func(trace_context, "EXTENT_POOL");
        break;
        case FBE_CLASS_ID_EXTENT_POOL_LUN:
            trace_func(trace_context, "EXTENT_POOL_LUN");
        break;
        case FBE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
            trace_func(trace_context, "EXTENT_POOL_METADATA_LUN");
        break;
        case FBE_CLASS_ID_BASE_ENVIRONMENT:
            trace_func(trace_context, "BASE_ENVIRONMENT");
        break;
        case FBE_CLASS_ID_ENCL_MGMT:
            trace_func(trace_context, "ENCL_MGMT");
        break;
        case FBE_CLASS_ID_SPS_MGMT:
            trace_func(trace_context, "SPS_MGMT");
        break;
        case FBE_CLASS_ID_DRIVE_MGMT:
            trace_func(trace_context, "DRIVE_MGMT");
        break;
        case FBE_CLASS_ID_MODULE_MGMT:
            trace_func(trace_context, "MODULE_MGMT");
        break;
        case FBE_CLASS_ID_PS_MGMT:
            trace_func(trace_context, "PS_MGMT");
        break;
        case FBE_CLASS_ID_BOARD_MGMT:
            trace_func(trace_context, "BOARD_MGMT");
        break;
        case FBE_CLASS_ID_COOLING_MGMT:
            trace_func(trace_context, "COOLING_MGMT");
        break;
        default:
            trace_func(trace_context, "UNKNOWN CLASS ID %d", class_id);
        break;
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_object_trace_class_id()
 ******************************************/

/*!***************************************************************************
 * fbe_base_object_trace_get_state_string()
 ******************************************************************************
 * @brief
 *  Return a string for Lifecycle State.
 *
 * @param lifecycle_state - lifecycle states for the object.
 *
 * @return - char * string that represents lifecycle state.
 *
 * @author
 *  05/04/2009 - Created. rfoley
 *
 ****************************************************************/
char *
fbe_base_object_trace_get_state_string(fbe_lifecycle_state_t lifecycle_state)
{
    switch (lifecycle_state)
    {
        case FBE_LIFECYCLE_STATE_SPECIALIZE:
            return("SPECLZ");
            break;
        case FBE_LIFECYCLE_STATE_ACTIVATE:
            return("ACTVT");
            break;
        case FBE_LIFECYCLE_STATE_READY:
            return("READY");
            break;
        case FBE_LIFECYCLE_STATE_HIBERNATE:
            return("HIBR");
            break;
        case FBE_LIFECYCLE_STATE_OFFLINE:
            return("OFFLN");
            break;
        case FBE_LIFECYCLE_STATE_FAIL:
            return("Fail");
            break;
        case FBE_LIFECYCLE_STATE_DESTROY:
            return("DSTRY");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_READY:
            return("PND_RDY");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
            return("PND_ACT");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
            return("PND_HIBR");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
            return("PND_OFFLN");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_FAIL:
            return("PND_FAIL");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
            return("PND_DSTRY");
            break;
        default:
            break;      
    };

    /* For default case */
    return("UNKNOWN");
}
/******************************************
 * end fbe_base_object_trace_get_state_string()
 ******************************************/

/*!***************************************************************************
 * fbe_base_object_trace_state()
 ******************************************************************************
 * @brief
 *  Display's the trace of the base object related to Lifecycle State.
 *
 * @param lifecycle_state - lifecycle states for the object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK 
 *
 * @author
 *  02/13/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t
fbe_base_object_trace_state(fbe_lifecycle_state_t lifecycle_state,
                            fbe_trace_func_t trace_func,
                            fbe_trace_context_t trace_context)
{
    trace_func(trace_context, fbe_base_object_trace_get_state_string(lifecycle_state));

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_object_trace_state()
 ******************************************/
/*!***************************************************************************
 * fbe_base_object_trace_get_level_string()
 ******************************************************************************
 * @brief
 *  Return a string for trace level.
 *
 * @param  trace_level - base object ptr.
 *
 * @return - String that represents trace level.
 *
 * @author
 *  05/04/2009 - Created. rfoley
 *
 ****************************************************************/
char *
fbe_base_object_trace_get_level_string(fbe_trace_level_t trace_level)
{
    switch(trace_level)
    {
        case FBE_TRACE_LEVEL_INVALID:
            return("Invalid");
            break;
        case FBE_TRACE_LEVEL_CRITICAL_ERROR:
            return("Critical Error");
            break;
        case FBE_TRACE_LEVEL_ERROR:
            return("Error");
            break;
        case FBE_TRACE_LEVEL_WARNING:
            return("Warning");
            break;
        case FBE_TRACE_LEVEL_INFO:
            return("Info");
            break;
        case FBE_TRACE_LEVEL_DEBUG_LOW:
            return("Debug Low");
            break;
        case FBE_TRACE_LEVEL_DEBUG_MEDIUM:
            return("Debug Medium");
            break;
        case FBE_TRACE_LEVEL_DEBUG_HIGH:
            return("Debug High");
            break;
        case FBE_TRACE_LEVEL_LAST:
        default:
            break;
    };

    return("UNKNOWN TRACE LEVEL");
}
/******************************************
 * end fbe_base_object_trace_get_level_string()
 ******************************************/


/*!***************************************************************************
 * fbe_base_object_trace_level()
 ******************************************************************************
 * @brief
 *  Display's the trace of the base object related to trace level.
 *
 * @param  trace_level - base object ptr.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK 
 *
 * @author
 *  02/13/2009 - Created. Nishit Singh
 *
 ****************************************************************/
fbe_status_t
fbe_base_object_trace_level(fbe_trace_level_t trace_level,
                            fbe_trace_func_t trace_func,
                            fbe_trace_context_t trace_context)
{
    trace_func(trace_context, fbe_base_object_trace_get_level_string(trace_level));
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_object_trace_level()
 ******************************************/

/*************************
 * end file fbe_base_object_trace.c
 *************************/
