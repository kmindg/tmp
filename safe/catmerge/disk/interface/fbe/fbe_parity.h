#ifndef FBE_PARITY_H
#define FBE_PARITY_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_parity.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the
 *  parity object.
 * 
 * @ingroup parity_class_files
 * 
 * @revision
 *   9/1/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_object.h"
#include "fbe/fbe_transport.h"

/*************************
 *   FUNCTION DEFINITIONS 
 *************************/

/*!*******************************************************************
 * @def FBE_PARITY_R5_MIN_WIDTH
 *********************************************************************
 * @brief Minimum width for R5 units. small than this is essentially a mirror.
 *
 *********************************************************************/
 #define FBE_PARITY_R5_MIN_WIDTH 3

/*!*******************************************************************
 * @def FBE_PARITY_MIN_WIDTH
 *********************************************************************
 * @brief This is the minimum width for parity units.
 *        R5s have the smallest width for parity units.
 *
 *********************************************************************/
#define FBE_PARITY_MIN_WIDTH FBE_PARITY_R5_MIN_WIDTH

/*!*******************************************************************
 * @def FBE_PARITY_MAX_WIDTH
 *********************************************************************
 * @brief This is the maximum width for parity units.
 *
 *********************************************************************/
#define FBE_PARITY_MAX_WIDTH    16

/*!*******************************************************************
 * @def FBE_PARITY_R6_MIN_WIDTH
 *********************************************************************
 * @brief This is the minimum width for r6  units.
 *
 *********************************************************************/
#define FBE_PARITY_R6_MIN_WIDTH 4

 /*! @defgroup parity_class parity Class
 *  @brief This is the set of definitions for the parity.
 *  @ingroup fbe_base_object
 */ 
/*! @defgroup parity_usurper_interface parity Usurper Interface
 *  @brief This is the set of definitions that comprise the parity class
 *  usurper interface.
 *  @ingroup fbe_classes_usurper_interface
 * @{
 */ 

/*!********************************************************************* 
 * @enum fbe_parity_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the base config specific control codes. 
 * These are control codes specific to the base config, which only 
 * the base config object will accept. 
 *         
 **********************************************************************/
typedef enum
{
    FBE_PARITY_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_PARITY),

    /*! Usurpur command to Raid class to calculate the capacity 
     *  for the metadata.
     */
    FBE_PARITY_CONTROL_CODE_CLASS_CALCULATE_CAPACITY,

    /* Insert new control codes here.
    */
    FBE_PARITY_CONTROL_CODE_LAST
}
fbe_parity_control_code_t;

/*!********************************************************************* 
 * @enum fbe_parity_flags_t
 *  
 * @brief 
 * This enumeration lists all the parity specific flags. 
 *         
 **********************************************************************/
typedef enum fbe_parity_flags_e
{
	FBE_PARITY_FLAG_INVALID,
	FBE_PARITY_FLAG_LAST
}
fbe_parity_flags_t;

/*!*******************************************************************
 * @struct fbe_parity_control_class_calculate_capacity_s
 *********************************************************************
 * @brief
 *  This structure is used to obtain the parity object capacity
 *
 *********************************************************************/
typedef struct fbe_parity_control_class_calculate_capacity_s 
{
	/*! PVD capacity
     */
    fbe_lba_t imported_capacity;

    /*! Downstream edge geometry
     */
	fbe_block_edge_geometry_t block_edge_geometry;

    /*! exported capacity
     */
	fbe_lba_t exported_capacity;
}
fbe_parity_control_class_calculate_capacity_t;

#endif /* FBE_PARITY_H */

/*************************
 * end file fbe_parity.h
 *************************/
