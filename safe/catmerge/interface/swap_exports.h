/*******************************************************************************
 * Copyright (C) EMC Corporation, 2006
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * File Name:
 *              swap_exports.h
 *
 * Description: 
 *              This header file contains functions to perform 16bit, 32bit and 
 *              64bit byte swaps for endian-ness purposes.
 *
 * History:
 *              05-01-06 - Phil leef - created
 *              06-15-06 - Chetan Loke - Modified swap64
 ******************************************************************************/

#ifndef _SWAP_EXPORTS_H_
#define _SWAP_EXPORTS_H_

#include "k10csx.h"
#include "generic_types.h"

/****************************************
 *** Functions to do endian-swapping. ***
 ****************************************/

/****************************************************************
 * swap16()
 ****************************************************************
 *
 *  Description:
 *     Inline function to perform endian-swapping of a 16-bit value.
 *
 *  Input:
 *     value - 16-bit data whose bytes should be swapped.
 *
 *  Output:
 *     The value, bytes-swapped.
 *
 ****************************************************************/
static __inline UINT_16E swap16( UINT_16E value )
{
    return csx_p_swap_16(value);
}

/****************************************************************
 * swap32()
 ****************************************************************
 *
 *  Description:
 *     Inline function to perform endian-swapping of a 32-bit value.
 *
 *  Input:
 *     value - 32-bit data whose bytes should be swapped.
 *
 *  Output:
 *     The value, byte-swapped.
 *
 ****************************************************************/
static __inline UINT_32E swap32( UINT_32E value )
{
    return csx_p_swap_32(value);
}

/****************************************************************
 * swap64()
 ****************************************************************
 *
 *  Description:
 *     Inline function to perform endian-swapping of a 64-bit value.
 *
 *  Input:
 *     value - 64-bit data whose bytes should be swapped.
 *
 *  Output:
 *     The value, byte-swapped.
 *
 ****************************************************************/
static __inline UINT_64E swap64( UINT_64E value )
{
	/* Existing code <<< 
	unsigned long dup = (unsigned long)(value >> 32);
	unsigned long dl = (unsigned long)value; // truncates upper 32
	//
	// swap unsigned longs & contents
	//
	value = swap32(dl);
	value = value << 32;
	value += swap32(dup);

	return value;

	Existing code >>> */

    return csx_p_swap_64(value);
}

#endif

