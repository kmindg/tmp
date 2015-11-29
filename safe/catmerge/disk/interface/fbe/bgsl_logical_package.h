#ifndef BGSL_LOGICAL_PACKAGE_H
#define BGSL_LOGICAL_PACKAGE_H

#include "fbe/bgsl_types.h"
#include "fbe/bgsl_service.h"

bgsl_status_t 
logical_package_init(void);

typedef struct bgsl_logical_package_data_s
{
    void*          bgs_sched_block_p;                   // Pointer to the scheduler data block
    void*          bgs_event_handler_p;                 // Pointer to the event handler data block
    void*          bgs_master_packet_queue_p;           // Pointer to the queue containing all of the master packets.
    bgsl_spinlock_t bgs_master_packet_queue_spin_lock;   // spin-lock for performing operations on the master packet queue

} bgsl_logical_package_data_t;

#endif
