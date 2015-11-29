/*****************************************************************************/
/*                                                                           */
/* cachesimserver_shmem.h Cache sim server to Sorcery shared memory defs     */
/*                                                                           */
/* Copyright (C) 2013 EMC Corporation                                        */
/* All rights reserved                                                       */
/*                                                                           */
/*****************************************************************************/

#ifndef _CSIMSSHM_H_
#define _CSIMSSHM_H_

#ifdef WIN32
#ifndef _INT64T_ALREADY_DEFINED
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#define _INT64T_ALREADY_DEFINED
#endif
#endif

#if defined(LINUX64) || defined(LINUX32)
#include <sys/types.h>
#endif

#if defined(SPARCSUNSOL24) || defined(SPARCSUNSOL25) || defined(SPARCSUNSOL26) || defined(SPARCSUNSOL27) || defined(SPARCSUNSOL28) || defined(SOLARIS10) 
#include <sys/types.h>
#endif

#if defined(HPHPUX1001) || defined(HPHPUX1010) || defined(HPHPUX1100)
#include <sys/types.h>
#endif

#if defined(RS6000IBMAIX411)
#include <sys/types.h>
#endif

/**************************************************************************/
/* This file defines the structure and values used by the shared memory   */
/* mechanism to communicate between csimsrv and Sorcery thread programs.  */
/*                                                                        */
/* The shared memory area is configured to be an array of ioreqtype       */
/* structs. Each struct is used by one thread program to perform          */
/* io using one clsim thread.                                             */
/*                                                                        */
/* Each thread can be used to read or write any of the predefined volumes */
/* using a syncronous semaphore to transfer control back and forth        */
/* between the clserve thread program and the internal csimsrv thread.    */
/**************************************************************************/

/* Use C conventions so the thread program doesn't require C++            */
#ifdef	__cplusplus
extern "C" {
#endif

/**************************************************************************/
/* Manifest constants                                                     */
/**************************************************************************/

#define SIM_SERVER_SHMEM_NAME       "SIM_SERVER_SHMEM"
#define SIM_SERVER_SHMEM_NAME_SIZE  32
#define SIM_SERVER_IC_NAME_SIZE     32
#define SIM_TEST_STATUS_WAITING   0
#define SIM_TEST_STATUS_SUCCESS   1
#define SIM_TEST_STATUS_FAILURE   2

/**************************************************************************/
/* Csim information to communicate among test control leg, SPs, mcccli and thread programs. */
/**************************************************************************/

typedef volatile struct _sim_server_struct_t {

    unsigned long SPA_sim_ic_id;   /* container IDs, used by clients to connect to remote device */
    unsigned long SPB_sim_ic_id;
    char          SPA_sim_ic_name[SIM_SERVER_IC_NAME_SIZE]; /* container names, ditto use */
    char          SPB_sim_ic_name[SIM_SERVER_IC_NAME_SIZE]; 
    unsigned long client_test_status;   /* allows client to return test status in test mode */

} sim_server_struct_t;

typedef sim_server_struct_t *pSim_server_struct_t ;


#ifdef	__cplusplus
}
#endif

#endif /* _CSIMSSHM_H_ */
