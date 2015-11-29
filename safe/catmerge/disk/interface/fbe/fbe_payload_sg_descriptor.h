#ifndef FBE_PAYLOAD_SG_DESCRIPTOR_H
#define FBE_PAYLOAD_SG_DESCRIPTOR_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
#include "fbe/fbe_types.h"

typedef struct fbe_payload_sg_descriptor_s {
	fbe_u32_t repeat_count; /* Repeat count */
}fbe_payload_sg_descriptor_t;

static __forceinline fbe_status_t
fbe_payload_sg_descriptor_get_repeat_count(fbe_payload_sg_descriptor_t * payload_sg_descriptor, fbe_u32_t * repeat_count)
{
	* repeat_count = payload_sg_descriptor->repeat_count;
	return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_payload_sg_descriptor_set_repeat_count(fbe_payload_sg_descriptor_t * payload_sg_descriptor, fbe_u32_t  repeat_count)
{
	payload_sg_descriptor->repeat_count = repeat_count;
	return FBE_STATUS_OK;
}

#endif /* FBE_PAYLOAD_SG_DESCRIPTOR_H */
