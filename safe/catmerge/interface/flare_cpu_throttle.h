#ifndef FLARE_CPU_THROTTLE_H
#define FLARE_CPU_THROTTLE_H 0x0000001
#define FLARE__STAR__H
/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008,2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  flare_cpu_throttle.h
 ***************************************************************************
 *
 *  Description
 *
 *      This file contains structures needed for cpu throttling for
 *      navi response time improvement
 *
 *  History:
 *
 *      06/19/10 Created - Ashwin Tamilarasan
 ***************************************************************************/

#include "generics.h"
//#include "k10ntddk.h"


// If the clients gets added please change the #define too.
#define CPU_THROTTLE_NUMBER_OF_CLIENTS 3
typedef enum client_ids
{
    CLIENT_ID_NONE = 0,
    NAVICIMOM_ID   = 1,
    NDU_ID         = 2,
    LAST_CLIENT_ID = NDU_ID,

} cpu_throttle_client_id;


//Allocate CPU for latency sensitive command processing.
// NAvicomom and NDU are the 2 clients
// Timer is timeout value in seconds after which the CPU throttle will be reset
// Default value for timer is -1 meaning Navi didn't set a timeout value
// In that case flare default timeout value of 5 min will kick in
typedef struct adm_user_space_needs_cpu
{
	cpu_throttle_client_id client_id;
	INT_32 timer;
}
ADM_USER_SPACE_NEEDS_CPU;



//Deallocate CPU for latency sensitive command processing.
//This structure corresponds to PROC_CPU_NEED_COMPLETED
typedef struct adm_user_space_complete_cpu
{
	cpu_throttle_client_id client_id;
}
ADM_USER_SPACE_COMPLETE_CPU;

//Resets CPU client id
// This structure corresponds to RESET_CPU_THROTTLE
typedef struct adm_user_space_reset_cpu
{
	cpu_throttle_client_id client_id;
}
ADM_USER_SPACE_RESET_CPU;

//Turn off/on CPU throttle feature
//This structure corresponds to SET_ENABLE_DISABLE_CPU_THROTTLE_FEATURE 
//If we get true we disable the feature
//If we get False we enable the feature 
typedef struct set_cpu_throttle_enable_disable
{
	BOOL turn_off;
}
SET_CPU_THROTTLE_ENABLE_DISABLE;

//Get the CPU throttle status
//This structure corresponds to GET_ENABLE_DISABLE_CPU_THROTTLE_FEATURE 
// If feature is enabled we return True
// If feature is disabled we return False
typedef struct get_cpu_throttle_enable_disable
{
	BOOL cpu_throttle_status;
}
GET_CPU_THROTTLE_ENABLE_DISABLE;

#endif /* FLARE_CPU_THROTTLE_H */
