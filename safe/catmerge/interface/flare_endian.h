#ifndef ENDIAN_H
#define	ENDIAN_H 0x00800000	/* from sim dir */
#define FLARE__STAR__H

/***************************************************************************
 *  Copyright (C)  EMC Corporation 1989-1998,2000
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/***************************************************************************
 * $Id: endian.h,v 1.1 1999/01/05 22:24:52 fladm Exp $
 ***************************************************************************
 *
 * DESCRIPTION:
 *   This header file contains the include files to support endian functions
 *
 * NOTES:
 *
 * HISTORY:
 *
 * $Log: endian.h,v $
 * Revision 1.1  1999/01/05 22:24:52  fladm
 * User: dibb
 * Reason: initial
 * Initial tree population
 *
 * Revision 1.4  1998/08/31 18:30:05  lathrop
 *  use dh_byte_order.h for little endian-ness
 *
 * Revision 1.3  1997/01/27 16:50:20  lathrop
 *  change big/little endian to FL_...
 *
 * Revision 1.2  1997/01/22 16:14:56  lathrop
 *  add little/big_endian defines depending upon cpu(i386)/__GNUC__
 *
 * Revision 1.1  1996/06/14 19:52:38  lathrop
 * Initial revision
 *
 ***************************************************************************/

/*
 * LOCAL TEXT unique_file_id = "$Header: /uddsw/fladm/repositories/project/CVS/flare/src/include/denali/sim/endian.h,v 1.1 1999/01/05 22:24:52 fladm Exp $"
 */

/*************************
 * INCLUDE FILES
 *************************/

#if		defined(K10_ENV)
#define	_LITTLE_ENDIAN		1
#define	FL_LITTLE_ENDIAN	1
#endif

/*
#if defined (__DGUX)
#if !defined(DG_BYTE_ORDER_H)
#include <dg_byte_order.h>
#endif
#endif
#if !defined(FLARE_NETINET_IN_H)
#include "flare_netinet_in.h"
#endif
#if !defined(FLARE_SYS_TYPES_H)
#include "flare_sys_types.h"
#endif
 */
/*******************************
 * LITERAL DEFINITIONS
 *******************************/

/*
#if _LITTLE_ENDIAN
#define FL_LITTLE_ENDIAN

#else
#define FL_BIG_ENDIAN

#endif
 */

/*********************************
 * STRUCTURE DEFINITIONS
 *********************************/

/*********************************
 * EXTERNAL REFERENCE DEFINITIONS
 *********************************/

/************************
 * PROTOTYPE DEFINITIONS
 ************************/

/*
 * NOTE: this is only here to pull in the htonl/htons macros
 */

#endif	/* ndef ENDIAN_H */ /* MUST be last line in file */
