#ifndef FBE_API_VIRTUAL_DRIVE_INTERFACE_H
#define FBE_API_VIRTUAL_DRIVE_INTERFACE_H

/*!**************************************************************************
 * @file fbe_api_virtual_drive_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_api_virtual_drive_interface.
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 * @version
 *   11/05/2009:  Created. guov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_virtual_drive.h"
#include "fbe_spare.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_physical_drive.h"

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
// Define the FBE API Virtual Drive Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_virtual_drive_interface_usurper_interface FBE API Virtual Drive Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Virtual Drive Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

/*!*******************************************************************
 * @struct fbe_api_virtual_drive_calculate_capacity_info_t
 *********************************************************************
 * @brief
 *  This is the virtual drive calculation capacity data structure. 
 *
 * @ingroup fbe_api_virtual_drive_interface
 * @ingroup fbe_api_virtual_drive_calculate_capacity_info
 *********************************************************************/
typedef struct fbe_api_virtual_drive_calculate_capacity_info_s
{
    fbe_lba_t imported_capacity; /*!< PVD exported */
    fbe_block_edge_geometry_t block_edge_geometry; /*!< Downstream edge geometry */
    fbe_lba_t exported_capacity; /*!< VD calculated capacity */
}fbe_api_virtual_drive_calculate_capacity_info_t;
/*! @} */ /* end of group fbe_api_virtual_drive_interface_usurper_interface */

/*!*******************************************************************
 * @struct fbe_api_virtual_drive_get_configuration_t
 *********************************************************************
 * @brief
 *  This is the virtual drive calculation capacity data structure. 
 *
 * @ingroup fbe_api_virtual_drive_interface
 * @ingroup fbe_api_virtual_drive_calculate_capacity_info
 *********************************************************************/
typedef struct fbe_api_virtual_drive_get_configuration_s
{
    fbe_virtual_drive_configuration_mode_t configuration_mode;
}fbe_api_virtual_drive_get_configuration_t;

/*!*******************************************************************
 * @struct fbe_api_virtual_drive_unused_drive_as_spare_flag_s
 *********************************************************************
 * @brief
 *  It is used to get the value of unused drive as spare flagfrom
 *  virtual drive object.
 *
 * @ingroup fbe_api_virtual_drive_interface
 *
 *********************************************************************/
typedef struct fbe_api_virtual_drive_unused_drive_as_spare_flag_s
{
    /*Virtual drive unused drive as spare flag*/
    fbe_bool_t unused_drive_as_spare_flag;
}
fbe_api_virtual_drive_unused_drive_as_spare_flag_t;

/*! @} */ /* end of group fbe_api_virtual_drive_interface_usurper_interface */

/*!*******************************************************************
 * @struct fbe_api_virtual_drive_get_permanent_spare_trigger_time_t
 *********************************************************************
 * @brief
 *  This is the virtual drive permanent spare trigger time data structure. 
 *
 * @ingroup fbe_api_virtual_drive_interface
 *********************************************************************/
typedef struct fbe_api_virtual_drive_permanent_spare_trigger_time_s
{
    fbe_u64_t permanent_spare_trigger_time;
}fbe_api_virtual_drive_permanent_spare_trigger_time_t;
/*! @} */ /* end of group fbe_api_virtual_drive_interface_usurper_interface */

/*!*******************************************************************
 * @struct fbe_api_virtual_drive_get_info_t
 *********************************************************************
 * @brief
 *  This is the virtual drive server info data structure. 
 *
 * @ingroup fbe_api_virtual_drive_interface
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_api_virtual_drive_get_info_s
{
    fbe_lba_t                               vd_checkpoint;          /* The rebuild checkpoint for the virtual drive */
    fbe_virtual_drive_configuration_mode_t  configuration_mode;     /* Current mode virtual drive is configured as */
    fbe_edge_index_t                        swap_edge_index;        /* Edge index that is / will be needed to swap out */
    fbe_object_id_t                         original_pvd_object_id; /* Object id of original / source pvd */
    fbe_object_id_t                         spare_pvd_object_id;    /* Object id of spare / destination pvd (if none then FBE_OBJECT_ID_INVALID) */
    fbe_bool_t                              b_request_in_progress;  /* Indicates if there is a job request (only valid on active SP) in progress */
    fbe_bool_t                              b_is_copy_complete;     /* FBE_TRUE if all the data has been copied */

} fbe_api_virtual_drive_get_info_t;
#pragma pack()
/*! @} */ /* end of group fbe_api_virtual_drive_interface_usurper_interface */

/*!*******************************************************************
 * @struct fbe_api_virtual_drive_get_performance_tier_t
 *********************************************************************
 * @brief
 *  Get the performance tier information. 
 *
 * @ingroup fbe_api_virtual_drive_interface
 *********************************************************************/
typedef struct fbe_api_virtual_drive_get_performance_tier_s
{
    fbe_u32_t   last_valid_drive_type_plus_one;
    fbe_u32_t   performance_tier[FBE_DRIVE_TYPE_LAST];   /*! Output performance tier for the drive type specified */
}fbe_api_virtual_drive_get_performance_tier_t;
/*! @} */ /* end of group fbe_api_virtual_drive_interface_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Virtual Drive Interface.  
// This is where all the function prototypes for the FBE API Virtual Drive.
//----------------------------------------------------------------
/*! @defgroup fbe_api_virtual_drive_interface FBE API Virtual Drive Interface
 *  @brief 
 *    This is the set of FBE API Virtual Drive Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_virtual_drive_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------

fbe_status_t FBE_API_CALL fbe_api_virtual_drive_get_spare_info(fbe_object_id_t vd_object_id,
                                                  fbe_spare_drive_info_t * spare_drive_info_p);

fbe_status_t FBE_API_CALL fbe_api_virtual_drive_calculate_capacity(fbe_api_virtual_drive_calculate_capacity_info_t * capacity_info);

fbe_status_t FBE_API_CALL fbe_api_virtual_drive_get_configuration(fbe_object_id_t vd_object_id,
                                                     fbe_api_virtual_drive_get_configuration_t * get_configuration_p);
fbe_status_t FBE_API_CALL
fbe_api_virtual_drive_get_unused_drive_as_spare_flag(fbe_object_id_t vd_object_id,
                                                     fbe_bool_t * get_unsued_drive_as_spare_flag_p);
fbe_status_t FBE_API_CALL
fbe_api_virtual_drive_set_unused_drive_as_spare_flag(fbe_object_id_t vd_object_id,
                                                     fbe_bool_t  set_unsued_drive_as_spare_flag);
fbe_status_t FBE_API_CALL fbe_api_virtual_drive_get_permanent_spare_trigger_time(fbe_api_virtual_drive_permanent_spare_trigger_time_t * spare_config_info);
fbe_status_t FBE_API_CALL fbe_api_virtual_drive_class_set_unused_as_spare_flag(fbe_bool_t enable);


fbe_status_t FBE_API_CALL
fbe_api_virtual_drive_set_debug_flags(fbe_object_id_t vd_object_id, 
                                      fbe_virtual_drive_debug_flags_t virtual_drive_debug_flags);

fbe_status_t FBE_API_CALL fbe_api_control_automatic_hot_spare(fbe_bool_t enable);
fbe_status_t FBE_API_CALL fbe_api_update_permanent_spare_swap_in_time(fbe_u64_t swap_in_time_in_sec);
fbe_status_t FBE_API_CALL fbe_api_virtual_drive_get_info(fbe_object_id_t vd_object_id, 
                                                         fbe_api_virtual_drive_get_info_t *vd_get_info_p);
fbe_status_t FBE_API_CALL fbe_api_virtual_drive_get_checkpoint(fbe_object_id_t vd_object_id, 
                                                               fbe_lba_t *vd_check_point_p);
fbe_status_t FBE_API_CALL fbe_api_virtual_drive_get_performance_tier(fbe_api_virtual_drive_get_performance_tier_t *api_get_performance_tier_p);

/*! @} */ /* end of group fbe_api_virtual_drive_interface */

FBE_API_CPP_EXPORT_END
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

#endif /*FBE_API_VIRTUAL_DRIVE_INTERFACE_H*/


