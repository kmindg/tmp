#ifndef FBE_BASE_H
#define FBE_BASE_H

#include "fbe/fbe_types.h"

typedef enum fbe_component_type_e{
	FBE_COMPONENT_TYPE_INVALID,
	FBE_COMPONENT_TYPE_PACKAGE,
	FBE_COMPONENT_TYPE_SERVICE,
	FBE_COMPONENT_TYPE_CLASS,
	FBE_COMPONENT_TYPE_OBJECT,
	FBE_COMPONENT_TYPE_LIBRARY,
	FBE_COMPONENT_TYPE_LAST
}fbe_component_type_t;

typedef struct fbe_base_s{
	fbe_u64_t magic_number;
	/* fbe_component_type_t component_type; */

}fbe_base_t;

typedef fbe_base_t * fbe_handle_t;
typedef fbe_handle_t fbe_object_handle_t;
typedef fbe_handle_t fbe_service_handle_t;


static __forceinline fbe_base_t * fbe_base_handle_to_pointer(fbe_handle_t handle)
{
	return (fbe_base_t *)handle;
}

static __forceinline fbe_handle_t fbe_base_pointer_to_handle(fbe_base_t * base)
{
	return (fbe_object_handle_t)base;
}
#if 0
static __forceinline fbe_component_type_t fbe_base_get_component_type(fbe_object_handle_t handle)
{
	fbe_base_t * base = fbe_base_handle_to_pointer(handle);

	if(base == NULL) {
		return FBE_COMPONENT_TYPE_INVALID;
	}
	return base->component_type;
}

static __forceinline fbe_status_t fbe_base_set_component_type(fbe_base_t * base, fbe_component_type_t component_type)
{
	if(base == NULL) {
		return FBE_STATUS_GENERIC_FAILURE;
	}
	base->component_type = component_type;
	return FBE_STATUS_OK;
}
#endif
static __forceinline fbe_status_t fbe_base_set_magic_number(fbe_base_t * base, fbe_u64_t magic_number)
{
	if(base == NULL) {
		return FBE_STATUS_GENERIC_FAILURE;
	}
	base->magic_number = magic_number;
	return FBE_STATUS_OK;
}

static __forceinline fbe_status_t fbe_base_get_magic_number(fbe_base_t * base, fbe_u64_t * magic_number)
{
	*magic_number = 0;
	if(base == NULL) {
		return FBE_STATUS_GENERIC_FAILURE;
	}

	*magic_number = base->magic_number;
	return FBE_STATUS_OK;
}

#endif /* FBE_BASE_H */