#ifndef fbe_cmi_H
#define fbe_cmi_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_sg_element.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe/fbe_transport.h"
#include "StaticAssert.h"
#include "processorCount.h"

/*!*******************************************************************
 * @def TOTAL_SEP_IO_MEM_MB
 *********************************************************************
 * @brief
 *   This is the total memory in MB assigned from cmm.
 *
 *********************************************************************/
#if defined(SIMMODE_ENV)
#define TOTAL_SEP_IO_MEM_MB 6
#define FBE_CMI_MAX_MESSAGE_SIZE    40960       /* Maximum message to send */
#else
#define TOTAL_SEP_IO_MEM_MB 100
#define FBE_CMI_MAX_MESSAGE_SIZE    (2048 * 512) /* Maximum message to send */
#endif

typedef enum fbe_cmi_conduit_id_e{
    FBE_CMI_CONDUIT_ID_INVALID,
    FBE_CMI_CONDUIT_ID_TOPLOGY,
    FBE_CMI_CONDUIT_ID_ESP,
    FBE_CMI_CONDUIT_ID_NEIT,
	#if 0 /*re-nebale when CMS come back*/
	FBE_CMI_CONDUIT_ID_CMS_STATE_MACHINE,/*used for CMS state machine*/
	FBE_CMI_CONDUIT_ID_CMS_TAGS,/*used for CMS data buffers*/
	#endif
    FBE_CMI_CONDUIT_ID_SEP_IO_FIRST,/*used for PVD single loop failure*/
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE0 = FBE_CMI_CONDUIT_ID_SEP_IO_FIRST,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE1,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE2,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE3,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE4,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE5,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE6,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE7,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE8,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE9,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE10,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE11,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE12,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE13,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE14,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE15,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE16,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE17,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE18,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE19,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE20,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE21,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE22,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE23,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE24,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE25,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE26,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE27,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE28,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE29,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE30,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE31,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE32,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE33,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE34,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE35,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE36,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE37,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE38,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE39,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE40,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE41,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE42,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE43,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE44,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE45,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE46,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE47,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE48,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE49,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE50,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE51,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE52,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE53,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE54,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE55,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE56,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE57,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE58,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE59,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE60,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE61,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE62,
    FBE_CMI_CONDUIT_ID_SEP_IO_CORE63,
    FBE_CMI_CONDUIT_ID_SEP_IO_LAST = FBE_CMI_CONDUIT_ID_SEP_IO_CORE63,

    /*when adding a new conduit, add it above this line and update functions:
	fbe_cmi_kernel_get_cmid_conduit_id + fbe_cmi_kernel_get_fbe_conduit_id+fbe_cmi_need_to_open_conduit
	 + fbe_cmi_map_client_to_conduit + fbe_cmi_common.c->conduit_to_port_map_spa+ conduit_to_port_map_spb*/
    FBE_CMI_CONDUIT_ID_LAST
}fbe_cmi_conduit_id_t;

STATIC_ASSERT(FBE_MAX_CORE_COUNT_NE_64, MAX_CORES <= 64);

/* 
   the fbe_cmi_client_id_t is used by the services who wants to use the FBE CMI service,
   they use it at registration and when they call the service
*/
typedef enum fbe_cmi_client_id_e{
    FBE_CMI_CLIENT_ID_INVALID,
    FBE_CMI_CLIENT_ID_CMI,/*used to send handshake commands between two sides*/
    FBE_CMI_CLIENT_ID_METADATA,
    FBE_CMI_CLIENT_ID_ENCL_MGMT,
    FBE_CMI_CLIENT_ID_SPS_MGMT,
    FBE_CMI_CLIENT_ID_DRIVE_MGMT,
    FBE_CMI_CLIENT_ID_MODULE_MGMT, 
    FBE_CMI_CLIENT_ID_PS_MGMT,           
    FBE_CMI_CLIENT_ID_RDGEN,
    FBE_CMI_CLIENT_ID_BOARD_MGMT,
    FBE_CMI_CLIENT_ID_PERSIST,
    FBE_CMI_CLIENT_ID_COOLING_MGMT,
    FBE_CMI_CLIENT_ID_DATABASE,
    FBE_CMI_CLIENT_ID_JOB_SERVICE,
	#if 0
	FBE_CMI_CLIENT_ID_CMS_STATE_MACHINE,
	FBE_CMI_CLIENT_ID_CMS_TAGS,
	#endif
    FBE_CMI_CLIENT_ID_SEP_IO,
    FBE_CMI_CLIENT_ID_LAST    
}fbe_cmi_client_id_t;

/* Description:
 *      An enumerated type for all the event-types which could
 *      provoke the cmi_shim to call back up to
 *      a client in order to request some service.
 *
 * Members:
 *  FBE_CMI_EVENT_MESSAGE_TRANSMITTED:  a message transmission
 *      attempt has completed.  The <data> argument to the event-handler
 *      function points to a fbe_cmi_MESSAGE_TRANSMISSION_EVENT_DATA packet
 *      describing the message.
 *  FBE_CMI_EVENT_MESSAGE_RECEIVED:  a message has been
 *      received.  The <data> argument to the event-handler function
 *      points to a fbe_cmi_MESSAGE_RECEPTION_EVENT_DATA packet describing
 *      the message. The context you get is the one you used when registering
 *  FBE_CMI_EVENT_DMA_ADDRESSES_NEEDED: The client on the peer
 *      sent data that needs to be DMAed into memory.   The <data> argument 
 *      to the event-handler function points to a 
 *      fbe_cmi_FIXED_DATA_DMA_NEEDED_EVENT_DATA packet describing
 *      the message.
 *  FBE_CMI_EVENT_CLOSE_COMPLETED:  a previous close
 *      operation has completed.  The <data> argument to the event-handler
 *      function points to a fbe_cmi_CLOSE_COMPLETION_EVENT_DATA packet
 *      describing the Conduit that finished closing.
 *  FBE_CMI_EVENT_SP_CONTACT_LOST:  the CMI driver has lost contact
 *      with a remote SP with which it was previously able to communicate.
 *      The context you get is the one you used when registering
 *  FBE_CMI_EVENT_FATAL_ERROR: there is something seriously wrong with the CMI
 *      bus, we may be doing something illeagal or can't talk to it at all
 *  FBE_CMI_EVENT_PEER_NOT_PRESENT: trying to send a message to a non present peer
 *  FBE_CMI_EVENT_PEER_BUSY: the peer is there but the service/object did not have time to register with the cmi service yet, you can retry that
 *  FBE_CMI_EVENT_INVALID:  terminator for the enumeration.
 */
typedef enum fbe_cmi_event_e {
    FBE_CMI_EVENT_MESSAGE_TRANSMITTED,
    FBE_CMI_EVENT_MESSAGE_RECEIVED,
    FBE_CMI_EVENT_CLOSE_COMPLETED,
    FBE_CMI_EVENT_SP_CONTACT_LOST,
    FBE_CMI_EVENT_DMA_ADDRESSES_NEEDED,
    FBE_CMI_EVENT_PEER_NOT_PRESENT,
    FBE_CMI_EVENT_FATAL_ERROR,
    FBE_CMI_EVENT_PEER_BUSY,
    FBE_CMI_EVENT_INVALID           /* terminator for enumeration*/
} fbe_cmi_event_t;

typedef void * fbe_cmi_event_callback_context_t;
typedef void * fbe_cmi_message_t;
typedef fbe_status_t (* fbe_cmi_event_callback_function_t) (fbe_cmi_event_t event, fbe_u32_t user_message_length, fbe_cmi_message_t user_message, fbe_cmi_event_callback_context_t context);

typedef struct fbe_cmi_memory_s
{
    fbe_cmi_client_id_t client_id;
    fbe_cmi_event_callback_context_t context;
    /* Fixed data */
    fbe_u64_t source_addr;
    fbe_u64_t dest_addr;
    fbe_u32_t data_length;
    /* Float data */
    fbe_cmi_message_t message;
    fbe_u32_t message_length;
} fbe_cmi_memory_t;
#define MAX_SEP_IO_MEM_MSG_LEN 64    /* Max length for message_length */
#define MAX_SEP_IO_MEM_LENGTH (1024*1024) /* Max length for data_length */

/*this function is used by any service to register to get callbacks from the cmi service*/
fbe_status_t fbe_cmi_register(fbe_cmi_client_id_t client_id, fbe_cmi_event_callback_function_t cmi_callback, void *context);
fbe_status_t fbe_cmi_unregister(fbe_cmi_client_id_t client_id);
fbe_status_t fbe_cmi_sync_open(fbe_cmi_client_id_t client_id);
fbe_status_t fbe_cmi_mark_ready(fbe_cmi_client_id_t client_id);

/*this function is used to send a message to the same service on the other SP
client_id - the client we send the message from/to, it can be a service or anything else, use fbe_cmi_client_id_t to add yours
message_length - the size of the message user send
fbe_cmi_message_t - user data structure, casted to fbe_cmi_message_t !!!! MUST BE PHYSICALLY CONTIGUOUS (use fbe_memory_native_allocate)!!!!
fbe_cmi_event_callback_context_t - context for completing processig once a FBE_CMI_EVENT_MESSAGE_TRANSMITTED is received  */
void fbe_cmi_send_message(fbe_cmi_client_id_t client_id,
                          fbe_u32_t message_length,
                          fbe_cmi_message_t message,/*!!!! MUST BE PHYSICALLY CONTIGUOUS (use fbe_memory_native_allocate)!!!!*/
                          fbe_cmi_event_callback_context_t context);
void fbe_cmi_send_mailbomb(fbe_cmi_client_id_t client_id);
fbe_status_t fbe_cmi_send_packet(fbe_packet_t * packet_p);
fbe_status_t fbe_cmi_io_init_data_memory(fbe_u64_t total_mem_mb, void * virtual_addr);
fbe_status_t fbe_cmi_send_memory(fbe_cmi_memory_t * send_memory_p);
fbe_u64_t fbe_cmi_io_get_physical_address(void *virt_address);


fbe_status_t fbe_cmi_get_sp_id(fbe_cmi_sp_id_t *cmi_sp_id);
fbe_status_t fbe_cmi_get_current_sp_state(fbe_cmi_sp_state_t *get_state);
fbe_bool_t fbe_cmi_is_peer_alive(void);
fbe_status_t fbe_cmi_panic_get_permission(fbe_bool_t b_panic_this_sp);

/* When database turns into service mode, it will set the SP into service mode too */
void fbe_cmi_set_current_sp_state(fbe_cmi_sp_state_t state);
void fbe_cmi_set_peer_service_mode(fbe_bool_t alive);
void fbe_cmi_send_handshake_to_peer(void); /* move from fbe_cmi_private.h */
void fbe_cmi_send_open_to_peer(fbe_cmi_client_id_t client_id);
fbe_bool_t fbe_cmi_is_active_sp(void);
fbe_bool_t fbe_cmi_service_disabled(void);

#endif /*FBE_CMI_H */

