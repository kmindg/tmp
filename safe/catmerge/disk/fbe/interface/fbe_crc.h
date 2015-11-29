/***************************************************************************
 * Copyright (C) EMC  Corporation 1989-2009
 * All rights reserved.
 * Licensed material -- property of EMC  Corporation
 ***************************************************************************/

/********************************************************************
 * bitmap.h
 ********************************************************************
 *
 * Description:
 *   This file contains the code for calculating CRC32
 *
 * Table of Contents:
 *   See function prototypes.
 *  
 * History:
 *           Tony Tang Created 2/5/2010
 *
 **************************************************************************/
#include "generic_types.h"
#include "fbe/fbe_types.h"

#define CRC32_XINIT 0xFFFFFFFFL		/* initial value */
#define CRC32_XOROT 0xFFFFFFFFL		/* final xor value */
#define MINIMUM_CHECKSUM_LEN	 8

/* function prototypes */
fbe_u32_t fbe_calculate_crc32(fbe_u8_t *, fbe_u32_t, fbe_u32_t, fbe_u32_t);
fbe_u32_t fbe_assign_crc32(fbe_u8_t *, fbe_u32_t, fbe_u32_t, fbe_u32_t);
fbe_u32_t fbe_compare_crc32(fbe_u8_t *, fbe_u32_t, fbe_u32_t, fbe_u32_t);

fbe_u32_t fbe_calculate_crc32_in_logical_blocks(fbe_u8_t *, fbe_u32_t, fbe_u32_t, fbe_u32_t);
fbe_u32_t fbe_assign_crc32_in_logical_blocks(fbe_u8_t *, fbe_u32_t, fbe_u32_t, fbe_u32_t);
fbe_u32_t fbe_compare_crc32_in_logical_blocks(fbe_u8_t *, fbe_u32_t, fbe_u32_t, fbe_u32_t);

