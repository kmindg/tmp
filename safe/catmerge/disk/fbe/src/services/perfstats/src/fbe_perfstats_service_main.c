
/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_perfstats_service_main.c
 ***************************************************************************
 *
 *  Description
 *      This file contains the implementation for the FBE Perfstats service code
 *      
 *    
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_service.h"
#include "fbe/fbe_perfstats_interface.h"
#include "fbe_perfstats_private.h"
#include "fbe_perfstats.h"
#ifdef C4_INTEGRATED
#include "stats_c_api.h"
#endif
 

/*local structures*/

/* Declare our service methods */
fbe_status_t fbe_perfstats_control_entry(fbe_packet_t * packet); 
fbe_service_methods_t fbe_perfstats_service_methods = {FBE_SERVICE_ID_PERFSTATS, fbe_perfstats_control_entry};

/* Forward declarations */
static fbe_status_t fbe_perfstats_get_mapped_stats_container(fbe_packet_t *packet);
static fbe_status_t fbe_perfstats_zero_stats(fbe_packet_t *packet);
static fbe_status_t fbe_perfstats_get_offset(fbe_packet_t *packet);
static fbe_status_t fbe_perfstats_get_enabled_state(fbe_packet_t *packet);
static fbe_status_t fbe_perfstats_set_enabled_state(fbe_packet_t *packet);
static fbe_status_t fbe_perfstats_get_offset_for_object_id(fbe_object_id_t object_id, fbe_u32_t *offset);
static fbe_status_t fbe_perfstats_add_object_to_map(fbe_object_id_t object_id, fbe_u32_t *offset);
static fbe_status_t fbe_perfstats_remove_object_from_map(fbe_object_id_t object_id, fbe_u32_t *offset);

/* Static members */
static fbe_base_service_t            fbe_perfstats_service;
static fbe_spinlock_t                map_lock;
static fbe_object_id_t               perfstats_object_map[PERFSTATS_MAX_SEP_OBJECTS];

static fbe_perfstats_sep_container_t                  *sep_container = NULL;
static fbe_perfstats_physical_package_container_t     *physical_container = NULL;
static fbe_bool_t                                     sep_stats_enabled = FBE_TRUE;
static fbe_bool_t                                     physical_stats_enabled = FBE_TRUE;

#ifdef C4_INTEGRATED
//histogram index helpers
static fbe_u32_t perfstats_lun_get_hist_index(fbe_u32_t num);
static fbe_u32_t perfstats_disk_get_hist_index(fbe_u32_t num);

//tick update helper

static inline void perfstats_update_disk_ticks(fbe_u32_t offset);

//OBS stuff
static FAMILY  *root;
static SET     *object_set;
static SET     *core_set;
static STYPE   *object_stype;
static STYPE   *core_stype;

static void fbe_obs_disk_enc_func(void *params);
static void fbe_obs_disk_core_enc_func(void *params);
static void fbe_obs_lun_enc_func(void *params);
static void fbe_obs_lun_core_enc_func(void *params);

static fbe_u64_t blockSize = 512;   //psuedo-constant used for block->byte OBS conversions
static fbe_u64_t coreCount = 1;

//structs to easily hold the computed stats
typedef struct fbe_obs_computed_stat_s
{
   const char* name;
   const char* summary;
   const char* title;
   VISIBILITY vis;
   COMPUTEDOPTYPE op;
   const char** statList; 
}fbe_obs_computed_stat_t;

typedef struct fbe_obs_byte_conversion_stat_s
{
   const char* name;
   const char* summary;
   const char* title;
   VISIBILITY vis;
   const char* statName; 
}fbe_obs_byte_conversion_stat_t;

static StatField disk_fields[] = 
{
   {NULL, "busyTicks", "Busy Ticks for this disk", ENG, "Busy Ticks", 0, U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_E64},
   {NULL, "idleTicks", "Idle Ticks for this disk", ENG, "Idle Ticks", offsetof(fbe_pdo_performance_counters_t, idle_ticks[0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_E64},
   {NULL, "nonZeroQueueArrivals", "This counter increments every time an IO arrives at this disk when its queue is not empty.", ENG, "Non-Zero Queue Arrivals", offsetof(fbe_pdo_performance_counters_t, non_zero_queue_arrivals[0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_E64},
   {NULL, "sumArrivalQueueLength", "This counter increases by the number of requests in the disk's queue when an IO arrives, including itself.", ENG, "Sum Arrival Queue Length", offsetof(fbe_pdo_performance_counters_t, sum_arrival_queue_length[0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_E64},
   {NULL, "readBlocks", "Number of blocks read from this disk", ENG, "Blocks read", offsetof(fbe_pdo_performance_counters_t, disk_blocks_read[0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "writeBlocks", "Number of blocks writen to this disk", ENG, "Blocks written", offsetof(fbe_pdo_performance_counters_t, disk_blocks_written[0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "reads", "Reads from this disk", ENG, "Reads", offsetof(fbe_pdo_performance_counters_t, disk_reads[0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "Count", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "writes", "Writes to this disk", ENG, "Writes", offsetof(fbe_pdo_performance_counters_t, disk_writes[0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "Count", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "blocksSeeked", "Total block sectors seeked or this disk", ENG, "Sum Blocks Seeked", offsetof(fbe_pdo_performance_counters_t, sum_blocks_seeked[0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount100us", "IOs that took less than 100 microseconds", ENG, "<100us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[0][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount200us", "IOs that took less than 200 microseconds", ENG, "<200us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[1][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount400us", "IOs that took less than 400 microseconds", ENG, "<400us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[2][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount800us", "IOs that took less than 800 microseconds", ENG, "<800us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[3][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount1600us", "IOs that took less than 1600 microseconds", ENG, "<1600us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[4][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount3200us", "IOs that took less than 3200 microseconds", ENG, "<3200us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[5][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount6400us", "IOs that took less than 6400 microseconds", ENG, "<6400us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[6][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount12800us", "IOs that took less than 12800 microseconds", ENG, "<12800us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[7][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount25600us", "IOs that took less than 25600 microseconds", ENG, "<25600us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[8][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount51200us", "IOs that took less than 51200 microseconds", ENG, "<51200us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[9][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount102400us", "IOs that took less than 102400 microseconds", ENG, "<102400us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[10][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount204800us", "IOs that took less than 204800 microseconds", ENG, "<204800us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[11][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount409600us", "IOs that took less than 409600 microseconds", ENG, "<409600us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[12][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount819200us", "IOs that took less than 819200 microseconds", ENG, "<819200us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[13][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount1638400us", "IOs that took less than 1638400 microseconds", ENG, "<1638400us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[14][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount3276800us", "IOs that took less than 3276800 microseconds", ENG, "<3276800us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[15][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount6553600us", "IOs that took less than 6553600 microseconds", ENG, "<6553600us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[16][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount13107200us", "IOs that took less than 13107200 microseconds", ENG, "<13107200us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[17][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount26214400us", "IOs that took less than 26214400 microseconds", ENG, "<26214400us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[18][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "latencyCount52428800us", "IOs that took less than 52428800 microseconds", ENG, "<52428800us IOs", offsetof(fbe_pdo_performance_counters_t, disk_srv_time_histogram[19][0])-offsetof(fbe_pdo_performance_counters_t, busy_ticks[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
};

static StatField lun_fields[] = 
{
   {NULL, "cumulativeReadResponseTime", "This counter increases on every read IO for this LUN by the number of milliseconds the IO spent in SEP. It always rounds up, so IOs taking less than 1 ms will still increment this counter by 1.", ENG, "Cumulative Read Response Time", 0, U_COUNTER64_FIELD, "ms", KSCALE_UNO, -1, WRAP_E64},
   {NULL, "cumulativeWriteResponseTime", "This counter increases on every write IO for this LUN by the number of milliseconds the IO spent in SEP. It always rounds up, so IOs taking less than 1 ms will still increment this counter by 1.", ENG, "Cumulative Write Response Time", offsetof(fbe_lun_performance_counters_t, cumulative_write_response_time[0]) - offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "ms", KSCALE_UNO, -1, WRAP_E64},
   {NULL, "readBlocks", "Number of blocks read from this LUN", ENG, "Blocks read", offsetof(fbe_lun_performance_counters_t, lun_blocks_read[0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "writeBlocks", "Number of blocks writen to this LUN", ENG, "Blocks written", offsetof(fbe_lun_performance_counters_t, lun_blocks_written[0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "reads", "Reads from this LUN", ENG, "Reads", offsetof(fbe_lun_performance_counters_t, lun_read_requests[0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "writes", "Writes to this LUN", ENG, "Writes", offsetof(fbe_lun_performance_counters_t, lun_write_requests[0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "stripeCrossings", "Total number of IOs that cross the RAID stripe boundary for this LUN", ENG, "Stripe Crossings", offsetof(fbe_lun_performance_counters_t, stripe_crossings[0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "stripeWrites", "Total number of IOs that were full stripe writes (also known as MR3 writes) for this LUN", ENG, "Stripe Writes", offsetof(fbe_lun_performance_counters_t, stripe_writes[0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "nonZeroQueueArrivals", "This counter increments every time an IO arrives at this LUN when its queue is not empty.", ENG, "Non-Zero Queue Arrivals", offsetof(fbe_lun_performance_counters_t, non_zero_queue_arrivals[0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "sumArrivalQueueLength", "This counter increases by the number of requests in the LUN queue when an IO arrives, including itself.", ENG, "Sum Arrrival Queue Length", offsetof(fbe_lun_performance_counters_t, sum_arrival_queue_length[0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadBlocksPosition0", "Blocks read from the disk in position 0 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Read Pos 0", offsetof(fbe_lun_performance_counters_t, disk_blocks_read[0][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadBlocksPosition1", "Blocks read from the disk in position 1 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks read Pos 1", offsetof(fbe_lun_performance_counters_t, disk_blocks_read[1][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadBlocksPosition2", "Blocks read from the disk in position 2 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks read Pos 2", offsetof(fbe_lun_performance_counters_t, disk_blocks_read[2][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadBlocksPosition3", "Blocks read from the disk in position 3 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks read Pos 3", offsetof(fbe_lun_performance_counters_t, disk_blocks_read[3][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadBlocksPosition4", "Blocks read from the disk in position 4 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks read Pos 4", offsetof(fbe_lun_performance_counters_t, disk_blocks_read[4][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadBlocksPosition5", "Blocks read from the disk in position 5 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks read Pos 5", offsetof(fbe_lun_performance_counters_t, disk_blocks_read[5][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadBlocksPosition6", "Blocks read from the disk in position 6 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks read Pos 6", offsetof(fbe_lun_performance_counters_t, disk_blocks_read[6][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadBlocksPosition7", "Blocks read from the disk in position 7 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks read Pos 7", offsetof(fbe_lun_performance_counters_t, disk_blocks_read[7][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadBlocksPosition8", "Blocks read from the disk in position 8 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks read Pos 8", offsetof(fbe_lun_performance_counters_t, disk_blocks_read[8][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadBlocksPosition9", "Blocks read from the disk in position 9 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks read Pos 9", offsetof(fbe_lun_performance_counters_t, disk_blocks_read[9][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadBlocksPosition10", "Blocks read from the disk in position 10 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks read Pos 10", offsetof(fbe_lun_performance_counters_t, disk_blocks_read[10][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadBlocksPosition11", "Blocks read from the disk in position 11 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks read Pos 11", offsetof(fbe_lun_performance_counters_t, disk_blocks_read[11][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadBlocksPosition12", "Blocks read from the disk in position 12 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks read Pos 12", offsetof(fbe_lun_performance_counters_t, disk_blocks_read[12][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadBlocksPosition13", "Blocks read from the disk in position 13 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks read Pos 13", offsetof(fbe_lun_performance_counters_t, disk_blocks_read[13][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadBlocksPosition14", "Blocks read from the disk in position 14 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks read Pos 14", offsetof(fbe_lun_performance_counters_t, disk_blocks_read[14][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadBlocksPosition15", "Blocks read from the disk in position 15 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks read Pos 15", offsetof(fbe_lun_performance_counters_t, disk_blocks_read[15][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWriteBlocksPosition0", "Blocks written to the disk in position 0 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Written Pos 0", offsetof(fbe_lun_performance_counters_t, disk_blocks_written[0][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWriteBlocksPosition1", "Blocks written to the disk in position 1 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Written Pos 1", offsetof(fbe_lun_performance_counters_t, disk_blocks_written[1][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWriteBlocksPosition2", "Blocks written to the disk in position 2 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Written Pos 2", offsetof(fbe_lun_performance_counters_t, disk_blocks_written[2][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWriteBlocksPosition3", "Blocks written to the disk in position 3 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Written Pos 3", offsetof(fbe_lun_performance_counters_t, disk_blocks_written[3][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWriteBlocksPosition4", "Blocks written to the disk in position 4 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Written Pos 4", offsetof(fbe_lun_performance_counters_t, disk_blocks_written[4][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWriteBlocksPosition5", "Blocks written to the disk in position 5 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Written Pos 5", offsetof(fbe_lun_performance_counters_t, disk_blocks_written[5][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWriteBlocksPosition6", "Blocks written to the disk in position 6 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Written Pos 6", offsetof(fbe_lun_performance_counters_t, disk_blocks_written[6][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWriteBlocksPosition7", "Blocks written to the disk in position 7 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Written Pos 7", offsetof(fbe_lun_performance_counters_t, disk_blocks_written[7][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWriteBlocksPosition8", "Blocks written to the disk in position 8 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Written Pos 8", offsetof(fbe_lun_performance_counters_t, disk_blocks_written[8][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWriteBlocksPosition9", "Blocks written to the disk in position 9 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Written Pos 9", offsetof(fbe_lun_performance_counters_t, disk_blocks_written[9][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWriteBlocksPosition10", "Blocks written to the disk in position 10 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Written Pos 10", offsetof(fbe_lun_performance_counters_t, disk_blocks_written[10][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWriteBlocksPosition11", "Blocks written to the disk in position 11 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Written Pos 11", offsetof(fbe_lun_performance_counters_t, disk_blocks_written[11][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWriteBlocksPosition12", "Blocks written to the disk in position 12 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Written Pos 12", offsetof(fbe_lun_performance_counters_t, disk_blocks_written[12][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWriteBlocksPosition13", "Blocks written to the disk in position 13 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Written Pos 13", offsetof(fbe_lun_performance_counters_t, disk_blocks_written[13][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWriteBlocksPosition14", "Blocks written to the disk in position 14 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Written Pos 14", offsetof(fbe_lun_performance_counters_t, disk_blocks_written[14][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWriteBlocksPosition15", "Blocks written to the disk in position 15 of the RAID group this LUN is bound to.", ENG, "RG Disk Blocks Written Pos 15", offsetof(fbe_lun_performance_counters_t, disk_blocks_written[15][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadsPosition0", "Reads from the disk in position 0 of the RAID group this LUN is bound to.", ENG, "RG Disk Reads Pos 0", offsetof(fbe_lun_performance_counters_t, disk_reads[0][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadsPosition1", "Reads from the disk in position 1 of the RAID group this LUN is bound to.", ENG, "RG Disk Reads Pos 1", offsetof(fbe_lun_performance_counters_t, disk_reads[1][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadsPosition2", "Reads from the disk in position 2 of the RAID group this LUN is bound to.", ENG, "RG Disk Reads Pos 2", offsetof(fbe_lun_performance_counters_t, disk_reads[2][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadsPosition3", "Reads from the disk in position 3 of the RAID group this LUN is bound to.", ENG, "RG Disk Reads Pos 3", offsetof(fbe_lun_performance_counters_t, disk_reads[3][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadsPosition4", "Reads from the disk in position 4 of the RAID group this LUN is bound to.", ENG, "RG Disk Reads Pos 4", offsetof(fbe_lun_performance_counters_t, disk_reads[4][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadsPosition5", "Reads from the disk in position 5 of the RAID group this LUN is bound to.", ENG, "RG Disk Reads Pos 5", offsetof(fbe_lun_performance_counters_t, disk_reads[5][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadsPosition6", "Reads from the disk in position 6 of the RAID group this LUN is bound to.", ENG, "RG Disk Reads Pos 6", offsetof(fbe_lun_performance_counters_t, disk_reads[6][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadsPosition7", "Reads from the disk in position 7 of the RAID group this LUN is bound to.", ENG, "RG Disk Reads Pos 7", offsetof(fbe_lun_performance_counters_t, disk_reads[7][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadsPosition8", "Reads from the disk in position 8 of the RAID group this LUN is bound to.", ENG, "RG Disk Reads Pos 8", offsetof(fbe_lun_performance_counters_t, disk_reads[8][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadsPosition9", "Reads from the disk in position 9 of the RAID group this LUN is bound to.", ENG, "RG Disk Reads Pos 9", offsetof(fbe_lun_performance_counters_t, disk_reads[9][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadsPosition10", "Reads from the disk in position 10 of the RAID group this LUN is bound to.", ENG, "RG Disk Reads Pos 10", offsetof(fbe_lun_performance_counters_t, disk_reads[10][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadsPosition11", "Reads from the disk in position 11 of the RAID group this LUN is bound to.", ENG, "RG Disk Reads Pos 11", offsetof(fbe_lun_performance_counters_t, disk_reads[11][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadsPosition12", "Reads from the disk in position 12 of the RAID group this LUN is bound to.", ENG, "RG Disk Reads Pos 12", offsetof(fbe_lun_performance_counters_t, disk_reads[12][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadsPosition13", "Reads from the disk in position 13 of the RAID group this LUN is bound to.", ENG, "RG Disk Reads Pos 13", offsetof(fbe_lun_performance_counters_t, disk_reads[13][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadsPosition14", "Reads from the disk in position 14 of the RAID group this LUN is bound to.", ENG, "RG Disk Reads Pos 14", offsetof(fbe_lun_performance_counters_t, disk_reads[14][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgReadsPosition15", "Reads from the disk in position 15 of the RAID group this LUN is bound to.", ENG, "RG Disk Reads Pos 15", offsetof(fbe_lun_performance_counters_t, disk_reads[15][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWritesPosition0", "Writes to the disk in position 0 of the RAID group this LUN is bound to.", ENG, "RG Disk Writes Pos 0", offsetof(fbe_lun_performance_counters_t, disk_writes[0][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWritesPosition1", "Writes to the disk in position 1 of the RAID group this LUN is bound to.", ENG, "RG Disk Writes Pos 1", offsetof(fbe_lun_performance_counters_t, disk_writes[1][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWritesPosition2", "Writes to the disk in position 2 of the RAID group this LUN is bound to.", ENG, "RG Disk Writes Pos 2", offsetof(fbe_lun_performance_counters_t, disk_writes[2][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWritesPosition3", "Writes to the disk in position 3 of the RAID group this LUN is bound to.", ENG, "RG Disk Writes Pos 3", offsetof(fbe_lun_performance_counters_t, disk_writes[3][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWritesPosition4", "Writes to the disk in position 4 of the RAID group this LUN is bound to.", ENG, "RG Disk Writes Pos 4", offsetof(fbe_lun_performance_counters_t, disk_writes[4][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWritesPosition5", "Writes to the disk in position 5 of the RAID group this LUN is bound to.", ENG, "RG Disk Writes Pos 5", offsetof(fbe_lun_performance_counters_t, disk_writes[5][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWritesPosition6", "Writes to the disk in position 6 of the RAID group this LUN is bound to.", ENG, "RG Disk Writes Pos 6", offsetof(fbe_lun_performance_counters_t, disk_writes[6][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWritesPosition7", "Writes to the disk in position 7 of the RAID group this LUN is bound to.", ENG, "RG Disk Writes Pos 7", offsetof(fbe_lun_performance_counters_t, disk_writes[7][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWritesPosition8", "Writes to the disk in position 8 of the RAID group this LUN is bound to.", ENG, "RG Disk Writes Pos 8", offsetof(fbe_lun_performance_counters_t, disk_writes[8][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWritesPosition9", "Writes to the disk in position 9 of the RAID group this LUN is bound to.", ENG, "RG Disk Writes Pos 9", offsetof(fbe_lun_performance_counters_t, disk_writes[9][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWritesPosition10", "Writes to the disk in position 10 of the RAID group this LUN is bound to.", ENG, "RG Disk Writes Pos 10", offsetof(fbe_lun_performance_counters_t, disk_writes[10][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWritesPosition11", "Writes to the disk in position 11 of the RAID group this LUN is bound to.", ENG, "RG Disk Writes Pos 11", offsetof(fbe_lun_performance_counters_t, disk_writes[11][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWritesPosition12", "Writes to the disk in position 12 of the RAID group this LUN is bound to.", ENG, "RG Disk Writes Pos 12", offsetof(fbe_lun_performance_counters_t, disk_writes[12][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWritesPosition13", "Writes to the disk in position 13 of the RAID group this LUN is bound to.", ENG, "RG Disk Writes Pos 13", offsetof(fbe_lun_performance_counters_t, disk_writes[13][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWritesPosition14", "Writes to the disk in position 14 of the RAID group this LUN is bound to.", ENG, "RG Disk Writes Pos 14", offsetof(fbe_lun_performance_counters_t, disk_writes[14][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "rgWritesPosition15", "Writes to the disk in position 15 of the RAID group this LUN is bound to.", ENG, "RG Disk Writes Pos 15", offsetof(fbe_lun_performance_counters_t, disk_writes[15][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "readSize1Block", "1 block reads for this LUN", ENG, "1 Block reads", offsetof(fbe_lun_performance_counters_t, lun_io_size_read_histogram[0][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "readSize2Block", "2 block reads for this LUN", ENG, "2 Block reads", offsetof(fbe_lun_performance_counters_t, lun_io_size_read_histogram[1][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "readSize4Block", "4 block reads for this LUN", ENG, "4 Block reads", offsetof(fbe_lun_performance_counters_t, lun_io_size_read_histogram[2][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "readSize8Block", "8 block reads for this LUN", ENG, "8 Block reads", offsetof(fbe_lun_performance_counters_t, lun_io_size_read_histogram[3][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "readSize16Block", "16 block reads for this LUN", ENG, "16 Block reads", offsetof(fbe_lun_performance_counters_t, lun_io_size_read_histogram[4][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "readSize32Block", "32 block reads for this LUN", ENG, "32 Block reads", offsetof(fbe_lun_performance_counters_t, lun_io_size_read_histogram[5][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "readSize64Block", "64 block reads for this LUN", ENG, "64 Block reads", offsetof(fbe_lun_performance_counters_t, lun_io_size_read_histogram[6][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "readSize128Block", "128 block reads for this LUN", ENG, "128 Block reads", offsetof(fbe_lun_performance_counters_t, lun_io_size_read_histogram[7][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "readSize256Block", "256 block reads for this LUN", ENG, "256 Block reads", offsetof(fbe_lun_performance_counters_t, lun_io_size_read_histogram[8][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "readSize512Block", "512 block reads for this LUN", ENG, "512 Block reads", offsetof(fbe_lun_performance_counters_t, lun_io_size_read_histogram[9][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "readSize1024BlockPlusOverflow", "1024+ block reads for this LUN", ENG, ">=1024 Block reads", offsetof(fbe_lun_performance_counters_t, lun_io_size_read_histogram[10][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "writeSize1Block", "1 block writes for this LUN", ENG, "1 Block writes", offsetof(fbe_lun_performance_counters_t, lun_io_size_write_histogram[0][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "writeSize2Block", "2 block writes for this LUN", ENG, "2 Block writes", offsetof(fbe_lun_performance_counters_t, lun_io_size_write_histogram[1][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "writeSize4Block", "4 block writes for this LUN", ENG, "4 Block writes", offsetof(fbe_lun_performance_counters_t, lun_io_size_write_histogram[2][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "writeSize8Block", "8 block writes for this LUN", ENG, "8 Block writes", offsetof(fbe_lun_performance_counters_t, lun_io_size_write_histogram[3][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "writeSize16Block", "16 block writes for this LUN", ENG, "16 Block writes", offsetof(fbe_lun_performance_counters_t, lun_io_size_write_histogram[4][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "writeSize32Block", "32 block writes for this LUN", ENG, "32 Block writes", offsetof(fbe_lun_performance_counters_t, lun_io_size_write_histogram[5][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "writeSize64Block", "64 block writes for this LUN", ENG, "64 Block writes", offsetof(fbe_lun_performance_counters_t, lun_io_size_write_histogram[6][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "writeSize128Block", "128 block writes for this LUN", ENG, "128 Block writes", offsetof(fbe_lun_performance_counters_t, lun_io_size_write_histogram[7][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "writeSize256Block", "256 block writes for this LUN", ENG, "256 Block writes", offsetof(fbe_lun_performance_counters_t, lun_io_size_write_histogram[8][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "writeSize512Block", "512 block writes for this LUN", ENG, "512 Block writes", offsetof(fbe_lun_performance_counters_t, lun_io_size_write_histogram[9][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
   {NULL, "writeSize1024BlockPlusOverflow", "1024+ block writes for this LUN", ENG, ">=1024 Block writes", offsetof(fbe_lun_performance_counters_t, lun_io_size_write_histogram[10][0])-offsetof(fbe_lun_performance_counters_t, cumulative_read_response_time[0]), U_COUNTER64_FIELD, "", KSCALE_UNO, -1, WRAP_W64},
};

static fbe_obs_computed_stat_t lun_computed_fields[] =
{
   {"cumulativeReadResponseTime", "This counter increases on every read IO for this LUN by the number of milliseconds the IO spent in SEP. It always rounds up, so IOs taking less than 1 ms will still increment this counter by 1.", "Cumulative Read Response Time", ENG, SUM, (const char*[]){"core.*.cumulativeReadResponseTime"}},
   {"cumulativeWriteResponseTime", "This counter increases on every write IO for this LUN by the number of milliseconds the IO spent in SEP. It always rounds up, so IOs taking less than 1 ms will still increment this counter by 1.", "Cumulative Write Response Time", ENG, SUM, (const char*[]){"core.*.cumulativeWriteResponseTime"}},
   {"readBlocks", "Number of blocks read from this LUN", "Blocks read", ENG, SUM, (const char*[]){"core.*.readBlocks"}},
   {"writeBlocks", "Number of blocks written to this LUN", "Blocks written", ENG, SUM, (const char*[]){"core.*.writeBlocks"}},
   {"reads", "Reads from this LUN", "Reads", ENG, SUM, (const char*[]){"core.*.reads"}},
   {"writes", "Writes to this LUN", "Writes", ENG, SUM, (const char*[]){"core.*.writes"}},
   {"stripeCrossings", "Total number of IOs that cross the RAID stripe boundary for this LUN", "Stripe Crossings", ENG, SUM, (const char*[]){"core.*.stripeCrossings"}},
   {"stripeWrites", "Total number of IOs that were full stripe writes (also known as MR3 writes) for this LUN", "Stripe Writes", ENG, SUM, (const char*[]){"core.*.stripeWrites"}},
   {"nonZeroQueueArrivals", "This counter increments every time an IO arrives at this LUN when its queue is not empty.", "Non-Zero Queue Arrivals", ENG, SUM, (const char*[]){"core.*.nonZeroQueueArrivals"}},
   {"sumArrivalQueueLength", "This counter increases by the number of requests in the LUN queue when an IO arrives, including itself.", "Sum Arrrival Queue Length", ENG, SUM, (const char*[]){"core.*.sumArrivalQueueLength"}},
   {"rgReadBlocksPosition0", "Blocks read from the disk in position 0 of the RAID group this LUN is bound to.", "RG Disk Blocks Read Pos 0", ENG, SUM, (const char*[]){"core.*.rgReadBlocksPosition0"}},
   {"rgReadBlocksPosition1", "Blocks read from the disk in position 1 of the RAID group this LUN is bound to.", "RG Disk Blocks read Pos 1", ENG, SUM, (const char*[]){"core.*.rgReadBlocksPosition1"}},
   {"rgReadBlocksPosition2", "Blocks read from the disk in position 2 of the RAID group this LUN is bound to.", "RG Disk Blocks read Pos 2", ENG, SUM, (const char*[]){"core.*.rgReadBlocksPosition2"}},
   {"rgReadBlocksPosition3", "Blocks read from the disk in position 3 of the RAID group this LUN is bound to.", "RG Disk Blocks read Pos 3", ENG, SUM, (const char*[]){"core.*.rgReadBlocksPosition3"}},
   {"rgReadBlocksPosition4", "Blocks read from the disk in position 4 of the RAID group this LUN is bound to.", "RG Disk Blocks read Pos 4", ENG, SUM, (const char*[]){"core.*.rgReadBlocksPosition4"}},
   {"rgReadBlocksPosition5", "Blocks read from the disk in position 5 of the RAID group this LUN is bound to.", "RG Disk Blocks read Pos 5", ENG, SUM, (const char*[]){"core.*.rgReadBlocksPosition5"}},
   {"rgReadBlocksPosition6", "Blocks read from the disk in position 6 of the RAID group this LUN is bound to.", "RG Disk Blocks read Pos 6", ENG, SUM, (const char*[]){"core.*.rgReadBlocksPosition6"}},
   {"rgReadBlocksPosition7", "Blocks read from the disk in position 7 of the RAID group this LUN is bound to.", "RG Disk Blocks read Pos 7", ENG, SUM, (const char*[]){"core.*.rgReadBlocksPosition7"}},
   {"rgReadBlocksPosition8", "Blocks read from the disk in position 8 of the RAID group this LUN is bound to.", "RG Disk Blocks read Pos 8", ENG, SUM, (const char*[]){"core.*.rgReadBlocksPosition8"}},
   {"rgReadBlocksPosition9", "Blocks read from the disk in position 9 of the RAID group this LUN is bound to.", "RG Disk Blocks read Pos 9", ENG, SUM, (const char*[]){"core.*.rgReadBlocksPosition9"}},
   {"rgReadBlocksPosition10", "Blocks read from the disk in position 10 of the RAID group this LUN is bound to.", "RG Disk Blocks read Pos 10", ENG, SUM, (const char*[]){"core.*.rgReadBlocksPosition10"}},
   {"rgReadBlocksPosition11", "Blocks read from the disk in position 11 of the RAID group this LUN is bound to.", "RG Disk Blocks read Pos 11", ENG, SUM, (const char*[]){"core.*.rgReadBlocksPosition11"}},
   {"rgReadBlocksPosition12", "Blocks read from the disk in position 12 of the RAID group this LUN is bound to.", "RG Disk Blocks read Pos 12", ENG, SUM, (const char*[]){"core.*.rgReadBlocksPosition12"}},
   {"rgReadBlocksPosition13", "Blocks read from the disk in position 13 of the RAID group this LUN is bound to.", "RG Disk Blocks read Pos 13", ENG, SUM, (const char*[]){"core.*.rgReadBlocksPosition13"}},
   {"rgReadBlocksPosition14", "Blocks read from the disk in position 14 of the RAID group this LUN is bound to.", "RG Disk Blocks read Pos 14", ENG, SUM, (const char*[]){"core.*.rgReadBlocksPosition14"}},
   {"rgReadBlocksPosition15", "Blocks read from the disk in position 15 of the RAID group this LUN is bound to.", "RG Disk Blocks read Pos 15", ENG, SUM, (const char*[]){"core.*.rgReadBlocksPosition15"}},
   {"rgWriteBlocksPosition0", "Blocks written to the disk in position 0 of the RAID group this LUN is bound to.", "RG Disk Blocks Written Pos 0", ENG, SUM, (const char*[]){"core.*.rgWriteBlocksPosition0"}},
   {"rgWriteBlocksPosition1", "Blocks written to the disk in position 1 of the RAID group this LUN is bound to.", "RG Disk Blocks Written Pos 1", ENG, SUM, (const char*[]){"core.*.rgWriteBlocksPosition1"}},
   {"rgWriteBlocksPosition2", "Blocks written to the disk in position 2 of the RAID group this LUN is bound to.", "RG Disk Blocks Written Pos 2", ENG, SUM, (const char*[]){"core.*.rgWriteBlocksPosition2"}},
   {"rgWriteBlocksPosition3", "Blocks written to the disk in position 3 of the RAID group this LUN is bound to.", "RG Disk Blocks Written Pos 3", ENG, SUM, (const char*[]){"core.*.rgWriteBlocksPosition3"}},
   {"rgWriteBlocksPosition4", "Blocks written to the disk in position 4 of the RAID group this LUN is bound to.", "RG Disk Blocks Written Pos 4", ENG, SUM, (const char*[]){"core.*.rgWriteBlocksPosition4"}},
   {"rgWriteBlocksPosition5", "Blocks written to the disk in position 5 of the RAID group this LUN is bound to.", "RG Disk Blocks Written Pos 5", ENG, SUM, (const char*[]){"core.*.rgWriteBlocksPosition5"}},
   {"rgWriteBlocksPosition6", "Blocks written to the disk in position 6 of the RAID group this LUN is bound to.", "RG Disk Blocks Written Pos 6", ENG, SUM, (const char*[]){"core.*.rgWriteBlocksPosition6"}},
   {"rgWriteBlocksPosition7", "Blocks written to the disk in position 7 of the RAID group this LUN is bound to.", "RG Disk Blocks Written Pos 7", ENG, SUM, (const char*[]){"core.*.rgWriteBlocksPosition7"}},
   {"rgWriteBlocksPosition8", "Blocks written to the disk in position 8 of the RAID group this LUN is bound to.", "RG Disk Blocks Written Pos 8", ENG, SUM, (const char*[]){"core.*.rgWriteBlocksPosition8"}},
   {"rgWriteBlocksPosition9", "Blocks written to the disk in position 9 of the RAID group this LUN is bound to.", "RG Disk Blocks Written Pos 9", ENG, SUM, (const char*[]){"core.*.rgWriteBlocksPosition9"}},
   {"rgWriteBlocksPosition10", "Blocks written to the disk in position 10 of the RAID group this LUN is bound to.", "RG Disk Blocks Written Pos 10", ENG, SUM, (const char*[]){"core.*.rgWriteBlocksPosition10"}},
   {"rgWriteBlocksPosition11", "Blocks written to the disk in position 11 of the RAID group this LUN is bound to.", "RG Disk Blocks Written Pos 11", ENG, SUM, (const char*[]){"core.*.rgWriteBlocksPosition11"}},
   {"rgWriteBlocksPosition12", "Blocks written to the disk in position 12 of the RAID group this LUN is bound to.", "RG Disk Blocks Written Pos 12", ENG, SUM, (const char*[]){"core.*.rgWriteBlocksPosition12"}},
   {"rgWriteBlocksPosition13", "Blocks written to the disk in position 13 of the RAID group this LUN is bound to.", "RG Disk Blocks Written Pos 13", ENG, SUM, (const char*[]){"core.*.rgWriteBlocksPosition13"}},
   {"rgWriteBlocksPosition14", "Blocks written to the disk in position 14 of the RAID group this LUN is bound to.", "RG Disk Blocks Written Pos 14", ENG, SUM, (const char*[]){"core.*.rgWriteBlocksPosition14"}},
   {"rgWriteBlocksPosition15", "Blocks written to the disk in position 15 of the RAID group this LUN is bound to.", "RG Disk Blocks Written Pos 15", ENG, SUM, (const char*[]){"core.*.rgWriteBlocksPosition15"}},
   {"rgReadsPosition0", "Reads from the disk in position 0 of the RAID group this LUN is bound to.", "RG Disk Reads Pos 0", ENG, SUM, (const char*[]){"core.*.rgReadsPosition0"}},
   {"rgReadsPosition1", "Reads from the disk in position 1 of the RAID group this LUN is bound to.", "RG Disk Reads Pos 1", ENG, SUM, (const char*[]){"core.*.rgReadsPosition1"}},
   {"rgReadsPosition2", "Reads from the disk in position 2 of the RAID group this LUN is bound to.", "RG Disk Reads Pos 2", ENG, SUM, (const char*[]){"core.*.rgReadsPosition2"}},
   {"rgReadsPosition3", "Reads from the disk in position 3 of the RAID group this LUN is bound to.", "RG Disk Reads Pos 3", ENG, SUM, (const char*[]){"core.*.rgReadsPosition3"}},
   {"rgReadsPosition4", "Reads from the disk in position 4 of the RAID group this LUN is bound to.", "RG Disk Reads Pos 4", ENG, SUM, (const char*[]){"core.*.rgReadsPosition4"}},
   {"rgReadsPosition5", "Reads from the disk in position 5 of the RAID group this LUN is bound to.", "RG Disk Reads Pos 5", ENG, SUM, (const char*[]){"core.*.rgReadsPosition5"}},
   {"rgReadsPosition6", "Reads from the disk in position 6 of the RAID group this LUN is bound to.", "RG Disk Reads Pos 6", ENG, SUM, (const char*[]){"core.*.rgReadsPosition6"}},
   {"rgReadsPosition7", "Reads from the disk in position 7 of the RAID group this LUN is bound to.", "RG Disk Reads Pos 7", ENG, SUM, (const char*[]){"core.*.rgReadsPosition7"}},
   {"rgReadsPosition8", "Reads from the disk in position 8 of the RAID group this LUN is bound to.", "RG Disk Reads Pos 8", ENG, SUM, (const char*[]){"core.*.rgReadsPosition8"}},
   {"rgReadsPosition9", "Reads from the disk in position 9 of the RAID group this LUN is bound to.", "RG Disk Reads Pos 9", ENG, SUM, (const char*[]){"core.*.rgReadsPosition9"}},
   {"rgReadsPosition10", "Reads from the disk in position 10 of the RAID group this LUN is bound to.", "RG Disk Reads Pos 10", ENG, SUM, (const char*[]){"core.*.rgReadsPosition10"}},
   {"rgReadsPosition11", "Reads from the disk in position 11 of the RAID group this LUN is bound to.", "RG Disk Reads Pos 11", ENG, SUM, (const char*[]){"core.*.rgReadsPosition11"}},
   {"rgReadsPosition12", "Reads from the disk in position 12 of the RAID group this LUN is bound to.", "RG Disk Reads Pos 12", ENG, SUM, (const char*[]){"core.*.rgReadsPosition12"}},
   {"rgReadsPosition13", "Reads from the disk in position 13 of the RAID group this LUN is bound to.", "RG Disk Reads Pos 13", ENG, SUM, (const char*[]){"core.*.rgReadsPosition13"}},
   {"rgReadsPosition14", "Reads from the disk in position 14 of the RAID group this LUN is bound to.", "RG Disk Reads Pos 14", ENG, SUM, (const char*[]){"core.*.rgReadsPosition14"}},
   {"rgReadsPosition15", "Reads from the disk in position 15 of the RAID group this LUN is bound to.", "RG Disk Reads Pos 15", ENG, SUM, (const char*[]){"core.*.rgReadsPosition15"}},
   {"rgWritesPosition0", "Writes to the disk in position 0 of the RAID group this LUN is bound to.", "RG Disk Writes Pos 0", ENG, SUM, (const char*[]){"core.*.rgWritesPosition0"}},
   {"rgWritesPosition1", "Writes to the disk in position 1 of the RAID group this LUN is bound to.", "RG Disk Writes Pos 1", ENG, SUM, (const char*[]){"core.*.rgWritesPosition1"}},
   {"rgWritesPosition2", "Writes to the disk in position 2 of the RAID group this LUN is bound to.", "RG Disk Writes Pos 2", ENG, SUM, (const char*[]){"core.*.rgWritesPosition2"}},
   {"rgWritesPosition3", "Writes to the disk in position 3 of the RAID group this LUN is bound to.", "RG Disk Writes Pos 3", ENG, SUM, (const char*[]){"core.*.rgWritesPosition3"}},
   {"rgWritesPosition4", "Writes to the disk in position 4 of the RAID group this LUN is bound to.", "RG Disk Writes Pos 4", ENG, SUM, (const char*[]){"core.*.rgWritesPosition4"}},
   {"rgWritesPosition5", "Writes to the disk in position 5 of the RAID group this LUN is bound to.", "RG Disk Writes Pos 5", ENG, SUM, (const char*[]){"core.*.rgWritesPosition5"}},
   {"rgWritesPosition6", "Writes to the disk in position 6 of the RAID group this LUN is bound to.", "RG Disk Writes Pos 6", ENG, SUM, (const char*[]){"core.*.rgWritesPosition6"}},
   {"rgWritesPosition7", "Writes to the disk in position 7 of the RAID group this LUN is bound to.", "RG Disk Writes Pos 7", ENG, SUM, (const char*[]){"core.*.rgWritesPosition7"}},
   {"rgWritesPosition8", "Writes to the disk in position 8 of the RAID group this LUN is bound to.", "RG Disk Writes Pos 8", ENG, SUM, (const char*[]){"core.*.rgWritesPosition8"}},
   {"rgWritesPosition9", "Writes to the disk in position 9 of the RAID group this LUN is bound to.", "RG Disk Writes Pos 9", ENG, SUM, (const char*[]){"core.*.rgWritesPosition9"}},
   {"rgWritesPosition10", "Writes to the disk in position 10 of the RAID group this LUN is bound to.", "RG Disk Writes Pos 10", ENG, SUM, (const char*[]){"core.*.rgWritesPosition10"}},
   {"rgWritesPosition11", "Writes to the disk in position 11 of the RAID group this LUN is bound to.", "RG Disk Writes Pos 11", ENG, SUM, (const char*[]){"core.*.rgWritesPosition11"}},
   {"rgWritesPosition12", "Writes to the disk in position 12 of the RAID group this LUN is bound to.", "RG Disk Writes Pos 12", ENG, SUM, (const char*[]){"core.*.rgWritesPosition12"}},
   {"rgWritesPosition13", "Writes to the disk in position 13 of the RAID group this LUN is bound to.", "RG Disk Writes Pos 13", ENG, SUM, (const char*[]){"core.*.rgWritesPosition13"}},
   {"rgWritesPosition14", "Writes to the disk in position 14 of the RAID group this LUN is bound to.", "RG Disk Writes Pos 14", ENG, SUM, (const char*[]){"core.*.rgWritesPosition14"}},
   {"rgWritesPosition15", "Writes to the disk in position 15 of the RAID group this LUN is bound to.", "RG Disk Writes Pos 15", ENG, SUM, (const char*[]){"core.*.rgWritesPosition15"}},
   {"readSize1Block", "1 block reads for this LUN", "1 Block reads", ENG, SUM, (const char*[]){"core.*.readSize1Block"}},
   {"readSize2Block", "2 block reads for this LUN", "2 Block reads", ENG, SUM, (const char*[]){"core.*.readSize2Block"}},
   {"readSize4Block", "4 block reads for this LUN", "4 Block reads", ENG, SUM, (const char*[]){"core.*.readSize4Block"}},
   {"readSize8Block", "8 block reads for this LUN", "8 Block reads", ENG, SUM, (const char*[]){"core.*.readSize8Block"}},
   {"readSize16Block", "16 block reads for this LUN", "16 Block reads", ENG, SUM, (const char*[]){"core.*.readSize16Block"}},
   {"readSize32Block", "32 block reads for this LUN", "32 Block reads", ENG, SUM, (const char*[]){"core.*.readSize32Block"}},
   {"readSize64Block", "64 block reads for this LUN", "64 Block reads", ENG, SUM, (const char*[]){"core.*.readSize64Block"}},
   {"readSize128Block", "128 block reads for this LUN", "128 Block reads", ENG, SUM, (const char*[]){"core.*.readSize128Block"}},
   {"readSize256Block", "256 block reads for this LUN", "256 Block reads", ENG, SUM, (const char*[]){"core.*.readSize256Block"}},
   {"readSize512Block", "512 block reads for this LUN", "512 Block reads", ENG, SUM, (const char*[]){"core.*.readSize512Block"}},
   {"readSize1024BlockPlusOverflow", "1024+ block reads for this LUN", ">=1024 Block reads", ENG, SUM, (const char*[]){"core.*.readSize1024BlockPlusOverflow"}},
   {"writeSize1Block", "1 block writes for this LUN", "1 Block writes", ENG, SUM, (const char*[]){"core.*.writeSize1Block"}},
   {"writeSize2Block", "2 block writes for this LUN", "2 Block writes", ENG, SUM, (const char*[]){"core.*.writeSize2Block"}},
   {"writeSize4Block", "4 block writes for this LUN", "4 Block writes", ENG, SUM, (const char*[]){"core.*.writeSize4Block"}},
   {"writeSize8Block", "8 block writes for this LUN", "8 Block writes", ENG, SUM, (const char*[]){"core.*.writeSize8Block"}},
   {"writeSize16Block", "16 block writes for this LUN", "16 Block writes", ENG, SUM, (const char*[]){"core.*.writeSize16Block"}},
   {"writeSize32Block", "32 block writes for this LUN", "32 Block writes", ENG, SUM, (const char*[]){"core.*.writeSize32Block"}},
   {"writeSize64Block", "64 block writes for this LUN", "64 Block writes", ENG, SUM, (const char*[]){"core.*.writeSize64Block"}},
   {"writeSize128Block", "128 block writes for this LUN", "128 Block writes", ENG, SUM, (const char*[]){"core.*.writeSize128Block"}},
   {"writeSize256Block", "256 block writes for this LUN", "256 Block writes", ENG, SUM, (const char*[]){"core.*.writeSize256Block"}},
   {"writeSize512Block", "512 block writes for this LUN", "512 Block writes", ENG, SUM, (const char*[]){"core.*.writeSize512Block"}},
   {"writeSize1024BlockPlusOverflow", "1024+ block writes for this LUN", ">=1024 Block writes", ENG, SUM, (const char*[]){"core.*.writeSize1024BlockPlusOverflow"}},
};

static fbe_obs_computed_stat_t disk_computed_fields[] =
{
   {"busyTicks", "Busy Ticks for this disk", "Busy Ticks", PRODUCT, SUM, (const char*[]){"core.*.busyTicks"}},
   {"idleTicks", "Idle Ticks for this disk", "Idle Ticks", PRODUCT, SUM, (const char*[]){"core.*.idleTicks"}},
   {"nonZeroQueueArrivals", "This counter increments every time an IO arrives at this disk when its queue is not empty.", "Non-Zero Queue Arrivals", ENG, SUM, (const char*[]){"core.*.nonZeroQueueArrivals"}},
   {"sumArrivalQueueLength", "This counter increases by the number of requests in the disk's queue when an IO arrives, including itself.", "Sum Arrrival Queue Length", PRODUCT, SUM, (const char*[]){"core.*.sumArrivalQueueLength"}},
   {"readBlocks", "Number of blocks read from this disk", "Blocks read", PRODUCT, SUM, (const char*[]){"core.*.readBlocks"}},
   {"writeBlocks", "Number of blocks writen to this disk", "Blocks written", PRODUCT, SUM, (const char*[]){"core.*.writeBlocks"}},
   {"blocksSeeked", "Total block sectors seeked or this disk", "Sum Blocks Seeked", ENG, SUM, (const char*[]){"core.*.blocksSeeked"}},
   {"latencyCount100us", "IOs that took less than 100 microseconds", "<100us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount100us"}},
   {"latencyCount200us", "IOs that took less than 200 microseconds", "<200us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount200us"}},
   {"latencyCount400us", "IOs that took less than 400 microseconds", "<400us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount400us"}},
   {"latencyCount800us", "IOs that took less than 800 microseconds", "<800us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount800us"}},
   {"latencyCount1600us", "IOs that took less than 1600 microseconds", "<1600us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount1600us"}},
   {"latencyCount3200us", "IOs that took less than 3200 microseconds", "<3200us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount3200us"}},
   {"latencyCount6400us", "IOs that took less than 6400 microseconds", "<6400us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount6400us"}},
   {"latencyCount12800us", "IOs that took less than 12800 microseconds", "<12800us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount12800us"}},
   {"latencyCount25600us", "IOs that took less than 25600 microseconds", "<25600us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount25600us"}},
   {"latencyCount51200us", "IOs that took less than 51200 microseconds", "<51200us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount51200us"}},
   {"latencyCount102400us", "IOs that took less than 102400 microseconds", "<102400us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount102400us"}},
   {"latencyCount204800us", "IOs that took less than 204800 microseconds", "<204800us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount204800us"}},
   {"latencyCount409600us", "IOs that took less than 409600 microseconds", "<409600us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount409600us"}},
   {"latencyCount819200us", "IOs that took less than 819200 microseconds", "<819200us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount819200us"}},
   {"latencyCount1638400us", "IOs that took less than 1638400 microseconds", "<1638400us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount1638400us"}},
   {"latencyCount3276800us", "IOs that took less than 3276800 microseconds", "<3276800us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount3276800us"}},
   {"latencyCount6553600us", "IOs that took less than 6553600 microseconds", "<6553600us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount6553600us"}},
   {"latencyCount13107200us", "IOs that took less than 13107200 microseconds", "<13107200us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount13107200us"}},
   {"latencyCount26214400us", "IOs that took less than 26214400 microseconds", "<26214400us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount26214400us"}},
   {"latencyCount52428800us", "IOs that took less than 52428800 microseconds", "<52428800us IOs", ENG, SUM, (const char*[]){"core.*.latencyCount52428800us"}},
};

static fbe_obs_byte_conversion_stat_t lun_byte_conversion[] =
{
   {"readBytes", "Number of bytes read from this LUN", "Read", ENG, "readBlocks"},
   {"writeBytes", "Number of bytes written to this LUN", "Written", ENG, "writeBlocks"},
   {"rgReadPosition0", "Bytes read from the disk in position 0 of the RAID group this LUN is bound to.", "RG Disk Bytes Read Pos 0", ENG, "rgReadBlocksPosition0"},
   {"rgReadPosition1", "Bytes read from the disk in position 1 of the RAID group this LUN is bound to.", "RG Disk Bytes read Pos 1", ENG, "rgReadBlocksPosition1"},
   {"rgReadPosition2", "Bytes read from the disk in position 2 of the RAID group this LUN is bound to.", "RG Disk Bytes read Pos 2", ENG, "rgReadBlocksPosition2"},
   {"rgReadPosition3", "Bytes read from the disk in position 3 of the RAID group this LUN is bound to.", "RG Disk Bytes read Pos 3", ENG, "rgReadBlocksPosition3"},
   {"rgReadPosition4", "Bytes read from the disk in position 4 of the RAID group this LUN is bound to.", "RG Disk Bytes read Pos 4", ENG, "rgReadBlocksPosition4"},
   {"rgReadPosition5", "Bytes read from the disk in position 5 of the RAID group this LUN is bound to.", "RG Disk Bytes read Pos 5", ENG, "rgReadBlocksPosition5"},
   {"rgReadPosition6", "Bytes read from the disk in position 6 of the RAID group this LUN is bound to.", "RG Disk Bytes read Pos 6", ENG, "rgReadBlocksPosition6"},
   {"rgReadPosition7", "Bytes read from the disk in position 7 of the RAID group this LUN is bound to.", "RG Disk Bytes read Pos 7", ENG, "rgReadBlocksPosition7"},
   {"rgReadPosition8", "Bytes read from the disk in position 8 of the RAID group this LUN is bound to.", "RG Disk Bytes read Pos 8", ENG, "rgReadBlocksPosition8"},
   {"rgReadPosition9", "Bytes read from the disk in position 9 of the RAID group this LUN is bound to.", "RG Disk Bytes read Pos 9", ENG, "rgReadBlocksPosition9"},
   {"rgReadPosition10", "Bytes read from the disk in position 10 of the RAID group this LUN is bound to.", "RG Disk Bytes read Pos 10", ENG, "rgReadBlocksPosition10"},
   {"rgReadPosition11", "Bytes read from the disk in position 11 of the RAID group this LUN is bound to.", "RG Disk Bytes read Pos 11", ENG, "rgReadBlocksPosition11"},
   {"rgReadPosition12", "Bytes read from the disk in position 12 of the RAID group this LUN is bound to.", "RG Disk Bytes read Pos 12", ENG, "rgReadBlocksPosition12"},
   {"rgReadPosition13", "Bytes read from the disk in position 13 of the RAID group this LUN is bound to.", "RG Disk Bytes read Pos 13", ENG, "rgReadBlocksPosition13"},
   {"rgReadPosition14", "Bytes read from the disk in position 14 of the RAID group this LUN is bound to.", "RG Disk Bytes read Pos 14", ENG, "rgReadBlocksPosition14"},
   {"rgReadPosition15", "Bytes read from the disk in position 15 of the RAID group this LUN is bound to.", "RG Disk Bytes read Pos 15", ENG, "rgReadBlocksPosition15"},
   {"rgWritePosition0", "Bytes written to the disk in position 0 of the RAID group this LUN is bound to.", "RG Disk Bytes Written Pos 0", ENG, "rgWriteBlocksPosition0"},
   {"rgWritePosition1", "Bytes written to the disk in position 1 of the RAID group this LUN is bound to.", "RG Disk Bytes Written Pos 1", ENG, "rgWriteBlocksPosition1"},
   {"rgWritePosition2", "Bytes written to the disk in position 2 of the RAID group this LUN is bound to.", "RG Disk Bytes Written Pos 2", ENG, "rgWriteBlocksPosition2"},
   {"rgWritePosition3", "Bytes written to the disk in position 3 of the RAID group this LUN is bound to.", "RG Disk Bytes Written Pos 3", ENG, "rgWriteBlocksPosition3"},
   {"rgWritePosition4", "Bytes written to the disk in position 4 of the RAID group this LUN is bound to.", "RG Disk Bytes Written Pos 4", ENG, "rgWriteBlocksPosition4"},
   {"rgWritePosition5", "Bytes written to the disk in position 5 of the RAID group this LUN is bound to.", "RG Disk Bytes Written Pos 5", ENG, "rgWriteBlocksPosition5"},
   {"rgWritePosition6", "Bytes written to the disk in position 6 of the RAID group this LUN is bound to.", "RG Disk Bytes Written Pos 6", ENG, "rgWriteBlocksPosition6"},
   {"rgWritePosition7", "Bytes written to the disk in position 7 of the RAID group this LUN is bound to.", "RG Disk Bytes Written Pos 7", ENG, "rgWriteBlocksPosition7"},
   {"rgWritePosition8", "Bytes written to the disk in position 8 of the RAID group this LUN is bound to.", "RG Disk Bytes Written Pos 8", ENG, "rgWriteBlocksPosition8"},
   {"rgWritePosition9", "Bytes written to the disk in position 9 of the RAID group this LUN is bound to.", "RG Disk Bytes Written Pos 9", ENG, "rgWriteBlocksPosition9"},
   {"rgWritePosition10", "Bytes written to the disk in position 10 of the RAID group this LUN is bound to.", "RG Disk Bytes Written Pos 10", ENG, "rgWriteBlocksPosition10"},
   {"rgWritePosition11", "Bytes written to the disk in position 11 of the RAID group this LUN is bound to.", "RG Disk Bytes Written Pos 11", ENG, "rgWriteBlocksPosition11"},
   {"rgWritePosition12", "Bytes written to the disk in position 12 of the RAID group this LUN is bound to.", "RG Disk Bytes Written Pos 12", ENG, "rgWriteBlocksPosition12"},
   {"rgWritePosition13", "Bytes written to the disk in position 13 of the RAID group this LUN is bound to.", "RG Disk Bytes Written Pos 13", ENG, "rgWriteBlocksPosition13"},
   {"rgWritePosition14", "Bytes written to the disk in position 14 of the RAID group this LUN is bound to.", "RG Disk Bytes Written Pos 14", ENG, "rgWriteBlocksPosition14"},
   {"rgWritePosition15", "Bytes written to the disk in position 15 of the RAID group this LUN is bound to.", "RG Disk Bytes Written Pos 15", ENG, "rgWriteBlocksPosition15"},
};

static fbe_obs_byte_conversion_stat_t disk_byte_conversion[] =
{
   {"readBytes", "Number of bytes read from this disk", "Read", PRODUCT, "readBlocks"},
   {"writeBytes", "Number of bytes written to this disk", "Written", PRODUCT, "writeBlocks"},
   {"seeked", "Total bytes seeked or this disk", "Sum Bytes Seeked", ENG, "blocksSeeked"},
};

//statlists for reads and writes
static const char* disk_reads_stat_list[] = {"core.*.reads"};
static const char* disk_writes_stat_list[] = {"core.*.writes"};
#endif
     
/************************************************************************************************************************************/
void fbe_perfstats_trace(fbe_trace_level_t trace_level, const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized(&fbe_perfstats_service)) {
        service_trace_level = fbe_base_service_get_trace_level(&fbe_perfstats_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_PERFSTATS,
                     trace_level,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     fmt, 
                     args);
    va_end(args);
}

static fbe_status_t 
fbe_perfstats_init(fbe_packet_t * packet)
{
    fbe_u32_t index = 0;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
#ifdef C4_INTEGRATED
    int obs_status = STATS_BAD_BEHAVIOR;
    
    COUNTER * block_size_fact;
    COUNTER * core_count_fact;
    META  * meta;
    META  * count_meta;
    META  * time_meta;
    META  * percent_meta;  
    FIELD * field;
#endif
    fbe_perfstats_trace(FBE_TRACE_LEVEL_INFO, "Initing Perfstats service for package: 0x%x", packet->package_id);
    FBE_ASSERT_AT_COMPILE_TIME(PERFSTATS_MAX_SEP_OBJECTS > PERFSTATS_MAX_PHYSICAL_OBJECTS);

    for(index = 0; index < PERFSTATS_MAX_SEP_OBJECTS; index++) 
    {
        perfstats_object_map[index] = FBE_OBJECT_ID_INVALID;
    }

    //init spinlock
    fbe_spinlock_init(&map_lock);

    //init service
    fbe_base_service_init(&fbe_perfstats_service);

    //allocate memory for perfstats and init container
    if (packet->package_id == FBE_PACKAGE_ID_SEP_0)
    {
        status = perfstats_allocate_perfstats_memory(FBE_PACKAGE_ID_SEP_0, (void**) &sep_container);
        if (status != FBE_STATUS_OK) 
        {
            fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "Error during perfstats init - SEP allocation\n");
        }
        else if (sep_container)
        {
            sep_container->valid_object_count = 0;
        }

        //obs
#ifdef C4_INTEGRATED
        root = getRootFamily("storage");

        //create generic META that we're going to reuse
        meta = createMeta("",
                           KSCALE_UNO,
                           5,
                           WRAP_W64);

        //create the block size counter
        obs_status = createReferenceFact(&block_size_fact,
                                            root,
                                            "blockSize",
                                            "Block size",
                                            &blockSize,
                                            PRODUCT,
                                            "Block Size",
                                            meta,
                                            U_METER_64);

        if (obs_status != STATS_SUCCESS) {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createReferenceCounter OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        //set coreCount 
        coreCount = csx_p_get_processor_count();
        obs_status = createReferenceFact(&core_count_fact,
                                            root,
                                            "coreCount",
                                            "Number of cores",
                                            &coreCount,
                                            ENG,
                                            "Core Count",
                                            meta,
                                            U_METER_64);

        if (obs_status != STATS_SUCCESS) {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createReferenceCounter OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        //create lun set/stype
        obs_status = createSType(&object_stype,
                                 "lun_stype",
                                 sizeof(fbe_lun_performance_counters_t),
                                 NULL,
                                 0);

        if (obs_status != STATS_SUCCESS) 
        {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createSType (LUN): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        obs_status = createSet(&object_set,
                               root,
                               "flu",
                               "MCR LUN objects (FLU)",
                               ENG,
                               "FLU",
                               32,
                               fbe_obs_lun_enc_func,
                               "FLU",
                               object_stype,
                               NORESOLVER);

        if (obs_status != STATS_SUCCESS) 
        {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createSet (LUN): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        
        //create core set
        obs_status = createSType(&core_stype,
                                 "lun_core_stype",
                                 sizeof(fbe_lun_performance_counters_t),
                                 lun_fields,
                                 sizeof(lun_fields)/sizeof(StatField));

        if (obs_status != STATS_SUCCESS) 
        {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createSType (Cores): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        obs_status = createSet(&core_set,
                               (FAMILY *)object_stype,
                               "core",
                               "Per-core stats",
                               ENG,
                               "Core",
                               16,
                               fbe_obs_lun_core_enc_func,
                               "core",
                               core_stype,
                               NORESOLVER);

        if (obs_status != STATS_SUCCESS) 
        {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createSet (Lun core): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        //computed stats - add "manually"
        for (fbe_u32_t i=0; i <sizeof(lun_computed_fields)/sizeof(fbe_obs_computed_stat_t); i++) 
        {
        
           obs_status = createComputedFieldStat(&field,
                                               object_stype,
                                               lun_computed_fields[i].name,
                                               lun_computed_fields[i].summary,
                                               lun_computed_fields[i].statList,
                                               1,
                                               lun_computed_fields[i].op,
                                               lun_computed_fields[i].vis,
                                               lun_computed_fields[i].title,
                                               meta,
                                               COUNTER_BEHAVIOR);
           if (obs_status != STATS_SUCCESS) 
           {
              fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, " computedStat(LUN): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
           }
        }

        meta = createMeta("B",
                           KSCALE_KIBI,
                           5,
                           WRAP_W64);
        //byte conversions, same as computed stats. Create for both core and lun families
        for (fbe_u32_t i=0; i <sizeof(lun_byte_conversion)/sizeof(fbe_obs_byte_conversion_stat_t); i++) 
        {
        
           obs_status = createComputed2FieldStat(&field,
                                                 object_stype,
                                                 lun_byte_conversion[i].name,
                                                 lun_byte_conversion[i].summary,
                                                 lun_byte_conversion[i].statName,
                                                 "storage.blockSize",
                                                 MULT,
                                                 lun_byte_conversion[i].vis,
                                                 lun_byte_conversion[i].title,
                                                 meta,
                                                 COUNTER_BEHAVIOR);
           if (obs_status != STATS_SUCCESS) 
           {
              fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, " computedStat(LUN byte conversion): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
           }
           obs_status = createComputed2FieldStat(&field,
                                                 core_stype,
                                                 lun_byte_conversion[i].name,
                                                 lun_byte_conversion[i].summary,
                                                 lun_byte_conversion[i].statName,
                                                 "storage.blockSize",
                                                 MULT,
                                                 lun_byte_conversion[i].vis,
                                                 lun_byte_conversion[i].title,
                                                 meta,
                                                 COUNTER_BEHAVIOR);
           if (obs_status != STATS_SUCCESS) 
           {
              fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, " computedStat(LUN byte conversion): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
           }
        }
        fbe_perfstats_trace(FBE_TRACE_LEVEL_INFO, "SEP OBS Registration complete.\n");
#endif
    }
    else if (packet->package_id == FBE_PACKAGE_ID_PHYSICAL)
    {
        status = perfstats_allocate_perfstats_memory(FBE_PACKAGE_ID_PHYSICAL, (void**) &physical_container);
        if (status != FBE_STATUS_OK) 
        {
            fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "Error during perfstats init - PP allocation\n");
        }
        else if (physical_container)
        {          
            physical_container->valid_object_count = 0;        
        }
#ifdef C4_INTEGRATED
        
        //physical root
		root = getRootFamily("physical");
		if (NULL == root) {
			fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "getRootFamily: OBS API error: No root family name of physical\n"); 
		}

        //create generic META that we're going to reuse
        meta = createMeta("",
                           KSCALE_UNO,
                           5,
                           WRAP_W64);

        //create the block size counter
        obs_status = createReferenceFact(&block_size_fact,
                                            root,
                                            "blockSize",
                                            "Block size",
                                            &blockSize,
                                            PRODUCT,
                                            "Block Size",
                                            meta,
                                            U_METER_64);
        if (obs_status != STATS_SUCCESS) 
        {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createReferencFact (PDO): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        //set coreCount 
        coreCount = csx_p_get_processor_count();
        obs_status = createReferenceFact(&core_count_fact,
                                            root,
                                            "coreCount",
                                            "Number of cores",
                                            &coreCount,
                                            PRODUCT,
                                            "Core Count",
                                            meta,
                                            U_METER_64);
        if (obs_status != STATS_SUCCESS) 
        {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createReferencFact (PDO): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }


        obs_status = createSType(&object_stype,
                                 "pdo_stype",
                                 sizeof(fbe_pdo_performance_counters_t),
                                 NULL,
                                 0);

        if (obs_status != STATS_SUCCESS) 
        {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createSType (PDO): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        obs_status = createSet(&object_set,
                               root,
                               "disk",
                               "MCR disk objects (PDO)",
                               PRODUCT,
                               "disk",
                               16,
                               fbe_obs_disk_enc_func,
                               "disk",
                               object_stype,
                               NORESOLVER);

        if (obs_status != STATS_SUCCESS) 
        {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createSet (LUN): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        
        //create core set
        obs_status = createSType(&core_stype,
                                 "pdo_core_stype",
                                 sizeof(fbe_pdo_performance_counters_t),
                                 disk_fields,
                                 sizeof(disk_fields)/sizeof(StatField));

        if (obs_status != STATS_SUCCESS) 
        {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createSType (Cores-PDO): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }


        obs_status = createSet(&core_set,
                               (FAMILY *)object_stype,
                               "core",
                               "Per-core stats",
                               ENG,
                               "core",
                               16,
                               fbe_obs_disk_core_enc_func,
                               "core",
                               core_stype,
                               NORESOLVER);

        if (obs_status != STATS_SUCCESS) 
        {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createSet (Cores): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        //computed stats
        for (fbe_u32_t i=0; i <sizeof(disk_computed_fields)/sizeof(fbe_obs_computed_stat_t); i++) 
        {
        
           obs_status = createComputedFieldStat(&field,
                                               object_stype,
                                               disk_computed_fields[i].name,
                                               disk_computed_fields[i].summary,
                                               disk_computed_fields[i].statList,
                                               1,
                                               disk_computed_fields[i].op,
                                               disk_computed_fields[i].vis,
                                               disk_computed_fields[i].title,
                                               meta,
                                               COUNTER_BEHAVIOR);
           if (obs_status != STATS_SUCCESS) 
           {
              fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, " computedStat(LUN): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
           }
        }

        meta = createMeta("B",
                           KSCALE_KIBI,
                           5,
                           WRAP_W64);

        time_meta = createMeta("us",
                               KSCALE_UNO,
                               5,
                               WRAP_W64);

        count_meta = createMeta("IO",
                                KSCALE_UNO,
                                5,
                                WRAP_W64);

        percent_meta = createMeta("%",
                                  KSCALE_UNO,
                                  5,
                                  WRAP_W64);

        //reads and writes, so they use the correct META from a user perspective
        obs_status = createComputedFieldStat(&field,
                                               object_stype,
                                               "reads",
                                               "Reads from this disk",
                                               disk_reads_stat_list,
                                               1,
                                               SUM,
                                               PRODUCT,
                                               "Reads",
                                               count_meta,
                                               COUNTER_BEHAVIOR);
        if (obs_status != STATS_SUCCESS) 
        {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, " computedStat(disk): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        obs_status = createComputedFieldStat(&field,
                                               object_stype,
                                               "writes",
                                               "Writes to this disk",
                                               disk_writes_stat_list,
                                               1,
                                               SUM,
                                               PRODUCT,
                                               "Writes",
                                               count_meta,
                                               COUNTER_BEHAVIOR);
        if (obs_status != STATS_SUCCESS) 
        {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, " computedStat(disk): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }
     
        //byte conversions, same as computed stats. Create for both core and lun families
        for (fbe_u32_t i=0; i <sizeof(disk_byte_conversion)/sizeof(fbe_obs_byte_conversion_stat_t); i++) 
        {
        
           obs_status = createComputed2FieldStat(&field,
                                                 object_stype,
                                                 disk_byte_conversion[i].name,
                                                 disk_byte_conversion[i].summary,
                                                 disk_byte_conversion[i].statName,
                                                 "physical.blockSize",
                                                 MULT,
                                                 disk_byte_conversion[i].vis,
                                                 disk_byte_conversion[i].title,
                                                 meta,
                                                 COUNTER_BEHAVIOR);
           if (obs_status != STATS_SUCCESS) 
           {
              fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, " computedStat(DISK byte conversion): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
           }
           obs_status = createComputed2FieldStat(&field,
                                                 core_stype,
                                                 disk_byte_conversion[i].name,
                                                 disk_byte_conversion[i].summary,
                                                 disk_byte_conversion[i].statName,
                                                 "physical.blockSize",
                                                 MULT,
                                                 ENG,
                                                 disk_byte_conversion[i].title,
                                                 meta,
                                                 COUNTER_BEHAVIOR);
           if (obs_status != STATS_SUCCESS) 
           {
              fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, " computedStat(DISK byte conversion): OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
           }
        }
        
        //special nested computed formulas
        //total calls
        obs_status = createComputed2FieldStat(&field,
                                               object_stype,
                                              "totalCalls",
                                              "Total Reads and Writes for this disk",
                                              "reads",
                                              "writes", 
                                               ADD,
                                               PRODUCT,
                                              "Total Calls",
                                               count_meta,
                                               COUNTER_BEHAVIOR);

        if (obs_status != STATS_SUCCESS) {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createFieldStat: OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        //total ticks
        obs_status = createComputed2FieldStat(&field,
                                               object_stype,
                                              "totalTicks",
                                              "Total Reads and Writes for this disk",
                                              "busyTicks",
                                              "idleTicks", 
                                               ADD,
                                               ENG,
                                              "Total Ticks",
                                               time_meta,
                                               ERROR_COUNTER_BEHAVIOR);

        if (obs_status != STATS_SUCCESS) {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createFieldStat: OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        //utilization %
        obs_status = createComputed2FieldStat(&field,
                                               object_stype,
                                              "utilization",
                                              "Utilization % for this disk",
                                              "busyTicks",
                                              "totalTicks", 
                                               PCT,
                                               PRODUCT,
                                              "Utilization %",
                                               percent_meta,
                                               FACT_BEHAVIOR);

        if (obs_status != STATS_SUCCESS) {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createFieldStat: OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        //average Queue Length, intermedite value
        obs_status = createComputed2FieldStat(&field,
                                               object_stype,
                                              "averageQueueLengthIntermediate",
                                              "Average Queue Length for this disk",
                                              "sumArrivalQueueLength",
                                              "totalCalls", 
                                               DIV,
                                               ENG,
                                              "Avg Queue Length",
                                               count_meta,
                                               FACT_BEHAVIOR);

        if (obs_status != STATS_SUCCESS) {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createFieldStat: OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        //busyTicksTimesQueueLength (intermedate)
        obs_status = createComputed2FieldStat(&field,
                                               object_stype,
                                              "busyTicksTimesQueueLength",
                                              "Busy ticks times the sum queue length for this disk. This is an intermediate stat used to calculate utilization" ,
                                              "busyTicks",
                                              "averageQueueLengthIntermediate", 
                                               MULT,
                                               ENG,
                                              "BT x Average Queue Length",
                                               count_meta,
                                               FACT_BEHAVIOR);

        if (obs_status != STATS_SUCCESS) {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createFieldStat: OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        //average response time, intermediate value
        obs_status = createComputed2FieldStat(&field,
                                               object_stype,
                                              "responseTimeIntermediate",
                                              "Average response time for this disk, measured in microseconds. This measures the average time it takes for a disk request to complete.",
                                              "busyTicksTimesQueueLength",
                                              "totalCalls", 
                                               DIV,
                                               ENG,
                                              "Average Response Time",
                                               time_meta,
                                               FACT_BEHAVIOR);

        if (obs_status != STATS_SUCCESS) {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createFieldStat: OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        //average service time, intermediate value
        obs_status = createComputed2FieldStat(&field,
                                               object_stype,
                                              "serviceTimeIntermediate",
                                              "Average service time for this disk, measured in microseconds. This measures the average time the disk spends on each individual request.",
                                              "busyTicks",
                                              "totalCalls", 
                                               DIV,
                                               ENG,
                                              "Average Service Time",
                                               time_meta,
                                               FACT_BEHAVIOR);

        if (obs_status != STATS_SUCCESS) {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createFieldStat: OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }
        
        //average Queue Length, intermedite value
        obs_status = createComputed2FieldStat(&field,
                                               object_stype,
                                              "averageQueueLength",
                                              "Average Queue Length for this disk",
                                              "averageQueueLengthIntermediate",
                                              "physical.coreCount", 
                                               DIV,
                                               PRODUCT,
                                              "Avg Queue Length",
                                               count_meta,
                                               FACT_BEHAVIOR);

        if (obs_status != STATS_SUCCESS) {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createFieldStat: OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        } 

        //average response time, intermediate value
        obs_status = createComputed2FieldStat(&field,
                                               object_stype,
                                              "responseTime",
                                              "Average response time for this disk, measured in microseconds. This measures the average time it takes for a disk request to complete.",
                                              "responseTimeIntermediate",
                                              "physical.coreCount", 
                                               DIV,
                                               PRODUCT,
                                              "Average Response Time",
                                               time_meta,
                                               FACT_BEHAVIOR);

        if (obs_status != STATS_SUCCESS) {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createFieldStat: OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }

        //average service time, intermediate value
        obs_status = createComputed2FieldStat(&field,
                                               object_stype,
                                              "serviceTime",
                                              "Average service time for this disk, measured in microseconds. This measures the average time the disk spends on each individual request.",
                                              "serviceTimeIntermediate",
                                              "physical.coreCount", 
                                               DIV,
                                               PRODUCT,
                                              "Average Service Time",
                                               time_meta,
                                               FACT_BEHAVIOR);

        if (obs_status != STATS_SUCCESS) {
           fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "createFieldStat: OBS API error: %d - %s\n", obs_status, getCAPIErrorMsg(obs_status));
        }
           
        fbe_perfstats_trace(FBE_TRACE_LEVEL_INFO, "Physical Package OBS Registration complete.\n");
#endif 
    }   

    if (status != FBE_STATUS_OK) {
        fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "Error during perfstats init - Invalid package or allocation error\n");
    }

    //complete packet
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return status;
}

static fbe_status_t 
fbe_perfstats_destroy(fbe_packet_t * packet)
{   
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_service_destroy(&fbe_perfstats_service);
    fbe_spinlock_destroy(&map_lock);

    if (packet->package_id == FBE_PACKAGE_ID_SEP_0 && sep_container)
    {
        status = perfstats_deallocate_perfstats_memory(FBE_PACKAGE_ID_SEP_0, (void**) &sep_container);
    }
    else if (packet->package_id == FBE_PACKAGE_ID_PHYSICAL && physical_container)
    {
        status = perfstats_deallocate_perfstats_memory(FBE_PACKAGE_ID_PHYSICAL, (void**) &physical_container);
    }

    if (status != FBE_STATUS_OK) {
        fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "Error during perfstats destroy\n");
    }



    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return status;
}

fbe_status_t 
fbe_perfstats_control_entry(fbe_packet_t * packet)
{    
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;

    control_code = fbe_transport_get_control_code(packet);
    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        status = fbe_perfstats_init(packet);
        return status;
    }

    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_DESTROY) {
        status = fbe_perfstats_destroy(packet);
        return status;
    }

    if(!fbe_base_service_is_initialized(&fbe_perfstats_service)){
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    switch(control_code) {
        case FBE_PERFSTATS_CONTROL_CODE_GET_STATS_CONTAINER:
            status = fbe_perfstats_get_mapped_stats_container(packet);
            break;
        case FBE_PERFSTATS_CONTROL_CODE_ZERO_STATS:
            status = fbe_perfstats_zero_stats(packet);
            break;
        case FBE_PERFSTATS_CONTROL_CODE_GET_OFFSET:
            status = fbe_perfstats_get_offset(packet);
            break;
        case FBE_PERFSTATS_CONTROL_CODE_IS_COLLECTION_ENABLED:
            status = fbe_perfstats_get_enabled_state(packet);
            break;
        case FBE_PERFSTATS_CONTROL_CODE_SET_ENABLED:
            status = fbe_perfstats_set_enabled_state(packet);
            break;
        default:
            status = fbe_base_service_control_entry(&fbe_perfstats_service, packet);
            break;
    }

    return status;
}

static fbe_status_t fbe_perfstats_get_mapped_stats_container(fbe_packet_t *packet){
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_u64_t                           *get_ptr;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &get_ptr); 

    perfstats_get_mapped_stats_container(get_ptr);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_perfstats_zero_stats(fbe_packet_t *packet){
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_u32_t                           obj_i = 0;
    fbe_u32_t                           cpu_i = 0;
    fbe_u32_t                           pos_i = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    if(packet->package_id == FBE_PACKAGE_ID_SEP_0 && sep_container)
    {
        for (obj_i = 0; obj_i < PERFSTATS_MAX_SEP_OBJECTS; obj_i++) 
        {   //zero everything but object ID and set timestamp to current time
            for (cpu_i = 0; cpu_i < PERFSTATS_CORES_SUPPORTED; cpu_i++)
            {
                sep_container->lun_stats[obj_i].cumulative_read_response_time[cpu_i] = 0;
                sep_container->lun_stats[obj_i].cumulative_write_response_time[cpu_i] = 0;
                sep_container->lun_stats[obj_i].lun_blocks_read[cpu_i] = 0;
                sep_container->lun_stats[obj_i].lun_blocks_written[cpu_i] = 0;
                sep_container->lun_stats[obj_i].lun_read_requests[cpu_i] = 0;
                sep_container->lun_stats[obj_i].lun_write_requests[cpu_i] = 0;
                sep_container->lun_stats[obj_i].non_zero_queue_arrivals[cpu_i] = 0;
                sep_container->lun_stats[obj_i].stripe_crossings[cpu_i] = 0;
                sep_container->lun_stats[obj_i].stripe_writes[cpu_i] = 0;
                sep_container->lun_stats[obj_i].sum_arrival_queue_length[cpu_i] = 0;

                //histograms
                for (pos_i = 0; pos_i < FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST; pos_i++) 
                {
                    sep_container->lun_stats[obj_i].lun_io_size_read_histogram[pos_i][cpu_i] = 0;
                    sep_container->lun_stats[obj_i].lun_io_size_write_histogram[pos_i][cpu_i] = 0;
                }

                //disk stats
                for (pos_i = 0; pos_i < FBE_XOR_MAX_FRUS; pos_i++) 
                {
                    sep_container->lun_stats[obj_i].disk_blocks_read[pos_i][cpu_i] = 0;
                    sep_container->lun_stats[obj_i].disk_blocks_written[pos_i][cpu_i] = 0;
                    sep_container->lun_stats[obj_i].disk_reads[pos_i][cpu_i] = 0;
                    sep_container->lun_stats[obj_i].disk_writes[pos_i][cpu_i] = 0;            
                }
            }
            sep_container->lun_stats[obj_i].timestamp = fbe_get_time();
        }

    }
    else if(packet->package_id == FBE_PACKAGE_ID_PHYSICAL && physical_container)
    {
         for (obj_i = 0; obj_i < PERFSTATS_MAX_PHYSICAL_OBJECTS; obj_i++) 
        {   //zero everything but object ID and set timestamp to current time
            for (cpu_i = 0; cpu_i < PERFSTATS_CORES_SUPPORTED; cpu_i++)
            {
                physical_container->pdo_stats[obj_i].busy_ticks[cpu_i] = 0;
                physical_container->pdo_stats[obj_i].idle_ticks[cpu_i] = 0;
                physical_container->pdo_stats[obj_i].disk_blocks_read[cpu_i] = 0;
                physical_container->pdo_stats[obj_i].disk_blocks_written[cpu_i] = 0;
                physical_container->pdo_stats[obj_i].disk_reads[cpu_i] = 0;
                physical_container->pdo_stats[obj_i].disk_writes[cpu_i] = 0;
                physical_container->pdo_stats[obj_i].sum_blocks_seeked[cpu_i] = 0;
                physical_container->pdo_stats[obj_i].sum_arrival_queue_length[cpu_i] = 0;
                physical_container->pdo_stats[obj_i].non_zero_queue_arrivals[cpu_i] = 0;

                //histograms
                for (pos_i = 0; pos_i < FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_LAST; pos_i++) 
                {
                    physical_container->pdo_stats[obj_i].disk_srv_time_histogram[cpu_i][pos_i] = 0;
                }
            }
            physical_container->pdo_stats[obj_i].timestamp = fbe_get_time();
        }
    }
    
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_perfstats_get_offset(fbe_packet_t *packet){
    fbe_payload_ex_t *                          payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_perfstats_service_get_offset_t          *get_offset;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &get_offset); 

    fbe_perfstats_get_offset_for_object_id(get_offset->object_id, &get_offset->offset);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_perfstats_get_enabled_state(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                          payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_bool_t                                  *is_enabled;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &is_enabled); 

    fbe_perfstats_is_collection_enabled_for_package(packet->package_id,
                                                    is_enabled);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_perfstats_set_enabled_state(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                          payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_bool_t                                  *is_enabled;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &is_enabled); 

    if (packet->package_id == FBE_PACKAGE_ID_SEP_0) {
        sep_stats_enabled = *is_enabled;
    }
    else if (packet->package_id == FBE_PACKAGE_ID_PHYSICAL) {
        physical_stats_enabled = *is_enabled;
    }
    else
    {   
        fbe_perfstats_trace(FBE_TRACE_LEVEL_WARNING, "%s: Can't set perfstats collection on unsupported package_id: %d\n", __FUNCTION__, packet->package_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_perfstats_add_object_to_map(fbe_object_id_t object_id, fbe_u32_t *offset){
    fbe_status_t status;
    fbe_u32_t new_offset;

    //linear search for first unoccupied slot
    fbe_spinlock_lock(&map_lock);

    status = fbe_perfstats_get_offset_for_object_id(FBE_OBJECT_ID_INVALID, &new_offset); //searching for invalid object ID returns index of free perfstats slice
    if (status == FBE_STATUS_OK) {
        perfstats_object_map[new_offset] = object_id;
        *offset = new_offset;
    }
    
    fbe_spinlock_unlock(&map_lock);

    return status;
}

static fbe_status_t fbe_perfstats_remove_object_from_map(fbe_object_id_t object_id, fbe_u32_t *offset){
    fbe_status_t status;
    fbe_u32_t deleted_offset;

    fbe_spinlock_lock(&map_lock);
    status = fbe_perfstats_get_offset_for_object_id(object_id, &deleted_offset); 
    if (status == FBE_STATUS_OK) 
    {
        perfstats_object_map[deleted_offset] = FBE_OBJECT_ID_INVALID;
        *offset = deleted_offset;
    }
    fbe_spinlock_unlock(&map_lock);

    return status;
}

static fbe_status_t fbe_perfstats_get_offset_for_object_id(fbe_object_id_t object_id, fbe_u32_t *offset){
    fbe_u32_t index;
    fbe_u32_t range = sep_container ? PERFSTATS_MAX_SEP_OBJECTS : PERFSTATS_MAX_PHYSICAL_OBJECTS;

    //linear search for object ID 
    for (index = 0; index < range; index++)
    {
       if (perfstats_object_map[index] == object_id)
       {
           *offset = index;
           //sanity check to prevent corruption
           if (index >= range) 
           {
              fbe_perfstats_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "Too many objects in Physical Package Perfstats object map\n");
              return FBE_STATUS_GENERIC_FAILURE;
           }
           return FBE_STATUS_OK;
       }
    }

    //if we haven't returned by now, fail out!
    *offset = FBE_MAX_SEP_OBJECTS;
    return FBE_STATUS_GENERIC_FAILURE;
}

static fbe_status_t fbe_perfstats_get_offset_for_lun_number(fbe_lun_number_t lun_number, fbe_u32_t *offset)
{
   fbe_u32_t index;

   if (sep_container && lun_number != FBE_U32_MAX) {
      //linear search for object ID 
      for (index = 0; index < PERFSTATS_MAX_SEP_OBJECTS; index++)
      {
         if (sep_container->lun_stats[index].lun_number == lun_number)
         {
             *offset = index;
             return FBE_STATUS_OK;
         }
      }
   }
   //if we haven't returned by now, fail out!
   *offset = FBE_MAX_SEP_OBJECTS;
   return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t fbe_perfstats_get_system_memory_for_object_id(fbe_object_id_t object_id, fbe_package_id_t package_id, void **ptr){
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t offset = 0;

    if (!sep_container && package_id == FBE_PACKAGE_ID_SEP_0) {
        fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "Perfstats container not initialized for FBE_PACKAGE_ID_SEP_0 \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!physical_container && package_id == FBE_PACKAGE_ID_PHYSICAL) {
        fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "Perfstats container not initialized for package id: FBE_PACKAGE_ID_PHYSICAL\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // return the existing offset if the object already exists in the map
    status = fbe_perfstats_get_offset_for_object_id(object_id, &offset);
    if (offset != FBE_MAX_SEP_OBJECTS) {
        fbe_perfstats_trace(FBE_TRACE_LEVEL_INFO, "Stats pdo id: 0x%x (offset: %d ) already exists\n", object_id, offset);
    } else {
        //try to add to map, and get resulting offset
        status = fbe_perfstats_add_object_to_map(object_id, &offset);
        if (status != FBE_STATUS_OK) {  
            *ptr = NULL;
            fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "Could not add object to perfstats object map for object_id: 0x%x\n", object_id);
            return status;
        }
    }

    if (package_id == FBE_PACKAGE_ID_PHYSICAL) {
        *ptr = &(physical_container->pdo_stats[offset]);
        physical_container->valid_object_count++;
        fbe_perfstats_trace(FBE_TRACE_LEVEL_INFO, "Giving stats ptr: 0x%x to pdo id: 0x%x (offset: %d )\n", (unsigned int)*ptr, object_id, offset);
        return FBE_STATUS_OK;
    }
    if (package_id == FBE_PACKAGE_ID_SEP_0) {
        *ptr = &(sep_container->lun_stats[offset]);
        sep_container->valid_object_count++;
        fbe_perfstats_trace(FBE_TRACE_LEVEL_INFO, "Giving stats ptr: 0x%x to lun id: 0x%x (offset: %d )\n", (unsigned int)*ptr, object_id, offset);
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t fbe_perfstats_delete_system_memory_for_object_id(fbe_object_id_t object_id,
                                                              fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_u32_t    offset;
    status = fbe_perfstats_remove_object_from_map(object_id, &offset);
    if (status == FBE_STATUS_OK) 
    {
        //zero out this slice of perfstats memory if we have a valid container
        if (package_id == FBE_PACKAGE_ID_SEP_0 && sep_container) 
        {
            fbe_zero_memory(&sep_container->lun_stats[offset], sizeof(fbe_lun_performance_counters_t));
            sep_container->lun_stats[offset].object_id = FBE_OBJECT_ID_INVALID;
            sep_container->valid_object_count--;
        }
        else if (package_id == FBE_PACKAGE_ID_PHYSICAL && physical_container)
        {
            fbe_zero_memory(&physical_container->pdo_stats[offset], sizeof(fbe_pdo_performance_counters_t));
            physical_container->pdo_stats[offset].object_id = FBE_OBJECT_ID_INVALID;
            physical_container->valid_object_count--;
        }
    }
    return status;
}

fbe_status_t fbe_perfstats_is_collection_enabled_for_package(fbe_package_id_t package_id,
                                                             fbe_bool_t *is_enabled)
{
    if (package_id == FBE_PACKAGE_ID_SEP_0) {
        *is_enabled = sep_stats_enabled;
    }
    else if (package_id == FBE_PACKAGE_ID_PHYSICAL) {
        *is_enabled = physical_stats_enabled; 
    }
    else
    {
        fbe_perfstats_trace(FBE_TRACE_LEVEL_WARNING, "%s: Can't set perfstats collection on unsupported package_id: %d\n", __FUNCTION__, package_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

#ifdef C4_INTEGRATED
static fbe_u32_t perfstats_lun_get_hist_index(fbe_u32_t num)
{
    fbe_u32_t index;

    for (index = 0; index < FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST; index++)
    {
        if ((num & (0x1 << index)) != 0)
        {
            return (index);
        }
    }
    return FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST - 1;
}

static fbe_u32_t perfstats_disk_get_hist_index(fbe_u32_t num)
{
    fbe_u32_t index;

    for (index = 0; index < FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_LAST; index++)
    {
        if ((num/100 & 0x1 << index) != 0)
        {
            return (index);
        }
    }
    return FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_LAST - 1;
}


/***************************************************************************
 *  perfstats_update_disk_ticks(fbe_u32_t offset)
 ***************************************************************************
 *
 *  Description
 *      This function was designed to let the Kittyhawk Observability poll function
 *      update the busy and idle ticks for a given drive, so that our utilization
 *      and response time metrics could get updated correctly.
 * 
 *      This is not the best place to do this and is very dependnent on our busy/idle ticks implementation
 *      not changing, but for now it's the only way to update the ticks as often as OBS polls. If the
 *      minimum configurable poll interval moves to something more reasonable (i.e. 30 or 60 seconds)
 *      we should revisit how we spoof the ticks for Rockies and apply that logic in the PDO space instead
 *      of allowing the caretaker of the stats data to modify counter content.      
 *    
 ***************************************************************************/
static void inline perfstats_update_disk_ticks(fbe_u32_t offset)
{
   fbe_time_t time;
   time = fbe_get_time_in_us();
   if (physical_container->pdo_stats[offset].timestamp & FBE_PERFSTATS_BUSY_STATE) 
   { //busy, add ticks
      physical_container->pdo_stats[offset].busy_ticks[0] += (time - physical_container->pdo_stats[offset].timestamp);
      physical_container->pdo_stats[offset].timestamp = (time | FBE_PERFSTATS_BUSY_STATE);
   }
   else
   { //idle, add ticks
      physical_container->pdo_stats[offset].idle_ticks[0] += (time - physical_container->pdo_stats[offset].timestamp);
      physical_container->pdo_stats[offset].timestamp = (time & ~FBE_PERFSTATS_BUSY_STATE);
   }
}

void fbe_obs_disk_enc_func(void *params)
{
   fbe_u32_t pos = 0;

   int rc=0;
   char elemName[256]={0};
   int elsec = getElementName(params,elemName,256, 1);

   if(elsec)
   {
      for (pos = 0; pos < PERFSTATS_MAX_PHYSICAL_OBJECTS; pos++) 
      {
         if (physical_container)
         {
            if (physical_container->pdo_stats[pos].object_id == FBE_OBJECT_ID_INVALID || physical_container->pdo_stats[pos].object_id == 0x0)
            {     //we've reached the end of valid stats memory, exit
                  return; 
            }
            //check if serial number matches

            if (!csx_p_strcmp(elemName, physical_container->pdo_stats[pos].serial_number)) 
            {
               //update busy and idle ticks, then update timestamp and re-record state.
               perfstats_update_disk_ticks(pos);
               rc = enumerateAndCallFunction(params,elemName,&physical_container->pdo_stats[pos]);
               return;              
            }
         }
      } //we should have returned within this loop, but just to be safe...
      return;
   }

   //enc Case
   for(pos=0; pos < PERFSTATS_MAX_PHYSICAL_OBJECTS; pos++ )
   {   
      if (physical_container->pdo_stats[pos].object_id != FBE_OBJECT_ID_INVALID && physical_container->pdo_stats[pos].object_id != 0x0) 
      {
         //convert pdo name to char[]
         sprintf(elemName,"%s",physical_container->pdo_stats[pos].serial_number);
         if (elemName[0] != '\0') 
         {
            perfstats_update_disk_ticks(pos);
            rc= enumerateAndCallFunction(params, elemName,&physical_container->pdo_stats[pos]);
            if(!rc)
               return;
         }        
      }
   }
   return;
}

void fbe_obs_disk_core_enc_func(void *params)
{
   fbe_u32_t pos = 0;
   fbe_u32_t core = 0;

   int rc=0;
   char elemName[256]={0};

   //first level index MUST exist
   int elsec = getElementName(params,elemName,256, 1);
   if (elsec == 0)
   {
      return;
   }
   //search for position based on serial number
   for (pos = 0; pos < PERFSTATS_MAX_PHYSICAL_OBJECTS; pos++) 
   {
      if (physical_container)
      {
         if (physical_container->pdo_stats[pos].object_id == FBE_OBJECT_ID_INVALID || physical_container->pdo_stats[pos].object_id == 0x0)
         {     //we've reached the end of valid stats memory, exit
               return; 
         }
         //check if serial number matches (THIS IS REALLY SLOW IN NESTED LOOKUPS!)
         if (!csx_p_strcmp(elemName, physical_container->pdo_stats[pos].serial_number)) 
         {
            //get second level index
            elsec = getElementName(params,elemName,256, 2);

            // LOOKUP Case
            if(elsec)
            {
               core = csx_p_atoi_u32(elemName);
               if (core >= csx_p_get_processor_count()) 
               {
                  return;
               }

               rc = enumerateAndCallFunction(params,elemName,&physical_container->pdo_stats[pos].busy_ticks[core]);
               return;
            }

            //enc Case
            for(core=0; core < csx_p_get_processor_count(); core++ ) //limits namespace at runtime
            {   
             

               sprintf(elemName,"%d",core);               
               rc= enumerateAndCallFunction(params, elemName,&physical_container->pdo_stats[pos].busy_ticks[core]);
               if(!rc)
               {
                  return;
               }
            }
            return;
         }
      }
   } 
   return;
}

void fbe_obs_lun_enc_func(void *params)
{
   fbe_u32_t pos = 0;
   fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
   fbe_lun_number_t lun_number = FBE_U32_MAX;

   int rc=0;
   char elemName[256]={0};
   int elsec = getElementName(params,elemName,256, 1);

   //get offset with perfstats linear search
   // LOOKUP Case
   if(elsec)
   {
      lun_number = csx_p_atoi_u32((char *)elemName);
      status = fbe_perfstats_get_offset_for_lun_number(lun_number, &pos);
      if (status == FBE_STATUS_OK) //match!
      {
         if (sep_container && lun_number != FBE_U32_MAX) 
         {
            rc = enumerateAndCallFunction(params,elemName,&sep_container->lun_stats[pos]);
            if(rc==0)
            {
                return;
            }
         }
      }
     return;
   }

   //enc Case
   for(pos=0; pos < PERFSTATS_MAX_SEP_OBJECTS; pos++ )
   {   
      if (sep_container->lun_stats[pos].object_id != FBE_OBJECT_ID_INVALID && sep_container->lun_stats[pos].object_id != 0x0) 
      {
         //if it still isn't set, move on without calling enumerateAndCall, because we polled at an awkward moment where
         //the perfstats memory is allocated but the lun number hasn't been assigned yet. 
         if (sep_container->lun_stats[pos].lun_number == FBE_U32_MAX)
         {
            continue;
         }
         //convert lun number to char[]
         sprintf(elemName,"%d",sep_container->lun_stats[pos].lun_number);
         rc= enumerateAndCallFunction(params, elemName,&sep_container->lun_stats[pos]);
         if(!rc)
         {
            return;
         }
      }
   }
   return;
}

void fbe_obs_lun_core_enc_func(void *params)
{
   fbe_u32_t pos = 0;
   fbe_u32_t core = 0;
   fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
   fbe_lun_number_t lun_number = FBE_U32_MAX;

   int rc=0;
   char elemName[256]={0};

   //first level index MUST exist
   int elsec = getElementName(params,elemName,256, 1);
   if (elsec == 0)
   {
      return;
   }

   lun_number = csx_p_atoi_u32((char *)&elemName);

   //get second level index
   elsec = getElementName(params,elemName,256, 2);

   //get offset with perfstats search
   status = fbe_perfstats_get_offset_for_lun_number(lun_number, &pos);
   if (status != FBE_STATUS_OK) 
   {
      return;
   }
   // LOOKUP Case
   if(elsec)
   {
      if (sep_container) 
      {
            core = csx_p_atoi_u32(elemName);
            if (core >= csx_p_get_processor_count()) 
            {
               return;
            }
            rc = enumerateAndCallFunction(params,elemName,&sep_container->lun_stats[pos].cumulative_read_response_time[core]);
            return;
      }
      return;
   }

   //enc Case
   for(core=0; core < csx_p_get_processor_count(); core++ ) //limits namespace at runtime
   {   
      if (sep_container->lun_stats[pos].object_id != FBE_OBJECT_ID_INVALID) 
      {
         //convert core number to char[]
         sprintf(elemName,"%d",core);
      
         rc= enumerateAndCallFunction(params, elemName, &sep_container->lun_stats[pos].cumulative_read_response_time[core]);
         if(!rc)
         {
            return;
         }
      }
   }
   return;
}
#endif 



