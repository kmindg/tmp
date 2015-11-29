/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                McrTypes.h
 ***************************************************************************
 *
 * DESCRIPTION: Only for type/constant definitions used for mcr test frameworks.  
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    18/05/2012 Created Wason Wang
 **************************************************************************/


#ifndef MCR_TYPES_2012_05_18_H
#define MCR_TYPES_2012_05_18_H

typedef enum LogMode_e{ LOG_CONSOLE = 0, LOG_FILE, LOG_BOTH, LOG_NONE }LogMode_t;

typedef enum drive_type_e {
    DRIVE_TYPE_INVALID = 0,
    DRIVE_TYPE_REMOTE_MEMORY, //currently used
	DRIVE_TYPE_LOCAL_MEMORY,  //default value, currently used
    DRIVE_TYPE_LOCAL_FILE,
    DRIVE_TYPE_REMOTE_FILE_SIMPLE,
    DRIVE_TYPE_VMWARE_REMOTE_FILE,

    DRIVE_TYPE_LAST,
}drive_type_t;



#endif//end MCR_TYPES_2012_05_18_H




