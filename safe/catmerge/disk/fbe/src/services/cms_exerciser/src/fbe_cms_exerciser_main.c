/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_cms_exerciser_main.c
 ***************************************************************************
 *
 *  Description
 *      main entry point for the CMS exerciser service.
 *    
 ***************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_cms_exerciser_interface.h"
#include "fbe_base.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_cms_interface.h"
#include "fbe_cms_exerciser_private.h"


/*local stuff*/
fbe_status_t fbe_cms_exerciser_control_entry(fbe_packet_t * packet);
fbe_service_methods_t fbe_cms_exerciser_service_methods = {FBE_SERVICE_ID_CMS_EXERCISER, fbe_cms_exerciser_control_entry};

typedef struct fbe_cms_exerciser_service_s {
    fbe_base_service_t base_service;
} fbe_cms_exerciser_service_t;

static fbe_cms_exerciser_service_t 					cms_exerciser_service;
static fbe_cms_control_get_access_func_ptrs_t		buffer_func_ptrs;


/*local functions*/
static fbe_status_t cms_exerciser_get_buffer_access_pointers(void);


/**********************************************************************************************************************/
void cms_exerciser_trace(fbe_trace_level_t trace_level,
                             const char * fmt, ...)
{
    va_list args;
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized(&cms_exerciser_service.base_service)) {
        service_trace_level = fbe_base_service_get_trace_level(&cms_exerciser_service.base_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
        FBE_SERVICE_ID_CMS_EXERCISER,
        trace_level,
        FBE_TRACE_MESSAGE_ID_INFO,
        fmt,
        args);
    va_end(args);
}

static fbe_status_t fbe_cms_exerciser_init(void)
{
	fbe_status_t 		status;

    /* Initialize the base service*/
    fbe_base_service_set_service_id(&cms_exerciser_service.base_service, FBE_SERVICE_ID_CMS_EXERCISER);
    fbe_base_service_set_trace_level(&cms_exerciser_service.base_service, fbe_trace_get_default_trace_level());
    fbe_base_service_init(&cms_exerciser_service.base_service);

	/*get the pointers to the cms functions we'll call directly*/
	status = cms_exerciser_get_buffer_access_pointers();
	if (status != FBE_STATUS_OK) {
		cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't get pointer\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    return FBE_STATUS_OK;
}


static fbe_status_t fbe_cms_exerciser_destroy(void)
{
    fbe_base_service_destroy(&cms_exerciser_service.base_service);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_exerciser_control_entry(fbe_packet_t * packet)
{    
    fbe_payload_control_operation_opcode_t control_code;
    fbe_status_t status;

    control_code = fbe_transport_get_control_code(packet);
    if (control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        fbe_cms_exerciser_init();
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    if (fbe_base_service_is_initialized((fbe_base_service_t*)&cms_exerciser_service) == FALSE) {
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    switch(control_code) {
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            fbe_cms_exerciser_destroy();
            fbe_base_service_destroy((fbe_base_service_t*)&cms_exerciser_service);
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_OK;
            break;
        case FBE_CMS_EXERCISER_CONTROL_CODE_EXCLUSIVE_ALLOC_TEST:
            status = fbe_cms_exerciser_control_run_exclusive_alloc_tests(packet);
            break;
        case FBE_CMS_EXERCISER_CONTROL_CODE_GET_INTERFACE_TEST_RESULTS:
            status = fbe_cms_exerciser_control_get_interface_tests_resutls(packet);
            break;
        default:
            fbe_base_service_trace((fbe_base_service_t*)&cms_exerciser_service,
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

static fbe_status_t cms_exerciser_get_buffer_access_pointers(void)
{
	fbe_status_t	status;
	status = cms_exerciser_send_packet_to_service(FBE_CMS_CONTROL_CODE_GET_ACCESS_FUNC_PTRS,
												  &buffer_func_ptrs,
												  sizeof(fbe_cms_control_get_access_func_ptrs_t),
												  FBE_SERVICE_ID_CMS,
												  FBE_PACKAGE_ID_SEP_0);

	return status;
}

fbe_cms_control_get_access_func_ptrs_t * cms_exerciser_get_access_lib_func_ptrs(void)
{
	return &buffer_func_ptrs;
}
