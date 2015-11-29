#include <stdlib.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe_interface.h"
#include "fbe_lifecycle.h"
#include "fbe/fbe_class.h"

void fbe_cli_cmd_object_classes(int argc , char ** argv)
{
#define MAX_CLASS_ID 20
    static fbe_class_id_t class_ids[MAX_CLASS_ID];
    fbe_const_class_info_t * p_class_info;
    fbe_lifecycle_const_t * p_class_const;
    fbe_class_id_t class_id;
    fbe_status_t status;
    fbe_u32_t ii;

    if (argc < 1) {
        fbe_cli_print_usage();
        return;
    }
    class_id = FBE_CLASS_ID_INVALID;
    class_id = (fbe_u32_t)strtoul(argv[0], 0, 16);
    if ((class_id == FBE_CLASS_ID_INVALID) || (class_id >= FBE_CLASS_ID_LAST)) {
        fbe_cli_error("Invalid class id: 0x%03x\n", class_id);
        return;
    }
    status = fbe_get_class_by_id(class_id, &p_class_info);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Unknown class, id: 0x%03x\n", class_id);
        return;
    }
    status = fbe_get_class_lifecycle_const_data(class_id, &p_class_const);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Operation not supported for this class, id: 0x%03x\n", class_id);
        return;
    }
    status = fbe_lifecycle_class_const_hierarchy(p_class_const, class_ids, MAX_CLASS_ID);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Operation failed for this class, id: 0x%03x\n", class_id);
        return;
    }
    fbe_cli_printf("\nClass Hierarchy for class 0x%03x (%s):\n\n", class_id, p_class_info->class_name);
    fbe_cli_printf("      Num Name\n");
    for (ii = 0; ii < MAX_CLASS_ID; ii++) {
        if (class_ids[ii] == FBE_CLASS_ID_INVALID) {
            break;
        }
        status = fbe_get_class_by_id(class_ids[ii], &p_class_info);
        if (status != FBE_STATUS_OK) {
            fbe_cli_error("Unknown class, id: 0x%03x\n", class_ids[ii]);
            return;
        }
        fbe_cli_printf("    0x%03x %s\n", class_ids[ii], p_class_info->class_name);
    }
#undef MAX_CLASS_ID
}
