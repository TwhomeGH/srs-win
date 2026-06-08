/*
 * copyright (c) 2011 Michael Niedermayer
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef LIBSWRESAMPLE_SWRESAMPLE_H
#define LIBSWRESAMPLE_SWRESAMPLE_H

#include <stdint.h>
#include "libavutil/avutil.h"
#include "libavutil/channel_layout.h"
#include "libavutil/frame.h"
#include "libavutil/log.h"
#include "libavutil/samplefmt.h"

struct SwrContext;

typedef struct SwrContext SwrContext;

/**
 * Allocate SwrContext.
 */
struct SwrContext *swr_alloc(void);

/**
 * Initialize swr context.
 */
int swr_init(struct SwrContext *s);

/**
 * Allocate and set options for swr context (FFmpeg 4.x API).
 */
struct SwrContext *swr_alloc_set_opts(struct SwrContext *s,
                                      int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
                                      int64_t in_ch_layout, enum AVSampleFormat in_sample_fmt, int in_sample_rate,
                                      int log_offset, void *log_ctx);

/**
 * Free swr context.
 */
void swr_free(struct SwrContext **s);

/**
 * Convert audio samples.
 */
int swr_convert(struct SwrContext *s, uint8_t **out, int out_count,
                const uint8_t **in, int in_count);

/**
 * FFmpeg 5.x API shim: swr_alloc_set_opts2 using AVChannelLayout.
 * Converts AVChannelLayout to uint64_t channel mask and delegates to swr_alloc_set_opts.
 */
static inline int swr_alloc_set_opts2(struct SwrContext **s,
                                       const AVChannelLayout *out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
                                       const AVChannelLayout *in_ch_layout, enum AVSampleFormat in_sample_fmt, int in_sample_rate,
                                       int log_offset, void *log_ctx)
{
    int64_t out_ch_layout_mask = 0;
    int64_t in_ch_layout_mask = 0;

    if (out_ch_layout && out_ch_layout->order == AV_CHANNEL_ORDER_NATIVE)
        out_ch_layout_mask = out_ch_layout->mask;
    if (out_ch_layout && out_ch_layout->order == AV_CHANNEL_ORDER_UNSPEC)
        out_ch_layout_mask = 0;

    if (in_ch_layout && in_ch_layout->order == AV_CHANNEL_ORDER_NATIVE)
        in_ch_layout_mask = in_ch_layout->mask;
    if (in_ch_layout && in_ch_layout->order == AV_CHANNEL_ORDER_UNSPEC)
        in_ch_layout_mask = 0;

    *s = swr_alloc_set_opts(NULL, out_ch_layout_mask, out_sample_fmt, out_sample_rate,
                             in_ch_layout_mask, in_sample_fmt, in_sample_rate,
                             log_offset, log_ctx);
    return *s ? 0 : -1;
}

/**
 * Get the number of output samples that would be generated from a given number of input samples.
 */
int swr_get_out_samples(struct SwrContext *s, int in_samples);

#endif /* LIBSWRESAMPLE_SWRESAMPLE_H */
