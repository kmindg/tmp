#ifndef FBE_API_BVD_INTERFACE_H
#define FBE_API_BVD_INTERFACE_H

/*!**************************************************************************
 * @file fbe_api_bvd_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_api_bvd_interface.
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_bvd_interface.h"
#include "fbe/fbe_sep_shim.h"


FBE_API_CPP_EXPORT_START

//----------------------------------------------------------------
// Define the top level group for the FBE Storage Extent Package APIs
//----------------------------------------------------------------
/*! @defgroup fbe_api_storage_extent_package_class FBE Storage Extent Package APIs
 *  @brief 
 *    This is the set of definitions for FBE Storage Extent Package APIs.
 *
 *  @ingroup fbe_api_storage_extent_package_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API BVD Test Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_bvd_interface_usurper_interface FBE API BVD Test Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API BVD Test Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------


/*! @} */ /* end of group fbe_api_bvd_interface_usurper_interface */

/* Common utilities */

//----------------------------------------------------------------
// Define the group for the FBE API BVD Test Interface.  
// This is where all the function prototypes for the FBE API BVD Test.
//----------------------------------------------------------------
/*! @defgroup fbe_api_bvd_interface FBE API BVD Interface
 *  @brief 
 *    This is the set of FBE API BVD Test Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_bvd_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------

fbe_status_t FBE_API_CALL fbe_api_bvd_interface_get_downstream_attr(fbe_object_id_t bvd_object_id, fbe_volume_attributes_flags* downstream_attr_p, fbe_object_id_t lun_object_id);
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_get_bvd_object_id(fbe_object_id_t *object_id);

fbe_status_t FBE_API_CALL fbe_api_bvd_interface_enable_peformance_statistics(void);
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_disable_peformance_statistics(void);
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_get_peformance_statistics(fbe_sep_shim_get_perf_stat_t *bvd_perf_stats);
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_clear_peformance_statistics(void);

fbe_status_t FBE_API_CALL fbe_api_bvd_interface_enable_async_io(void);
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_disable_async_io(void);

fbe_status_t FBE_API_CALL fbe_api_bvd_interface_enable_async_io_compl(void);
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_disable_async_io_compl(void);
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_set_rq_method(fbe_transport_rq_method_t rq_method);
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_set_alert_time(fbe_u32_t alert_time);

fbe_status_t FBE_API_CALL fbe_api_bvd_interface_enable_group_priority(fbe_bool_t apply_to_pp);
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_disable_group_priority(fbe_bool_t apply_to_pp);

fbe_status_t FBE_API_CALL fbe_api_bvd_interface_shutdown(void);
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_unexport_lun(fbe_object_id_t lun_object_id);
FBE_API_CPP_EXPORT_END
/*! @} */ /* end of group fbe_api_bvd_interface */

//----------------------------------------------------------------
// Define the group for all FBE Storage Extent Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API Storage Extent
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_storage_extent_package_interface_class_files FBE Storage Extent Package APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE Storage Extent Package APIs Interface.
 * 
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------


#endif /* FBE_API_BVD_INTERFACE_H */


