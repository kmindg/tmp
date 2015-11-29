#ifndef COMPARE_H
#define COMPARE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2001
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  compare.h
 ***************************************************************************
 *
 *  Description:
 *
 *      This file contains macros to compare datum.
 *
 *  History:
 *
 *      09/28/01 BMR    Created (from disk\interface\std.h)
 ***************************************************************************/

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) max(a,b)
#endif
#ifndef MIN
#define MIN(a,b) min(a,b)
#endif

#define sgn(a)   (((a) == 0) ? 0 : (((a) <= 0) ? -1 : 1))

#define SGN(a)   sgn(a)


#endif // last line of file
