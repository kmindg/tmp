/* dbo_crc32.h */

#ifndef DBO_CRC32_H_INCLUDED
#define DBO_CRC32_H_INLCUDED 0x00000001

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2001
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * DESCRIPTION:
 *   This header file contains definitions for CRC32 algorithm.
 *
 * NOTES:
 *
 *
 * HISTORY:
 *   09-Aug-00: Created.                               Mike Zelikov (MZ)
 *
 ***************************************************************************/

/* INCLUDE FILES */
#include "odb_defs.h"


/* LITERALS */
#define DBO_CRC32_DEFAULT_SEED                    ((UINT_32E) UNSIGNED_MINUS_1)
#define DBO_CRC32_SIZE                            sizeof(UINT_32E)


/* PROTOTYPES */
UINT_32E dbo_Crc32Buffer(UINT_32E seed, const UINT_8E *buff, const UINT_32 length);


#endif /* DBO_CRC32_H_INCLUDED */
