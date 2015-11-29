#include "fbe_cli_private.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_library_interface.h"

extern fbe_status_t
fbe_get_service_by_id(fbe_service_id_t service_id, fbe_const_service_info_t ** pp_service_info);

static fbe_trace_level_t trace_get_level(fbe_trace_type_t trace_type, fbe_object_id_t id, fbe_package_id_t package_id)
{
    fbe_api_trace_level_control_t level_control;
    fbe_status_t status;

    level_control.trace_type = trace_type;
    level_control.fbe_id = id;
    level_control.trace_level = FBE_TRACE_LEVEL_INVALID;
    status = fbe_api_trace_get_level(&level_control, package_id);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("can't get trace level\n");
        return FBE_TRACE_LEVEL_INVALID;
    }
    return level_control.trace_level;
}

static fbe_trace_level_t trace_set_level(fbe_trace_type_t trace_type, fbe_object_id_t id, fbe_trace_level_t level, fbe_package_id_t package_id)
{
    fbe_api_trace_level_control_t level_control = {0};
    fbe_status_t status;

    level_control.trace_type = trace_type;
    level_control.fbe_id = id;
    level_control.trace_level = level;
    status = fbe_api_trace_set_level(&level_control, package_id);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("can't get trace level\n");
        return FBE_TRACE_LEVEL_INVALID;
    }
    return trace_get_level(trace_type, id, package_id);
}

static void trace_run(fbe_u32_t id , fbe_trace_level_t trace_level, fbe_trace_type_t trace_type, fbe_package_id_t package_id)
{
    fbe_trace_level_t level;
    fbe_status_t status;
    const char * p_type_tag;
    const char * p_level_tag;

    level = trace_level;

    if (level == FBE_TRACE_LEVEL_INVALID) {
        level = trace_get_level(trace_type, id, package_id);
    }
    else {
        level = trace_set_level(trace_type, id, level, package_id);
    }
    if (level != FBE_TRACE_LEVEL_INVALID) {
        switch (trace_type) {
            case FBE_TRACE_TYPE_DEFAULT:
                 p_type_tag = "default";
                 break;
            case FBE_TRACE_TYPE_OBJECT:
                 p_type_tag = "object";
                 break;
            case FBE_TRACE_TYPE_SERVICE:
            {
                 fbe_const_service_info_t * p_service_info;
                 p_type_tag = "service";
                 status = fbe_get_service_by_id(id, &p_service_info);
                 if (status == FBE_STATUS_OK) {
                     p_type_tag = p_service_info->service_name;
                 }
                 break;
            }
            case FBE_TRACE_TYPE_LIB:
            {
                 fbe_const_library_info_t * p_library_info;
                 p_type_tag = "library";
                 status = fbe_get_library_by_id(id, &p_library_info);
                 if (status == FBE_STATUS_OK) {
                     p_type_tag = p_library_info->library_name;
                 }
                 break;
            }
            default:
                 p_type_tag = "?";
                 break;
        }
        p_level_tag = fbe_trace_get_level_stamp(level);
        fbe_cli_printf("FBE Trace %s level: %d=%s\n", p_type_tag, (int)level, p_level_tag);
    }
}
void fbe_cli_cmd_trace(int argc , char ** argv)
{

    fbe_object_id_t id = 0;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_INVALID;
    fbe_trace_type_t trace_type;
    fbe_trace_level_t trace_level = FBE_TRACE_LEVEL_INVALID;
    /*
    * Parse the command line.
    */
    while (argc > 0)
    {
       if ((strcmp(*argv, "-help") == 0) ||
           (strcmp(*argv, "-h") == 0))
       {
           /* If they are asking for help, just display help and exit.
            */
           fbe_cli_printf("%s", TRACE_USAGE);
           return;
       }
       else if ((strcmp(*argv, "-default_level") == 0) ||
                (strcmp(*argv, "-dl") == 0))
       {
           fbe_trace_level_t trace_level;
           argc--;
           argv++;
           if(argc == 0)
           {
               fbe_cli_printf("default trace level is: %d\n", fbe_trace_get_default_trace_level());
               return;
           }
           trace_level = (fbe_u32_t)strtoul(*argv, 0, 10);
           if ((trace_level >= FBE_TRACE_LEVEL_LAST) ||
               (trace_level == FBE_TRACE_LEVEL_INVALID))
           {
               fbe_cli_error("Unexpected trace level %d\n", trace_level);
               return;
           }
           fbe_trace_set_default_trace_level(trace_level);
           fbe_cli_printf("default trace level is now: %d\n", fbe_trace_get_default_trace_level());
       }
       else if ((strcmp(*argv, "-default") == 0) ||
                (strcmp(*argv, "-d") == 0))
       {
         trace_type = FBE_TRACE_TYPE_DEFAULT;
       }
       else if ((strcmp(*argv, "-object") == 0) ||
                (strcmp(*argv, "-o") == 0))
       {
       
           argc--;
           argv++;
           if(argc == 0)
           {
               fbe_cli_error("-object, expected object_id, too few arguments \n");
               return;
           }
           id = (fbe_u32_t)strtoul(*argv, 0, 16);
           trace_type = FBE_TRACE_TYPE_OBJECT;
       }
       else if ((strcmp(*argv, "-service") == 0) ||
                (strcmp(*argv, "-s") == 0))
       {
       
           argc--;
           argv++;
           if(argc == 0)
           {
               fbe_cli_error("-service, expected service, too few arguments \n");
               return;
           }
           id = (fbe_u32_t)strtoul(*argv, 0, 16);
           trace_type = FBE_TRACE_TYPE_SERVICE;
           if ((id <= FBE_SERVICE_ID_BASE_SERVICE) || (id >= FBE_SERVICE_ID_LAST) || (id == FBE_SERVICE_ID_TRACE)) {
                     fbe_cli_error("invalid service id: %x\n", id);
                     return;
            }

       }
       else if ((strcmp(*argv, "-set_trace") == 0) ||
                (strcmp(*argv, "-st") == 0))
       {
       
           argc--;
           argv++;
           if(argc == 0)
           {
               fbe_cli_error("-set_trace, expected trace level argument. \n");
               return;
           }
           /* Get the new trace level to set.
            */
           trace_level = (fbe_u32_t)strtoul(*argv, 0, 10);
           if  ((trace_level == FBE_TRACE_LEVEL_INVALID) ||
            (trace_level >= FBE_TRACE_LEVEL_LAST)) {
            fbe_cli_error("invalid trace level: %d\n", trace_level);
            return;
        }
       }
       else if ((strcmp(*argv, "-library") == 0) ||
                (strcmp(*argv, "-l") == 0))
       {
           /* Set the trace level.
            */
           argc--;
           argv++;
           if(argc == 0)
           {
               fbe_cli_error("-library, expected library argument. \n");
               return;
           }
           id = (fbe_u32_t)strtoul(*argv, 0, 16);
           trace_type = FBE_TRACE_TYPE_LIB;
           if ((id == FBE_LIBRARY_ID_INVALID) || (id >= FBE_LIBRARY_ID_LAST)) {
                     fbe_cli_error("invalid library id: %d\n", id);
                     return;
           }
       }
       else if ((strcmp(*argv, "-package") == 0) ||
                (strcmp(*argv, "-p") == 0))
       {
           /* Filter by one object id.
            */
           argc--;
           argv++;
           if(argc == 0)
           {
               fbe_cli_error("-package, expected package_id, too few arguments \n");
               return;
           }
           if((strcmp(*argv, "phy") == 0) ||
              (strcmp(*argv, "pp") == 0))
            {
              package_id = FBE_PACKAGE_ID_PHYSICAL;
            }
           else if(strcmp(*argv, "sep") == 0)
            {
              package_id = FBE_PACKAGE_ID_SEP_0;
            }
            else if (strcmp(*argv, "esp") == 0)
            {
              package_id = FBE_PACKAGE_ID_ESP;
            }
            else if (strcmp(*argv, "neit") == 0)
            {
              package_id = FBE_PACKAGE_ID_NEIT;
            }
       }

       argc--;
       argv++;

   }   /* end of while */
   if(package_id == FBE_PACKAGE_ID_INVALID)
   {
       fbe_cli_printf("%s", TRACE_USAGE);
       return; 
   }
   trace_run(id, trace_level, trace_type, package_id);
   return;  

}
