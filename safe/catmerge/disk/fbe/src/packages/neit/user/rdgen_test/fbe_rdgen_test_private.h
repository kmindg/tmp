#ifndef __FBE_RDGEN_TEST_PRIVATE_H__
#define __FBE_RDGEN_TEST_PRIVATE_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_test_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local definitions of the rdgen test.
 *
 * @version
 *   9/2/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
void fbe_rdgen_test_add_unit_tests(mut_testsuite_t *suite_p);

void fbe_rdgen_test_setup(void);

/*************************
 * end file fbe_rdgen_test_private.h
 *************************/

#endif /* end __FBE_RDGEN_TEST_PRIVATE_H__ */
