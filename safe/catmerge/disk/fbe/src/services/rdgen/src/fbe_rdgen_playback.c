/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_playback.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions of the rdgen I/O Sequence Playback functionality.
 *
 * @version
 *   7/29/2013:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_rdgen.h"
#include "fbe_rdgen_private_inlines.h"
#include "fbe/fbe_file.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/
/*!**************************************************************
 * fbe_rdgen_open_file()
 ****************************************************************
 * @brief
 *  Open a file with a given path.
 *
 * @param file_p - Full path to the file.
 *
 * @return fbe_file_handle_t
 *
 * @author
 *  9/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_file_handle_t fbe_rdgen_open_file(fbe_char_t *file_p)
{
    fbe_file_handle_t file_handle = FBE_FILE_INVALID_HANDLE;
    // Open the file 
    file_handle = fbe_file_open(file_p, FBE_FILE_RDONLY, 0/* protection mode, unused */, NULL);
    return file_handle;
}
/******************************************
 * end fbe_rdgen_open_file()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_read_file()
 ****************************************************************
 * @brief
 *  Read a certain number of bytes from this file.
 *
 * @param file_handle
 * @param file_p
 * @param bytes_to_read
 * @param bytes_read_p
 * @param buffer_p
 * 
 * @return fbe_status_t     
 *
 * @author
 *  9/26/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_read_file(fbe_file_handle_t file_handle, fbe_char_t *file_p,
                                 fbe_u32_t bytes_to_read,
                                 fbe_u32_t *bytes_read_p, void *buffer_p)
{
    fbe_u32_t   bytes_read = 0;
    fbe_status_t status;

    if(file_handle != FBE_FILE_INVALID_HANDLE)
    {
        // Read the file
        bytes_read = fbe_file_read(file_handle, buffer_p, bytes_to_read, NULL);
        
        if(bytes_read == FBE_FILE_ERROR)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s, read file %s failed.\n", __FUNCTION__, file_p); 
            *bytes_read_p = 0;
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            *bytes_read_p = bytes_read;
            status = FBE_STATUS_OK;
        }
    }
    else
    {        
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, open file %s failed.\n", __FUNCTION__, file_p); 
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/******************************************
 * end fbe_rdgen_read_file()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_playback_get_header()
 ****************************************************************
 * @brief
 *  Read in the header from the disk.
 *   
 * @param targets_p
 * @param filter_p
 * @param file_p
 * 
 * @return fbe_status_t    
 *
 * @author
 *  9/26/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_playback_get_header(fbe_u32_t *targets_p,
                                           fbe_rdgen_root_file_filter_t **filter_p,
                                           fbe_char_t *file_p)
{
    fbe_rdgen_root_file_filter_t *local_filter_p = NULL;
    fbe_u32_t   status = FBE_STATUS_OK;
    fbe_u32_t filter_index;
    fbe_file_handle_t file_handle;
    fbe_u32_t bytes;
    fbe_rdgen_root_file_header_t header;

    file_handle = fbe_rdgen_open_file(file_p);

    if(file_handle == FBE_FILE_INVALID_HANDLE) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, Open file failed for %s.\n", __FUNCTION__, file_p); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_rdgen_read_file(file_handle, file_p, sizeof(fbe_rdgen_root_file_header_t), &bytes, &header);

    if (status != FBE_STATUS_OK || bytes != sizeof(fbe_rdgen_root_file_header_t)){
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, read file for header %s failed.\n", __FUNCTION__, file_p); 
        fbe_file_close(file_handle);
        return status;
    }
    
    *targets_p = header.num_targets;
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                            "Success reading file %s objects: %d\n", file_p, header.num_targets);
    if (filter_p != NULL)
    {
        local_filter_p = fbe_memory_native_allocate(sizeof(fbe_rdgen_root_file_filter_t) * header.num_targets);
        *filter_p = local_filter_p;
        for (filter_index = 0; filter_index < header.num_targets; filter_index++)
        {
            status = fbe_rdgen_read_file(file_handle, file_p, sizeof(fbe_rdgen_root_file_filter_t), &bytes, local_filter_p);
            if (status != FBE_STATUS_OK || bytes != sizeof(fbe_rdgen_root_file_filter_t))
            {
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s, read file status: %d bytes: %d for filter %d %s failed.\n", 
                                        __FUNCTION__, status, bytes, filter_index, file_p); 
                fbe_file_close(file_handle);
                status = FBE_STATUS_GENERIC_FAILURE;
                /* Release memory since the call failed.  The caller will not expect memory returned if the read failed.
                 */
                fbe_memory_native_release(*filter_p);
                *filter_p = NULL;
                return status;
            }

            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Found filter: filter: 0x%x obj: 0x%x pkg: 0x%x th: %d dev_name: %s\n", 
                                    local_filter_p->filter_type, local_filter_p->object_id, local_filter_p->package_id, 
                                    local_filter_p->threads,
                                    (local_filter_p->device_name[0] == 0) ? " " : local_filter_p->device_name);
            local_filter_p++;
        }
    }
    fbe_file_close(file_handle);

    return status;
}
/******************************************
 * end fbe_rdgen_playback_get_header()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_request_inc_all_pass_counts()
 ****************************************************************
 * @brief
 *  Increment the pass counts for all ts on a given request.
 *
 * @param request_p -               
 *
 * @return None   
 *
 * @author
 *  9/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_request_inc_all_pass_counts(fbe_rdgen_request_t *request_p)
{
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_rdgen_ts_t *current_ts_p = NULL;
    /* First mark everything
     */
    queue_element_p = fbe_queue_front(&request_p->active_ts_head);
    while (queue_element_p != NULL)
    {
        current_ts_p = fbe_rdgen_ts_request_queue_element_to_ts_ptr(queue_element_p);
        /* Hit the end, starting a new pass for everyone.
         */
        current_ts_p->pass_count++;
        queue_element_p = fbe_queue_next(&request_p->active_ts_head, queue_element_p);
    }    /* while the queue element is not null. */
}
/******************************************
 * end fbe_rdgen_request_inc_all_pass_counts()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_playback_get_next_io()
 ****************************************************************
 * @brief
 *  Determine the next I/O we should issue with this ts.
 *
 * @param ts_p -               
 *
 * @return None. 
 *
 * @author
 *  9/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_ts_playback_get_next_io(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_object_t *object_p = ts_p->object_p;
    fbe_u32_t cpu_count;
    /* Set the new values for this operation.
     */
    fbe_rdgen_ts_set_corrupt_bitmask(ts_p, (UNSIGNED_64_MINUS_1));
    fbe_rdgen_ts_set_blocks(ts_p, object_p->playback_records_p->record_data[object_p->playback_record_index].blocks);
    fbe_rdgen_ts_set_lba(ts_p, object_p->playback_records_p->record_data[object_p->playback_record_index].lba);
    fbe_rdgen_ts_set_io_interface(ts_p, object_p->playback_records_p->record_data[object_p->playback_record_index].io_interface);

    if (ts_p->lba + ts_p->blocks >= object_p->capacity){
        fbe_lba_t end_lba = ts_p->lba + ts_p->blocks;
        if ((end_lba % object_p->capacity) < ts_p->blocks){
            fbe_rdgen_ts_set_lba(ts_p, 0);
        }
        else {
            fbe_rdgen_ts_set_lba(ts_p, (end_lba % object_p->capacity) - ts_p->blocks);
        }
    }
    fbe_rdgen_ts_set_opcode(ts_p, object_p->playback_records_p->record_data[object_p->playback_record_index].opcode);
    fbe_rdgen_ts_set_core(ts_p, object_p->playback_records_p->record_data[object_p->playback_record_index].cpu);
    fbe_rdgen_ts_set_priority(ts_p, object_p->playback_records_p->record_data[object_p->playback_record_index].priority);

    if ((ts_p->priority > FBE_PACKET_PRIORITY_INVALID) &&
        (ts_p->priority < FBE_PACKET_PRIORITY_LAST)){
        fbe_packet_t *packet_p = NULL;
        fbe_packet_t *write_packet_p = NULL;
        packet_p = fbe_rdgen_ts_get_packet(ts_p);
        write_packet_p = fbe_rdgen_ts_get_write_packet(ts_p);

        fbe_transport_set_packet_priority(packet_p, ts_p->priority);
        fbe_transport_set_packet_priority(write_packet_p, ts_p->priority);
    }

    cpu_count = fbe_get_cpu_count();

    if (ts_p->core_num >= cpu_count){
        /* If the core number looks incorrect, just fix it to match the possible 
         * number of cores on this system. 
         */
        fbe_rdgen_ts_set_core(ts_p, (ts_p->core_num % cpu_count));
    }
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                            "gen playback: lba: 0x%llx bl: 0x%llx op: 0x%x core: 0x%x th: %d cth: %d p: %d\n",
                            ts_p->lba, ts_p->blocks, ts_p->operation, ts_p->core_num, 
                            object_p->playback_records_p->record_data[object_p->playback_record_index].threads,
                            fbe_rdgen_object_get_current_threads(object_p), 
                            ts_p->priority);
    fbe_rdgen_object_inc_playback_index(object_p);
}
/******************************************
 * end fbe_rdgen_ts_playback_get_next_io()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_playback_generate_wait()
 ****************************************************************
 * @brief
 *  Generate the next I/O and determine if we need to
 *  wait either for time to elapse (if we need to delay this thread),
 *  or if we need to wait for the next set of pages to be read
 *  off of the disk.
 *
 * @param ts_p -               
 *
 * @return fbe_bool_t FBE_TRUE if we need to wait
 *                    FBE_FALSE if we have the next I/O and
 *                    need to continue executing.
 *
 * @author
 *  9/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_rdgen_playback_generate_wait(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_object_t *object_p = ts_p->object_p;
    fbe_bool_t b_signal = FBE_FALSE;
    fbe_u32_t threads;
    fbe_rdgen_ts_clear_flag(ts_p, FBE_RDGEN_TS_FLAGS_LOCK_OBTAINED);
    fbe_rdgen_object_lock(ts_p->object_p);

    if ((object_p->playback_records_p != NULL) &&
        (object_p->playback_record_index >= object_p->playback_records_p->num_valid_records) &&
        (object_p->playback_total_records <= FBE_RDGEN_PLAYBACK_RECORDS_PER_BUFFER)){

        /* If we only needed one buffer. Just set the index back to zero.
         */
        object_p->playback_record_index = 0;
        object_p->playback_num_chunks_processed++;
    }
    else if ((object_p->playback_records_p != NULL) &&
             (object_p->playback_record_index >= object_p->playback_records_p->num_valid_records) &&
             (object_p->playback_total_chunks > FBE_RDGEN_PLAYBACK_CHUNK_COUNT)){

        /* We needed more than can fit in memory, so just free the current one and try to get another.
         */
        fbe_rdgen_object_dequeue_record(object_p, object_p->playback_records_p);
        fbe_rdgen_object_enqueue_free_record(object_p, object_p->playback_records_p);
        object_p->playback_num_chunks_processed++;
        object_p->playback_records_p = NULL;
        /* Signal the thread to read the next piece.
         */
        b_signal = FBE_TRUE;
    }
    else if ((object_p->playback_records_p != NULL) &&
             (object_p->playback_record_index >= object_p->playback_records_p->num_valid_records) &&
             (object_p->playback_total_chunks <= FBE_RDGEN_PLAYBACK_CHUNK_COUNT)){

        /* If they all fit in memory, then just re-use them by pulling from head and adding to tail.
         */
        fbe_rdgen_object_dequeue_record(object_p, object_p->playback_records_p);
        fbe_rdgen_object_enqueue_valid_record(object_p, object_p->playback_records_p);
        object_p->playback_records_p = NULL;
        object_p->playback_num_chunks_processed++;
    }

    if (object_p->playback_records_p == NULL){

        if ((object_p->playback_num_chunks_processed > 0) &&
            ((object_p->playback_num_chunks_processed % object_p->playback_total_chunks) == 0)){

            fbe_rdgen_object_unlock(object_p);

            /* Inc pass counts for all threads once we finish one pass.
             */
            fbe_rdgen_request_lock(ts_p->request_p);
            fbe_rdgen_request_inc_all_pass_counts(ts_p->request_p);
            fbe_rdgen_request_unlock(ts_p->request_p);

            fbe_rdgen_object_lock(object_p);
        }
        if (fbe_queue_is_empty(&object_p->valid_record_queue)){

            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, object_p->object_id,
                                    "gen playback: (%d) wait for read of records.\n", ts_p->thread_num);
            fbe_rdgen_object_dec_current_threads(object_p);
            fbe_rdgen_ts_set_flag(ts_p, FBE_RDGEN_TS_FLAGS_WAIT_OBJECT);
            fbe_rdgen_object_enqueue_to_thread(object_p);
            fbe_rdgen_object_unlock(object_p);
            return FBE_TRUE;
        }
        else {
            object_p->playback_records_p = (fbe_rdgen_object_record_t*)fbe_queue_front(&object_p->valid_record_queue);
            object_p->playback_record_index = 0;
        }
    }

    threads = object_p->playback_records_p->record_data[object_p->playback_record_index].threads;

    if (threads != fbe_rdgen_object_get_max_threads(object_p)){
        /* Our max number of threads changed either up or down. 
         * Set it here, and then react to the decrease or increase below. 
         */
        fbe_rdgen_object_set_max_threads(object_p, threads);
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, object_p->object_id,
                                "gen playback: (%d) max threads: %d current threads: %d\n",
                                ts_p->thread_num, fbe_rdgen_object_get_max_threads(object_p),
                                fbe_rdgen_object_get_current_threads(object_p));
    }
    if (fbe_rdgen_object_get_current_threads(object_p) > fbe_rdgen_object_get_max_threads(object_p)){
        /* We need to idle this thread, so back off.
         */
        fbe_rdgen_object_dec_current_threads(object_p);
        fbe_rdgen_ts_set_flag(ts_p, FBE_RDGEN_TS_FLAGS_WAIT_OBJECT);

        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, object_p->object_id,
                                "gen playback: (%d) now idle max threads: %d current: %d\n",
                                ts_p->thread_num, fbe_rdgen_object_get_max_threads(object_p),
                                fbe_rdgen_object_get_current_threads(object_p));

        if (fbe_rdgen_object_get_current_threads(object_p) == 0){

            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, object_p->object_id,
                                    "gen playback: going idle for %d msec\n",
                                    fbe_rdgen_object_get_restart_msec(object_p));
            /* Wakeup the object thread to kick off more ts.
             */
            fbe_rdgen_object_set_restart_time(object_p);
            fbe_rdgen_object_enqueue_to_thread(object_p);
        }
        fbe_rdgen_object_unlock(object_p);
        return FBE_TRUE;
    }
    else if (fbe_rdgen_object_get_current_threads(object_p) < fbe_rdgen_object_get_max_threads(object_p)){
        /* Wakeup the object thread to kick off more ts.
         */
        fbe_rdgen_object_enqueue_to_thread(object_p);
    }

    /* Fill in ts with all information on next I/O to execute.
     */
    fbe_rdgen_ts_playback_get_next_io(ts_p);

    if (b_signal){
        /* Kick off the object thread to read in the next piece of data.
         */
        fbe_rdgen_object_enqueue_to_thread(object_p);
    }
    fbe_rdgen_object_unlock(object_p);
    /* Kick the thread to read in the next chunk.
     */
    return FBE_FALSE;
}
/******************************************
 * end fbe_rdgen_playback_generate_wait()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_playback_read_object_header()
 ****************************************************************
 * @brief
 *  Read in one object's header from its file.
 *
 * @param object_p - 
 *
 * @return fbe_status_t
 *
 * @author
 *  9/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_playback_read_object_header(fbe_rdgen_object_t *object_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_file_handle_t file_handle;
    fbe_u32_t bytes;
    fbe_rdgen_object_file_header_t object_header;
    fbe_char_t *file_p = &object_p->file_name[0];

    file_handle = fbe_rdgen_open_file(file_p);

    if(file_handle == FBE_FILE_INVALID_HANDLE) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, Open file failed for %s.\n", __FUNCTION__, file_p); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The first time we read the file, populate the number of records we found.
     */
    if (object_p->playback_total_records == 0){
        status = fbe_rdgen_read_file(file_handle, file_p, sizeof(fbe_rdgen_object_file_header_t), &bytes, &object_header);
    
        if (status != FBE_STATUS_OK || bytes != sizeof(fbe_rdgen_object_file_header_t)){
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s, read file for header %s failed.\n", __FUNCTION__, file_p); 
            fbe_file_close(file_handle);
            return status;
        }
        object_p->playback_total_records = object_header.records;
        object_p->playback_total_chunks = object_header.records / FBE_RDGEN_PLAYBACK_RECORDS_PER_BUFFER;
        if (object_header.records % FBE_RDGEN_PLAYBACK_RECORDS_PER_BUFFER){
            object_p->playback_total_chunks++;
        }
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "Found object header for file: %s num_records: %d num_chunks %d\n",
                                file_p, object_p->playback_total_records, object_p->playback_total_chunks);

        if (object_header.records == 0){
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "read object header %s unexpected record count %d\n",
                                    file_p, object_p->playback_total_records);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return status;
}
/******************************************
 * end fbe_rdgen_playback_read_object_header()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_playback_read_records()
 ****************************************************************
 * @brief
 *  Read in some number of records that were already allocated.
 *  The records are read in for a specific object and
 *  stored in the items on the object's record queue.
 *
 * @param object_p
 *
 * @return fbe_status_t
 *
 * @author
 *  9/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_playback_read_records(fbe_rdgen_object_t *object_p)
{
    fbe_status_t status;
    fbe_u32_t read_offset = sizeof(fbe_rdgen_object_file_header_t);
    fbe_file_handle_t file_handle;
    fbe_u32_t bytes;
    fbe_rdgen_object_file_header_t object_header;
    fbe_u64_t seek_status;
    fbe_char_t *file_p = &object_p->file_name[0];
    fbe_u32_t num_records;
    fbe_u32_t remaining_records;
    fbe_rdgen_object_record_t *object_record_p = NULL;

    file_handle = fbe_rdgen_open_file(file_p);

    if(file_handle == FBE_FILE_INVALID_HANDLE) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, Open file failed for %s.\n", __FUNCTION__, file_p); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The first time we read the file, populate the number of records we found.
     */
    if (object_p->playback_total_records == 0){
        status = fbe_rdgen_read_file(file_handle, file_p, sizeof(fbe_rdgen_object_file_header_t), &bytes, &object_header);
    
        if (status != FBE_STATUS_OK || bytes != sizeof(fbe_rdgen_object_file_header_t)){
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s, read file for header %s failed.\n", __FUNCTION__, file_p); 
            fbe_file_close(file_handle);
            return status;
        }
        object_p->playback_total_records = object_header.records;
        object_p->playback_total_chunks = object_header.records / FBE_RDGEN_PLAYBACK_RECORDS_PER_BUFFER;
        if (object_header.records % FBE_RDGEN_PLAYBACK_RECORDS_PER_BUFFER){
            object_p->playback_total_chunks++;
        }
    }

    /* Get a buffer to use.
     */
    fbe_rdgen_object_lock(object_p);
    object_record_p = (fbe_rdgen_object_record_t*)fbe_queue_pop(&object_p->free_record_queue);
    fbe_rdgen_object_unlock(object_p);
    while (object_record_p != NULL) {
        
        /* If we wrap around again, start reading from the beginning.
         */
        if (object_p->playback_chunk_index >= object_p->playback_total_chunks){
            object_p->playback_chunk_index = 0;
        }
        read_offset += (object_p->playback_chunk_index * sizeof(fbe_rdgen_object_file_record_t) * FBE_RDGEN_PLAYBACK_RECORDS_PER_BUFFER);
    
        seek_status = fbe_file_lseek(file_handle, read_offset, 0);
        if (seek_status == FBE_FILE_ERROR){
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s, read file for header %s failed.\n", __FUNCTION__, file_p); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* Determine what is left to read in the file.
         */
        if (object_p->playback_total_records > FBE_RDGEN_PLAYBACK_RECORDS_PER_BUFFER){
            remaining_records = object_p->playback_total_records - (object_p->playback_chunk_index * FBE_RDGEN_PLAYBACK_RECORDS_PER_BUFFER);
        }
        else {
            remaining_records = object_p->playback_total_records;
        }
    
        /* Read either the full buffer or whatever is remaining to read.
         */
        num_records = FBE_MIN(FBE_RDGEN_PLAYBACK_RECORDS_PER_BUFFER, remaining_records);
        
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, object_p->object_id,
                                "obj thread: read %d records at chunk index: %d\n",
                                num_records, object_p->playback_chunk_index);
        status = fbe_rdgen_read_file(file_handle, file_p, sizeof(fbe_rdgen_object_file_record_t) * num_records, 
                                     &bytes, &object_record_p->record_data[0]);
        
        if (status != FBE_STATUS_OK || bytes != sizeof(fbe_rdgen_object_file_record_t) * num_records){
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s, read file for header %s failed.\n", __FUNCTION__, file_p); 
            fbe_file_close(file_handle);
            status = FBE_STATUS_FAILED;
            return status;
        }
        fbe_rdgen_object_lock(object_p);
        object_record_p->num_valid_records = num_records;
        fbe_rdgen_object_enqueue_valid_record(object_p, object_record_p);

        object_p->playback_chunk_index++;

        /* Get the next record to read.
         */
        object_record_p = (fbe_rdgen_object_record_t*)fbe_queue_pop(&object_p->free_record_queue);
        fbe_rdgen_object_unlock(object_p);
    }
    fbe_file_close(file_handle);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_playback_read_records()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_restart_object_waiters()
 ****************************************************************
 * @brief
 *  Kick off any ts structures that are marked waiting for
 *  this object.
 *
 * @param object_p - Current object.
 *
 * @return None
 *
 * @author
 *  9/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_restart_object_waiters(fbe_rdgen_object_t *object_p)
{
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_rdgen_ts_t *ts_p = NULL;
	fbe_queue_head_t tmp_queue;
    fbe_queue_init(&tmp_queue);

    fbe_rdgen_object_lock(object_p);

    /* Simply traverse the list of active structures. 
     * Stop if we see we generated enough, 
     * or if we need to kick off more structures then kick off any 
     * that are marked waiting. 
     */
    queue_element_p = fbe_queue_front(&object_p->active_ts_head);
    while (queue_element_p != NULL){
        ts_p = (fbe_rdgen_ts_t*)queue_element_p;
        if (fbe_rdgen_object_get_current_threads(object_p) >= fbe_rdgen_object_get_max_threads(object_p)){
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, object_p->object_id,
                                    "obj thread: stop dispatch (%d) max threads: %d current: %d\n",
                                    ts_p->thread_num, fbe_rdgen_object_get_max_threads(object_p),
                                    fbe_rdgen_object_get_current_threads(object_p));
            break;
        }
        if (fbe_rdgen_ts_is_flag_set(ts_p, FBE_RDGEN_TS_FLAGS_WAIT_OBJECT)){
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, object_p->object_id,
                                    "obj thread: restart thread (%d) max threads: %d current: %d\n",
                                    ts_p->thread_num, fbe_rdgen_object_get_max_threads(object_p),
                                    fbe_rdgen_object_get_current_threads(object_p));
            fbe_rdgen_object_inc_current_threads(object_p);
            fbe_queue_push(&tmp_queue, &ts_p->thread_queue_element);
            fbe_rdgen_ts_clear_flag(ts_p, FBE_RDGEN_TS_FLAGS_WAIT_OBJECT);
        }
        queue_element_p = fbe_queue_next(&object_p->active_ts_head, queue_element_p);
    }
    fbe_rdgen_object_unlock(object_p);

    /* just dispatch the structures that we enqueued to the local list.
     */
    while(queue_element_p = fbe_queue_pop(&tmp_queue)){
        ts_p = fbe_rdgen_ts_thread_queue_element_to_ts_ptr(queue_element_p);
        fbe_rdgen_ts_enqueue_to_thread(ts_p);
    }
}
/******************************************
 * end fbe_rdgen_restart_object_waiters()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_set_object_waiters_error()
 ****************************************************************
 * @brief
 *  Mark all ts structures that were waiting as complete in error.
 *
 * @param object_p - Current object.
 *
 * @return None
 *
 * @author
 *  9/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_set_object_waiters_error(fbe_rdgen_object_t *object_p)
{
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_rdgen_ts_t *ts_p = NULL;

    fbe_rdgen_object_lock(object_p);

    queue_element_p = fbe_queue_front(&object_p->active_ts_head);
    while (queue_element_p != NULL){
        ts_p = (fbe_rdgen_ts_t*)queue_element_p;
        if (fbe_rdgen_ts_is_flag_set(ts_p, FBE_RDGEN_TS_FLAGS_WAIT_OBJECT)){
            /* Mark the request as complete so we will stop. 
             * Mark the ts with error to return to caller. 
             */
            fbe_rdgen_request_set_complete(ts_p->request_p);
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        }
        queue_element_p = fbe_queue_next(&object_p->active_ts_head, queue_element_p);
    }
    fbe_rdgen_object_unlock(object_p);
}
/******************************************
 * end fbe_rdgen_set_object_waiters_error()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_playback_handle_object()
 ****************************************************************
 * @brief
 *  For this object read in more records if the free record
 *  queue has more items we can fill up.
 *
 * @param object_p - Current object.
 *
 * @return None
 *
 * @author
 *  9/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_playback_handle_object(fbe_rdgen_object_t *object_p)
{
    fbe_status_t status;

    if (object_p->playback_records_memory_p == NULL){
        fbe_u32_t index;
        fbe_u32_t chunk_count;
        fbe_rdgen_object_record_t *current_record_p = NULL;

        status = fbe_rdgen_playback_read_object_header(object_p);
        if (status != FBE_STATUS_OK){

            /* We got an error, so set the error and restart the TS below.
             */
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: unable to read object header for object_id: 0x%x\n", __FUNCTION__, object_p->object_id);
            fbe_rdgen_set_object_waiters_error(object_p);
            fbe_rdgen_restart_object_waiters(object_p);
            return;
        }
        /* We only want to allocate either a default number of buffers (if the file is big) 
         * or all the buffers we need for this file (if the file fits in memory).
         */
        chunk_count = FBE_MIN(FBE_RDGEN_PLAYBACK_CHUNK_COUNT, object_p->playback_total_chunks);
        object_p->playback_records_memory_p = fbe_rdgen_allocate_memory(sizeof(fbe_rdgen_object_record_t) * chunk_count);
        if (object_p->playback_records_memory_p == NULL) {

            fbe_rdgen_object_unlock(object_p);
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: unable to allocate buffer for object_id: 0x%x\n", __FUNCTION__, object_p->object_id);
            fbe_rdgen_set_object_waiters_error(object_p);
            fbe_rdgen_restart_object_waiters(object_p);
            return;
        }
        current_record_p = object_p->playback_records_memory_p;
        for ( index = 0; index < chunk_count; index++){
            fbe_rdgen_object_enqueue_free_record(object_p, current_record_p);
            current_record_p++;
        }
    }
    fbe_rdgen_object_lock(object_p);

    /* If we have completely run out of records or 
     * if we have free records to read, then read in some records now. 
     */
    if ((object_p->playback_records_p == NULL) ||
        !fbe_queue_is_empty(&object_p->free_record_queue)){
        /* Just read the entire area.
         */
        fbe_rdgen_object_unlock(object_p);
        status = fbe_rdgen_playback_read_records(object_p);
        if (status != FBE_STATUS_OK){
            
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Error reading records for object_id: 0x%x\n", __FUNCTION__, object_p->object_id);
            fbe_rdgen_set_object_waiters_error(object_p);
        }
        fbe_rdgen_restart_object_waiters(object_p);
    }
    else { /* Just read the next set of records*/

        fbe_rdgen_object_unlock(object_p);
        fbe_rdgen_restart_object_waiters(object_p);
    }
}
/******************************************
 * end fbe_rdgen_playback_handle_object()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_playback_handle_objects()
 ****************************************************************
 * @brief
 *  Loop over any objects that might need to be serviced and
 *  perform operations like:
 *  - Reading in more records from the disk.
 *  - And/or restart these objects as needed.
 *
 * @param thread_p
 * @param min_msecs_to_wait_p
 *
 * @return None.
 *
 * @author
 *  9/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_playback_handle_objects(fbe_rdgen_thread_t *thread_p,
                                       fbe_time_t *min_msecs_to_wait_p)
{
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_queue_element_t *next_queue_element_p = NULL;
    fbe_rdgen_object_t *object_p = NULL;
	fbe_queue_head_t tmp_queue;
    fbe_time_t current_time;
    fbe_time_t min_msecs_to_wait = FBE_U64_MAX;
    fbe_queue_init(&tmp_queue);

    /* Prevent things from changing while we are pulling
     * all the items off the queue.
     */
    fbe_rdgen_thread_lock(thread_p);

    /* Clear event under lock.  This allows us to coalesce the wait on events.
     */
    fbe_rendezvous_event_clear(&thread_p->event);

    /* Populate temp queue from ts queue.  Pull off everything that is ready to run.
     */
    current_time = fbe_get_time();
    queue_element_p = fbe_queue_front(&thread_p->ts_queue_head);
    while (queue_element_p) {
        next_queue_element_p = fbe_queue_next(&thread_p->ts_queue_head, queue_element_p);
        object_p = fbe_rdgen_object_thread_queue_element_to_ts_ptr(queue_element_p);

        /* Just leave it on the queue if it is not time to process it yet.
         */
        if ((object_p->time_to_restart != 0) &&
            (current_time < object_p->time_to_restart)){

            min_msecs_to_wait = FBE_MIN(min_msecs_to_wait, (object_p->time_to_restart - current_time));
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, object_p->object_id,
                                    "%s: wait for %llu more msec\n", __FUNCTION__, object_p->time_to_restart - current_time);
            queue_element_p = next_queue_element_p;
            continue;
        }

        /* We intentionally clear the flag under the thread lock.  The thread lock is held when we check this too.
         */
        fbe_rdgen_object_clear_flag(object_p, FBE_RDGEN_OBJECT_FLAGS_QUEUED);
        fbe_queue_remove(queue_element_p);
        fbe_queue_push(&tmp_queue, &object_p->process_queue_element);
        queue_element_p = next_queue_element_p;
    }
    /* Unlock to let other things end up on the queue.
     */
    fbe_rdgen_thread_unlock(thread_p);

    /* Process items from temp queue.
     */
    while(queue_element_p = fbe_queue_pop(&tmp_queue)) {
        object_p = fbe_rdgen_object_process_queue_element_to_ts_ptr(queue_element_p);

        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, object_p->object_id, "%s: dispatch\n", __FUNCTION__);
        if (object_p->time_to_restart != 0){
            /* Allow one thread to get started.
             * We need to manually skip over the record that indicates no threads in progress, 
             * since we do not issue a request for this record. 
             */
            fbe_rdgen_object_reset_restart_time(object_p);
            fbe_rdgen_object_set_max_threads(object_p, 1);
            object_p->playback_record_index++;
        }
        fbe_rdgen_object_lock(object_p);

        /* We let the thread dispose of the object for playbacks to coordinate when we dequeue it.
         * We do not want to destroy out of this context since it might be on the thread queue. 
         */
        if ((fbe_rdgen_object_get_num_threads(object_p) == 0) &&
            !fbe_rdgen_object_is_flag_set(object_p, FBE_RDGEN_OBJECT_FLAGS_QUEUED)){

            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, object_p->object_id, "%s: destroy object\n", __FUNCTION__);
            fbe_rdgen_object_unlock(object_p);
            fbe_rdgen_lock();
            fbe_rdgen_object_lock(object_p);
            fbe_rdgen_dequeue_object(object_p);
            fbe_rdgen_dec_num_objects();
            fbe_rdgen_object_unlock(object_p);
            fbe_rdgen_unlock();
            fbe_rdgen_object_destroy(object_p);
            continue;
        }
        fbe_rdgen_object_unlock(object_p);

        fbe_rdgen_playback_handle_object(object_p);

        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, object_p->object_id,
                                "%s: done with handle object\n", __FUNCTION__);
    }
    fbe_queue_destroy(&tmp_queue);
    *min_msecs_to_wait_p = min_msecs_to_wait;
}
/******************************************
 * end fbe_rdgen_playback_handle_objects()
 ******************************************/
/*************************
 * end file fbe_rdgen_playback.c
 *************************/


