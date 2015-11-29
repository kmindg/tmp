#ifndef CPD_INTERFACE_EXTENSION_H
#define CPD_INTERFACE_EXTENSION_H

#ifdef ALAMOSA_WINDOWS_ENV

#include "storport.h"         /* For SCSI_REQUEST_BLOCK */

typedef SCSI_REQUEST_BLOCK	            CPD_SCSI_REQ_BLK;
typedef PSCSI_REQUEST_BLOCK	            PCPD_SCSI_REQ_BLK; 

#define cpd_os_io_get_next_srb(os_io_handle) \
			(((CPD_SCSI_REQ_BLK *)(os_io_handle))->NextSrb)
#define cpd_os_io_set_next_srb(os_io_handle, new_next_srb) \
			((((CPD_SCSI_REQ_BLK *)(os_io_handle))->NextSrb) = (new_next_srb))

#define cpd_os_io_get_path_id(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->PathId)
#define cpd_os_io_set_path_id(os_io_handle, pathId) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->PathId) = (pathId))            
#define cpd_os_io_get_target_id(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->TargetId)
#define cpd_os_io_set_target_id(os_io_handle, targetId) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->TargetId) = (targetId))           
#define os_io_get_lun(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->Lun)
#define cpd_os_io_set_lun(os_io_handle, new_lun) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->Lun) = (new_lun))            
#define cpd_os_io_get_command(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->Function)
#define cpd_os_io_set_command(os_io_handle, command) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->Function) = (command))            
#define cpd_os_io_get_scsi_status(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->ScsiStatus)
#define cpd_os_io_set_scsi_status(os_io_handle,status) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->ScsiStatus) = \
                (status)
#define cpd_os_io_get_io_ext(os_io_handle) \
            ((CPD_OSW_SRB_EXT *) \
                (((CPD_SCSI_REQ_BLK *)(os_io_handle))->SrbExtension))
#define cpd_os_io_set_io_ext(os_io_handle, srb_ext) \
                (((CPD_SCSI_REQ_BLK *)(os_io_handle))->SrbExtension) = \
                (srb_ext)

#define cpd_os_io_get_data_buf(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->DataBuffer)
#define cpd_os_io_set_data_buf(os_io_handle,buf_ptr) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->DataBuffer) = (buf_ptr))
#define cpd_os_io_get_xfer_len(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->DataTransferLength)
#define cpd_os_io_set_xfer_len(os_io_handle,new_xfer_len) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->DataTransferLength) = \
                (new_xfer_len))
#define cpd_os_io_get_cdb(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->Cdb)
#define cpd_os_io_set_cdb(os_io_handle, index, value) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->Cdb[index]) = (value))            
#define cpd_os_io_get_cdb_length(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->CdbLength)
#define cpd_os_io_set_cdb_length(os_io_handle, length) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->CdbLength) = (length)) 
#define cpd_os_io_get_length(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->Length)
#define cpd_os_io_set_length(os_io_handle, new_length) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->Length) = (new_length))             
#define cpd_os_io_get_queue_action(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->QueueAction)
#define cpd_os_io_set_queue_action(os_io_handle, new_queue_action) \
	        ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->QueueAction) = (new_queue_action))

#define cpd_os_io_get_sense_buf(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->SenseInfoBuffer)
#define cpd_os_io_set_sense_buf(os_io_handle, sense_buffer) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->SenseInfoBuffer) = \
            (sense_buffer))            
#define cpd_os_io_get_sense_len(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->SenseInfoBufferLength)
#define cpd_os_io_set_sense_len(os_io_handle,new_len) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->SenseInfoBufferLength) = \
                (new_len))

#define cpd_os_io_get_status(os_io_handle) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->SrbStatus)
#define cpd_os_io_set_status(os_io_handle, status) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->SrbStatus = (status))
#define cpd_os_io_get_native_time_out(os_io_handle) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->TimeOutValue)
#define cpd_os_io_set_native_time_out(os_io_handle, timeout) \
    ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->TimeOutValue) = (timeout))    

#define cpd_os_io_get_flags(os_io_handle) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->SrbFlags)
#define cpd_os_io_set_flags(os_io_handle, flag) \
    ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->SrbFlags) = flag)
#define cpd_os_io_mask_flags(os_io_handle, flag) \
    ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->SrbFlags) |= flag)    
#define cpd_os_io_get_original_request(os_io_handle) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->OriginalRequest)
#define cpd_os_io_set_original_request(os_io_handle, orig_req) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->OriginalRequest = (orig_req))    
#define cpd_os_io_get_queue_sort_key(os_io_handle) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->QueueSortKey)
#define cpd_os_io_set_queue_sort_key(os_io_handle, key) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->QueueSortKey = (key))    
#define cpd_os_io_get_queue_tag(os_io_handle) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->QueueTag)
#define cpd_os_io_set_queue_tag(os_io_handle, new_queue_tag) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->QueueTag = (new_queue_tag))    


#else

typedef struct CPD_SCSI_REQ_BLK_TAG {
	USHORT length;
	UCHAR function;
	UCHAR srb_status;
	UCHAR scsi_status;
	UCHAR path_id;
	UCHAR target_id;
	UCHAR lun;
	UCHAR queue_tag;
	UCHAR queue_action;
	UCHAR cdb_length;
	UCHAR sense_info_buffer_length;
	ULONG srb_flags;
	ULONG data_transfer_length;
	ULONG time_out_value;
	PVOID data_buffer;
	PVOID sense_info_buffer;
	struct CPD_SCSI_REQ_BLK_TAG *next_srb;
	PVOID original_request;
	PVOID srb_extension;
	CSX_ANONYMOUS_UNION union {
		ULONG internal_status;
		ULONG queue_sort_key;
	};
	UCHAR cdb[16];
} CPD_SCSI_REQ_BLK,
*PCPD_SCSI_REQ_BLK;	

#define cpd_os_io_get_next_srb(os_io_handle) \
			(((CPD_SCSI_REQ_BLK *)(os_io_handle))->next_srb)
#define cpd_os_io_set_next_srb(os_io_handle, new_next_srb) \
			((((CPD_SCSI_REQ_BLK *)(os_io_handle))->next_srb) = (new_next_srb))

#define cpd_os_io_get_path_id(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->path_id)
#define cpd_os_io_set_path_id(os_io_handle, pathId) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->path_id) = (pathId))             
#define cpd_os_io_get_target_id(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->target_id)
#define cpd_os_io_set_target_id(os_io_handle, targetId) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->target_id) = (targetId))            
#define os_io_get_lun(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->lun) 
#define cpd_os_io_set_lun(os_io_handle, new_lun) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->lun) = (new_lun))             
#define cpd_os_io_get_command(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->function)
#define cpd_os_io_set_command(os_io_handle, command) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->function) = (command))            
#define cpd_os_io_get_scsi_status(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->scsi_status)
#define cpd_os_io_set_scsi_status(os_io_handle,status) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->scsi_status) = \
                (status)
#define cpd_os_io_get_io_ext(os_io_handle) \
            ((CPD_OSW_SRB_EXT *) \
                (((CPD_SCSI_REQ_BLK *)(os_io_handle))->srb_extension))
#define cpd_os_io_set_io_ext(os_io_handle, srb_ext) \
                (((CPD_SCSI_REQ_BLK *)(os_io_handle))->srb_extension) = \
                (srb_ext)                

#define cpd_os_io_get_data_buf(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->data_buffer)
#define cpd_os_io_set_data_buf(os_io_handle,buf_ptr) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->data_buffer) = (buf_ptr))
#define cpd_os_io_get_xfer_len(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->data_transfer_length)
#define cpd_os_io_set_xfer_len(os_io_handle,new_xfer_len) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->data_transfer_length) = \
                (new_xfer_len))
#define cpd_os_io_get_cdb(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->cdb)
#define cpd_os_io_set_cdb(os_io_handle, index, value) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->cdb[index]) = (value))                
#define cpd_os_io_get_cdb_length(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->cdb_length)
#define cpd_os_io_set_cdb_length(os_io_handle, length) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->cdb_length) = (length))   
#define cpd_os_io_get_length(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->length)
#define cpd_os_io_set_length(os_io_handle, new_length) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->length) = (new_length))            
#define cpd_os_io_get_queue_action(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->queue_action)
#define cpd_os_io_set_queue_action(os_io_handle, new_queue_action) \
	        ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->queue_action) = (new_queue_action))

#define cpd_os_io_get_sense_buf(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->sense_info_buffer)
#define cpd_os_io_set_sense_buf(os_io_handle, sense_buffer) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->sense_info_buffer) = \
            (sense_buffer))              
#define cpd_os_io_get_sense_len(os_io_handle) \
            (((CPD_SCSI_REQ_BLK *)(os_io_handle))->sense_info_buffer_length)
#define cpd_os_io_set_sense_len(os_io_handle,new_len) \
            ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->sense_info_buffer_length) = \
                (new_len))             

#define cpd_os_io_get_status(os_io_handle) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->srb_status)
#define cpd_os_io_set_status(os_io_handle, status) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->srb_status = (status))
#define cpd_os_io_get_native_time_out(os_io_handle) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->time_out_value)  
#define cpd_os_io_set_native_time_out(os_io_handle, timeout) \
    ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->time_out_value) = (timeout))     

#define cpd_os_io_get_flags(os_io_handle) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->srb_flags)
#define cpd_os_io_set_flags(os_io_handle, flag) \
    ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->srb_flags) = flag)
#define cpd_os_io_mask_flags(os_io_handle, flag) \
    ((((CPD_SCSI_REQ_BLK *)(os_io_handle))->srb_flags) |= flag)     
#define cpd_os_io_get_original_request(os_io_handle) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->original_request)
#define cpd_os_io_set_original_request(os_io_handle, orig_req) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->original_request = (orig_req))     
#define cpd_os_io_get_queue_sort_key(os_io_handle) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->queue_sort_key)
#define cpd_os_io_set_queue_sort_key(os_io_handle, key) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->queue_sort_key = (key))    
#define cpd_os_io_get_queue_tag(os_io_handle) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->queue_tag)
#define cpd_os_io_set_queue_tag(os_io_handle, new_queue_tag) \
    (((CPD_SCSI_REQ_BLK *)(os_io_handle))->queue_tag = (new_queue_tag))    

#endif /* ALAMOSA_WINDOWS_ENV */
#endif /* CPD_INTERFACE_EXTENSION_H */
