
#ifndef __DiskSectorCount_h__
#define __DiskSectorCount_h__

/************************************************************************
 *
 *  Copyright (c) 2002, 2005-2006 EMC Corporation.
 *  All rights reserved.
 *
 *  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
 *  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
 *  BE KEPT STRICTLY CONFIDENTIAL.
 *
 *  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
 *  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR
 *  THE PURPOSE OF CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION
 *  MAY RESULT.
 *
 ************************************************************************/

//++
// File Name:
//
//      DiskSectorCount.h
//
// Contents:
//
//  The type DiskSectorCount is a 64 bit integer indicating the number of
//  disk sectors (rather than # of bytes).  Conversion routines
//  to/from bytes are provided here.

const ULONG    BYTES_PER_SECTOR        = 512;  //  Needed by other files in GLD

const ULONG    BYTES_PER_SECTOR_SHIFT  = 9;    //  2^9 = 512 Shifting by 9 divides and 
//  multiples by 512

# if DBG && CHECK_FOR_DiskSectorCount_Misuse
//
//  The type DiskSectorCount is a 64 bit integer indicating the number of
//  disk sectors (rather than # of bytes).
//
//  In debug mode, DiskSectorCount is a class, in order to detect erroneous mixing of
//  byte counts and sector counts at compile time.  The class DiskSectorCount is expected
//  to behave exactly like a ULONGLONG, except that it should be incompatible with ULONGLONG.
//
class DiskSectorCount {
public:
    // In a free build, there is no constructor, so in a checked build we pick an inital value 
    // that is likely to cause errors if it is used.
    DiskSectorCount() : mCount(0xdeadbeef) {}

    // Support "DiskSectorCount x = 1;"
    DiskSectorCount(const int & count) : mCount(count) {}

    // Support "DiskSectorCount x = someUlongLong;"
    DiskSectorCount(const ULONGLONG & count) : mCount(count) {}

    // Support "DiskSectorCount x = someUlong;"
    DiskSectorCount(const ULONG & count) : mCount(count) {}

    // Copy Constructor...
    DiskSectorCount(const DiskSectorCount & other) : mCount(other.mCount) {}

    // Standard operator
    bool operator == (const DiskSectorCount & other) const
    {
        return mCount == other.mCount;
    }
        
    bool operator != (const DiskSectorCount & other) const
    {
        return !(*this->mCount == other.mCount);
    }

    // Standard operator
    bool operator <(const DiskSectorCount & other) const
    {
        return mCount <other.mCount;
    }

    // Standard operator
    bool operator > (const DiskSectorCount & other) const
    {
        return mCount > other.mCount;
    }

    // Standard operator
    bool operator >= (const DiskSectorCount & other) const
    {
        return mCount >= other.mCount;
    }

    // Standard operator
    DiskSectorCount & operator = (const DiskSectorCount & other)
    {
        mCount = other.mCount;
        return *this;
    }

    // Standard operator
    DiskSectorCount & operator /= (const DiskSectorCount & other)
    {
        mCount /= other.mCount;
        return *this;
    }

    // Standard operator
    DiskSectorCount & operator /= (const ULONG other)
    {
        mCount /= other;
        return *this;
    }

    // Standard operator
    DiskSectorCount  operator -(const DiskSectorCount & other) const
    {
        DiskSectorCount result;

        result.mCount = mCount - other.mCount;
        return result;
    }

    // Standard operator
    DiskSectorCount  operator +(const DiskSectorCount & other) const
    {
        DiskSectorCount result;

        result.mCount = mCount + other.mCount;
        return result;
    }

    // Standard operator
    DiskSectorCount  operator /(const DiskSectorCount & other) const
    {
        DiskSectorCount result;

        result.mCount = mCount / other.mCount;
        return result;
    }

    // Standard operator
    DiskSectorCount  operator /(const ULONGLONG other) const
    {
        DiskSectorCount result;

        result.mCount = mCount / other;
        return result;
    }

    // Standard operator
    DiskSectorCount  operator /(const ULONG other) const
    {
        DiskSectorCount result;

        result.mCount = mCount / other;
        return result;
    }


    friend ULONGLONG DiskSectorsToBytes(DiskSectorCount count);
    friend DiskSectorCount DiskBytesToSectors(ULONGLONG count);
    friend ULONGLONG DiskSectorToULONGLONG(DiskSectorCount sectors);

private:
    ULONGLONG mCount;
};

// Returns the number of sectors as a ULONGLONG.
inline ULONGLONG DiskSectorToULONGLONG(DiskSectorCount sectors)
{
    return sectors.mCount;
}

// Converts a sector count to a byte count
inline ULONGLONG DiskSectorsToBytes(DiskSectorCount sectors)
{
    return sectors.mCount * BYTES_PER_SECTOR;
}

// Converts a byte count to a sector count
inline DiskSectorCount DiskBytesToSectors(ULONGLONG bytes)
{
    DiskSectorCount ret;

    ret.mCount = bytes / BYTES_PER_SECTOR;
    return ret;
}

# else // Free build version




typedef ULONGLONG DiskSectorCount;

inline ULONGLONG 
DiskSectorsToBytes(DiskSectorCount sectors)
{
    return( sectors << BYTES_PER_SECTOR_SHIFT );    // Multiply by 512
}


inline DiskSectorCount 
DiskBytesToSectors(ULONGLONG bytes)
{
    return( bytes >> BYTES_PER_SECTOR_SHIFT );      // Divide by 512
}

inline DiskSectorCount 
DiskBytesToSectorsRoundUp(ULONGLONG bytes)
{
    return( (bytes + BYTES_PER_SECTOR -1)  >> BYTES_PER_SECTOR_SHIFT );      // Divide by 512
}
 
// Converts a sector count to a byte count
inline ULONGLONG DiskSectorToULONGLONG(DiskSectorCount sectors)
{
    return sectors;
}

# endif

#endif  // __DiskSectorCount_h__

