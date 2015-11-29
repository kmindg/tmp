#include "terminator_test.h"
#include "terminator_board.h"

void board_new_test(void)
{
    terminator_board_t * uut = NULL;

    uut = board_new();

    MUT_ASSERT_NOT_NULL(&uut->base);
    board_destroy(uut);
}

//void board_set_info_test(void)
//void board_get_board_type_test(void)
//void board_destroy_test(void)
