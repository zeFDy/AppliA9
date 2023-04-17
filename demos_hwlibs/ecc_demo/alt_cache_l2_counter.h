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

#ifndef ___ALT_CACHE_L2_COUNTER_H___
#define ___ALT_CACHE_L2_COUNTER_H___

#include "hwlib.h"

enum ALT_CACHE_L2_COUNTER_ID_e
{
    ALT_CACHE_L2_COUNTER_0 = 0,
    ALT_CACHE_L2_COUNTER_1 = 1
};
typedef enum ALT_CACHE_L2_COUNTER_ID_e ALT_CACHE_L2_COUNTER_ID_t;

enum ALT_CACHE_L2_COUNTER_EVENT_e
{
    ALT_CACHE_L2_COUNTER_EVENT_DISABLED = 0x0,
    ALT_CACHE_L2_COUNTER_EVENT_CO       = 0x1,
    ALT_CACHE_L2_COUNTER_EVENT_DRHIT    = 0x2,
    ALT_CACHE_L2_COUNTER_EVENT_DRREQ    = 0x3,
    ALT_CACHE_L2_COUNTER_EVENT_DWHIT    = 0x4,
    ALT_CACHE_L2_COUNTER_EVENT_DWREQ    = 0x5,
    ALT_CACHE_L2_COUNTER_EVENT_DWTREQ   = 0x6,
    ALT_CACHE_L2_COUNTER_EVENT_IRHIT    = 0x7,
    ALT_CACHE_L2_COUNTER_EVENT_IRREQ    = 0x8,
    ALT_CACHE_L2_COUNTER_EVENT_WA       = 0x9,
    ALT_CACHE_L2_COUNTER_EVENT_IPFALLOC = 0xa,
    ALT_CACHE_L2_COUNTER_EVENT_EPFHIT   = 0xb,
    ALT_CACHE_L2_COUNTER_EVENT_EPFALLOC = 0xc,
    ALT_CACHE_L2_COUNTER_EVENT_SRRCVD   = 0xd,
    ALT_CACHE_L2_COUNTER_EVENT_SRCONF   = 0xe,
    ALT_CACHE_L2_COUNTER_EVENT_EPFRCVD  = 0xf
};
typedef enum ALT_CACHE_L2_COUNTER_EVENT_e ALT_CACHE_L2_COUNTER_EVENT_t;

enum ALT_CACHE_L2_COUNTER_INT_e
{
    ALT_CACHE_L2_COUNTER_INT_DISABLED,
    ALT_CACHE_L2_COUNTER_INT_INCREMENT,
    ALT_CACHE_L2_COUNTER_INT_OVERFLOW,
    ALT_CACHE_L2_COUNTER_INT_MASKED
};
typedef enum ALT_CACHE_L2_COUNTER_INT_e ALT_CACHE_L2_COUNTER_INT_t;

ALT_STATUS_CODE alt_cache_l2_counter_init(void);
ALT_STATUS_CODE alt_cache_l2_counter_uninit(void);

ALT_STATUS_CODE alt_cache_l2_counter_enable(ALT_CACHE_L2_COUNTER_ID_t cid,
                                            ALT_CACHE_L2_COUNTER_EVENT_t eid, ALT_CACHE_L2_COUNTER_INT_t iid);
ALT_STATUS_CODE alt_cache_l2_counter_disable(ALT_CACHE_L2_COUNTER_ID_t cid);

ALT_STATUS_CODE alt_cache_l2_counter_reset(ALT_CACHE_L2_COUNTER_ID_t cid);
ALT_STATUS_CODE alt_cache_l2_counter_reset_all(void);

ALT_STATUS_CODE alt_cache_l2_counter_value_get(ALT_CACHE_L2_COUNTER_ID_t cid, uint32_t * value);
ALT_STATUS_CODE alt_cache_l2_counter_value_set(ALT_CACHE_L2_COUNTER_ID_t cid, uint32_t value);

#endif // ___ALT_CACHE_L2_COUNTER_H___
