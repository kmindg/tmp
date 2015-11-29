

#ifndef FBE_PERFSTATS_INTERFACE_H
#define FBE_PERFSTATS_INTERFACE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_package.h"
#include "fbe_xor_api.h"
#include "fbe/fbe_time.h"
#include "fbe/fbe_scsi_interface.h"

/*!*******************************************************************
 * @def PERFSTATS_SHARED_MEMORY_NAME_SEP
 *********************************************************************
 * @brief Memory tag we use to allocate the memory pool for the SEP stats container
 *
 *********************************************************************/
#define PERFSTATS_SHARED_MEMORY_NAME_SEP "FBEPerfstatsMemSEP"

/*!*******************************************************************
 * @def PERFSTATS_SHARED_MEMORY_NAME_PHYSICAL_PACKAGE
 *********************************************************************
 * @brief Memory tag we use to allocate the memory pool for the PP stats container
 *
 *********************************************************************/
#define PERFSTATS_SHARED_MEMORY_NAME_PHYSICAL_PACKAGE "FBEPerfstatsMemPP"

/*!*******************************************************************
 * @def PERFSTATS_CORES_SUPPORTED
 *********************************************************************
 * @brief number of per-SP physical cores we support for the perfstats service
 *
 *********************************************************************/
#define PERFSTATS_CORES_SUPPORTED 20

/*!*******************************************************************
 * @def PERFSTATS_MAX_SEP_OBJECTS
 *********************************************************************
 * @brief number of SEP stat objects (for LUNs) supported by perfstats service
 *
 *********************************************************************/
#define PERFSTATS_MAX_SEP_OBJECTS 8226  //8192 user luns, 34 system luns

/*!*******************************************************************
 * @def PERFSTATS_MAX_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief number of PP stat objects (for PDOs) supported by perfstats service
 *
 *********************************************************************/
#define PERFSTATS_MAX_PHYSICAL_OBJECTS 3000

/*!*******************************************************************
 * @def PERFSTATS_INVALID_OBJECT_OFFSET
 *********************************************************************
 * @brief This value indicates an object offset for an object that doesn't
 * have a slice of perfstats container memory because it's not in the object map.
 *
 *********************************************************************/
#define PERFSTATS_INVALID_OBJECT_OFFSET CSX_MAX(PERFSTATS_MAX_SEP_OBJECTS, PERFSTATS_MAX_PHYSICAL_OBJECTS)

/*!*******************************************************************
 * @enum fbe_perfstats_control_code_t
 *********************************************************************
 * @brief Control codes for usurper commands in the perfstats service.
 *
 *********************************************************************/
typedef enum fbe_perfstats_control_code_e {
    FBE_PERFSTATS_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_PERFSTATS),
    FBE_PERFSTATS_CONTROL_CODE_GET_STATS_CONTAINER, //return mapped pointer to stat container
    FBE_PERFSTATS_CONTROL_CODE_GET_OFFSET, //get slice of system memory for object
    FBE_PERFSTATS_CONTROL_CODE_ZERO_STATS, //zero stat container
    FBE_PERFSTATS_CONTROL_CODE_IS_COLLECTION_ENABLED, //return true if collection enabled for package
    FBE_PERFSTATS_CONTROL_CODE_SET_ENABLED, //set stat enabled state for package
    FBE_PERFSTATS_CONTROL_CODE_LAST
} fbe_perfstats_control_code_t;



//----------------------------------------------------------------
// Define the FBE API Perfstats Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_perfstats_interface_usurper_interface FBE API perfstats Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API perfstats Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

/*!*******************************************************************
 * @enum fbe_perfstats_histogram_bucket_t
 *********************************************************************
 * @brief Defines the various histogram buckets for our lun read and write
 * histograms.
 *
 *********************************************************************/
typedef enum fbe_perfstats_histogram_bucket_s{
    FBE_PERFSTATS_HISTOGRAM_BUCKET_1_BLOCK,
    FBE_PERFSTATS_HISTOGRAM_BUCKET_2_BLOCK,
    FBE_PERFSTATS_HISTOGRAM_BUCKET_4_BLOCK,
    FBE_PERFSTATS_HISTOGRAM_BUCKET_8_BLOCK,
    FBE_PERFSTATS_HISTOGRAM_BUCKET_16_BLOCK,
    FBE_PERFSTATS_HISTOGRAM_BUCKET_32_BLOCK,
    FBE_PERFSTATS_HISTOGRAM_BUCKET_64_BLOCK,
    FBE_PERFSTATS_HISTOGRAM_BUCKET_128_BLOCK,
    FBE_PERFSTATS_HISTOGRAM_BUCKET_256_BLOCK,
    FBE_PERFSTATS_HISTOGRAM_BUCKET_512_BLOCK,
    FBE_PERFSTATS_HISTOGRAM_BUCKET_1024_BLOCK_PLUS_OVERFLOW,
    FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST
} fbe_perfstats_histogram_bucket_t;

/*!*******************************************************************
 * @def PERFSTATS_MAX_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief The minimum service time interval that's recorded in the first
 * histogram bucket.
 *
 *********************************************************************/
#define FBE_PERFSTATS_SRV_TIME_HISTOGRAM_MIN_TIME 100

/*!*******************************************************************
 * @enum fbe_perfstats_srv_time_histogram_bucket_t
 *********************************************************************
 * @brief Defines the various buckets for our pdo service time
 * histograms.
 *
 *********************************************************************/
typedef enum fbe_perfstats_srv_time_histogram_bucket_s{
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_0_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_100_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_200_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_400_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_800_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_1600_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_3200_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_6400_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_12800_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_25600_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_51200_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_102400_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_204800_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_409600_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_819200_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_1638400_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_3276800_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_6553600_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_13107200_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_26214400_US,
    FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_LAST
} fbe_perfstats_srv_time_histogram_bucket_t;

/*!*******************************************************************
 * @struct fbe_lun_performance_counters_t
 *********************************************************************
 * @brief Defines all stats we collect for objects in SEP. This is distributed
 * to LUN objects, but will also track some stats on the RAID group on which it sits.
 *
 *********************************************************************/
typedef struct fbe_lun_performance_counters_s{
    fbe_time_t timestamp;
    fbe_object_id_t object_id;
    fbe_u32_t lun_number;
    fbe_u64_t cumulative_read_response_time[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t cumulative_write_response_time[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t lun_blocks_read[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t lun_blocks_written[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t lun_read_requests[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t lun_write_requests[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t stripe_crossings[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t stripe_writes[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t non_zero_queue_arrivals[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t sum_arrival_queue_length[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t disk_blocks_read[FBE_XOR_MAX_FRUS][PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t disk_blocks_written[FBE_XOR_MAX_FRUS][PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t disk_reads[FBE_XOR_MAX_FRUS][PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t disk_writes[FBE_XOR_MAX_FRUS][PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t lun_io_size_read_histogram[FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST][PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t lun_io_size_write_histogram[FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST][PERFSTATS_CORES_SUPPORTED];
}fbe_lun_performance_counters_t;

/*!*******************************************************************
 * @struct fbe_pdo_performance_counters_t
 *********************************************************************
 * @brief Defines all stats we collect for objects in Physical. This is distributed
 * to SAS PDO objects.
 *
 *********************************************************************/
typedef struct fbe_pdo_performance_counters_s{
    fbe_time_t timestamp;
    fbe_object_id_t object_id;
    fbe_u64_t busy_ticks[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t idle_ticks[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t non_zero_queue_arrivals[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t sum_arrival_queue_length[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t disk_blocks_read[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t disk_blocks_written[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t disk_reads[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t disk_writes[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t sum_blocks_seeked[PERFSTATS_CORES_SUPPORTED];
    fbe_u64_t disk_srv_time_histogram[FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_LAST][PERFSTATS_CORES_SUPPORTED];
    fbe_time_t last_monitor_timestamp;
    //serial number should always be last
    fbe_u8_t  serial_number[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+4];
}fbe_pdo_performance_counters_t;
                                 
#define FBE_PERFSTATS_BUSY_STATE 0x0000000000000001ULL

/*!*******************************************************************
 * @struct fbe_perfstats_lun_sum_t
 *********************************************************************
 * @brief Struct to hold summed LUN/RG perfstats data.
 *********************************************************************/
typedef struct fbe_perfstats_lun_sum_s{
    fbe_time_t timestamp;
    fbe_object_id_t object_id;
    fbe_u64_t cumulative_read_response_time;
    fbe_u64_t cumulative_write_response_time;
    fbe_u64_t lun_blocks_read;
    fbe_u64_t lun_blocks_written;
    fbe_u64_t lun_read_requests;
    fbe_u64_t lun_write_requests;
    fbe_u64_t stripe_crossings;
    fbe_u64_t stripe_writes; //MR3 writes
    fbe_u64_t non_zero_queue_arrivals;
    fbe_u64_t sum_arrival_queue_length;
    fbe_u64_t disk_blocks_read[FBE_XOR_MAX_FRUS];
    fbe_u64_t disk_blocks_written[FBE_XOR_MAX_FRUS];
    fbe_u64_t disk_reads[FBE_XOR_MAX_FRUS];
    fbe_u64_t disk_writes[FBE_XOR_MAX_FRUS];
    fbe_u64_t lun_io_size_read_histogram[FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST];
    fbe_u64_t lun_io_size_write_histogram[FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST];
}fbe_perfstats_lun_sum_t;

/*!*******************************************************************
 * @struct fbe_perfstats_pdo_sum_t
 *********************************************************************
 * @brief Struct to hold summed PDO perfstats data.
 *********************************************************************/
typedef struct fbe_perfstats_pdo_sum_s{
    // Last update timestamp
    fbe_time_t timestamp; 
    fbe_time_t last_monitor_timestamp;
    fbe_object_id_t object_id;
    fbe_u64_t busy_ticks;
    fbe_u64_t idle_ticks;
    fbe_u64_t non_zero_queue_arrivals;
    fbe_u64_t sum_arrival_queue_length;
    fbe_u64_t disk_blocks_read;
    fbe_u64_t disk_blocks_written;
    fbe_u64_t disk_reads;
    fbe_u64_t disk_writes;
    fbe_u64_t sum_blocks_seeked;
    fbe_u64_t disk_srv_time_histogram[FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_LAST];
}fbe_perfstats_pdo_sum_t;

/*!*******************************************************************
 * @struct fbe_performance_statistics_t
 *********************************************************************
 * @brief General-purpose container to hold performance statistics data
 * for either LUNs or PDOs.
 *********************************************************************/
typedef struct fbe_performance_statistics_s{
    fbe_time_t timestamp;
    union {
        fbe_lun_performance_counters_t *lun_counters;
        fbe_pdo_performance_counters_t *pdo_counters;
    } counter_ptr;
}fbe_performance_statistics_t;

/*!*******************************************************************
 * @struct fbe_perfstats_sep_container_t
 *********************************************************************
 * @brief Holds the stats data for every object in SEP.
 *********************************************************************/
typedef struct fbe_perfstats_sep_container_s{
    fbe_lun_performance_counters_t lun_stats[PERFSTATS_MAX_SEP_OBJECTS];
    fbe_u32_t                      valid_object_count; 
}fbe_perfstats_sep_container_t;

/*!*******************************************************************
 * @struct fbe_perfstats_physical_package_container_t
 *********************************************************************
 * @brief Holds the stats data for every object in PP.
 *********************************************************************/
typedef struct fbe_perfstats_physical_package_container_s{
    fbe_pdo_performance_counters_t pdo_stats[PERFSTATS_MAX_PHYSICAL_OBJECTS];
    fbe_u32_t                      valid_object_count;  
}fbe_perfstats_physical_package_container_t;

/*!*******************************************************************
 * @struct fbe_perfstats_service_get_object_memory_t
 *********************************************************************
 * @brief
 *  This is the strucutre used for FBE_PERFSTATS_CONTROL_CODE_GET_OFFSET opcode.
 * 
 *  Pass in an object ID to get the offset within the performance struct for
 *  that object;
 *
 * @ingroup fbe_api_perfstats_interface
 *********************************************************************/

typedef struct fbe_perfstats_service_get_offset_s{
    fbe_object_id_t     object_id;
    fbe_u32_t           offset; 
}fbe_perfstats_service_get_offset_t;

/*! @} */ /* end of group fbe_api_perfstats_interface_usurper_interface */


#endif /* FBE_PERFSTATS_INTERFACE_H */

