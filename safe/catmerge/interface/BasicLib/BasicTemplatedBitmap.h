# if !__BasicTemplatedBitmap_h_
# define __BasicTemplatedBitmap_h_ 1

//***************************************************************************
// Copyright (C) EMC Corporation 2009
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicTemplatedBitmap.h
//
// Contents:
//      Defines & Implements the BasicTemplatedBitmap class.
//      See also BasicBitmap where number of bits can be specified at run time.
//
// Revision History:
//      --

template <int size> class BasicTemplatedBitmap {
	enum { S = (sizeof(ULONG)* 8), SM1= S-1, Elements= (size+S-1)/S };
public:

    BasicTemplatedBitmap() : mSize (size)
    {
        for(ULONG i=0; i < Elements; i++) mMap[i] = 0;
    }
    ~BasicTemplatedBitmap()
    {
    }

#if 0
    BasicTemplatedBitmap( BasicTemplatedBitmap & orig ) : mSize(orig.mSize)
    {
        for (ULONG i=0; i < Elements; i++)
        {
            mMap[i] = orig.mMap[i];
        }
    }
#endif
	void Clear()
	{
        for(ULONG i=0; i < Elements; i++) mMap[i] = 0;
    }
	bool IsAnySet() const
	{
		for(ULONG i=0; i < Elements; i++) {
			if(mMap[i]) return true;
		}

		return false;
	}

    void Set(ULONG bit)
    {
        FF_ASSERT(bit < mSize);
        mMap[bit/S] |= 1<<(bit & SM1);

    }
    void Set(ULONG bit, ULONG value)
    {
        // Only allow 1 bit to be changed.
        ULONG modVal = value & 0x1;
        FF_ASSERT(bit < mSize);
        // Make sure bit is cleared before setting value incase value is 0.
        mMap[bit/S] = (mMap[bit/S] & ~(1<<(bit & SM1))) | modVal<<(bit & SM1);
    }
        
	void Set()
	{
        for(ULONG i=0; i < Elements; i++) mMap[i] = ~(0);
    }
	void Clear(ULONG bit)
    {
        FF_ASSERT(bit < mSize);
        mMap[bit/S] &= ~(1<<(bit & SM1));

    }
    bool Get(ULONG bit) const
    {
       FF_ASSERT(bit < mSize);
       return (mMap[bit/S] & 1<<(bit & SM1)) != 0;
    }

    ULONG GetNumBits() const
    {
        return mSize;
    }

    ULONG GetByteCount() const
    {
        return ((Elements * S)/8);
    }

    PVOID GetBitmapBuffer()
    {
        return mMap;
    }

    ULONG FindFirst() const
    {
        ULONG firstBit = 0;
        ULONG i;
        for (i=0; i < Elements; i++) {
            ULONG bit=0;
            if (mMap[i]) {
                while((mMap[i] & 1<<bit) != 0 && bit < SM1)
                {
                    bit++;
                    firstBit++;
                }
                if (bit < SM1) return firstBit;
            } 
            else
            {
                firstBit += S;
            }
        }
        return firstBit;
    }

    BasicTemplatedBitmap<size> & operator= ( const BasicTemplatedBitmap<size> &right )
    {
        // Only allow copy if both are the same size and not to self
        if ( right.mSize == mSize && &right != this )
        {
            ULONG i;
            for (i=0; i < Elements; i++) {
                mMap[i] = right.mMap[i];
            }
        }
        return *this;
    }

    BasicTemplatedBitmap<size> & operator|= ( const BasicTemplatedBitmap<size> &right)
    {
        // Or the 2 bitmaps together if they are the same size
        if (right.mSize == mSize && &right != this )
        {
            ULONG i;
            for (i=0; i < Elements; i++) {
                mMap[i] |= right.mMap[i];
            }
        }
        return *this;
    }

    const bool operator== ( const BasicTemplatedBitmap<size> &right )
    {
        bool result = TRUE;
        // Only allow copy if both are the same size and not to self
        if ( right.mSize == mSize && &right != this )
        {
            ULONG i;
            for (i=0; (i < Elements) && result; i++) {
                result = (mMap[i] == right.mMap[i]);
            }
        }
        return result;
    }
protected:
    // Field is first, so that a persistent image saves its size.
    ULONG  mSize;

    ULONG  mMap[Elements];

};

# endif
