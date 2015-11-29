/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               UINT_32_HashKey.h
 ***************************************************************************
 *
 * DESCRIPTION: Implementation of a unsigned 32 bit HashKey
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    02/01/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
# ifndef _UINT32HASHKEY_
# define _UINT32HASHKEY_

# include "generic_types.h"

class UINT_32_HashKey
{
public:
    UINT_32_HashKey()               { mValue = 0; }
    UINT_32_HashKey(UINT_32 value)  { mValue = value; }

    inline void setValue(UINT_32 value) { mValue = value; }
    inline UINT_32 getValue()           { return mValue;  }

    UINT_32 Hash() const { return mValue; }

    inline bool operator == (UINT_32_HashKey & other) const { return mValue == other.getValue(); }
    inline bool operator  > (UINT_32_HashKey & other) const { return mValue  > other.getValue(); }

private:
    UINT_32 mValue;
};

#endif
