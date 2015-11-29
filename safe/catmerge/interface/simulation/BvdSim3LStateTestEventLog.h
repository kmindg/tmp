#ifndef __BvdSim3LStateTestEventLog_h__
#define __BvdSim3LStateTestEventLog_h__

//***********************************************************************
//
//  Copyright (c) 2009, 2011-2015 EMC Corporation.
//  All rights reserved.
//
//  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
//  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
//  BE KEPT STRICTLY CONFIDENTIAL.
//
//  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT CONTRACT WITH EMC
//  CORPORATION.  IF SUCH MODIFICATIONS ARE FOR THE PURPOSE OF CIRCUMVENTING LICENSING
//  LIMITATIONS, LEGAL ACTION MAY RESULT.
//
//***********************************************************************

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BvdSimStateTestEventLog.h
//
// Contents:
//  This header file contains classes used by test code to process BvdSim state
//  machine event logs. 
//
// History:
//04-Feb-2011    Sonal Saxena
//
#include "simulation/BvdSim3LStateMachineTestBase.h"
#include "generic_event.h"


// BvdSimEventLogTracer class generates ktrace output of an event log.
class BvdSimEventLogTracer {

public:
    BvdSimEventLogTracer(GenericStateEventLog * eventLog) : mIndex(0), mNumberOfEntries(0) {
        mEventLog = eventLog;
        EmcpalZeroMemory(&mLogText, sizeof(mLogText));
    }
    virtual ~BvdSimEventLogTracer() {};

    // Display a CSM event log object with all the state variables.
    virtual void DisplayEventLog() {};

    // Display an expected event log object.
    virtual void DisplayExpectedEventLog() {};

private:

    // Translate a boolean value to a text string.
    char* GetBoolText(bool value)
    {
        if (value) {
            return(" 1  ");
        }
        else {
            return(" 0  ");
        }
    }

protected:

    // The event log to operate on.
    GenericStateEventLog * mEventLog;

    // The event log entry to process.
    GenericEvent mLogEntry;

    // Number of entries in the event log.
    ULONG mNumberOfEntries;

    // Index of the log entry to process.
    ULONG mIndex;

    // Text buffer of the parsed log entry.
    char mLogText[140];
};


// BvdSimStateTestEventLog defines a logging facility that captures expected
// BvdSimStateMachine events in a ring buffer and compares them against
// BvdSimStateMachine events logged by the TargetDriver.
class BvdSimStateTestEventLog : public GenericStateEventLog {

public:
    // Constructor
    BvdSimStateTestEventLog() { };

    // Destructor
    ~BvdSimStateTestEventLog() {};

    BvdSimStateTestEventLog (const GenericStateEventLog & TestEvent) : GenericStateEventLog(TestEvent)
    {}

    // Add old/new state pair to ring buffer.
    void AddTestEvent(GenericEvent genericEvent) {
        AddEventLockHeld(genericEvent);
    }

    // operator == compares the one BvdSimStateTestEventLog object with another and returns
    // true only when all transitions match.
    //
    // Returns: 
    // true if matched else false
    //
    bool operator == (const BvdSimStateTestEventLog &right) const
    {
        ULONG rightCount = right.mNextEntry;
        ULONG rightOffset = 0;
        ULONG Count = mNextEntry;
        bool retVal = true;
        ULONG rightStart = 0;
        
        if(right.mIsBufferWrapped) {            
            rightCount = GENERIC_EVENT_LOG_SIZE;
            rightOffset = right.mOldestEntry;
        }
        
        // This internally calls the operator == of CSMEvent Class which compares the one
        // CSM event with another.
        // Once an individual CSM event has matched we'll break from the inner loop and
        // continue comparing further events present in RingBuffer.
        // If that individual CSM event is not found then will check with the next csm
        // event present in right's RingBuffer.
        // It will return true only when all the CSM events present in RingBuffer match
        // with the CSM events present in right's RingBuffer.
        for(ULONG start = 0; start < Count; start++) {
            bool result = false;
            for(ULONG i = rightStart; i < rightCount; i++) {
                ULONG actualIndex = (i + rightOffset) % rightCount;
                if(mRingBuffer[start]== right.mRingBuffer[actualIndex])
                {
                    // If we find the match then break the loop.
                    rightStart = (i + 1);
                    result = true;
                    break;
                }
            }   
            
            // If we didn't find the expected event then break the main loop and return
            // false.
            if(!result) {
                retVal = false;
                break;
            }
        }
        
        return retVal;
    }

};


// This class stores all the expected events that our test expects. We compare each of
// events of type BvdSimStateTestEventLog with the GenericEvent event and for the comparison
// we internally call == operator of class BvdSimStateTestEventLog.
class BvdSimExpectedEventContainer {
public:
    //constructor
    BvdSimExpectedEventContainer() : mCount(0) {};

    // Add the expected events in an array of type BvdSimStateTestEventLog
    //
    // @param expected  - pointer to BvdSimStateTestEventLog
    void AddExpectedLog(BvdSimStateTestEventLog *expected){
        mExpectedEventLog[mCount] = expected;
        mCount++;
    }

    // Display the expected event logs
    virtual void DisplayExpectedLogs(){
        for (ULONG i=0; i < mCount; i++) {
            BvdSim_Trace(MUT_LOG_HIGH, "CSM: Expected Event Log [%d]:", i);
            BvdSimEventLogTracer * eventLogTracer =  new BvdSimEventLogTracer(mExpectedEventLog[i]);
            eventLogTracer->DisplayExpectedEventLog();
            delete eventLogTracer;
        }
    }

    // operator == compares each entry of type BvdSimStateTestEventLog with right side
    // operand and return true if any of them matches.
    // This internally calls the operator == of class BvdStateTestEventLog.
    //
    // Returns: 
    // true if matched else false
    //
    bool operator == (const BvdSimStateTestEventLog &right) const{
        bool retVal = false;
        for(int i = 0; i<mCount; i++){
            if((*(mExpectedEventLog[i])== right)){
                   retVal = true;
                   break;
            }
        }
        return retVal;
    }
protected:
    BvdSimStateTestEventLog *mExpectedEventLog[10];
    ULONG mCount;
};

#endif  // __BvdSim3LStateTestEventLog_h__
