#include "fbe/fbe_types.h"
#include "fbe_cms_environment.h"
#include "fbe_cms_state_machine.h"

typedef struct fbe_cms_sm_event_info_s{
	fbe_queue_element_t queue_element;/*must be first*/
	fbe_cms_sm_event_data_t event_data;
}fbe_cms_sm_event_info_t;

typedef struct fbe_cms_sm_event_memory_s{
	fbe_queue_element_t event_memory_queue_element;/*must be first*/
	fbe_cms_sm_event_info_t event_memory;
}fbe_cms_sm_event_memory_t;

fbe_status_t fbe_cms_sm_allocate_event_queue_memory(void);
fbe_status_t fbe_cms_sm_destroy_event_queue_memory(void);
fbe_cms_sm_event_info_t * fbe_cms_sm_get_event_memory(void);
void fbe_cms_sm_return_event_memory(fbe_cms_sm_event_info_t *retrunred_memory);

