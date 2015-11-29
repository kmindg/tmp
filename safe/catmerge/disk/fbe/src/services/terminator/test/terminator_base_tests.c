#include "terminator_test.h"
#include "terminator_base.h"
#include "fbe_terminator_common.h"

void baseComponentTest_init(void)
{
    base_component_t *uut = fbe_terminator_allocate_memory(sizeof(base_component_t));
    MUT_ASSERT_TRUE(uut);

    base_component_init(uut);
    MUT_ASSERT_TRUE(uut->queue_element.next == uut->queue_element.prev)
    MUT_ASSERT_TRUE(uut->child_list_head.next == uut->child_list_head.prev)
    MUT_ASSERT_NULL(uut->attributes)
    base_component_destroy(uut, FBE_TRUE);
}

void baseComponentTest_add_child(void)
{
    base_component_t *uut        = fbe_terminator_allocate_memory(sizeof(base_component_t));
    base_component_t *q_element1 = fbe_terminator_allocate_memory(sizeof(base_component_t));
    base_component_t *q_element2 = fbe_terminator_allocate_memory(sizeof(base_component_t));

    MUT_ASSERT_TRUE(uut
                 && q_element1
                 && q_element2);

    base_component_init(q_element1);
    base_component_init(uut);
    base_component_add_child(uut, &q_element1->queue_element);
    MUT_ASSERT_TRUE(uut->child_list_head.next == uut->child_list_head.prev)
    MUT_ASSERT_TRUE(uut->child_list_head.next == q_element1)
    MUT_ASSERT_TRUE(&uut->child_list_head == q_element1->queue_element.next)

    base_component_init(q_element2);
    base_component_add_child(uut, &q_element2->queue_element);
    MUT_ASSERT_TRUE(uut->child_list_head.next == q_element1)
    MUT_ASSERT_TRUE(q_element1->queue_element.next == q_element2)
    MUT_ASSERT_TRUE(q_element2->queue_element.next == &uut->child_list_head)

    MUT_ASSERT_TRUE(uut->child_list_head.prev == q_element2)
    MUT_ASSERT_TRUE(q_element2->queue_element.prev == q_element1)
    MUT_ASSERT_TRUE(q_element1->queue_element.prev == &uut->child_list_head)
    
    base_component_remove_child(&q_element2->queue_element);
    base_component_destroy(q_element2, FBE_TRUE);
    base_component_remove_child(&q_element1->queue_element);
    base_component_destroy(q_element1, FBE_TRUE);
    base_component_destroy(uut,        FBE_TRUE);
}

void baseComponentTest_add_remove_child(void)
{
    base_component_t *uut    = fbe_terminator_allocate_memory(sizeof(base_component_t));
    base_component_t *child1 = fbe_terminator_allocate_memory(sizeof(base_component_t));
    base_component_t *child2 = fbe_terminator_allocate_memory(sizeof(base_component_t));
    base_component_t *child3 = fbe_terminator_allocate_memory(sizeof(base_component_t));

    MUT_ASSERT_TRUE(uut
                 && child1
                 && child2
                 && child3);

    base_component_init(uut);
    MUT_ASSERT_TRUE(fbe_queue_is_empty(&uut->child_list_head))

    base_component_init(child1);
    base_component_add_child(uut, &child1->queue_element);
    base_component_init(child2);
    base_component_add_child(uut, &child2->queue_element);
    base_component_init(child3);
    base_component_add_child(uut, &child3->queue_element);

    base_component_remove_child(&child2->queue_element);

    MUT_ASSERT_FALSE(fbe_queue_is_empty(&uut->child_list_head));

    base_component_remove_child(&child1->queue_element);
    base_component_remove_child(&child3->queue_element);

    base_component_destroy(child3, FBE_TRUE);
    base_component_destroy(child2, FBE_TRUE);
    base_component_destroy(child1, FBE_TRUE);
    base_component_destroy(uut,    FBE_TRUE);
}

void baseComponentTest_get_child_count(void)
{
    base_component_t *uut    = fbe_terminator_allocate_memory(sizeof(base_component_t));
    base_component_t *child1 = fbe_terminator_allocate_memory(sizeof(base_component_t));
    base_component_t *child2 = fbe_terminator_allocate_memory(sizeof(base_component_t));
    base_component_t *child3 = fbe_terminator_allocate_memory(sizeof(base_component_t));

    MUT_ASSERT_TRUE(uut
                 && child1
                 && child2
                 && child3);

    base_component_init(uut);
    MUT_ASSERT_TRUE(fbe_queue_is_empty(&uut->child_list_head))

    base_component_init(child1);
    base_component_add_child(uut, &child1->queue_element);
    base_component_init(child2);
    base_component_add_child(uut, &child2->queue_element);
    base_component_init(child3);
    base_component_add_child(uut, &child3->queue_element);

    MUT_ASSERT_TRUE(3 == base_component_get_child_count(uut))

    base_component_remove_child(&child2->queue_element);
    MUT_ASSERT_TRUE(2 == base_component_get_child_count(uut))

    base_component_remove_child(&child3->queue_element);
    base_component_remove_child(&child1->queue_element);
    base_component_destroy(child3, FBE_TRUE);
    base_component_destroy(child2, FBE_TRUE);
    base_component_destroy(child1, FBE_TRUE);
    base_component_destroy(uut,    FBE_TRUE);
}

void baseComponentTest_chain_component(void)
{
    base_component_t *uut1 = fbe_terminator_allocate_memory(sizeof(base_component_t));
    base_component_t *uut2 = fbe_terminator_allocate_memory(sizeof(base_component_t));
    base_component_t *uut3 = fbe_terminator_allocate_memory(sizeof(base_component_t));

    MUT_ASSERT_TRUE(uut1
                 && uut2
                 && uut3);

    base_component_init(uut1);
    base_component_init(uut2);
    base_component_init(uut3);
    MUT_ASSERT_TRUE(fbe_queue_is_empty(&uut1->child_list_head))

    base_component_add_child(uut1, &uut2->queue_element);
    MUT_ASSERT_TRUE(1 == base_component_get_child_count(uut1))

    base_component_add_child(uut1, &uut3->queue_element);
    MUT_ASSERT_TRUE(2 == base_component_get_child_count(uut1))

    base_component_remove_child(&uut2->queue_element);
    MUT_ASSERT_TRUE(1 == base_component_get_child_count(uut1))

    base_component_remove_child(&uut3->queue_element);
    base_component_destroy(uut3, FBE_TRUE);
    base_component_destroy(uut2, FBE_TRUE);
    base_component_destroy(uut1, FBE_TRUE);
}

void baseComponentTest_base_component_get_first_child(void)
{
    base_component_t *uut    = fbe_terminator_allocate_memory(sizeof(base_component_t));
    base_component_t *child1 = fbe_terminator_allocate_memory(sizeof(base_component_t));
    base_component_t *child2 = fbe_terminator_allocate_memory(sizeof(base_component_t));
    base_component_t *child3 = fbe_terminator_allocate_memory(sizeof(base_component_t));
    base_component_t *child4 = fbe_terminator_allocate_memory(sizeof(base_component_t));

    base_component_t *matching_child = NULL;

    MUT_ASSERT_TRUE(uut
                 && child1
                 && child2
                 && child3
                 && child4);

    base_component_init(uut);
    base_component_init(child1);
    base_component_init(child2);
    base_component_init(child3);
    base_component_init(child4);

    MUT_ASSERT_TRUE(fbe_queue_is_empty(&uut->child_list_head))

//  child1.id = 2;
//  child2.id = 4;
//  child3.id = 6;
//  child4.id = 8;
    base_component_add_child(uut, &child1->queue_element);
    base_component_add_child(uut, &child2->queue_element);
    base_component_add_child(uut, &child3->queue_element);
    base_component_add_child(uut, &child4->queue_element);

    MUT_ASSERT_TRUE(4 == base_component_get_child_count(uut))

    matching_child = base_component_get_first_child(uut);
    MUT_ASSERT_TRUE(child1 == matching_child)

    base_component_remove_child(&child4->queue_element);
    base_component_remove_child(&child3->queue_element);
    base_component_remove_child(&child2->queue_element);
    base_component_remove_child(&child1->queue_element);

    base_component_destroy(child4, FBE_TRUE);
    base_component_destroy(child3, FBE_TRUE);
    base_component_destroy(child2, FBE_TRUE);
    base_component_destroy(child1, FBE_TRUE);
    base_component_destroy(uut,    FBE_TRUE);
}
