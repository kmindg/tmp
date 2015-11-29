#if !defined(USER_GENERICS_H)
#define	USER_GENERICS_H 0x00000001	/* from common dir */

/***********************************************************************
 *  Copyright (C)  EMC Corporation 2001
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 ***********************************************************************/

/***************************************************************************
 * user_generics.h
 ***************************************************************************
 *
 * DESCRIPTION:
 *   This header file defines the standard types to be in this operating
 *   system, and software written for it for user (as opposed to driver)
 *   code.  It simply defines a flag indicating user-level code and
 *   includes generics.h
 *
 * NOTES:
 *
 * HISTORY:
 *      07/05/2001  CJH     Created
 ***************************************************************************/

/*************************
 * LITERAL DEFINITIONS
 *************************/

/*
 * This file should only be included in a non-Flare environment.
 * Therefore, define a constant so that headers later can tell that they're
 * not being used in a Flare environment.  This is necessary because many of
 * the Flare headers use UMODE_ENV to indicate that the file should be
 * compiled for simulation, however non-Flare includers want the real
 * version, not simulation.
 */
#define NON_FLARE_ENV


/*************************
 * INCLUDE FILES
 *************************/

#include "generics.h"

#endif	/* !defined USER_GENERICS_H */ /* MUST be last line in file */
