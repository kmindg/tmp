#ifndef FBE_CTYPE_H_
#define FBE_CTYPE_H_

/*
This file is a copy of the CSX ctype code. If/when the FBE code is ported to
CSX, this copy can be removed.
*/

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
 **** FILE: csx_rt_ctype_if.h
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

#if ((defined(__GNUC__) && __GNUC__ >= 3 && __GNUC_MINOR__ >= 4)) || defined(_MSC_VER)
#pragma once
#endif                                     /* ((defined(__GNUC__) && __GNUC__ >= 3 && __GNUC_MINOR__ >= 4)) || defined(_MSC_VER) */

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/* QQQ - BADNAMES - QQQ */
#define XCT_C  0x01                        /* cntrl */
#define XCT_S  0x02                        /* space */
#define XCT_P  0x04                        /* punct */
#define XCT_D  0x08                        /* digit */
#define XCT_U  0x10                        /* upper */
#define XCT_L  0x20                        /* lower */
#define XCT_X  0x40                        /* xdigit */
#define XCT_E  0x80                        /* (' ') */
#define XCT_A  0x100                       /* alpha */
/* QQQ - BADNAMES - QQQ */

const unsigned short * csx_rt_ctype_array_get(void);                           //PQPQ MARK PURE!!!

#define csx_rt_ctype_isctype(c, x) ((csx_rt_ctype_array_get())[(unsigned char)(1 + (c))] & (x))

#define csx_rt_ctype_isalnum(c)   csx_rt_ctype_isctype(c, XCT_D | XCT_U | XCT_L)
#define csx_rt_ctype_isalpha(c)   csx_rt_ctype_isctype(c, XCT_A)
#define csx_rt_ctype_iscntrl(c)   csx_rt_ctype_isctype(c, XCT_C)
#define csx_rt_ctype_isdigit(c)   csx_rt_ctype_isctype(c, XCT_D)
#define csx_rt_ctype_isgraph(c)   csx_rt_ctype_isctype(c, XCT_D | XCT_U | XCT_L | XCT_P)
#define csx_rt_ctype_islower(c)   csx_rt_ctype_isctype(c, XCT_L)
#define csx_rt_ctype_isprint(c)   csx_rt_ctype_isctype(c, XCT_D | XCT_U | XCT_L | XCT_P | XCT_E)
#define csx_rt_ctype_ispunct(c)   csx_rt_ctype_isctype(c, XCT_P)
#define csx_rt_ctype_isspace(c)   csx_rt_ctype_isctype(c, XCT_S)
#define csx_rt_ctype_isupper(c)   csx_rt_ctype_isctype(c, XCT_U)
#define csx_rt_ctype_isxdigit(c)  csx_rt_ctype_isctype(c, XCT_X)

#define csx_rt_ctype_tolower(c) ((csx_rt_ctype_isupper(c)) ? ((c) + 0x20) : (c))
#define csx_rt_ctype_toupper(c) ((csx_rt_ctype_islower(c)) ? ((c) - 0x20) : (c))

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
 **** END: csx_rt_ctype_if.h
 ****
 ****************************************************************************
 ****************************************************************************
 *@TE************************************************************************/
#endif                                     /* FBE_CTYPE_H_ */
