/*@LB************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****
 ****  Copyright (c) 2004-2008 EMC Corporation.
 ****  All rights reserved.
 ****
 ****  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.
 ****  ACCESS TO THIS SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT.
 ****  THIS CODE IS TO BE KEPT STRICTLY CONFIDENTIAL.
 ****
 ****************************************************************************
 ****************************************************************************
 *@LE************************************************************************/

/*@FB************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****
 **** FILE: csx_rt_ctype.c
 ****
 **** DESCRIPTION:
 ****    <TBS>
 ****
 **** NOTES:
 ****    <TBS>
 ****
 **** STX: @CSXV2OK@
 ****
 ****************************************************************************
 ****************************************************************************
 *@FE************************************************************************/

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#include "fbe/fbe_ctype.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/*
 * Copyright (c) 1996, 1997 Robert Nordier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

const unsigned short csx_rt_ctype_array[257] = {
    0,
    XCT_C, XCT_C, XCT_C, XCT_C, XCT_C, XCT_C, XCT_C, XCT_C,
    XCT_C, XCT_C | XCT_S, XCT_C | XCT_S, XCT_C | XCT_S, XCT_C | XCT_S, XCT_C | XCT_S, XCT_C, XCT_C,
    XCT_C, XCT_C, XCT_C, XCT_C, XCT_C, XCT_C, XCT_C, XCT_C,
    XCT_C, XCT_C, XCT_C, XCT_C, XCT_C, XCT_C, XCT_C, XCT_C,
    XCT_S | XCT_E, XCT_P, XCT_P, XCT_P, XCT_P, XCT_P, XCT_P, XCT_P,
    XCT_P, XCT_P, XCT_P, XCT_P, XCT_P, XCT_P, XCT_P, XCT_P,
    XCT_D | XCT_X, XCT_D | XCT_X, XCT_D | XCT_X, XCT_D | XCT_X, XCT_D | XCT_X, XCT_D | XCT_X, XCT_D | XCT_X, XCT_D | XCT_X,
    XCT_D | XCT_X, XCT_D | XCT_X, XCT_P, XCT_P, XCT_P, XCT_P, XCT_P, XCT_P,
    XCT_P, XCT_U | XCT_A | XCT_X, XCT_U | XCT_A | XCT_X, XCT_U | XCT_A | XCT_X, XCT_U | XCT_A | XCT_X, XCT_U | XCT_A | XCT_X,
    XCT_U | XCT_A | XCT_X, XCT_U | XCT_A,
    XCT_U | XCT_A, XCT_U | XCT_A, XCT_U | XCT_A, XCT_U | XCT_A, XCT_U | XCT_A, XCT_U | XCT_A, XCT_U | XCT_A, XCT_U | XCT_A,
    XCT_U | XCT_A, XCT_U | XCT_A, XCT_U | XCT_A, XCT_U | XCT_A, XCT_U | XCT_A, XCT_U | XCT_A, XCT_U | XCT_A, XCT_U | XCT_A,
    XCT_U | XCT_A, XCT_U | XCT_A, XCT_U | XCT_A, XCT_P, XCT_P, XCT_P, XCT_P, XCT_P,
    XCT_P, XCT_L | XCT_A | XCT_X, XCT_L | XCT_A | XCT_X, XCT_L | XCT_A | XCT_X, XCT_L | XCT_A | XCT_X, XCT_L | XCT_A | XCT_X,
    XCT_L | XCT_A | XCT_X, XCT_L | XCT_A,
    XCT_L | XCT_A, XCT_L | XCT_A, XCT_L | XCT_A, XCT_L | XCT_A, XCT_L | XCT_A, XCT_L | XCT_A, XCT_L | XCT_A, XCT_L | XCT_A,
    XCT_L | XCT_A, XCT_L | XCT_A, XCT_L | XCT_A, XCT_L | XCT_A, XCT_L | XCT_A, XCT_L | XCT_A, XCT_L | XCT_A, XCT_L | XCT_A,
    XCT_L | XCT_A, XCT_L | XCT_A, XCT_L | XCT_A, XCT_P, XCT_P, XCT_P, XCT_P, XCT_C
};

const unsigned short *
csx_rt_ctype_array_get(void)
{
    return csx_rt_ctype_array;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/*@TB************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****
 **** END: csx_rt_ctype.c
 ****
 ****************************************************************************
 ****************************************************************************
 *@TE************************************************************************/
