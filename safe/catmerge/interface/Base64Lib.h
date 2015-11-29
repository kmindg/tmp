/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  FILENAME
 *      Base64Lib.h
 *  
 *  DESCRIPTION
 *      This header file lists the functions exported by base-64 library.
 *      The library implements the Base-64 encoding algorithm.
 *
 *  HISTORY
 *     14-April-2009     Created.   -Ashwin Tidke
 *
 ***************************************************************************/

#pragma once

#include<windows.h>
//#include <winerror.h>

INT Base64EncodeDigest(UCHAR Digest[], 
                        ULONG DigestInputBytes,
                        UCHAR B64Digest[], 
                        ULONG DigestOutputBytes);