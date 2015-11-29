/***************************************************************************
 * Copyright (C) EMC Corp 1999 - 2014
 *
 * All rights reserved.
 * 
 * This software contains the intellectual property of EMC Corporation or
 * is licensed to EMC Corporation from third parties. Use of this software
 * and the intellectual property contained therein is expressly limited to
 * the terms and conditions of the License Agreement under which it is
 * provided by or on behalf of EMC
 **************************************************************************/

/*******************************************************************************
 * flare_sgl.h
 *
 * This header file defines constants and structures associated with 
 * Scatter-Gather Lists.
 *
 ******************************************************************************/

#if !defined (FLARE_SGL_H)
#define FLARE_SGL_H

typedef ULONG       SGL_LENGTH;

//This retains the independence from NT data types in kernel builds which was 
//requested by the physical package people.
typedef unsigned __int64    SGL_PADDR; //SGL Physical Address

//typedef void *              SGL_VADDR; //SGL Virtual Address
typedef unsigned __int64    SGL_VADDR; //SGL Virtual Address

/*
 * Calculating the physical address from the virtual address
 */ 
static __forceinline SGL_PADDR SGL_PADDR_GET(SGL_PADDR paddr, unsigned __int64 by) 
{
    return (SGL_PADDR)(paddr - by);
}

/*
 * Calculating the pseudo-virtual address from the physical address
 */ 
static __forceinline SGL_PADDR SGL_VADDR_GET(SGL_PADDR paddr, unsigned __int64 by) 
{
    return (SGL_PADDR)(paddr + by);
}

#pragma pack(4)
typedef struct _SGL         // Scatter/Gather List entry.
{
    SGL_LENGTH  PhysLength;
    SGL_PADDR   PhysAddress;
} SGL, *PSGL;
#pragma pack()

#pragma pack(4)
typedef struct _VSGL            // Scatter/Gather List entry.
{
    SGL_LENGTH  virt_length;
    SGL_VADDR   virt_address;
} VSGL, *PVSGL;
#pragma pack()

// values used in SGL_DESCRIPTOR.sgl_ctrl.xlat_type
#define SGL_unused0_TRANSLATION     0
#define SGL_CONSTANT_TRANSLATION    1   // each SGL entry has same phys offset
#define SGL_FUNCTION_TRANSLATION    2   // each SGL entry phys addr must be
                                        // calculated by function call
#define SGL_unused3_TRANSLATION     3


// function to translate virtual to physical.
typedef SGL_PADDR (SGL_GET_PHYS_ADDR) (void * vaddr, void * xlat_context);

#pragma pack(4)
typedef struct _SGL_DESCRIPTOR     // Extended Scatter/Gather List descriptor.
{
    struct
    {
        unsigned int xlat_type          : 2;
        unsigned int repeat_this_sgl    : 1;
        unsigned int repeat_all_sgls    : 1;   // valid only in first
            // SGL descriptor of array
        unsigned int skip_metadata      : 1;
        unsigned int last_descriptor    : 1;
    } sgl_ctrl;

    unsigned int repeat_count;

    union
    {
        struct
        {
            SGL_GET_PHYS_ADDR   * sgl_get_paddr; // SGL_FUNCTION_TRANSLATION -
                                                //   call with virt addr and
                                                //   and context to get phys
            void                * xlat_context; // arg to sgl_get_paddr func
        } sgl_xlat_func;

        unsigned __int64    physical_offset;// SGL_CONSTANT_TRANSLATION -
                                            // add to virt addr to get phys.
    } sgl_xlat;

    VSGL    * sgl_ptr;      // pointer to actual Scatter/Gather List.

} SGL_DESCRIPTOR, *PSGL_DESCRIPTOR;
#pragma pack()


/*
 * The maximum number of entries in an SG list.  
 */

#define FLARE_MAX_SG_ENTRIES    256

#endif /* !defined (FLARE_SGL_H) */
