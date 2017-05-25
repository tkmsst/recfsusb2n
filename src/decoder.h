/* -*- tab-width: 4; indent-tabs-mode: nil -*- */
#ifndef _DECODER_H_
#define _DECODER_H_

#include <stdint.h>

#ifdef STD_B25

#include <aribb25/arib_std_b25.h>
#include <aribb25/b_cas_card.h>

#else

typedef struct {
    uint8_t *data;
    int32_t  size;
} ARIB_STD_B25_BUFFER;

#endif

#endif
