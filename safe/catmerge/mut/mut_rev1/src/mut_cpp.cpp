/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_cpp.cpp
 ***************************************************************************
 *
 * DESCRIPTION: MUT/C++ implementation file
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    9/2/2010 Gutted, ready for new cpp implementation JD
 **************************************************************************/

#include "mut_cpp.h"

//CTest implementation

MUT_API CTest::CTest(void(*test)(void),  void(*startup)(void), void(*teardown)(void), const char *short_desc, \
                  const char *long_desc)
{ // constuctor for C tests
    cTest = test;
    cStartUp = startup;
    cTearDown = teardown;
    set_short_description(short_desc ? short_desc : "<no description provided>");
    
    set_long_description(long_desc ? long_desc : "<no description provided>");

}

void CTest::Test()
{
    (*cTest)();
}

// If StartUp and Teardown functions were provided, call them, otherwise do nothing.
void CTest::StartUp()
{
    if (cStartUp)
    {
        (*cStartUp)();
    }
}
    
void CTest::TearDown()
{
    if (cTearDown)
    {
        (*cTearDown)();
    }
}
   
