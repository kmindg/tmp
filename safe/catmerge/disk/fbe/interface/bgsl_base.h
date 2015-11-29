#ifndef BGSL_BASE_H
#define BGSL_BASE_H

#include "fbe/bgsl_types.h"

typedef enum bgsl_component_type_e{
	BGSL_COMPONENT_TYPE_INVALID,
	BGSL_COMPONENT_TYPE_PACKAGE,
	BGSL_COMPONENT_TYPE_SERVICE,
	BGSL_COMPONENT_TYPE_CLASS,
	BGSL_COMPONENT_TYPE_OBJECT,
	BGSL_COMPONENT_TYPE_LIBRARY,
	BGSL_COMPONENT_TYPE_LAST
}bgsl_component_type_t;

typedef struct bgsl_base_s{
	bgsl_u64_t magic_number;
	bgsl_component_type_t component_type;

}bgsl_base_t;

typedef bgsl_base_t * bgsl_object_handle_t;

static __forceinline bgsl_base_t * bgsl_base_handle_to_pointer(bgsl_object_handle_t handle)
{
	return (bgsl_base_t *)handle;
}

static __forceinline bgsl_object_handle_t bgsl_base_pointer_to_handle(bgsl_base_t * base)
{
	return (bgsl_object_handle_t)base;
}

static __forceinline bgsl_component_type_t bgsl_base_get_component_type(bgsl_object_handle_t handle)
{
	bgsl_base_t * base = bgsl_base_handle_to_pointer(handle);

	if(base == NULL) {
		return BGSL_COMPONENT_TYPE_INVALID;
	}
	return base->component_type;
}

static __forceinline bgsl_status_t bgsl_base_set_component_type(bgsl_base_t * base, bgsl_component_type_t component_type)
{
	if(base == NULL) {
		return BGSL_STATUS_GENERIC_FAILURE;
	}
	base->component_type = component_type;
	return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t bgsl_base_set_magic_number(bgsl_base_t * base, bgsl_u64_t magic_number)
{
	if(base == NULL) {
		return BGSL_STATUS_GENERIC_FAILURE;
	}
	base->magic_number = magic_number;
	return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t bgsl_base_get_magic_number(bgsl_base_t * base, bgsl_u64_t * magic_number)
{
	*magic_number = 0;
	if(base == NULL) {
		return BGSL_STATUS_GENERIC_FAILURE;
	}

	*magic_number = base->magic_number;
	return BGSL_STATUS_OK;
}

#endif /* BGSL_BASE_H */