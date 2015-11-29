/*
 * ----------------------------------------------------------------------------
 * Copyright (C) EMC Corporation, 1999-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 */

#if !defined(KTRACE_COND_H)
#define KTRACE_COND_H

/*#if !defined(_X86_)
#define _X86_ 1
#endif
*/
#if !defined(BLK_SIZE)
#define BLK_SIZE 512
#endif

#if !defined(DO_BIST)
#define DO_BIST 0
#endif

#if !defined(KTRACE_MARKER)
#define KTRACE_MARKER 0
#endif

#if !defined(KTRACE_IO_RING_SIZE)
#define      KTRACE_IO_RING_SIZE  (64*1024)
#endif

#if !defined(KTRACE_STD_RING_SIZE)
#define      KTRACE_STD_RING_SIZE (32*1024)
#endif

#if !defined(KTRACE_START_RING_SIZE)
#define      KTRACE_START_RING_SIZE (4*1024)
#endif

#if !defined(KTRACE_TRAFFIC_RING_SIZE)
#define      KTRACE_TRAFFIC_RING_SIZE (64*1024)
#endif

#if !defined(KTRACE_USER_RING_SIZE)
#define      KTRACE_USER_RING_SIZE (32*1024)
#endif

/* The CONFIG and redir buffers are small buffers used 
   to log hostisde configuration events and redirector events */

#if !defined(KTRACE_CONFIG_RING_SIZE)
#define      KTRACE_CONFIG_RING_SIZE 512
#endif

#if !defined(KTRACE_REDIRECTOR_RING_SIZE)
#define      KTRACE_REDIRECTOR_RING_SIZE (1*1024)
#endif

#if !defined(KTRACE_THRPRI_RING_SIZE)
#define      KTRACE_THRPRI_RING_SIZE (1*1024)
#endif

#if defined LOG_EVENTS_TO_NT_EVENT_LOG
#if !defined(KTRACE_EVT_LOG_RING_SIZE)
#define      KTRACE_EVT_LOG_RING_SIZE (4*1024)
#endif
#else
#if !defined(KTRACE_EVT_LOG_RING_SIZE)
#define      KTRACE_EVT_LOG_RING_SIZE 1
#endif
#endif

#if !defined(KTRACE_PSM_RING_SIZE)
#define      KTRACE_PSM_RING_SIZE      (4*1024)
#endif

#if !defined(KTRACE_ENV_POLL_RING_SIZE)
#define      KTRACE_ENV_POLL_RING_SIZE (2*1024)
#endif

#if !defined(KTRACE_XOR_START_RING_SIZE)
#define      KTRACE_XOR_START_RING_SIZE (2*1024)
#endif

#if !defined(KTRACE_XOR_RING_SIZE)
#define      KTRACE_XOR_RING_SIZE       (2*1024)
#endif

#if !defined(KTRACE_PEER_RING_SIZE)
#define      KTRACE_PEER_RING_SIZE      (32*1024)
#endif

#if !defined(KTRACE_MLU_RING_SIZE)
#define      KTRACE_MLU_RING_SIZE       (32*1024)
#endif                                          

#if !defined(KTRACE_CBFS_RING_SIZE)
#define      KTRACE_CBFS_RING_SIZE       (32*1024)
#endif 

#if !defined(KTRACE_RBAL_RING_SIZE)
#define      KTRACE_RBAL_RING_SIZE       (2*1024)
#endif 

#if !defined(KTRACE_SADE_RING_SIZE)
#define      KTRACE_SADE_RING_SIZE       (4*1024)
#endif 

#define TIME_SLOT_0
/*
 * implications
 */
#if defined(_GRASP_IGNORE)
#indef DO_BIST
#define DO_BIST 1
#endif

#if DO_BIST
#define UMODE_ENV
#endif

#endif /* !defined(KTRACE_COND_H) */
/* end of file ktrace_cond.h */
