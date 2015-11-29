/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                      shmem.h
 ***************************************************************************
 *
 * DESCRIPTION: external declarations for shmem, share memory support
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    06/07/2008 Martin J. Buckley  Initial Definition
 *
 **************************************************************************/

#ifndef _SHMEM_H_
#define _SHMEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windef.h>
#include "csx_ext.h"
#include "generic_types.h"


typedef void  shmem_segment_desc;
typedef void  shmem_service_desc;
typedef void  shmem_syncevent_desc;

#define SHMEM_MAX_NAME 64

typedef struct shmem_semaphore_s
{
    csx_p_native_sem_handle_t semaphore;
    char    name[SHMEM_MAX_NAME];
} shmem_semaphore_t;


extern CSX_MOD_EXPORT
shmem_segment_desc *shmem_segment_create(const char *segment_id, UINT_64 segment_size);

extern CSX_MOD_EXPORT
const char *shmem_segment_get_id(shmem_segment_desc *segment);

extern CSX_MOD_EXPORT
void *shmem_segment_get_address(shmem_segment_desc *segment);

extern CSX_MOD_EXPORT
void *shmem_segment_get_base(shmem_segment_desc *segment);

extern CSX_MOD_EXPORT
UINT_64 shmem_segment_get_size(shmem_segment_desc *segment);

extern CSX_MOD_EXPORT
const char *shmem_format_segment_id(char *id, UINT_64 size);

extern CSX_MOD_EXPORT
char *shmem_get_segment_name_using_id(char *segment_id);

extern CSX_MOD_EXPORT
UINT_32 shmem_get_segment_index_using_id(char *segment_id);

extern CSX_MOD_EXPORT
char *shmem_get_segment_basename_using_id(char *id);

extern CSX_MOD_EXPORT
shmem_segment_desc *shmem_get_segment_using_id(const char *segment_id);

extern CSX_MOD_EXPORT
void shmem_segment_release(shmem_segment_desc *segment);

extern CSX_MOD_EXPORT
void shmem_segment_lock(shmem_segment_desc *segment);

extern CSX_MOD_EXPORT
void shmem_segment_unlock(shmem_segment_desc *segment);

extern CSX_MOD_EXPORT
void shmem_segment_dump(shmem_segment_desc *segment);



extern CSX_MOD_EXPORT
void shmem_service_dump(shmem_service_desc *service);

extern CSX_MOD_EXPORT
shmem_service_desc *shmem_service_add(shmem_segment_desc *segment, UINT_32 id, UINT_64 size);

extern CSX_MOD_EXPORT
shmem_service_desc *shmem_service_find(shmem_segment_desc *segment, UINT_32 id);

extern CSX_MOD_EXPORT
shmem_service_desc *shmem_named_service_add(shmem_segment_desc *segment, const char name[SHMEM_MAX_NAME], UINT_64 size);

extern CSX_MOD_EXPORT
shmem_service_desc *shmem_named_service_find(shmem_segment_desc *segment, const char name[SHMEM_MAX_NAME]);

extern CSX_MOD_EXPORT
void *shmem_service_get_base(shmem_segment_desc *segment, shmem_service_desc *service);

extern CSX_MOD_EXPORT
UINT_64 shmem_service_get_size(shmem_segment_desc *segment, shmem_service_desc *service);

extern CSX_MOD_EXPORT
void shmem_service_set_flags_with_mask(shmem_service_desc *service, UINT_64 flags, UINT_64 mask);

extern CSX_MOD_EXPORT
UINT_64 shmem_service_get_flags(shmem_service_desc *service);

extern CSX_MOD_EXPORT
UINT_64 shmem_service_get_set_flags(shmem_service_desc *service, UINT_64 flags);

extern CSX_MOD_EXPORT
void shmem_service_set_flags(shmem_service_desc *service, UINT_64 flags);

extern CSX_MOD_EXPORT
void shmem_service_set_flag(shmem_service_desc *service, UINT_32 index, UINT_32 value);

extern CSX_MOD_EXPORT
UINT_32 shmem_service_get_flag(shmem_service_desc *service, UINT_32 index);

extern CSX_MOD_EXPORT
shmem_syncevent_desc *shmem_service_get_syncevent(shmem_service_desc *service);

extern CSX_MOD_EXPORT
char *shmem_service_get_name(shmem_service_desc *service);

extern CSX_MOD_EXPORT
void shmem_service_lock(shmem_service_desc *service);

extern CSX_MOD_EXPORT
void shmem_service_unlock(shmem_service_desc *service);

extern CSX_MOD_EXPORT
void shmem_service_wait(shmem_service_desc *service);

extern CSX_MOD_EXPORT
void shmem_service_notify(shmem_service_desc *service);


extern CSX_MOD_EXPORT
shmem_semaphore_t *shmem_semaphore_allocate(void);

extern CSX_MOD_EXPORT
void shmem_semaphore_init(shmem_semaphore_t *sem, UINT_32 index);

extern CSX_MOD_EXPORT
void shmem_semaphore_destroy(shmem_semaphore_t* sem);

extern CSX_MOD_EXPORT
void shmem_semaphore_release(shmem_semaphore_t *sem);

extern CSX_MOD_EXPORT
void shmem_semaphore_wait(shmem_semaphore_t *sem);

extern CSX_MOD_EXPORT
UINT_32 shmem_semaphore_wait_with_timeout(shmem_semaphore_t *sem, UINT_32 milliseconds);


extern CSX_MOD_EXPORT
shmem_syncevent_desc *shmem_syncevent_allocate(void);

extern CSX_MOD_EXPORT
void shmem_syncevent_init(shmem_syncevent_desc *event, UINT_32 index);

extern CSX_MOD_EXPORT
void shmem_syncevent_destroy(shmem_syncevent_desc* event);

extern CSX_MOD_EXPORT
void shmem_syncevent_release(shmem_syncevent_desc *event);

extern CSX_MOD_EXPORT
void shmem_syncevent_wait(shmem_syncevent_desc *event);

extern CSX_MOD_EXPORT
UINT_32 shmem_syncevent_wait_with_timeout(shmem_syncevent_desc *event, UINT_32 milliseconds);


extern CSX_MOD_EXPORT
void shmem_sleep(unsigned long ms);

extern CSX_MOD_EXPORT
unsigned long shmem_thread_id(void);

#ifdef __cplusplus
};
#endif





#endif /*_SHMEM_H_*/
