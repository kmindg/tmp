/* odb_dbo_utils.c */

/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2001
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *
 * DESCRIPTION:
 *   This file contains OBJECT DATABASE DBO utilities.
 *
 * FUNCTIONS:
 *   
 *   dbo_strcmp()
 *   dbo_strcpy()
 *   dbo_memcmp()
 *   dbo_memcpy()
 *   dbo_memset()
 *   dbo_DbolbData2UserData()
 *   dbo_DbolbHost2Net()
 *   dbo_DbolbNet2Host()
 *   dbo_UserData2DbolbData()
 *   dbo_UserDataCount2DbolbCount()
 *   
 * NOTES:
 *   In this version of DBO header we assume 32 bit LBA address
 *   if this changed to 64 bit LBA asssert will fail. We need to
 *   bump-up the revision and add code to handle upgrades/downgrades.
 *
 * HISTORY:
 *   09-Aug-00: Created.                               Mike Zelikov (MZ)
 *
 **************************************************************************/


/*************************
 * INCLUDE FILES
 *************************/
#include "odb_dbo.h"
#include "dbo_crc32.h"

/**************************
 * IMPORTS 
 **************************/


/**************************
 * LOCAL FUNCTION PROTOTYPES 
 **************************/


/**************************
 * INITIALIZATIONS 
 **************************/


/***********************************************************************
 *                  dbo_memset()
 ***********************************************************************
 *
 * DESCRIPTION:
 *   *UTILITY FUNCTION*
 *   This function fills memory on byte basis using specified symbol.
 *
 * PARAMETERS:
 *   data           - Ptr to data to be filled;
 *   sym            - Charater to use for filling;
 *   size           - Size of the data;
 *
 * RETURN VALUES/ERRORS:
 *   TRUE -- if an error was discoverd,
 *   FALSE otherwise.
 *
 * NOTES:
 *   
 *
 * HISTORY:
 *   27-Nov-01: Created.                               Mike Zelikov (MZ)
 *
 ***********************************************************************/
BOOL dbo_memset(UINT_8E *data, UINT_8E sym, UINT_32 size)
{
     UINT_32 i=0;


     odb_assert(data != NULL);
     odb_assert(size > 0);

     for (i=0; i<size; i++)
     {
          data[i] = sym;
     }

     return (FALSE);

} /* dbo_memset() */


/***********************************************************************
 *                  dbo_memcmp()
 ***********************************************************************
 *
 * DESCRIPTION:
 *   *UTILITY FUNCTION*
 *   This function compares memory blocks.
 *
 * PARAMETERS:
 *   rd1            - Ptr to data block 1;
 *   rd2            - Ptr to data block 2;
 *   size           - Size of the data to be compared;
 *   status         - Ptr to return status: FALSE buffers are not the same
 *                                          TRUE otherwise;
 *
 * RETURN VALUES/ERRORS:
 *   TRUE -- if an error was discoverd,
 *   FALSE otherwise.
 *
 * NOTES:
 *   
 *
 * HISTORY:
 *   27-Nov-01: Created.                               Mike Zelikov (MZ)
 *
 ***********************************************************************/
BOOL dbo_memcmp(const void *rd1, const void *rd2, UINT_32 size, BOOL *status)
{
     UINT_32 i=0;
     UINT_8E *d1=NULL, *d2=NULL;


     odb_assert(rd1 != NULL);
     odb_assert(rd2 != NULL);
     odb_assert(size > 0);
     odb_assert(status != NULL);

     *status = TRUE;

     d1 = (UINT_8E *) rd1;
     d2 = (UINT_8E *) rd2;

     for (i=0; i<size; i++)
     {
          if (d1[i] != d2[i])
          {
               *status = FALSE;
               break;
          }
     }

     return(FALSE);

} /* dbo_memcmp() */


/***********************************************************************
 *                  dbo_memcpy()
 ***********************************************************************
 *
 * DESCRIPTION:
 *   *UTILITY FUNCTION*
 *   This function copies contents of one memory block to antother.
 *
 * PARAMETERS:
 *   rd_dest        - Ptr to destination data block;
 *   rd_src         - Ptr to data source data block;
 *   size           - Size of the data to be copied;
 *
 * RETURN VALUES/ERRORS:
 *   TRUE -- if an error was discoverd,
 *   FALSE otherwise.
 *
 * NOTES:
 *   
 *
 * HISTORY:
 *   27-Nov-01: Created.                               Mike Zelikov (MZ)
 *
 ***********************************************************************/
BOOL dbo_memcpy(void *rd_dest, const void *rd_src, UINT_32 size)
{
     UINT_32 i=0;
     UINT_8E *d_src=NULL, *d_dest=NULL;


     odb_assert(rd_src != NULL);
     odb_assert(rd_dest != NULL);
     odb_assert(size > 0);

     d_dest = (UINT_8E *) rd_dest;
     d_src = (UINT_8E *) rd_src;

     for (i=0; i<size; i++)
     {
          d_dest[i] = d_src[i];
     }

     return (FALSE);

} /* dbo_memcpy() */


/***********************************************************************
 *                  dbo_strcmp()
 ***********************************************************************
 *
 * DESCRIPTION:
 *   *UTILITY FUNCTION*
 *   This function compares character strings;
 *
 * PARAMETERS:
 *   s1             - String 1;
 *   s2             - String 2;
 *   status         - Ptr to return status: FALSE buffers are not the same
 *                                          TRUE otherwise;
 *
 * RETURN VALUES/ERRORS:
 *   TRUE -- if an error was discoverd,
 *   FALSE otherwise.
 *
 * NOTES:
 *   
 *
 * HISTORY:
 *   27-Nov-01: Created.                               Mike Zelikov (MZ)
 *
 ***********************************************************************/
BOOL dbo_strcmp(const char *s1, const char *s2, BOOL *status)
{
     odb_assert(s1 != NULL);
     odb_assert(s2 != NULL);
     odb_assert(status != NULL);

     *status = TRUE;
     
     while ((*s2 != '\0') && (*s1 != '\0'))
     {
          if (*s1 != *s2) break;

          s1++;
          s2++;
     }

     /* This covers when *s1 != *s2 and we did break and
      * when one string is shorter than other one.
      */
     if (*s1 != *s2) (*status = FALSE);

     return(FALSE);

} /* dbo_strcmp() */


/***********************************************************************
 *                  dbo_strcpy()
 ***********************************************************************
 *
 * DESCRIPTION:
 *   *UTILITY FUNCTION*
 *   This function copies character strings;
 *
 * PARAMETERS:
 *   s1             - String 1;
 *   s2             - String 2;
 *
 * RETURN VALUES/ERRORS:
 *   TRUE -- if an error was discoverd,
 *   FALSE otherwise.
 *
 * NOTES:
 *   
 *
 * HISTORY:
 *   27-Nov-01: Created.                               Mike Zelikov (MZ)
 *
 ***********************************************************************/
BOOL dbo_strcpy(char *s1, const char *s2)
{
     odb_assert(s1 != NULL);
     odb_assert(s2 != NULL);

     while (*s2 != '\0')
     {
          *s1 = *s2;

          s1++;
          s2++;
     }
     
     *s1 = '\0';

     return(FALSE);

} /* dbo_strcpy() */


/***********************************************************************
 *                  dbo_DbolbData2UserData()
 ***********************************************************************
 *
 * DESCRIPTION:
 *   *UTILITY FUNCTION*
 *   This function converts DBOLB data to Regular RAW data (data w/o 
 *   DBO header);
 *
 * PARAMETERS:
 *   count_ptr      - (IN) size in blocks; (OUT) size in bytes;
 *   start_addr     - address from where dbolb data was obtained;
 *   dbo_type_ptr   - ptr to DBO type - extracted from DBO header;
 *   dbo_id_ptr     - ptr to BDO id -  extracted from DBO header;
 *   status_ptr     - ptr to completion status: TRUE if data was 
 *                    converted successfully, FALSE otherwise;
 *
 * RETURN VALUES/ERRORS:
 *   TRUE -- if an error was discoverd,
 *   FALSE otherwise.
 *
 * NOTES:
 *   Completion status on data convertion is returned through status_ptr
 *
 * HISTORY:
 *   09-Aug-00: Created.                               Mike Zelikov (MZ)
 *
 ***********************************************************************/
BOOL dbo_DbolbData2UserData(UINT_8E *buf, 
                            UINT_32 *count_ptr, 
                            DBOLB_ADDR  start_addr, 
                            DBO_TYPE *dbo_type_ptr, 
                            DBO_ID *dbo_id_ptr,
                            BOOL *status_ptr)
{
     DBO_HEADER *dbo_h = NULL;
     UINT_8E *shift_start = NULL, *curr_data_ptr = NULL;
     BOOL error = FALSE, status = FALSE;
     UINT_32 total_length=0, data_left=0, b=0, byte=0, lb_counter=0, amount_to_shift=0;
     DBO_TYPE dbo_type=DBO_INVALID_TYPE;
     DBO_ID dbo_id=DBO_INVALID_ID;               
     DBO_REVISION rev_num = 0;

     // 64-bit address
     odb_assert(sizeof(start_addr) == sizeof(DBOLB_ADDR)); 

     odb_assert(buf != NULL);
     odb_assert(count_ptr != NULL);
     
     lb_counter = *count_ptr;
     odb_assert(lb_counter > 0);
     
     odb_assert(status_ptr != NULL);
     
     *status_ptr = FALSE;
     shift_start = buf;

     for(b=0; b<lb_counter; b++)
     {
          /* Convert DBOLB Header from Network to Host format */
          dbo_h = (DBO_HEADER *) &(buf[DBOLB_PAGE_SIZE * b]);

          error = dbo_DbolbNet2Host(dbo_h, &rev_num, &status);
          if (error)
          {
               /* We got error while extracting DBO header from the data */
               break;
          }
          
          if (!status)
          {
               /* The data is bad - can not convert! */
               break;
          }

          /* We only support revision ONE at this time. */
          odb_assert (rev_num == DBO_REVISION_ONE); /* we have valid CRC but unsupported REV */

          if (b == 0)
          {
               /* Setup initial values for TYPE, ID and total data length. */
               dbo_type = dbo_h->u.fields.type;
               dbo_id = dbo_h->u.fields.id;
               total_length = data_left = dbo_h->u.fields.length;
               
               /* We want to be sure that we will be processing all the data. */
               odb_assert(dbo_UserDataCount2DbolbCount(total_length) == lb_counter);
          }
          
          if ((dbo_type != dbo_h->u.fields.type) ||
              (!DBO_ID_IS_EQUAL(&dbo_id, &(dbo_h->u.fields.id))) ||
              (data_left != dbo_h->u.fields.length) ||
              (b != dbo_h->u.fields.count) ||
              (DBO_HEADER_SIZE != dbo_h->u.fields.offset) ||
              ((data_left > DBOLB_MANAGEMENT_DATA_SIZE) &&
               (dbo_h->u.fields.next != (start_addr + b))) ||
              ((data_left <= DBOLB_MANAGEMENT_DATA_SIZE) &&
               (dbo_h->u.fields.next != UINT32_INVALID_ADDRESS)))
          {
               /* We found inconsistent data */
               error = TRUE;
               
               odb_assert(!error); /* We have valid CRC but inconsistent data */
               break;
          }
          
          /* Get the pointer to current USER data within dbolb */
          curr_data_ptr = (UINT_8E *) &(dbo_h[1]);

          /* Shift actuall data - erasing DBO header information */
          if (data_left < DBOLB_MANAGEMENT_DATA_SIZE)
          {
               amount_to_shift = data_left;
          }
          else
          {
               amount_to_shift = DBOLB_MANAGEMENT_DATA_SIZE;
          }

          for (byte=0; byte < amount_to_shift; byte++)
          {
               shift_start[byte] = curr_data_ptr[byte];
          }

          shift_start += DBOLB_MANAGEMENT_DATA_SIZE;
          data_left -= amount_to_shift;
     }

     if (!error && status)
     {
          odb_assert(data_left == 0);

          *count_ptr = total_length;
          *dbo_type_ptr = dbo_type;
          *dbo_id_ptr = dbo_id;

          /* We converted data successfully */
          *status_ptr = TRUE;
     }

     return(error);
     
} /* dbo_Dbolb2UserData() */


/***********************************************************************
 *                  dbo_UserData2DbolbData()
 ***********************************************************************
 *
 * DESCRIPTION:
 *   *UTILITY FUNCTION*
 *   This function will convert RAW User data into DBOLB format (data
 *   with added DBO header).
 *
 * PARAMETERS:
 *   buf            - ptr to buffer with data;
 *   count_ptr      - (OUT) size in blocks; (IN) size in bytes;
 *   start_addr     - address where dbolb data will be stored;
 *   dbo_type_ptr   - ptr to DBO type - put into DBO header;
 *   dbo_id_ptr     - ptr to BDO id -  put into DBO header;
 *
 * RETURN VALUES/ERRORS:
 *   TRUE - if an error was discoverd,
 *   FALSE otherwise.
 *
 * NOTES:
 *   If no errors are discovered this function always succeeds.
 *
 * HISTORY:
 *   09-Aug-00: Created.                               Mike Zelikov (MZ)
 *
 ***********************************************************************/
BOOL dbo_UserData2DbolbData(UINT_8E *buf, 
                            UINT_32 *count_ptr, 
                            const DBOLB_ADDR start_addr, 
                            const DBO_TYPE *dbo_type_ptr, 
                            const DBO_ID *dbo_id_ptr)
{
     DBO_HEADER *curr_dbo_h = NULL;
     UINT_8E *shift_start = NULL, *curr_data_ptr = NULL;
     BOOL error = FALSE;
     UINT_32 total_length=0, length=0, b=0, byte=0, lb_counter=0, amount_to_shift=0;
     UINT_32E start_addr_32bit = UINT32_INVALID_ADDRESS;
     
     // 64-bit address
     odb_assert(sizeof(start_addr) == sizeof(DBOLB_ADDR)); 

     odb_assert(buf != NULL);
     odb_assert(count_ptr != NULL);

     // We are passed in a 64-bit address, but we need to
     // fill in the header with a 32-bit address, so that
     // the MZM can remain consistent. We only use the lower
     // addresses for the data directory management regions.
     // The address passed in should either be invalid (all Fs)
     // or the upper 32 bits should be zero.
     if (start_addr == DBOLB_INVALID_ADDRESS)
     {
         start_addr_32bit = UINT32_INVALID_ADDRESS;
     }
     else
     {
         odb_assert ((start_addr & 0xFFFFFFFF00000000) == (ULONGLONG)0);
         start_addr_32bit = (UINT_32E)start_addr;
     }

     
     total_length = *count_ptr;
     lb_counter = dbo_UserDataCount2DbolbCount(total_length);
     
     /* Start moving data attaching DBO header - creating DBOLB
      * We will be moving backwards - from the last block to the first one.
      */
     for(b=0; b<lb_counter; b++)
     {
          if (b == 0)
          {
               amount_to_shift = total_length % DBOLB_MANAGEMENT_DATA_SIZE;
               if (amount_to_shift == 0)
               {
                    amount_to_shift = DBOLB_MANAGEMENT_DATA_SIZE;
               }
          }
          else
          {
               amount_to_shift = DBOLB_MANAGEMENT_DATA_SIZE;
          }

          curr_data_ptr = &(buf[DBOLB_MANAGEMENT_DATA_SIZE*(lb_counter-b-1)]);
          curr_dbo_h = (DBO_HEADER *)&(buf[DBOLB_PAGE_SIZE*(lb_counter-b-1)]);
          shift_start = ((UINT_8E *) curr_dbo_h) + DBO_HEADER_SIZE;

          /* Shift data going backwards */
          for (byte = 0; byte < amount_to_shift; byte++)
          {
               shift_start[amount_to_shift - byte - 1] = 
                    curr_data_ptr[amount_to_shift - byte - 1];
          }
          
          length += amount_to_shift;

          /* Fill in DBO HEADER information */
          curr_dbo_h->u.fields.type   = *dbo_type_ptr;
          curr_dbo_h->u.fields.id     = *dbo_id_ptr;
          curr_dbo_h->u.fields.count  = lb_counter - b - 1;
          curr_dbo_h->u.fields.offset = DBO_HEADER_SIZE;
          curr_dbo_h->u.fields.length = length;
          curr_dbo_h->u.fields.next   = ((b == 0) ? 
                                         UINT32_INVALID_ADDRESS : 
                                         (start_addr_32bit + lb_counter - b));

          /* Convert DBOLB Header from Host to Network format */
          error = dbo_DbolbHost2Net(curr_dbo_h, DBO_REVISION_ONE);
          if (error)
          {
               odb_assert(!error);                 
               
               /* We got error while converting DBO header to NET format */
               break;
          }
     }
     
     if (!error)
     {
          odb_assert(total_length == length);  
          *count_ptr = lb_counter;
     }
     
     return(error);
     
} /* dbo_UserData2DbolbData() */


/***********************************************************************
 *                  dbo_UserDataCount2DbolbCount()
 ***********************************************************************
 *
 * DESCRIPTION:
 *   *UTILITY FUNCTION*
 *   Finds number of requered DBOLBs for user data size.
 *
 * PARAMETERS:
 *   user_data_count - size of user data in RAW format;  
 *
 * RETURN VALUES/ERRORS:
 *   Number of required DBOLBs.
 *
 * NOTES:
 *
 *
 * HISTORY:
 *   09-Aug-00: Created.                               Mike Zelikov (MZ)
 *
 ***********************************************************************/
UINT_32 dbo_UserDataCount2DbolbCount(UINT_32 user_data_count)
{
     UINT_32 dbolb_count = 0;

     
     dbolb_count = user_data_count / DBOLB_MANAGEMENT_DATA_SIZE;
     if ((user_data_count % DBOLB_MANAGEMENT_DATA_SIZE) != 0)
     {
          dbolb_count++;
     }

     return(dbolb_count);

} /* dbo_UserDataCount2DbolbCount() */


/* LOCAL FUNCTIONS */
/***********************************************************************
 *                  dbo_DbolbNet2Host()
 ***********************************************************************
 *
 * DESCRIPTION:
 *   *UTILITY FUNCTION*
 *   This function will convert DBOLB header from NET to HOST format
 *
 * PARAMETERS:
 *   dbo_h       - ptr to dbo header; 
 *   rev_num_ptr - ptr to DBO header revision number;
 *   status_ptr  - ptr to completion status: TRUE if data was converted
 *                 successfully, FALSE otherwise;
 *
 * RETURN VALUES/ERRORS:
 *   TRUE -- if an error was discovered,
 *   FALSE otherwise;
 *
 * NOTES:
 *   Completion status on data convertion is returned through status_ptr
 *
 * HISTORY:
 *   09-Aug-00: Created.                               Mike Zelikov (MZ)
 *
 ***********************************************************************/
BOOL dbo_DbolbNet2Host(DBO_HEADER *dbo_h, 
                       DBO_REVISION *rev_num_ptr,
                       BOOL *status_ptr)
{
     UINT_16E *data = NULL;
     UINT_32 w_length = 0, w=0;
     BOOL error = FALSE;

     
     odb_assert(dbo_h != NULL);
     odb_assert(status_ptr != NULL);

     *status_ptr = FALSE;

     dbo_h->rev_num = dbo_ntohs(dbo_h->rev_num); /* SHORT - 16 bit value */
     
     if (dbo_h->rev_num != DBO_REVISION_ONE)
     {
          /* We only support one revision of DBO header! */
          /* !!!MZ error = TRUE; */
     }
     else
     {
          /* Put data in the HOST order.
           * Data length must be aligned with UINT_16E
           */
          odb_assert((DBO_HEADER_CRC_SIZE % (sizeof(UINT_16E))) == 0); 
          
          data = (UINT_16E *)(dbo_h->u.raw);
          w_length = DBO_HEADER_CRC_SIZE / sizeof(UINT_16E);
          
          for (w=0; w < w_length; ++w)
          {
               data[w] = dbo_ntohs(data[w]);
          }
          
          /* Check CRC - notice that crc and rev_num are not protected by CRC */
          dbo_h->crc = dbo_ntohl(dbo_h->crc);        /* LONG - 32 bit value */
          dbo_h->crc ^= dbo_Crc32Buffer(DBO_CRC32_DEFAULT_SEED,
                                        dbo_h->u.raw, 
                                        DBO_HEADER_CRC_SIZE);
          if (0 == dbo_h->crc)
          {
               /* CRC does match -- converted successfully! */
               *status_ptr = TRUE;
          }
     }

     if ((!error) && (*status_ptr))
     {
          /* The data was valid */
          *rev_num_ptr = dbo_h->rev_num;
     }

     return (error);

} /* dbo_DbolbNet2Host() */


/***********************************************************************
 *                  dbo_DbolbHost2Net()
 ***********************************************************************
 *
 * DESCRIPTION:
 *   *UTILITY FUNCTION*
 *   This function will convert DBOLB header from HOST to NET format.
 *
 * PARAMETERS:
 *   dbo_h       - ptr to dbo header; 
 *   rev_num     - DBO header revision number;
 *
 * RETURN VALUES/ERRORS:
 *   TRUE -- if an error was discovered,
 *   FALSE otherwise;
 *
 * NOTES:
 *  If no errors are discovered this function always succeeds.
 *
 * HISTORY:
 *   09-Aug-00: Created.                               Mike Zelikov (MZ)
 *
 ***********************************************************************/
BOOL dbo_DbolbHost2Net(DBO_HEADER *dbo_h, const DBO_REVISION rev_num)
{
     UINT_16E *data = NULL;
     UINT_32 w_length = 0;
     UINT_32 w = 0;
     BOOL error = FALSE;

     
     odb_assert(dbo_h != NULL);

     if (rev_num != DBO_REVISION_ONE)
     {
          /* We only support one revision of DBO header! */
          error = TRUE;
     }
     else
     {
          dbo_h->rev_num = dbo_htons(rev_num); /* SHORT - 16 bit value */

          /* Check CRC - notice that crc and rev_num are not protected by CRC */
          dbo_h->crc = dbo_Crc32Buffer(DBO_CRC32_DEFAULT_SEED,
                                       dbo_h->u.raw,
                                       DBO_HEADER_CRC_SIZE);

          dbo_h->crc = dbo_htonl(dbo_h->crc);        /* LONG - 32 bit value */
          
          /* Put data in the NETWORK order.
           * Data length must be aligned with UINT_16E
           */
          odb_assert((DBO_HEADER_CRC_SIZE % (sizeof(UINT_16E))) == 0); 
          
          data = (UINT_16E *)(dbo_h->u.raw);
          w_length = DBO_HEADER_CRC_SIZE / sizeof(UINT_16E);
          
          for (w=0; w < w_length; ++w)
          {
               data[w] = dbo_htons(data[w]);
          }
     }

     return (error);

} /* dbo_DbolbHost2Net() */

