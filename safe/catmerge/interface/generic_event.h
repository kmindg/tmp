//***********************************************************************
//
//  Copyright (c) 2015 EMC Corporation.
//  All rights reserved.
//
//  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
//  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
//  BE KEPT STRICTLY CONFIDENTIAL.
//
//  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
//  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR THE PURPOSE OF
//  CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION MAY RESULT.
//
//***********************************************************************

//++
// File Name:
//      generic_event.h
//
// Created:   MJC 6/2015
//
//
//  GenericEvent class used to create Events for the Event Log.
//
//--

#ifndef GENERIC_EVENT_H_
#define GENERIC_EVENT_H_

#include "csx_ext.h"
#include "EmcUTIL_Time.h"
#include "EmcPAL_Memory.h"

// Number of entries in the cache state machine log
#define GENERIC_EVENT_LOG_SIZE 100

// Max limit to avoid the infinite looping in certain cases
#define GENERIC_COUNT_LOOP_LIMIT 10


// GenericEvent - this class defines an event entry that is logged by the CacheStateEventLog
//            class.
//
typedef struct _GENERIC_EVENT_DATA
{
    unsigned int hi;
    unsigned int low;
} GENERIC_EVENT_DATA, *PGENERIC_EVENT_DATA;


class GenericEvent {
public:

    // Default constructor
    GenericEvent():mTimeStamp(0)
    {
        EmcpalZeroMemory(&mEventData, sizeof(mEventData));
    }

    // Destructor
    ~GenericEvent() {};

    // Constructor
    //
    // @param time      - current systime time
    // @param eventData - packed event data
    //
    GenericEvent(EMCUTIL_SYSTEM_TIME time, GENERIC_EVENT_DATA eventData) : mTimeStamp(time), mEventData(eventData) {};


    GenericEvent& operator = (const GenericEvent& right) {
        mEventData.hi = right.mEventData.hi;
        mEventData.low = right.mEventData.low;
        mTimeStamp = right.mTimeStamp;
        return *this;
    }

    bool operator == (const GenericEvent& right) const {
        if(mEventData.hi == right.mEventData.hi && mEventData.low == right.mEventData.low) {
            return true;
        }
        return false;
    }

    //*****************************************************************************
    // Get methods
    // These methods extract a given field from the event data and either compare
    // it to return a boolean value, or shift it right to return an enumerated
    // value.
    //*****************************************************************************

    // Get the Time stamp
    //
    // Returns:
    //  the time stamp as in nano seconds
    //
    EMCUTIL_SYSTEM_TIME GetTimestamp(void) {
        return (mTimeStamp); }

protected:

    // Timestamp
    EMCUTIL_SYSTEM_TIME mTimeStamp;

    // The event data in packed format.
    GENERIC_EVENT_DATA mEventData;
};

// GenericStateEventLog defines the logging facility that captures BvdSimStateMachine events
// in a ring buffer.
//
// This class expects the containing class to provide the locking.
//
class GenericStateEventLog {

public:
    // Constructor
    GenericStateEventLog() : mOldestEntry(0), mNextEntry(0), mIsBufferWrapped(false)
    {
        EmcpalZeroMemory(&mRingBuffer, sizeof(mRingBuffer));
    };

    //copy constructor
    GenericStateEventLog(const GenericStateEventLog &copy) {
        mOldestEntry = copy.mOldestEntry;
        mNextEntry = copy.mNextEntry;
        mIsBufferWrapped = copy.mIsBufferWrapped;
        memcpy(mRingBuffer, copy.mRingBuffer, sizeof(mRingBuffer));
    }

    // Destructor
    virtual ~GenericStateEventLog() {};

    // Operator = will copy desired CacheStateEventLog object to another. It will check the
    // value of mNextEntry before starting the copy and after the copy of log completes,
    // and will return only when there is no change in mNextEntry or we exceed the limit of
    // maximum 10 iterations.
    GenericStateEventLog& operator = (const GenericStateEventLog & right) {
        ULONG count = 0;
        while(true) {
            ULONG prevNextEntry = right.mNextEntry;
            memcpy(mRingBuffer, right.mRingBuffer, sizeof(mRingBuffer));
            mNextEntry = right.mNextEntry;
            mOldestEntry = right.mOldestEntry;
            mIsBufferWrapped = right.mIsBufferWrapped;
            if(prevNextEntry == right.mNextEntry) {
                break;
            }
            else {
                count++;
                //We'll loop through maximum 10 times to avoid the infinite loop.
                if(count > GENERIC_COUNT_LOOP_LIMIT) {
                    return *this;
                }
            }
        }
        return *this;
    }
    // Add an event.
    // CSM lock is acquired before call to this function.
    // @param logEntry the cache state machine event to log.
    void AddEventLockHeld(GenericEvent logEntry)
    {
        mRingBuffer[mNextEntry] = logEntry;
        mNextEntry++;

        if (mNextEntry == GENERIC_EVENT_LOG_SIZE)
        {
            mNextEntry = 0;
            mIsBufferWrapped = true;
        }

        if(mIsBufferWrapped == true)
        {
            mOldestEntry = mNextEntry;
        }
    };

    // Get an event log entry.
    // @param index - index of the next entry
    // Return: contents of indexed entry
    GenericEvent GetEventLogEntry(ULONG index)
    {
        // The index is allowed to go past the end of the ring buffer.
        // It is normalized here to account for wrap around.
        ULONG nextEntry = index;
        if (index >= GENERIC_EVENT_LOG_SIZE) {
            nextEntry = index % GENERIC_EVENT_LOG_SIZE;
        }
        return mRingBuffer[nextEntry];
    }

    // Get the index of the next entry.
    // This data may be stale since no locks are held.
    ULONG GetNextEntry() const { return mNextEntry; }

    // Get the index of the oldest entry.
    // This data may be stale since no locks are held.
    ULONG GetOldestEntry() const { return mOldestEntry; }

    // Is the ring buffer wrapped around?
    bool IsBufferWrapped() const { return mIsBufferWrapped; }

protected:

    // Buffer of Cache state machine event entries
    GenericEvent mRingBuffer[GENERIC_EVENT_LOG_SIZE];

    // Index to the next event to add to the buffer.
    ULONG mNextEntry;

    // Index to the oldest event in the buffer.
    ULONG mOldestEntry;

    // Flag to indicate when the buffer has wrapped around;
    bool mIsBufferWrapped;
};



#endif /* GENERIC_EVENT_H_ */
