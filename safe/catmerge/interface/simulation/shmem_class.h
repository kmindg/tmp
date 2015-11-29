/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                      shmem_class.h
 ***************************************************************************
 *
 * DESCRIPTION: Declaration of classes shmem_segment, shmem_service.
 *              These classes encapsulates the K&R shmem API for use
 *              in C++
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/09/2009 Martin J. Buckley  Initial Definition
 *    03/14/2012 GSchaffer. get_flag(63) enable TRC_K_IO ntfy/wait/wake
 **************************************************************************/
#ifndef _SHMEMCLASS_H_
#define _SHMEMCLASS_H_

# include "shmem.h"
# include "generic_types.h"

#if defined(SHMEM_DLL_EXPORT)
#define SHMEM_CLASSPUBLIC CSX_CLASS_EXPORT
#else
#define SHMEM_CLASSPUBLIC CSX_CLASS_IMPORT
#endif

 
class shmem_service;

class SHMEM_CLASSPUBLIC shmem_segment
{
public:
    /* 
     * If SegmentSize is not passed, default segment size will be set 
     * to -1 (max value of Segment_size). If the size is -1, first argument 
     * segment_id passed will be in base_segment_name_<<instance>>:<<size>> format
     */
    shmem_segment(char *segment_id, UINT_64 segment_size);
    shmem_segment(shmem_segment_desc *segment_desc);
    virtual ~shmem_segment();

    char *get_id();
    void *get_address();
    void *get_base();
    UINT_64 get_size();
    shmem_segment_desc *get_descriptor();
    virtual void                lock();
    virtual void                unlock();

    void dump();
    static bool isSegmentExist(char *segment_id, UINT_32 segment_size);
    
private:
    void release();


    shmem_segment_desc  *mSegment_Desc;
    char                *mSegment_Id;
    UINT_64              mSegment_Size;
};

class SHMEM_CLASSPUBLIC shmem_syncevent
{
public:
    shmem_syncevent(shmem_service *service);
    shmem_syncevent(shmem_service_desc  *Service_Descriptor);
    ~shmem_syncevent();
    
    void                notify();
    bool                wait();
    bool                wait(ULONG millisecondTimeout);
    shmem_syncevent_desc   *get_descriptor(); 
    
    
private:
    shmem_syncevent_desc   *mSyncevent;
};

class SHMEM_CLASSPUBLIC shmem_service
{
public:

    shmem_service(shmem_segment *segment, char *name, UINT_64 size = 0);
    shmem_service(shmem_segment_desc  *segment_desc, char *name, UINT_64 size = 0);
    virtual ~shmem_service();

    shmem_segment               *get_segment();
    virtual char                *get_name();
    virtual void                *get_base();
    virtual UINT_64              get_size();
    // bit 63 reserved to enable TRC_K_IO ntfy/wait/wake
    virtual ULONGLONG           get_flags();
    virtual void                set_flags(ULONGLONG flags, ULONGLONG mask);
    virtual void                set_flags(ULONGLONG);
    virtual ULONGLONG           get_set_flags(ULONGLONG flags);
    virtual bool                get_flag(ULONG index);
    virtual void                set_flag(ULONG index, bool value);
    virtual shmem_service_desc  *get_descriptor();
    virtual void                lock();
    virtual void                unlock();
    virtual bool                wait();
    virtual bool                wait(ULONG millisecondTimeout);
    virtual void                notify();

protected:
    virtual void initialize();

private:

    void  find_service_initialize_as_needed();

    shmem_segment           *mSegment;
    shmem_segment_desc      *mSegment_Descriptor;
    shmem_service_desc      *mService_Descriptor;
    shmem_syncevent         *mSyncevent;
    char                    *mName;
    UINT_64                  mSize;
}; 

#endif // _SHMEMCLASS_H_
