#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H 0x00800000	/* from sim dir */
#define FLARE__STAR__H

/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/********************************************************************
 *      environment.h
 ********************************************************************
 *
 * Description:
 *   This file contains one #define which may be used to tell the compiler if
 *   we're compiling for simulation or sauna.  There is an analagous file in
 *   the hardware directory.
 *
 * History:
 * 	20-Jan-93  Created.		bmiller
 *
 *  $Log: environment.h,v $
 *  Revision 1.3  1999/02/04 00:55:06  fladm
 *  User: jslovin
 *  Reason: cleanup
 *  Remove LAB_DEBUG and SIM_ENV defined from environment.h.
 *  They will now be supplied by the build process.
 *
 *  Revision 1.2  1999/01/28 19:59:56  fladm
 *  User: modonnell
 *  Reason: fcb_support
 *  Fix Enhanced Raid EPRs 392 and 402.
 *
 *  Revision 1.1  1999/01/05 22:24:54  fladm
 *  User: dibb
 *  Reason: initial
 *  Initial tree population
 *
 *  Revision 1.4  1998/10/28 19:51:59  bruss
 *   Changes made as part of K2_ARCH -> XP_ARCH k2.2 tree renovation
 *
 * Revision 1.3  1998/10/13  17:01:07  snickel
 *  Define K2_ARCH_NOXOR to prevent xor processor from being used; operations will be performed on the JP instead.
 *
 *  Revision 1.2  1998/07/30 19:52:28  mhartman
 *   enabled FCF_FABRIC and removed K2_BETA_R3_DISABLED
 *
 *  Revision 1.1  1998/06/29 19:03:12  mhartman
 *  Initial revision
 *
 *  Revision 1.11  1998/05/14 13:08:28  kcosta
 *   removed #define K2_BETA_R3_DISABLED This way we can sim R3.
 *
 *  Revision 1.10  1998/05/07 13:41:35  mhartman
 *   added FULL_SECTOR_CHECKSUM, FIXED_SEED_CHECKSUMMING, K2_BETA_R3_DISABLED defines
 *
 *  Revision 1.9  1997/11/19 20:28:31  ginga
 *   Added K_ARCH define
 *
 *  Revision 1.8  1997/07/09 14:34:44  kdobkows
 *   added K2_ARCH #define
 *
 *  Revision 1.7  1997/06/20 17:54:45  greid
 *   modified for k2 simulation
 *
 *  Revision 1.1  1997/06/19 19:29:38  greid
 *  Initial revision
 *
 *  Revision 1.6  1997/05/27 13:29:50  andrea
 *   Add definition for HBA_ARCH (an architecture with RAID on an HBA vs a remote SP).
 *
 *  Revision 1.5  1997/03/26 13:50:20  andrea
 *   Remove define of DBL_WORD_XOR_CHKSUM to return to db checksum format same as data.
 *
 *  Revision 1.4  1997/03/10 20:33:53  ginga
 *   Added FIBRE_CMI switch
 *
 *  Revision 1.3  1997/02/22 16:59:55  andrea
 *   Re-org to match K5 layout and new defines for new dbase format.
 *
 *  Revision 1.2  1996/06/27 18:41:59  chughes
 *   Merge Flare 9 into Espresso
 *
 *  Revision 1.1  1996/06/26 23:41:24  chughes
 *  Initial revision
 *
 *  Revision 1.4  1996/02/28 13:17:47  luongo
 *   Added Fibre Channel compilation switches.
 *
 * Revision 1.3  1996/02/19  13:23:39  lathrop
 *  add dm_arch, ifndef around file
 *
 *
 *********************************************************************/

/*
 *  static TEXT unique_file_id[] = "$Header: /uddsw/fladm/repositories/project/CVS/flare/src/include/denali/sim/environment.h,v 1.3 1999/02/04 00:55:06 fladm Exp $";
 */
/* *INDENT-OFF* */

#include "core_config_runtime.h"

/* Hardware Specific definitions */

#ifdef C4_INTEGRATED
/* turn this macro on for Morpheus JetFire bring up */
//#define C4_JETFIRE_HW  1
/* turn this macro on for Morepheus Argo bring up */
/* #define C4_ARGO_HW 1*/
#endif /* C4_INTEGRATED - C4HW - C4BUG - dead code */

#define K10_ENV
/*#define K10_WIN_SIM*/
#if defined(_MSC_VER)
#pragma warning(disable:4018)
#endif

/*
 * TESTPROC_ENV controls the inclusion and execution of internal
 * testing code in the build according to the following schedule...
 *
 * TESTPROC_ENV value       Result
 *      Positive                Internal testing threads included and started
 *      Zero                    Internal testing threads included only
 *      Negative                Internal testing threads not included
 */

#define LCC_SES_ENHANCEMENTS 1

#if defined (UMODE_ENV) || defined(SIMMODE_ENV)
#define TESTPROC_ENV        1
#else
#define TESTPROC_ENV        0
#endif	// UMODE_ENV

/* rlomas
 * SMALL_GLUT when set to 1 keeps fru_tbl and related structures compatible with
 * FILES. This allows switching between files and disks when debugging when this is
 * set.PRODUCTION SOFTWARE SHOULD NOT HAVE THIS SET!! If the disks being used have
 * been configured with SMALL_GLUT set to a different value than the currently running
 * software is set to, the system will crash with a memory page fault.
 */
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
#define SMALL_GLUT	1	// make sure set to 0 for shipping code
#else
#define SMALL_GLUT	0
#endif

#define FLARE_REVISION_CHECK_ENABLE		1	// enables checking of revision of
											// flare aginst revision in
											// configuration on the drives

/* rlomas
 * if SMALL_GLUT is NOT being used, modify the revision by putting a 1 in the
 * high order byte of the revision word. As this is currently 25 and SMALL_GLUT has
 * been the default, we will use 1 in the high byte to differentiate the two modes.
 * If you try to run code for the wrong GLUT_SIZE, the system will panic before
 * a problem occurs.
 */
#define	GLUT_SIZE_MASK	0xff000000
#define LARGE_GLUT_SIZE 0x01000000

/*
10/15/99 bump revision to account for new data in vault that
would be undefined if not reinitialized
*/
#if	SMALL_GLUT
#define CURRENT_FLARE_DEBUG_REVISION	41	// build 41
#else
#define	CURRENT_FLARE_DEBUG_REVISION	(41 | LARGE_GLUT_SIZE)
#endif

/*
 * SES_SCSI conditional
 *          0 -- use old serial line access to LCCs
 *          1 -- use SCSI access to LCCs
 */
//#define SES_SCSI 0
#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
#define SES_SCSI 0
#else
#define SES_SCSI 0
#endif



/* Program Specific definitions */
#define HN_ARCH			/* Applies to any board in a Hornet's Nest */
#define SPS_ENV			/* Applies to a system configured with an sps
						 * backup unit */
#define T3VOLT			/* This is a Tachyon 3-Volt part build.   */
#define FIBRE_FE		/* Fibre front-end */
/* #define K2_ARCH */           /* Applies to Xor Processing in sim */


/* Debug Switches */

#define FCB_DEBUG		/* This is a back end driver debug switch */

#define FCF_FABRIC

/* Checksum Switches */
#define FULL_SECTOR_CHECKSUM  /* if defined, will checksum the entire
                               * sector instead of just the first and
                               * last 8 words of the sector.  */

#define K10_OPTIMAL_BACKEND_IO_ALIGNMENT 1

/*
 * End of $Id: environment.h,v 1.3 1999/02/04 00:55:06 fladm Exp $
 */
#endif /* ndef ENVIRONMENT_H */
