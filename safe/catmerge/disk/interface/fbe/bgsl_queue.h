#ifndef BGSL_QUEUE_H
#define BGSL_QUEUE_H

#include "bgsl_types.h" 

typedef struct bgsl_queue_element_s {
	void * next;
	void * prev;
}bgsl_queue_element_t;

typedef bgsl_queue_element_t bgsl_queue_head_t;

__forceinline static void bgsl_queue_init(bgsl_queue_head_t * head)
{
	head->next = head;
	head->prev = head;
}

__forceinline static void bgsl_queue_element_init(bgsl_queue_element_t * queue_element)
{
	queue_element->next = queue_element;
	queue_element->prev = queue_element;
}

__forceinline static bgsl_bool_t bgsl_queue_is_element_on_queue(bgsl_queue_element_t * queue_element)
{
	return (queue_element->next != queue_element);
}


__forceinline static void bgsl_queue_destroy(bgsl_queue_head_t * head)
{
	BGSL_UNREFERENCED_PARAMETER(head);
}


__forceinline static void bgsl_queue_push(bgsl_queue_head_t * head, bgsl_queue_element_t * element)
{
	bgsl_queue_element_t * last = (bgsl_queue_element_t *)head->prev;
	element->next = head;
	element->prev = last;
	last->next = element;
	head->prev = element;
}

__forceinline static void bgsl_queue_push_front(bgsl_queue_head_t * head, bgsl_queue_element_t * element)
{
	bgsl_queue_element_t * first = (bgsl_queue_element_t *)head->next;
	element->next = first;
	element->prev = head;
	first->prev = element;
	head->next = element;
}

__forceinline static bgsl_queue_element_t * bgsl_queue_pop(bgsl_queue_head_t * head)
{
	bgsl_queue_element_t * first = (bgsl_queue_element_t *)head->next;
	if(first == head){
		return NULL;
	} else {
		head->next = first->next;
		((bgsl_queue_element_t *)first->next)->prev = head;
		first->next = first;
		first->prev = first;
		return first;
	}
}

__forceinline static bgsl_bool_t bgsl_queue_is_empty(bgsl_queue_head_t * head)
{
	return (head->next == head);
}

__forceinline static bgsl_queue_element_t * bgsl_queue_front(bgsl_queue_head_t * head)
{
	if(bgsl_queue_is_empty(head)){
		return NULL;
	} else {
		return ((bgsl_queue_element_t *)head->next);
	}
}

__forceinline static void bgsl_queue_remove(bgsl_queue_element_t * element)
{
	((bgsl_queue_element_t *)(element->prev))->next = element->next;
	((bgsl_queue_element_t *)(element->next))->prev = element->prev;
	element->next = element;
	element->prev = element;
}

/* return next element or NULL for error */
__forceinline static bgsl_queue_element_t * bgsl_queue_next(bgsl_queue_head_t * head, bgsl_queue_element_t * element)
{
	if(bgsl_queue_is_empty(head)){
		return NULL;
	}
	if(element->next == head){
		return NULL;
	}
	if(element->next == element){
		return NULL;
	}
	return ((bgsl_queue_element_t *)element->next);
}

// The functions below were added for the BGS Queuing Utility Library.  See bgs_queue_utils.c for details.
__forceinline static void bgsl_queue_insert(bgsl_queue_element_t* element, bgsl_queue_element_t* next_element)
{
    bgsl_queue_element_t* prev_element = (bgsl_queue_element_t *)next_element->prev;
    element->next = next_element;
    element->prev = prev_element;
    prev_element->next = element;
    next_element->prev = element;
}
#endif /*BGSL_QUEUE_H */
