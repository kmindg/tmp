#ifndef FBE_API_COMMON_INTERFACE_H
#define FBE_API_COMMON_INTERFACE_H

#include "fbe/fbe_limits.h"

#if defined(__cplusplus)
#define FBE_API_CALL __stdcall
#else
#define FBE_API_CALL
#endif

#if defined(__cplusplus) || defined(c_plusplus)
# define FBE_API_CPP_EXPORT_START   extern "C" {
# define FBE_API_CPP_EXPORT_END }
#else
# define FBE_API_CPP_EXPORT_START
# define FBE_API_CPP_EXPORT_END
#endif

/*!********************************************************************* 
 * @def FBE_API_ENCLOSURES_PER_BUS
 *  
 * @brief 
 *   This represents the default number of bus on the enclosure.
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
#define FBE_API_ENCLOSURES_PER_BUS  FBE_ENCLOSURES_PER_BUS

/*!********************************************************************* 
 * @def FBE_API_ENCLOSURE_SLOTS
 *  
 * @brief 
 *   This represents the default number of slots per enclosure.
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
#define FBE_API_ENCLOSURE_SLOTS     FBE_ENCLOSURE_SLOTS

/*!********************************************************************* 
 * @def FBE_API_PHYSICAL_BUS_COUNT
 *  
 * @brief 
 *   This represents the default number of bus.
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
#define FBE_API_PHYSICAL_BUS_COUNT  FBE_PHYSICAL_BUS_COUNT

/*!********************************************************************* 
 * @def FBE_API_ENCLOSURE_COUNT
 *  
 * @brief 
 *   This represents the default number of enclosures.
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
#define FBE_API_ENCLOSURE_COUNT     (FBE_API_PHYSICAL_BUS_COUNT * FBE_API_ENCLOSURES_PER_BUS)

/*!********************************************************************* 
 * @def FBE_API_MAX_ALLOC_ENCL_PER_BUS
 *  
 * @brief 
 *   This represents the default maximum number of enclosure per bus
 *   allocated.
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
#define FBE_API_MAX_ALLOC_ENCL_PER_BUS  FBE_API_ENCLOSURES_PER_BUS + 1

/*!********************************************************************* 
 * @def FBE_API_MAX_ENCL_COMPONENTS
 *  
 * @brief 
 *   This represents the default maximum number of enclosure per bus
 *   allocated.
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
// two to skip over index values 0 and 1. 2 and 3 will be unused and then one for each edge expander on the Voyager.
// two to skip over index values 0 and 1. 6 and 7 will be unused and then one for DRVSXP on the Viking.
#define FBE_API_MAX_ENCL_COMPONENTS 8 
/*!********************************************************************* 
 * @def FBE_API_DRIVE_FORMAT
 *  
 * @brief 
 *   This represents the drive format
 *   allocated.
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
#define FBE_API_VOYAGER_BANK_WIDTH  12 //Used for printing the drive status in a format
#define FBE_API_CAYENNE_BANK_WIDTH  12 //Used for printing the drive status in a format
#define FBE_API_VIKING_BANK_WIDTH   20 //viking bank width
#define FBE_API_NAGA_BANK_WIDTH     30 //viking bank width

/*!********************************************************************* 
 * @def FBE_API_MAX_DRIVE_EE
 *  
 * @brief 
 *   This represents the default maximum number of drive EE
 *   allocated.
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
#define FBE_API_MAX_DRIVE_EE 30
#define FBE_API_MAX_DRIVE_CAYENNE 60
#define FBE_API_MAX_DRIVE_VIKING 120
#define FBE_API_MAX_DRIVE_NAGA 120

/*!********************************************************************* 
 * @def IO_AND_CONTROL_ENTRY_MAGIC_NUMBER
 *  
 * @brief 
 *   This represents the I/O and control entry magic number.
 *   allocated.
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
#define IO_AND_CONTROL_ENTRY_MAGIC_NUMBER   0x0FBE0001

/*!********************************************************************* 
 * @def FBE_API_ENUM_TO_STR
 *  
 * @brief 
 *   This changes the enum to the string
 *   allocated.
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
#define FBE_API_ENUM_TO_STR(_enum){_enum, #_enum}

/*!********************************************************************* 
 * @def fbe_common_cache_status_e
 *  
 * @brief
 *  Enumeration of Cache HW status.
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
typedef enum fbe_common_cache_status_e
{
    FBE_CACHE_STATUS_OK = 0,            // no problems
    FBE_CACHE_STATUS_DEGRADED,          // some problems, but not critical
    FBE_CACHE_STATUS_FAILED,            // problems that require cache disable
    FBE_CACHE_STATUS_FAILED_SHUTDOWN,   // problems that require cache dump & shutdown
    FBE_CACHE_STATUS_FAILED_ENV_FLT,            // problems that require cache disable are caused by Env Interface Faults
    FBE_CACHE_STATUS_FAILED_SHUTDOWN_ENV_FLT,   // problems that require cache dump & shutdown are caused by Env Interface Faults
    FBE_CACHE_STATUS_APPROACHING_OVER_TEMP,     // OverTemp condition with Shutdown coming if not addressed
    FBE_CACHE_STATUS_UNINITIALIZE,      // cache status is not initialized
} fbe_common_cache_status_t;

/*!********************************************************************* 
 * @def fbe_common_cache_status_info_s
 *  
 * @brief
 *  Cache related data
 *
 * @ingroup fbe_api_system_interface_usurper_interface
 **********************************************************************/
typedef struct fbe_common_cache_status_info_s
{
    fbe_common_cache_status_t       cacheStatus;
    fbe_u32_t                       batteryTime;            // ***** currently set to zero - need to agree on calculation to use *****
    fbe_bool_t                      ssdFaulted;             // local SP's SSD status
} fbe_common_cache_status_info_t;

#endif /* FBE_API_COMMON_INTERFACE_H */
