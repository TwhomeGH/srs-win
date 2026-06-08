// Minimal stub implementations of FFmpeg API functions used by SRS.
// These allow the SRS binary to link without requiring the full FFmpeg libraries.
// Runtime calls to these functions will fail gracefully (return NULL/error).

// MSVC-specific workaround - disable C++ header inclusion for C compilation
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4005) // disable macro redefinition warnings
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

// MSVC: snprintf is _snprintf in the global namespace
#if defined(_MSC_VER) && !defined(snprintf)
#define snprintf _snprintf
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Type declarations matching ffmpeg-4-fit headers
typedef struct AVCodecContext AVCodecContext;
typedef struct AVCodec AVCodec;
typedef struct AVFrame AVFrame;
typedef struct AVPacket AVPacket;
typedef struct AVAudioFifo AVAudioFifo;
typedef struct SwrContext SwrContext;
typedef struct AVDictionary AVDictionary;

enum AVSampleFormat {
    AV_SAMPLE_FMT_NONE = -1,
    AV_SAMPLE_FMT_U8,
    AV_SAMPLE_FMT_S16,
    AV_SAMPLE_FMT_S32,
    AV_SAMPLE_FMT_FLT,
    AV_SAMPLE_FMT_DBL,
};

// libavutil/error.h
int av_strerror(int errnum, char *errbuf, size_t errbuf_size)
{
    if (errbuf && errbuf_size > 0) {
        snprintf(errbuf, errbuf_size, "Error %d", errnum);
    }
    return 0;
}

// libavutil/mem.h
void av_freep(void *ptr)
{
    if (ptr) {
        void **p = (void **)ptr;
        if (*p) {
            free(*p);
            *p = NULL;
        }
    }
}

// libavutil/mathematics.h
int64_t av_rescale(int64_t a, int64_t b, int64_t c)
{
    // Simplified rescale (no overflow handling)
    if (c == 0) return 0;
    return (int64_t)((double)a * b / c);
}

// libavutil/log.h
static int srs_av_log_level = 0;
void av_log_set_level(int level)
{
    srs_av_log_level = level;
}
void av_log_set_callback(void (*callback)(void*, int, const char*, va_list))
{
    (void)callback;
}

// libavutil/samplefmt.h
int av_samples_alloc(uint8_t **audio_data, int *linesize, int nb_channels,
                     int nb_samples, enum AVSampleFormat sample_fmt, int align)
{
    int sample_size = 2; // default S16
    int total_size = nb_channels * nb_samples * sample_size;
    *audio_data = (uint8_t *)malloc(total_size);
    if (!*audio_data) return -1;
    *linesize = nb_samples * sample_size;
    return total_size;
}

// libavutil/channel_layout.h
int64_t av_get_default_channel_layout(int nb_channels)
{
    // Simplified: return mask for common configs
    switch (nb_channels) {
        case 1: return 4;      // mono
        case 2: return 3;      // stereo
        default: return (1 << nb_channels) - 1;
    }
}

// libavutil/frame.h
AVFrame *av_frame_alloc(void)
{
    return (AVFrame *)calloc(1, 128); // rough allocation
}
void av_frame_free(AVFrame **frame)
{
    if (frame && *frame) {
        free(*frame);
        *frame = NULL;
    }
}
int av_frame_get_buffer(AVFrame *frame, int align)
{
    (void)frame; (void)align;
    return 0;
}
int av_frame_make_writable(AVFrame *frame)
{
    (void)frame;
    return 0;
}

// libavcodec/avcodec.h
AVCodecContext *avcodec_alloc_context3(const AVCodec *codec)
{
    (void)codec;
    return (AVCodecContext *)calloc(1, 256);
}
void avcodec_free_context(AVCodecContext **avctx)
{
    if (avctx && *avctx) {
        free(*avctx);
        *avctx = NULL;
    }
}
int avcodec_open2(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options)
{
    (void)avctx; (void)codec; (void)options;
    return -1; // stub: no real codec support
}

AVPacket *av_packet_alloc(void)
{
    return (AVPacket *)calloc(1, 64);
}
void av_packet_free(AVPacket **pkt)
{
    if (pkt && *pkt) {
        free(*pkt);
        *pkt = NULL;
    }
}
void av_init_packet(AVPacket *pkt)
{
    if (pkt) memset(pkt, 0, 64);
}

AVCodec *avcodec_find_decoder_by_name(const char *name)
{
    (void)name;
    return NULL; // stub
}
AVCodec *avcodec_find_encoder_by_name(const char *name)
{
    (void)name;
    return NULL; // stub
}

int avcodec_send_packet(AVCodecContext *avctx, const AVPacket *avpkt)
{
    (void)avctx; (void)avpkt;
    return -1;
}
int avcodec_receive_frame(AVCodecContext *avctx, AVFrame *frame)
{
    (void)avctx; (void)frame;
    return -1;
}
int avcodec_send_frame(AVCodecContext *avctx, const AVFrame *frame)
{
    (void)avctx; (void)frame;
    return -1;
}
int avcodec_receive_packet(AVCodecContext *avctx, AVPacket *avpkt)
{
    (void)avctx; (void)avpkt;
    return -1;
}

// libavutil/audio_fifo.h
void av_audio_fifo_free(AVAudioFifo *af)
{
    if (af) free(af);
}
AVAudioFifo *av_audio_fifo_alloc(enum AVSampleFormat sample_fmt, int channels,
                                 int nb_samples)
{
    (void)sample_fmt; (void)channels; (void)nb_samples;
    return (AVAudioFifo *)calloc(1, 64);
}
int av_audio_fifo_realloc(AVAudioFifo *af, int nb_samples)
{
    (void)af; (void)nb_samples;
    return 0;
}
int av_audio_fifo_write(AVAudioFifo *af, void **data, int nb_samples)
{
    (void)af; (void)data;
    return nb_samples;
}
int av_audio_fifo_read(AVAudioFifo *af, void **data, int nb_samples)
{
    (void)af; (void)data;
    return nb_samples;
}
int av_audio_fifo_size(AVAudioFifo *af)
{
    (void)af;
    return 0;
}

// libswresample/swresample.h
int swr_init(struct SwrContext *s)
{
    (void)s;
    return 0;
}
struct SwrContext *swr_alloc_set_opts(struct SwrContext *s,
                                      int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
                                      int64_t in_ch_layout, enum AVSampleFormat in_sample_fmt, int in_sample_rate,
                                      int log_offset, void *log_ctx)
{
    (void)s; (void)out_ch_layout; (void)out_sample_fmt; (void)out_sample_rate;
    (void)in_ch_layout; (void)in_sample_fmt; (void)in_sample_rate;
    (void)log_offset; (void)log_ctx;
    return (struct SwrContext *)calloc(1, 64);
}
void swr_free(struct SwrContext **s)
{
    if (s && *s) {
        free(*s);
        *s = NULL;
    }
}
int swr_convert(struct SwrContext *s, uint8_t **out, int out_count,
                const uint8_t **in, int in_count)
{
    (void)s; (void)out; (void)out_count; (void)in; (void)in_count;
    return 0;
}
int swr_get_out_samples(struct SwrContext *s, int in_samples)
{
    (void)s;
    return in_samples; // assume 1:1 ratio as fallback
}
