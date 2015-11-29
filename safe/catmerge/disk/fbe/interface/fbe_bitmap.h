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
 *   This file contains the code for the functions to manipulate a simple bit map
 *
 * Table of Contents:
 *   See function prototypes.
 *  
 * History:
 *           Tony Tang Created 2/10/2009
 *
 **************************************************************************/
#include "generic_types.h"

//Shift to convert a bit index to byte offset from start of the array
#define BIT_TO_BYTE_SHIFT	3
//Mask to convert a bit index to the bit position within a byte
#define BIT_IN_BYTE_MASK	7

#define bitIsClear(bitArr, bitno) \
     ((bitArr[bitno >> BIT_TO_BYTE_SHIFT] &  (1 << (bitno & BIT_IN_BYTE_MASK))) == 0)
 
#define bitIsSet(bitArr, bitno)  \
	 ((bitArr[bitno >> BIT_TO_BYTE_SHIFT] & (1 << (bitno & BIT_IN_BYTE_MASK))) != 0)
 
#define clearBit(bitArr, bitno) \
	 (bitArr[bitno >> BIT_TO_BYTE_SHIFT] &= ~(1 << (bitno & BIT_IN_BYTE_MASK))) 

#define setBit(bitArr, bitno) \
    (bitArr[bitno >> BIT_TO_BYTE_SHIFT] |= (1 << (bitno & BIT_IN_BYTE_MASK)))
