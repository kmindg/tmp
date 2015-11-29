/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
#include "fbe_database_private.h"
#include "fbe/fbe_time.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_platform.h"

extern fbe_database_service_t fbe_database_service;

// Forward declarations 
void fbe_database_poll_set_pointer_to_next_record(database_poll_ring_buffer_t *database_ring_buffer_p); 
void fbe_database_poll_clear_record(fbe_database_poll_record_t *poll_record) ;
fbe_bool_t fbe_database_poll_is_complete(fbe_database_poll_record_t *poll_record); 
void fbe_database_poll_check_happy(database_poll_ring_buffer_t *database_ring_buffer_p); 
fbe_bool_t fbe_database_poll_is_poll_flag_set(fbe_database_poll_record_t *poll_record,
                                              fbe_database_poll_request_t poll_flag); 
void fbe_database_poll_set_poll_flag(fbe_database_poll_record_t *poll_record, 
                                     fbe_database_poll_request_t poll_flag); 

/*!**************************************************************
 * fbe_database_poll_initialize_poll_ring_buffer()
 ****************************************************************
 * @brief
 *  Initialize the ring buffer containing a reference to the list
 *  of full poll records  
 *
 * @param 
 *  database_ring_buffer_p - the ring buffer containing the records
 *
 * @return 
 *
 * @author
 *  2/13/2013 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_database_poll_initialize_poll_ring_buffer(database_poll_ring_buffer_t *database_ring_buffer_p) 
{
    database_ring_buffer_p->is_active = fbe_database_poll_is_recordkeeping_active();
    fbe_spinlock_init(&database_ring_buffer_p->lock);
    database_ring_buffer_p->threshold_count = 0;
    database_ring_buffer_p->start = 0;
    database_ring_buffer_p->end = 0;
    fbe_zero_memory(database_ring_buffer_p->records, FBE_DATABASE_MAX_POLL_RECORDS*sizeof(fbe_database_poll_record_t));
    // initialize the first record
    fbe_database_poll_clear_record(&database_ring_buffer_p->records[database_ring_buffer_p->end]);
}

/*!**************************************************************
 * fbe_database_poll_set_pointer_to_next_record()
 ****************************************************************
 * @brief
 *  Adjust the current record to the next record and reinitialize it
 *
 * @param 
 *  database_ring_buffer_p - the ring buffer containing the records
 *
 * @return 
 *
 * @author
 *  2/13/2013 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_database_poll_set_pointer_to_next_record(database_poll_ring_buffer_t *database_ring_buffer_p) 
{
    // move the end marker of the ring buffer
    database_ring_buffer_p->end = fbe_database_poll_get_next_index(database_ring_buffer_p->end);
    // move the start index to the next slot if the ring buffer is full
    if (database_ring_buffer_p->end == database_ring_buffer_p->start) {
        database_ring_buffer_p->start = fbe_database_poll_get_next_index(database_ring_buffer_p->start); 
    }

    // clear out the current record 
    fbe_database_poll_clear_record(&database_ring_buffer_p->records[database_ring_buffer_p->end]);
}

/*!**************************************************************
 * fbe_database_poll_get_next_index()
 ****************************************************************
 * @brief
 *  Given an index, find the next index in the ring buffer
 *
 * @param 
 *  index - the current index
 *
 * @return 
 *  next_index - the index after the current index
 *
 * @author
 *  2/14/2013 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_u32_t fbe_database_poll_get_next_index(fbe_u32_t index) {
    return (index + 1) % FBE_DATABASE_MAX_POLL_RECORDS;
}

/*!**************************************************************
 * fbe_database_poll_clear_record()
 ****************************************************************
 * @brief
 *  Reinitialize the record 
 *
 * @param 
 *  poll_record - the record to initialize
 *
 * @return 
 *
 * @author
 *  2/13/2013 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_database_poll_clear_record(fbe_database_poll_record_t *poll_record) 
{
    // clear out the current record 
    poll_record->poll_request_bitmap = 0;
    poll_record->time_of_poll = 0;
}

/*!**************************************************************
 * fbe_database_poll_record_poll()
 ****************************************************************
 * @brief
 *  If get_all_luns, get_all_raid_groups, or get_all_pvds was 
 *  issued, record it using the appropriate poll_flag. 
 *
 * @param 
 *  database_ring_buffer_p - record to updat te flag for
 *  poll_flag - the flag to set
 *
 * @return 
 *
 * @author
 *  2/13/2013 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_database_poll_record_poll(fbe_database_poll_request_t poll_flag) 
{
    database_poll_ring_buffer_t *database_ring_buffer_p = &fbe_database_service.poll_ring_buffer;
    fbe_database_poll_record_t  *poll_record            = NULL;

    if (!database_ring_buffer_p->is_active) {
        return;
    }

    fbe_spinlock_lock(&database_ring_buffer_p->lock);
    poll_record = &database_ring_buffer_p->records[database_ring_buffer_p->end];
    // 2 is the number of milliseconds allowed between the last poll to determine if it was a full poll
    if (fbe_get_elapsed_seconds(poll_record->time_of_poll) >= FBE_DATABASE_CONTIGUOUS_POLL_TIMER_SECONDS) {
        fbe_database_poll_clear_record(poll_record);
    }

    // set the poll flag
    fbe_database_poll_set_poll_flag(poll_record, poll_flag);
    poll_record->time_of_poll = fbe_get_time();

    if (fbe_database_poll_is_complete(poll_record)) {
        // we have a complete poll, now check if we need to panic
        fbe_database_poll_check_happy(database_ring_buffer_p);
        // add a new empty record to fill
        fbe_database_poll_set_pointer_to_next_record(database_ring_buffer_p);
    }

    fbe_spinlock_unlock(&database_ring_buffer_p->lock);

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s time of poll: %lld pollflags: %d\n", 
                   __FUNCTION__, poll_record->time_of_poll, poll_record->poll_request_bitmap);
}

/*!**************************************************************
 * fbe_database_poll_check_happy()
 ****************************************************************
 * @brief
 *  If the number of full polls (FBE_DATABASE_MAX_POLL_RECORDS) 
 *  was exceeded within some specified time (FBE_DATABASE_MAX_FULL_POLL_TIMER_SECONDS), 
 *  take the appropriate action
 *
 * @param 
 *  database_ring_buffer_p - the ring buffer containing the records
 *
 * @return 
 *
 * @author
 *  2/13/2013 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_database_poll_check_happy(database_poll_ring_buffer_t *database_ring_buffer_p) 
{
    fbe_database_poll_record_t *newest_record = &database_ring_buffer_p->records[database_ring_buffer_p->end];
    fbe_database_poll_record_t *oldest_record = &database_ring_buffer_p->records[database_ring_buffer_p->start];
    fbe_time_t delta_time = (newest_record->time_of_poll - oldest_record->time_of_poll) / (fbe_time_t)1000;
    // We complain if we have FBE_DATABASE_MAX_POLL_RECORDS polls in the span of N seconds
    if (delta_time < FBE_DATABASE_MAX_FULL_POLL_TIMER_SECONDS &&
        database_ring_buffer_p->start == fbe_database_poll_get_next_index(database_ring_buffer_p->end)) 
    {
        database_ring_buffer_p->threshold_count = database_ring_buffer_p->threshold_count+1;
        fbe_database_poll_complain();
    }
}


/*!**************************************************************
 * fbe_database_poll_is_complete()
 ****************************************************************
 * @brief
 *  Determine if all poll flags were set
 *
 * @param 
 *  poll_record - record to check the flags for
 *
 * @return 
 *  fbe_bool_t - true or false
 *
 * @author
 *  2/13/2013 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_bool_t fbe_database_poll_is_complete(fbe_database_poll_record_t *poll_record) 
{ 
    return (fbe_database_poll_is_poll_flag_set(poll_record, POLL_REQUEST_GET_ALL_LUNS) &&
            fbe_database_poll_is_poll_flag_set(poll_record, POLL_REQUEST_GET_ALL_RAID_GROUPS) &&
            fbe_database_poll_is_poll_flag_set(poll_record, POLL_REQUEST_GET_ALL_PVDS));
}

/*!**************************************************************
 * is_poll_flag_set()
 ****************************************************************
 * @brief
 *  Determine if a certain poll flag was set in this record
 *
 * @param 
 *  poll_record - pointer to the record 
 *  poll_flag - the flag to check
 *
 * @return 
 *  fbe_bool_t - true or false
 *
 * @author
 *  2/13/2013 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_bool_t fbe_database_poll_is_poll_flag_set(fbe_database_poll_record_t *poll_record,
                                              fbe_database_poll_request_t poll_flag) 
{
    return ((poll_record->poll_request_bitmap & poll_flag) == poll_flag);
}

/*!**************************************************************
 * fbe_database_poll_set_poll_flag()
 ****************************************************************
 * @brief
 *  Set the poll flag for the record
 *
 * @param 
 *  poll_record - record to updat te flag for
 *  poll_flag - the flag to set
 *
 * @return 
 *
 * @author
 *  2/13/2013 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_database_poll_set_poll_flag(fbe_database_poll_record_t *poll_record, 
                                     fbe_database_poll_request_t poll_flag) 
{
     poll_record->poll_request_bitmap |= poll_flag;
}
