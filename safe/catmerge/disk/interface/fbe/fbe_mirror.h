#ifndef FBE_MIRROR_H
#define FBE_MIRROR_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_mirror.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the
 *  mirror object.
 * 
 * @ingroup mirror_class_files
 * 
 * @revision
 *   5/20/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_object.h"

/*************************
 * LITERAL DEFINITIONS.
 *************************/

/*! @def FBE_MIRROR_MAX_WIDTH 
 *  @brief this is the max number of mirror components currently supported 
 */
#define FBE_MIRROR_MAX_WIDTH    3

/*! @def FBE_MIRROR_MIN_WIDTH
 *  @brief this is the minimum number of mirror components 
 */
#define FBE_MIRROR_MIN_WIDTH    1

#define FBE_MIRROR_MIN_VERIFY_DRIVES    2

/*************************
 *   FUNCTION DEFINITIONS 
 *************************/

 /*! @defgroup mirror_class Mirror Class
 *  @brief This is the set of definitions for the mirror.
 *  @ingroup fbe_base_object
 */ 
/*! @defgroup mirror_usurper_interface Mirror Usurper Interface
 *  @brief This is the set of definitions that comprise the mirror class
 *  usurper interface.
 *  @ingroup fbe_classes_usurper_interface
 * @{
 */ 

/*!********************************************************************* 
 * @enum fbe_mirror_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the base config specific control codes. 
 * These are control codes specific to the base config, which only 
 * the base config object will accept. 
 *         
 **********************************************************************/
typedef enum
{
	FBE_MIRROR_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_MIRROR),

    /* Insert new control codes here.
     */
	FBE_MIRROR_CONTROL_CODE_LAST
}
fbe_mirror_control_code_t;

/*!********************************************************************* 
 * @enum fbe_mirror_flags_t
 *  
 * @brief 
 * This enumeration lists all the mirror specific flags. 
 *         
 **********************************************************************/
typedef enum fbe_mirror_flags_e
{
	FBE_MIRROR_FLAG_INVALID,
	FBE_MIRROR_FLAG_LAST
}
fbe_mirror_flags_t;

#endif /* FBE_MIRROR_H */

/****************************
 * fbe_mirror_main.c *
 ****************************/

/*************************
 * end file fbe_mirror.h
 *************************/
