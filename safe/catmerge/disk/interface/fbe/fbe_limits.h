#ifndef FBE_LIMITS_H
#define FBE_LIMITS_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_limits.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the values that define limits for logical objects
 * 
 * @ingroup
 * 
 * @revision
 *   02/17/2010:  Created. Jesus Medina
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
//#include "fbe/fbe_object.h"
//#include "fbe/fbe_physical_drive.h"
//#include "fbe/fbe_port.h"
//#include "fbe_raid_geometry.h"

/*!@todo Following two limits are currently hard coded to 128 but this limit 
 *  depends on the maximum number of many to one and one to many relationship
 *  we can have between different hierarchical objects in logical package.
 */
#define     FBE_MAX_UPSTREAM_OBJECTS        256
#define     FBE_MAX_DOWNSTREAM_OBJECTS      16

/**/
#define     FBE_PHYSICAL_BUS_COUNT          16
#define     FBE_ENCLOSURES_PER_BUS          10
#define     FBE_ENCLOSURE_SLOTS             120 //60

#endif /* FBE_LIMITS_H */

/***********************
 * end file fbe_limits.h
 ***********************/


