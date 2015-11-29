/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               BvdSimBadBlockMonitor.h
 ***************************************************************************
 *
 * DESCRIPTION: Test utility classes used to monitor/create BadBlocks 
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    09/05/2011  Martin Buckley Initial Version
 *
 **************************************************************************/
# include "simulation/HashTable.h"
# include "simulation/AbstractFlags.h"
# include "simulation/BvdSim_Registry.h"

enum LBATracker_enum {
    FIRST_BAD_CRC = 0,
    INVALIDATE_SECTOR = 1,
};

class LBATracker;
typedef LBATracker *PLBATracker;

class LBATrackerFactory: public SimpleFlags {
public:
    LBATrackerFactory(UINT_64 LBA): mLBA(LBA) {}
    ~LBATrackerFactory() {}

    virtual PLBATracker factory(UINT_64 LBA) = 0;

    UINT_64         Key() const {
        return mLBA;
    }

    LBATrackerFactory *mHashNext;

private:
    UINT_64 mLBA;
};

class LBATracker: public SimpleFlags  {
public:
    LBATracker(UINT_64 LBA): mLBA(LBA) { }

    virtual void processSector(UINT_8 *sector) = 0;

    UINT_64 getLBA() {
        return mLBA;
    }

    UINT_64         Key() const { return mLBA; }

    PLBATracker   mHashNext;    // for use in IOSectorMagement

private:
    UINT_64 mLBA;
};

class CorruptCRC: public LBATracker{
public:
    CorruptCRC(UINT_64 LBA): LBATracker(LBA) {}

    void processSector(UINT_8 *sector) {

        if(!get_flag(FIRST_BAD_CRC)) {
            /*
             * Corrupt this sector's CRC
             */
             IOTaskId::corruptChecksum(sector);
             set_flag(FIRST_BAD_CRC, true);
        }
        else {
            if(get_flag(INVALIDATE_SECTOR)) {
                /*
                 * Invalidate this sector
                 */
                 IOTaskId::MakeSectorKnownBad(sector, BvdSim_Registry::getSpId(), getLBA());
            }
        }
    }
};

class CorruptCRCFactory: public LBATrackerFactory {
public:
    CorruptCRCFactory(UINT_64 LBA): LBATrackerFactory(LBA) {}

    PLBATracker factory(UINT_64 LBA) {
        CorruptCRC *cc = new CorruptCRC(LBA);
        cc->set_flags(get_flags());
        return cc;
    }
};

class AlwaysInvalidateSector: public LBATracker{
public:
    AlwaysInvalidateSector(UINT_64 LBA): LBATracker(LBA) {}

    void processSector(UINT_8 *sector) {
        /*
         * Invalidate this sector
         */
        IOTaskId::MakeSectorKnownBad(sector, BvdSim_Registry::getSpId(), getLBA());
    }
};

class AlwaysInvalidateSectorFactory: public LBATrackerFactory {
public:
    AlwaysInvalidateSectorFactory(UINT_64 LBA): LBATrackerFactory(LBA) {}

    PLBATracker factory(UINT_64 LBA) {
        return new AlwaysInvalidateSector(LBA);
    }
};



const int LBATrackerHashTableSize = 1024;
typedef HashTable<LBATrackerFactory,
                        UINT_64,
                        LBATrackerHashTableSize,
                        &LBATrackerFactory::mHashNext> HashOfTrackerFactory;

typedef HashTable<LBATracker,
                        UINT_64,
                        LBATrackerHashTableSize,
                        &LBATracker::mHashNext> HashOfLBATrackers;

// The user of this class may need to call MCCDisableBEReadCRCCheck in
// CacheOpsLib.lib in order to disable MCC Backend CRC Checking
class BvdSimBadBlockTestMonitor: public VirtualFlareIODummyMonitor {
public:
    BvdSimBadBlockTestMonitor() {}
    ~BvdSimBadBlockTestMonitor() {
        mBadBlockTable.Clear();
        mTargets.Clear();
    }

    void VFD_Read_Sector(UINT_64 LBA, UINT_8 *buffer) {
        LBATrackerFactory *f = mTargets.Find(LBA);
        if(f != NULL) {
            /*
             * This LBA is significant
             */
            PLBATracker p = mBadBlockTable.Find(LBA);
            if(p == NULL) {
                /*
                 * Create & Register the LBATracker,
                 */
                p = f->factory(LBA);
                mBadBlockTable.Add(p);
            }

            p->processSector(buffer);
        }
    }

    void VFD_Write_Sector(UINT_64 LBA, UINT_8 *buffer) {
        /*
         * A write operation resets any BadBlock processing
         *  that was in effect.  
         * Delete the Tracker.  On next read, the
         *  factory will re-populate the Tracker
         */
        LBATrackerFactory *f = mTargets.Find(LBA);
        if(f != NULL) {
            PLBATracker p = mBadBlockTable.Find(LBA);
            if(p != NULL) {
                mBadBlockTable.Remove(p);
            }
        }
    }

    void corruptCRC(UINT_64 LBA, bool corruptSector = false) {
        CorruptCRCFactory *f = new CorruptCRCFactory(LBA);
        f->set_flag(INVALIDATE_SECTOR, corruptSector);
         mTargets.Add(f);
    }

    void invalidatedSector(UINT_64 LBA) {
         mTargets.Add(new AlwaysInvalidateSectorFactory(LBA));
    }

private:
    HashOfTrackerFactory mTargets;
    HashOfLBATrackers mBadBlockTable;
};

