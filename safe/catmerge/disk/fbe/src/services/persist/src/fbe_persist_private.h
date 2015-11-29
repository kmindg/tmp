#ifndef FBE_PERSIST_PRIVATE_H
#define FBE_PERSIST_PRIVATE_H

#include "csx_ext.h"
#include "fbe/fbe_persist_interface.h"
#include "fbe_persist_database.h"
#include "fbe_persist_transaction.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe_cmi.h"

typedef struct fbe_persist_service_s {
    fbe_base_service_t  base_service;
    fbe_object_id_t     database_lun_object_id;
} fbe_persist_service_t;

extern fbe_persist_service_t fbe_persist_service;

void fbe_persist_trace(fbe_trace_level_t trace_level, fbe_trace_message_id_t message_id, const char * fmt, ...) __attribute__((format(__printf_func__,3,4)));
fbe_status_t fbe_persist_cmi_init(void);
fbe_status_t fbe_persist_cmi_destroy(void);


#endif /* FBE_PERSIST_PRIVATE_H */
