/******************************************************************************
*
* Copyright 2014 Altera Corporation. All Rights Reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. The name of the author may not be used to endorse or promote products
* derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED. IN NO
* EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
*
******************************************************************************/

#include "alt_pt.h"

// MMU Page table - 16KB aligned at 8KB boundary
static uint32_t __attribute__ ((aligned (0x4000))) alt_pt_storage[8192];

/******************************************************************************/
/*!
 * Allocate MMU page table
 *
 * \return      page table
 */
void * alt_pt_alloc(const size_t size, void * context)
{
    return context;
}

/******************************************************************************/
/*!
 * Initialize MMU page tables
 *
 * \param       segment
 * \param       segment count
 * \return      result of the function
 */
ALT_STATUS_CODE alt_pt_init(ALT_PT_SEGMENT_t * segments, size_t count)
{
    if (count == 0)
    {
        return ALT_E_ERROR;
    }

    if (segments[0].size == 0)
    {
        return ALT_E_ERROR;
    }

    ALT_MMU_MEM_REGION_t regions[(count * 2) + 1];
    int next_region = 0;

    if (segments[0].addr != 0)
    {
        regions[next_region++] = (ALT_MMU_MEM_REGION_t)
        {
            .va         = (void *)0x0,
            .pa         = (void *)0x0,
            .size       = (uintptr_t)segments[0].addr,
            .access     = ALT_MMU_AP_PRIV_ACCESS,
            .attributes = ALT_MMU_ATTR_DEVICE_NS,
            .shareable  = ALT_MMU_TTB_S_NON_SHAREABLE,
            .execute    = ALT_MMU_TTB_XN_DISABLE,
            .security   = ALT_MMU_TTB_NS_SECURE
        };
    }

    for (size_t i = 0; i < count - 1; ++i)
    {
        regions[next_region++] = (ALT_MMU_MEM_REGION_t)
        {
            .va         = segments[i].addr,
            .pa         = segments[i].addr,
            .size       = segments[i].size,
            .access     = ALT_MMU_AP_PRIV_ACCESS,
            .attributes = ALT_MMU_ATTR_WBA,
            .shareable  = ALT_MMU_TTB_S_NON_SHAREABLE,
            .execute    = ALT_MMU_TTB_XN_DISABLE,
            .security   = ALT_MMU_TTB_NS_SECURE
        };

        if ((uintptr_t)segments[i].addr + segments[i].size != (uintptr_t)segments[i + 1].addr)
        {
            uintptr_t ucached_addr = (uintptr_t)segments[i].addr + segments[i].size;
            size_t    ucached_size = (uintptr_t)segments[i + 1].addr - ((uintptr_t)segments[i].addr + segments[i].size);

            // A region that describes non-pageable memory is needed between the segments.
            regions[next_region++] = (ALT_MMU_MEM_REGION_t)
            {
                .va         = (void *)ucached_addr,
                .pa         = (void *)ucached_addr,
                .size       = ucached_size,
                .access     = ALT_MMU_AP_PRIV_ACCESS,
                .attributes = ALT_MMU_ATTR_DEVICE_NS,
                .shareable  = ALT_MMU_TTB_S_NON_SHAREABLE,
                .execute    = ALT_MMU_TTB_XN_DISABLE,
                .security   = ALT_MMU_TTB_NS_SECURE
            };
        }
    }

    // Last segment and last non-pageable memory.

    regions[next_region++] = (ALT_MMU_MEM_REGION_t)
    {
        .va         = segments[count - 1].addr,
        .pa         = segments[count - 1].addr,
        .size       = segments[count - 1].size,
        .access     = ALT_MMU_AP_PRIV_ACCESS,
        .attributes = ALT_MMU_ATTR_WBA,
        .shareable  = ALT_MMU_TTB_S_NON_SHAREABLE,
        .execute    = ALT_MMU_TTB_XN_DISABLE,
        .security   = ALT_MMU_TTB_NS_SECURE
    };

    if ((uintptr_t)segments[count - 1].addr + segments[count -1].size != 0x0)
    {
        uintptr_t ucached_addr = (uintptr_t)segments[count - 1].addr + segments[count - 1].size;
        size_t    ucached_size = 0xffffffff - ((uintptr_t)segments[count - 1].addr + segments[count - 1].size) + 1;

        regions[next_region++] = (ALT_MMU_MEM_REGION_t)
        {
            .va         = (void *)ucached_addr,
            .pa         = (void *)ucached_addr,
            .size       = ucached_size,
            .access     = ALT_MMU_AP_PRIV_ACCESS,
            .attributes = ALT_MMU_ATTR_DEVICE_NS,
            .shareable  = ALT_MMU_TTB_S_NON_SHAREABLE,
            .execute    = ALT_MMU_TTB_XN_DISABLE,
            .security   = ALT_MMU_TTB_NS_SECURE
        };
    }

    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    uint32_t * ttb1 = NULL;

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_init();
    }

    if (status == ALT_E_SUCCESS)
    {
        size_t reqsize = alt_mmu_va_space_storage_required(regions, next_region);
        if (reqsize > sizeof(alt_pt_storage))
        {
            status = ALT_E_ERROR;
        }
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_space_create(&ttb1, regions, next_region, alt_pt_alloc, alt_pt_storage);
    }

    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_va_space_enable(ttb1);
    }

    return status;
}

/******************************************************************************/
/*!
 * Cleanup MMU page tables
 *
 * \return      result of the function
 */
ALT_STATUS_CODE alt_pt_uninit(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    
    if (status == ALT_E_SUCCESS)
    {
        status = alt_mmu_disable();
    }

    return status;
}
