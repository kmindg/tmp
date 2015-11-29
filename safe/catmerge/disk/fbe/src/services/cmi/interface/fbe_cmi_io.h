#ifndef FBE_CMI_IO_H
#define FBE_CMI_IO_H

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "CmiUpperInterface.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe_cmi_private.h"
#include "fbe/fbe_winddk.h"

/*!*******************************************************************
 * @def MAX_SEP_IO_SLOTS
 *********************************************************************
 * @brief
 *   This is the slots that FBE CMI allocates during initialization.
 *
 *********************************************************************/
//#define MAX_SEP_IO_SLOTS 3

/*!*******************************************************************
 * @def MAX_SEP_IO_DATA_LENGTH
 *********************************************************************
 * @brief
 *   This is the data area that FBE CMI allocates during initialization.
 *
 *********************************************************************/
#define MAX_SEP_IO_DATA_LENGTH (0x800 * 520)
#define LARGE_SEP_IO_DATA_LENGTH (0x800 * 520)
#define SMALL_SEP_IO_DATA_LENGTH (0x80 * 520)

/*!*******************************************************************
 * @def MAX_SEP_IO_SG_COUNT
 *********************************************************************
 * @brief
 *   This is the max SG elements allowed for data transfer.
 *   128 for data and 1 for NULL terminator.
 *
 *********************************************************************/
#define MAX_SEP_IO_SG_COUNT 129

#define MAX_SEP_IO_DATA_POOLS 2
#define INVALID_SEP_IO_POOL_SLOT 0xFFFFFFFF


/*!*******************************************************************
 * @enum fbe_cmi_io_message_type_e
 *********************************************************************
 * @brief
 *  Message types that FBE CMI uses for SEP IO.
 *
 *********************************************************************/
typedef enum fbe_cmi_io_message_type_e {
	FBE_CMI_IO_MESSAGE_TYPE_INVALID,
	FBE_CMI_IO_MESSAGE_TYPE_PACKET_REQUEST,
	FBE_CMI_IO_MESSAGE_TYPE_PACKET_RESPONSE,
	FBE_CMI_IO_MESSAGE_TYPE_TRANSLATION_INFO,
	FBE_CMI_IO_MESSAGE_TYPE_PACKET_ABORT,
	FBE_CMI_IO_MESSAGE_TYPE_SEND_MEMORY,
}fbe_cmi_io_message_type_t;

typedef enum fbe_cmi_io_context_attr_e {
	FBE_CMI_IO_CONTEXT_ATTR_HOLD            = 0x00000001,	/* This context should not be reused */
	FBE_CMI_IO_CONTEXT_ATTR_NEED_START      = 0x00000002,	/* This context needs to be started */
	FBE_CMI_IO_CONTEXT_ATTR_ABORT_SENT      = 0x00000004,	/* This context has sent abort to peer */
	FBE_CMI_IO_CONTEXT_ATTR_READ_COPY       = 0x00000008,	/* This context need to copy read data to packet */
	FBE_CMI_IO_CONTEXT_ATTR_MEMORY_SENT     = 0x00000010,	/* This context has memory sent to other SP */
}fbe_cmi_io_context_attr_t;

/*!*******************************************************************
 * @struct fbe_cmi_fixed_data_blob_t
 *********************************************************************
 * @brief
 *  This is the definition for blob structure.
 *  It is used to describe destination address for fixed data transfer.
 *
 *********************************************************************/
typedef struct fbe_cmi_fixed_data_blob_s
{
    CMI_PHYSICAL_SG_ELEMENT				blob_sgl[MAX_SEP_IO_SG_COUNT];
} fbe_cmi_fixed_data_blob_t;

/*!*******************************************************************
 * @struct fbe_cmi_io_message_info_t
 *********************************************************************
 * @brief
 *  This is structure for SEP IO message.
 *
 *********************************************************************/
typedef struct fbe_cmi_io_message_info_s
{
    PEMCPAL_IRP	                        pirp;
    CMI_TRANSMIT_MESSAGE_IOCTL_INFO		cmid_ioctl;
    fbe_cmi_client_id_t					client_id;
    fbe_cmi_event_callback_function_t 	callback;
    fbe_cmi_event_callback_context_t 	context;
    fbe_cmi_message_t 					user_msg;
    CMI_PHYSICAL_SG_ELEMENT				cmi_sgl[2];
    CMI_VIRTUAL_SG_ELEMENT	            virtual_float_sgl[2];
    CMI_PHYSICAL_SG_ELEMENT				fixed_data_sgl[MAX_SEP_IO_SG_COUNT];
    fbe_cmi_fixed_data_blob_t           fixed_data_blob;
} fbe_cmi_io_message_info_t;

/*!*******************************************************************
 * @struct fbe_cmi_io_packet_request_t
 *********************************************************************
 * @brief
 *  This is floating data area for packet request.
 *
 *********************************************************************/
typedef struct fbe_cmi_io_packet_request_s
{
    /* from packet */
    fbe_object_id_t                     object_id;
    fbe_cpu_id_t                        cpu_id;
    fbe_packet_attr_t                   packet_attr;
    fbe_packet_priority_t               packet_priority;
    fbe_traffic_priority_t              traffic_priority;
    fbe_time_t                          physical_drive_service_time_ms; /*! Total time spent at PDO. */

    union {
        struct {
            /* from block payload */
            fbe_payload_block_operation_opcode_t	block_opcode;
            fbe_payload_block_operation_flags_t	    block_flags;
            fbe_lba_t								lba;
            fbe_block_count_t						block_count;
            fbe_block_size_t						block_size;
            fbe_block_size_t						optimum_block_size;
            fbe_payload_sg_descriptor_t	            payload_sg_descriptor;
        } block_payload;

        struct {
            fbe_payload_control_operation_opcode_t	control_opcode;		
            fbe_payload_control_buffer_length_t		buffer_length; 
        } control_payload;
    } u;
} fbe_cmi_io_packet_request_t;

/*!*******************************************************************
 * @struct fbe_cmi_io_packet_status_t
 *********************************************************************
 * @brief
 *  This is floating data area for packet response.
 *
 *********************************************************************/
typedef struct fbe_cmi_io_packet_status_s
{
    fbe_packet_status_t                     packet_status;

    union {
        struct {
            fbe_payload_block_operation_status_t	block_status;
            fbe_payload_block_operation_qualifier_t block_status_qualifier;
            fbe_time_t                              retry_msecs;
            fbe_lba_t                               bad_lba;
            fbe_time_t                              physical_drive_service_time_ms;
        } block_status;

        struct {
            fbe_payload_control_status_t			control_status;
            fbe_payload_control_status_qualifier_t  control_status_qualifier;
        } control_status;
    } u;
} fbe_cmi_io_packet_status_t;

/*!*******************************************************************
 * @struct fbe_cmi_io_translation_table_t
 *********************************************************************
 * @brief
 *  This is the starting physical address of each pool.
 *
 *********************************************************************/
typedef struct fbe_cmi_io_translation_table_s
{
    fbe_bool_t table_valid;
    fbe_u64_t send_slot_address[MAX_SEP_IO_DATA_POOLS];
    fbe_u64_t receive_slot_address[MAX_SEP_IO_DATA_POOLS];
} fbe_cmi_io_translation_table_t;

/*!*******************************************************************
 * @struct fbe_cmi_io_send_memory_t
 *********************************************************************
 * @brief
 *  This is the float data structure for sending memory.
 *
 *********************************************************************/
typedef struct fbe_cmi_io_send_memory_s
{
    fbe_cmi_client_id_t                 client_id;
    fbe_cmi_event_callback_context_t    context;
    fbe_u32_t                           message_length;
    fbe_u8_t                            message[MAX_SEP_IO_MEM_MSG_LEN];
} fbe_cmi_io_send_memory_t;

/*!*******************************************************************
 * @struct fbe_cmi_io_float_data_t
 *********************************************************************
 * @brief
 *  This is the structure for floating data area.
 *
 *********************************************************************/
typedef struct fbe_cmi_io_float_data_s
{
    fbe_cmi_io_message_type_t           msg_type;
    fbe_u32_t                           pool;
    fbe_u32_t                           slot;
    fbe_u32_t                           fixed_data_length;
    void *                              original_packet;
    fbe_payload_opcode_t                payload_opcode;

    union {
        /* for FBE_CMI_IO_MESSAGE_TYPE_PACKET_REQUEST */
        fbe_cmi_io_packet_request_t request_info;
        /* for FBE_CMI_IO_MESSAGE_TYPE_PACKET_RESPONSE */
        fbe_cmi_io_packet_status_t response_info;
        /* for FBE_CMI_IO_MESSAGE_TYPE_TRANSLATION_INFO */
        fbe_cmi_io_translation_table_t translation_info;
        /* for FBE_CMI_IO_MESSAGE_TYPE_SEND_MEMORY */
        fbe_cmi_io_send_memory_t memory_info;
    } msg;
} fbe_cmi_io_float_data_t;


/*!*******************************************************************
 * @struct fbe_cmi_io_context_info_t
 *********************************************************************
 * @brief
 *  This structure contains context information for SEP IO.
 *
 *********************************************************************/
typedef struct fbe_cmi_io_context_info_s
{
    fbe_cmi_io_message_info_t           kernel_message;

    fbe_cmi_io_float_data_t             float_data;
    fbe_u32_t                           pool;
    fbe_u32_t                           slot;
    fbe_cmi_io_context_attr_t           context_attr;

    /* the following fields are for receive side only */
    void *packet;
    void *data;
    fbe_sg_element_t sg_element[2];
    fbe_u32_t alloc_length; /* the allocated lenght for data */

} fbe_cmi_io_context_info_t;

/*!*******************************************************************
 * @struct fbe_cmi_io_context_t
 *********************************************************************
 * @brief
 *  This structure contains context for queueing.
 *
 *********************************************************************/
typedef struct fbe_cmi_io_context_s
{
    fbe_queue_element_t				 queue_element;/*always first*/
    fbe_cmi_io_context_info_t        context_info;
    fbe_spinlock_t	                 context_lock;
} fbe_cmi_io_context_t;

/*!*******************************************************************
 * @struct fbe_cmi_io_msg_t
 *********************************************************************
 * @brief
 *  This structure contains context for message (not packet).
 *
 *********************************************************************/
typedef struct fbe_cmi_io_abort_s
{
    fbe_cmi_io_float_data_t          float_data;
    fbe_atomic_t                     in_use;
} fbe_cmi_io_msg_t;

/*!*******************************************************************
 * @struct fbe_cmi_io_statistics_t
 *********************************************************************
 * @brief
 *  This structure contains IO statistics for each conduit.
 *
 *********************************************************************/
typedef struct fbe_cmi_io_statistics_s
{
    fbe_atomic_t sent_ios;
    fbe_atomic_t sent_bytes;
    fbe_atomic_t received_ios;
    fbe_atomic_t received_bytes;
    fbe_atomic_t sent_errors;
    fbe_atomic_t received_errors;
} fbe_cmi_io_statistics_t;

typedef struct fbe_cmi_io_data_pool_s
{
    fbe_u64_t pool_start_address;
    fbe_u32_t pool_slots;
    fbe_u32_t data_length;
} fbe_cmi_io_data_pool_t;

#endif/*FBE_CMI_IO_H*/
