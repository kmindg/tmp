#ifndef	LXPMP_H
#define	LXPMP_H
/* lxpmp.h:         interface for the lxpmp program.
 *
 * Copyright (C) 2012   EMC Corporation
 *
 * This file defines the structures shared between the lxpmp program
 * and libpmp.
 *
 *  Revision History
 *  ----------------
 *  08 Nov 2012     Created by Steve Ofsthun
 */


/*
 * The following structures have aligned 64-bit values.  To maintain this
 * alignment, both substructures and the main structures should carefully
 * layout structure members to align datatypes naturally.
 *
 * To prevent unwanted member padding, we override the default gcc structure
 * alignments.  This is not strictly necessary with properly aligned datatypes.
 */
#pragma pack(1)

/*
 * lxPMP file header.
 */
struct lxpmp_hdr {
	uint32_t	lxpmp_signature;	/* 00 - signature */
#define	LXPMP_SIGNATURE (('P' << 16) | ('M' << 8) | 'P')
	uint16_t	lxpmp_flags;		/* 04 - lxPMP flags */
#define	LXPMP_ENABLED	                    0x01
#define LXPMP_MP_DISABLE_REQ                0x02
/*
 * The following components must all approve of shutting down Memory Persistence
 * before it will be allowed.
 */
#define LXPMP_MP_DISABLE_REQ_APPROVE_MCC    0x04
#define LXPMP_MP_DISABLE_REQ_APPROVE_PRAMFS 0x08
#define LXPMP_MP_DISABLE_REQ_APPROVE_2      0x10
#define LXPMP_MP_DISABLE_REQ_APPROVE_3      0x20
#define LXPMP_MP_DISABLE_REQ_APPROVE_ALL    (LXPMP_MP_DISABLE_REQ_APPROVE_MCC + LXPMP_MP_DISABLE_REQ_APPROVE_PRAMFS)
#define LXPMP_PRECONDITION_COMPLETE         0x40
	uint16_t	lxpmp_revision;		/* 06 - lxPMP revision */
#define	LXPMP_REVISION	0x1
	uint64_t	lxpmp_capacity;		/* 08 - maximum protected size*/
	uint32_t	lxpmp_io_alignment;	/* 10 - optimal I/O alignment */
	uint32_t	lxpmp_io_size;		/* 14 - optimal I/O size */
	uint32_t	lxpmp_lba_size;		/* 18 - I/O LBA (sector) size */
	uint32_t	lxpmp_data_offset;	/* 1C - data start file offset*/
	uint64_t	lxpmp_max_sgl_size;	/* 20 - max SGL entry size */
	uint32_t	lxpmp_user_offset;	/* 28 - user start file offset*/
	uint32_t	lxpmp_user_count;	/* 2c - user entry count */
	uint32_t	lxpmp_user_limit;	/* 30 - user entry limit */
	uint32_t	lxpmp_ring_offset;	/* 34 - ring buffer offset */
	uint32_t	lxpmp_ring_limit;	/* 38 - ring buffer limit */
	uint32_t	lxpmp_ring_entries;	/* 3c - ring buffer entries */
	uint64_t	lxpmp_ring_address;	/* 40 - ring buffer address */
	uint32_t	lxpmp_e820_offset;	/* 50 - e820 map offset */
	uint32_t	lxpmp_e820_limit;	/* 54 - e820 map limit */
#define	LXPMP_SERIAL_MAX	16
	char		lxpmp_chassis_serial[LXPMP_SERIAL_MAX+1];
						/* 58 - chassis serial number */
	char		lxpmp_board_serial[LXPMP_SERIAL_MAX+1];
						/* 68 - SP serial number */
};

struct lxpmp_sgl {
	uint64_t	lxpmp_sgl_src;		/* 00 - source memory address */
	uint64_t	lxpmp_sgl_len;		/* 08 - source memory length */
	uint64_t	lxpmp_sgl_dst;		/* 0C - dst PMP file offset */
};

#define	LXPMP_USER_ID_LIMIT	4	/* number of allocated user slots */

#define	LXPMP_USER_ID_MCC	0	/* predefined user mcc */
#define	LXPMP_USER_ID_PRAMFS	1	/* predefined user pramfs */
#define	LXPMP_USER_ID_TOOL	2	/* predefined user tool */

struct lxpmp_user {
	uint32_t	lxpmp_usr_signature;	/* 00 - signature */
#define	LXPMP_USR_SIGNATURE (('r' << 16) | ('s' << 8) | 'u')
#define	LXPMP_USR_NAME_LEN	12
	char		lxpmp_usr_name[LXPMP_USR_NAME_LEN];
						/* 04 - user name (for logs) */
	uint64_t	lxpmp_usr_reservation;	/* 10 - capacity reservation */
	uint32_t	lxpmp_usr_sgl_count;	/* 18 - used SGL count */
	uint32_t	lxpmp_usr_sgl_limit;	/* 1C - allocated SGL limit */
	uint32_t	lxpmp_usr_flags;	/* 20 - operational flags */
#define	LXPMP_FLAGS_IMAGE_RESTORED	0x00000001
#define	LXPMP_FLAGS_IMAGE_VALID		0x00000002
#define	LXPMP_FLAGS_DEVICE_FAILURE	0x00000004
#define	LXPMP_FLAGS_INVALID_SGL_SIZE	0x00000008
#define	LXPMP_FLAGS_IO_ERROR		0x00000010
#define	LXPMP_FLAGS_CAPACITY_EXCEEDED	0x00000020
#define	LXPMP_FLAGS_DEVICE_ZEROED	0x00000040
#define	LXPMP_FLAGS_NO_SGLS_FOUND	0x00000080
#define	LXPMP_FLAGS_RESTORE_FAILED	0x00000100
#define	LXPMP_FLAGS_MP_PROBLEM		0x00000200
#define	LXPMP_FLAGS_INVALID_REQUEST	0x00000400
#define	LXPMP_FLAGS_SGL_SRC_ALIGNMENT	0x00000800
#define	LXPMP_FLAGS_SGL_SRC_RANGE	0x00001000
#define	LXPMP_FLAGS_SGL_LEN_RANGE	0x00002000
#define	LXPMP_FLAGS_SGL_LEN_INVALID	0x00004000
#define	LXPMP_FLAGS_ERRORS		0x00007ffc
#define	LXPMP_FLAGS_RESERVED		0xffff8000
	uint16_t	lxpmp_app_flags;	/* 24 - application flags */
#define	LXPMP_APP_REQUEST_PMP		0x0001
#define	LXPMP_APP_ENCRYPT		0x0002
#define	LXPMP_APP_RESERVED		0xfffc
	uint16_t	lxpmp_app_revision;	/* 26 - application revision */
#define	LXPMP_APP_REVISION		0x0001
	uint32_t        lxpmp_usr_mp_status;    /* 28 - Memory Persistence status */
	uint32_t        lxpmp_usr_reserve_pages;/* 2C - Rebalance save area */
	uint32_t	lxpmp_error_sgl;	/* 30 - First error SGL index */
	uint32_t	lxpmp_error_lba;	/* 34 - First error LBA */
	uint64_t	lxpmp_error_address;	/* 38 - First error address */
#define LXPMP_SGL_MAX			168
	struct lxpmp_sgl lxpmp_sgls[LXPMP_SGL_MAX];	/* 40 ... SGLs */
};

struct lxpmp_ring_entry {
#define	LXPMP_RING_ENTRY_SIZE		128
	char		text[LXPMP_RING_ENTRY_SIZE];
};

struct lxpmp_ring_map {
	uint32_t	lxpmp_ring_signature;	/* 00 - ring signature */
#define	LXPMP_RING_SIGNATURE (('g' << 16) | ('n' << 8) | 'r')
	uint32_t	lxpmp_ring_entries;	/* 04 - ring buffer entries */
	uint32_t	lxpmp_ring_producer;	/* 08 - ring buffer producer */
	uint32_t	lxpmp_ring_consumer;	/* 0c - ring buffer consumer */
	uint32_t	lxpmp_ring_pad2[28];	/* 10 - more padding */
#define LXPMP_RING_ENTRIES 255
	struct lxpmp_ring_entry e[LXPMP_RING_ENTRIES];	/* 80 - ring entries */
};

struct lxpmp_e820_entry {
	uint64_t	addr;			/* 00 - memory region address */
	uint64_t	size;			/* 08 - memory region length */
	uint32_t	type;			/* 10 - memory region type */
};

#define	LXPMP_E820_TYPE_RAM		1
#define	LXPMP_E820_TYPE_RESERVED	2
#define	LXPMP_E820_TYPE_ACPI		3
#define	LXPMP_E820_TYPE_NVS		4

#define LXPMP_E820_MAX_ENTRIES		16

struct lxpmp_e820_map {
	uint32_t		nr_maps;
	struct lxpmp_e820_entry region[LXPMP_E820_MAX_ENTRIES];
};

void ring_dump(char *path, int verbose);
int pmp_log(char *msg);



#ifdef __cplusplus
extern "C"
{
#endif

int lxpmp_main(int argc, char **argv);
int ring_clear(char *path);
#define PMP_PATH_LEN 200
char * pmp_locate(void);
char * pmp_raw_locate(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*
 * Reset the gcc structure packing rules.
 */
#pragma pack()

#endif	/* LXPMP_H */
