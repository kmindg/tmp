#ifndef FBE_QUEUE_H
#define FBE_QUEUE_H

#include "fbe_types.h" 

typedef struct fbe_queue_element_s {
	FBE_ALIGN(8)void * next;
	FBE_ALIGN(8)void * prev;
}fbe_queue_element_t;

typedef fbe_queue_element_t fbe_queue_head_t;

__forceinline static void fbe_queue_init(fbe_queue_head_t * head)
{
	head->next = head;
	head->prev = head;
}

__forceinline static void fbe_queue_element_init(fbe_queue_element_t * queue_element)
{
	queue_element->next = queue_element;
	queue_element->prev = queue_element;
}

__forceinline static fbe_bool_t fbe_queue_is_element_on_queue(fbe_queue_element_t * queue_element)
{
	return (queue_element->next != queue_element);
}


__forceinline static void fbe_queue_destroy(fbe_queue_head_t * head)
{
	FBE_UNREFERENCED_PARAMETER(head);
}


__forceinline static void fbe_queue_push(fbe_queue_head_t * head, fbe_queue_element_t * element)
{
	fbe_queue_element_t * last = (fbe_queue_element_t *)head->prev;
	element->next = head;
	element->prev = last;
	last->next = element;
	head->prev = element;
}

__forceinline static void fbe_queue_push_front(fbe_queue_head_t * head, fbe_queue_element_t * element)
{
	fbe_queue_element_t * first = (fbe_queue_element_t *)head->next;
	element->next = first;
	element->prev = head;
	first->prev = element;
	head->next = element;
}

__forceinline static fbe_queue_element_t * fbe_queue_pop(fbe_queue_head_t * head)
{
	fbe_queue_element_t * first = (fbe_queue_element_t *)head->next;
	if(first == head){
		return NULL;
	} else {
		head->next = first->next;
		((fbe_queue_element_t *)first->next)->prev = head;
		first->next = first;
		first->prev = first;
		return first;
	}
}

__forceinline static fbe_bool_t fbe_queue_is_empty(fbe_queue_head_t * head)
{
	return (head->next == head);
}

__forceinline static fbe_queue_element_t * fbe_queue_front(fbe_queue_head_t * head)
{
	if(fbe_queue_is_empty(head)){
		return NULL;
	} else {
		return ((fbe_queue_element_t *)head->next);
	}
}

__forceinline static void fbe_queue_remove(fbe_queue_element_t * element)
{
	((fbe_queue_element_t *)(element->prev))->next = element->next;
	((fbe_queue_element_t *)(element->next))->prev = element->prev;
	element->next = element;
	element->prev = element;
}

/* return next element or NULL for error */
__forceinline static fbe_queue_element_t * fbe_queue_next(fbe_queue_head_t * head, fbe_queue_element_t * element)
{
	if(fbe_queue_is_empty(head)){
		return NULL;
	}
	if(element->next == head){
		return NULL;
	}
	if(element->next == element){
		return NULL;
	}
	return ((fbe_queue_element_t *)element->next);
}

/* return next element or NULL for error */
__forceinline static fbe_queue_element_t * fbe_queue_prev(fbe_queue_head_t * head, fbe_queue_element_t * element)
{
	if(fbe_queue_is_empty(head)){
		return NULL;
	}
	if(element->prev == head){
		return NULL;
	}
	if(element->prev == element){
		return NULL;
	}
	return ((fbe_queue_element_t *)element->prev);
}

__forceinline static void fbe_queue_insert(fbe_queue_element_t* element, fbe_queue_element_t* next_element)
{
    fbe_queue_element_t* prev_element = (fbe_queue_element_t *)next_element->prev;
    element->next = next_element;
    element->prev = prev_element;
    prev_element->next = element;
    next_element->prev = element;
}

__forceinline static fbe_u32_t fbe_queue_length(fbe_queue_head_t * head)
{
	fbe_u32_t length = 0;
	fbe_queue_element_t *element;

	if(fbe_queue_is_empty(head)){
		/* Return immediately if the queue is empty.
		 */
		return 0;
	}

	element = fbe_queue_front(head);

	/* Count all the items on the list.
	 */
	while (element != NULL){
		length++;
		element = fbe_queue_next(head, element);
	}
	return length;
}

__forceinline static void fbe_queue_merge(fbe_queue_head_t * target_head, fbe_queue_head_t * head)
{
	fbe_queue_element_t * element1, * element2;

	if(fbe_queue_is_empty(head)){
		/* Return immediately if the queue is empty.
		 */
		return ;
	}

    /* last in the source queue */
    element1 = (fbe_queue_element_t *)head->next;
    element2 = (fbe_queue_element_t *)target_head->prev;
    element2->next = element1;
    element1->prev = element2;
    element1 = (fbe_queue_element_t *)head->prev;
    target_head->prev = element1;
    element1->next = target_head;

    fbe_queue_init(head);

    return;
}

#endif /*FBE_QUEUE_H */