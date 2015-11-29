#ifndef FBE_NEIT_FLARE_PACKAGE_H
#define FBE_NEIT_FLARE_PACKAGE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_neit.h"
#include "fbe/fbe_dest.h"

#define FBE_NEIT_PACKAGE_DEVICE_NAME "\\Device\\NeitPackage"
#define FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR "\\Device\\NeitPackage"
#define FBE_NEIT_PACKAGE_DEVICE_LINK "\\DosDevices\\NeitPackageLink"

#define FBE_NEIT_PACKAGE_DEVICE_TYPE EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN


/* Values in the range 0x000 to 0x7ff are reserved by Microsoft for public I/O control codes. */
#define FBE_NEIT_PACKAGE_ADD_RECORD					0x801
#define FBE_NEIT_PACKAGE_REMOVE_RECORD				0x802
#define FBE_NEIT_PACKAGE_REMOVE_OBJECT				0x803
#define FBE_NEIT_PACKAGE_GET_RECORD					0x804
#define FBE_NEIT_PACKAGE_START						0x805
#define FBE_NEIT_PACKAGE_STOP						0x806
#define FBE_NEIT_PACKAGE_INIT						0x807

/* DEST is piggybacking on NEIT driver*/
#define FBE_DEST_PACKAGE_ADD_RECORD					0x811
#define FBE_DEST_PACKAGE_REMOVE_RECORD				0x812
#define FBE_DEST_PACKAGE_REMOVE_OBJECT				0x813
#define FBE_DEST_PACKAGE_GET_RECORD					0x814
#define FBE_DEST_PACKAGE_START						0x815
#define FBE_DEST_PACKAGE_STOP						0x816
#define FBE_DEST_PACKAGE_INIT						0x817


#define FBE_NEIT_PACKAGE_ADD_RECORD_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
															FBE_NEIT_PACKAGE_ADD_RECORD,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_NEIT_PACKAGE_REMOVE_RECORD_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
															FBE_NEIT_PACKAGE_REMOVE_RECORD,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_NEIT_PACKAGE_REMOVE_OBJECT_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
															FBE_NEIT_PACKAGE_REMOVE_OBJECT,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_NEIT_PACKAGE_GET_RECORD_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
															FBE_NEIT_PACKAGE_GET_RECORD,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_NEIT_PACKAGE_START_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
															FBE_NEIT_PACKAGE_START,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_NEIT_PACKAGE_STOP_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
															FBE_NEIT_PACKAGE_STOP,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_NEIT_PACKAGE_INIT_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
															FBE_NEIT_PACKAGE_INIT,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

/* Drive Error Simulation Tool IOCTLS */

#define FBE_DEST_PACKAGE_ADD_RECORD_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
															FBE_DEST_PACKAGE_ADD_RECORD,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_DEST_PACKAGE_REMOVE_RECORD_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
															FBE_DEST_PACKAGE_REMOVE_RECORD,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_DEST_PACKAGE_REMOVE_OBJECT_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
															FBE_DEST_PACKAGE_REMOVE_OBJECT,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_DEST_PACKAGE_GET_RECORD_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
															FBE_DEST_PACKAGE_GET_RECORD,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_DEST_PACKAGE_START_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
															FBE_DEST_PACKAGE_START,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_DEST_PACKAGE_STOP_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
															FBE_DEST_PACKAGE_STOP,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_DEST_PACKAGE_INIT_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
															FBE_DEST_PACKAGE_INIT,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)


/* FBE_NEIT_PACKAGE_ADD_RECORD_IOCTL */
typedef struct fbe_neit_package_add_record_s{
	fbe_neit_error_record_t neit_error_record; /* INPUT */
	fbe_neit_record_handle_t neit_record_handle; /* OUTPUT */
}fbe_neit_package_add_record_t;

/* FBE_NEIT_PACKAGE_REMOVE_RECORD_IOCTL */
typedef struct fbe_neit_package_remove_record_s{
	fbe_neit_record_handle_t neit_record_handle; /* INPUT */
}fbe_neit_package_remove_record_t;

/* FBE_NEIT_PACKAGE_REMOVE_OBJECT_IOCTL */
typedef struct fbe_neit_package_remove_object_s{
	fbe_object_id_t object_id; /* INPUT */
}fbe_neit_package_remove_object_t;

/* FBE_NEIT_PACKAGE_GET_RECORD_IOCTL */
typedef struct fbe_neit_package_get_record_s{
	fbe_neit_record_handle_t neit_record_handle; /* INPUT */
	fbe_neit_error_record_t neit_error_record; /* OUTPUT */	
}fbe_neit_package_get_record_t;


/* Drive Error Simulation Tool IOCTL records */

/* FBE_DEST_PACKAGE_DEST_ADD_RECORD_IOCTL */
typedef struct fbe_dest_add_record_s{
	fbe_dest_error_record_t dest_error_record; /* INPUT */
	fbe_dest_record_handle_t dest_record_handle; /* OUTPUT */
}fbe_dest_add_record_t;

/* FBE_DEST_PACKAGE_DEST_REMOVE_RECORD_IOCTL */
typedef struct fbe_dest_remove_record_s{
	fbe_dest_record_handle_t dest_record_handle; /* INPUT */
}fbe_dest_remove_record_t;

/* FBE_DEST_PACKAGE_DEST_REMOVE_OBJECT_IOCTL */
typedef struct fbe_dest_remove_object_s{
	fbe_object_id_t object_id; /* INPUT */
}fbe_dest_remove_object_t;

/* FBE_DEST_PACKAGE_DEST_GET_RECORD_IOCTL */
typedef struct fbe_dest_get_record_s{
	fbe_dest_record_handle_t dest_record_handle; /* INPUT */
	fbe_dest_error_record_t dest_error_record; /* OUTPUT */	
}fbe_dest_get_record_t;


fbe_status_t neit_flare_package_init(void);
fbe_status_t neit_flare_package_destroy(void);
fbe_status_t neit_package_add_record(fbe_neit_error_record_t * neit_error_record, fbe_neit_record_handle_t * neit_record_handle);
fbe_status_t neit_package_remove_record(fbe_neit_record_handle_t neit_record_handle);
fbe_status_t neit_package_remove_object(fbe_object_id_t object_id);
fbe_status_t neit_package_get_record(fbe_neit_record_handle_t neit_record_handle, fbe_neit_error_record_t * neit_error_record);
fbe_status_t neit_package_start(void);
fbe_status_t neit_package_stop(void);
fbe_status_t neit_package_cleanup_queue(void);


#endif /* FBE_NEIT_FLARE_PACKAGE_H */
