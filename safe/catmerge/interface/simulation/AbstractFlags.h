/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractFlags.h
 ***************************************************************************
 *
 * DESCRIPTION:  Abstract Class definitions that define a group of flag bits
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    11/19/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __ABSTRACTFLAGS__
#define  __ABSTRACTFLAGS__
# include "generic_types.h"
# include "K10Assert.h"

class AbstractFlags {
public:
    virtual ~AbstractFlags() {}

    virtual UINT_64 get_flags() = 0;
    virtual void  set_flags(UINT_64 flags) = 0;
    virtual UINT_64 set_get_flags(UINT_64 flags) = 0;

    virtual void set_flag(UINT_32 index, bool b) = 0;
    virtual bool get_flag(UINT_32 index) = 0;
};

class SimpleFlags: AbstractFlags {
public:
    SimpleFlags(UINT_64 flags = 0): mFlags(flags) { }
    ~SimpleFlags() {}

    virtual UINT_64 get_flags() {
        return mFlags;
    }

    virtual void set_flags(UINT_64 flags) {
        mFlags = flags;
    }

    virtual UINT_64 set_get_flags(UINT_64 flags) {
        UINT_64 old_flags = mFlags;
        mFlags = flags;
        return old_flags;
    }

    void set_flag(UINT_32 index, bool b) {
        FF_ASSERT(index < 64);
        
        if(b) {
            mFlags |= ((UINT_64)1 << index);
        }
        else {
            mFlags &= (((UINT_64)1 << index) ^ -1);
        }
    }

    bool get_flag(UINT_32 index) {
        FF_ASSERT(index < 64);
        
        UINT_64 mask = (UINT_64)1 << index;
        return ((mFlags & mask) == mask);
    }


private:
    UINT_64 mFlags;
};

#endif // __ABSTRACTFLAGS__
