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

#include "alt_cache_l2_counter.h"
#include "socal/hps.h"
#include "socal/socal.h"

#define ALT_MPUL2_EV_COUNTER_CTRL_OFST    0x200
#define ALT_MPUL2_EV_COUNTER1_CFG_OFST    0x204
#define ALT_MPUL2_EV_COUNTER0_CFG_OFST    0x208
#define ALT_MPUL2_EV_COUNTERx_CFG_OFST(x) 0x208 - (x * 0x4)
#define ALT_MPUL2_EV_COUNTER1_OFST        0x20c
#define ALT_MPUL2_EV_COUNTER0_OFST        0x210
#define ALT_MPUL2_EV_COUNTERx_OFST(x)     0x210 - (x * 0x4)

#define ALT_MPUL2_EV_COUNTER_CTRL_ADDR    ALT_CAST(void *, (ALT_CAST(char *, ALT_MPUL2_ADDR) + ALT_MPUL2_EV_COUNTER_CTRL_OFST))
#define ALT_MPUL2_EV_COUNTER1_CFG_ADDR    ALT_CAST(void *, (ALT_CAST(char *, ALT_MPUL2_ADDR) + ALT_MPUL2_EV_COUNTER1_CFG_OFST))
#define ALT_MPUL2_EV_COUNTER0_CFG_ADDR    ALT_CAST(void *, (ALT_CAST(char *, ALT_MPUL2_ADDR) + ALT_MPUL2_EV_COUNTER0_CFG_OFST))
#define ALT_MPUL2_EV_COUNTERx_CFG_ADDR(x) ALT_CAST(void *, (ALT_CAST(char *, ALT_MPUL2_ADDR) + ALT_MPUL2_EV_COUNTERx_CFG_OFST(x)))
#define ALT_MPUL2_EV_COUNTER1_ADDR        ALT_CAST(void *, (ALT_CAST(char *, ALT_MPUL2_ADDR) + ALT_MPUL2_EV_COUNTER1_OFST))
#define ALT_MPUL2_EV_COUNTER0_ADDR        ALT_CAST(void *, (ALT_CAST(char *, ALT_MPUL2_ADDR) + ALT_MPUL2_EV_COUNTER0_OFST))
#define ALT_MPUL2_EV_COUNTERx_ADDR(x)     ALT_CAST(void *, (ALT_CAST(char *, ALT_MPUL2_ADDR) + ALT_MPUL2_EV_COUNTERx_OFST(x)))


#define ALT_MPUL2_EV_COUNTER_ENABLE_SET_MSK    (1 << 0)
#define ALT_MPUL2_EV_COUNTER_RESET0_SET_MSK    (1 << 1)
#define ALT_MPUL2_EV_COUNTER_RESET1_SET_MSK    (1 << 2)
#define ALT_MPUL2_EV_COUNTER_RESETx_SET_MSK(x) (1 << (1 + (x)))

#define ALT_MPUL2_EV_COUNTERx_CFG_EVENT_VALUE_SET_MSK    (0xf << 2)
#define ALT_MPUL2_EV_COUNTERx_CFG_EVENT_VALUE_SET(value) (((value) << 2) & ALT_MPUL2_EV_COUNTERx_CFG_EVENT_VALUE_SET_MSK)
#define ALT_MPUL2_EV_COUNTERx_CFG_INT_VALUE_SET_MSK      (0x3 << 0)
#define ALT_MPUL2_EV_COUNTERx_CFG_INT_VALUE_SET(value)   (((value) << 0) & ALT_MPUL2_EV_COUNTERx_CFG_INT_VALUE_SET_MSK)

/////

ALT_STATUS_CODE alt_cache_l2_counter_init(void)
{
    alt_write_word(ALT_MPUL2_EV_COUNTER_CTRL_ADDR,
                   ALT_MPUL2_EV_COUNTER_ENABLE_SET_MSK | ALT_MPUL2_EV_COUNTER_RESET0_SET_MSK | ALT_MPUL2_EV_COUNTER_RESET1_SET_MSK);
    return ALT_E_SUCCESS;
}

ALT_STATUS_CODE alt_cache_l2_counter_uninit(void)
{
    alt_write_word(ALT_MPUL2_EV_COUNTER_CTRL_ADDR, 0);
    return ALT_E_SUCCESS;
}

ALT_STATUS_CODE alt_cache_l2_counter_enable(ALT_CACHE_L2_COUNTER_ID_t cid,
                                            ALT_CACHE_L2_COUNTER_EVENT_t eid, ALT_CACHE_L2_COUNTER_INT_t iid)
{
    // Validate eid
    if (cid > 0xf)
    {
        return ALT_E_BAD_ARG;
    }

    // Validate iid
    if (iid > 0x3)
    {
        return ALT_E_BAD_ARG;
    }

    switch (cid)
    {
    case ALT_CACHE_L2_COUNTER_0:
    case ALT_CACHE_L2_COUNTER_1:
        alt_write_word(ALT_MPUL2_EV_COUNTERx_CFG_ADDR(cid),
                       ALT_MPUL2_EV_COUNTERx_CFG_EVENT_VALUE_SET(eid) | ALT_MPUL2_EV_COUNTERx_CFG_INT_VALUE_SET(iid));
        return ALT_E_SUCCESS;
    default:
        return ALT_E_BAD_ARG;
    }
}


ALT_STATUS_CODE alt_cache_l2_counter_disable(ALT_CACHE_L2_COUNTER_ID_t cid)
{
    return alt_cache_l2_counter_enable(cid,
                                       ALT_CACHE_L2_COUNTER_EVENT_DISABLED, ALT_CACHE_L2_COUNTER_INT_DISABLED);
}

ALT_STATUS_CODE alt_cache_l2_counter_reset(ALT_CACHE_L2_COUNTER_ID_t cid)
{
    switch (cid)
    {
    case ALT_CACHE_L2_COUNTER_0:
    case ALT_CACHE_L2_COUNTER_1:
        alt_setbits_word(ALT_MPUL2_EV_COUNTER_CTRL_ADDR, ALT_MPUL2_EV_COUNTER_RESETx_SET_MSK(cid));
        return ALT_E_SUCCESS;
    default:
        return ALT_E_BAD_ARG;
    }
}

ALT_STATUS_CODE alt_cache_l2_counter_reset_all(void)
{
    alt_setbits_word(ALT_MPUL2_EV_COUNTER_CTRL_ADDR,
                     ALT_MPUL2_EV_COUNTER_RESET0_SET_MSK | ALT_MPUL2_EV_COUNTER_RESET1_SET_MSK);
    return ALT_E_SUCCESS;
}

ALT_STATUS_CODE alt_cache_l2_counter_value_get(ALT_CACHE_L2_COUNTER_ID_t cid, uint32_t * value)
{
    switch (cid)
    {
    case ALT_CACHE_L2_COUNTER_0:
    case ALT_CACHE_L2_COUNTER_1:
        *value = alt_read_word(ALT_MPUL2_EV_COUNTERx_ADDR(cid));
        return ALT_E_SUCCESS;
    default:
        return ALT_E_BAD_ARG;
    }
}

ALT_STATUS_CODE alt_cache_l2_counter_value_set(ALT_CACHE_L2_COUNTER_ID_t cid, uint32_t value)
{
    switch (cid)
    {
    case ALT_CACHE_L2_COUNTER_0:
    case ALT_CACHE_L2_COUNTER_1:
        alt_write_word(ALT_MPUL2_EV_COUNTERx_ADDR(cid), value);
        return ALT_E_SUCCESS;
    default:
        return ALT_E_BAD_ARG;
    }
}
