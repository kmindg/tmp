#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe_interface.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"

static void list_trace_levels(int argc, char ** argv)
{
    fbe_trace_level_t trace_level;
    const char * p_trace_stamp;

    fbe_cli_printf("\nFBE Trace levels:\n\n");
    fbe_cli_printf("    Num Name\n");
    for (trace_level = FBE_TRACE_LEVEL_CRITICAL_ERROR; trace_level < FBE_TRACE_LEVEL_LAST; trace_level++) {
        p_trace_stamp = fbe_trace_get_level_stamp(trace_level);
        fbe_cli_printf("    %d %s\n", trace_level, p_trace_stamp);
    }
}

static void list_libraries(int argc, char ** argv)
{
    fbe_const_library_info_t * p_library_info;
    fbe_library_id_t library_id;
    fbe_status_t status;

    fbe_cli_printf("\nFBE Libs list:\n\n");
    fbe_cli_printf("      Id Name\n");
    for (library_id = 0; library_id < FBE_LIBRARY_ID_LAST; library_id++) {
        status = fbe_get_library_by_id(library_id, &p_library_info);
        if (status == FBE_STATUS_OK) {
            fbe_cli_printf("    0x%02x %s\n", library_id, p_library_info->library_name);
        }
    }
}

static void list_services(int argc, char ** argv)
{
	/*
    fbe_const_service_info_t * p_service_info;
    fbe_service_id_t service_id;
    fbe_status_t status;

    fbe_cli_printf("\nFBE Service list:\n\n");
    fbe_cli_printf("      Id Name\n");
	
    for (service_id = FBE_SERVICE_ID_FIRST + 1; service_id < FBE_SERVICE_ID_LAST; service_id++) {
        status = fbe_get_service_by_id(service_id, &p_service_info);
        if (status == FBE_STATUS_OK) {
            fbe_cli_printf("    0x%02x %s\n", service_id, p_service_info->service_name);
        }
    }
	*/
}

static void list_classes(int argc, char ** argv)
{
    fbe_const_class_info_t * p_class_info;
    fbe_class_id_t class_id;
    fbe_status_t status;

    fbe_cli_printf("\nFBE Class list:\n\n");
    fbe_cli_printf("      Num Name\n");
    for (class_id = 0; class_id < FBE_CLASS_ID_LAST; class_id++) {
        status = fbe_get_class_by_id(class_id, &p_class_info);
        if (status == FBE_STATUS_OK) {
            fbe_cli_printf("    0x%03x %s\n", class_id, p_class_info->class_name);
        }
    }
}

static void list_objects(int argc, char ** argv)
{
	fbe_object_id_t	*	object_list = NULL;
	fbe_u32_t			total_objects;
	fbe_class_id_t      class_id;
	fbe_lifecycle_state_t  lifecycle_state;
	fbe_status_t        status;
	fbe_u32_t           ii;
    fbe_const_class_info_t *p_class_info;
    const fbe_u8_t      *p_class_name;
    const fbe_u8_t      *p_lifecycle_state_name;
	fbe_u32_t			object_count = 0;

	total_objects = 0;

	status = fbe_api_get_total_objects(&object_count, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
		return;
	}

	object_list = (fbe_object_id_t *)fbe_api_allocate_memory(sizeof(fbe_object_id_t) * object_count);

    status = fbe_api_enumerate_objects (object_list, object_count, &total_objects, FBE_PACKAGE_ID_PHYSICAL);
	if (status != FBE_STATUS_OK) {
		fbe_api_free_memory(object_list);
		return;
	}

	fbe_cli_printf("Discovered 0x%.4X objects:\n", total_objects);

    fbe_cli_printf("Id  \tClassType \t\tLifecycleState\n");
	for (ii = 0; ii < total_objects; ii++) {
		status = fbe_api_get_object_class_id(object_list[ii], &class_id, FBE_PACKAGE_ID_PHYSICAL);
		if (status == FBE_STATUS_OK) {
            status = fbe_get_class_by_id(class_id, &p_class_info);
            if (status == FBE_STATUS_OK) {
                p_class_name = p_class_info->class_name;
            }
            else {
                p_class_name = "<error>";
            }
        }
        else {
            p_class_name = "<error>";
		}
		status = fbe_api_get_object_lifecycle_state(object_list[ii], &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
		if (status == FBE_STATUS_OK) {
			/*
            status = fbe_lifecycle_get_state_name(lifecycle_state, &p_lifecycle_state_name);
            if (status != FBE_STATUS_OK) {
                p_lifecycle_state_name = "<error>";
            }
			*/
			 p_lifecycle_state_name = "<error>";
        }
        else {
            p_lifecycle_state_name = "<error>";
        }
        fbe_cli_printf("0x%02X \t0x%02X:%-20s  0x%02X:%s\n",
                       object_list[ii], class_id, p_class_name, lifecycle_state, p_lifecycle_state_name);
	}

	fbe_api_free_memory(object_list);
}

void fbe_cli_cmd_list(int argc , char ** argv)
{
    argc--;
    if ((*argv == NULL) || (strcmp(*argv, "-help") == 0) || (strcmp(*argv, "-h") == 0)) {
        fbe_cli_printf("%s", LIST_USAGE);
    }
    else if ((strcmp(*argv, "-trace") == 0) || (strcmp(*argv, "-t") == 0)) {
        argv++;
        list_trace_levels(argc, argv);
    }
    else if ((strcmp(*argv, "-classes") == 0) || (strcmp(*argv, "-c") == 0)) {
        argv++;
        list_classes(argc, argv);
    }
    else if ((strcmp(*argv, "-objects") == 0) || (strcmp(*argv, "-o") == 0)) {
        argv++;
        list_objects(argc, argv);
    }
    else if ((strcmp(*argv, "-services") == 0) || (strcmp(*argv, "-s") == 0)) {
        argv++;
        list_services(argc , argv);
    }
    else if ((strcmp(*argv, "-library") == 0) || (strcmp(*argv, "-l") == 0)) {
        argv++;
        list_libraries(argc, argv);
    }
    else {
        fbe_cli_error("unknown argv parameter: \"%s\"\n", *argv);
    }
}
