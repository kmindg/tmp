/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_cmi_service_main.c
 ***************************************************************************
 *
 *  Description
 *      This file contains the implementation for the FBE CMI service code
 *      
 *    
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_service.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe_cmi_private.h"
#include "fbe_cmi.h"
#include "fbe/fbe_registry.h"

/*local structures*/
typedef struct fbe_cmi_service_s{
    fbe_base_service_t  base_service;
	fbe_atomic_t 		outstanding_message_counter;
	fbe_atomic_t 		received_message_counter;
	fbe_bool_t 			traffic_disabled;
}fbe_cmi_service_t;

typedef struct conduit_to_package_map_s{
    fbe_cmi_conduit_id_t    conduit_id;
    fbe_package_id_t        package_id;
}conduit_to_package_map_t;

/* Declare our service methods */
fbe_status_t fbe_cmi_control_entry(fbe_packet_t * packet); 
fbe_service_methods_t fbe_cmi_service_methods = {FBE_SERVICE_ID_CMI, fbe_cmi_control_entry};


/* Forward declarations */
static fbe_status_t fbe_cmi_map_client_to_conduit (fbe_cmi_client_id_t client_id, fbe_cmi_conduit_id_t * conduit);
static fbe_status_t cmi_service_get_info(fbe_packet_t *packet);
static fbe_status_t cmi_service_disable_traffic(fbe_packet_t *packet);
static fbe_status_t cmi_service_enable_traffic(fbe_packet_t *packet);
static fbe_status_t cmi_service_get_io_statistics(fbe_packet_t *packet);
static fbe_status_t cmi_service_clear_io_statistics(fbe_packet_t *packet);

/* Static members */
static fbe_cmi_service_t            fbe_cmi_service;
fbe_cmi_client_info_t               client_list[FBE_CMI_CLIENT_ID_LAST];

/* The following code will insure that it will be nor transmissions after peer lost event */
fbe_atomic_t fbe_cmi_service_peer_lost = 0; /* Will be set to 1 when peer lost and reset to 0 when handshake completes after peer reboot*/
typedef enum cmi_service_thread_flag_e{
    CMI_SERVICE_THREAD_RUN,
    CMI_SERVICE_THREAD_STOP,
    CMI_SERVICE_THREAD_DONE
}cmi_service_thread_flag_t;

static fbe_thread_t					cmi_service_thread_handle;
static fbe_rendezvous_event_t		cmi_service_event;
static cmi_service_thread_flag_t	cmi_service_thread_flag;

static fbe_bool_t                   cmi_service_disabled = FBE_FALSE;

static void cmi_service_thread_func(void * context);

static fbe_status_t cmi_service_set_peer_version(fbe_packet_t *packet);
static fbe_status_t cmi_service_clear_peer_version(fbe_packet_t *packet);

static fbe_status_t fbe_cmi_service_state_init(void);

static conduit_to_package_map_t     conduit_package_map [] = 
{
    {FBE_CMI_CONDUIT_ID_TOPLOGY, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_ESP, FBE_PACKAGE_ID_ESP},
    {FBE_CMI_CONDUIT_ID_NEIT, FBE_PACKAGE_ID_NEIT},
	#if 0 /*return when cms is back*/
	{FBE_CMI_CONDUIT_ID_CMS_STATE_MACHINE, FBE_PACKAGE_ID_SEP_0},
	{FBE_CMI_CONDUIT_ID_CMS_TAGS, FBE_PACKAGE_ID_SEP_0},
	#endif 
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE0, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE1, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE2, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE3, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE4, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE5, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE6, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE7, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE8, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE9, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE10, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE11, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE12, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE13, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE14, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE15, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE16, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE17, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE18, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE19, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE20, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE21, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE22, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE23, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE24, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE25, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE26, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE27, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE28, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE29, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE30, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE31, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE32, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE33, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE34, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE35, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE36, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE37, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE38, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE39, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE40, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE41, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE42, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE43, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE44, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE45, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE46, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE47, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE48, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE49, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE50, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE51, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE52, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE53, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE54, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE55, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE56, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE57, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE58, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE59, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE60, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE61, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE62, FBE_PACKAGE_ID_SEP_0},
    {FBE_CMI_CONDUIT_ID_SEP_IO_CORE63, FBE_PACKAGE_ID_SEP_0},
    /*fill new stuff above this line*/
    {FBE_CMI_CONDUIT_ID_INVALID, FBE_PACKAGE_ID_INVALID}
};

STATIC_ASSERT(FBE_CONDUIT_MAP_MAX_CORE_COUNT_NE_64, MAX_CORES <= 64);

/************************************************************************************************************************************/
void
fbe_cmi_trace(fbe_trace_level_t trace_level, const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized(&fbe_cmi_service.base_service)) {
        service_trace_level = fbe_base_service_get_trace_level(&fbe_cmi_service.base_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_CMI,
                     trace_level,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     fmt, 
                     args);
    va_end(args);
}

static fbe_status_t 
fbe_cmi_init(fbe_packet_t * packet)
{
    fbe_cmi_client_id_t     client = FBE_CMI_CLIENT_ID_INVALID;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cmi_sp_state_t		sp_state;
    fbe_u32_t				wait_count = 0;
	fbe_u32_t				wait_limit = 300;   // 12s CMI peer timeout
                                                // 10s to bring up link (recovery, etc.)
                                                // 3s for intermediate drivers
                                                // 5s buffer
    fbe_package_id_t		package_id = FBE_PACKAGE_ID_INVALID;
	EMCPAL_STATUS	        nt_status;


    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,"%s: entry\n", __FUNCTION__);

    fbe_cmi_panic_init();

    fbe_base_service_set_service_id((fbe_base_service_t *) &fbe_cmi_service, FBE_SERVICE_ID_CMI);
    fbe_base_service_set_trace_level((fbe_base_service_t *)&fbe_cmi_service, fbe_trace_get_default_trace_level());
    fbe_base_service_init((fbe_base_service_t *) &fbe_cmi_service);

    /*reset the client list*/
    for (client ++; client < FBE_CMI_CLIENT_ID_LAST; client ++) {
        client_list[client].client_callback = NULL;
        client_list[client].client_context = NULL;
        client_list[client].open_state = FBE_CMI_OPEN_NOT_NEEDED;
        fbe_semaphore_init(&client_list[client].sync_open_sem, 0, 10);
        fbe_spinlock_init(&client_list[client].open_state_lock);

    }

#ifdef C4_INTEGRATED
    /*read the cmi service state from registry*/
    status = fbe_cmi_service_state_init();
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to init service state\n", __FUNCTION__);
        fbe_cmi_service.traffic_disabled = FBE_TRUE;
        cmi_service_handshake_set_peer_alive(FBE_FALSE);
        fbe_cmi_set_current_sp_state(FBE_CMI_STATE_SERVICE_MODE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we don't load the cmi drivers(CmiPci, CMIscd and cmid) in bootflash, just assume both sps are active and peer is not alive.*/
    if (fbe_cmi_service_disabled()) {
        fbe_cmi_init_sp_id();
        fbe_cmi_set_current_sp_state(FBE_CMI_STATE_ACTIVE);
        fbe_cmi_service.traffic_disabled = FBE_TRUE;
        cmi_service_handshake_set_peer_alive(FBE_FALSE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }
#endif

    /*and we also register ourselvs so we can handle handshake callbacks*/
    status =  fbe_cmi_register(FBE_CMI_CLIENT_ID_CMI, fbe_cmi_service_msg_callback, NULL);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: unable to register ourselvs\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	status  = fbe_cmi_service_init_event_process();
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to init handshake module\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	cmi_service_thread_flag = CMI_SERVICE_THREAD_RUN;    
	fbe_rendezvous_event_init(&cmi_service_event);
	fbe_cmi_service_peer_lost = 0;


    /*initialize the kerenl/sim connections*/
    status = fbe_cmi_init_connections();
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: unable to initialize CMI connections\n", __FUNCTION__);
        fbe_cmi_service.traffic_disabled = FBE_TRUE;
        cmi_service_handshake_set_peer_alive(FBE_FALSE);
        fbe_cmi_set_current_sp_state(FBE_CMI_STATE_SERVICE_MODE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }    

	fbe_cmi_service.outstanding_message_counter = 0;
	fbe_cmi_service.received_message_counter = 0;
	fbe_cmi_service.traffic_disabled = FBE_FALSE;

    /*we will start the handshake process by sending a request to the other side and then continue to deal with
    it in the context of the callback*/
    fbe_cmi_send_handshake_to_peer();

    /*we'll have to wait here until we are setteled down as far as who is active and who is not
    this should not take too long*/
    do {
        status = fbe_cmi_get_current_sp_state(&sp_state);
        if (wait_count != 0) {
            fbe_thread_delay(100);
        }
        wait_count++;
    } while ((sp_state == FBE_CMI_STATE_BUSY) && (status == FBE_STATUS_OK) && (wait_count < wait_limit));

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: waited %d times\n", __FUNCTION__, wait_count);

    if (wait_count == wait_limit || status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: handshake with peer failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    {
        fbe_get_package_id(&package_id);
        if (package_id == FBE_PACKAGE_ID_SEP_0) {
            /*initialize memory for SEP IO*/
            status = fbe_cmi_io_init();
            if (status != FBE_STATUS_OK) {
                fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s failed to initialize for SEP IO\n", __FUNCTION__);
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /*register client for SEP IO*/
            status =  fbe_cmi_register(FBE_CMI_CLIENT_ID_SEP_IO, fbe_cmi_service_sep_io_callback, NULL);
            if (status != FBE_STATUS_OK) {
                fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: unable to register sep io client\n", __FUNCTION__);
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }


	/* Clear peer lost event ( fbe_cmi_init_connections may report peer dead which we want to ignore */
	fbe_rendezvous_event_clear(&cmi_service_event);

    /* Start peer lost thread */
    nt_status = fbe_thread_init(&cmi_service_thread_handle, "fbe_cmi_serv", cmi_service_thread_func, NULL);
    if (nt_status != EMCPAL_STATUS_SUCCESS) {
		fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: fbe_thread_init fail\n", __FUNCTION__);
	}

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_cmi_destroy(fbe_packet_t * packet)
{
    fbe_cmi_client_id_t     client = FBE_CMI_CLIENT_ID_INVALID;
    fbe_package_id_t		package_id = FBE_PACKAGE_ID_INVALID;

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s: entry\n", __FUNCTION__);

    if (!fbe_cmi_service_disabled()) {

        fbe_cmi_unregister(FBE_CMI_CLIENT_ID_CMI);

        {
            fbe_get_package_id(&package_id);
            if (package_id == FBE_PACKAGE_ID_SEP_0) {
                /*unregister client for SEP IO*/
                fbe_cmi_unregister(FBE_CMI_CLIENT_ID_SEP_IO);
                /*destroy memory SEP IO*/
                fbe_cmi_io_destroy();
            }
        }

        for (client ++; client < FBE_CMI_CLIENT_ID_LAST; client ++) {
            if(client_list[client].client_callback != NULL){
                fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: client %d did not unregister and we are destroying\n", __FUNCTION__, client);
            }
        }


        cmi_service_thread_flag = CMI_SERVICE_THREAD_STOP; 
        fbe_rendezvous_event_set(&cmi_service_event);
        fbe_thread_wait(&cmi_service_thread_handle);
        fbe_thread_destroy(&cmi_service_thread_handle);

        fbe_cmi_close_connections();

        fbe_cmi_service_destroy_event_process();

        /* This event may be set during fbe_cmi_close_connections -> mark peer dead, so we need to destroy it later */
        fbe_rendezvous_event_destroy(&cmi_service_event);
    }

    client = FBE_CMI_CLIENT_ID_INVALID;
    for (client ++; client < FBE_CMI_CLIENT_ID_LAST; client ++) {
        fbe_spinlock_destroy(&client_list[client].open_state_lock);
        fbe_semaphore_destroy(&client_list[client].sync_open_sem);
    }

    fbe_cmi_panic_destroy();

    fbe_base_service_destroy((fbe_base_service_t *) &fbe_cmi_service);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_cmi_control_entry(fbe_packet_t * packet)
{    
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;

    control_code = fbe_transport_get_control_code(packet);
    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        status = fbe_cmi_init(packet);
        return status;
    }

    if(!fbe_base_service_is_initialized((fbe_base_service_t *) &fbe_cmi_service)){
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    switch(control_code) {
        
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = fbe_cmi_destroy( packet);
            break;
        case FBE_CMI_CONTROL_CODE_GET_INFO:
            status = cmi_service_get_info(packet);
            break;
		case FBE_CMI_CONTROL_CODE_DISABLE_TRAFFIC:
            status = cmi_service_disable_traffic(packet);
            break;
		case FBE_CMI_CONTROL_CODE_ENABLE_TRAFFIC:
            status = cmi_service_enable_traffic(packet);
            break;
        case FBE_CMI_CONTROL_CODE_GET_IO_STATISTICS:
            status = cmi_service_get_io_statistics(packet);
            break;
        case FBE_CMI_CONTROL_CODE_CLEAR_IO_STATISTICS:
            status = cmi_service_clear_io_statistics(packet);
            break;
        case FBE_CMI_CONTROL_CODE_SET_PEER_VERSION:
            status = cmi_service_set_peer_version(packet);
            break;
        case FBE_CMI_CONTROL_CODE_CLEAR_PEER_VERSION:
            status = cmi_service_clear_peer_version(packet);
            break;
        default:
            status = fbe_base_service_control_entry((fbe_base_service_t*)&fbe_cmi_service, packet);
            break;
    }

    return status;
}
/*
client_id - the client who registers and the one that will get the message on the other side
cmi_callback - the function that gets called when there are CMI events
context - context you can use when getting FBE_CMI_EVENT_SP_CONTACT_LOST or FBE_CMI_EVENT_MESSAGE_RECEIVED
(we need these teo since these two messages are asynchronous in nature and the reveiver might need some context
in order to process them.)
*/
fbe_status_t fbe_cmi_register(fbe_cmi_client_id_t client_id, fbe_cmi_event_callback_function_t cmi_callback, void * context)
{
    if ((client_id < 0) || (client_id >= FBE_CMI_CLIENT_ID_LAST)  || (cmi_callback == NULL)) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, Illegal client registration or callback pointer passed.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (client_list[client_id].client_callback != NULL) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, client already registered, check your logic.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    client_list[client_id].client_callback = cmi_callback;
    client_list[client_id].client_context = context;

    return FBE_STATUS_OK;
}
fbe_status_t fbe_cmi_mark_ready(fbe_cmi_client_id_t client_id)
{
    fbe_spinlock_lock(&client_list[client_id].open_state_lock);
    client_list[client_id].open_state |= (FBE_CMI_OPEN_REQUIRED|FBE_CMI_OPEN_LOCAL_READY);
    fbe_spinlock_unlock(&client_list[client_id].open_state_lock);

    return FBE_STATUS_OK;
}
/* Client can use this to open the connection with its counterpart on peer side.
 * It's no-op on the active side.
 * On the passive side, special open message will be sent to peer.  It will be
 * blocked until peer message comes in.  The same message will be resent every
 * 3 second.
 */
fbe_status_t fbe_cmi_sync_open(fbe_cmi_client_id_t client_id)
{
    fbe_cmi_sp_state_t sp_state;

    fbe_spinlock_lock(&client_list[client_id].open_state_lock);
    client_list[client_id].open_state |= FBE_CMI_OPEN_REQUIRED;
    fbe_spinlock_unlock(&client_list[client_id].open_state_lock);

    fbe_cmi_get_current_sp_state(&sp_state);

    // we will retry this forever until open established or
    // we change to active.
    while((sp_state == FBE_CMI_STATE_PASSIVE) &&
          ((client_list[client_id].open_state & FBE_CMI_OPEN_ESTABLISHED) != FBE_CMI_OPEN_ESTABLISHED))
    {
        fbe_cmi_send_open_to_peer(client_id);
        fbe_semaphore_wait_ms(&client_list[client_id].sync_open_sem, 3000);
        fbe_cmi_get_current_sp_state(&sp_state);
    }

    return FBE_STATUS_OK;
}

/* this function will not do callback */
void fbe_cmi_send_mailbomb(fbe_cmi_client_id_t client_id)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cmi_conduit_id_t    conduit_id = FBE_CMI_CONDUIT_ID_INVALID;

    status = fbe_cmi_map_client_to_conduit(client_id, &conduit_id);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, map_client_to_conduit failed, client %d, conduit %d\n", 
            __FUNCTION__, client_id, conduit_id);
        return ;
    }

    status = fbe_cmi_send_mailbomb_to_other_sp(client_id);

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, client %d, conduit %d, status %d\n", 
            __FUNCTION__, client_id, conduit_id, status);

    return;
}

/*we always get back from here and a callback is where we process the result*/
/* message  -->>> !!!! MUST BE PHYSICALLY CONTIGUOUS (use fbe_memory_native_allocate)!!!!*/
void fbe_cmi_send_message(fbe_cmi_client_id_t client_id,
                          fbe_u32_t message_length,
                          fbe_cmi_message_t message,/*!!!! MUST BE PHYSICALLY CONTIGUOUS (use fbe_memory_native_allocate)!!!!*/
                          fbe_cmi_event_callback_context_t context)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cmi_conduit_id_t    conduit_id = FBE_CMI_CONDUIT_ID_INVALID;

    if (client_list[client_id].client_callback == NULL) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, unregistered client %d is trying to send a message\n", 
            __FUNCTION__, client_id);
        return;
    }

    if (!fbe_cmi_is_peer_alive() || fbe_cmi_service.traffic_disabled ||
		((fbe_cmi_service_peer_lost == 1) && (client_id == FBE_CMI_CLIENT_ID_METADATA))){
       /*we just fake a peer dead message on the same context*/
        client_list[client_id].client_callback(FBE_CMI_EVENT_PEER_NOT_PRESENT, message_length, message, context);  
        return;
    }
    
    status = fbe_cmi_map_client_to_conduit(client_id, &conduit_id);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, map_client_to_conduit failed, client %d, conduit %d\n", 
            __FUNCTION__, client_id, conduit_id);
        client_list[client_id].client_callback(FBE_CMI_EVENT_FATAL_ERROR, message_length, message, context);
        return ;
    }

    if (((client_list[client_id].open_state & FBE_CMI_OPEN_REQUIRED) == FBE_CMI_OPEN_REQUIRED) &&
        ((client_list[client_id].open_state & FBE_CMI_OPEN_ESTABLISHED) == 0)) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, client %d has not established\n", 
            __FUNCTION__, client_id);
       /*we just fake a peer dead message on the same context*/
        client_list[client_id].client_callback(FBE_CMI_EVENT_PEER_NOT_PRESENT, message_length, message, context);  
        return;
    }

    fbe_atomic_increment(&fbe_cmi_service.outstanding_message_counter);

    status = fbe_cmi_send_message_to_other_sp(client_id,
                                              conduit_id,
                                              message_length,
                                              message,
                                              context);

    /*what if the peer is not there*/
    if (status == FBE_STATUS_NO_DEVICE) {
        /*we just fake a peer dead message on the same context*/
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, send_message_to_other_sp failed, client %d, conduit %d status: %d\n", 
                      __FUNCTION__, client_id, conduit_id, status);
		fbe_atomic_decrement(&fbe_cmi_service.outstanding_message_counter);
        client_list[client_id].client_callback(FBE_CMI_EVENT_PEER_NOT_PRESENT, message_length, message, context);
    }else if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s, send_message_to_other_sp failed, client %d, conduit %d status: %d\n", 
            __FUNCTION__, client_id, conduit_id, status);
		fbe_atomic_decrement(&fbe_cmi_service.outstanding_message_counter);
        client_list[client_id].client_callback(FBE_CMI_EVENT_FATAL_ERROR, message_length, message, context);
	}

    return;
}

fbe_status_t fbe_cmi_unregister(fbe_cmi_client_id_t client_id)
{
    if ((client_id < 0) || (client_id >= FBE_CMI_CLIENT_ID_LAST)) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, Illegal client unregistration\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (client_list[client_id].client_callback == NULL) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, client already unregistered, check your logic.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    client_list[client_id].client_callback = NULL;
    client_list[client_id].client_context = NULL;
    fbe_spinlock_lock(&client_list[client_id].open_state_lock);
    client_list[client_id].open_state = FBE_CMI_OPEN_NOT_NEEDED;
    fbe_spinlock_unlock(&client_list[client_id].open_state_lock);
    return FBE_STATUS_OK;

}

/* 
   message_length is only valid for received messages. In all other cases, 0 should be passed for length.
 */
fbe_status_t fbe_cmi_call_registered_client(fbe_cmi_event_t event_id,
                                            fbe_cmi_client_id_t client_id,
                                            fbe_u32_t message_length,
                                            fbe_cmi_message_t message,
                                            fbe_cmi_event_callback_context_t context)
{
	fbe_status_t status;

	if (event_id == FBE_CMI_EVENT_MESSAGE_RECEIVED && fbe_cmi_service.traffic_disabled) {
		return FBE_STATUS_OK;
	}

	/* Drop all incoming messages */
	if((event_id == FBE_CMI_EVENT_MESSAGE_RECEIVED) && (fbe_cmi_service_peer_lost == 1) && (client_id == FBE_CMI_CLIENT_ID_METADATA)){
		return FBE_STATUS_OK;	
	}

    if (client_list[client_id].client_callback != NULL) {
        /*for FBE_CMI_EVENT_MESSAGE_RECEIVED and FBE_CMI_EVENT_SP_CONTACT_LOST we use the clinet's context since
        they will be NULL anyway due to their asynchronous nature. We give the user it's registration context
        so he can use it to distintuish which registered entity should get the notification in case he
        has a few clients on the same callback*/
        if ((event_id == FBE_CMI_EVENT_MESSAGE_RECEIVED) || (event_id == FBE_CMI_EVENT_SP_CONTACT_LOST)) {
			if (event_id == FBE_CMI_EVENT_MESSAGE_RECEIVED) {
				fbe_atomic_increment(&fbe_cmi_service.received_message_counter);
			}else{
                fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, Rcvd or contact lost client: %d event: %d\n", __FUNCTION__, client_id, event_id);
			}
			
            status = client_list[client_id].client_callback(event_id, message_length, message, client_list[client_id].client_context);

			if (event_id == FBE_CMI_EVENT_MESSAGE_RECEIVED) {
				fbe_atomic_decrement(&fbe_cmi_service.received_message_counter);
			}

			return status;
        }else{
			if (event_id == FBE_CMI_EVENT_MESSAGE_TRANSMITTED || FBE_CMI_EVENT_PEER_BUSY ||
				event_id == FBE_CMI_EVENT_PEER_NOT_PRESENT || event_id == FBE_CMI_EVENT_FATAL_ERROR) {
				fbe_atomic_decrement(&fbe_cmi_service.outstanding_message_counter);
			}else{
                fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, Unexpected event client: %d event: %d\n", __FUNCTION__, client_id, event_id);
			}

            return client_list[client_id].client_callback(event_id, message_length, message, context);
        }
    }else{
        fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s, CMI event: %d received for unregistered client_id: %d\n", 
            __FUNCTION__, event_id, client_id);
        return FBE_STATUS_BUSY;
    }
}

fbe_status_t fbe_cmi_mark_other_sp_dead(fbe_cmi_conduit_id_t conduit)
{
    fbe_cmi_client_id_t     client = FBE_CMI_CLIENT_ID_INVALID;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cmi_conduit_id_t    temp_conduit = FBE_CMI_CONDUIT_ID_INVALID;
    fbe_cmi_sp_state_t      current_state;

    fbe_cmi_get_current_sp_state(&current_state);

    /*first of all mark the peer sp dead*/
    cmi_service_handshake_set_peer_alive(FBE_FALSE);
	if (current_state != FBE_CMI_STATE_SERVICE_MODE) {
		cmi_service_handshake_set_sp_state(FBE_CMI_STATE_ACTIVE);/*when the peer dies, ipso facto, we are the masters*/
	}

    fbe_cmi_panic_reset();


    /*start with the first client after FBE_CMI_CLIENT_ID_INVALID*/
    for (client++; client < FBE_CMI_CLIENT_ID_LAST; client ++) {
        status = fbe_cmi_map_client_to_conduit(client, &temp_conduit);
        if (status != FBE_STATUS_OK) {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if ((temp_conduit == conduit) && (client_list[client].client_callback != NULL)) {
			if(client != FBE_CMI_CLIENT_ID_METADATA){ /* FBE_CMI_CLIENT_ID_METADATA is a special case */
				client_list[client].client_callback(FBE_CMI_EVENT_SP_CONTACT_LOST, 
													0,
													NULL, 
													client_list[client].client_context);
			} else{
				/* Setting fbe_cmi_service_peer_lost to 1 will preventany future messages to be sent or received untill next handshake */
				fbe_atomic_exchange(&fbe_cmi_service_peer_lost, 1); /* The peer is dead */
				fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, fbe_cmi_service_peer_lost = %lld \n", __FUNCTION__, fbe_cmi_service_peer_lost);

				/* Close the gate right now and deliver peer lost callback later */
				fbe_rendezvous_event_set(&cmi_service_event);
			}
        }

    }

    return FBE_STATUS_OK;
}


static fbe_status_t fbe_cmi_map_client_to_conduit (fbe_cmi_client_id_t client_id, fbe_cmi_conduit_id_t * conduit)
{
    /*some special case for CMI to CMI connections, we will do these based on the package they reside on*/
    if (FBE_CMI_CLIENT_ID_CMI == client_id) {
        fbe_package_id_t        package_id;
        fbe_get_package_id(&package_id);
        switch(package_id) {
        case FBE_PACKAGE_ID_SEP_0:
            *conduit = FBE_CMI_CONDUIT_ID_TOPLOGY;
            break;
        case FBE_PACKAGE_ID_ESP:
            *conduit = FBE_CMI_CONDUIT_ID_ESP;
            break;
        case FBE_PACKAGE_ID_NEIT:
            *conduit = FBE_CMI_CONDUIT_ID_NEIT;
            break;
        default:
            fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, can't map client %d to any package\n", __FUNCTION__, client_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        return FBE_STATUS_OK;

    }

    switch (client_id) {
        case FBE_CMI_CLIENT_ID_METADATA:
        case FBE_CMI_CLIENT_ID_PERSIST:
        case FBE_CMI_CLIENT_ID_JOB_SERVICE:
        case FBE_CMI_CLIENT_ID_DATABASE:
            *conduit = FBE_CMI_CONDUIT_ID_TOPLOGY;
            break;

        case FBE_CMI_CLIENT_ID_ENCL_MGMT:
        case FBE_CMI_CLIENT_ID_SPS_MGMT:
        case FBE_CMI_CLIENT_ID_DRIVE_MGMT:
        case FBE_CMI_CLIENT_ID_MODULE_MGMT:
        case FBE_CMI_CLIENT_ID_PS_MGMT:
        case FBE_CMI_CLIENT_ID_BOARD_MGMT:
        case FBE_CMI_CLIENT_ID_COOLING_MGMT:
            *conduit = FBE_CMI_CONDUIT_ID_ESP;
            break;

        case FBE_CMI_CLIENT_ID_RDGEN:
            *conduit = FBE_CMI_CONDUIT_ID_NEIT;
            break;
#if 0
		case FBE_CMI_CLIENT_ID_CMS_STATE_MACHINE:
			*conduit = FBE_CMI_CONDUIT_ID_CMS_STATE_MACHINE;
			break;

		case FBE_CMI_CLIENT_ID_CMS_TAGS:
			*conduit = FBE_CMI_CONDUIT_ID_CMS_TAGS;
			break;
#endif
        case FBE_CMI_CLIENT_ID_SEP_IO:
            *conduit = fbe_cmi_service_sep_io_get_conduit(0);
            break;
        default:
            fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, can't map client %d to any conduit\n", __FUNCTION__, client_id);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

static fbe_status_t cmi_service_get_info(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_cmi_service_get_info_t *        get_info = NULL; 
    fbe_payload_control_buffer_length_t len = 0;
    fbe_bool_t                          peer_alive = FBE_FALSE;
    fbe_cmi_sp_id_t                     this_sp_id = FBE_CMI_SP_ID_INVALID;
    fbe_cmi_sp_state_t                  sp_state = FBE_CMI_STATE_INVALID;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &get_info); 
    if(get_info == NULL){
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_cmi_service_get_info_t)){
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s %X len invalid\n", __FUNCTION__ , len);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    peer_alive = fbe_cmi_is_peer_alive();
    fbe_cmi_get_sp_id(&this_sp_id);
    fbe_cmi_get_current_sp_state(&sp_state);

    get_info->peer_alive = peer_alive;
    get_info->sp_id = this_sp_id;
    get_info->sp_state = sp_state;

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*map between the package and the conduits we need to open on it*/
fbe_bool_t fbe_cmi_need_to_open_conduit(fbe_cmi_conduit_id_t conduit_id, fbe_package_id_t package_id)
{
    fbe_bool_t  found_conduit = FBE_FALSE;

    conduit_to_package_map_t *map = conduit_package_map;
    while (map->conduit_id != FBE_CMI_CONDUIT_ID_INVALID) {
        if (map->conduit_id == conduit_id && map->package_id == package_id) {
            if (fbe_cmi_service_need_to_open_sep_io_conduit(conduit_id) == FBE_FALSE)
            {
                return FBE_FALSE;
            }
            return FBE_TRUE;
        }

        if (map->conduit_id == conduit_id) {
            found_conduit = FBE_TRUE;/*make sure we know we at lease found a conduit*/
        }

        map++;
    }

    if (!found_conduit) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s No conduit to match found, check tables setup\n", __FUNCTION__);
    }

    return FBE_FALSE;

}

/*makes sure no more cmi messages can arrive from peer*/
static fbe_status_t cmi_service_disable_traffic(fbe_packet_t *packet)
{
	fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
	fbe_u32_t 							delay_count = 0;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);


    /* First let's disable incoming CMI traffic*/
    fbe_cmi_service.traffic_disabled = FBE_TRUE;

	/*let's wait for all pending stuff to complete*/
	while ((fbe_cmi_service.outstanding_message_counter != 0 || fbe_cmi_service.received_message_counter != 0) &&
           (delay_count < 1000)) {
		fbe_thread_delay(10);/*wait 10ms*/
		delay_count++;
	}

	if (delay_count == 1000) {
		 fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, 
                       "%s outstanding: %lld received: %lld timed-out(%d seconds) waiting for CMI to drain\n", 
                       __FUNCTION__, (long long)fbe_cmi_service.outstanding_message_counter,
                       (long long)fbe_cmi_service.received_message_counter, ((1000 * 10) / 1000));
		 fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		 fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		 fbe_transport_complete_packet(packet);
		 return FBE_STATUS_OK;
	}else{
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

}

static fbe_status_t cmi_service_enable_traffic(fbe_packet_t *packet)
{
	fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /*let's enable incoming CMI traffic*/
    fbe_cmi_service.traffic_disabled = FBE_FALSE;

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}


static fbe_status_t cmi_service_get_io_statistics(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *      control_operation = NULL;
    fbe_cmi_service_get_io_statistics_t *  get_stat = NULL; 
    fbe_payload_control_buffer_length_t    len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &get_stat); 
    if(get_stat == NULL){
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_cmi_service_get_io_statistics_t)){
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s %X len invalid\n", __FUNCTION__ , len);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((get_stat->conduit_id >= FBE_CMI_CONDUIT_ID_SEP_IO_FIRST) && 
        (get_stat->conduit_id <= FBE_CMI_CONDUIT_ID_SEP_IO_LAST))
    {
        fbe_cmi_io_get_statistics(get_stat);
    }
    else if (get_stat->conduit_id == 0)
    {
        fbe_u32_t i;
        fbe_cmi_service_get_io_statistics_t temp_stat;

        get_stat->sent_ios = 0;
        get_stat->sent_bytes = 0;
        get_stat->received_ios = 0;
        get_stat->received_bytes = 0;
        for (i = FBE_CMI_CONDUIT_ID_SEP_IO_FIRST; i <= FBE_CMI_CONDUIT_ID_SEP_IO_LAST; i++)
        {
            temp_stat.conduit_id = i;
            fbe_cmi_io_get_statistics(&temp_stat);
            get_stat->sent_ios += temp_stat.sent_ios;
            get_stat->sent_bytes += temp_stat.sent_bytes;
            get_stat->received_ios += temp_stat.received_ios;
            get_stat->received_bytes += temp_stat.received_bytes;
        }
    }
    else
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s %d conduit invalid\n", __FUNCTION__ , get_stat->conduit_id);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t cmi_service_clear_io_statistics(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *      control_operation = NULL;
    fbe_u32_t                              i;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    for (i = FBE_CMI_CONDUIT_ID_SEP_IO_FIRST; i <= FBE_CMI_CONDUIT_ID_SEP_IO_LAST; i++)
    {
        fbe_cmi_io_clear_statistics(i);
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_cmi_send_packet(fbe_packet_t * packet_p)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cmi_conduit_id_t    conduit_id = FBE_CMI_CONDUIT_ID_INVALID;
    fbe_cmi_client_id_t     client_id = FBE_CMI_CLIENT_ID_SEP_IO;
                         
    if (client_list[client_id].client_callback == NULL) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, unregistered client %d is trying to send packet %p\n", 
            __FUNCTION__, client_id, packet_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_FAILED, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    if (!fbe_cmi_is_peer_alive() || fbe_cmi_service.traffic_disabled){
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, peer not alive, packet %p\n", 
            __FUNCTION__, packet_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_FAILED, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    
    status = fbe_cmi_map_client_to_conduit(client_id, &conduit_id);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, map_client_to_conduit failed, client %d, conduit %d\n", 
            __FUNCTION__, client_id, conduit_id);
        fbe_transport_set_status(packet_p, FBE_STATUS_FAILED, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    status = fbe_cmi_send_packet_to_other_sp(packet_p);

    /*what if the peer is not there*/
    if (status == FBE_STATUS_NO_DEVICE) {
        /*we just fake a peer dead message on the same context*/
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, failed no device status %d packet %p\n", 
                      __FUNCTION__, status, packet_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_FAILED, 0);
        fbe_transport_complete_packet(packet_p);
    }else if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, failed status %d packet %p\n", 
                      __FUNCTION__, status, packet_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_FAILED, 0);
        fbe_transport_complete_packet(packet_p);
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_cmi_send_memory(fbe_cmi_memory_t * send_memory_p)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cmi_conduit_id_t    conduit_id = FBE_CMI_CONDUIT_ID_INVALID;
    fbe_cmi_client_id_t     sep_io_id = FBE_CMI_CLIENT_ID_SEP_IO;
    fbe_cmi_client_id_t     client_id = send_memory_p->client_id;

    /* First check SEP_IO client */
    if (client_list[sep_io_id].client_callback == NULL) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, unregistered SEP_IO client %d\n", __FUNCTION__, sep_io_id);
        return status;
    }

    /* Then check client client */
	if (client_list[client_id].client_callback == NULL) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, unregistered client %d\n", __FUNCTION__, client_id);
        return status;
	}
    
    status = fbe_cmi_map_client_to_conduit(sep_io_id, &conduit_id);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, map_client_to_conduit failed, client %d, conduit %d\n", __FUNCTION__, sep_io_id, conduit_id);
        return status;
    }

    if (!fbe_cmi_is_peer_alive() || fbe_cmi_service.traffic_disabled){
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, peer not alive\n", __FUNCTION__);
        /* We just fake a peer dead message on the same context*/
        client_list[client_id].client_callback(FBE_CMI_EVENT_PEER_NOT_PRESENT, send_memory_p->message_length, send_memory_p->message, send_memory_p->context); 
        return status;
    }

    fbe_cmi_service_increase_message_counter();
    status = fbe_cmi_send_memory_to_other_sp(send_memory_p);
    if (status == FBE_STATUS_NO_DEVICE) {
        /* We just fake a peer dead message on the same context*/
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, failed no device status %d\n", __FUNCTION__, status);
        fbe_cmi_service_decrease_message_counter();
        client_list[client_id].client_callback(FBE_CMI_EVENT_PEER_NOT_PRESENT, send_memory_p->message_length, send_memory_p->message, send_memory_p->context); 
    }else if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, failed status %d\n", __FUNCTION__, status);
        fbe_cmi_service_decrease_message_counter();
        client_list[client_id].client_callback(FBE_CMI_EVENT_FATAL_ERROR, send_memory_p->message_length, send_memory_p->message, send_memory_p->context);
    }

    return status;
}

void fbe_cmi_service_increase_message_counter(void)
{
    fbe_atomic_increment(&fbe_cmi_service.outstanding_message_counter);
    return;
}

void fbe_cmi_service_decrease_message_counter(void)
{
    fbe_atomic_decrement(&fbe_cmi_service.outstanding_message_counter);
    return;
}

static void cmi_service_thread_func(void * context)
{
	EMCPAL_STATUS nt_status;
    fbe_cmi_client_id_t     client = FBE_CMI_CLIENT_ID_INVALID;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cmi_conduit_id_t    temp_conduit = FBE_CMI_CONDUIT_ID_INVALID;


	FBE_UNREFERENCED_PARAMETER(context);
	
	memory_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
	                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                     "%s entry\n", __FUNCTION__);

    while(1)    
	{
		nt_status = fbe_rendezvous_event_wait(&cmi_service_event, 0);
		if(cmi_service_thread_flag == CMI_SERVICE_THREAD_RUN) {
			fbe_rendezvous_event_clear(&cmi_service_event);

			/* Setting fbe_cmi_service_peer_lost to 1 will preventany future messages to be sent or received untill next handshake */
			fbe_atomic_exchange(&fbe_cmi_service_peer_lost, 1); /* The peer is dead */
			fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, fbe_cmi_service_peer_lost = %lld \n", __FUNCTION__, fbe_cmi_service_peer_lost);

			/* It is possible that we have incoming events on multiple cores ( Megatron has 4 CMI cores ) 
			We can not interlock them (large preformance impact).
			However, these events run at DPC context and expected to finish within microseconds.
			So 100 ms. delay is VERY safe.			
			*/
			fbe_thread_delay(100); 
			/* Now it is time to deliver peer lost to all the clients */

			/* Start with the first client after FBE_CMI_CLIENT_ID_INVALID */
			client = FBE_CMI_CLIENT_ID_INVALID;
			for (client++; client < FBE_CMI_CLIENT_ID_LAST; client ++) {
				if(client != FBE_CMI_CLIENT_ID_METADATA){ /* FBE_CMI_CLIENT_ID_METADATA is a special case */
					continue;
				}

				status = fbe_cmi_map_client_to_conduit(client, &temp_conduit);
				if (status != FBE_STATUS_OK) {
					continue;
				}

				if (client_list[client].client_callback != NULL) {
					fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, send contact lost to FBE_CMI_CLIENT_ID_METADATA \n", __FUNCTION__);
					client_list[client].client_callback(FBE_CMI_EVENT_SP_CONTACT_LOST, 
														0,
														NULL, 
														client_list[client].client_context);
				}
			}

		} else {
			break;
		}
    } /* while(1) */

    cmi_service_thread_flag = CMI_SERVICE_THREAD_DONE;
	fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

static fbe_bool_t is_peer_before_snake_river = FBE_FALSE;

fbe_bool_t cmi_service_get_peer_version(void)
{
    return (is_peer_before_snake_river);
}

static fbe_status_t cmi_service_set_peer_version(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /*let's set the value*/
    is_peer_before_snake_river = FBE_TRUE;

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t cmi_service_clear_peer_version(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /*let's clear the value*/
    is_peer_before_snake_river = FBE_FALSE;

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_cmi_service_state_init(void)
{
    fbe_status_t status;
    fbe_bool_t defaultInputValue = FBE_FALSE; /* default is NOT in bootflash*/

    /* read BootFromFlash key from registry,
     * if the key is not set, write the default value to registry.
     */
    status = fbe_registry_read(NULL,
                               SERVICES_REG_PATH,
                               BOOT_FROM_FLASH_KEY,
                               &cmi_service_disabled,
                               sizeof(fbe_bool_t),
                               FBE_REGISTRY_VALUE_BINARY,
                               &defaultInputValue,
                               sizeof(fbe_bool_t),
                               FBE_TRUE);     /* if registry is not found, write the default value*/

    if(status != FBE_STATUS_OK)
    {
#if defined(SIMMODE_ENV)
        fbe_package_id_t package_id;
        /*assuming cmi service is enabled if the registry file is not found in simulation.
         *in fbe simulation, some tests may only load physical and neit packages, they don't create registry file in this case.
         */
        fbe_get_package_id(&package_id);
        if (package_id == FBE_PACKAGE_ID_NEIT) {
            cmi_service_disabled = FBE_FALSE;
            return FBE_STATUS_OK;
        }
#endif
        /* unable to read state (bootflash or normal) from registry */
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, unable to read registry value\n", __FUNCTION__);
    }

    return status;
}

fbe_bool_t fbe_cmi_service_disabled(void)
{
    return cmi_service_disabled;
}
