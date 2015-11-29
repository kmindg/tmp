#ifndef FBE_STRIPER_H
#define FBE_STRIPER_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_striper.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the
 *  striper object.
 * 
 * @ingroup striper_class_files
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
 *   FUNCTION DEFINITIONS 
 *************************/

 /*! @defgroup striper_class striper Class
 *  @brief This is the set of definitions for the striper.
 *  @ingroup fbe_base_object
 */ 
/*! @defgroup striper_usurper_interface striper Usurper Interface
 *  @brief This is the set of definitions that comprise the striper class
 *  usurper interface.
 *  @ingroup fbe_classes_usurper_interface
 * @{
 */ 

/*! @def FBE_STRIPER_MAX_WIDTH 
 *  @brief this is the max number of striper components currently supported 
 */
#define FBE_STRIPER_MAX_WIDTH    16

/*! @def FBE_STRIPER_MIN_WIDTH
 *  @brief this is the minimum number of striper components 
 */
#define FBE_STRIPER_MIN_WIDTH    1

/*!********************************************************************* 
 * @enum fbe_striper_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the base config specific control codes. 
 * These are control codes specific to the base config, which only 
 * the base config object will accept. 
 *         
 **********************************************************************/
typedef enum
{
	FBE_STRIPER_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_STRIPER),

    /* Insert new control codes here.
     */
	FBE_STRIPER_CONTROL_CODE_LAST
}
fbe_striper_control_code_t;

/*!********************************************************************* 
 * @enum fbe_striper_flags_t
 *  
 * @brief 
 * This enumeration lists all the striper specific flags. 
 *         
 **********************************************************************/
typedef enum fbe_striper_flags_e
{
	FBE_STRIPER_FLAG_INVALID,
	FBE_STRIPER_FLAG_LAST
}
fbe_striper_flags_t;

#endif /* FBE_STRIPER_H */

/*************************
 * end file fbe_striper.h
 *************************/
