#ifndef FBE_CPD_SHIM_SIM_H
#define FBE_CPD_SHIM_SIM_H

#include "fbe/fbe_types.h"


#define INQ_ID_FIELD_OFFSET		8		/*based on DH_INQ_ID_FIELD_OFFSET*/
#define DUMMY_MS_HEADER_LEN	 4
#define DUMMY_MS_BLKDES_LEN	 8
#define DUMMY_MS_PAGE_1_LEN	 10	/* Mode sense page lengths EXCLUDE */
#define DUMMY_MS_PAGE_2_LEN	 14	/* the first 2 bytes of the page.  */
#define DUMMY_MS_PAGE_3_LEN	 22
#define DUMMY_MS_PAGE_4_LEN	 22
#define DUMMY_MS_PAGE_7_LEN  10
#define DUMMY_MS_PAGE_8_LEN	 18
#define DUMMY_MS_PAGE_A_LEN	 10
#define DUMMY_MS_PAGE_19_LEN 4
#define DUMMY_MS_PAGE_1A_LEN 10
#define DUMMY_MS_PAGE_1C_LEN 10
#define DUMMY_MS_PAGE_0_LEN	 10	/* Note - page 0 comes last */
#define DUMMY_MS_SENSE_DATA_LENGTH			 \
			(DUMMY_MS_HEADER_LEN  +			 \
			 DUMMY_MS_BLKDES_LEN  +			 \
			 DUMMY_MS_PAGE_1_LEN  + 2 +		 \
			 DUMMY_MS_PAGE_2_LEN  + 2 +		 \
			 DUMMY_MS_PAGE_3_LEN  + 2 +		 \
			 DUMMY_MS_PAGE_4_LEN  + 2 +		 \
             		 DUMMY_MS_PAGE_7_LEN  + 2 +      	 \
			 DUMMY_MS_PAGE_8_LEN  + 2 +		 \
			 DUMMY_MS_PAGE_A_LEN  + 2 +		 \
			 DUMMY_MS_PAGE_19_LEN + 2 +		 \
			 DUMMY_MS_PAGE_1A_LEN + 2 +		 \
             		 DUMMY_MS_PAGE_1C_LEN + 2 +      	 \
			 DUMMY_MS_PAGE_0_LEN  + 2	)

#define SECTOR_SIZE 520

/* This value is the number of BYTES we must go into the SCSI
    * Inquiry block to arrive at the Vendor and Product ID fields.
    */
#define DH_PG19_FC_DTOLI_BIT_MASK   ((fbe_u8_t) 0x01)
#define DH_PG19_FC_ALWLI_BIT_MASK   ((fbe_u8_t) 0x04)
#define DH_PG19_FC_DSA_BIT_MASK     ((fbe_u8_t) 0x08)
#define DH_PG19_FC_DLM_BIT_MASK     ((fbe_u8_t) 0x10)

#define DH_PG19_DEFAULT_FC_VALUE  ((fbe_u8_t) (DH_PG19_FC_DTOLI_BIT_MASK | \
                                             DH_PG19_FC_ALWLI_BIT_MASK | \
                                             DH_PG19_FC_DSA_BIT_MASK   | \
                                             DH_PG19_FC_DLM_BIT_MASK))

fbe_status_t fbe_cpd_shim_execute_irp(fbe_u32_t port_number, const void * irp);
fbe_io_block_request_status_t fbe_cpd_shim_execute_cdb(fbe_u32_t port_number, fbe_io_block_t * io_block_p);


#endif
