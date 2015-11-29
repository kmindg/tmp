/**********************************************************************
 *  Copyright (C)  EMC Corporation 2011
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_system_configuration_lib.c
 ***************************************************************************
 * @brief
 *  This file is in charge of giving the user the parameters of max 
 *  objects, based on the the platform type.
 *  
 ***************************************************************************/

#include "fbe_system_limits.h"
#include "spid_enum_types.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_service_manager_interface.h"


/*functions*/

static fbe_status_t system_config_get_max_pp_objects(fbe_u32_t *max_objects);
static fbe_status_t system_config_get_max_sep_objects(fbe_u32_t *max_objects);
static fbe_status_t system_config_get_max_esp_objects(fbe_u32_t *max_objects);
static fbe_status_t system_config_get_max_neit_objects(fbe_u32_t *max_objects);

/*********************************************************************************************************************/

fbe_status_t fbe_system_limits_get_max_objects(fbe_package_id_t package, fbe_u32_t *max_objects)
{
	fbe_status_t		status;
    
   switch (package) {
	case FBE_PACKAGE_ID_PHYSICAL:
		status = system_config_get_max_pp_objects(max_objects);
		break;
	case FBE_PACKAGE_ID_SEP_0:
		status = system_config_get_max_sep_objects(max_objects);
		break;
	case FBE_PACKAGE_ID_NEIT:
		status = system_config_get_max_neit_objects(max_objects);
		break;
	case FBE_PACKAGE_ID_ESP:
		status = system_config_get_max_esp_objects(max_objects);
		break;
   default:
	   fbe_base_library_trace (FBE_LIBRARY_ID_SYSTEM_LIMITS,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s:unrecognized package %d\n",__FUNCTION__, package);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	return status;
}


static fbe_status_t system_config_get_max_pp_objects(fbe_u32_t *max_objects)
{
	
    *max_objects = FBE_MAX_PHYSICAL_OBJECTS;

	return FBE_STATUS_OK;
}

static fbe_status_t system_config_get_max_sep_objects(fbe_u32_t *max_objects)
{
    *max_objects = FBE_MAX_SEP_OBJECTS;

	return FBE_STATUS_OK;
}

static fbe_status_t system_config_get_max_esp_objects(fbe_u32_t *max_objects)
{

	*max_objects = FBE_MAX_ESP_OBJECTS;/*should be more than enough for ESP*/
	return FBE_STATUS_OK;
}

static fbe_status_t system_config_get_max_neit_objects(fbe_u32_t *max_objects)
{

	*max_objects = 0;/*NEIT does not instantiate any objects*/
	return FBE_STATUS_OK;
}


