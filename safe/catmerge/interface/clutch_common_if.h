#ifndef CLUTCH_COMMON_IF_H
#define CLUTCH_COMMON_IF_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2003
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  DESCRIPTION
 *      This file defines the common (kernel & user) interfaces to the ASE
 *      clutch service.
 *   
 *
 *  HISTORY
 *      08-Jan-2003     Created     Joe Shimkus
 *
 ***************************************************************************/

#include "generics.h"
#include "k10defs.h"

/*
 * Constants.
 */
 
/*
 * Clutch attributes.
 *
 * The clutch mechanism defines only one attribute of a clutch; the
 * clutch type.  The clutch mechanism supports 255 client-defined
 * attributes.
 *
 * The clutch type can only be specified at clutch creation.  Any
 * attempt to change the clutch type via clutchSetAttributes() will be
 * silently ignored.
 *  
 * Clutch attributes are stored as a contiguous byte grouping of triples where each
 * triple is composed as:
 * 
 *              <id><byte length><attribute>
 * 
 *              <id>            :   1 byte
 *              <byte length>   :   2 bytes
 *              <attribute>     :   attribute bytes
 * 
 * The end of the clutch attributes byte grouping is specified by a combination of
 * ID 0x00 and byte length of 0x0000.
 * 
 * The clutch mechanism reserves attribute ID 0x00 for the purposes of identifying
 * a clutch’s type.  The clutch type field will be 1 byte in size.  Therefore, the
 * clutch type attribute will be:
 * 
 *              <0x00><0x0001><type>
 */
 
#define CLUTCH_ATTRIBUTE_ID_SIZE       1    // Bytes
#define CLUTCH_ATTRIBUTE_LENGTH_SIZE   2    // Bytes

#define CLUTCH_ATTRIBUTE_MINIMUM       0

#define CLUTCH_ATTRIBUTE_TYPE          CLUTCH_ATTRIBUTE_MINIMUM

#define CLUTCH_ATTRIBUTE_MAXIMUM       255

#define CLUTCH_NUMBER_OF_ATTRIBUTES    (CLUTCH_ATTRIBUTE_MAXIMUM+1)

#define CLUTCH_ATTRIBUTE_TYPE_SIZE     1   // Bytes

/*
 * CLUTCH_ALL_CLUTCHES defines the input argument to use for those calls which can
 * specify to operate on all clutches.
 */
 
#define CLUTCH_ALL_CLUTCHES             ((CLUTCH_ID *) -1)

/*
 * CLUTCH_ALL_MEMBERS defines the input argument to use for those calls which can
 * specify to operate on all members of a clutch.
 */
 
#define CLUTCH_ALL_MEMBERS              ((CLUTCH_MEMBER_LIST) -1)

/*
 *
 * CLUTCH_MAX_ATTRIBUTE_LENGTH defines the maximum total byte length which can
 * comprise a set of clutch attributes.  This count includes the necessary
 * attribute identifiers and lengths together with the terminating ID/length pair
 * and excludes the type attribute since this cannot be set via
 * clutchSetAttributes().
 * 
 * It is used externally for clients to know clutch limits and internally for
 * enforcing those limits as well as sizing necessary clutch data structures.
 */
 
#define CLUTCH_MAX_ATTRIBUTE_LENGTH                         \
                ((32*1024) -                                \
                    (CLUTCH_ATTRIBUTE_ID_SIZE +             \
                        CLUTCH_ATTRIBUTE_LENGTH_SIZE +      \
                            CLUTCH_ATTRIBUTE_TYPE_SIZE))

/*
 * CLUTCH_MAX_NAME_LENGTH defines the maximum number of characters which can
 * comprise a clutch name including the necessary terminating NUL (ASCII code 0). 
 * 
 * It is used externally for clients to know clutch limits and internally for
 * enforcing those limits as well as sizing necessary clutch data structures.
 */

#ifdef ALAMOSA_WINDOWS_ENV
#define CLUTCH_MAX_NAME_LENGTH              64
#else
#define CLUTCH_MAX_NAME_LENGTH              256
#endif

/*
 * CLUTCH_MAX_NUMBER_OF_CLUTCHES defines the maximum number of clutches supported
 * by the clutch mechanism.  
 *
 * It is used externally for clients to know clutch
 * limits and internally for enforcing those limits as well as sizing necessary
 * clutch data structures.
 */

#ifdef ALAMOSA_WINDOWS_ENV
#define CLUTCH_MAX_NUMBER_OF_CLUTCHES       64
#else
#define CLUTCH_MAX_NUMBER_OF_CLUTCHES       256
#endif

/*
 * CLUTCH_MAX_MEMBERS_PER_CLUTCH defines the maximum number of members which can
 * be assigned to an individual clutch.  
 * 
 * It is used externally for clients to know
 * clutch limits and internally for enforcing those limits as well as sizing
 * necessary clutch data structures.
 */
 
#ifdef ALAMOSA_WINDOWS_ENV
#define CLUTCH_MAX_MEMBERS_PER_CLUTCH       64
#else
#define CLUTCH_MAX_MEMBERS_PER_CLUTCH       128
#endif

/*
 * CLUTCH_MEMBER_SIZE defines the byte size of the opaque CLUTCH_MEMBER_ID.
 */

#define CLUTCH_MEMBER_SIZE                  64

/*
 * CLUTCH_ZERO_ENUM_KEY defines the initial and ending enumeration key for
 * iterative calls to the clutch listing function.
 */
 
#define CLUTCH_ZERO_ENUM_KEY                0

/* 
 * CLUTCH_ZERO_ENUM_MEMBER_KEY defines the initial and ending enumeration key
 * for iterative calls to the clutch member listing function.
 */

#define CLUTCH_ZERO_ENUM_MEMBER_KEY         0

/* 

 * Clutch types.
 *
 * The clutch mechanism only supports those types which it defines.  The total
 * possible number of types that the clutch mechanism could support is 255.
 *
 * The 255 limit comes from the fact that the clutch type is used to identify
 * clutch existence when reading the clutch from permanent storage and so
 * excludes a type of zero (0).  If zero were allowed, there would be no way
 * to distinguish a nonexistent clutch from a clutch with a type of zero (0).
 */

#define CLUTCH_NONEXISTENT_TYPE             0

#define CLUTCH_MINIMUM_TYPE                 1

#define CLUTCH_GENERIC_TYPE                 CLUTCH_MINIMUM_TYPE
#define CLUTCH_CONSISTENCY_GROUP            (CLUTCH_GENERIC_TYPE+1)
#define CLUTCH_FAR_GROUP					(CLUTCH_GENERIC_TYPE+2)

#define CLUTCH_MAXIMUM_TYPE                 (CLUTCH_FAR_GROUP)

#define CLUTCH_NUMBER_OF_TYPES              (CLUTCH_MAXIMUM_TYPE+1)

/*
 * Status Codes.
 *
 * The clutch mechanism returns the following status codes:
 */

#define CLUTCH_SUCCESS                      (EMCPAL_STATUS_SUCCESS)

#define CLUTCH_ATTRIBUTE_BAD_FORMAT         (-1     )
#define CLUTCH_ATTRIBUTE_TOO_LONG           (-2     )
#define CLUTCH_DOES_NOT_EXIST               (-3     )
#define CLUTCH_INVALID_ARGUMENTS            (-4     )
#define CLUTCH_INVALID_KEY                  (-5     )
#define CLUTCH_NAME_INVALID                 (-6     )
#define CLUTCH_NAME_NOT_UNIQUE              (-7     )
#define CLUTCH_NO_AVAILABLE_CLUTCHES        (-8     )
#define CLUTCH_TOO_MANY_MEMBERS             (-9     )
#define CLUTCH_UNKNOWN_MPS_MESSAGE          (-10    )
#define CLUTCH_UNKNOWN_MPS_REVISION         (-11    )
#define CLUTCH_UNKNOWN_OPERATION            (-12    )
#define CLUTCH_UNKNOWN_TYPE                 (-13    )
#define CLUTCH_NO_BACKING_STORE				(-14	)

/* This should always be the last error code.
 */
#define CLUTCH_MAX_ERROR_CODE				(-15	)
/*
 * CLUTCH_ERROR_STRING_COUNT
 *
 * This is a count of the total clutch error codes
 * accounted for in the following list..
 */
#define CLUTCH_ERROR_STRING_COUNT			(-CLUTCH_MAX_ERROR_CODE)

/*
 * CLUTCH_ERROR_STRINGS
 *
 * For each error code, an absolute value of the error
 * code corresponds to an entry in this set of strings.
 */
#define CLUTCH_ERROR_STRINGS\
    "STATUS_SUCCESS",\
    "CLUTCH_ATTRIBUTE_BAD_FORMAT",\
    "CLUTCH_ATTRIBUTE_TOO_LONG",\
    "CLUTCH_DOES_NOT_EXIST",\
    "CLUTCH_INVALID_ARGUMENTS",\
    "CLUTCH_INVALID_KEY",\
    "CLUTCH_NAME_INVALID",\
    "CLUTCH_NAME_NOT_UNIQUE",\
    "CLUTCH_NO_AVAILABLE_CLUTCHES",\
    "CLUTCH_TOO_MANY_MEMBERS",\
    "CLUTCH_UNKNOWN_MPS_MESSAGE",\
    "CLUTCH_UNKNOWN_MPS_REVISION",\
    "CLUTCH_UNKNOWN_OPERATION",\
    "CLUTCH_UNKNOWN_TYPE",\
    "CLUTCH_NO_BACKING_STORE"

/*
 * Data Types.
 */
 
/*
 * CLUTCH_ATTRIBUTES defines the input structure for specifying clutch attributes.
 * Change the type from char * to unsigned char * to prevent numbers in attributes
 * be interpreted as negtive (those > 0x7F in one byte).
 */
 
typedef unsigned char   *CLUTCH_ATTRIBUTES;

/*
 * CLUTCH_ENUM_KEY defines the type for the iteration key for the clutch list function.
 */
 
typedef UINT_32     CLUTCH_ENUM_KEY;

/* 
 * CLUTCH_ENUM_MEMBER_KEY defines the type for the iteration key for the clutch
 * member list function.
 */
 
typedef UINT_32     CLUTCH_ENUM_MEMBER_KEY;

/*
 * CLUTCH_NUMBER defines the type for the array local clutch number.
 */

typedef UINT_32     CLUTCH_NUMBER;

/*
 * CLUTCH_ID defines the type for the clutch identifier.
 */
 
typedef struct CLUTCH_ID
{
    K10_ARRAY_ID    arrayId;
    CLUTCH_NUMBER   clutchNumber;
} CLUTCH_ID;

/*
 * CLUTCH_LIST defines the type for the clutch list.
 * 
 * The CLUTCH_LIST is treated as an array of CLUTCH_IDs and the number of entries
 * in the CLUTCH_LIST is required by those functions which take a CLUTCH_LIST as
 * argument.
 */

typedef CLUTCH_ID   *CLUTCH_LIST;

/* 
 * CLUTCH_MEMBER_ID defines the type for the clutch member identifier.
 */
 
typedef struct CLUTCH_MEMBER_ID
{
    char    opaque[CLUTCH_MEMBER_SIZE];
} CLUTCH_MEMBER_ID;

/* 
 * CLUTCH_MEMBER_LIST defines the type for the clutch member list.
 * 
 * The CLUTCH_MEMBER_LIST is treated as an array of CLUTCH_MEMBER_IDs and the
 * number of entries in the CLUTCH_MEMBER_LIST is required by those functions which
 * take a CLUTCH_MEMBER_LIST as argument.
 */
 
typedef CLUTCH_MEMBER_ID    *CLUTCH_MEMBER_LIST;

/*
 * CLUTCH_NAME defines the type for the clutch name.
 */
 
typedef char                *CLUTCH_NAME;

/*
 * CLUTCH_TYPE defines the type for the clutch type.
 */
 
typedef UINT_32             CLUTCH_TYPE;

/*
 * Function interfaces.
 */
 
/*
 * Macros.
 */

#define CLUTCH_SET_ATTRIBUTE_LENGTH(attributeLengthPtr, attributeLength)    \
do                                                                          \
{                                                                           \
    *(attributeLengthPtr)       = (((attributeLength) & 0xFF00) >> 8);      \
    *((attributeLengthPtr)+1)   =  ((attributeLength) & 0x00FF);            \
}                                                                           \
while (FALSE)
 
#endif // CLUTCH_COMMON_IF_H
