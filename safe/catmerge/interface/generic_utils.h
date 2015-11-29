#ifndef __GENERIC_UTILS__
#define __GENERIC_UTILS__

//***************************************************************************
// Copyright (C) EMC 2006
// All rights reserved.
// Licensed material -- property of EMC
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      generic_utils.h
//
// Contents:
//      Various utilities and macros
//
// Revision History:
//  05-02-06    Phil Leef    Created.
//--


//
// Header files
//
#include "k10defs.h"

//++
// Name: 
//     BOOLEAN_TO_TEXT
//
// Description:
//     inline  
//--
#define BOOLEAN_TO_TEXT(m_bool)               \
    (m_bool ? "True" : "False")

//++
// Name: 
//     BOOLEAN_TO_ENABLE_TEXT
//
// Description:
//     inline  
//--
#define BOOLEAN_TO_ENABLE_TEXT(m_bool)  \
    (m_bool ? "Enable" : "Disable")


//++
// Name: 
//     IS_LEAP_YEAR
//
// Description:
//     Determines if year is a Leap year
//--
#define IS_LEAP_YEAR(m_year)        \
    ((((m_year % 4) == 0) &&        \
      (((m_year % 100) != 0) ||     \
       ((m_year % 400) == 0))) ?    \
     TRUE : FALSE )


//++
// Name: 
//     BYTE_BCD_TO_DEC
//
// Description:
//     convert BCD byte to Dec value
//--
#define BYTE_BCD_TO_DEC(m_bcd)        \
    ( (((m_bcd) & 0xFF) >> 4) * 10 + ((m_bcd) & 0x0F) )

#endif
