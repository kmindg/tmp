#ifndef FBE_CMI_PRIVATE_H
#define FBE_CMI_PRIVATE_H

#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe_cmi.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe_cmi_io.h"
#include "processorCount.h"

#define	FBE_CMI_VERSION					1
#define FBE_CMI_IO_MAX_CONDUITS         MAX_CORES

typedef enum fbe_cmi_internal_message_type_e{
    FBE_CMI_INTERNAL_MESSAGE_INVALID,
    FBE_CMI_INTERNAL_MESSAGE_HANDSHAKE,
    FBE_CMI_INTERNAL_MESSAGE_OPEN,
    FBE_CMI_INTERNAL_MESSAGE_LAST
}fbe_cmi_internal_message_type_t;

/* Description:
 *      An enumerated flags to track the open status.
 */
typedef enum fbe_cmi_open_state_e {
    FBE_CMI_OPEN_NOT_NEEDED       = 0x0000,  // nothing needed.
    FBE_CMI_OPEN_REQUIRED         = 0x0001,  // local/remote open received.  When this is set
                                             // the sync open is required prior to processing
                                             // any other message.
    FBE_CMI_OPEN_LOCAL_READY      = 0x0002,  // local is ready to process open from peer
    FBE_CMI_OPEN_RECEIVED         = 0x0004,  // peer open arrived.  It will be cleared after
                                             // local replies.
    FBE_CMI_OPEN_ESTABLISHED      = 0x0008,  // local has replied the open.
    FBE_CMI_OPEN_STATE_INVALID    = 0xffff
} fbe_cmi_open_state_t;

typedef struct fbe_cmi_handshake_info_s{
    fbe_u32_t					fbe_cmi_version;
    fbe_cmi_sp_state_t			current_state;
}fbe_cmi_handshake_info_t;

typedef struct fbe_cmi_open_info_s{
    fbe_u32_t					fbe_cmi_version;
    fbe_cmi_client_id_t			client_id;
}fbe_cmi_open_info_t;

typedef struct fbe_cmi_internal_message_s{
    fbe_cmi_internal_message_type_t	internal_msg_type;
    union{
        fbe_cmi_handshake_info_t	handshake;
        fbe_cmi_open_info_t         open;
    }internal_msg_union;
}fbe_cmi_internal_message_t;

typedef struct fbe_cmi_client_info_s{
    fbe_cmi_event_callback_function_t   client_callback;
    void *                              client_context;
	volatile fbe_cmi_open_state_t       open_state;
    fbe_spinlock_t      open_state_lock;
	fbe_semaphore_t                     sync_open_sem;
}fbe_cmi_client_info_t;

void fbe_cmi_trace(fbe_trace_level_t trace_level, const char * fmt, ...) __attribute__((format(__printf_func__,2,3)));
fbe_status_t fbe_cmi_init_connections(void);
fbe_status_t fbe_cmi_close_connections(void);
fbe_status_t fbe_cmi_init_sp_id(void);

/* Description:
 *      Function used to open a CMI Conduit.  
  *     Only one opener at a time is allowed
 *      on a given Conduit on a given SP, since only a single event
 *      callback routine can be associated with the Conduit.
 *
 *		Possible return values are:
 *
 *		FBE_STATUS_OK:  the open request succeeded.
 *		FBE_STATUS_GENERIC_FAILURE:  otherwise.
 *
 * Arguments:
 *  conduit_id:  the one to open.
 *      event_callback_routine:  pointer to the function which the FBE CMI shim
 *      must call in order to handle all events (incoming
 *      traffic, errors, etc.) that occur on the Conduit.  The function
 *      takes two arguments:  a description of the type of event that
 *      occurred, and a pointer to some event-specific information.
 *      +---------------------------------------------------------------+
 *      |   WARNING!  A Conduit callback routine must be capable of     |
 *      |   running at DISPACH_LEVEL.  A callback routine is allowed    |
 *      |   to submit additional CMI message transmission requests of   |
 *      |   its own, but it must not block waiting for the completion   |
 *      |   notification event of such a request.  It cannot block      |
 *      |   waiting for resources to be freed by a previous request,    |
 *      |   or for any resources dependent on a previous request.       |
 *      +---------------------------------------------------------------+
 *      A callback routine's return status is ignored for most events,
 *      but is interpreted for fbe_cmi_EVENT_MESSAGE_RECEIVED events.
 *      See the description of fbe_cmi_MESSAGE_RECEPTION_EVENT_DATA below
 *      for details.
 *  confirm_reception:  TRUE if message receptions must be ACKed.
 *  notify_of_remote_array_failures:  TRUE if the opener wants to be notified
 *      about all SP failures (even SPs in remote arrays), not just about
 *      peer SPs in the local array.
 */

fbe_status_t fbe_cmi_open_conduit(fbe_cmi_conduit_id_t conduit_id, 
                                  fbe_cmi_event_callback_function_t event_callback_function,
                                  fbe_bool_t confirm_reception,
                                  fbe_bool_t notify_of_remote_array_failures);



/* Description:
 *      Function used to close a CMI Conduit that was previously opened.
 *
 *		FBE_STATUS_OK:  the open request succeeded.
 *		FBE_STATUS_GENERIC_FAILURE:  otherwise.
 *
 * Members:
 *  close_handle:  an opaque but unique identifier for this close
 *      attempt, usually the address of some data structure
 *      that persists for the duration of the close attempt.
 *  conduit_id:  the one to close.
 *  close_status:  upon completion of the close attempt, this field
 *      will be updated to indicate whether the close completed
 *      correctly (STATUS_SUCCESS), or what kind of error occurred.
 *		See the description of CMI_CONDUIT_CLOSE_COMPLETION_EVENT_DATA
 *		below for details on specific <close_status> values.  This field
 *		is not filled in by the original caller, and only becomes valid
 *		when the close attempt finishes and the closer receives a callback
 *		event of variety Cmi_Conduit_Event_Close_Completed.
 */
fbe_status_t fbe_cmi_close_conduit(fbe_cmi_conduit_id_t conduit_id);
fbe_status_t fbe_cmi_send_mailbomb_to_other_sp(fbe_cmi_conduit_id_t conduit_id);

fbe_status_t fbe_cmi_send_message_to_other_sp(fbe_cmi_client_id_t client_id,
                                              fbe_cmi_conduit_id_t	conduit,
                                              fbe_u32_t message_length,
                                              fbe_cmi_message_t message,
                                              fbe_cmi_event_callback_context_t context);


fbe_status_t fbe_cmi_call_registered_client(fbe_cmi_event_t event_id,
                                            fbe_cmi_client_id_t client_id,
                                            fbe_u32_t         message_length,
                                            fbe_cmi_message_t message,
                                            fbe_cmi_event_callback_context_t context);
        
fbe_status_t fbe_cmi_mark_other_sp_dead(fbe_cmi_conduit_id_t conduit);

fbe_status_t fbe_cmi_service_msg_callback(fbe_cmi_event_t event, fbe_u32_t user_message_length, fbe_cmi_message_t user_message, fbe_cmi_event_callback_context_t context);
fbe_status_t fbe_cmi_service_init_event_process(void);
void fbe_cmi_service_destroy_event_process(void);
void cmi_service_handshake_set_peer_alive(fbe_bool_t alive);
fbe_bool_t fbe_cmi_need_to_open_conduit(fbe_cmi_conduit_id_t conduit_id, fbe_package_id_t package_id);
void cmi_service_handshake_set_sp_state(fbe_cmi_sp_state_t set_state);

fbe_status_t fbe_cmi_send_packet_to_other_sp(fbe_packet_t * packet_p);
fbe_status_t fbe_cmi_service_sep_io_callback(fbe_cmi_event_t event, 
											 fbe_u32_t cmi_message_length, 
											 fbe_cmi_message_t cmi_message,  
											 fbe_cmi_event_callback_context_t context);
fbe_status_t fbe_cmi_io_start_send(fbe_cmi_io_context_info_t * ctx_info,
                                   fbe_packet_t * packet_p);
fbe_status_t fbe_cmi_send_memory_to_other_sp(fbe_cmi_memory_t * send_memory_p);
fbe_status_t fbe_cmi_memory_start_send(fbe_cmi_io_context_info_t * ctx_info, fbe_cmi_memory_t * send_memory_p);
fbe_status_t fbe_cmi_io_return_send_context_for_memory(fbe_cmi_io_context_info_t * ctx_info);

fbe_status_t fbe_cmi_io_read_request_preprocess(fbe_cmi_io_context_info_t * ctx_info, 
                                                fbe_packet_t * packet_p, 
                                                fbe_u32_t max_sgl_count,
                                                fbe_u32_t * read_sgl_count,
                                                fbe_bool_t * read_copy_needed);
fbe_status_t fbe_cmi_io_write_request_preprocess(fbe_cmi_io_context_info_t * ctx_info, 
                                                 fbe_packet_t * packet_p, 
                                                 fbe_sg_element_t **sg_list);

fbe_u64_t fbe_cmi_io_get_peer_slot_address(fbe_bool_t is_request, fbe_u32_t pool_id, fbe_u32_t slot);
fbe_status_t fbe_cmi_io_init(void);
void fbe_cmi_io_destroy(void);
fbe_status_t fbe_cmi_io_return_held_context(fbe_cmi_io_context_info_t *ctx_info);

//fbe_u64_t fbe_cmi_io_get_physical_address(void *virt_address);
fbe_cmi_conduit_id_t fbe_cmi_service_sep_io_get_conduit(fbe_cpu_id_t cpu_id);
fbe_bool_t fbe_cmi_service_need_to_open_sep_io_conduit(fbe_cmi_conduit_id_t conduit_id);
fbe_status_t fbe_cmi_io_scan_for_cancelled_packets(void);

fbe_status_t fbe_cmi_io_cancel_thread_init(void);
fbe_status_t fbe_cmi_io_cancel_thread_destroy(void);
fbe_status_t fbe_cmi_io_cancel_function(fbe_packet_completion_context_t context);
fbe_status_t fbe_cmi_io_cancel_thread_notify(void);
fbe_status_t fbe_cmi_io_get_statistics(fbe_cmi_service_get_io_statistics_t * get_stat);
fbe_status_t fbe_cmi_io_clear_statistics(fbe_cmi_conduit_id_t conduit_id);
void fbe_cmi_service_increase_message_counter(void);
void fbe_cmi_service_decrease_message_counter(void);
void fbe_cmi_panic_init(void);
void fbe_cmi_panic_destroy(void);
void fbe_cmi_panic_reset(void);

fbe_bool_t cmi_service_get_peer_version(void);

/* Will be set to TRUE when peer lost and reset to FALSE when handshake completes after peer reboot*/
extern fbe_atomic_t fbe_cmi_service_peer_lost;

#endif /*FBE_CMI_PRIVATE_H*/
  
