#ifndef BGS_MEMORY_H
#define BGS_MEMORY_H

//  BGS-TODO-POST-TAURUS: 
//  In R29, this file was named bgsl_memory.h and was part of the FBE infrastruture.  In R30, it was renamed to 
//  bgs_memory.h and made private to BGS.  It should be put in the BGS template, follow bgs coding conventions, 
//  return status types changed to bgs_status_t, etc.   JMM 08/05/2009.
// 


//  Number of bytes in a megabyte 
#define BYTES_PER_MB                    1048576             //  1024 bytes/kb * 1024 kb/mb

//  Number of megabytes that BGS is allowed in CMM memory.  All memory allocated via the bgs_memory functions
//  will come from the CMM memory pool, from the BGS portion of it.
#define BGS_MEMORY_IN_MB                30                  //  30 MB

//  BGS memory pool size - the number of bytes that BGS is allowed in the CMM memory pool 
#define BGS_MEMORY_POOL_SIZE            BGS_MEMORY_IN_MB * BYTES_PER_MB            

//  Size of each memory allocation for BGS.  When allocating from the CMM pool, all allocations will be the same 
//  fixed size, regardless of how much memory is actually requested. 
//
//  This is set to the size of an FBE packet, because that is the largest memory allocation needed.  The packet 
//  size is currently 1496 bytes in R30.  We are using a fixed number and not sizeof(), because if the packet size 
//  increases, we need to be aware of that and adjust the BGS CMM memory pool size accordingly. 
//
#define BGS_MEMORY_ALLOCATION_SIZE      1496

//  When CMM memory is allocated, there is a CMM descriptor (structure CMM_MEMORY_DESCRIPTOR) added onto the memory
//  allocated.  The size of this descriptor is 64 bytes.  It is defined in a private CMM file, cmm_memdesc.h, so 
//  we can not access the struct itself to use sizeof().  Therefore it is defined here. 
//
#define CMM_HEADER_DESCRIPTOR_SIZE      64
#define BGS_TOTAL_ALLOCATION_SIZE       (BGS_MEMORY_ALLOCATION_SIZE + CMM_HEADER_DESCRIPTOR_SIZE)

//  Maximum number of memory allocations that BGS can make, given the size of the pool and the size of each 
//  allocation
#define BGS_MAX_MEMORY_ALLOCATIONS      (BGS_MEMORY_POOL_SIZE / BGS_TOTAL_ALLOCATION_SIZE)


bgsl_status_t bgs_memory_init(void);
bgsl_status_t bgs_memory_destroy(void);

// These will be deleted when all references to these are replaced by
//  bgs_memory_service object based interface.
void * bgs_malloc(bgsl_u32_t size);
void bgs_free(void *ptr);


// FBE Memory interface
typedef struct bgs_memory_service_s
{
    void* (*malloc)(bgsl_u32_t size);
    void  (*free)(void* ptr);
} bgs_memory_service_t;

extern bgs_memory_service_t bgs_memory_service;



#endif /* BGS_MEMORY_H */
