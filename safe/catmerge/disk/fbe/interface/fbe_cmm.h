#ifndef FBE_CMM_H
#define FBE_CMM_H

#include "fbe/fbe_types.h"

#if !(defined(UMODE_ENV) || defined(SIMMODE_ENV))
#include "cmm_types.h"
#include "cmm.h"

#else /* UMODE_ENV */
#include <malloc.h>

#define	CMM_MAXIMUM_MEMORY	-1

#define CMM_STATUS_BASE						0x42420000

#define	CMM_STATUS_SUCCESS					CMM_STATUS_BASE + 0
#define	CMM_STATUS_OBJECT_DOES_NOT_EXIST	CMM_STATUS_BASE + 1
#define	CMM_STATUS_REQUEST_CANCELED			CMM_STATUS_BASE + 2
#define	CMM_STATUS_INSUFFICIENT_MEMORY		CMM_STATUS_BASE + 3
#define	CMM_STATUS_ILLEGAL_OPERATION		CMM_STATUS_BASE + 4
#define	CMM_STATUS_QUOTA_EXHAUSTED			CMM_STATUS_BASE + 5
#define	CMM_STATUS_REQUEST_DEFERRED			CMM_STATUS_BASE + 6
#define	CMM_STATUS_RESOURCE_EXHAUSTED		CMM_STATUS_BASE + 7

#define	CMM_STATUS_NO_SUCH_FUNCTION			CMM_STATUS_BASE + 8
#define	CMM_STATUS_ASSERT_FAILED			CMM_STATUS_BASE + 9
#define	CMM_STATUS_MEMORY_CORRUPTION_DETECTED	CMM_STATUS_BASE + 10
#define	CMM_STATUS_MEMORY_SIZE_MISMATCH_DETECTED	CMM_STATUS_BASE + 11

typedef fbe_u32_t CMM_POOL_ID;
typedef fbe_u32_t CMM_CLIENT_ID;
typedef fbe_u32_t CMM_ERROR;

typedef fbe_u64_t CMM_MEMORY_ADDRESS;
typedef fbe_u64_t CMM_MEMORY_SIZE;


#pragma pack(4)
typedef struct _CMM_SGL
{
    fbe_u64_t MemSegLength;
    fbe_u64_t MemSegAddress;
} CMM_SGL, *PCMM_SGL;
#pragma pack()

enum pool_id_e{
	LongTermControlPoolID,
	SEPDataPoolID
};

static CMM_SGL data_pool_sgl[2];

static CMM_POOL_ID	cmmGetLongTermControlPoolID (void)
{ 
	return LongTermControlPoolID;
}

static CMM_POOL_ID  cmmGetSEPDataPoolID (void)
{
	return SEPDataPoolID; /* Just not NULL */
}

static CMM_ERROR cmmRegisterClient (CMM_POOL_ID poolId,
									void * 	freeCallback,
									fbe_u32_t			minimumAllocSize,
									fbe_u32_t			maximumAllocSize,
									char				*clientName,
									fbe_u32_t			*clientIdPtr)
{
	*clientIdPtr = (fbe_u32_t)poolId;
	return CMM_STATUS_SUCCESS;
}

static CMM_ERROR cmmGetMemoryVA ( CMM_CLIENT_ID	clientId, fbe_u32_t numberOfBytes, void ** memoryVA)
{
	* memoryVA = malloc(numberOfBytes);
	return CMM_STATUS_SUCCESS;
}

static CMM_ERROR cmmFreeMemoryVA(CMM_CLIENT_ID	clientId, void * memoryVA)
{
	free(memoryVA);
	return CMM_STATUS_SUCCESS;
}

static CMM_ERROR cmmDeregisterClient ( CMM_CLIENT_ID	clientId)
{
	if(clientId == SEPDataPoolID){
		free(CSX_CAST_PTRMAX_TO_PTR(data_pool_sgl[0].MemSegAddress));
	}
	return CMM_STATUS_SUCCESS;
}


static PCMM_SGL cmmGetPoolSGL(CMM_POOL_ID poolId)
{
	data_pool_sgl[0].MemSegLength = 200 * 1024 * 1024; /* 260 MB for now */
	data_pool_sgl[0].MemSegAddress = (fbe_u64_t)malloc((size_t)data_pool_sgl[0].MemSegLength);

	data_pool_sgl[1].MemSegAddress = 0;
	data_pool_sgl[1].MemSegLength = 0;
	return data_pool_sgl;
}

static CMM_ERROR       cmmMapMemorySVA (
                                CMM_MEMORY_ADDRESS    PhysAddress,
                                CMM_MEMORY_SIZE       PhysLength,
                                void *                 *virtualAddress,
                                void *        *mapObj)
{
	*virtualAddress = CSX_CAST_PTRMAX_TO_PTR(PhysAddress);
	return CMM_STATUS_SUCCESS;
}

#endif /* UMODE_ENV */

#endif /* FBE_CMM_H */
