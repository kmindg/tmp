#ifndef __BasicBitmap
#define __BasicBitmap

//***************************************************************************
// Copyright (C) EMC Corporation 2009, 2012
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicBitmap.h
//
// Contents:
//      Defines & Implements the BasicBitmap class, where number of bits is determined at run time. 
//      See also BasicTemplatedBitmap to set the number of bits at compile time. 
//
// Revision History:
//      07/20/2009 Initial Revision - Bhavesh Patel

#include "BasicMemoryAllocator.h"

class BasicBitmap 
{
    enum { BytesPerElement = sizeof(ULONGLONG), BitsPerElement = (BytesPerElement * 8), Mask= BitsPerElement-1};
public:

    BasicBitmap()
    {
        mMap = NULL;
    }
    
    ~BasicBitmap()
    {
        delete[] mMap;
    }

    // Calculates that how much memory is going to
    // require to allocate bitmap of provided size.
    //
    // @param size - Largest index# in the bitmap.
    //
    // Returns:
    //      Number of bytes required to allocate bitmap.
    //      It will be sector multiple count.
    static ULONG RequiredMemory(ULONG size)
    {
        ULONG bytes = 0;
        ULONG sector = 0;

        // size is the maximum index of LUNs for which we want to maintain
        // the bitmap. So we need to (size) number of bits for it. Calculate
        // how much bytes we require to maintain bitmap for (size) number of LUNs.
        bytes = ((size + BITS_PER_BYTE - 1)/BITS_PER_BYTE);

        // We have to allocate sector aligned memory as lower driver will not
        // support sub sector IOs.
        sector = ((bytes + BYTES_PER_SECTOR -1) / BYTES_PER_SECTOR );

        // Re-calculate the number of bytes as we calculated the memory in sectors.
        // Now the size will be the sector aligned.
        size = (sector * BYTES_PER_SECTOR);

        // Make it in multiple of element size.
        size = ((size + BytesPerElement - 1) / BytesPerElement) * BytesPerElement;

        return size;    
    }
    
    // Initializes members of class. It also allocates memory for Bitmap.
    // It asserts if it won't be able to allocate memory for the bitmap.
    //
    // @param size - Size of bitmap in terms of bits.
    // @param memAllocator - Optional Memory allocator object contains  
    //                  pre allocated memory. If NULL allocate with new.
    //                  If non-NULL then bitmap will be allocated on the memory
    //                  provided by allocator object.
    //                  
    // Returns:  
    //          NA  
    bool Initialize(ULONG size, BasicMemoryAllocator * memAllocator = NULL)
    {
        FF_ASSERT(mMap == NULL);

        mSize = size;        

        // Calculate how much memory we are going to need to
        // allocate bitmap.
        ULONG requiredMemoryInBytes = RequiredMemory(size);

        // How much elements we need to maintain the bitmap?
        mElements = requiredMemoryInBytes/BytesPerElement;

        if(memAllocator) {
            // If memory allocator object has been provided then 
            // get the memory from the allocator object and allocate
            // bitmap on it. 
            mMap = (ULONGLONG*) memAllocator->getMemory(sizeof(ULONGLONG) * mElements);
        }
        else {
            // No memory is provided so allocate new.
            mMap = new ULONGLONG[mElements];
        }

        if (!mMap)
        {
            return false;
        }

        // We have successfully allocated required memory, now fill it with zero.
        Clear();
        
        return true;
    }

    // Clears whole bitmap. It fills whole bitmap with zero.
    //
    // @param : NA
    // Returns:  
    //          NA    
    void Clear()
    {
        for(ULONG i=0; i < mElements; i++) mMap[i] = 0;
    }

    // Clears the particular bit in the bitmap.
    //
    // @param bit - Index of the bit which needs to be cleared.
    // Returns:  
    //          NA    
    void Clear(ULONG bit)
    {
        FF_ASSERT(bit < mSize);
        mMap[bit/BitsPerElement] &= ~((ULONGLONG)1<<(bit & Mask));
    }

    // It checks whether any bits set in the bitmap.
    //
    // @param - NA
    // Returns:  
    //          true: If it finds at least one bit set in the bitmap.
    //          false: It none of the bit set in the bitmap.
    bool IsAnySet() const
    {
        for(ULONG i=0; i < mElements; i++) {
            if(mMap[i]) return true;
        }

        return false;
    }

    // Sets the particular bit in the bitmap.
    //
    // @param bit - Index of bit which needs to be set.
    // Returns:  
    //          NA
    void Set(ULONG bit)
    {
        FF_ASSERT(bit < mSize);
        mMap[bit/BitsPerElement] |= (ULONGLONG) 1<<(bit & Mask);
    }

    // Sets the particular bit to the specifid value.
    //
    // @param bit - Index of bit which needs to be set.
    //              value - Value which needs to be assigned.
    // Returns:  
    //          NA
    void Set(ULONG bit, ULONG value)
    {
        // Only allow 1 bit to be changed.
        ULONGLONG modVal = value & 0x1;
        FF_ASSERT(bit < mSize);
        // Make sure bit is cleared before setting value incase value is 0.
        mMap[bit/BitsPerElement] = (mMap[bit/BitsPerElement] & ~((ULONGLONG)1<<(bit & Mask))) | modVal<<(bit & Mask);
    }

    // Sets whole bitmap.
    //
    // @param - NA
    // Returns:  
    //          NA        
    void Set()
    {        
        for(ULONG i=0; i < mElements; i++) mMap[i] = ~((ULONGLONG)0);
    }

    // Returns the value for particular bit from the bitmap.
    //
    // @param bit - Index of bit for which we want the value.
    // Returns:  
    //          Value of particular bit.    
    bool Get(ULONG bit) const
    {
       FF_ASSERT(bit < mSize);
       return (mMap[bit/BitsPerElement] & (ULONGLONG)1<<(bit & Mask)) != 0;
    }

    // Returns size of bitmap.
    //
    // @param - NA
    // Returns:  
    //          Size of bitmap in bits.    
    ULONG GetNumBits() const
    {
        return mSize;
    }

    // Returns size of bitmap.
    //
    // @param - NA
    // Returns:  
    //          Size of bitmap in bytes. 
    ULONG GetByteCount() const
    {
        return (mElements * BytesPerElement);
    }

    // Returns pointer to bitmap.
    //
    // @param - NA
    // Returns:  
    //          Pointer to Bitmap. 
    PVOID GetBitmapBuffer()
    {
        return mMap;
    }

    ULONG FindFirst() const
    {
        ULONG firstBit = 0;
        ULONG i;
        for (i=0; i < mElements; i++) {
            ULONG bit=0;
            if (mMap[i]) {
                while((mMap[i] & (ULONGLONG)1<<bit) != 0 && bit < BitsPerElement)
                {
                    bit++;
                    firstBit++;
                }
                if (bit < BitsPerElement) return firstBit;
            } 
            else
            {
                firstBit += BitsPerElement;
            }
        }
        return firstBit;
    }

    // Overloaded Assignment operator.
    BasicBitmap & operator= ( const BasicBitmap &right )
    {
        // Only allow copy if both are the same size and not to self
        FF_ASSERT(right.mSize == mSize && &right != this)     

        ULONG i;
        for (i=0; i < mElements; i++) 
        {
            mMap[i] = right.mMap[i];
        }

        return *this;
    }

    //Overloaded |= operator.
    BasicBitmap & operator|= ( const BasicBitmap &right)
    {
        // Or the 2 bitmaps together if they are the same size
        FF_ASSERT(right.mSize == mSize && &right != this)
        ULONG i;
        for (i=0; i < mElements; i++) 
        {
            mMap[i] |= right.mMap[i];
        }
        return *this;
    }

    //Overloaded "==" operator.
    const bool operator== ( const BasicBitmap &right )
    {      
        // Only allow copy if both are the same size and not to self
        FF_ASSERT(right.mSize == mSize && &right != this)
        ULONG i;
        for (i=0; i < mElements; i++) 
        {
            if(mMap[i] != right.mMap[i])
            {
                return false;
            }
        }
        
        return true;
    }
    
protected:
    // Field is first, so that a persistent image saves its size.
    // Size in bits.
    ULONG  mSize;

    // Pointer to Bitmap.
    ULONGLONG*  mMap;

private:
    // Copy constructor        
    BasicBitmap( BasicBitmap const & orig ) : mSize(orig.mSize)
    {
        Initialize(mSize);

        ULONG i;
        for (i=0; i < mElements; i++) 
        {
            mMap[i] = orig.mMap[i];
        }
    }

    // Number of elements require to map the given size.
    ULONG mElements;
};

# endif
